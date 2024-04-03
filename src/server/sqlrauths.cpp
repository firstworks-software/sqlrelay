// Copyright (c) 1999-2018 David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>

#include <rudiments/domnode.h>
#include <rudiments/process.h>
#include <rudiments/stdio.h>

#include <config.h>

#ifndef SQLRELAY_ENABLE_SHARED
	extern "C" {
		#include "sqlrauthdeclarations.cpp"
	}
#endif

class sqlrauthsprivate {
	friend class sqlrauths;
	private:
};

sqlrauths::sqlrauths(sqlrservercontroller *cont) : sqlrservermodules(cont) {
	pvt=new sqlrauthsprivate;
	setDebug(cont->getConfig()->getDebugAuths());
}

sqlrauths::~sqlrauths() {
	unload();
	delete pvt;
}

bool sqlrauths::load(domnode *parameters, sqlrpwdencs *sqlrpe) {

	unload();

	// run through each set of auths
	for (domnode *auth=parameters->getFirstTagChild("auth");
				!auth->isNullNode();
				auth=auth->getNextTagSibling("auth")) {

		if (isModuleDisabled(auth)) {
			continue;
		}

		// load password encryption
		loadAuth(auth,sqlrpe);
	}
	return true;
}

void sqlrauths::loadAuth(domnode *auth, sqlrpwdencs *sqlrpe) {

	// get the auth name
	const char	*module=auth->getAttributeValue("module");
	if (!charstring::getLength(module)) {
		// try "file", that's what it used to be called
		module=auth->getAttributeValue("file");
		if (!charstring::getLength(module)) {
			// fall back to default if no module is specified
			module="default";
		}
	}

	debugWrite("loading auth module: %s",module);

#ifdef SQLRELAY_ENABLE_SHARED
	// load the password encryption module
	stringbuffer	modulename;
	modulename.append(cont->getPaths()->getLibExecDir());
	modulename.append(SQLR);
	modulename.append("auth_");
	modulename.append(module)->append(".")->append(SQLRELAY_MODULESUFFIX);
	dynamiclib	*dl=new dynamiclib();
	if (!dl->open(modulename.getString(),true,true)) {
		stdoutput.printf("failed to load auth module: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		delete dl;
		return;
	}

	// load the password encryption itself
	stringbuffer	functionname;
	functionname.append("new_sqlrauth_")->append(module);
	sqlrauth *(*newAuth)(sqlrservercontroller *,
					sqlrpwdencs *,
					domnode *)=
			(sqlrauth *(*)(sqlrservercontroller *,
					sqlrpwdencs *,
					domnode *))
				dl->getSymbol(functionname.getString());
	if (!newAuth) {
		stdoutput.printf("failed to load auth: %s\n",module);
		char	*error=dl->getError();
		stdoutput.printf("%s\n",(error)?error:"");
		delete[] error;
		dl->close();
		delete dl;
		return;
	}
	sqlrauth	*au=(*newAuth)(cont,sqlrpe,auth);

#else

	dynamiclib	*dl=NULL;
	sqlrauth	*au;
	#include "sqlrauthassignments.cpp"
	{
		au=NULL;
	}
#endif

	debugWrite("success");

	// add the plugin to the list
	sqlrmoduleplugin	*sqlrmp=new sqlrmoduleplugin;
	sqlrmp->m=au;
	sqlrmp->dl=dl;
	sqlrmp->module=module;
	blist.append(sqlrmp);
}

const char *sqlrauths::auth(sqlrcredentials *cred) {
	if (!cred) {
		return NULL;
	}
	for (listnode< sqlrmoduleplugin * > *node=blist.getFirst();
						node; node=node->getNext()) {

		debugWrite("running auth: %s...",node->getValue()->module);
		
		sqlrauth	*m=(sqlrauth *)node->getValue()->m;
		const char	*autheduser=m->auth(cred);
		if (autheduser) {
			return autheduser;
		}
	}
	return NULL;
}
