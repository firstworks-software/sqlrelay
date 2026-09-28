// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/charstring.h>
#include <rudiments/sensitivevalue.h>

class SQLRSERVER_DLLSPEC sqlrauth_tds_userlist : public sqlrauth {
	public:
		sqlrauth_tds_userlist(sqlrservercontroller *cont,
							domnode *parameters);
		~sqlrauth_tds_userlist();
		const char	*auth(sqlrcredentials *cred);
	private:
		const char	*userPassword(const char *user,
						const char *password,
						uint64_t index);
		const char	**users;
		char		**passwords;
		const char	**passwordencryptions;
		uint64_t	usercount;

		sensitivevalue	passwordvalue;
};

sqlrauth_tds_userlist::sqlrauth_tds_userlist(
					sqlrservercontroller *cont,
					domnode *parameters) :
					sqlrauth(cont,parameters) {

	users=NULL;
	passwords=NULL;
	passwordencryptions=NULL;
	usercount=parameters->getChildCount();
	if (!usercount) {
		return;
	}

	// cache users/passwords from the config; faster than
	// walking the xml repeatedly
	users=new const char *[usercount];
	passwords=new char *[usercount];
	passwordencryptions=new const char *[usercount];

	passwordvalue.setPath(cont->getConfig()->getPasswordPath());

	domnode *user=parameters->getFirstTagChild("user");
	for (uint64_t i=0; i<usercount; i++) {

		users[i]=user->getAttributeValue("user");
		passwordvalue.parse(user->getAttributeValue("password"));
		passwords[i]=passwordvalue.detachTextValue();

		// support modern "passwordencryptionid" and fall back to
		// older "passwordencryption" attribute
		const char	*pwdencid=
				user->getAttributeValue("passwordencryptionid");
		if (!pwdencid) {
			pwdencid=user->getAttributeValue("passwordencryption");
		}
		passwordencryptions[i]=pwdencid;

		user=user->getNextTagSibling("user");
	}
}

sqlrauth_tds_userlist::~sqlrauth_tds_userlist() {
	delete[] users;
	for (uint64_t i=0; i<usercount; i++) {
		delete[] passwords[i];
	}
	delete[] passwords;
	delete[] passwordencryptions;
}

const char *sqlrauth_tds_userlist::auth(sqlrcredentials *cred) {

	// this module only supports user/password credentials
	if (charstring::compare(cred->getType(),"userpassword")) {
		return NULL;
	}

	// get the user/password from the creds
	const char	*user=
			((sqlruserpasswordcredentials *)cred)->getUser();
	const char	*password=
			((sqlruserpasswordcredentials *)cred)->getPassword();

	// run through the user/password arrays...
	for (uint64_t i=0; i<usercount; i++) {
		const char	*result=userPassword(user,password,i);
		if (result) {
			return result;
		}
	}
	return NULL;
}

const char *sqlrauth_tds_userlist::userPassword(
						const char *user,
						const char *password,
						uint64_t index) {

	// bail if the user doesn't match
	if (charstring::compare(user,users[index])) {
		return NULL;
	}

	// if password encryption is being used...
	if (charstring::getLength(passwordencryptions[index])) {

		// get the module
		sqlrpwdenc	*pe=cont->getPasswordEncryptionById(
						passwordencryptions[index]);
		if (!pe) {
			return NULL;
		}

		bool	result=false;
		char	*pwd=NULL;
		if (pe->oneWay()) {

			// encrypt the password that was passed in
			pwd=pe->encrypt(password);

			// compare to the encrypted password
			// from the configuration
			result=!charstring::compare(pwd,passwords[index]);

		} else {

			// decrypt the password from the configuration
			pwd=pe->decrypt(passwords[index]);

			// compare to the password that was passed in
			result=!charstring::compare(password,pwd);
		}

		// clean up
		delete[] pwd;

		return (result)?user:NULL;
	}

	// no encryption: return user if passwords match
	return (!charstring::compare(password,passwords[index]))?user:NULL;
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrauth *new_sqlrauth_tds_userlist(
						sqlrservercontroller *cont,
						domnode *parameters) {
		return new sqlrauth_tds_userlist(cont,parameters);
	}
}
