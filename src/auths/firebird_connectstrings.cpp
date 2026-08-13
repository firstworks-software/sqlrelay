// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/charstring.h>
#include <rudiments/des.h>
#include <rudiments/parameterstring.h>
#include <rudiments/sensitivevalue.h>

#include "../protocols/firebirdsrp.h"

// the salt firebird's legacy_auth hashes every password with
#define FIREBIRD_LEGACY_SALT	"9z"

// The Srp and Srp256 plugins that firebird's wire protocol 13 and up
// negotiate need two round trips, and the inputs of the second one have to
// survive the first.  They ride in the credentials' "extra" field, as a
// rudiments parameterstring, following the pattern that
// src/auths/oracle_userlist.cpp documents at the top of that file: a wider
// sqlrfirebirdcredentials would relink libsqlrserver.
//
//	challenge(), with method "firebird_srp" or "firebird_srp256":
//		clientpublickey		the client's A, hex
//		serverprivatekey	the b to use, hex
//	  out:	salt			the salt this login got, hex
//		serverpublickey		the B to answer with, hex
//
//	auth(), with method "firebird_srp" or "firebird_srp256":
//		password		the client's proof M1, hex
//		clientpublickey		as above
//		serverprivatekey	as above
//		salt			what challenge() returned
//
// The protocol module supplies b rather than getting it back from
// challenge().  challenge() keeps no state, so the ephemeral private key has
// to be held by the side that owns the session, and sqlrfirebirdsrp exposes
// setServerPrivateKey() but no getter.  Feeding the same b back in at auth()
// time reproduces the same B, and with it the same session key.
//
// Both phases derive the verifier from the password, so the cleartext
// password is required, exactly as firebird_legacy already needs it.

