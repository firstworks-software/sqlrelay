// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

#include <rudiments/process.h>
#include <rudiments/character.h>
#include <rudiments/stdio.h>
//#define DEBUG_MESSAGES
#include <rudiments/debugprint.h>

#include <config.h>

#ifndef SQLRELAY_ENABLE_SHARED
	extern "C" {
		#include "sqlrmoduledatadeclarations.cpp"
	}
#endif

class sqlrdatabaseobject {
	public:
		const char	*database;
		const char	*schema;
		const char	*object;
		const char	*dependency;
};

class sqlrmoduledatasprivate {
	friend class sqlrmoduledatas;
	private:
		bool		_debug;

		singlylinkedlist< sqlrmoduleplugin * >		_mlist;
		dictionary< const char *, sqlrmoduledata * >	_mdict;
};

sqlrmoduledatas::sqlrmoduledatas(sqlrservercontroller *cont) :
						sqlrservermodules(cont) {
	debugFunction();
	pvt=new sqlrmoduledatasprivate;
	pvt->_debug=cont->getConfig()->getDebugModuleDatas();
}

sqlrmoduledatas::~sqlrmoduledatas() {
	debugFunction();
	unload();
	delete pvt;
}

bool sqlrmoduledatas::load(domnode *parameters) {
	debugFunction();

	unload();

	// run through the moduledata list
	for (domnode *moduledata=parameters->getFirstTagChild();
				!moduledata->isNullNode();
				moduledata=moduledata->getNextTagSibling()) {

		if (isModuleDisabled(moduledata)) {
			continue;
		}

		// load moduledata
		loadModuleData(moduledata);
	}

	return true;
}

void sqlrmoduledatas::unload() {
	debugFunction();
	for (listnode< sqlrmoduleplugin * > *node=
						pvt->_mlist.getFirst();
						node; node=node->getNext()) {
		sqlrmoduleplugin	*sqlrmp=node->getValue();
		delete sqlrmp->m;
		delete sqlrmp->dl;
		delete sqlrmp;
	}
	pvt->_mlist.clear();
}

void sqlrmoduledatas::loadModuleData(domnode *moduledata) {
	debugFunction();

	// ignore non-moduledatas
	if (charstring::compare(moduledata->getName(),"moduledata")) {
		return;
	}

	// get the moduledata name
	const char	*module=moduledata->getAttributeValue("module");
	if (!charstring::getLength(module)) {
		// try "file", that's what it used to be called
		module=moduledata->getAttributeValue("file");
		if (!charstring::getLength(module)) {
			return;
		}
	}

	if (pvt->_debug) {
		stdoutput.printf("loading moduledata module: %s\n",module);
	}

#ifdef SQLRELAY_ENABLE_SHARED
	// load the moduledata module
	stringbuffer	modulename;
	modulename.append(cont->getPaths()->getLibExecDir());
	modulename.append(SQLR);
	modulename.append("moduledata_");
	modulename.append(module)->append(".")->append(SQLRELAY_MODULESUFFIX);
	dynamiclib	*dl=new dynamiclib();
	if (!dl->open(modulename.getString(),true,true)) {
		stdoutput.printf("failed to load "
				"moduledata module: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		delete dl;
		return;
	}

	// load the moduledata itself
	stringbuffer	functionname;
	functionname.append("new_sqlrmoduledata_")->append(module);
	sqlrmoduledata *(*newModuleData)(domnode *)=
		(sqlrmoduledata *(*)(domnode *))
				dl->getSymbol(functionname.getString());
	if (!newModuleData) {
		stdoutput.printf("failed to load moduledata: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		dl->close();
		delete dl;
		return;
	}
	sqlrmoduledata	*md=(*newModuleData)(moduledata);

#else
	dynamiclib	*dl=NULL;
	sqlrmoduledata	*md;
	#include "sqlrmoduledataassignments.cpp"
	{
		md=NULL;
	}
#endif

	if (pvt->_debug) {
		stdoutput.printf("success\n");
	}

	// add the plugin to the list
	sqlrmoduleplugin	*sqlmp=new sqlrmoduleplugin;
	sqlmp->m=md;
	sqlmp->dl=dl;
	sqlmp->module=module;
	pvt->_mlist.append(sqlmp);
	pvt->_mdict.setValue(md->getId(),md);
}

sqlrmoduledata *sqlrmoduledatas::getModuleData(const char *id) {
	return pvt->_mdict.getValue(id);
}

void sqlrmoduledatas::closeResultSet(sqlrservercursor *sqlrcur) {
	for (listnode< sqlrmoduleplugin * >
				*node=pvt->_mlist.getFirst();
				node; node=node->getNext()) {
		sqlrmoduledata	*md=(sqlrmoduledata *)node->getValue()->m;
		md->closeResultSet(sqlrcur);
	}
}

void sqlrmoduledatas::endTransaction(bool commit) {
	for (listnode< sqlrmoduleplugin * >
				*node=pvt->_mlist.getFirst();
				node; node=node->getNext()) {
		node->getValue()->m->endTransaction(commit);
	}
}

void sqlrmoduledatas::endSession() {
	for (listnode< sqlrmoduleplugin * >
				*node=pvt->_mlist.getFirst();
				node; node=node->getNext()) {
		node->getValue()->m->endSession();
	}
}
