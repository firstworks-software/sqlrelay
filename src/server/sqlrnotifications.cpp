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
		#include "sqlrnotificationdeclarations.cpp"
	}
#endif

class sqlrnotificationplugin {
	public:
		sqlrnotification	*n;
		dynamiclib		*dl;
};

class sqlrnotificationsprivate {
	friend class sqlrnotifications;
	private:
		const char	*_libexecdir;

		singlylinkedlist< sqlrnotificationplugin * >	_llist;
};

sqlrnotifications::sqlrnotifications(sqlrpaths *sqlrpth) {
	debugFunction();
	pvt=new sqlrnotificationsprivate;
	pvt->_libexecdir=sqlrpth->getLibExecDir();
}

sqlrnotifications::~sqlrnotifications() {
	debugFunction();
	unload();
	delete pvt;
}

bool sqlrnotifications::load(domnode *parameters) {
	debugFunction();

	unload();

	// run through the notification list
	for (domnode *notification=parameters->getFirstTagChild();
			!notification->isNullNode();
			notification=notification->getNextTagSibling()) {

		debugPrintf("loading notification ...\n");

		// load notification
		loadNotification(notification);
	}
	return true;
}

void sqlrnotifications::unload() {
	debugFunction();
	for (listnode< sqlrnotificationplugin * > *node=
						pvt->_llist.getFirst();
						node; node=node->getNext()) {
		sqlrnotificationplugin	*sqlrnp=node->getValue();
		delete sqlrnp->n;
		delete sqlrnp->dl;
		delete sqlrnp;
	}
	pvt->_llist.clear();
}

void sqlrnotifications::loadNotification(domnode *notification) {

	debugFunction();

	// ignore non-notifications
	if (charstring::compare(notification->getName(),"notification")) {
		return;
	}

	// get the notification name
	const char	*module=notification->getAttributeValue("module");
	if (!charstring::getLength(module)) {
		// try "file", that's what it used to be called
		module=notification->getAttributeValue("file");
		if (!charstring::getLength(module)) {
			return;
		}
	}

	debugPrintf("loading notification: %s\n",module);

#ifdef SQLRELAY_ENABLE_SHARED
	// load the notification module
	stringbuffer	modulename;
	modulename.append(pvt->_libexecdir);
	modulename.append(SQLR);
	modulename.append("notification_");
	modulename.append(module)->append(".")->append(SQLRELAY_MODULESUFFIX);
	dynamiclib	*dl=new dynamiclib();
	if (!dl->open(modulename.getString(),true,true)) {
		stdoutput.printf(
			"failed to load notification module: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		delete dl;
		return;
	}

	// load the notification itself
	stringbuffer	functionname;
	functionname.append("new_sqlrnotification_")->append(module);
	sqlrnotification *(*newNotification)(domnode *)=
		(sqlrnotification *(*)(domnode *))
				dl->getSymbol(functionname.getString());
	if (!newNotification) {
		stdoutput.printf("failed to load notification: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		dl->close();
		delete dl;
		return;
	}
	sqlrnotification	*n=(*newNotification)(notification);

#else

	dynamiclib	*dl=NULL;
	sqlrnotification	*n;
	#include "sqlrnotificationassignments.cpp"
	{
		n=NULL;
	}
#endif

	// add the plugin to the list
	sqlrnotificationplugin	*sqlrnp=new sqlrnotificationplugin;
	sqlrnp->n=n;
	sqlrnp->dl=dl;
	pvt->_llist.append(sqlrnp);
}

void sqlrnotifications::run(sqlrlistener *sqlrl,
					sqlrserverconnection *sqlrcon,
					sqlrservercursor *sqlrcur,
					sqlrevent_t event,
					const char *info) {
	debugFunction();
	for (listnode< sqlrnotificationplugin * > *node=
						pvt->_llist.getFirst();
						node; node=node->getNext()) {
		node->getValue()->n->run(sqlrl,sqlrcon,sqlrcur,event,info);
	}
}

void sqlrnotifications::endTransaction(bool commit) {
	for (listnode< sqlrnotificationplugin * > *node=
						pvt->_llist.getFirst();
						node; node=node->getNext()) {
		node->getValue()->n->endTransaction(commit);
	}
}

void sqlrnotifications::endSession() {
	for (listnode< sqlrnotificationplugin * > *node=
						pvt->_llist.getFirst();
						node; node=node->getNext()) {
		node->getValue()->n->endSession();
	}
}