class SQLRSERVER_DLLSPEC sqlrauth_firebird_connectstrings : public sqlrauth {
	public:
		sqlrauth_firebird_connectstrings(sqlrservercontroller *cont,
							domnode *parameters);
		~sqlrauth_firebird_connectstrings();
		const char	*auth(sqlrcredentials *cred);
		bool		challenge(sqlrcredentials *cred,
						stringbuffer *challenge);
	private:
		const char	*userPassword(const char *user,
						const char *password,
						const char *method,
						const char *extra,
						uint64_t index);
		bool		compare(const char *suppliedpassword,
						const char *validpassword,
						const char *method,
						const char *user,
						const char *extra);
		char		*legacyHash(const char *password);
		char		*getClearTextPassword(const char *user);
		bool		srpVerify(const char *user,
						const char *password,
						const char *proof,
						const char *method,
						const char *extra);

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

#define FIREBIRD_CLEARTEXT	"firebird_cleartext"
#define FIREBIRD_LEGACY		"firebird_legacy"
#define FIREBIRD_SRP		"firebird_srp"
#define FIREBIRD_SRP256		"firebird_srp256"

static const char *supportedmethods[]={
	FIREBIRD_CLEARTEXT,
	FIREBIRD_LEGACY,
	FIREBIRD_SRP,
	FIREBIRD_SRP256,
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
	const char	*extra=
			((sqlrfirebirdcredentials *)cred)->getExtra();

	if (getDebug()) {
		debugStart("auth");
		debugWrite("user: \"%s\"",user);
		debugWrite("method: \"%s\"",method);
		debugWrite("extra: \"%s\"",extra);
		debugEnd();
	}

	// sanity check on method
	// (a dpb that carried no password at all leaves the method NULL)
	if (!charstring::isInSet(method,supportedmethods)) {
		return NULL;
	}

	// run through the user/password arrays...
	for (uint64_t i=0; i<usercount; i++) {
		const char	*result=userPassword(user,password,
							method,extra,i);
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
						const char *extra,
						uint64_t index) {

	// bail if the user doesn't match
	if (charstring::compare(user,users[index])) {
		return NULL;
	}

	// if password encryption isn't being used, then compare against
	// the password from the configuration as it stands
	if (!charstring::getLength(passwordencryptions[index])) {
		return (compare(password,passwords[index],
					method,user,extra))?user:NULL;
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

		// One-way encryption can't be used with firebird_legacy or
		// with either srp method.  The legacy hash and the srp
		// verifier can only be derived from the password itself, and
		// a one-way module can't recover it from the configuration.
		if (charstring::compare(method,FIREBIRD_CLEARTEXT)) {
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
		result=compare(password,pwd,method,user,extra);
	}

	// clean up
	delete[] pwd;

	// return the result
	return (result)?user:NULL;
}

bool sqlrauth_firebird_connectstrings::compare(const char *suppliedpassword,
						const char *validpassword,
						const char *method,
						const char *user,
						const char *extra) {

	// the srp methods supply a proof rather than a password, and verifying
	// it takes the whole first round trip back
	if (!charstring::compare(method,FIREBIRD_SRP) ||
			!charstring::compare(method,FIREBIRD_SRP256)) {
		return srpVerify(user,validpassword,
					suppliedpassword,method,extra);
	}

	// firebird_cleartext supplies the password itself,
	// firebird_legacy supplies a hash of it
	char	*expectedpassword=NULL;
	if (!charstring::compare(method,FIREBIRD_LEGACY)) {
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

	// des truncates the password at 8 characters, so "testpassword" and
	// "testpass" hash identically - firebird's legacy_auth does the same
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

char *sqlrauth_firebird_connectstrings::getClearTextPassword(
						const char *user) {

	for (uint64_t i=0; i<usercount; i++) {

		if (charstring::compare(user,users[i])) {
			continue;
		}

		if (!charstring::getLength(passwordencryptions[i])) {
			return charstring::duplicate(passwords[i]);
		}

		sqlrpwdenc	*pe=cont->getPasswordEncryptionById(
						passwordencryptions[i]);

		// a password stored under a one-way password encryption
		// module can't be recovered, so there's no verifier for it
		if (!pe || pe->oneWay()) {
			if (getDebug()) {
				debugStart("challenge");
				debugWrite("one-way password encryption "
						"can't be used with the "
						"srp methods");
				debugEnd();
			}
			return NULL;
		}

		return pe->decrypt(passwords[i]);
	}
	return NULL;
}

static sqlrfirebirdsrphash_t srpHashType(const char *method) {
	return (!charstring::compare(method,FIREBIRD_SRP256))?
			SQLRFIREBIRDSRP_SRP256:SQLRFIREBIRDSRP_SRP;
}

bool sqlrauth_firebird_connectstrings::srpVerify(const char *user,
						const char *password,
						const char *proof,
						const char *method,
						const char *extra) {

	parameterstring	p;
	p.parse(extra);

	// setServerPrivateKey() before generateServerPublicKey() makes the
	// second round reproduce the B, and with it the session key, that the
	// first round sent
	sqlrfirebirdsrp	srp(srpHashType(method));
	bool	result=(srp.setSalt(p.getValue("salt")) &&
			srp.setClientPublicKey(p.getValue("clientpublickey")) &&
			srp.setServerPrivateKey(
					p.getValue("serverprivatekey")) &&
			srp.generateServerPublicKey(user,password) &&
			srp.computeServerSessionKey() &&
			srp.verifyProof(user,proof));

	if (getDebug()) {
		debugStart("auth compare");
		debugWrite("expected proof: \"%s\"",srp.getProof());
		debugWrite("supplied proof: \"%s\"",proof);
		if (!result) {
			debugWrite("srp error: \"%s\"",srp.getError());
		}
		debugEnd();
	}

	return result;
}

bool sqlrauth_firebird_connectstrings::challenge(sqlrcredentials *cred,
						stringbuffer *challenge) {

	// this module only supports firebird credentials
	if (charstring::compare(cred->getType(),"firebird")) {
		return false;
	}

	const char	*user=
			((sqlrfirebirdcredentials *)cred)->getUser();
	const char	*method=
			((sqlrfirebirdcredentials *)cred)->getMethod();
	const char	*extra=
			((sqlrfirebirdcredentials *)cred)->getExtra();

	if (getDebug()) {
		debugStart("challenge");
		debugWrite("user: \"%s\"",user);
		debugWrite("method: \"%s\"",method);
		debugWrite("extra: \"%s\"",extra);
		debugEnd();
	}

	// the srp methods are the only things this builds
	if (charstring::compare(method,FIREBIRD_SRP) &&
			charstring::compare(method,FIREBIRD_SRP256)) {
		return false;
	}

	// the verifier is derived from the password, so the cleartext
	// password is required
	char	*validpassword=getClearTextPassword(user);
	if (!validpassword) {
		return false;
	}

	parameterstring	p;
	p.parse(extra);

	sqlrfirebirdsrp	srp(srpHashType(method));
	bool	result=(srp.setClientPublicKey(p.getValue("clientpublickey")) &&
			srp.setServerPrivateKey(
					p.getValue("serverprivatekey")) &&
			srp.generateSalt() &&
			srp.generateServerPublicKey(user,validpassword));

	if (result) {
		challenge->append("salt=")->append(srp.getSalt());
		challenge->append(";serverpublickey=")->
					append(srp.getServerPublicKey());
	} else if (getDebug()) {
		debugStart("challenge");
		debugWrite("srp error: \"%s\"",srp.getError());
		debugEnd();
	}

	delete[] validpassword;

	return result;
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrauth *new_sqlrauth_firebird_connectstrings(
						sqlrservercontroller *cont,
						domnode *parameters) {
		return new sqlrauth_firebird_connectstrings(cont,parameters);
	}
}
