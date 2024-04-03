// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

#include <rudiments/domnode.h>
#include <rudiments/stdio.h>

#include <config.h>

#ifndef SQLRELAY_ENABLE_SHARED
	extern "C" {
		#include "sqlrquerydeclarations.cpp"
	}
#endif

class sqlrqueriesprivate {
	friend class sqlrqueries;
	private:
};

sqlrqueries::sqlrqueries(sqlrservercontroller *cont) : sqlrservermodules(cont) {
	pvt=new sqlrqueriesprivate;
}

sqlrqueries::~sqlrqueries() {
	unload();
	delete pvt;
}

bool sqlrqueries::load(domnode *parameters) {

	unload();

	// run through the query list
	for (domnode *query=parameters->getFirstTagChild();
		!query->isNullNode(); query=query->getNextTagSibling()) {

		if (isModuleDisabled(query)) {
			continue;
		}

		// load query
		loadQuery(query);
	}
	return true;
}

void sqlrqueries::loadQuery(domnode *query) {

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

	debugWrite("loading query module: %s",module);

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

	debugWrite("success");

	// add the plugin to the list
	sqlrmoduleplugin	*sqlrmp=new sqlrmoduleplugin;
	sqlrmp->m=qr;
	sqlrmp->dl=dl;
	sqlrmp->module=module;
	blist.append(sqlrmp);
}

sqlrquerycursor *sqlrqueries::match(sqlrserverconnection *sqlrcon,
					const char *querystring,
					uint32_t querysize,
					uint16_t id) {

	for (listnode< sqlrmoduleplugin * > *node=blist.getFirst();
						node; node=node->getNext()) {

		debugWrite("matching against query: %s...",
					node->getValue()->module);

		sqlrquery	*qr=(sqlrquery *)node->getValue()->m;
		if (qr->match(querystring,querysize)) {
			return qr->newCursor(sqlrcon,id);
		}
	}
	return NULL;
}
