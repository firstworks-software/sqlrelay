// Copyright (c) David Muse
// See the file COPYING for more information

#ifndef FIREBIRDSRP_H
#define FIREBIRDSRP_H

#include <rudiments/private/inttypes.h>

// The SRP-6a crypto core used by firebird's Srp and Srp256 authentication
// plugins, which wire protocol 13 and higher negotiate.
//
// This is just the math.  It knows nothing about the wire.  Everything goes
// in and comes out as either a plain byte buffer or a hex string, in exactly
// the form firebird puts on the wire.
//
// It is written against firebird's own implementation, which lives in:
//
//	src/auth/SecureRemotePassword/srp.h
//	src/auth/SecureRemotePassword/srp.cpp
//	src/auth/SecureRemotePassword/server/SrpServer.cpp
//	src/auth/SecureRemotePassword/client/SrpClient.cpp
//
// Methods below cite the lines of those files that they mirror.  Firebird's
// implementation deviates from textbook SRP-6a (and from RFC 5054) in several
// places, so where it does, the comment says so.
//
// Both halves of the exchange are implemented.  sqlrelay only ever plays the
// server, but the client half is there to run the server half against, since
// there is otherwise nothing to test it with.
//
// Note that firebird's server never sends a server proof (M2) back.  It just
// checks the client's proof and moves on - SrpServer.cpp:365-391.  So there
// is no M2 here either.
//
// Server-side usage:
//
//	sqlrfirebirdsrp	srp(SQLRFIREBIRDSRP_SRP256);
//
//	// the client sent us its public key...
//	srp.setClientPublicKey(a);
//
//	// pick a salt and answer with our own public key...
//	srp.generateSalt();
//	srp.generateServerPublicKey(username,password);
//	... send srp.getSalt() and srp.getServerPublicKey() ...
//
//	// work out the session key...
//	srp.computeServerSessionKey();
//
//	// the client sent its proof...
//	if (srp.verifyProof(username,m1)) {
//		... authenticated, srp.getSessionKey() is the wire crypt key ...
//	}
//
// Client-side usage:
//
//	sqlrfirebirdsrp	srp(SQLRFIREBIRDSRP_SRP256);
//	srp.generateClientPublicKey();
//	... send srp.getClientPublicKey(), get back salt and server public key ...
//	srp.setSalt(salt);
//	srp.setServerPublicKey(b);
//	srp.computeClientSessionKey(username,password);
//	srp.computeProof(username);
//	... send srp.getProof() ...

// Which hash the plugin uses.  The two plugins run the identical algorithm.
// The only thing that differs is the hash used to build the proof.  Firebird
// hardwires sha-1 for everything else - see the comment on computeProof().
enum sqlrfirebirdsrphash_t {
	SQLRFIREBIRDSRP_SRP=0,		// plugin "Srp"
	SQLRFIREBIRDSRP_SRP256		// plugin "Srp256"
};

class sqlrfirebirdsrpprivate;

class sqlrfirebirdsrp {
	public:
		sqlrfirebirdsrp(sqlrfirebirdsrphash_t hashtype);
		~sqlrfirebirdsrp();

		// Resets everything but the hash choice.
		void	clear();


		// salt - 32 random bytes, carried as hex text
		bool		generateSalt();
		bool		setSalt(const char *salt);
		const char	*getSalt() const;

		// public keys - hex text, as they appear on the wire
		bool		setClientPublicKey(const char *clientpublickey);
		const char	*getClientPublicKey() const;
		bool		setServerPublicKey(const char *serverpublickey);
		const char	*getServerPublicKey() const;

		// private keys - normally random, settable to make tests
		// reproducible
		bool	setClientPrivateKey(const char *clientprivatekey);
		bool	setServerPrivateKey(const char *serverprivatekey);

		// server half
		bool	generateServerPublicKey(const char *username,
						const char *password);
		bool	computeServerSessionKey();

		// client half
		bool	generateClientPublicKey();
		bool	computeClientSessionKey(const char *username,
						const char *password);

		// the session key, and the verifier that the server derived
		// on the way to it
		const byte_t	*getSessionKey() const;
		uint64_t	getSessionKeySize() const;
		const char	*getVerifier() const;

		// proof
		bool		computeProof(const char *username);
		const char	*getProof() const;
		bool		verifyProof(const char *username,
						const char *proof);

		// what went wrong, or NULL
		const char	*getError() const;

		// The group parameters, for tests and diagnostics.
		static const char	*getPrime();
		static const char	*getGenerator();

	private:
		bool	setError(const char *error);
		bool	computeVerifier(const char *username,
						const char *password);

		sqlrfirebirdsrpprivate	*pvt;
};

#endif
