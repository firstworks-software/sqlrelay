// Copyright (c) David Muse
// See the file COPYING for more information

#include "firebirdsrp.h"

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/sha1.h>
#include <rudiments/sha256.h>
#include <rudiments/csprng.h>

#include <openssl/bn.h>

// The group.  This is NOT one of the RFC 5054 groups.  It is a 1024-bit
// prime that firebird picked for itself.  See srp.cpp:14-19, where it appears
// as primeStr and genStr.
static const char	*_sqlrfirebirdsrp_prime=
			"E67D2E994B2F900C3F41F08F5BB2627ED0D49EE1FE767A52EFCD565C"
			"D6E768812C3E1E9CE8F0A8BEA6CB13CD29DDEBF7A96D4A93B55D488D"
			"F099A15C89DCB0640738EB2CBDD9A8F7BAB561AB1B0DC1C6CDABF303"
			"264A08D1BCA932D1F1EE428B619D970F342ABA9A65793B8B2F041AE5"
			"364350C16F735F56ECBCA87BD57B29E7";
static const char	*_sqlrfirebirdsrp_generator="02";

// srp.h:108-110 - SRP_KEY_SIZE, SRP_SALT_SIZE
#define SQLRFIREBIRDSRP_KEY_SIZE	128
#define SQLRFIREBIRDSRP_SALT_SIZE	32


// A digest, wrapped up so that it can be fed bignums the way firebird's
// SecureHash template does.  See srp.h:55-82.
class sqlrfirebirdsrpdigest {
	public:
			sqlrfirebirdsrpdigest(bool sha256hash);
			~sqlrfirebirdsrpdigest();

		void	reset();

		// SHA::process(size,bytes) - sha.h:50
		void	process(const byte_t *bytes, uint32_t size);

		// SHA::process(const char *) hashes strlen() bytes, without
		// the terminating null - sha.h:62-65
		void	process(const char *str);

		// SecureHash::processInt() - srp.h:65-70.  Firebird's
		// BigInteger::getBytes() is mp_to_ubin(), so the bytes are
		// big-endian and minimal - no leading zero, no sign byte.
		void	processInt(const BIGNUM *bn);

		// SecureHash::processStrippedInt() - srp.h:72-81.  Drops a
		// leading zero byte if there is one.  Given how getBytes()
		// works there never is one, so this only differs from
		// processInt() for the value zero.  Kept anyway, to match.
		void	processStrippedInt(const BIGNUM *bn);

		// SecureHash::getInt() - srp.h:58-63.  The digest bytes read
		// as a big-endian integer.
		void	getInt(BIGNUM *bn);

		const byte_t	*getHash();
		uint64_t	getHashSize();

	private:
		sha1	s1;
		sha256	s256;
		hash	*h;
};

sqlrfirebirdsrpdigest::sqlrfirebirdsrpdigest(bool sha256hash) {
	h=(sha256hash)?static_cast<hash *>(&s256):static_cast<hash *>(&s1);
}

sqlrfirebirdsrpdigest::~sqlrfirebirdsrpdigest() {
}

void sqlrfirebirdsrpdigest::reset() {
	h->clear();
}

void sqlrfirebirdsrpdigest::process(const byte_t *bytes, uint32_t size) {
	h->append(bytes,size);
}

void sqlrfirebirdsrpdigest::process(const char *str) {
	h->append((const byte_t *)str,charstring::getLength(str));
}

void sqlrfirebirdsrpdigest::processInt(const BIGNUM *bn) {
	int	size=BN_num_bytes(bn);
	if (size<=0) {
		return;
	}
	byte_t	*bytes=new byte_t[size];
	BN_bn2bin(bn,bytes);
	h->append(bytes,(uint32_t)size);
	delete[] bytes;
}

void sqlrfirebirdsrpdigest::processStrippedInt(const BIGNUM *bn) {
	int	size=BN_num_bytes(bn);
	if (size<=0) {
		return;
	}
	byte_t	*bytes=new byte_t[size];
	BN_bn2bin(bn,bytes);
	uint32_t	skip=(bytes[0]==0)?1:0;
	h->append(bytes+skip,(uint32_t)size-skip);
	delete[] bytes;
}

void sqlrfirebirdsrpdigest::getInt(BIGNUM *bn) {
	const byte_t	*hashbytes=h->getHash();
	BN_bin2bn(hashbytes,(int)h->getHashSize(),bn);
}

