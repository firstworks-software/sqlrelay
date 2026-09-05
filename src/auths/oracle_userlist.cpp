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
#include <rudiments/singledes.h>
#include <rudiments/tripledes.h>
#include <rudiments/pbkdf2.h>
#include <rudiments/sensitivevalue.h>
#include <rudiments/csprng.h>
#include <rudiments/tls.h>

// O5LOGON is offered at runtime only when tls::isSupported() is true, since
// rudiments' pbkdf2-hmac-sha512 (needed for 12c verifiers) has no
// non-openssl fallback and fails outright without it.  Where it isn't
// supported, oracle_clear_password still works; O5LOGON just isn't offered.
//
// That said, no protocol module SQL Relay ships - including src/protocols/
// oracle.cpp and the native SQLRClient protocol - ever builds a credential
// with method "oracle_clear_password", so this module's support for it has
// no reachable caller today.  It's kept as a hook for a protocol module
// that needs password-based auth without O5LOGON, not for current use.

// verifier types, from python-oracledb's constants.pxi
#define VERIFIER_TYPE_11G_1	0xb152
#define VERIFIER_TYPE_11G_2	0x1b25
#define VERIFIER_TYPE_12C	0x4815

// O3LOGON's verifier type.  Not an oracle constant - O3LOGON sends no
// AUTH_VFR_DATA, and the verifier type only rides in that field's flags, so
// nothing carries a verifier type on the wire for an O3LOGON login.  This
// value exists to select the code path and must match the protocol module's.
#define VERIFIER_TYPE_9I	0x0900

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
//
// O3LOGON is the pre-O5LOGON verifier, the one an OCI7 era client speaks.  It
// uses the same "extra" mechanism, with far fewer inputs, and has no server
// response phase - a real server answers a successful O3LOGON login with a
// bare success summary rather than an AUTH_SVR_RESPONSE.
//
//	challenge(), with method "O3LOGON":
//		verifiertype		2304
//	  out:	the AUTH_SESSKEY to send, uppercase hex, 32 characters
//
//	auth(), with method "O3LOGON":
//		password		AUTH_PASSWORD from the client, hex
//		verifiertype		as above
//		serverauthsesskey	what challenge() returned
//
// The client sends no AUTH_SESSKEY of its own for O3LOGON, so there is no
// clientauthsesskey.  serverauthsesskey is needed for the same reason it is
// for O5LOGON: challenge() keeps no state, so decrypting the challenge it
// produced is the only way to get the session key back at verify time.

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
					const char *user,
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
	"O5LOGON",
	"O3LOGON",
	"oracle_clear_password",
	NULL
};

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

static size_t sessionKeySize(uint32_t verifiertype) {
	// the client tells 11g and 12c apart by the length of the session key
	// rather than by the verifier type it was told, so these lengths are
	// load-bearing on the wire
	return (verifiertype==VERIFIER_TYPE_12C)?
			SESSION_KEY_SIZE_12C:SESSION_KEY_SIZE_11G;
}

static bool supportedVerifierType(uint32_t verifiertype) {
	return (verifiertype==VERIFIER_TYPE_11G_1 ||
		verifiertype==VERIFIER_TYPE_11G_2 ||
		verifiertype==VERIFIER_TYPE_12C);
}

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
	// padding isn't there.  A 12c verifier has no padding.
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

	// decrypt part a from the server's challenge and part b from the
	// client's AUTH_SESSKEY
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

// The O3LOGON exchange below is the server side of the client side described
// by john the ripper's src/o3logon_fmt_plug.c, Copyright (c) 2014 JimF, "hereby
// released to the general public under the following terms: Redistribution and
// use in source and binary forms, with or without modification, are permitted."
// Every constant and every step is pinned against two real captures of an
// oracle 9.2 OCI7 client logging into a real 10.2 server; see trac #9658.

// The 3DES CBC initialization vector, the same for every 3DES operation here,
// and the two entropy blobs the key derivation folds in.  All three are baked
// into the oracle client library - fixed, public, and not per session.
static const byte_t	o3logoniv[]={
	0x80, 0x20, 0x40, 0x04, 0x08, 0x02, 0x10, 0x01
};
static const byte_t	o3logonsesskeyentropy[]={
	0xa2, 0xfb, 0xe6, 0xad, 0x4c, 0x7d, 0x1e, 0x3d,
	0x6e, 0xb0, 0xb7, 0x6c, 0x97, 0xef, 0xff, 0x84,
	0x44, 0x71, 0x02, 0x84, 0xac, 0xf1, 0x3b, 0x29,
	0x5c, 0x0f, 0x0c, 0xb1, 0x87, 0x75, 0xef
};
static const byte_t	o3logonpasswordentropy[]={
	0xf2, 0xff, 0x97, 0x87, 0x15, 0x37, 0x07, 0x76,
	0x07, 0x27, 0xe2, 0x7f, 0xa3, 0xb1, 0xd6, 0x73,
	0x3f, 0x2f, 0xd1, 0x52, 0xab, 0xac, 0xc0
};

