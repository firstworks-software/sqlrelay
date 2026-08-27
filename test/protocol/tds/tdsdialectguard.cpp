// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/inetsocketclient.h>
#include <rudiments/bytebuffer.h>
#include <rudiments/bytestring.h>
#include <rudiments/charstring.h>
#include <rudiments/stdio.h>

// Regression coverage for ticket #9481: recvPacket() in src/protocols/tds.cpp
// used to accept SQL_BATCH (0x01), BULK_LOAD_DATA (0x07) and RPC (0x03)
// packets from a session that logged in speaking pre-tds7 (tds 5.0), even
// though sqlBatch(), bulkLoad() and remoteProcedureCall() each parse and
// write pure ms-tds (tds 7.x) data.  Those three now start with a guard
// that refuses the packet with a tds protocol error when pretds7 is set.
//
// A real client can't reach this - it only ever sends packet types its own
// dialect uses - so this drives the wire protocol directly: a hand-built
// pre-tds7 login, then a hand-crafted SQL_BATCH packet behind it, bypassing
// ct-lib/FreeTDS entirely.  This mirrors how
// test/protocol/mysql/mysql.cpp's sendMalformedHandshakeResponse() drives
// the mysql wire protocol directly for the #9047/#9048 regressions.

// packet header fields - see PACKET_HEADER_SIZE, STATUS_EOM and the packet
// type constants in src/protocols/tds.cpp
static const size_t		PACKET_HEADER_SIZE=8;
static const unsigned char	STATUS_EOM=0x01;
static const unsigned char	PRE_TDS7_LOGIN=0x02;
static const unsigned char	SQL_BATCH=0x01;
static const unsigned char	TABULAR_RESULT=0x04;
static const unsigned char	TOKEN_LOGIN_ACK=0xAD;
static const unsigned char	TOKEN_EED=0xE5;

// pre-tds7 login record field sizes - see the PRE_TDS7_*_SIZE defines and
// preTds7Login() in src/protocols/tds.cpp
static const size_t	NAME_SIZE=30;
static const size_t	REMOTE_PASSWORD_SIZE=255;
static const size_t	PROGNAME_SIZE=10;
static const size_t	PACKET_SIZE_SIZE=6;
static const size_t	TYPE_FLAGS_SIZE=6;
static const size_t	SPARE_SIZE=3;
static const size_t	SESSION_ID_SIZE=6;
static const size_t	SEC_SPARE_SIZE=2;
static const size_t	DUMMY_SIZE=4;
static const size_t	PRE_TDS7_LOGIN_SIZE=568;

int	status=0;
const char	*success="\033[32msuccess\033[0m";
const char	*failure="\033[31mfailure\033[0m";

static void report(const char *label, bool ok) {
	stdoutput.printf("%s: %s\n",label,(ok)?success:failure);
	if (!ok) {
		status=1;
	}
}

// encode an ascii string as ucs-2 (utf-16le), one 16-bit little-endian code
// unit per character, high byte 0 - the format sqlBatch() in
// src/protocols/tds.cpp expects for the sql it reads out of a SQL_BATCH
// packet.  There's no length prefix - sqlBatch() takes the whole remaining
// packet as the query (rpsize/sizeof(ucs2_t) code units), after stripping
// ALL_HEADERS when negotiatedtdsversion>=720.  This test's pre-tds7 login
// negotiates tds 5.0, well under that, so no ALL_HEADERS block belongs here.
static void appendUcs2String(bytebuffer *body, const char *value) {
	for (const char *ch=value; *ch; ch++) {
		body->append((unsigned char)*ch);
		body->append((unsigned char)0);
	}
}

// append a fixed-size, nul-padded field followed by its trailing length
// byte - the layout readPreTds7Field() in src/protocols/tds.cpp expects
static void appendField(bytebuffer *body, const char *value, size_t size) {
	size_t	len=charstring::getLength(value);
	if (len>size) {
		len=size;
	}
	unsigned char	buffer[256];
	bytestring::zero(buffer,size);
	bytestring::copy(buffer,value,len);
	body->append(buffer,size);
	body->append((unsigned char)len);
}