const byte_t *sqlrfirebirdsrpdigest::getHash() {
	return h->getHash();
}

uint64_t sqlrfirebirdsrpdigest::getHashSize() {
	return h->getHashSize();
}


class sqlrfirebirdsrpprivate {
	public:
		sqlrfirebirdsrphash_t	_hash;

		BN_CTX	*_ctx;

		BIGNUM	*_n;
		BIGNUM	*_g;
		BIGNUM	*_k;

		BIGNUM	*_clientprivatekey;
		BIGNUM	*_clientpublickey;
		BIGNUM	*_serverprivatekey;
		BIGNUM	*_serverpublickey;
		BIGNUM	*_verifier;

		char	*_salt;
		char	*_clientpublickeystr;
		char	*_serverpublickeystr;
		char	*_verifierstr;
		char	*_proof;

		byte_t		*_sessionkey;
		uint64_t	_sessionkeysize;

		const char	*_error;
};


// Firebird's BigInteger::getText() is mp_to_radix(), which writes uppercase
// hex with no leading zeros (BigInteger.cpp:205-212).  OpenSSL's BN_bn2hex()
// writes uppercase hex too, but always pads out to a whole number of bytes,
// so a value whose top nibble is zero comes out one digit longer.  That
// difference matters, because the salt and the public keys are hashed as
// text, not as bytes.  So, strip the leading zeros.
static char *sqlrfirebirdsrpBnToHex(const BIGNUM *bn) {
	char	*hex=BN_bn2hex(bn);
	if (!hex) {
		return NULL;
	}
	char	*start=hex;
	while (*start=='0' && *(start+1)) {
		start++;
	}
	char	*retval=charstring::duplicate(start);
	OPENSSL_free(hex);
	return retval;
}


sqlrfirebirdsrp::sqlrfirebirdsrp(sqlrfirebirdsrphash_t hashtype) {

	pvt=new sqlrfirebirdsrpprivate;

	pvt->_hash=hashtype;

	pvt->_ctx=BN_CTX_new();

	pvt->_n=BN_new();
	pvt->_g=BN_new();
	pvt->_k=BN_new();
	BN_hex2bn(&pvt->_n,_sqlrfirebirdsrp_prime);
	BN_hex2bn(&pvt->_g,_sqlrfirebirdsrp_generator);

	pvt->_clientprivatekey=BN_new();
	pvt->_clientpublickey=BN_new();
	pvt->_serverprivatekey=BN_new();
	pvt->_serverpublickey=BN_new();
	pvt->_verifier=BN_new();

	pvt->_salt=NULL;
	pvt->_clientpublickeystr=NULL;
	pvt->_serverpublickeystr=NULL;
	pvt->_verifierstr=NULL;
	pvt->_proof=NULL;

	pvt->_sessionkey=NULL;
	pvt->_sessionkeysize=0;

	pvt->_error=NULL;

	// k=H(N,PAD(g)) - RemoteGroup::RemoteGroup(), srp.cpp:29-46.
	//
	// Note that k is hashed with sha-1 no matter which plugin is in play.
	// srp.cpp:32 names Firebird::Sha1 outright, rather than using the
	// plugin's hash.
	//
	// The zero padding at srp.cpp:35-42 left-pads g out to the length of
	// the prime, which is what SRP-6a calls for.
	{
		sqlrfirebirdsrpdigest	digest(false);
		digest.reset();
		digest.processInt(pvt->_n);
		int	nlen=BN_num_bytes(pvt->_n);
		int	glen=BN_num_bytes(pvt->_g);
		if (nlen>glen) {
			int	pad=nlen-glen;
			byte_t	*zeros=new byte_t[pad];
			bytestring::zero(zeros,pad);
			digest.process(zeros,(uint32_t)pad);
			delete[] zeros;
		}
		digest.processInt(pvt->_g);
		digest.getInt(pvt->_k);
	}
}

sqlrfirebirdsrp::~sqlrfirebirdsrp() {

	delete[] pvt->_salt;
	delete[] pvt->_clientpublickeystr;
	delete[] pvt->_serverpublickeystr;
	delete[] pvt->_verifierstr;
	delete[] pvt->_proof;
	delete[] pvt->_sessionkey;

	BN_clear_free(pvt->_clientprivatekey);
	BN_clear_free(pvt->_clientpublickey);
	BN_clear_free(pvt->_serverprivatekey);
	BN_clear_free(pvt->_serverpublickey);
	BN_clear_free(pvt->_verifier);

	BN_free(pvt->_n);
	BN_free(pvt->_g);
	BN_free(pvt->_k);

	BN_CTX_free(pvt->_ctx);

	delete pvt;
}

