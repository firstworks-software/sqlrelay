// Copyright (c) David Muse
// See the file COPYING for more information

// The O5LOGON exchange below is the server side of the client side in
// python-oracledb, src/oracledb/impl/thin/messages/auth.pyx and
// src/oracledb/impl/thin/crypto.pyx.
// Copyright (c) 2021, 2025, Oracle and/or its affiliates.
// Taken under the Universal Permissive License 1.0, which is at
// https://oss.oracle.com/licenses/upl, and not under python-oracledb's
// Apache 2.0 option, which isn't compatible with the GPL version 2.
// See COPYING.

#include <sqlrelay/sqlrserver.h>
#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/bytebuffer.h>
#include <rudiments/parameterstring.h>
#include <rudiments/sha1.h>
#include <rudiments/sha512.h>
#include <rudiments/md5.h>
#include <rudiments/aes192.h>
#include <rudiments/aes256.h>
#include <rudiments/pbkdf2.h>
#include <rudiments/sensitivevalue.h>
#include <rudiments/csprng.h>

// rudiments has sha-1, sha-512, md5, aes-192, aes-256, pbkdf2-hmac-sha512
// (#9100) and csprng (#9211), so O5LOGON's sha-512, aes-cbc-with-no-padding
// and session-key/salt needs are all covered - the module no longer touches
// openssl directly.  RUDIMENTS_HAS_SSL is still the right guard for
// SQLRAUTH_ORACLE_O5LOGON, since rudiments' sha512/aes192/aes256 are
// themselves openssl-backed.  Where it's not defined, the module still
// builds and oracle_clear_password still works; O5LOGON just isn't offered.
#if defined(RUDIMENTS_HAS_SSL)
	#define SQLRAUTH_ORACLE_O5LOGON
#endif

// verifier types, from python-oracledb's constants.pxi
#define VERIFIER_TYPE_11G_1	0xb152
#define VERIFIER_TYPE_11G_2	0x1b25
#define VERIFIER_TYPE_12C	0x4815

// session key lengths, which the client tells the two verifier types apart by
#define SESSION_KEY_SIZE_11G	48
#define SESSION_KEY_SIZE_12C	32

// how much of the 11g session key is pkcs#7 padding rather than key material
#define SESSION_KEY_PAD_SIZE_11G	8

// AUTH_SVR_RESPONSE - a 16-byte salt, then this, then a whole block of number
// padding, because 16+16 is already block aligned.  48 bytes for both verifier
// types.
#define SERVER_RESPONSE_PAYLOAD		"SERVER_TO_CLIENT"
#define SERVER_RESPONSE_SIZE		48

// The O5LOGON inputs ride in the credentials' "extra" field, as a rudiments
// parameterstring.  sqlroraclecredentials has 5 fields and O5LOGON needs 8
// inputs on the verify side, and widening it means relinking libsqlrserver.
//
//	challenge(), with method "O5LOGON":
//		verifiertype		45394, 6949 or 18453 (0x prefix ok)
//		authvfrdata		AUTH_VFR_DATA, hex
//		authpbkdf2vgencount	AUTH_PBKDF2_VGEN_COUNT, 12c only
//	  out:	the AUTH_SESSKEY to send, uppercase hex, 96 characters
//		for 11g and 64 for 12c
//
//	auth(), with method "O5LOGON":
//		password		AUTH_PASSWORD from the client, hex
//		verifiertype		as above
//		authvfrdata		as above
//		authpbkdf2vgencount	as above
//		serverauthsesskey	what challenge() returned
//		clientauthsesskey	AUTH_SESSKEY from the client, hex
//		authpbkdf2csksalt	AUTH_PBKDF2_CSK_SALT, hex, 12c only
//		authpbkdf2sdercount	AUTH_PBKDF2_SDER_COUNT, 12c only
//
//	challenge(), with method "O5LOGON-SERVER-RESPONSE":
//		the same 8 inputs auth() was given, minus password
//	  out:	the AUTH_SVR_RESPONSE to send, uppercase hex, 96 characters
//		for both verifier types
//
// serverauthsesskey is the one that's easy to miss.  challenge() generates
// session key part A and keeps no state, so the only way to get part A back at
// verify time is to decrypt the challenge it produced.  The protocol module
// has to hand its own AUTH_SESSKEY back.
//
// AUTH_SVR_RESPONSE proves to the client that the server also knew the
// password, and a real client refuses the login without it.  It rides on
// challenge() rather than on auth() because auth() returns only the
// authenticated user name, and giving it an out parameter means widening
// sqlrcredentials, which relinks libsqlrserver.  Call it after auth() has
// succeeded.

