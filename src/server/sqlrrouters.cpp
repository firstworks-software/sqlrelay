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
		#include "sqlrrouterdeclarations.cpp"
	}
#endif

class sqlrroutersprivate {
	friend class sqlrrouters;
	private:
		const char		*_connid;
		const char		**_connids;
		sqlrconnection		**_conns;
		uint16_t		_conncount;

		singlylinkedlist< sqlrmoduleplugin * >	_llist;
};

sqlrrouters::sqlrrouters(sqlrservercontroller *cont,
				const char **connectionids,
				sqlrconnection **connections,
				uint16_t connectioncount) :
				sqlrservermodules(cont) {
	debugFunction();
	pvt=new sqlrroutersprivate;
	pvt->_connid=NULL;
	pvt->_connids=connectionids;
	pvt->_conns=connections;
	pvt->_conncount=connectioncount;
}

sqlrrouters::~sqlrrouters() {
	debugFunction();
	unload();
	delete pvt;
}

bool sqlrrouters::load(domnode *parameters) {
	debugFunction();

	unload();

	// run through the router list
	for (domnode *router=parameters->getFirstTagChild();
			!router->isNullNode();
			router=router->getNextTagSibling()) {

		if (isModuleDisabled(router)) {
			continue;
		}

		// load router
		loadRouter(router);
	}
	return true;
}

void sqlrrouters::unload() {
	debugFunction();
	for (listnode< sqlrmoduleplugin * > *node=pvt->_llist.getFirst();
						node; node=node->getNext()) {
		sqlrmoduleplugin	*sqlrmp=node->getValue();
		delete sqlrmp->m;
		delete sqlrmp->dl;
		delete sqlrmp;
	}
	pvt->_llist.clear();
}

void sqlrrouters::loadRouter(domnode *router) {

	debugFunction();

	// ignore non-routers
	if (charstring::compare(router->getName(),"router")) {
		return;
	}

	// get the router name
	const char	*module=router->getAttributeValue("module");
	if (!charstring::getLength(module)) {
		// try "file", that's what it used to be called
		module=router->getAttributeValue("file");
		if (!charstring::getLength(module)) {
			return;
		}
	}

	if (cont->getConfig()->getDebugRouters()) {
		stdoutput.printf("loading router: %s\n",module);
	}

#ifdef SQLRELAY_ENABLE_SHARED
	// load the router module
	stringbuffer	modulename;
	modulename.append(cont->getPaths()->getLibExecDir());
	modulename.append(SQLR);
	modulename.append("router_");
	modulename.append(module)->append(".")->append(SQLRELAY_MODULESUFFIX);
	dynamiclib	*dl=new dynamiclib();
	if (!dl->open(modulename.getString(),true,true)) {
		stdoutput.printf(
			"failed to load router module: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		delete dl;
		return;
	}

	// load the router itself
	stringbuffer	functionname;
	functionname.append("new_sqlrrouter_")->append(module);
	sqlrrouter *(*newRouter)(sqlrservercontroller *,
					sqlrrouters *,
					domnode *)=
			(sqlrrouter *(*)(sqlrservercontroller *,
						sqlrrouters *,
						domnode *))
				dl->getSymbol(functionname.getString());
	if (!newRouter) {
		stdoutput.printf("failed to load router: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		dl->close();
		delete dl;
		return;
	}
	sqlrrouter	*r=(*newRouter)(cont,this,router);

#else

	dynamiclib	*dl=NULL;
	sqlrrouter	*r;
	#include "sqlrrouterassignments.cpp"
	{
		r=NULL;
	}
#endif

	// add the plugin to the list
	sqlrmoduleplugin	*sqlrmp=new sqlrmoduleplugin;
	sqlrmp->m=r;
	sqlrmp->dl=dl;
	sqlrmp->module=module;
	pvt->_llist.append(sqlrmp);
}

const char *sqlrrouters::route(sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					const char **err,
					int64_t *errn) {
	debugFunction();
	for (listnode< sqlrmoduleplugin * > *node=pvt->_llist.getFirst();
						node; node=node->getNext()) {
		sqlrrouter	*r=(sqlrrouter *)node->getValue()->m;
		const char	*connid=r->route(sqlrcon,sqlrcur,err,errn);
		if (connid) {
			return connid;
		}
	}
	return NULL;
}

bool sqlrrouters::routeEntireSession() {
	debugFunction();
	for (listnode< sqlrmoduleplugin * > *node=pvt->_llist.getFirst();
						node; node=node->getNext()) {
		sqlrrouter	*r=(sqlrrouter *)node->getValue()->m;
		if (!r->routeEntireSession()) {
			return false;
		}
	}
	return true;
}

void sqlrrouters::endTransaction(bool commit) {
	for (listnode< sqlrmoduleplugin * > *node=pvt->_llist.getFirst();
						node; node=node->getNext()) {
		node->getValue()->m->endTransaction(commit);
	}
}

void sqlrrouters::endSession() {
	for (listnode< sqlrmoduleplugin * > *node=pvt->_llist.getFirst();
						node; node=node->getNext()) {
		node->getValue()->m->endSession();
	}
}

void sqlrrouters::setCurrentConnectionId(const char *connid) {
	pvt->_connid=connid;
}

const char *sqlrrouters::getCurrentConnectionId() {
	return pvt->_connid;
}

const char **sqlrrouters::getConnectionIds() {
	return pvt->_connids;
}

sqlrconnection **sqlrrouters::getConnections() {
	return pvt->_conns;
}

uint16_t sqlrrouters::getConnectionCount() {
	return pvt->_conncount;
}