void sqlrfirebirdsrp::clear() {

	delete[] pvt->_salt;
	pvt->_salt=NULL;
	delete[] pvt->_clientpublickeystr;
	pvt->_clientpublickeystr=NULL;
	delete[] pvt->_serverpublickeystr;
	pvt->_serverpublickeystr=NULL;
	delete[] pvt->_verifierstr;
	pvt->_verifierstr=NULL;
	delete[] pvt->_proof;
	pvt->_proof=NULL;
	delete[] pvt->_sessionkey;
	pvt->_sessionkey=NULL;
	pvt->_sessionkeysize=0;

	BN_clear(pvt->_clientprivatekey);
	BN_clear(pvt->_clientpublickey);
	BN_clear(pvt->_serverprivatekey);
	BN_clear(pvt->_serverpublickey);
	BN_clear(pvt->_verifier);

	pvt->_error=NULL;
}

bool sqlrfirebirdsrp::setError(const char *error) {
	pvt->_error=error;
	return false;
}

const char *sqlrfirebirdsrp::getError() const {
	return pvt->_error;
}

const char *sqlrfirebirdsrp::getPrime() {
	return _sqlrfirebirdsrp_prime;
}

const char *sqlrfirebirdsrp::getGenerator() {
	return _sqlrfirebirdsrp_generator;
}


// The salt is 32 random bytes, but it never travels or gets hashed as bytes.
// SrpServer.cpp:323-326 reads it out of the security database, runs it
// through a BigInteger, and takes the hex text.  Everything downstream - the
// wire, and getUserHash() - uses that text.  So do the same here: make the
// bytes, run them through a bignum, and keep the text.
//
// One consequence, inherited from firebird: a salt that happens to start with
// a zero byte loses it, and the text is shorter.  That is fine.  Both sides
// only ever see the text.
bool sqlrfirebirdsrp::generateSalt() {

	byte_t	bytes[SQLRFIREBIRDSRP_SALT_SIZE];
	csprng	rng;
	if (!rng.generateBytes(bytes,sizeof(bytes))) {
		return setError("failed to generate salt");
	}

	BIGNUM	*salt=BN_new();
	BN_bin2bn(bytes,SQLRFIREBIRDSRP_SALT_SIZE,salt);

	delete[] pvt->_salt;
	pvt->_salt=sqlrfirebirdsrpBnToHex(salt);

	BN_clear_free(salt);

	return (pvt->_salt!=NULL)?true:setError("failed to generate salt");
}

bool sqlrfirebirdsrp::setSalt(const char *salt) {
	if (!salt || !salt[0]) {
		return setError("empty salt");
	}
	if (charstring::getLength(salt)>SQLRFIREBIRDSRP_SALT_SIZE*2) {
		return setError("salt too long");
	}
	delete[] pvt->_salt;
	pvt->_salt=charstring::duplicate(salt);
	return true;
}

const char *sqlrfirebirdsrp::getSalt() const {
	return pvt->_salt;
}


// RemotePassword::setKey() - srp.cpp:222-229.  A key that is 0 or 1 mod the
// prime is rejected outright.
static bool sqlrfirebirdsrpSetKey(BIGNUM *key, const char *keystr,
					const BIGNUM *n, BN_CTX *ctx) {

	if (!keystr || !keystr[0]) {
		return false;
	}

	BIGNUM	*k=key;
	if (!BN_hex2bn(&k,keystr)) {
		return false;
	}

	BIGNUM	*mod=BN_new();
	BN_nnmod(mod,key,n,ctx);
	bool	trivial=(BN_cmp(mod,BN_value_one())<=0);
	BN_free(mod);

	return !trivial;
}

bool sqlrfirebirdsrp::setClientPublicKey(const char *clientpublickey) {
	if (!sqlrfirebirdsrpSetKey(pvt->_clientpublickey,clientpublickey,
						pvt->_n,pvt->_ctx)) {
		return setError("trivial or invalid client public key");
	}
	delete[] pvt->_clientpublickeystr;
	pvt->_clientpublickeystr=charstring::duplicate(clientpublickey);
	return true;
}

