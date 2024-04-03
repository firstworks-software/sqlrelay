// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

#include <rudiments/stdio.h>
//#define DEBUG_MESSAGES 1
#include <rudiments/debugprint.h>

#include <config.h>

#ifndef SQLRELAY_ENABLE_SHARED
	extern "C" {
		#include "sqlrprotocoldeclarations.cpp"
	}
#endif

class sqlrprotocolsprivate {
	friend class sqlrprotocols;
	private:
		dictionary< uint16_t , sqlrmoduleplugin * >	_protos;
};

sqlrprotocols::sqlrprotocols(sqlrservercontroller *cont) :
						sqlrservermodules(cont) {
	debugFunction();
	pvt=new sqlrprotocolsprivate;
}

sqlrprotocols::~sqlrprotocols() {
	debugFunction();
	unload();
	delete pvt;
}

bool sqlrprotocols::load(domnode *parameters) {
	debugFunction();

	unload();

	// run through the listeners
	uint16_t	i=0;
	for (domnode *listener=parameters->getFirstTagChild();
			!listener->isNullNode();
			listener=listener->getNextTagSibling()) {

		if (isModuleDisabled(listener)) {
			continue;
		}

		debugPrintf("loading protocol ...\n");

		// load protocol
		loadProtocol(i,listener);

		i++;
	}
	return true;
}

void sqlrprotocols::loadProtocol(uint16_t index, domnode *listener) {
	debugFunction();

	// ignore any non-listener entries
	if (charstring::compare(listener->getName(),"listener")) {
		return;
	}

	// get the protocol name
	const char	*module=listener->getAttributeValue("protocol");

	debugPrintf("loading protocol: %s\n",module);

#ifdef SQLRELAY_ENABLE_SHARED
	// load the protocol module
	stringbuffer	modulename;
	modulename.append(cont->getPaths()->getLibExecDir());
	modulename.append(SQLR);
	modulename.append("protocol_");
	modulename.append(module)->append(".")->append(SQLRELAY_MODULESUFFIX);
	dynamiclib	*dl=new dynamiclib();
	if (!dl->open(modulename.getString(),true,true)) {
		stdoutput.printf("failed to load protocol module: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		delete dl;
		return;
	}

	// load the protocol itself
	stringbuffer	functionname;
	functionname.append("new_sqlrprotocol_")->append(module);
	sqlrprotocol *(*newProtocol)(sqlrservercontroller *,
					domnode *)=
			(sqlrprotocol *(*)(sqlrservercontroller *,
						domnode *))
				dl->getSymbol(functionname.getString());
	if (!newProtocol) {
		stdoutput.printf("failed to load protocol: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		dl->close();
		delete dl;
		return;
	}
	sqlrprotocol	*pr=(*newProtocol)(cont,listener);

#else

	dynamiclib	*dl=NULL;
	sqlrprotocol	*pr;
	#include "sqlrprotocolassignments.cpp"
	{
		pr=NULL;
	}
#endif

	// add the plugin to the list
	sqlrmoduleplugin	*sqlrmp=new sqlrmoduleplugin;
	sqlrmp->m=pr;
	sqlrmp->dl=dl;
	blist.append(sqlrmp);
	pvt->_protos.setValue(index,sqlrmp);
}

sqlrprotocol *sqlrprotocols::getProtocol(uint16_t index) {
	debugFunction();
	sqlrmoduleplugin	*pp=NULL;
	if (!pvt->_protos.getValue(index,&pp)) {
		return NULL;
	}
	return (sqlrprotocol *)pp->m;
}
