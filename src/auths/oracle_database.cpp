// Copyright (c) 2016  David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/charstring.h>
#include <rudiments/stringbuffer.h>

class SQLRSERVER_DLLSPEC sqlrauth_oracle_database : public sqlrauth {
	public:
		sqlrauth_oracle_database(sqlrservercontroller *cont,
							domnode *parameters);
		const char	*auth(sqlrcredentials *cred);
	private:
		bool		first;
		stringbuffer	lastuser;
		stringbuffer	lastpassword;
};

sqlrauth_oracle_database::sqlrauth_oracle_database(
					sqlrservercontroller *cont,
					domnode *parameters) :
					sqlrauth(cont,parameters) {
	first=true;
}

const char *sqlrauth_oracle_database::auth(sqlrcredentials *cred) {

	// This module only supports the "oracle_clear_password" method
	// (see the sanity check below).  No protocol module SQL Relay
	// ships ever sends that method, so it has no reachable caller
	// today - see oracle_userlist.cpp's note on oracle_clear_password
	// for why it's kept anyway.

	// this module only supports oracle credentials
	if (charstring::compare(cred->getType(),"oracle")) {
		return NULL;
	}

	const char	*user=((sqlroraclecredentials *)cred)->getUser();
	const char	*password=((sqlroraclecredentials *)cred)->getPassword();
	uint64_t	passwordsize=((sqlroraclecredentials *)cred)->
							getPasswordSize();
	const char	*method=((sqlroraclecredentials *)cred)->getMethod();
	const char	*extra=((sqlroraclecredentials *)cred)->getExtra();

	if (getDebug()) {
		debugStart("auth %s",method);
		debugWrite("user: \"%s\"",user);
		stringbuffer	b;
		b.append("password: \"");
		b.safePrint(password,passwordsize);
		b.append("\"");
		debugWrite("%s",b.getString());
		debugWrite("method: \"%s\"",method);
		debugWrite("extra: \"%s\"",extra);
		debugEnd();
	}

	// sanity check on method
	if (charstring::compare(method,"oracle_clear_password")) {
		return NULL;
	}

	// seed lastuser/lastpassword from the original login
	if (first) {
		lastuser.append(cont->getLoginUser());
		lastpassword.append(cont->getLoginPassword());
		first=false;
	}

	// change user if it differs from the one currently logged in
	bool	success=true;
	if ((lastuser.getSize()==0 &&
		lastpassword.getSize()==0) ||
		charstring::compare(lastuser.getString(),user) ||
		charstring::compare(lastpassword.getString(),password)) {

		debugStart("auth");
		debugStart("changing user to %s",user);
		debugEnd();

		// change user
		success=cont->changeUser(user,password);

		// record whether the change succeeded
		lastuser.clear();
		lastpassword.clear();
		if (success) {
			lastuser.append(user);
			lastpassword.append(password);
		}

	} else {

		debugStart("auth");
		debugWrite("already logged in as %s",user);
		debugEnd();
	}
	return (success)?user:NULL;
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrauth *new_sqlrauth_oracle_database(
						sqlrservercontroller *cont,
						domnode *parameters) {
		return new sqlrauth_oracle_database(cont,parameters);
	}
}