const char *sqlrfirebirdsrp::getClientPublicKey() const {
	return pvt->_clientpublickeystr;
}

bool sqlrfirebirdsrp::setServerPublicKey(const char *serverpublickey) {
	if (!sqlrfirebirdsrpSetKey(pvt->_serverpublickey,serverpublickey,
						pvt->_n,pvt->_ctx)) {
		return setError("trivial or invalid server public key");
	}
	delete[] pvt->_serverpublickeystr;
	pvt->_serverpublickeystr=charstring::duplicate(serverpublickey);
	return true;
}

const char *sqlrfirebirdsrp::getServerPublicKey() const {
	return pvt->_serverpublickeystr;
}

bool sqlrfirebirdsrp::setClientPrivateKey(const char *clientprivatekey) {
	BIGNUM	*k=pvt->_clientprivatekey;
	if (!BN_hex2bn(&k,clientprivatekey)) {
		return setError("invalid client private key");
	}
	BN_nnmod(pvt->_clientprivatekey,pvt->_clientprivatekey,
						pvt->_n,pvt->_ctx);
	return true;
}

bool sqlrfirebirdsrp::setServerPrivateKey(const char *serverprivatekey) {
	BIGNUM	*k=pvt->_serverprivatekey;
	if (!BN_hex2bn(&k,serverprivatekey)) {
		return setError("invalid server private key");
	}
	BN_nnmod(pvt->_serverprivatekey,pvt->_serverprivatekey,
						pvt->_n,pvt->_ctx);
	return true;
}

// RemotePassword::makePrivate() - srp.cpp:75-83.  128 random bytes, reduced
// mod the prime.
static bool sqlrfirebirdsrpMakePrivate(BIGNUM *privatekey,
					const BIGNUM *n, BN_CTX *ctx) {

	byte_t	bytes[SQLRFIREBIRDSRP_KEY_SIZE];
	csprng	rng;
	if (!rng.generateBytes(bytes,sizeof(bytes))) {
		return false;
	}
	BN_bin2bn(bytes,SQLRFIREBIRDSRP_KEY_SIZE,privatekey);
	BN_nnmod(privatekey,privatekey,n,ctx);
	bytestring::zero(bytes,sizeof(bytes));
	return true;
}


// RemotePassword::getUserHash() - srp.cpp:85-101, then computeVerifier() at
// srp.cpp:103-107.
//
//	x=H(salt,H(username:password))
//	v=g^x mod N
//
// Two things to notice.
//
// First, this is not RFC 5054's x.  RFC 5054 hashes the username into the
// inner hash and the salt as raw bytes.  Firebird hashes the salt as its hex
// TEXT (srp.cpp:88-96 takes a const char * and runs it through
// SHA::process(const char *), which is strlen-based - sha.h:62-65).
//
// Second, the hash here is sha-1 for both plugins.  It comes from
// RemotePassword::hash, declared SecureHash<Firebird::Sha1> at srp.h:91,
// not from the plugin's template parameter.
static void sqlrfirebirdsrpGetUserHash(BIGNUM *x,
					const char *username,
					const char *salt,
					const char *password) {

	sqlrfirebirdsrpdigest	digest(false);

	digest.reset();
	digest.process(username);
	digest.process(":");
	digest.process(password);

	uint64_t	hash1size=digest.getHashSize();
	byte_t		*hash1=new byte_t[hash1size];
	bytestring::copy(hash1,digest.getHash(),hash1size);

	digest.reset();
	digest.process(salt);
	digest.process(hash1,(uint32_t)hash1size);
	digest.getInt(x);

	delete[] hash1;
}

bool sqlrfirebirdsrp::computeVerifier(const char *username,
					const char *password) {

	if (!pvt->_salt) {
		return setError("no salt");
	}

	BIGNUM	*x=BN_new();
	sqlrfirebirdsrpGetUserHash(x,username,pvt->_salt,password);

	// v=g^x mod N - srp.cpp:106
	BN_mod_exp(pvt->_verifier,pvt->_g,x,pvt->_n,pvt->_ctx);

	BN_clear_free(x);

	delete[] pvt->_verifierstr;
	pvt->_verifierstr=sqlrfirebirdsrpBnToHex(pvt->_verifier);

	return true;
}