// the fixed key the oracle password hash's first pass runs under, and the
// all-zero iv both of its passes run under
static const byte_t	o3logonhashkey[]={
	0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef
};
static const byte_t	o3logonhashiv[]={
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define O3LOGON_BLOCK_SIZE		8
#define O3LOGON_HASH_SIZE		8
#define O3LOGON_KEY_SIZE		24
#define O3LOGON_SESSION_KEY_SIZE	16

// how much of the obfuscated password buffer is not password: 4 bytes of salt
// at the front, and 4 bytes at the back that the rotation duplicates
#define O3LOGON_PASSWORD_OVERHEAD	4

// A single or triple des cbc block transform, chosen by key size, with padding
// off.  The password hash runs under a zero iv and every 3des operation under
// o3logoniv, so the caller passes the one it wants.
static bool desCbc(bool encrypt,
			const byte_t *key, size_t keysize,
			const byte_t *iv,
			const byte_t *in, size_t insize,
			byte_t *out) {

	if (!insize || insize%O3LOGON_BLOCK_SIZE ||
		(keysize!=O3LOGON_HASH_SIZE && keysize!=O3LOGON_KEY_SIZE)) {
		return false;
	}

	encryption	*enc=(keysize==O3LOGON_HASH_SIZE)?
				(encryption *)new singledes():
				(encryption *)new tripledes();
	enc->setUsePadding(false);

	bool	retval=false;
	if (enc->setKey(key,keysize) &&
		enc->setIv(iv,O3LOGON_BLOCK_SIZE) &&
		enc->append(in,(uint32_t)insize)) {
		// getEncryptedDataSize() and getDecryptedDataSize() each re-run
		// the cipher in their own direction, so they have to be paired
		// with the matching getter or the buffer gets replaced by the
		// opposite operation's result
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

// The oracle des password hash - the 8 bytes SYS.USER$.PASSWORD held before
// 11g.  uppercase(user||password) as utf-16be, zero padded to a block
// multiple, des-cbc encrypted under a fixed key, then des-cbc encrypted again
// under the last block of that first pass.  The last block of the second pass
// is the hash.
//
// Note that it folds the case of the password as well as of the user name, so
// the hash alone can't tell "testpassword" from "TESTPASSWORD".  Case still
// decides the login: the password auth() recovers is compared to the stored
// one byte for byte.
static bool o3logonPasswordHash(const char *user, const char *password,
							byte_t *hash) {

	stringbuffer	userpassword;
	userpassword.append(user)->append(password);
	char	*upper=userpassword.detachString();
	charstring::upper(upper);

	size_t	upperlen=charstring::getLength(upper);
	size_t	insize=((upperlen*2)+O3LOGON_BLOCK_SIZE-1)/
				O3LOGON_BLOCK_SIZE*O3LOGON_BLOCK_SIZE;
	if (!insize) {
		insize=O3LOGON_BLOCK_SIZE;
	}

	byte_t	*in=new byte_t[insize];
	byte_t	*out=new byte_t[insize];
	bytestring::zero(in,insize);
	for (size_t i=0; i<upperlen; i++) {
		in[i*2+1]=(byte_t)upper[i];
	}

	bool	retval=desCbc(true,o3logonhashkey,sizeof(o3logonhashkey),
					o3logonhashiv,in,insize,out);
	if (retval) {
		byte_t	key[O3LOGON_HASH_SIZE];
		bytestring::copy(key,out+insize-O3LOGON_HASH_SIZE,sizeof(key));
		retval=desCbc(true,key,sizeof(key),
					o3logonhashiv,in,insize,out);
		if (retval) {
			bytestring::copy(hash,out+insize-O3LOGON_HASH_SIZE,
							O3LOGON_HASH_SIZE);
		}
		bytestring::zero(key,sizeof(key));
	}

	bytestring::zero(in,insize);
	bytestring::zero(out,insize);
	bytestring::zero(upper,upperlen);
	delete[] in;
	delete[] out;
	delete[] upper;

	return retval;
}

// The o3logon key derivation - a 24 byte 3des key from an input and one of the
// two entropy blobs.  The first 20 bytes are sha1(input||entropy).  The last 4
// are the front of a second sha1, over the input, a 0x02 byte, all but the
// first byte of the first digest, and the entropy again.
static bool o3logonCreateKey(const byte_t *in, size_t insize,
				const byte_t *entropy, size_t entropysize,
				byte_t *key) {

	sha1	first;
	if (!first.append(in,(uint32_t)insize) ||
		!first.append(entropy,(uint32_t)entropysize)) {
		return false;
	}
	const byte_t	*digest=first.getHash();
	if (!digest) {
		return false;
	}
	bytestring::copy(key,digest,20);

	// getHash() finalizes the digest, so each sha1 gets its own object and
	// exactly one call
	sha1	second;
	byte_t	two=0x02;
	if (!second.append(in,(uint32_t)insize) ||
		!second.append(&two,1) ||
		!second.append(key+1,19) ||
		!second.append(entropy,(uint32_t)entropysize)) {
		return false;
	}
	digest=second.getHash();
	if (!digest) {
		return false;
	}
	bytestring::copy(key+20,digest,4);

	return true;
}

static bool o3logonSupported() {
	singledes	sd;
	tripledes	td;
	return (sd.isSupported() && td.isSupported());
}

static bool o3logonVerifierType(parameterstring *p) {
	return ((uint32_t)charstring::convertToUnsignedInteger(
				p->getValue("verifiertype"),(int32_t)0)==
							VERIFIER_TYPE_9I);
}

static bool o3logonChallenge(const char *user,
				const char *password,
				const char *extra,
				stringbuffer *challenge) {

	parameterstring	p;
	p.parse(extra);
	if (!o3logonVerifierType(&p)) {
		return false;
	}

	byte_t	hash[O3LOGON_HASH_SIZE];
	if (!o3logonPasswordHash(user,password,hash)) {
		return false;
	}

	// AUTH_SESSKEY = 3des-cbc(createkey(hash), 16 random bytes)
	byte_t	key[O3LOGON_KEY_SIZE];
	byte_t	sesskey[O3LOGON_SESSION_KEY_SIZE];
	byte_t	encsesskey[O3LOGON_SESSION_KEY_SIZE];
	csprng	csr;
	bool	retval=(o3logonCreateKey(hash,sizeof(hash),
					o3logonsesskeyentropy,
					sizeof(o3logonsesskeyentropy),key) &&
			csr.generateBytes(sesskey,sizeof(sesskey),
						sizeof(sesskey)) &&
			desCbc(true,key,sizeof(key),o3logoniv,
					sesskey,sizeof(sesskey),encsesskey));
	if (retval) {
		char	*hex=hexEncodeUpper(encsesskey,sizeof(encsesskey));
		challenge->append(hex);
		delete[] hex;
	}

	bytestring::zero(hash,sizeof(hash));
	bytestring::zero(key,sizeof(key));
	bytestring::zero(sesskey,sizeof(sesskey));

	return retval;
}

static bool o3logonVerify(const char *authpassword,
				const char *user,
				const char *password,
				const char *extra,
				stringbuffer *supplied) {

	parameterstring	p;
	p.parse(extra);
	if (!o3logonVerifierType(&p)) {
		return false;
	}

	byte_t	hash[O3LOGON_HASH_SIZE];
	if (!o3logonPasswordHash(user,password,hash)) {
		return false;
	}

	// recover the session key from the module's own challenge, then derive
	// the key the client obfuscated the password under from it
	byte_t	key[O3LOGON_KEY_SIZE];
	byte_t	encsesskey[O3LOGON_SESSION_KEY_SIZE];
	byte_t	sesskey[O3LOGON_SESSION_KEY_SIZE];
	bool	ok=(o3logonCreateKey(hash,sizeof(hash),
					o3logonsesskeyentropy,
					sizeof(o3logonsesskeyentropy),key) &&
		hexDecodeExactly(p.getValue("serverauthsesskey"),
					sizeof(encsesskey),encsesskey) &&
		desCbc(false,key,sizeof(key),o3logoniv,
				encsesskey,sizeof(encsesskey),sesskey) &&
		o3logonCreateKey(sesskey,sizeof(sesskey),
					o3logonpasswordentropy,
					sizeof(o3logonpasswordentropy),key));

	bytestring::zero(hash,sizeof(hash));
	bytestring::zero(sesskey,sizeof(sesskey));

	if (!ok) {
		bytestring::zero(key,sizeof(key));
		return false;
	}

	// AUTH_PASSWORD = 3des-cbc(key, 4 bytes of salt || the password ||
	// filler), with the password rotated so that its first 4 bytes sit at
	// the end of the buffer.  there is no length field and no padding.
	byte_t		*encpassword=NULL;
	uint64_t	encpasswordsize=0;
	charstring::hexDecode(authpassword,
				charstring::getLength(authpassword),
				&encpassword,&encpasswordsize);
	byte_t	*obf=new byte_t[encpasswordsize+1];
	uint64_t	passwordsize=charstring::getLength(password);
	ok=(encpasswordsize>=(uint64_t)O3LOGON_BLOCK_SIZE &&
		!(encpasswordsize%O3LOGON_BLOCK_SIZE) &&
		passwordsize &&
		passwordsize+O3LOGON_PASSWORD_OVERHEAD<=encpasswordsize &&
		desCbc(false,key,sizeof(key),o3logoniv,
				encpassword,encpasswordsize,obf));

	bytestring::zero(key,sizeof(key));
	delete[] encpassword;

	if (ok) {

		// undo the rotation.  what follows the password is filler, and
		// the last 4 bytes are the rotation's leftovers, so only the
		// first passwordsize bytes mean anything
		bytestring::copy(obf,obf+encpasswordsize-
					O3LOGON_PASSWORD_OVERHEAD,
					O3LOGON_PASSWORD_OVERHEAD);

		if (supplied) {
			supplied->append((const char *)obf,passwordsize);
		}

		ok=!bytestring::compare(obf,password,passwordsize);
	}

	bytestring::zero(obf,encpasswordsize);
	delete[] obf;

	return ok;
}

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

		// no encryption: compare passwords directly
		if (!charstring::getLength(passwordencryptions[i])) {
			return (compare(password,passwordsize,user,passwords[i],
						method,extra))?user:NULL;
		}

		// get the module
		sqlrpwdenc	*pe=cont->getPasswordEncryptionById(
						passwordencryptions[i]);
		if (!pe) {
			return NULL;
		}

		// one-way encryption: encrypt and compare
		// Only oracle_clear_password passes the password itself in -
		// O5LOGON and O3LOGON need the cleartext to derive their
		// challenges.
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

		// two-way encryption: decrypt and compare
		char	*pwd=pe->decrypt(passwords[i]);
		bool	retval=compare(password,passwordsize,user,
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

		// A password stored under a one-way encryption module can't
		// be recovered, so there's no challenge for it.
		if (!pe || pe->oneWay()) {
			return NULL;
		}

		return pe->decrypt(passwords[i]);
	}
	return NULL;
}

bool sqlrauth_oracle_userlist::compare(const char *suppliedresponse,
						uint64_t suppliedresponsesize,
						const char *user,
						const char *validpassword,
						const char *method,
						const char *extra) {

	// oracle_clear_password: compare directly
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

	bool	o5logon=(tls::isSupported() &&
			!charstring::compare(method,"O5LOGON"));
	bool	o3logon=(o3logonSupported() &&
			!charstring::compare(method,"O3LOGON"));
	if (!o5logon && !o3logon) {
		return false;
	}

	if (!getDebug()) {
		return (o5logon)?
			o5logonVerify(suppliedresponse,
					validpassword,extra,NULL):
			o3logonVerify(suppliedresponse,user,
					validpassword,extra,NULL);
	}

	stringbuffer	supplied;
	bool		retval=(o5logon)?
			o5logonVerify(suppliedresponse,
					validpassword,extra,&supplied):
			o3logonVerify(suppliedresponse,user,
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

bool sqlrauth_oracle_userlist::challenge(sqlrcredentials *cred,
						stringbuffer *challenge) {

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

	// the two O5LOGON phases and the one O3LOGON phase are the only things
	// this builds.  the crypto each needs is gated separately - O5LOGON's
	// pbkdf2-hmac-sha512 has no non-openssl fallback in rudiments, and
	// O3LOGON's des and 3des may or may not be there on their own.
	bool	serverresponse=
			!charstring::compare(method,"O5LOGON-SERVER-RESPONSE");
	bool	o5logon=(serverresponse ||
			!charstring::compare(method,"O5LOGON"));
	bool	o3logon=!charstring::compare(method,"O3LOGON");
	if (o5logon) {
		if (!tls::isSupported()) {
			return false;
		}
	} else if (o3logon) {
		if (!o3logonSupported()) {
			return false;
		}
	} else {
		return false;
	}

	// Every phase is derived from the password, so the cleartext
	// password is required.
	char	*validpassword=getClearTextPassword(user);
	bool	retval=false;
	if (validpassword) {
		retval=(o3logon)?
			o3logonChallenge(user,validpassword,extra,challenge):
			((serverresponse)?
			o5logonServerResponse(validpassword,extra,challenge):
			o5logonChallenge(validpassword,extra,challenge));
	}
	delete[] validpassword;

	return retval;
}

extern "C" {
	SQLRSERVER_DLLSPEC sqlrauth *new_sqlrauth_oracle_userlist(
						sqlrservercontroller *cont,
						domnode *parameters) {
		return new sqlrauth_oracle_userlist(cont,parameters);
	}
}