// build a valid pre-tds7 (tds 5.0) login record - field order and sizes
// mirror the reads in sqlrprotocol_tds::preTds7Login()
static void buildPreTds7Login(bytebuffer *body,
				const char *username, const char *password) {

	appendField(body,"",NAME_SIZE);			// hostname
	appendField(body,username,NAME_SIZE);
	appendField(body,password,NAME_SIZE);
	appendField(body,"",NAME_SIZE);			// hostproc

	// Typeflags - where the client declares the byte order it lays
	// multi-byte values out in.  These are the six bytes a real ct-lib
	// client on x86 sends: little-endian 2-byte ints, little-endian
	// 4-byte ints, ascii, little-endian ieee floats, little-endian
	// date/time, and the usedb flag.  See preTds7ByteOrder() in
	// src/protocols/tds.cpp.
	//
	// These have to be filled in rather than zeroed.  A zeroed block
	// isn't a valid declaration at all, and the 4-byte-int byte in
	// particular reads as big-endian when it's 0, which is not what
	// this test's client - the c api on x86 - wants.
	unsigned char	typeflags[TYPE_FLAGS_SIZE]=
				{0x03,0x01,0x06,0x0a,0x09,0x01};
	body->append(typeflags,sizeof(typeflags));

	body->append((unsigned char)0);			// dumpload
	body->append((unsigned char)0);			// interfacespare
	body->append((unsigned char)0);			// type

	unsigned char	deprecated[4]={0,0,0,0};
	body->append(deprecated,sizeof(deprecated));

	unsigned char	spare[SPARE_SIZE];
	bytestring::zero(spare,sizeof(spare));
	body->append(spare,sizeof(spare));

	appendField(body,"sqlrelaytest",NAME_SIZE);		// appname
	appendField(body,"sqlrelaytest",NAME_SIZE);		// servername
	appendField(body,"",REMOTE_PASSWORD_SIZE);		// remotepassword

	// tdsversion, big-endian: 0x05000000 is tds 5.0 - see
	// tdsVersionHexToDec() in src/protocols/tds.cpp
	unsigned char	tdsversion[4]={0x05,0x00,0x00,0x00};
	body->append(tdsversion,sizeof(tdsversion));

	appendField(body,"sqlrtest",PROGNAME_SIZE);		// progname

	unsigned char	progversion[4]={0,0,0,0};
	body->append(progversion,sizeof(progversion));

	body->append((unsigned char)0);			// noshort
	body->append((unsigned char)0);			// flt4type
	body->append((unsigned char)0);			// date4type

	appendField(body,"",NAME_SIZE);			// language

	body->append((unsigned char)0);			// suppresslanguage

	unsigned char	oldsecure[2]={0,0};
	body->append(oldsecure,sizeof(oldsecure));

	body->append((unsigned char)0);			// seclogin: plaintext
	body->append((unsigned char)0);			// secbulk
	body->append((unsigned char)0);			// halogin

	unsigned char	hasessionid[SESSION_ID_SIZE];
	bytestring::zero(hasessionid,sizeof(hasessionid));
	body->append(hasessionid,sizeof(hasessionid));

	unsigned char	secspare[SEC_SPARE_SIZE];
	bytestring::zero(secspare,sizeof(secspare));
	body->append(secspare,sizeof(secspare));

	appendField(body,"",NAME_SIZE);			// charset

	body->append((unsigned char)0);			// charsetchange

	appendField(body,"512",PACKET_SIZE_SIZE);		// packetsize

	unsigned char	dummy[DUMMY_SIZE];
	bytestring::zero(dummy,sizeof(dummy));
	body->append(dummy,sizeof(dummy));
}

// write one tds packet: an 8 byte header (type, status, big-endian size,
// spid, packet id, window) followed by "datasize" bytes of payload - see
// sqlrprotocol_tds::sendPacket()/recvPacket()
static bool sendTdsPacket(inetsocketclient *sock, unsigned char packettype,
				const unsigned char *data, size_t datasize) {

	uint16_t	packetsize=(uint16_t)(datasize+PACKET_HEADER_SIZE);

	unsigned char	header[PACKET_HEADER_SIZE];
	header[0]=packettype;
	header[1]=STATUS_EOM;
	header[2]=(unsigned char)((packetsize>>8)&0xff);
	header[3]=(unsigned char)(packetsize&0xff);
	header[4]=0;		// spid
	header[5]=0;
	header[6]=1;		// packet id
	header[7]=0;		// window

	if (sock->write(header,sizeof(header))!=(ssize_t)sizeof(header)) {
		return false;
	}
	if (datasize && sock->write(data,datasize)!=(ssize_t)datasize) {
		return false;
	}
	return true;
}

// a response is expected within this long; used on every read below so a
// server that hangs instead of answering fails the test instead of hanging
// it
static const int32_t	RESPONSE_TIMEOUT_SEC=10;