const char *sqlrfirebirdsrp::getVerifier() const {
	return pvt->_verifierstr;
}


// RemotePassword::genClientKey() - srp.cpp:109-124.
//
//	A=g^a mod N
//
// Retried until A>1.
bool sqlrfirebirdsrp::generateClientPublicKey() {

	for (uint16_t i=0; i<16; i++) {

		if (BN_is_zero(pvt->_clientprivatekey)) {
			if (!sqlrfirebirdsrpMakePrivate(pvt->_clientprivatekey,
						pvt->_n,pvt->_ctx)) {
				return setError("failed to generate "
						"client private key");
			}
		}

		BN_mod_exp(pvt->_clientpublickey,pvt->_g,
				pvt->_clientprivatekey,pvt->_n,pvt->_ctx);

		if (BN_cmp(pvt->_clientpublickey,BN_value_one())>0) {
			delete[] pvt->_clientpublickeystr;
			pvt->_clientpublickeystr=
				sqlrfirebirdsrpBnToHex(pvt->_clientpublickey);
			return true;
		}

		// srp.cpp:122 - try again with a different private key
		BN_clear(pvt->_clientprivatekey);
	}

	return setError("failed to generate client public key");
}


// RemotePassword::genServerKey() - srp.cpp:126-145.
//
//	B=(k*v + g^b) mod N
//
// Retried until B>1.  The verifier is computed here rather than looked up,
// since sqlrelay has the configured password in hand and firebird's server
// has only a verifier in its security database.
bool sqlrfirebirdsrp::generateServerPublicKey(const char *username,
						const char *password) {

	if (!computeVerifier(username,password)) {
		return false;
	}

	BIGNUM	*gb=BN_new();
	BIGNUM	*kv=BN_new();

	bool	success=false;

	for (uint16_t i=0; i<16; i++) {

		if (BN_is_zero(pvt->_serverprivatekey)) {
			if (!sqlrfirebirdsrpMakePrivate(pvt->_serverprivatekey,
						pvt->_n,pvt->_ctx)) {
				setError("failed to generate "
						"server private key");
				break;
			}
		}

		// g^b - srp.cpp:131
		BN_mod_exp(gb,pvt->_g,pvt->_serverprivatekey,
						pvt->_n,pvt->_ctx);

		// (k*v)%N - srp.cpp:134
		BN_mod_mul(kv,pvt->_k,pvt->_verifier,pvt->_n,pvt->_ctx);

		// (kv+g^b)%N - srp.cpp:136
		BN_add(pvt->_serverpublickey,kv,gb);
		BN_nnmod(pvt->_serverpublickey,pvt->_serverpublickey,
						pvt->_n,pvt->_ctx);

		if (BN_cmp(pvt->_serverpublickey,BN_value_one())>0) {
			delete[] pvt->_serverpublickeystr;
			pvt->_serverpublickeystr=
				sqlrfirebirdsrpBnToHex(pvt->_serverpublickey);
			success=true;
			break;
		}

		// srp.cpp:143 - try again with a different private key
		BN_clear(pvt->_serverprivatekey);
	}

	BN_clear_free(gb);
	BN_clear_free(kv);

	if (!success && !pvt->_error) {
		setError("failed to generate server public key");
	}
	return success;
}


// RemotePassword::computeScramble() - srp.cpp:147-155.
//
//	u=H(A,B)
//
// sha-1 for both plugins, and the operands go in stripped - see
// processStrippedInt() above.
static void sqlrfirebirdsrpComputeScramble(BIGNUM *u,
					const BIGNUM *clientpublickey,
					const BIGNUM *serverpublickey) {
	sqlrfirebirdsrpdigest	digest(false);
	digest.reset();
	digest.processStrippedInt(clientpublickey);
	digest.processStrippedInt(serverpublickey);
	digest.getInt(u);
}

// The session key is H(S), sha-1 for both plugins - srp.cpp:176-178 and
// srp.cpp:193-195.  So it is always 20 bytes, even for Srp256.
void sqlrfirebirdsrpSetSessionKey(sqlrfirebirdsrpprivate *pvt,
					const BIGNUM *sessionsecret);


