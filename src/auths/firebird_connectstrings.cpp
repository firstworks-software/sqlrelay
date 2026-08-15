// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrserver.h>
#include <rudiments/character.h>
#include <rudiments/charstring.h>
#include <rudiments/des.h>
#include <rudiments/stringbuffer.h>
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
// The srp session key goes the other way, and "extra" is an input the
// protocol module owns, so it rides back on the credentials themselves -
// sqlrfirebirdcredentials::setSessionKey(), which srpVerify() calls once the
// proof checks out.  Firebird's wire encryption keys its cipher with it, and
// nothing else here can produce it.
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
		const char	*userPassword(sqlrfirebirdcredentials *cred,
						const char *user,
						const char *password,
						const char *method,
						const char *extra,
						uint64_t index);
		bool		compare(sqlrfirebirdcredentials *cred,
						const char *suppliedpassword,
						const char *validpassword,
						const char *method,
						const char *user,
						const char *extra);
		char		*legacyHash(const char *password);
		char		*getClearTextPassword(const char *user);
		bool		srpVerify(sqlrfirebirdcredentials *cred,
						const char *user,
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

	// cache the config's users/passwords in arrays instead of
	// rereading the xml every time
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

// fb_utils::sqlSymbolChar() - utils.cpp:1560-1565
static bool accountSymbolChar(char c, bool first) {
	if (c&0x80) {
		return false;
	}
	return ((character::isDigit(c) && !first) ||
			character::isAlphabetical(c) ||
			c=='_' || c=='$');
}

// The login the srp math hashes is not the login as typed.  Both sides run
// it through firebird's login rules first - fb_utils::dpbItemUpper(),
// utils.cpp:1567-1611 - the client in ClntAuthBlock::loadClnt()
// (interface.cpp:10272-10281) before SrpClient::authenticate() hashes
// cb->getLogin() (SrpClient.cpp:149,152), and the server in
// SrvAuthBlock::load() (server.cpp:7520-7524) as it reads CNCT_login.
//
// The rules:
//	* a quoted login loses its quotes, and doubled quotes inside it
//	  collapse to one.  It is upcased only if it was single-quoted and
//	  what was inside was all sql symbol characters.
//	* an unquoted login is upcased, unless it has anything in it but sql
//	  symbol characters, in which case it is left exactly as it came in
//	  (which is also what firebird does with a utf-8 login)
//	* sql symbol characters are the ascii letters, _, $, and the digits
//	  in any but the first position, so the upcasing is always ascii
//
// Malformed quoting raises isc_quoted_str_bad/isc_quoted_str_miss inside
// firebird.  Here the login is just left alone, and the proof fails on its
// own.
//
// Returns a new string in every case.
static char *accountName(const char *login) {

	size_t	len=charstring::getLength(login);

	// quoted login - strip the quotes
	if (len && (login[0]=='"' || login[0]=='\'')) {

		char		endquote=login[0];
		bool		ascii=true;
		stringbuffer	buf;

		for (size_t i=1; i<len; i++) {

			if (login[i]==endquote) {

				if (++i>=len) {
					char	*account=buf.detachString();
					if (ascii && endquote=='\'') {
						charstring::upper(account);
					}
					return account;
				}

				// a quote that isn't doubled ends the login
				// early - it's malformed
				if (login[i]!=endquote) {
					return charstring::duplicate(login);
				}

				// skipped the escape quote, keep going

			} else if (!accountSymbolChar(login[i],i==1)) {
				ascii=false;
			}

			buf.append(login[i]);
		}

		// ran off the end without a closing quote
		return charstring::duplicate(login);
	}

	// unquoted login - upcase it, but only if it's all sql symbol
	// characters
	for (size_t i=0; i<len; i++) {
		if (!accountSymbolChar(login[i],i==0)) {
			return charstring::duplicate(login);
		}
	}

	char	*account=charstring::duplicate(login);
	charstring::upper(account);
	return account;
}

// A firebird login is an sql identifier, so "testuser", "TestUser" and
// TESTUSER are all the same login - the server runs whatever came in through
// the rules above and looks that up (SrvAuthBlock::load(),
// server.cpp:7520-7524, and ServerAuth::ServerAuth(), server.cpp:584-594,
// which also refuses a dpb user name that disagrees with CNCT_login once
// both have been through them).  The configured connect string user plays
// the part of the name a "create user" wrote, which firebird would have run
// through the same rules, so both sides go through them here before being
// compared.
static bool sameAccount(const char *user, const char *configuser) {
	char	*useraccount=accountName(user);
	char	*configaccount=accountName(configuser);
	bool	same=!charstring::compare(useraccount,configaccount);
	delete[] useraccount;
	delete[] configaccount;
	return same;
}

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
		const char	*result=userPassword(
						(sqlrfirebirdcredentials *)cred,
						user,password,
						method,extra,i);
		if (result) {
			return result;
		}
	}
	return NULL;
}