// read one tds response packet, returning its packet type and the first
// byte of its payload (the token id, for the responses this test expects).
// A single packet is enough here - the login ack and the protocol error are
// both far smaller than the negotiated packet size, so both come back with
// STATUS_EOM already set on the first (and only) packet.
static bool readTdsPacketFirstToken(inetsocketclient *sock,
					unsigned char *packettype,
					unsigned char *firsttoken) {

	unsigned char	header[PACKET_HEADER_SIZE];
	if (sock->read(header,sizeof(header),RESPONSE_TIMEOUT_SEC,0)!=
					(ssize_t)sizeof(header)) {
		return false;
	}
	*packettype=header[0];
	size_t	packetsize=((size_t)header[2]<<8)|(size_t)header[3];
	if (packetsize<PACKET_HEADER_SIZE) {
		return false;
	}
	size_t	datasize=packetsize-PACKET_HEADER_SIZE;
	if (!datasize) {
		return false;
	}
	if (sock->read(firsttoken,1,RESPONSE_TIMEOUT_SEC,0)!=1) {
		return false;
	}
	datasize--;

	unsigned char	discard[4096];
	while (datasize) {
		size_t	chunk=(datasize>sizeof(discard))?
					sizeof(discard):datasize;
		if (sock->read(discard,chunk,RESPONSE_TIMEOUT_SEC,0)!=
						(ssize_t)chunk) {
			return false;
		}
		datasize-=chunk;
	}
	return true;
}

int main(int argc, char **argv) {

	stdoutput.printf("\n====== #9481 pre-tds7 dialect guard ======\n\n");

	// the tdssapprotocol test instance - see
	// test/sqlrelay.conf.d/tdssapprotocol.conf; it has no <auths> block,
	// so it falls back to a "tds_connectstrings" auth, which accepts the
	// user/password embedded in its <connection> string
	const char	*host="127.0.0.1";
	uint16_t	port=9031;
	const char	*user="testuser";
	const char	*password="testpassword";

	inetsocketclient	sock;
	sock.setHost(host);
	sock.setPort(port);
	if (!sock.connect()) {
		report("connect",false);
		return status;
	}

	bytebuffer	loginbody;
	buildPreTds7Login(&loginbody,user,password);
	if (loginbody.getSize()!=PRE_TDS7_LOGIN_SIZE) {
		stdoutput.printf("built a %d byte login record, expected %d\n",
					(int)loginbody.getSize(),
					(int)PRE_TDS7_LOGIN_SIZE);
		report("pre-tds7 login record size",false);
		sock.close();
		return status;
	}

	if (!sendTdsPacket(&sock,PRE_TDS7_LOGIN,
				(const unsigned char *)loginbody.getBuffer(),
				loginbody.getSize())) {
		report("send pre-tds7 login",false);
		sock.close();
		return status;
	}

	unsigned char	packettype=0;
	unsigned char	firsttoken=0;
	bool	gotresponse=readTdsPacketFirstToken(&sock,
						&packettype,&firsttoken);
	report("pre-tds7 login accepted",
			gotresponse && packettype==TABULAR_RESULT &&
					firsttoken==TOKEN_LOGIN_ACK);

	// an ms-tds-only packet type, sent behind the pre-tds7 login just
	// completed above.  The payload is a real, valid "select 1" query,
	// ucs-2 encoded exactly as an ms-tds client would send it - a bare
	// empty payload isn't enough to prove the guard works: sqlBatch()
	// would decode it as a zero-length query, fail prepare/execute for
	// that unrelated reason, and appendQueryError() would write
	// TOKEN_EED anyway, even on a build without the guard.  A real
	// query only comes back as TOKEN_EED up front if the pretds7
	// guard actually fires before query execution; on an unguarded
	// build it runs and returns column metadata instead.
	bytebuffer	sqlbatchbody;
	appendUcs2String(&sqlbatchbody,"select 1");
	if (!sendTdsPacket(&sock,SQL_BATCH,
				(const unsigned char *)sqlbatchbody.getBuffer(),
				sqlbatchbody.getSize())) {
		report("send sql batch behind pre-tds7 login",false);
		sock.close();
		return status;
	}

	gotresponse=readTdsPacketFirstToken(&sock,&packettype,&firsttoken);
	report("sql batch refused with a protocol error",
			gotresponse && packettype==TABULAR_RESULT &&
					firsttoken==TOKEN_EED);

	// pre-fix, sqlBatch() would actually execute this query and answer
	// with column metadata (TOKEN_COLMETADATA) instead of refusing it;
	// the error response above, arriving well inside
	// RESPONSE_TIMEOUT_SEC and correctly framed, is what rules that
	// out.  sqlrservercontroller::closeClientConnection() then waits
	// for the client to close its end before it closes its own, so
	// this test does that instead of reading for a server-side close.
	sock.close();

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
