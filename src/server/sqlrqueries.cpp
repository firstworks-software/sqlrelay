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
		#include "sqlrquerydeclarations.cpp"
	}
#endif

class sqlrqueriesprivate {
	friend class sqlrqueries;
	private:
		singlylinkedlist< sqlrmoduleplugin * >	_llist;
};

sqlrqueries::sqlrqueries(sqlrservercontroller *cont) : sqlrservermodules(cont) {
	debugFunction();
	pvt=new sqlrqueriesprivate;
}

sqlrqueries::~sqlrqueries() {
	debugFunction();
	unload();
	delete pvt;
}

bool sqlrqueries::load(domnode *parameters) {
	debugFunction();

	unload();

	// run through the query list
	for (domnode *query=parameters->getFirstTagChild();
		!query->isNullNode(); query=query->getNextTagSibling()) {

		if (isModuleDisabled(query)) {
			continue;
		}

		debugPrintf("loading query ...\n");

		// load query
		loadQuery(query);
	}
	return true;
}

void sqlrqueries::unload() {
	debugFunction();
	for (listnode< sqlrmoduleplugin * > *node=
						pvt->_llist.getFirst();
						node; node=node->getNext()) {
		sqlrmoduleplugin	*sqlrmp=node->getValue();
		delete sqlrmp->m;
		delete sqlrmp->dl;
		delete sqlrmp;
	}
	pvt->_llist.clear();
}

void sqlrqueries::loadQuery(domnode *query) {

	debugFunction();

	// ignore non-queries
	if (charstring::compare(query->getName(),"query")) {
		return;
	}

	// get the query name
	const char	*module=query->getAttributeValue("module");
	if (!charstring::getLength(module)) {
		// try "file", that's what it used to be called
		module=query->getAttributeValue("file");
		if (!charstring::getLength(module)) {
			return;
		}
	}

	debugPrintf("loading query: %s\n",module);

#ifdef SQLRELAY_ENABLE_SHARED
	// load the query module
	stringbuffer	modulename;
	modulename.append(cont->getPaths()->getLibExecDir());
	modulename.append(SQLR);
	modulename.append("query_");
	modulename.append(module)->append(".")->append(SQLRELAY_MODULESUFFIX);
	dynamiclib	*dl=new dynamiclib();
	if (!dl->open(modulename.getString(),true,true)) {
		stdoutput.printf("failed to load query module: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		delete dl;
		return;
	}

	// load the query itself
	stringbuffer	functionname;
	functionname.append("new_sqlrquery_")->append(module);
	sqlrquery *(*newQuery)(sqlrservercontroller *,
					domnode *)=
			(sqlrquery *(*)(sqlrservercontroller *,
						domnode *))
				dl->getSymbol(functionname.getString());
	if (!newQuery) {
		stdoutput.printf("failed to load query: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		dl->close();
		delete dl;
		return;
	}
	sqlrquery	*qr=(*newQuery)(cont,query);

#else

	dynamiclib	*dl=NULL;
	sqlrquery	*qr;
	#include "sqlrqueryassignments.cpp"
	{
		qr=NULL;
	}
#endif

	// add the plugin to the list
	sqlrmoduleplugin	*sqlrmp=new sqlrmoduleplugin;
	sqlrmp->m=qr;
	sqlrmp->dl=dl;
	sqlrmp->module=module;
	pvt->_llist.append(sqlrmp);
}

sqlrquerycursor *sqlrqueries::match(sqlrserverconnection *sqlrcon,
					const char *querystring,
					uint32_t querysize,
					uint16_t id) {
	debugFunction();
	for (listnode< sqlrmoduleplugin * > *node=
						pvt->_llist.getFirst();
						node; node=node->getNext()) {
		sqlrquery	*qr=(sqlrquery *)node->getValue()->m;
		if (qr->match(querystring,querysize)) {
			return qr->newCursor(sqlrcon,id);
		}
	}
	return NULL;
}

void sqlrqueries::endTransaction(bool commit) {
	for (listnode< sqlrmoduleplugin * > *node=
						pvt->_llist.getFirst();
						node; node=node->getNext()) {
		node->getValue()->m->endTransaction(commit);
	}
}

void sqlrqueries::endSession() {
	for (listnode< sqlrmoduleplugin * > *node=
						pvt->_llist.getFirst();
						node; node=node->getNext()) {
		node->getValue()->m->endSession();
	}
}