// RemotePassword::serverSessionKey() - srp.cpp:181-196.
//
//	u=H(A,B)
//	S=(A * v^u) ^ b mod N
//	K=H(S)
bool sqlrfirebirdsrp::computeServerSessionKey() {

	if (BN_is_zero(pvt->_clientpublickey)) {
		return setError("no client public key");
	}
	if (BN_is_zero(pvt->_serverprivatekey)) {
		return setError("no server private key");
	}

	BIGNUM	*u=BN_new();
	BIGNUM	*vu=BN_new();
	BIGNUM	*avu=BN_new();
	BIGNUM	*s=BN_new();

	sqlrfirebirdsrpComputeScramble(u,pvt->_clientpublickey,
						pvt->_serverpublickey);

	// v^u - srp.cpp:186
	BN_mod_exp(vu,pvt->_verifier,u,pvt->_n,pvt->_ctx);

	// (A*v^u)%N - srp.cpp:187
	BN_mod_mul(avu,pvt->_clientpublickey,vu,pvt->_n,pvt->_ctx);

	// (A*v^u)^b mod N - srp.cpp:189
	BN_mod_exp(s,avu,pvt->_serverprivatekey,pvt->_n,pvt->_ctx);

	sqlrfirebirdsrpSetSessionKey(pvt,s);

	BN_clear_free(u);
	BN_clear_free(vu);
	BN_clear_free(avu);
	BN_clear_free(s);

	return true;
}


// RemotePassword::clientSessionKey() - srp.cpp:157-179.
//
//	u=H(A,B)
//	x=H(salt,H(username:password))
//	S=(B - k*g^x) ^ (a + u*x) mod N
//	K=H(S)
bool sqlrfirebirdsrp::computeClientSessionKey(const char *username,
						const char *password) {

	if (BN_is_zero(pvt->_serverpublickey)) {
		return setError("no server public key");
	}
	if (BN_is_zero(pvt->_clientprivatekey)) {
		return setError("no client private key");
	}
	if (!pvt->_salt) {
		return setError("no salt");
	}

	BIGNUM	*u=BN_new();
	BIGNUM	*x=BN_new();
	BIGNUM	*gx=BN_new();
	BIGNUM	*kgx=BN_new();
	BIGNUM	*diff=BN_new();
	BIGNUM	*ux=BN_new();
	BIGNUM	*aux=BN_new();
	BIGNUM	*s=BN_new();

	sqlrfirebirdsrpComputeScramble(u,pvt->_clientpublickey,
						pvt->_serverpublickey);

	// x - srp.cpp:163
	sqlrfirebirdsrpGetUserHash(x,username,pvt->_salt,password);

	// g^x - srp.cpp:165
	BN_mod_exp(gx,pvt->_g,x,pvt->_n,pvt->_ctx);

	// (k*g^x)%N - srp.cpp:166
	BN_mod_mul(kgx,pvt->_k,gx,pvt->_n,pvt->_ctx);

	// (B - k*g^x)%N - srp.cpp:168.  Firebird's % is libtommath's mp_mod,
	// which is never negative, so use BN_nnmod rather than BN_mod here.
	BN_sub(diff,pvt->_serverpublickey,kgx);
	BN_nnmod(diff,diff,pvt->_n,pvt->_ctx);

	// (u*x)%N - srp.cpp:169
	BN_mod_mul(ux,u,x,pvt->_n,pvt->_ctx);

	// (a+u*x)%N - srp.cpp:170
	BN_add(aux,pvt->_clientprivatekey,ux);
	BN_nnmod(aux,aux,pvt->_n,pvt->_ctx);

	// (B - k*g^x) ^ (a+u*x) mod N - srp.cpp:173
	BN_mod_exp(s,diff,aux,pvt->_n,pvt->_ctx);

	sqlrfirebirdsrpSetSessionKey(pvt,s);

	BN_clear_free(u);
	BN_clear_free(x);
	BN_clear_free(gx);
	BN_clear_free(kgx);
	BN_clear_free(diff);
	BN_clear_free(ux);
	BN_clear_free(aux);
	BN_clear_free(s);

	return true;
}

void sqlrfirebirdsrpSetSessionKey(sqlrfirebirdsrpprivate *pvt,
					const BIGNUM *sessionsecret) {
	sqlrfirebirdsrpdigest	digest(false);
	digest.reset();
	digest.processStrippedInt(sessionsecret);
	delete[] pvt->_sessionkey;
	pvt->_sessionkeysize=digest.getHashSize();
	pvt->_sessionkey=new byte_t[pvt->_sessionkeysize];
	bytestring::copy(pvt->_sessionkey,digest.getHash(),
						pvt->_sessionkeysize);
}

