// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/charstring.h>
#include <rudiments/des.h>
#include <rudiments/sensitivevalue.h>

// the salt firebird's legacy_auth hashes every password with
#define FIREBIRD_LEGACY_SALT	"9z"

class SQLRSERVER_DLLSPEC sqlrauth_firebird_connectstrings : public sqlrauth {
	public:
		sqlrauth_firebird_connectstrings(sqlrservercontroller *cont,
							domnode *parameters);
		~sqlrauth_firebird_connectstrings();
		const char	*auth(sqlrcredentials *cred);
	private:
		const char	*userPassword(const char *user,
						const char *password,
						const char *method,
						uint64_t index);
		bool		compare(const char *suppliedpassword,
						const char *validpassword,
						const char *method);
		char		*legacyHash(const char *password);

		const char	**users;
		char		**passwords;
		const char	**passwordencryptions;
		uint64_t	usercount;

		sensitivevalue	passwordvalue;
};

sqlrauth_firebird_connectstrings::sqlrauth_firebird_connectstrings(
					sqlrservercontroller *cont,
					domnode *parameters) :
					sqlrauth(cont,parameters) {

	linkedlist< connectstringcontainer * >	*connectstrings=
				cont->getConfig()->getConnectStringList();

	users=NULL;
	passwords=NULL;
	passwordencryptions=NULL;
	usercount=connectstrings->getCount();
	if (!usercount) {
		return;
	}

	// create an array of users and passwords and store the
	// users and passwords from the configuration in them
	// this is faster than running through the xml over and over
	users=new const char *[usercount];
	passwords=new char *[usercount];
	passwordencryptions=new const char *[usercount];

	passwordvalue.setPath(cont->getConfig()->getPasswordPath());

	uint64_t	i=0;
	for (listnode< connectstringcontainer * > *node=
				connectstrings->getFirst();
				node; node=node->getNext()) {

		users[i]=node->getValue()->
				getConnectStringValue("user");
		passwordvalue.parse(node->getValue()->
				getConnectStringValue("password"));
		passwords[i]=passwordvalue.detachTextValue();

		passwordencryptions[i]=node->getValue()->
						getPasswordEncryption();
		i++;
	}
}

sqlrauth_firebird_connectstrings::~sqlrauth_firebird_connectstrings() {
	delete[] users;
	for (uint64_t i=0; i<usercount; i++) {
		delete[] passwords[i];
	}
	delete[] passwords;
	delete[] passwordencryptions;
}

static const char *supportedmethods[]={
	"firebird_cleartext",
	"firebird_legacy",
	NULL
};

const char *sqlrauth_firebird_connectstrings::auth(sqlrcredentials *cred) {

	// this module only supports firebird credentials
	if (charstring::compare(cred->getType(),"firebird")) {
		return NULL;
	}

	// get the user/password/method from the creds
	const char	*user=
			((sqlrfirebirdcredentials *)cred)->getUser();
	const char	*password=
			((sqlrfirebirdcredentials *)cred)->getPassword();
	const char	*method=
			((sqlrfirebirdcredentials *)cred)->getMethod();

	if (getDebug()) {
		debugStart("auth");
		debugWrite("user: \"%s\"",user);
		debugWrite("method: \"%s\"",method);
		debugEnd();
	}

	// sanity check on method
	// (a dpb that carried no password at all leaves the method NULL)
	if (!charstring::isInSet(method,supportedmethods)) {
		return NULL;
	}

	// run through the user/password arrays...
	for (uint64_t i=0; i<usercount; i++) {
		const char	*result=userPassword(user,password,method,i);
		if (result) {
			return result;
		}
	}
	return NULL;
}

const char *sqlrauth_firebird_connectstrings::userPassword(
						const char *user,
						const char *password,
						const char *method,
						uint64_t index) {

	// bail if the user doesn't match
	if (charstring::compare(user,users[index])) {
		return NULL;
	}

	// if password encryption isn't being used, then compare against
	// the password from the configuration as it stands
	if (!charstring::getLength(passwordencryptions[index])) {
		return (compare(password,passwords[index],method))?user:NULL;
	}

	// get the module
	sqlrpwdenc	*pe=cont->getPasswordEncryptionById(
					passwordencryptions[index]);
	if (!pe) {
		return NULL;
	}

	bool	result=false;
	char	*pwd=NULL;
	if (pe->oneWay()) {

		// One-way encryption can't be used with firebird_legacy.  The
		// legacy hash can only be derived from the password itself,
		// and a one-way module can't recover it from the
		// configuration.  Since fbclient always sends the legacy hash,
		// a firebird listener whose connect string uses a one-way
		// password encryption can't authenticate anyone.
		if (charstring::compare(method,"firebird_cleartext")) {
			if (getDebug()) {
				debugStart("auth");
				debugWrite("one-way password encryption "
						"can't be used with the "
						"%s method",method);
				debugEnd();
			}
			return NULL;
		}

		// encrypt the password that was passed in and compare it
		// to the encrypted password from the configuration
		pwd=pe->encrypt(password);
		result=!charstring::compare(pwd,passwords[index]);

	} else {

		// decrypt the password from the configuration
		// and compare it to the password that was passed in
		pwd=pe->decrypt(passwords[index]);
		result=compare(password,pwd,method);
	}

	// clean up
	delete[] pwd;

	// return the result
	return (result)?user:NULL;
}

bool sqlrauth_firebird_connectstrings::compare(const char *suppliedpassword,
						const char *validpassword,
						const char *method) {

	// firebird_cleartext supplies the password itself,
	// firebird_legacy supplies a hash of it
	char	*expectedpassword=NULL;
	if (!charstring::compare(method,"firebird_legacy")) {
		expectedpassword=legacyHash(validpassword);
	}

	const char	*expected=(expectedpassword)?
					expectedpassword:validpassword;

	if (getDebug()) {
		debugStart("auth compare");
		debugWrite("expected password: \"%s\"",expected);
		debugWrite("supplied password: \"%s\"",suppliedpassword);
		debugEnd();
	}

	bool	result=!charstring::compare(expected,suppliedpassword);

	delete[] expectedpassword;

	return result;
}

char *sqlrauth_firebird_connectstrings::legacyHash(const char *password) {

	// Note that des truncates the password at 8 characters, so
	// "testpassword" and "testpass" hash identically.  That is firebird's
	// legacy_auth behavior, not something introduced here.
	des	d;

	size_t	saltsize=d.getRequiredSaltSize();

	d.setSalt((const byte_t *)FIREBIRD_LEGACY_SALT,saltsize);

	d.append((const byte_t *)password,charstring::getLength(password));

	// the first characters of the hash are the salt,
	// and firebird sends only what follows them
	const char	*hash=(const char *)d.getHash();
	return (charstring::getLength(hash)<saltsize)?
			charstring::duplicate(hash):
			charstring::duplicate(hash+saltsize);
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrauth *new_sqlrauth_firebird_connectstrings(
						sqlrservercontroller *cont,
						domnode *parameters) {
		return new sqlrauth_firebird_connectstrings(cont,parameters);
	}
}
