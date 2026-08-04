// Copyright (c) David Muse
// See the file COPYING for more information

#include <defaults.h>
#include <sqlrelay/sqlrutil.h>
#include <config.h>

sqlrcmdline::sqlrcmdline(int argc, const char **argv) : commandline(argc,argv) {
	id=getValue("-id");
	if (charstring::isNullOrEmpty(id)) {
		id=DEFAULT_ID;
	}
}

const char *sqlrcmdline::getId() const {
	return id;
}

// the connection options that require a value, shared by every client tool
// (the on/off options aren't here, and neither are -krb and -tls, because a
// bare one of those is legal and means on)
static const char * const	connectionoptions[]={
	"config",
	"id",
	"host",
	"port",
	"socket",
	"user",
	"password",
	"krbservice",
	"krbmech",
	"krbflags",
	"tlsversion",
	"tlscert",
	"tlspassword",
	"tlsciphers",
	"tlsvalidate",
	"tlsca",
	"tlsdepth",
	"localstatedir",
	NULL
};

static const char *valuelessOption(commandline *cmdline,
					const char * const *options) {

	// getValue() returns an empty string for an option with no value, for
	// an option followed by another option, and for an option that isn't
	// there at all, so isFound() is what decides presence
	for (const char * const *o=options; o && *o; o++) {
		if (cmdline->isFound(*o) &&
			charstring::isNullOrEmpty(cmdline->getValue(*o))) {
			return *o;
		}
	}
	return NULL;
}

const char *sqlrcmdline::missingValueOption(const char * const *extraoptions) {
	const char	*o=valuelessOption(this,connectionoptions);
	if (!o) {
		o=valuelessOption(this,extraoptions);
	}
	return o;
}
