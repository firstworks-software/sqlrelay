// Copyright (c) David Muse
// See the file COPYING for more information

#include "firebirdarc4.h"

#include <rudiments/bytestring.h>

// Arc4.cpp's Arc4Key::createKey() - textbook RC4 key scheduling.  The key is
// used raw, with no hashing and no "RC4-drop" discard of the first bytes of
// keystream.
firebirdarc4::firebirdarc4(const unsigned char *key, size_t keylen) {

	for (uint16_t n=0; n<256; n++) {
		_state[n]=(unsigned char)n;
	}

	if (key && keylen) {
		unsigned char	k2=0;
		for (uint16_t k1=0; k1<256; k1++) {
			k2=(unsigned char)(k2+key[k1%keylen]+_state[k1]);
			unsigned char	tmp=_state[k1];
			_state[k1]=_state[k2];
			_state[k2]=tmp;
		}
	}

	_s1=0;
	_s2=0;
}

// The state array is the key schedule, and the key schedule is key
// material - the rest of the module zeroes key bytes on the way out, so
// this does too.
firebirdarc4::~firebirdarc4() {
	bytestring::zero(_state,sizeof(_state));
	_s1=0;
	_s2=0;
}

// Arc4.cpp's Arc4Key::getBytes() - textbook RC4 pseudo-random generation,
// applied byte-by-byte as a keystream that's XORed against the buffer.
// s1/s2 are unsigned char so they wrap mod 256 on their own, and calling
// this again later just continues the same keystream - nothing here resets
// between calls, which is what lets one instance run across a whole stream
// of reads or writes.
void firebirdarc4::crypt(unsigned char *buffer, size_t len) {
	for (size_t i=0; i<len; i++) {
		_s1++;
		_s2=(unsigned char)(_s2+_state[_s1]);
		unsigned char	tmp=_state[_s1];
		_state[_s1]=_state[_s2];
		_state[_s2]=tmp;
		unsigned char	k=_state[(unsigned char)(_state[_s1]+_state[_s2])];
		buffer[i]=(unsigned char)(buffer[i]^k);
	}
}
