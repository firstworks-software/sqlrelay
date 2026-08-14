// Copyright (c) David Muse
// See the file COPYING for more information

// Unit tests for the firebird protocol module's Arc4 wire-crypt cipher
// (src/protocols/firebirdarc4.h).  Pure cipher-correctness checks - no
// firebird client library, no database, and no running sqlrelay instance,
// so this runs as part of "make tests" without anything else set up.
//
// Covers:
//  - the standard RC4 test vector: key "Key" against plaintext "Plaintext"
//    encrypts to BBF316E8D940AF0AD3
//  - the streaming property: crypt() called across multiple buffers on one
//    instance has to produce the same result as a single call with all of
//    the data at once, since a real socket delivers the stream across many
//    reads/writes, not in one shot
//  - round trip: crypt() again, with a fresh instance and the same key,
//    recovers the original plaintext, since RC4 encryption and decryption
//    are the same xor-the-keystream operation

#include "../../src/protocols/firebirdarc4.h"
#include <rudiments/stdio.h>
#include <rudiments/charstring.h>

static int	status=0;

static void hexEncode(const unsigned char *buf, size_t len, char *out) {
	static const char	*hex="0123456789ABCDEF";
	for (size_t i=0; i<len; i++) {
		out[i*2]=hex[buf[i]>>4];
		out[i*2+1]=hex[buf[i]&0x0F];
	}
	out[len*2]='\0';
}

static void check(const char *name, bool passed) {
	stdoutput.printf("%s: %s\n",name,(passed)?"success":"failed");
	if (!passed) {
		status=1;
	}
}

int main(int argc, char **argv) {

	const unsigned char	key[]={0x4b,0x65,0x79}; // "Key"

	// standard RC4 test vector
	{
		unsigned char	buf[9];
		charstring::copy((char *)buf,"Plaintext",9);

		firebirdarc4	cipher(key,sizeof(key));
		cipher.crypt(buf,sizeof(buf));

		char	hex[19];
		hexEncode(buf,sizeof(buf),hex);
		check("standard RC4 test vector",
			!charstring::compare(hex,"BBF316E8D940AF0AD3"));
	}

	// streaming property: crypt("Plain") then crypt("text") on one
	// instance has to equal crypt("Plaintext") in one call on a fresh
	// instance constructed from the same key
	{
		unsigned char	streamed[9];
		charstring::copy((char *)streamed,"Plaintext",9);
		firebirdarc4	streamcipher(key,sizeof(key));
		streamcipher.crypt(streamed,5);	// "Plain"
		streamcipher.crypt(streamed+5,4);	// "text"

		unsigned char	single[9];
		charstring::copy((char *)single,"Plaintext",9);
		firebirdarc4	singlecipher(key,sizeof(key));
		singlecipher.crypt(single,sizeof(single));

		check("streaming property",
			!charstring::compare((char *)streamed,
						(char *)single,
						sizeof(single)));
	}

	// round trip: encrypting then decrypting with a fresh cipher
	// instance (same key) recovers the original plaintext
	{
		unsigned char	buf[9];
		charstring::copy((char *)buf,"Plaintext",9);

		firebirdarc4	enc(key,sizeof(key));
		enc.crypt(buf,sizeof(buf));

		firebirdarc4	dec(key,sizeof(key));
		dec.crypt(buf,sizeof(buf));

		check("round trip",
			!charstring::compare((char *)buf,"Plaintext",9));
	}

	if (status==0) {
		stdoutput.printf("\nAll tests succeeded\n");
	} else {
		stdoutput.printf("\nSome tests failed\n");
	}

	return status;
}
