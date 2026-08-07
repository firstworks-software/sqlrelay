// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/inetsocketclient.h>
#include <rudiments/commandline.h>
#include <rudiments/charstring.h>
#include <rudiments/stdio.h>

int main(int argc, const char **argv) {

	commandline	cmdl(argc,argv);

	// default to the oracletest instance
	const char	*host="sqlrelay";
	uint16_t	port=9001;

	if (cmdl.isFound("host")) {
		host=cmdl.getValue("host");
	}
	if (cmdl.isFound("port")) {
		port=charstring::convertToUnsignedInteger(
						cmdl.getValue("port"));
	}

	inetsocketclient	cl;
	cl.setHost(host);
	cl.setPort(port);
	uint32_t i=0;
	while (true) {
		stdoutput.printf("%d: ",i);
		if (!cl.connect()) {
			stdoutput.printf("failed\n");
			break;
		}
		stdoutput.printf("success\n");
		cl.close();
		i++;
	}
}