const byte_t *sqlrfirebirdsrp::getSessionKey() const {
	return pvt->_sessionkey;
}

uint64_t sqlrfirebirdsrp::getSessionKeySize() const {
	return pvt->_sessionkeysize;
}


// RemotePassword::clientProof() - srp.cpp:199-217 - and
// RemotePasswordImpl::makeProof() - srp.h:137-151.
//
//	n1=H(N)
//	n2=H(g)
//	n1=n1^n2 mod N
//	n2=H(username)
//	M=HASH(n1, n2, salt, A, B, K)
//
// Two deviations from textbook SRP-6a to be careful about.
//
// First, classic SRP-6a computes H(N) XOR H(g).  Firebird does not xor.  It
// does a modular exponentiation - srp.cpp:210 is n1.modPow(n2,prime).  The
// comment at srp.cpp:198 still says "^", which reads as xor, but the code is
// modPow.  This is deliberate on their end and has been that way since the
// plugin shipped, so follow the code.
//
// Second, only this final digest uses the plugin's hash.  n1 and n2 are
// built with RemotePassword::hash, which srp.h:91 fixes at sha-1.  So Srp256
// differs from Srp in one hash and one hash only - the one right here.  The
// verifier, the scramble, and the session key are sha-1 in both.
//
// Both sides compute this same value.  Firebird calls it the client proof:
// the client sends it (SrpClient.cpp:152-156) and the server recomputes it
// and compares (SrpServer.cpp:359-365).
bool sqlrfirebirdsrp::computeProof(const char *username) {

	if (!pvt->_sessionkey) {
		return setError("no session key");
	}
	if (!pvt->_salt) {
		return setError("no salt");
	}

	// n1 and n2 - sha-1 in both plugins
	sqlrfirebirdsrpdigest	sha1digest(false);

	BIGNUM	*n1=BN_new();
	BIGNUM	*n2=BN_new();

	// n1=H(N) - srp.cpp:201-204
	sha1digest.reset();
	sha1digest.processInt(pvt->_n);
	sha1digest.getInt(n1);

	// n2=H(g) - srp.cpp:206-209
	sha1digest.reset();
	sha1digest.processInt(pvt->_g);
	sha1digest.getInt(n2);

	// n1=n1^n2 mod N - srp.cpp:210
	BN_mod_exp(n1,n1,n2,pvt->_n,pvt->_ctx);

	// n2=H(username) - srp.cpp:212-214
	sha1digest.reset();
	sha1digest.process(username);
	sha1digest.getInt(n2);

	// the proof itself - srp.h:140-149
	sqlrfirebirdsrpdigest	digest(pvt->_hash==SQLRFIREBIRDSRP_SRP256);
	digest.reset();
	digest.processInt(n1);
	digest.processInt(n2);
	digest.process(pvt->_salt);
	digest.processInt(pvt->_clientpublickey);
	digest.processInt(pvt->_serverpublickey);
	digest.process(pvt->_sessionkey,(uint32_t)pvt->_sessionkeysize);

	BIGNUM	*proof=BN_new();
	digest.getInt(proof);

	delete[] pvt->_proof;
	pvt->_proof=sqlrfirebirdsrpBnToHex(proof);

	BN_free(n1);
	BN_free(n2);
	BN_clear_free(proof);

	return (pvt->_proof!=NULL)?true:setError("failed to compute proof");
}

const char *sqlrfirebirdsrp::getProof() const {
	return pvt->_proof;
}

// SrpServer.cpp:357-365 compares the proofs as numbers, not as text, so that
// leading zeros or a difference in case can't matter.  Do the same.
bool sqlrfirebirdsrp::verifyProof(const char *username, const char *proof) {

	if (!computeProof(username)) {
		return false;
	}

	if (!proof || !proof[0]) {
		return setError("empty proof");
	}

	BIGNUM	*theirs=NULL;
	if (!BN_hex2bn(&theirs,proof)) {
		BN_free(theirs);
		return setError("invalid proof");
	}

	BIGNUM	*ours=NULL;
	BN_hex2bn(&ours,pvt->_proof);

	bool	match=(BN_cmp(theirs,ours)==0);

	BN_free(theirs);
	BN_free(ours);

	return (match)?true:setError("proof mismatch");
}
