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
		#include "sqlrloggerdeclarations.cpp"
	}
#endif

class sqlrloggerplugin {
	public:
		sqlrlogger	*lg;
		dynamiclib	*dl;
};

class sqlrloggersprivate {
	friend class sqlrloggers;
	private:
		const char	*_libexecdir;

		singlylinkedlist< sqlrloggerplugin * >	_llist;
};

sqlrloggers::sqlrloggers(sqlrpaths *sqlrpth) {
	debugFunction();
	pvt=new sqlrloggersprivate;
	pvt->_libexecdir=sqlrpth->getLibExecDir();
}

sqlrloggers::~sqlrloggers() {
	debugFunction();
	unload();
	delete pvt;
}

bool sqlrloggers::load(domnode *parameters) {
	debugFunction();

	unload();

	// run through the logger list
	for (domnode *logger=parameters->getFirstTagChild();
		!logger->isNullNode(); logger=logger->getNextTagSibling()) {

		debugPrintf("loading logger ...\n");

		// load logger
		loadLogger(logger);
	}
	return true;
}

void sqlrloggers::unload() {
	debugFunction();
	for (listnode< sqlrloggerplugin * > *node=
						pvt->_llist.getFirst();
						node; node=node->getNext()) {
		sqlrloggerplugin	*sqlrlp=node->getValue();
		delete sqlrlp->lg;
		delete sqlrlp->dl;
		delete sqlrlp;
	}
	pvt->_llist.clear();
}

void sqlrloggers::loadLogger(domnode *logger) {

	debugFunction();

	// ignore non-loggers
	if (charstring::compare(logger->getName(),"logger")) {
		return;
	}

	// get the logger name
	const char	*module=logger->getAttributeValue("module");
	if (!charstring::getLength(module)) {
		// try "file", that's what it used to be called
		module=logger->getAttributeValue("file");
		if (!charstring::getLength(module)) {
			return;
		}
	}

	debugPrintf("loading logger: %s\n",module);

#ifdef SQLRELAY_ENABLE_SHARED
	// load the logger module
	stringbuffer	modulename;
	modulename.append(pvt->_libexecdir);
	modulename.append(SQLR);
	modulename.append("logger_");
	modulename.append(module)->append(".")->append(SQLRELAY_MODULESUFFIX);
	dynamiclib	*dl=new dynamiclib();
	if (!dl->open(modulename.getString(),true,true)) {
		stdoutput.printf("failed to load logger module: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		delete dl;
		return;
	}

	// load the logger itself
	stringbuffer	functionname;
	functionname.append("new_sqlrlogger_")->append(module);
	sqlrlogger *(*newLogger)(domnode *)=
			(sqlrlogger *(*)(domnode *))
				dl->getSymbol(functionname.getString());
	if (!newLogger) {
		stdoutput.printf("failed to load logger: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		delete dl;
		return;
	}
	sqlrlogger	*lg=(*newLogger)(logger);

#else

	dynamiclib	*dl=NULL;
	sqlrlogger	*lg;
	#include "sqlrloggerassignments.cpp"
	{
		lg=NULL;
	}
#endif

	// add the plugin to the list
	sqlrloggerplugin	*sqlrlp=new sqlrloggerplugin;
	sqlrlp->lg=lg;
	sqlrlp->dl=dl;
	pvt->_llist.append(sqlrlp);
}

void sqlrloggers::init(sqlrlistener *sqlrl,
				sqlrserverconnection *sqlrcon) {
	debugFunction();
	for (listnode< sqlrloggerplugin * > *node=
						pvt->_llist.getFirst();
						node; node=node->getNext()) {
		node->getValue()->lg->init(sqlrl,sqlrcon);
	}
}

void sqlrloggers::run(sqlrlistener *sqlrl,
				sqlrserverconnection *sqlrcon,
				sqlrservercursor *sqlrcur,
				sqlrloglevel_t level,
				sqlrevent_t event,
				const char *info) {
	debugFunction();
	for (listnode< sqlrloggerplugin * > *node=
						pvt->_llist.getFirst();
						node; node=node->getNext()) {
		node->getValue()->lg->run(sqlrl,sqlrcon,sqlrcur,
						level,event,info);
	}
}

void sqlrloggers::endTransaction(bool commit) {
	for (listnode< sqlrloggerplugin * > *node=
						pvt->_llist.getFirst();
						node; node=node->getNext()) {
		node->getValue()->lg->endTransaction(commit);
	}
}

void sqlrloggers::endSession() {
	for (listnode< sqlrloggerplugin * > *node=
						pvt->_llist.getFirst();
						node; node=node->getNext()) {
		node->getValue()->lg->endSession();
	}
}
