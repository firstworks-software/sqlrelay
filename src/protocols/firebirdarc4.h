// Copyright (c) David Muse
// See the file COPYING for more information

#ifndef FIREBIRDARC4_H
#define FIREBIRDARC4_H

#include <rudiments/private/inttypes.h>

// The Arc4 (RC4) wire-crypt cipher used by firebird's "Arc4" crypt plugin,
// which wire protocol 13 and higher can negotiate after SRP authentication.
//
// This is just the cipher.  It knows nothing about the wire, the key
// exchange, or which key to use.  It is written against firebird's own
// implementation, which lives in:
//
//	src/plugins/crypt/arc4/Arc4.cpp
//
// Firebird uses the SRP session key (see sqlrfirebirdsrp::getSessionKey() /
// getSessionKeySize() in firebirdsrp.h) directly as the RC4 key - raw, no
// hashing, no key-schedule discard.  This class doesn't depend on
// firebirdsrp.h though; the caller is expected to pull the key bytes out of
// the srp exchange and hand them here.
//
// RC4 is a stream cipher, and firebird needs one direction's worth of
// keystream for reads and an independently-running one for writes.  So each
// instance of this class keeps its own running state, and crypt() can be
// called repeatedly across many buffers - one socket read or write at a
// time - without resetting anything in between.  Construct two instances
// from the same key, one per direction.
//
// Usage:
//
//	firebirdarc4	readcipher(key,keysize);
//	firebirdarc4	writecipher(key,keysize);
//
//	... readcipher.crypt(buffer,size) as bytes come off the socket ...
//	... writecipher.crypt(buffer,size) as bytes go out to the socket ...
//
// Encryption and decryption are the same operation - RC4 just XORs the
// keystream against the buffer - so there is only one method.

class firebirdarc4 {
	public:
			firebirdarc4(const unsigned char *key, size_t keylen);
			~firebirdarc4();

		// XORs len bytes of buffer, in place, against the next len
		// bytes of this instance's keystream.  Safe to call
		// repeatedly on a running stream; the keystream picks up
		// where the previous call left off.
		void	crypt(unsigned char *buffer, size_t len);

	private:
		// not implemented - not copyable
			firebirdarc4(const firebirdarc4 &);
		firebirdarc4	&operator=(const firebirdarc4 &);

		unsigned char	_state[256];
		unsigned char	_s1;
		unsigned char	_s2;
};

#endif
