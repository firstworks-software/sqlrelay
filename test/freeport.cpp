// Copyright (c) David Muse
// See the file COPYING for more information

// Prints free tcp ports, one per line, so that a test run can put its
// listeners somewhere nothing else on the host is using.
//
// Each port comes from listening on port 0 and asking the kernel which port
// it picked.  All of them are held open at once and only closed on exit, so
// no two ports printed by one run can be the same.
//
// Nothing stops something else on the host from taking one of them in the gap
// between this exiting and the listener starting.  That window is narrow, and
// it is the same window every "find a free port" scheme has, but it is real.

#include <rudiments/inetsocketserver.h>
#include <rudiments/charstring.h>
#include <rudiments/stdio.h>

int main(int argc, char **argv) {

	// how many ports to print
	int32_t	count=1;
	if (argc>1) {
		count=(int32_t)charstring::convertToInteger(argv[1]);
		if (count<1 || count>1024) {
			stderror.printf("usage: freeport [count]\n");
			return 1;
		}
	}

	inetsocketserver	*socks=new inetsocketserver[count];

	// bind them all before printing any, so they can't collide
	for (int32_t i=0; i<count; i++) {
		if (!socks[i].listen(NULL,0,1)) {
			stderror.printf("failed to get a free port\n");
			delete[] socks;
			return 1;
		}
	}

	for (int32_t i=0; i<count; i++) {
		stdoutput.printf("%d\n",(int32_t)socks[i].getPort());
	}

	delete[] socks;
	return 0;
}