class SQLRSERVER_DLLSPEC sqlrauth_oracle_userlist : public sqlrauth {
	public:
			sqlrauth_oracle_userlist(sqlrservercontroller *cont,
							domnode *parameters);
		const char	*auth(sqlrcredentials *cred);
		bool		challenge(sqlrcredentials *cred,
						stringbuffer *challenge);
	private:
		char		*getClearTextPassword(const char *user);
		bool		compare(const char *suppliedresponse,
					uint64_t suppliedresponsesize,
					const char *validpassword,
					const char *method,
					const char *extra);

		const char	**users;
		const char	**passwords;
		const char	**passwordencryptions;
		uint64_t	usercount;

		sensitivevalue	passwordvalue;
};

sqlrauth_oracle_userlist::sqlrauth_oracle_userlist(
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

	// create an array of users and passwords and store the
	// users and passwords from the configuration in them
	// this is faster than running through the xml over and over
	users=new const char *[usercount];
	passwords=new const char *[usercount];
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

static const char *supportedauthmethods[]={
	#ifdef SQLRAUTH_ORACLE_O5LOGON
	"O5LOGON",
	#endif
	"oracle_clear_password",
	NULL
};

#ifdef SQLRAUTH_ORACLE_O5LOGON

// aes-cbc with a zero iv and no padding, over whole blocks.  rudiments' aes128
// can't do this - wrong key size, and it always pads - but aes192 and aes256
// can, via setUsePadding(false).
static bool aesCbc(bool encrypt,
			const byte_t *key, size_t keysize,
			const byte_t *in, size_t insize,
			byte_t *out) {

	if (insize%16 || (keysize!=24 && keysize!=32)) {
		return false;
	}

	byte_t	iv[16];
	bytestring::zero(iv,sizeof(iv));

	encryption	*enc=(keysize==24)?
				(encryption *)new aes192():
				(encryption *)new aes256();
	enc->setUsePadding(false);

	bool	retval=false;
	if (enc->setKey(key,keysize) && enc->setIv(iv,sizeof(iv)) &&
					enc->append(in,(uint32_t)insize)) {
		const byte_t	*result=(encrypt)?
				enc->getEncryptedData():enc->getDecryptedData();
		uint64_t	resultsize=(encrypt)?
				enc->getEncryptedDataSize():
				enc->getDecryptedDataSize();
		if (result && resultsize==insize) {
			bytestring::copy(out,result,insize);
			retval=true;
		}
	}

	delete enc;

	return retval;
}

// sha-512, via rudiments' sha512 class (#9100)
static bool sha512Hash(const byte_t *in, size_t insize, byte_t *out) {
	sha512	s;
	if (!s.append(in,(uint32_t)insize)) {
		return false;
	}
	const byte_t	*digest=s.getHash();
	if (!digest) {
		return false;
	}
	bytestring::copy(out,digest,64);
	return true;
}

static bool derivedKey(const byte_t *password, uint32_t passwordsize,
			const byte_t *salt, size_t saltsize,
			uint32_t keysize, uint32_t iterations,
			byte_t *out) {

	pbkdf2	p;
	p.setAlgorithm(PBKDF2_ALGORITHM_SHA512);
	p.setIterations(iterations);
	p.setKeySize(keysize);
	if (!p.setSalt(salt,saltsize) || !p.append(password,passwordsize)) {
		return false;
	}

	const byte_t	*key=p.getHash();
	if (!key) {
		return false;
	}

	bytestring::copy(out,key,keysize);

	return true;
}

// password_hash is the key AUTH_SESSKEY is encrypted under.  24 bytes for an
// 11g verifier and 32 for a 12c one.
static bool passwordHash(const char *password,
				uint32_t verifiertype,
				const byte_t *vfrdata, uint64_t vfrdatasize,
				uint32_t vgencount,
				byte_t *passwordhash,
				size_t *passwordhashsize) {

	if (verifiertype==VERIFIER_TYPE_12C) {

		// password_key = pbkdf2-hmac-sha512(password,
		//		vfrdata || "AUTH_PBKDF2_SPEEDY_KEY",
		//		64 bytes, vgencount iterations)
		bytebuffer	salt;
		salt.append(vfrdata,vfrdatasize);
		salt.append("AUTH_PBKDF2_SPEEDY_KEY");
		byte_t		passwordkey[64];
		if (!vgencount ||
			!derivedKey((const byte_t *)password,
					(uint32_t)charstring::getLength(
								password),
					salt.getBuffer(),salt.getSize(),
					(uint32_t)sizeof(passwordkey),
					vgencount,passwordkey)) {
			return false;
		}

		// password_hash = sha512(password_key || vfrdata)[0..31]
		bytebuffer	in;
		in.append(passwordkey,sizeof(passwordkey));
		in.append(vfrdata,vfrdatasize);
		byte_t		digest[64];
		bool		retval=sha512Hash(in.getBuffer(),
						in.getSize(),digest);
		if (retval) {
			bytestring::copy(passwordhash,digest,32);
			*passwordhashsize=32;
		}
		bytestring::zero(passwordkey,sizeof(passwordkey));
		bytestring::zero(digest,sizeof(digest));
		return retval;
	}

	// password_hash = sha1(password || vfrdata) || 4 zero bytes
	sha1	s;
	if (!s.append((const byte_t *)password,
				charstring::getLength(password)) ||
		!s.append(vfrdata,(uint32_t)vfrdatasize)) {
		return false;
	}
	const byte_t	*digest=s.getHash();
	if (!digest) {
		return false;
	}
	bytestring::copy(passwordhash,digest,20);
	bytestring::zero(passwordhash+20,4);
	*passwordhashsize=24;

	return true;
}

// The client tells 11g and 12c apart by the length of the session key rather
// than by the verifier type it was told, so the lengths are load-bearing on
// the wire.
static size_t sessionKeySize(uint32_t verifiertype) {
	return (verifiertype==VERIFIER_TYPE_12C)?
			SESSION_KEY_SIZE_12C:SESSION_KEY_SIZE_11G;
}

static bool supportedVerifierType(uint32_t verifiertype) {
	return (verifiertype==VERIFIER_TYPE_11G_1 ||
		verifiertype==VERIFIER_TYPE_11G_2 ||
		verifiertype==VERIFIER_TYPE_12C);
}

// hex-decodes "value" and requires it to come out "size" bytes long
static bool hexDecodeExactly(const char *value, size_t size, byte_t *out) {

	byte_t		*decoded=NULL;
	uint64_t	decodedsize=0;
	charstring::hexDecode(value,charstring::getLength(value),
						&decoded,&decodedsize);
	bool	retval=(decodedsize==size);
	if (retval) {
		bytestring::copy(out,decoded,size);
	}
	delete[] decoded;

	return retval;
}

static char *hexEncodeUpper(const byte_t *in, uint64_t insize) {
	char	*hex=charstring::hexEncode(in,insize);
	charstring::upper(hex);
	return hex;
}

// the password hash and the session key size, which both phases start from
static bool o5logonParameters(const char *password,
				parameterstring *p,
				uint32_t *verifiertype,
				byte_t *passwordhash,
				size_t *passwordhashsize,
				size_t *sesskeysize) {

	*verifiertype=(uint32_t)charstring::convertToUnsignedInteger(
					p->getValue("verifiertype"),(int32_t)0);
	if (!supportedVerifierType(*verifiertype)) {
		return false;
	}

	const char	*vfrdatahex=p->getValue("authvfrdata");
	byte_t		*vfrdata=charstring::hexDecode(vfrdatahex);
	uint64_t	vfrdatasize=charstring::getLength(vfrdatahex)/2;
	uint32_t	vgencount=(uint32_t)
				charstring::convertToUnsignedInteger(
					p->getValue("authpbkdf2vgencount"));

	bool	retval=(vfrdatasize &&
			passwordHash(password,*verifiertype,
					vfrdata,vfrdatasize,vgencount,
					passwordhash,passwordhashsize));

	delete[] vfrdata;

	*sesskeysize=sessionKeySize(*verifiertype);

	return retval;
}

// AUTH_SESSKEY - session key part A, encrypted under the password hash
static bool o5logonChallenge(const char *password,
				const char *extra,
				stringbuffer *challenge) {

	parameterstring	p;
	p.parse(extra);

	uint32_t	verifiertype=0;
	byte_t		passwordhash[32];
	size_t		passwordhashsize=0;
	size_t		sesskeysize=0;
	if (!o5logonParameters(password,&p,&verifiertype,
				passwordhash,&passwordhashsize,&sesskeysize)) {
		return false;
	}

	// For an 11g verifier the plaintext isn't 48 random bytes.  Real oracle
	// sends 40 bytes of key material plus 8 bytes of 0x08 - pkcs#7 padding
	// up to the 48 byte boundary - and the client rejects the login if the
	// padding isn't there.  A 12c verifier has no padding.  See #9118.
	size_t	padsize=(sesskeysize==SESSION_KEY_SIZE_11G)?
					SESSION_KEY_PAD_SIZE_11G:0;
	size_t	materialsize=sesskeysize-padsize;

	byte_t	sesskey[SESSION_KEY_SIZE_11G];
	byte_t	encsesskey[SESSION_KEY_SIZE_11G];
	bytestring::set(sesskey+materialsize,(byte_t)padsize,padsize);
	csprng	csr;
	bool	retval=(csr.generateBytes(sesskey,sizeof(sesskey),materialsize) &&
			aesCbc(true,passwordhash,passwordhashsize,
					sesskey,sesskeysize,encsesskey));
	if (retval) {
		char	*hex=hexEncodeUpper(encsesskey,sesskeysize);
		challenge->append(hex);
		delete[] hex;
	}

	bytestring::zero(sesskey,sizeof(sesskey));
	bytestring::zero(passwordhash,sizeof(passwordhash));

	return retval;
}

// the combo key, which AUTH_PASSWORD is encrypted under
static bool o5logonComboKey(uint32_t verifiertype,
				parameterstring *p,
				const byte_t *parta, const byte_t *partb,
				byte_t *combokey, size_t *combokeysize) {

	if (verifiertype==VERIFIER_TYPE_12C) {

		// combo_key = pbkdf2-hmac-sha512(
		//		uppercase_hex(part_b || part_a),
		//		csksalt, 32 bytes, sdercount iterations)
		byte_t	temp[SESSION_KEY_SIZE_12C*2];
		bytestring::copy(temp,partb,SESSION_KEY_SIZE_12C);
		bytestring::copy(temp+SESSION_KEY_SIZE_12C,
					parta,SESSION_KEY_SIZE_12C);
		char	*hex=hexEncodeUpper(temp,sizeof(temp));

		const char	*csksalthex=p->getValue("authpbkdf2csksalt");
		byte_t		*csksalt=charstring::hexDecode(csksalthex);
		uint64_t	csksaltsize=charstring::getLength(csksalthex)/2;
		uint32_t	sdercount=(uint32_t)
					charstring::convertToUnsignedInteger(
					p->getValue("authpbkdf2sdercount"));

		*combokeysize=32;
		bool	retval=(csksaltsize && sdercount &&
				derivedKey((const byte_t *)hex,
					(uint32_t)charstring::getLength(hex),
					csksalt,csksaltsize,
					(uint32_t)*combokeysize,
					sdercount,combokey));

		bytestring::zero(temp,sizeof(temp));
		delete[] hex;
		delete[] csksalt;

		return retval;
	}

	// b = part_a[16..39] xor part_b[16..39]
	// combo_key = (md5(b[0..15]) || md5(b[16..23]))[0..23]
	byte_t	b[24];
	for (size_t i=0; i<sizeof(b); i++) {
		b[i]=parta[16+i]^partb[16+i];
	}

	md5	m1;
	md5	m2;
	bool	retval=(m1.append(b,16) && m2.append(b+16,8));
	if (retval) {
		const byte_t	*part1=m1.getHash();
		const byte_t	*part2=m2.getHash();
		retval=(part1 && part2);
		if (retval) {
			bytestring::copy(combokey,part1,16);
			bytestring::copy(combokey+16,part2,8);
			*combokeysize=24;
		}
	}

	bytestring::zero(b,sizeof(b));

	return retval;
}

// Rebuilds the combo key from the two session keys in "p".  Both phase-two
// paths need it - the one that verifies AUTH_PASSWORD and the one that builds
// AUTH_SVR_RESPONSE.
static bool o5logonComboKeyFromExtra(const char *password,
					parameterstring *p,
					byte_t *combokey,
					size_t *combokeysize) {

	uint32_t	verifiertype=0;
	byte_t		passwordhash[32];
	size_t		passwordhashsize=0;
	size_t		sesskeysize=0;
	if (!o5logonParameters(password,p,&verifiertype,
				passwordhash,&passwordhashsize,&sesskeysize)) {
		return false;
	}

	// Session key part A comes back by decrypting the challenge that
	// challenge() produced, and part B by decrypting the client's
	// AUTH_SESSKEY.  Both are encrypted under the password hash.
	byte_t	encsesskey[SESSION_KEY_SIZE_11G];
	byte_t	parta[SESSION_KEY_SIZE_11G];
	byte_t	partb[SESSION_KEY_SIZE_11G];
	bool	ok=(hexDecodeExactly(p->getValue("serverauthsesskey"),
					sesskeysize,encsesskey) &&
		aesCbc(false,passwordhash,passwordhashsize,
					encsesskey,sesskeysize,parta) &&
		hexDecodeExactly(p->getValue("clientauthsesskey"),
					sesskeysize,encsesskey) &&
		aesCbc(false,passwordhash,passwordhashsize,
					encsesskey,sesskeysize,partb));

	bytestring::zero(passwordhash,sizeof(passwordhash));

	ok=(ok && o5logonComboKey(verifiertype,p,parta,partb,
						combokey,combokeysize));

	bytestring::zero(parta,sizeof(parta));
	bytestring::zero(partb,sizeof(partb));

	return ok;
}

// Verifies AUTH_PASSWORD against "password".  If "supplied" isn't NULL then
// the password the client actually sent is appended to it, for the debug
// output that the caller does.
static bool o5logonVerify(const char *authpassword,
				const char *password,
				const char *extra,
				stringbuffer *supplied) {

	parameterstring	p;
	p.parse(extra);

	byte_t	combokey[32];
	size_t	combokeysize=0;
	if (!o5logonComboKeyFromExtra(password,&p,combokey,&combokeysize)) {
		return false;
	}

	// AUTH_PASSWORD = aes-cbc(combo_key, 16 random bytes || password)
	byte_t		*encpassword=NULL;
	uint64_t	encpasswordsize=0;
	charstring::hexDecode(authpassword,
				charstring::getLength(authpassword),
				&encpassword,&encpasswordsize);
	byte_t	*decpassword=new byte_t[encpasswordsize+1];
	bool	ok=(encpasswordsize>16 && !(encpasswordsize%16) &&
		aesCbc(false,combokey,combokeysize,
			encpassword,encpasswordsize,decpassword));

	bytestring::zero(combokey,sizeof(combokey));
	delete[] encpassword;

	// strip the 16-byte salt and the number padding, each of whose bytes
	// is the number of padding bytes
	uint64_t	passwordsize=0;
	if (ok) {
		byte_t	pad=decpassword[encpasswordsize-1];
		ok=(pad>=1 && pad<=16 &&
			encpasswordsize>=(uint64_t)pad+16);
		for (byte_t i=0; ok && i<pad; i++) {
			ok=(decpassword[encpasswordsize-1-i]==pad);
		}
		if (ok) {
			passwordsize=encpasswordsize-16-pad;
		}
	}

	if (ok) {
		if (supplied) {
			supplied->append((const char *)decpassword+16,
								passwordsize);
		}
		ok=(passwordsize==charstring::getLength(password) &&
			!bytestring::compare(decpassword+16,
						password,passwordsize));
	}

	bytestring::zero(decpassword,encpasswordsize);
	delete[] decpassword;

	return ok;
}

// AUTH_SVR_RESPONSE - the server's proof to the client that it knew the
// password too.  The same shape as AUTH_PASSWORD in the other direction: a
// 16-byte random salt, the payload, then number padding.
static bool o5logonServerResponse(const char *password,
					const char *extra,
					stringbuffer *response) {

	parameterstring	p;
	p.parse(extra);

	byte_t	combokey[32];
	size_t	combokeysize=0;
	if (!o5logonComboKeyFromExtra(password,&p,combokey,&combokeysize)) {
		return false;
	}

	byte_t	plaintext[SERVER_RESPONSE_SIZE];
	byte_t	encresponse[SERVER_RESPONSE_SIZE];
	bytestring::copy(plaintext+16,SERVER_RESPONSE_PAYLOAD,16);
	bytestring::set(plaintext+32,16,16);
	csprng	csr;
	bool	retval=(csr.generateBytes(plaintext,sizeof(plaintext),16) &&
			aesCbc(true,combokey,combokeysize,
				plaintext,sizeof(plaintext),encresponse));
	if (retval) {
		char	*hex=hexEncodeUpper(encresponse,sizeof(encresponse));
		response->append(hex);
		delete[] hex;
	}

	bytestring::zero(plaintext,sizeof(plaintext));
	bytestring::zero(combokey,sizeof(combokey));

	return retval;
}

#endif

const char *sqlrauth_oracle_userlist::auth(sqlrcredentials *cred) {

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
	if (!charstring::isInSet(method,supportedauthmethods)) {
		return NULL;
	}

	// run through the user/password arrays...
	for (uint64_t i=0; i<usercount; i++) {

		// bail if the user doesn't match
		if (charstring::compare(user,users[i])) {
			continue;
		}

		// if password encryption isn't being used, return the user
		// if the passwords match
		if (!charstring::getLength(passwordencryptions[i])) {
			return (compare(password,passwordsize,passwords[i],
						method,extra))?user:NULL;
		}

		// get the module
		sqlrpwdenc	*pe=cont->getPasswordEncryptionById(
						passwordencryptions[i]);
		if (!pe) {
			return NULL;
		}

		// For one-way encryption, encrypt the password that was
		// passed in and compare it to the encrypted password in the
		// configuration.  That only works for oracle_clear_password,
		// which is the only method that passes the password itself
		// in.  O5LOGON derives its challenge from the password, so it
		// needs the cleartext.
		if (pe->oneWay()) {
			if (charstring::compare(method,
						"oracle_clear_password")) {
				return NULL;
			}
			char	*pwd=pe->encrypt(password);
			bool	retval=!charstring::compare(pwd,passwords[i]);
			delete[] pwd;
			return (retval)?user:NULL;
		}

		// For two-way encryption, decrypt the password from the
		// configuration and compare it to the password that was
		// passed in...
		char	*pwd=pe->decrypt(passwords[i]);
		bool	retval=compare(password,passwordsize,
						pwd,method,extra);
		delete[] pwd;
		return (retval)?user:NULL;
	}
	return NULL;
}

char *sqlrauth_oracle_userlist::getClearTextPassword(const char *user) {

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
		// module can't be recovered, so there's no challenge for it
		if (!pe || pe->oneWay()) {
			return NULL;
		}

		return pe->decrypt(passwords[i]);
	}
	return NULL;
}

