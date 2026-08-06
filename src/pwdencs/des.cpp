// Copyright (c) David Muse
// See the file COPYING for more information

#include <sqlrelay/sqlrutil.h>
#include <rudiments/charstring.h>
#include <rudiments/des.h>

class SQLRUTIL_DLLSPEC sqlrpwdenc_des : public sqlrpwdenc {
	public:
		sqlrpwdenc_des(domnode *parameters, bool debug);
		bool	oneWay();
		char	*encrypt(const char *value);
};

sqlrpwdenc_des::sqlrpwdenc_des(domnode *parameters, bool debug) :
						sqlrpwdenc(parameters,debug) {
}

bool sqlrpwdenc_des::oneWay() {
	return true;
}

char *sqlrpwdenc_des::encrypt(const char *value) {

	// The hasher has to be local to this call.  des::append() accumulates,
	// and getHash() hashes everything accumulated so far without clearing
	// it, so a hasher kept across calls hashes every value it has ever
	// been given, concatenated.  des also truncates at 8 characters, so
	// once 8 have accumulated every later call returns the same hash no
	// matter what it was passed - which, on the one-way comparison path,
	// means any password matches.
	des	d;

	size_t	saltsize=d.getRequiredSaltSize();

	d.setSalt((const byte_t *)
			getParameters()->getAttributeValue("salt"),saltsize);

	d.append((const byte_t *)value,charstring::getLength(value));

	const char	*encrypted=(const char *)d.getHash();

	// the first two characters of the result string are the salt,
	// so don't include them in the result, if possible
	return (charstring::getLength(encrypted)<saltsize)?
			charstring::duplicate(encrypted):
			charstring::duplicate(encrypted+saltsize);
}

extern "C" {
	SQLRUTIL_DLLSPEC sqlrpwdenc *new_sqlrpwdenc_des(
						domnode *parameters,
						bool debug) {
		return new sqlrpwdenc_des(parameters,debug);
	}
}
