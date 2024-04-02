// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

#include <rudiments/domnode.h>
#include <rudiments/stdio.h>
//#define DEBUG_MESSAGES 1
#include <rudiments/debugprint.h>

#include <config.h>

#ifndef SQLRELAY_ENABLE_SHARED
	extern "C" {
		#include "sqlrtriggerdeclarations.cpp"
	}
#endif

class sqlrtriggerplugin {
	public:
		sqlrtrigger	*tr;
		dynamiclib	*dl;
};

class sqlrtriggersprivate {
	friend class sqlrtriggers;
	private:
		bool	_debug;

		singlylinkedlist< sqlrtriggerplugin * >	_beforetriggers;
		singlylinkedlist< sqlrtriggerplugin * >	_aftertriggers;
};

sqlrtriggers::sqlrtriggers(sqlrservercontroller *cont) :
					sqlrservermodules(cont) {
	debugFunction();
	pvt=new sqlrtriggersprivate;
	pvt->_debug=cont->getConfig()->getDebugTriggers();
}

sqlrtriggers::~sqlrtriggers() {
	debugFunction();
	unload();
	delete pvt;
}

bool sqlrtriggers::load(domnode *parameters) {
	debugFunction();

	unload();

	// run through the trigger list
	for (domnode *trigger=parameters->getFirstTagChild();
		!trigger->isNullNode(); trigger=trigger->getNextTagSibling()) {

		if (isModuleDisabled(trigger)) {
			continue;
		}

		bool	before=(charstring::contains(
					trigger->getAttributeValue("when"),
					"before") ||
				charstring::contains(
					trigger->getAttributeValue("when"),
					"both"));
		bool	after=(charstring::contains(
					trigger->getAttributeValue("when"),
					"after") ||
				charstring::contains(
					trigger->getAttributeValue("when"),
					"both"));

		// load the trigger
		sqlrtriggerplugin	*p=loadTrigger(trigger);
		if (!p) {
			continue;
		}

		// add trigger to before list
		if (before) {
			if (pvt->_debug) {
				stdoutput.printf("before trigger\n");
			}
			pvt->_beforetriggers.append(p);
		}

		// add trigger to after list
		if (after) {
			if (pvt->_debug) {
				stdoutput.printf("after trigger\n");
			}
			pvt->_aftertriggers.append(p);
		}
	}
	return true;
}

void sqlrtriggers::unload() {
	debugFunction();
	for (listnode< sqlrtriggerplugin * > *bnode=
				pvt->_beforetriggers.getFirst();
					bnode; bnode=bnode->getNext()) {
		sqlrtriggerplugin	*sqlt=bnode->getValue();
		if (!pvt->_aftertriggers.find(sqlt)) {
			delete sqlt->tr;
			delete sqlt->dl;
			delete sqlt;
		}
	}
	pvt->_beforetriggers.clear();
	for (listnode< sqlrtriggerplugin * > *anode=
				pvt->_aftertriggers.getFirst();
					anode; anode=anode->getNext()) {
		sqlrtriggerplugin	*sqlt=anode->getValue();
		delete sqlt->tr;
		delete sqlt->dl;
		delete sqlt;
	}
	pvt->_aftertriggers.clear();
}

sqlrtriggerplugin *sqlrtriggers::loadTrigger(domnode *trigger) {

	debugFunction();

	// ignore non-triggers
	if (charstring::compare(trigger->getName(),"trigger")) {
		return NULL;
	}

	// get the trigger name
	const char	*module=trigger->getAttributeValue("module");
	if (!charstring::getLength(module)) {
		// try "file", that's what it used to be called
		module=trigger->getAttributeValue("file");
		if (!charstring::getLength(module)) {
			return NULL;
		}
	}

	if (pvt->_debug) {
		stdoutput.printf("loading trigger: %s\n",module);
	}

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
		return NULL;
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
		return NULL;
	}
	sqlrtrigger	*tr=(*newTrigger)(cont,trigger);

#else

	dynamiclib	*dl=NULL;
	sqlrtrigger	*tr;
	#include "sqlrtriggerassignments.cpp"
	{
		tr=NULL;
	}
#endif

	if (pvt->_debug) {
		stdoutput.printf("success\n");
	}

	// build and return the plugin
	sqlrtriggerplugin	*sqltp=new sqlrtriggerplugin;
	sqltp->tr=tr;
	sqltp->dl=dl;
	return sqltp;
}

bool sqlrtriggers::runBeforeTriggers(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur) {
	debugFunction();
	return runBefore(sqlrcon,sqlrcur,&pvt->_beforetriggers);
}

bool sqlrtriggers::runAfterTriggers(sqlrserverconnection *sqlrcon,
						sqlrservercursor *sqlrcur) {
	debugFunction();
	return runAfter(sqlrcon,sqlrcur,&pvt->_aftertriggers);
}

bool sqlrtriggers::runBefore(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				singlylinkedlist< sqlrtriggerplugin * > *list) {
	debugFunction();
	for (listnode< sqlrtriggerplugin * > *node=list->getFirst();
						node; node=node->getNext()) {
		if (pvt->_debug) {
			stdoutput.printf("\nrunning before trigger...\n\n");
		}
		if (!node->getValue()->tr->runBefore(sqlrcon,sqlrcur)) {
			return false;
		}
	}
	return true;
}

bool sqlrtriggers::runAfter(sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				singlylinkedlist< sqlrtriggerplugin * > *list) {
	debugFunction();
	for (listnode< sqlrtriggerplugin * > *node=list->getFirst();
						node; node=node->getNext()) {
		if (pvt->_debug) {
			stdoutput.printf("\nrunning after trigger...\n\n");
		}
		if (!node->getValue()->tr->runAfter(sqlrcon,sqlrcur)) {
			return false;
		}
	}
	return true;
}

void sqlrtriggers::endTransaction(bool commit) {
	for (listnode< sqlrtriggerplugin * >
				*node=pvt->_beforetriggers.getFirst();
				node; node=node->getNext()) {
		node->getValue()->tr->endTransaction(commit);
	}
	for (listnode< sqlrtriggerplugin * >
				*node=pvt->_aftertriggers.getFirst();
				node; node=node->getNext()) {
		node->getValue()->tr->endTransaction(commit);
	}
}

void sqlrtriggers::endSession() {
	for (listnode< sqlrtriggerplugin * >
				*node=pvt->_beforetriggers.getFirst();
				node; node=node->getNext()) {
		node->getValue()->tr->endSession();
	}
	for (listnode< sqlrtriggerplugin * >
				*node=pvt->_aftertriggers.getFirst();
				node; node=node->getNext()) {
		node->getValue()->tr->endSession();
	}
}