bool sqlrauth_oracle_userlist::compare(const char *suppliedresponse,
						uint64_t suppliedresponsesize,
						const char *validpassword,
						const char *method,
						const char *extra) {

	// oracle_clear_password is really simple
	if (!charstring::compare(method,"oracle_clear_password")) {
		if (getDebug()) {
			debugStart("auth compare");
			stringbuffer	b;
			b.append("expected response: ");
			b.safePrint(validpassword);
			debugWrite("%s",b.getString());
			b.clear();
			b.append("supplied response: ");
			b.safePrint(suppliedresponse,suppliedresponsesize);
			debugWrite("%s",b.getString());
			debugEnd();
		}
		return !charstring::compare(suppliedresponse,validpassword);
	}

	#ifdef SQLRAUTH_ORACLE_O5LOGON
	if (!charstring::compare(method,"O5LOGON")) {

		if (!getDebug()) {
			return o5logonVerify(suppliedresponse,
						validpassword,extra,NULL);
		}

		stringbuffer	supplied;
		bool		retval=o5logonVerify(suppliedresponse,
						validpassword,extra,&supplied);
		debugStart("auth compare");
		stringbuffer	b;
		b.append("expected response: ");
		b.safePrint(validpassword);
		debugWrite("%s",b.getString());
		b.clear();
		b.append("supplied response: ");
		b.safePrint(supplied.getString(),supplied.getStringLength());
		debugWrite("%s",b.getString());
		debugEnd();
		return retval;
	}
	#endif

	return false;
}

