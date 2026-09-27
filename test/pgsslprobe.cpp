// Copyright (c) David Muse
// See the file COPYING for more information

// test.sh's listenerdisabled suite (#10308) shells out to this to confirm
// that a <listener> tag configured after a disabled one is still paired
// with its own protocol module (the regression fixed by #10305), without
// needing a real psql client.
//
// sqlrprotocol_postgresql answers a client's SSLRequest packet with a
// single 'N' byte (TLS not offered) before any other exchange happens, so
// that byte fingerprints the postgresql protocol module specifically - a
// listener paired with the wrong module, or with none at all, won't
// produce it.  This is the same raw-packet probe used to verify #10305 by
// hand.
//
// usage: pgsslprobe <port>
//
// prints the single reply byte to stdout and exits 0 on success; exits 1
// and prints nothing to stdout if the connection failed or no byte arrived
// in time.

#include <rudiments/inetsocketclient.h>
#include <rudiments/charstring.h>
#include <rudiments/stdio.h>

int main(int argc, char **argv) {

	if (argc<2) {
		stderror.printf("usage: pgsslprobe <port>\n");
		return 1;
	}
	uint16_t	port=(uint16_t)charstring::convertToInteger(argv[1]);

	inetsocketclient	s;
	s.setHost("127.0.0.1");
	s.setPort(port);
	s.setTimeoutSeconds(5);
	s.setTimeoutMicroseconds(0);
	if (s.connect()!=RESULT_SUCCESS) {
		stderror.printf("connect failed\n");
		return 1;
	}

	// the postgresql SSLRequest packet: a 4-byte big-endian length (8),
	// followed by the 4-byte big-endian SSL request code 80877103
	// (0x04d2162f)
	unsigned char	packet[8]={0,0,0,8,0x04,0xd2,0x16,0x2f};
	if (s.write(packet,sizeof(packet))!=(ssize_t)sizeof(packet)) {
		stderror.printf("write failed\n");
		s.close();
		return 1;
	}

	unsigned char	reply=0;
	if (s.read(&reply,5,0)!=1) {
		stderror.printf("no reply\n");
		s.close();
		return 1;
	}
	s.close();

	stdoutput.printf("%c",(char)reply);
	return 0;
}