const char *sqlrauth_firebird_connectstrings::userPassword(
						sqlrfirebirdcredentials *cred,
						const char *user,
						const char *password,
						const char *method,
						const char *extra,
						uint64_t index) {

	// bail if the user doesn't match
	if (!sameAccount(user,users[index])) {
		return NULL;
	}

	// no encryption configured, compare directly
	if (!charstring::getLength(passwordencryptions[index])) {
		return (compare(cred,password,passwords[index],
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

		// encrypt and compare
		pwd=pe->encrypt(password);
		result=!charstring::compare(pwd,passwords[index]);

	} else {

		// decrypt and compare
		pwd=pe->decrypt(passwords[index]);
		result=compare(cred,password,pwd,method,user,extra);
	}

	// clean up
	delete[] pwd;

	return (result)?user:NULL;
}

bool sqlrauth_firebird_connectstrings::compare(
						sqlrfirebirdcredentials *cred,
						const char *suppliedpassword,
						const char *validpassword,
						const char *method,
						const char *user,
						const char *extra) {

	// the srp methods supply a proof rather than a password, and verifying
	// it takes the whole first round trip back
	if (!charstring::compare(method,FIREBIRD_SRP) ||
			!charstring::compare(method,FIREBIRD_SRP256)) {
		return srpVerify(cred,user,validpassword,
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

		if (!sameAccount(user,users[i])) {
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

bool sqlrauth_firebird_connectstrings::srpVerify(
						sqlrfirebirdcredentials *cred,
						const char *user,
						const char *password,
						const char *proof,
						const char *method,
						const char *extra) {

	parameterstring	p;
	p.parse(extra);

	// the account the proof was built from, rather than the login as
	// typed - see accountName() above
	char	*account=accountName(user);

	// setServerPrivateKey() before generateServerPublicKey() makes the
	// second round reproduce the B, and with it the session key, that the
	// first round sent
	sqlrfirebirdsrp	srp(srpHashType(method));
	bool	result=(srp.setSalt(p.getValue("salt")) &&
			srp.setClientPublicKey(p.getValue("clientpublickey")) &&
			srp.setServerPrivateKey(
					p.getValue("serverprivatekey")) &&
			srp.generateServerPublicKey(account,password) &&
			srp.computeServerSessionKey() &&
			srp.verifyProof(account,proof));

	delete[] account;

	// The session key the exchange just produced is what firebird's wire
	// encryption keys its cipher with, and the srp object that holds it
	// goes out of scope at the end of this method.  Hand it back through
	// the credentials, which the protocol module reads once auth()
	// returns.
	if (result) {
		cred->setSessionKey(srp.getSessionKey(),
					srp.getSessionKeySize());
	}

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

	// the verifier has to be built from the same account the client will
	// build its proof from - see accountName() above
	char	*account=accountName(user);

	sqlrfirebirdsrp	srp(srpHashType(method));
	bool	result=(srp.setClientPublicKey(p.getValue("clientpublickey")) &&
			srp.setServerPrivateKey(
					p.getValue("serverprivatekey")) &&
			srp.generateSalt() &&
			srp.generateServerPublicKey(account,validpassword));

	delete[] account;

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
