// Copyright (c) David Muse
// See the file COPYING for more information

#include "firebirdsrp.h"

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/sha1.h>
#include <rudiments/sha256.h>
#include <rudiments/csprng.h>
#include <rudiments/bignumber.h>

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
		void	processInt(bignumber &bn);

		// SecureHash::processStrippedInt() - srp.h:72-81.  Drops a
		// leading zero byte if there is one.  Given how getBytes()
		// works there never is one, so this only differs from
		// processInt() for the value zero.  Kept anyway, to match.
		void	processStrippedInt(bignumber &bn);

		// SecureHash::getInt() - srp.h:58-63.  The digest bytes read
		// as a big-endian integer.
		void	getInt(bignumber *bn);

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

// Firebird's getBytes() gives no bytes at all for the value zero, but
// bignumber::getMagnitude() writes a single 0 byte for it.  So zero gets
// skipped explicitly here, rather than falling out of a zero length.
void sqlrfirebirdsrpdigest::processInt(bignumber &bn) {
	if (bn.isZero()) {
		return;
	}
	size_t	size=bn.getMagnitudeSize();
	byte_t	*bytes=new byte_t[size];
	bn.getMagnitude(bytes,size);
	h->append(bytes,(uint32_t)size);
	delete[] bytes;
}

void sqlrfirebirdsrpdigest::processStrippedInt(bignumber &bn) {
	if (bn.isZero()) {
		return;
	}
	size_t	size=bn.getMagnitudeSize();
	byte_t	*bytes=new byte_t[size];
	bn.getMagnitude(bytes,size);
	uint32_t	skip=(bytes[0]==0)?1:0;
	h->append(bytes+skip,(uint32_t)size-skip);
	delete[] bytes;
}

void sqlrfirebirdsrpdigest::getInt(bignumber *bn) {
	bn->setValue(h->getHash(),(size_t)h->getHashSize());
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

		bignumber	_n;
		bignumber	_g;
		bignumber	_k;

		bignumber	_clientprivatekey;
		bignumber	_clientpublickey;
		bignumber	_serverprivatekey;
		bignumber	_serverpublickey;
		bignumber	_verifier;

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
// hex with no leading zeros (BigInteger.cpp:205-212).  bignumber::getString(16)
// writes uppercase hex too, but always pads out to a whole number of bytes,
// so a value whose top nibble is zero comes out one digit longer.  That
// difference matters, because the salt and the public keys are hashed as
// text, not as bytes.  So, strip the leading zeros.
static char *sqlrfirebirdsrpBnToHex(bignumber &bn) {
	const char	*hex=bn.getString(16);
	if (!hex) {
		return NULL;
	}
	const char	*start=hex;
	while (*start=='0' && *(start+1)) {
		start++;
	}
	return charstring::duplicate(start);
}


sqlrfirebirdsrp::sqlrfirebirdsrp(sqlrfirebirdsrphash_t hashtype) {

	pvt=new sqlrfirebirdsrpprivate;

	pvt->_hash=hashtype;

	pvt->_n.setValue(_sqlrfirebirdsrp_prime,16);
	pvt->_g.setValue(_sqlrfirebirdsrp_generator,16);

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
		size_t	nlen=pvt->_n.getMagnitudeSize();
		size_t	glen=pvt->_g.getMagnitudeSize();
		if (nlen>glen) {
			size_t	pad=nlen-glen;
			byte_t	*zeros=new byte_t[pad];
			bytestring::zero(zeros,pad);
			digest.process(zeros,(uint32_t)pad);
			delete[] zeros;
		}
		digest.processInt(pvt->_g);
		digest.getInt(&pvt->_k);
	}
}