bool sqlrauth_oracle_userlist::challenge(sqlrcredentials *cred,
						stringbuffer *challenge) {

#ifdef SQLRAUTH_ORACLE_O5LOGON

	// this module only supports oracle credentials
	if (charstring::compare(cred->getType(),"oracle")) {
		return false;
	}

	const char	*user=((sqlroraclecredentials *)cred)->getUser();
	const char	*method=((sqlroraclecredentials *)cred)->getMethod();
	const char	*extra=((sqlroraclecredentials *)cred)->getExtra();

	if (getDebug()) {
		debugStart("challenge %s",method);
		debugWrite("user: \"%s\"",user);
		debugWrite("extra: \"%s\"",extra);
		debugEnd();
	}

	// the two O5LOGON phases are the only things this builds
	bool	serverresponse=
			!charstring::compare(method,"O5LOGON-SERVER-RESPONSE");
	if (charstring::compare(method,"O5LOGON") && !serverresponse) {
		return false;
	}

	// both phases are derived from the password, so the cleartext
	// password is required
	char	*validpassword=getClearTextPassword(user);
	bool	retval=false;
	if (validpassword) {
		retval=(serverresponse)?
			o5logonServerResponse(validpassword,extra,challenge):
			o5logonChallenge(validpassword,extra,challenge);
	}
	delete[] validpassword;

	return retval;

#else

	return false;

#endif
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrauth *new_sqlrauth_oracle_userlist(
						sqlrservercontroller *cont,
						domnode *parameters) {
		return new sqlrauth_oracle_userlist(cont,parameters);
	}
}