sqlrfirebirdsrp::~sqlrfirebirdsrp() {

	delete[] pvt->_salt;
	delete[] pvt->_clientpublickeystr;
	delete[] pvt->_serverpublickeystr;
	delete[] pvt->_verifierstr;
	delete[] pvt->_proof;
	delete[] pvt->_sessionkey;

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

	// Note that this just sets the values to zero.  bignumber has no
	// equivalent of BN_clear(), so the old bytes are not securely erased.
	pvt->_clientprivatekey.setValue((int32_t)0);
	pvt->_clientpublickey.setValue((int32_t)0);
	pvt->_serverprivatekey.setValue((int32_t)0);
	pvt->_serverpublickey.setValue((int32_t)0);
	pvt->_verifier.setValue((int32_t)0);

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

	bignumber	salt(bytes,SQLRFIREBIRDSRP_SALT_SIZE);

	delete[] pvt->_salt;
	pvt->_salt=sqlrfirebirdsrpBnToHex(salt);

	bytestring::zero(bytes,sizeof(bytes));

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
static bool sqlrfirebirdsrpSetKey(bignumber *key, const char *keystr,
						bignumber &n) {

	if (!keystr || !keystr[0]) {
		return false;
	}

	if (!key->setValue(keystr,16)) {
		return false;
	}

	// reduce a copy - the key itself is kept as it came in
	bignumber	mod(*key);
	mod.nonNegativeModulo(n);
	bool	trivial=(mod.compare(bignumber((int32_t)1))<=0);

	return !trivial;
}

bool sqlrfirebirdsrp::setClientPublicKey(const char *clientpublickey) {
	if (!sqlrfirebirdsrpSetKey(&pvt->_clientpublickey,clientpublickey,
								pvt->_n)) {
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
	if (!sqlrfirebirdsrpSetKey(&pvt->_serverpublickey,serverpublickey,
								pvt->_n)) {
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
	if (!pvt->_clientprivatekey.setValue(clientprivatekey,16)) {
		return setError("invalid client private key");
	}
	pvt->_clientprivatekey.nonNegativeModulo(pvt->_n);
	return true;
}

bool sqlrfirebirdsrp::setServerPrivateKey(const char *serverprivatekey) {
	if (!pvt->_serverprivatekey.setValue(serverprivatekey,16)) {
		return setError("invalid server private key");
	}
	pvt->_serverprivatekey.nonNegativeModulo(pvt->_n);
	return true;
}

// RemotePassword::makePrivate() - srp.cpp:75-83.  128 random bytes, reduced
// mod the prime.
static bool sqlrfirebirdsrpMakePrivate(bignumber *privatekey, bignumber &n) {

	byte_t	bytes[SQLRFIREBIRDSRP_KEY_SIZE];
	csprng	rng;
	if (!rng.generateBytes(bytes,sizeof(bytes))) {
		return false;
	}
	privatekey->setValue(bytes,SQLRFIREBIRDSRP_KEY_SIZE);
	privatekey->nonNegativeModulo(n);
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
static void sqlrfirebirdsrpGetUserHash(bignumber *x,
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

	bignumber	x;
	sqlrfirebirdsrpGetUserHash(&x,username,pvt->_salt,password);

	// v=g^x mod N - srp.cpp:106
	pvt->_verifier=pvt->_g;
	if (!pvt->_verifier.modPow(x,pvt->_n)) {
		return setError("failed to compute verifier");
	}

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

		if (pvt->_clientprivatekey.isZero()) {
			if (!sqlrfirebirdsrpMakePrivate(
					&pvt->_clientprivatekey,pvt->_n)) {
				return setError("failed to generate "
						"client private key");
			}
		}

		pvt->_clientpublickey=pvt->_g;
		if (!pvt->_clientpublickey.modPow(
					pvt->_clientprivatekey,pvt->_n)) {
			return setError("failed to generate "
					"client public key");
		}

		if (pvt->_clientpublickey.compare(bignumber((int32_t)1))>0) {
			delete[] pvt->_clientpublickeystr;
			pvt->_clientpublickeystr=
				sqlrfirebirdsrpBnToHex(pvt->_clientpublickey);
			return true;
		}

		// srp.cpp:122 - try again with a different private key.
		// Note that this just sets the value to zero.  bignumber has
		// no equivalent of BN_clear(), so the old bytes are not
		// securely erased.
		pvt->_clientprivatekey.setValue((int32_t)0);
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

	bignumber	gb;
	bignumber	kv;

	bool	success=false;

	for (uint16_t i=0; i<16; i++) {

		if (pvt->_serverprivatekey.isZero()) {
			if (!sqlrfirebirdsrpMakePrivate(
					&pvt->_serverprivatekey,pvt->_n)) {
				setError("failed to generate "
						"server private key");
				break;
			}
		}

		// g^b - srp.cpp:131
		gb=pvt->_g;
		if (!gb.modPow(pvt->_serverprivatekey,pvt->_n)) {
			setError("failed to generate server public key");
			break;
		}

		// (k*v)%N - srp.cpp:134
		kv=pvt->_k;
		kv.modMul(pvt->_verifier,pvt->_n);

		// (kv+g^b)%N - srp.cpp:136
		pvt->_serverpublickey=kv;
		pvt->_serverpublickey.modAdd(gb,pvt->_n);

		if (pvt->_serverpublickey.compare(bignumber((int32_t)1))>0) {
			delete[] pvt->_serverpublickeystr;
			pvt->_serverpublickeystr=
				sqlrfirebirdsrpBnToHex(pvt->_serverpublickey);
			success=true;
			break;
		}

		// srp.cpp:143 - try again with a different private key.
		// Note that this just sets the value to zero.  bignumber has
		// no equivalent of BN_clear(), so the old bytes are not
		// securely erased.
		pvt->_serverprivatekey.setValue((int32_t)0);
	}

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
static void sqlrfirebirdsrpComputeScramble(bignumber *u,
					bignumber &clientpublickey,
					bignumber &serverpublickey) {
	sqlrfirebirdsrpdigest	digest(false);
	digest.reset();
	digest.processStrippedInt(clientpublickey);
	digest.processStrippedInt(serverpublickey);
	digest.getInt(u);
}

// The session key is H(S), sha-1 for both plugins - srp.cpp:176-178 and
// srp.cpp:193-195.  So it is always 20 bytes, even for Srp256.
void sqlrfirebirdsrpSetSessionKey(sqlrfirebirdsrpprivate *pvt,
					bignumber &sessionsecret);


// RemotePassword::serverSessionKey() - srp.cpp:181-196.
//
//	u=H(A,B)
//	S=(A * v^u) ^ b mod N
//	K=H(S)
bool sqlrfirebirdsrp::computeServerSessionKey() {

	if (pvt->_clientpublickey.isZero()) {
		return setError("no client public key");
	}
	if (pvt->_serverprivatekey.isZero()) {
		return setError("no server private key");
	}

	bignumber	u;
	bignumber	vu;
	bignumber	avu;
	bignumber	s;

	sqlrfirebirdsrpComputeScramble(&u,pvt->_clientpublickey,
						pvt->_serverpublickey);

	// v^u - srp.cpp:186
	vu=pvt->_verifier;
	if (!vu.modPow(u,pvt->_n)) {
		return setError("failed to compute server session key");
	}

	// (A*v^u)%N - srp.cpp:187
	avu=pvt->_clientpublickey;
	avu.modMul(vu,pvt->_n);

	// (A*v^u)^b mod N - srp.cpp:189
	s=avu;
	if (!s.modPow(pvt->_serverprivatekey,pvt->_n)) {
		return setError("failed to compute server session key");
	}

	sqlrfirebirdsrpSetSessionKey(pvt,s);

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

	if (pvt->_serverpublickey.isZero()) {
		return setError("no server public key");
	}
	if (pvt->_clientprivatekey.isZero()) {
		return setError("no client private key");
	}
	if (!pvt->_salt) {
		return setError("no salt");
	}

	bignumber	u;
	bignumber	x;
	bignumber	gx;
	bignumber	kgx;
	bignumber	diff;
	bignumber	ux;
	bignumber	aux;
	bignumber	s;

	sqlrfirebirdsrpComputeScramble(&u,pvt->_clientpublickey,
						pvt->_serverpublickey);

	// x - srp.cpp:163
	sqlrfirebirdsrpGetUserHash(&x,username,pvt->_salt,password);

	// g^x - srp.cpp:165
	gx=pvt->_g;
	if (!gx.modPow(x,pvt->_n)) {
		return setError("failed to compute client session key");
	}

	// (k*g^x)%N - srp.cpp:166
	kgx=pvt->_k;
	kgx.modMul(gx,pvt->_n);

	// (B - k*g^x)%N - srp.cpp:168.  Firebird's % is libtommath's mp_mod,
	// which is never negative, so use modSub() rather than a plain -
	// followed by a plain % here.
	diff=pvt->_serverpublickey;
	diff.modSub(kgx,pvt->_n);

	// (u*x)%N - srp.cpp:169
	ux=u;
	ux.modMul(x,pvt->_n);

	// (a+u*x)%N - srp.cpp:170
	aux=pvt->_clientprivatekey;
	aux.modAdd(ux,pvt->_n);

	// (B - k*g^x) ^ (a+u*x) mod N - srp.cpp:173
	s=diff;
	if (!s.modPow(aux,pvt->_n)) {
		return setError("failed to compute client session key");
	}

	sqlrfirebirdsrpSetSessionKey(pvt,s);

	return true;
}

void sqlrfirebirdsrpSetSessionKey(sqlrfirebirdsrpprivate *pvt,
					bignumber &sessionsecret) {
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

	bignumber	n1;
	bignumber	n2;

	// n1=H(N) - srp.cpp:201-204
	sha1digest.reset();
	sha1digest.processInt(pvt->_n);
	sha1digest.getInt(&n1);

	// n2=H(g) - srp.cpp:206-209
	sha1digest.reset();
	sha1digest.processInt(pvt->_g);
	sha1digest.getInt(&n2);

	// n1=n1^n2 mod N - srp.cpp:210
	if (!n1.modPow(n2,pvt->_n)) {
		return setError("failed to compute proof");
	}

	// n2=H(username) - srp.cpp:212-214
	sha1digest.reset();
	sha1digest.process(username);
	sha1digest.getInt(&n2);

	// the proof itself - srp.h:140-149
	sqlrfirebirdsrpdigest	digest(pvt->_hash==SQLRFIREBIRDSRP_SRP256);
	digest.reset();
	digest.processInt(n1);
	digest.processInt(n2);
	digest.process(pvt->_salt);
	digest.processInt(pvt->_clientpublickey);
	digest.processInt(pvt->_serverpublickey);
	digest.process(pvt->_sessionkey,(uint32_t)pvt->_sessionkeysize);

	bignumber	proof;
	digest.getInt(&proof);

	delete[] pvt->_proof;
	pvt->_proof=sqlrfirebirdsrpBnToHex(proof);

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

	bignumber	theirs;
	if (!theirs.setValue(proof,16)) {
		return setError("invalid proof");
	}

	bignumber	ours;
	ours.setValue(pvt->_proof,16);

	bool	match=(theirs.compare(ours)==0);

	return (match)?true:setError("proof mismatch");
}
