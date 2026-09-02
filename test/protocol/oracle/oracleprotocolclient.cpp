// Copyright (c) David Muse
// See the file COPYING for more information.

// A hand-built oracle wire protocol client - the tns/tti bytes, written
// straight onto a socket, with no OCI and no thin driver in between.
//
// It exists because some paths in src/protocols/oracle.cpp can't be reached
// through a real client.  OCI answers an OCI_DESCRIBE_ONLY out of its own
// client-side cache once a statement has executed, so it never puts a
// describe-only TTI_QUERY3 on the wire for an already-executed cursor, and
// the guard in query3() that refuses to re-execute one goes untested.  This
// class puts those exact bytes on the wire instead.  It mirrors what
// test/protocol/tds/tdsdialectguard.cpp does for the analogous tds guard.
//
// This file is meant to be #include'd by a test's main(), the same way
// asserts.cpp is - see oracledescribeonly.cpp.
//
// The byte layouts here were read off two things, and both are worth
// checking against before changing anything:
//	- test/testdetails-oracleprotocol.log, which carries full hex dumps
//	  of a real OCI client's traffic against this listener, because
//	  test/sqlrelay.conf.d/oracleprotocol.conf.in sets debug="protocols"
//	- the parsers in src/protocols/oracle.cpp themselves -
//	  recvConnectRequest(), recvTtiRequest(), recvDataTypeRequest(),
//	  recvAuthenticationRequest(), open(), getQuery3Request() and
//	  fetch3()
//
// The o5logon math is the client side of src/auths/oracle_userlist.cpp's
// passwordHash(), o5logonChallenge(), o5logonComboKey() and o5logonVerify().
// Only the 11g path is implemented: the oracleprotocol listener pins
// serverversion="11.2", which defaults verifiertype to VERIFIER_TYPE_11G_2.

#include <rudiments/inetsocketclient.h>
#include <rudiments/bytebuffer.h>
#include <rudiments/bytestring.h>
#include <rudiments/charstring.h>
#include <rudiments/sha1.h>
#include <rudiments/md5.h>
#include <rudiments/aes192.h>
#include <rudiments/csprng.h>
#include <rudiments/stdio.h>

// ns layer packet types - the PACKET_* defines in src/protocols/oracle.cpp
static const unsigned char	ORA_PACKET_CONNECT=1;
static const unsigned char	ORA_PACKET_ACCEPT=2;
static const unsigned char	ORA_PACKET_REFUSE=4;
static const unsigned char	ORA_PACKET_DATA=6;
static const unsigned char	ORA_PACKET_RESEND=11;

// two task common codes - the TTC_* defines in src/protocols/oracle.cpp
static const unsigned char	ORA_TTC_PROTOCOL_NEGOTIATION=0x01;
static const unsigned char	ORA_TTC_DATATYPE_NEGOTIATION=0x02;
static const unsigned char	ORA_TTC_TTI_FUNCTION=0x03;
static const unsigned char	ORA_TTC_ERROR=0x04;
static const unsigned char	ORA_TTC_ROW_HEADER=0x06;
static const unsigned char	ORA_TTC_ROW_DATA=0x07;
static const unsigned char	ORA_TTC_OK=0x08;
static const unsigned char	ORA_TTC_DESCRIBE_INFO=0x10;

// two task interface functions - the TTI_* defines in
// src/protocols/oracle.cpp
static const unsigned char	ORA_TTI_OPEN=0x02;
static const unsigned char	ORA_TTI_QUERY=0x03;
static const unsigned char	ORA_TTI_EXECUTE=0x04;
static const unsigned char	ORA_TTI_FETCH=0x05;
static const unsigned char	ORA_TTI_DISCONNECT=0x09;
static const unsigned char	ORA_TTI_QUERY3=0x5e;
static const unsigned char	ORA_TTI_LOGON_PRESENT_PWD_SEND_AUTH_PASSWORD=0x73;
static const unsigned char	ORA_TTI_LOGON_PRESENT_USER_REQ_AUTH_SESSKEY=0x76;

// query3 options - the OPTION_* defines in src/protocols/oracle.cpp
static const uint32_t	ORA_OPTION_PARSE=(1<<0);
static const uint32_t	ORA_OPTION_DEFINE=(1<<4);
static const uint32_t	ORA_OPTION_EXECUTE=(1<<5);
static const uint32_t	ORA_OPTION_FETCH=(1<<6);
static const uint32_t	ORA_OPTION_EXACTFETCH=(1<<9);
static const uint32_t	ORA_OPTION_SNDIOV=(1<<10);
static const uint32_t	ORA_OPTION_NOPLSQL=(1<<15);
static const uint32_t	ORA_OPTION_DESCRIBE=(1<<17);

// PROTOCOL_VERSION_12 in src/protocols/oracle.cpp.  sendAccept() switches
// to a 32-bit packet length at and above it, so a client that offers it has
// to switch too - on the packet after the accept, not on the accept itself
static const uint16_t	ORA_PROTOCOL_VERSION_12=0x013b;

// NSI_NA_NO_SERVICES in src/protocols/oracle.cpp, written into both flag
// bytes.  anoNegotiation() only runs an ano exchange for a client that set
// NSI_NA_WANTED (0x01), and this client has no use for one, so it says so
// and the handshake goes straight from the accept to the tti negotiation
static const uint16_t	ORA_ANO_FLAGS_NO_SERVICES=0x0808;

// AL32UTF8, which is what the listener answers with whatever the client
// asks for - see the charset parameter in src/protocols/oracle.cpp
static const uint16_t	ORA_CHARSET_AL32UTF8=873;

// CCAP_FIELD_VERSION and CCAP_FIELD_VERSION_11_2 in src/protocols/oracle.cpp
static const size_t		ORA_CCAP_FIELD_VERSION=7;
static const unsigned char	ORA_CCAP_FIELD_VERSION_11_2=6;
static const size_t		ORA_CCAP_SIZE=42;
static const size_t		ORA_RCAP_SIZE=7;

// o5logon sizes - the 11g half of src/auths/oracle_userlist.cpp
static const size_t	ORA_SESSION_KEY_SIZE_11G=48;
static const size_t	ORA_SESSION_KEY_PAD_SIZE_11G=8;
static const size_t	ORA_PASSWORD_HASH_SIZE_11G=24;
static const size_t	ORA_COMBO_KEY_SIZE_11G=24;

// a response is expected within this long; used on every read, so a
// listener that hangs instead of answering fails the test rather than
// hanging it
static const int32_t	ORA_RESPONSE_TIMEOUT_SEC=30;

// no packet this client sends or receives comes anywhere near this - the
// listener caps its own at the negotiated sdu, 8192
static const size_t	ORA_MAX_PACKET_SIZE=65536;

class oracleprotocolclient {
	public:
			oracleprotocolclient();
			~oracleprotocolclient();

		// the whole handshake through the accept: the connect
		// packet (twice - the listener asks for a resend, the way a
		// real database does), then the tti protocol and data type
		// negotiations
		bool	connect(const char *host, uint16_t port,
							const char *sid);

		// the two phase o5logon exchange, 11g verifier
		bool	login(const char *user, const char *password);

		// TTI_DISCONNECT, then close the socket
		void	disconnect();

		// TTI_OPEN.  "cursorid" comes back as the id the wire uses,
		// which is the listener's own id plus one
		bool	open(uint32_t *cursorid);

		// TTI_QUERY3.  "query" may be NULL, which is what a call
		// that only describes or only executes an already-parsed
		// cursor sends
		bool	query3(uint32_t options, uint32_t cursorid,
					uint32_t prefetchrows,
					const char *query);

		// TTI_FETCH
		bool	fetch(uint32_t cursorid, uint32_t rowstofetch);

		// the pre-query3 calls: TTI_QUERY parses, TTI_EXECUTE
		// executes what it parsed, and the legacy TTI_FETCH asks
		// for the rows.  a session that only ever sends these
		// leaves query3session false in src/protocols/oracle.cpp,
		// which is what keeps fetch() on the legacy
		// sendFetchResponse() rather than handing off to fetch3().
		// "cursorid" is the wire id open() handed back, not the
		// listener's own - the legacy fetch takes none at all and
		// answers for whichever cursor open/query/execute touched
		// last
		bool	legacyQuery(uint16_t options, uint16_t moreoptions,
					uint16_t cursorid, const char *query);
		bool	legacyExecute(uint16_t options, uint16_t moreoptions,
						uint16_t cursorid);
		bool	legacyFetch(uint16_t options, uint16_t moreoptions);

		// what the last call answered with.  the ttc code is the
		// first thing in a response body, behind the two data flag
		// bytes, so it identifies the response as a whole; the
		// substring search is how a test asks whether a particular
		// column value came back, since a character column's bytes
		// go out as-is
		const unsigned char	*getResponse();
		size_t			getResponseSize();
		unsigned char		getResponseTtcCode();
		bool			responseContains(const char *text);

		// what went wrong, for a method that returned false
		const char	*getError();

		// the building blocks the calls above are made of, public so
		// a test that needs a call this class doesn't implement -
		// the legacy TTI_QUERY/TTI_EXECUTE path, say - can build one
		// without reimplementing the handshake
		void	beginPacket(unsigned char packettype);
		void	beginTtiCall(unsigned char ttifunction);
		void	appendByte(unsigned char value);
		void	appendBytes(const unsigned char *value, size_t size);
		void	appendBE16(uint16_t value);
		void	appendBE32(uint32_t value);
		void	appendLE16(uint16_t value);
		void	appendLenPreInt(uint32_t value);
		void	appendLenString(const char *value, size_t size);
		bool	sendPacket();
		bool	recvPacket();
		unsigned char	getPacketType();

		// response walking, in the same spirit - a test that has to
		// pick a response apart field by field, rather than just
		// search it for a value, needs these
		void	rewindResponse();
		bool	readByte(unsigned char *value);
		bool	readBytes(unsigned char *value, size_t size);
		bool	readLenPreInt(uint32_t *value);
		bool	readLenString(char **value);

	private:
		void	setError(const char *message);
		void	setError(const char *message, const char *detail);

		bool	sendConnect(const char *sid);
		bool	sendProtocolNegotiation();
		bool	sendDataTypeNegotiation();
		void	sendAuthRequest(unsigned char ttifunction,
					const char *user,
					uint32_t authmode,
					uint32_t fieldcount);
		void	appendAuthField(const char *name,
					const char *value,
					uint32_t flags);
		bool	findAuthField(const char *name, char **value);

		inetsocketclient	sock;
		bool			connected;
		bool			largeheader;

		bytebuffer	reqpacket;
		unsigned char	reqpackettype;

		unsigned char	*resppacket;
		size_t		respsize;
		unsigned char	resppackettype;
		size_t		respposition;

		char		errormessage[1024];
};

// aes-cbc with a zero iv and no padding, exactly as
// src/auths/oracle_userlist.cpp's aesCbc() does it.  the 11g verifier only
// ever uses a 24-byte key, so this is aes192 and nothing else
static bool oracleAes192Cbc(bool encrypt,
				const unsigned char *key,
				const unsigned char *in, size_t insize,
				unsigned char *out) {

	if (insize%16) {
		return false;
	}

	unsigned char	iv[16];
	bytestring::zero(iv,sizeof(iv));

	aes192	enc;
	enc.setUsePadding(false);
	if (!enc.setKey(key,24) || !enc.setIv(iv,sizeof(iv)) ||
					!enc.append(in,(uint32_t)insize)) {
		return false;
	}

	const unsigned char	*result=(encrypt)?
					enc.getEncryptedData():
					enc.getDecryptedData();
	uint64_t		resultsize=(encrypt)?
					enc.getEncryptedDataSize():
					enc.getDecryptedDataSize();
	if (!result || resultsize!=insize) {
		return false;
	}
	bytestring::copy(out,result,insize);
	return true;
}

// uppercase hex, which is what every AUTH_ field on the wire is in
static char *oracleHexEncodeUpper(const unsigned char *in, size_t insize) {
	char	*hex=charstring::hexEncode(in,insize);
	charstring::upper(hex);
	return hex;
}

oracleprotocolclient::oracleprotocolclient() {
	connected=false;
	largeheader=false;
	reqpackettype=0;
	resppacket=new unsigned char[ORA_MAX_PACKET_SIZE];
	respsize=0;
	resppackettype=0;
	respposition=0;
	errormessage[0]='\0';
}

oracleprotocolclient::~oracleprotocolclient() {
	if (connected) {
		sock.close();
	}
	delete[] resppacket;
}

void oracleprotocolclient::setError(const char *message) {
	charstring::copy(errormessage,message,sizeof(errormessage)-1);
	errormessage[sizeof(errormessage)-1]='\0';
}

void oracleprotocolclient::setError(const char *message, const char *detail) {
	charstring::printf(errormessage,sizeof(errormessage),
				"%s: %s",message,detail);
}

const char *oracleprotocolclient::getError() {
	return errormessage;
}

const unsigned char *oracleprotocolclient::getResponse() {
	return resppacket;
}

size_t oracleprotocolclient::getResponseSize() {
	return respsize;
}

unsigned char oracleprotocolclient::getPacketType() {
	return resppackettype;
}

// the first byte behind the two data flag bytes every data packet starts
// with
unsigned char oracleprotocolclient::getResponseTtcCode() {
	return (respsize>=3)?resppacket[2]:0;
}

bool oracleprotocolclient::responseContains(const char *text) {
	size_t	textsize=charstring::getLength(text);
	if (!textsize || respsize<textsize) {
		return false;
	}
	for (size_t i=0; i<=respsize-textsize; i++) {
		if (!bytestring::compare(resppacket+i,text,textsize)) {
			return true;
		}
	}
	return false;
}


// ---- packet building and framing ----

// 8 placeholder bytes for the header, which sendPacket() overwrites once
// the body's size is known - the same thing
// sqlrprotocol_oracle::resetSendPacketBuffer() does
void oracleprotocolclient::beginPacket(unsigned char packettype) {
	reqpacket.clear();
	unsigned char	header[8];
	bytestring::zero(header,sizeof(header));
	reqpacket.append(header,sizeof(header));
	reqpackettype=packettype;
}

// a data packet carrying one tti call: the data flags, then the ttc code
// that says a tti function follows, then the function itself
void oracleprotocolclient::beginTtiCall(unsigned char ttifunction) {
	beginPacket(ORA_PACKET_DATA);
	appendBE16(0);
	appendByte(ORA_TTC_TTI_FUNCTION);
	appendByte(ttifunction);
}

void oracleprotocolclient::appendByte(unsigned char value) {
	reqpacket.append(value);
}

void oracleprotocolclient::appendBytes(const unsigned char *value,
							size_t size) {
	reqpacket.append(value,size);
}

void oracleprotocolclient::appendBE16(uint16_t value) {
	reqpacket.append((unsigned char)((value>>8)&0xff));
	reqpacket.append((unsigned char)(value&0xff));
}

void oracleprotocolclient::appendBE32(uint32_t value) {
	reqpacket.append((unsigned char)((value>>24)&0xff));
	reqpacket.append((unsigned char)((value>>16)&0xff));
	reqpacket.append((unsigned char)((value>>8)&0xff));
	reqpacket.append((unsigned char)(value&0xff));
}

// the one field on the legacy query path that isn't big-endian: query()
// in src/protocols/oracle.cpp reads its query size with readLE()
void oracleprotocolclient::appendLE16(uint16_t value) {
	reqpacket.append((unsigned char)(value&0xff));
	reqpacket.append((unsigned char)((value>>8)&0xff));
}

// a count byte, then that many big-endian bytes - the mirror of
// sqlrprotocol::writeLenPreInt()/readLenPreInt() in
// src/server/sqlrprotocol.cpp
void oracleprotocolclient::appendLenPreInt(uint32_t value) {
	if (!value) {
		appendByte(0);
	} else if (value<=0xff) {
		appendByte(1);
		appendByte((unsigned char)value);
	} else if (value<=0xffff) {
		appendByte(2);
		appendBE16((uint16_t)value);
	} else {
		appendByte(4);
		appendBE32(value);
	}
}

// a text - one length byte, then that many bytes.  the long form is only
// needed past 252 bytes and nothing here goes that far
void oracleprotocolclient::appendLenString(const char *value, size_t size) {
	appendByte((unsigned char)size);
	reqpacket.append(value,size);
}

bool oracleprotocolclient::sendPacket() {

	size_t		packetsize=reqpacket.getSize();
	unsigned char	*buffer=(unsigned char *)reqpacket.getBuffer();

	// a 32-bit length at PROTOCOL_VERSION_12 and above, a 16-bit length
	// and a zero packet checksum below it - see
	// sqlrprotocol_oracle::sendPacket()
	if (largeheader) {
		buffer[0]=(unsigned char)((packetsize>>24)&0xff);
		buffer[1]=(unsigned char)((packetsize>>16)&0xff);
		buffer[2]=(unsigned char)((packetsize>>8)&0xff);
		buffer[3]=(unsigned char)(packetsize&0xff);
	} else {
		buffer[0]=(unsigned char)((packetsize>>8)&0xff);
		buffer[1]=(unsigned char)(packetsize&0xff);
		buffer[2]=0;
		buffer[3]=0;
	}
	buffer[4]=reqpackettype;
	buffer[5]=0;		// packet flags
	buffer[6]=0;		// header checksum
	buffer[7]=0;

	if (sock.write(buffer,packetsize)!=(ssize_t)packetsize) {
		setError("failed to write packet");
		return false;
	}
	sock.flushWriteBuffer(-1,-1);
	return true;
}

bool oracleprotocolclient::recvPacket() {

	unsigned char	header[8];
	if (sock.read(header,sizeof(header),
			ORA_RESPONSE_TIMEOUT_SEC,0)!=(ssize_t)sizeof(header)) {
		setError("failed to read packet header");
		return false;
	}

	size_t	packetsize=0;
	if (largeheader) {
		packetsize=((size_t)header[0]<<24)|((size_t)header[1]<<16)|
				((size_t)header[2]<<8)|(size_t)header[3];
	} else {
		packetsize=((size_t)header[0]<<8)|(size_t)header[1];
	}
	resppackettype=header[4];

	if (packetsize<sizeof(header) || packetsize>ORA_MAX_PACKET_SIZE) {
		setError("bad packet size");
		return false;
	}

	respsize=packetsize-sizeof(header);
	respposition=0;
	if (respsize && sock.read(resppacket,respsize,
			ORA_RESPONSE_TIMEOUT_SEC,0)!=(ssize_t)respsize) {
		setError("failed to read packet body");
		return false;
	}
	return true;
}


// ---- response walking ----

void oracleprotocolclient::rewindResponse() {
	respposition=0;
}

bool oracleprotocolclient::readByte(unsigned char *value) {
	if (respposition>=respsize) {
		return false;
	}
	*value=resppacket[respposition];
	respposition++;
	return true;
}

bool oracleprotocolclient::readBytes(unsigned char *value, size_t size) {
	if (size>respsize-respposition) {
		return false;
	}
	bytestring::copy(value,resppacket+respposition,size);
	respposition+=size;
	return true;
}

bool oracleprotocolclient::readLenPreInt(uint32_t *value) {
	unsigned char	count=0;
	if (!readByte(&count) || count>4 ||
			(size_t)count>respsize-respposition) {
		return false;
	}
	*value=0;
	for (unsigned char i=0; i<count; i++) {
		*value=((*value)<<8)|resppacket[respposition];
		respposition++;
	}
	return true;
}

bool oracleprotocolclient::readLenString(char **value) {
	unsigned char	length=0;
	if (!readByte(&length) || (size_t)length>respsize-respposition) {
		return false;
	}
	*value=charstring::duplicate((const char *)resppacket+respposition,
								length);
	respposition+=length;
	return true;
}


// ---- handshake ----

// the connect packet: 50 bytes of fixed fields, the 32-bit sdu and tdu a
// client at PROTOCOL_VERSION_12 repeats behind them, 8 bytes of filler,
// and then the connect descriptor.  field order and the 74-byte data
// offset are what a real OCI client sends, and what
// recvConnectRequest() reads
bool oracleprotocolclient::sendConnect(const char *sid) {

	char	*connectdata=NULL;
	charstring::printf(&connectdata,
		"(DESCRIPTION=(CONNECT_DATA=(SID=%s))"
		"(ADDRESS=(PROTOCOL=TCP)(Host=localhost)(Port=1521)))",sid);
	size_t	connectdatasize=charstring::getLength(connectdata);

	beginPacket(ORA_PACKET_CONNECT);
	appendBE16(ORA_PROTOCOL_VERSION_12+5);	// version offered
	appendBE16(0x012c);			// lowest version supported
	appendBE16(0x0c41);			// global service options
	appendBE16(8192);			// sdu
	appendBE16(0xffff);			// tdu
	appendBE16(0x7f08);			// protocol characteristics
	appendBE16(0);				// max packets before ack
	// the byte order marker, always 1, written in host byte order on
	// purpose - the listener compares the raw bytes to work out which
	// end this is.  this test only ever runs on the same machine as
	// the listener, but write it the way a little endian client does
	appendByte(1);
	appendByte(0);
	appendBE16((uint16_t)connectdatasize);
	appendBE16(74);				// connect data offset
	appendBE32(5120);			// max connect data receivable
	appendBE16(ORA_ANO_FLAGS_NO_SERVICES);
	appendBE32(0);				// trace cross facility item 1
	appendBE32(0);				// trace cross facility item 2
	appendBE32(0);				// trace unique connection id 1
	appendBE32(0);
	appendBE32(0);				// trace unique connection id 2
	appendBE32(0);
	appendBE32(8192);			// sdu, 32 bit
	appendBE32(2097152);			// tdu, 32 bit
	unsigned char	filler[8]={0,0,0,0,0,0,0,1};
	appendBytes(filler,sizeof(filler));
	reqpacket.append(connectdata,connectdatasize);

	delete[] connectdata;

	return sendPacket();
}

// version 6, which is the highest sendTtiResponse() implements
bool oracleprotocolclient::sendProtocolNegotiation() {

	beginPacket(ORA_PACKET_DATA);
	appendBE16(0);
	appendByte(ORA_TTC_PROTOCOL_NEGOTIATION);
	appendByte(6);
	appendByte(0);				// end of the version array
	reqpacket.append("x86_64/Linux 2.4.xx");
	appendByte(0);

	return sendPacket();
}

// the data type negotiation.  two things here are load-bearing:
//
//	- the compile capability array's CCAP_FIELD_VERSION byte, which
//	  recvDataTypeRequest() negotiates down to the lower of the two
//	  ends' values.  the listener is pinned at 11.2, so offering 11.2
//	  lands on 11.2 either way
//	- exactly one representation per data type.  OCI is the only client
//	  that offers two, and recvDataTypeRequest() sets ociclient from
//	  that.  offering one keeps ociclient false, which puts
//	  getQuery3Request() and putDescribeInfo() on their thin-driver
//	  branches - and the thin-driver branch is the one whose query
//	  length byte has to equal the declared query size, which is what
//	  query3() below writes
bool oracleprotocolclient::sendDataTypeNegotiation() {

	beginPacket(ORA_PACKET_DATA);
	appendBE16(0);
	appendByte(ORA_TTC_DATATYPE_NEGOTIATION);

	// the client's remote-in and remote-out character sets, little
	// endian.  the listener uses its own whatever these say
	appendByte((unsigned char)(ORA_CHARSET_AL32UTF8&0xff));
	appendByte((unsigned char)((ORA_CHARSET_AL32UTF8>>8)&0xff));
	appendByte((unsigned char)(ORA_CHARSET_AL32UTF8&0xff));
	appendByte((unsigned char)((ORA_CHARSET_AL32UTF8>>8)&0xff));

	// encoding flags: ENCODING_CONV_LENGTH, what a real OCI client and
	// 9i both send alone
	appendByte(0x02);

	// the compile capabilities.  everything but the field version is
	// zero: recvDataTypeRequest() reads no other index out of a
	// client's array, and a zero at CCAP_TTC3 leaves the time zone
	// version out of the request, which keeps this simple
	unsigned char	compilecaps[ORA_CCAP_SIZE];
	bytestring::zero(compilecaps,sizeof(compilecaps));
	compilecaps[ORA_CCAP_FIELD_VERSION]=ORA_CCAP_FIELD_VERSION_11_2;
	appendByte((unsigned char)sizeof(compilecaps));
	appendBytes(compilecaps,sizeof(compilecaps));

	// the runtime capabilities.  RCAP_DB_TIMEZONE is 0, so the db time
	// zone group that would otherwise follow the arrays is left out
	unsigned char	runtimecaps[ORA_RCAP_SIZE];
	bytestring::zero(runtimecaps,sizeof(runtimecaps));
	appendByte((unsigned char)sizeof(runtimecaps));
	appendBytes(runtimecaps,sizeof(runtimecaps));

	// the data type list: the type, the type it converts to, and one
	// representation of that type, ended by a zero type.  nothing in
	// the listener's answer depends on this list - it answers with its
	// own table - but the one-representation-per-type shape is what
	// keeps ociclient false
	static const uint16_t	datatypes[][3]={
		{1,1,1},	// VARCHAR
		{2,2,10},	// NUMBER
		{12,12,10},	// DATE
		{23,23,1},	// RAW
		{96,96,1}	// CHAR
	};
	for (size_t i=0; i<sizeof(datatypes)/sizeof(datatypes[0]); i++) {
		appendBE16(datatypes[i][0]);
		appendBE16(datatypes[i][1]);
		appendBE16(datatypes[i][2]);
		appendBE16(0);
	}
	appendBE16(0);

	return sendPacket();
}

bool oracleprotocolclient::connect(const char *host, uint16_t port,
							const char *sid) {

	sock.setHost(host);
	sock.setPort(port);
	if (!sock.connect()) {
		setError("failed to connect to the listener");
		return false;
	}
	connected=true;

	// connect() in src/protocols/oracle.cpp reads the connect packet,
	// asks for a resend the way a real database does, and reads it again
	if (!sendConnect(sid) || !recvPacket()) {
		return false;
	}
	if (resppackettype!=ORA_PACKET_RESEND) {
		setError("expected a resend packet");
		return false;
	}
	if (!sendConnect(sid) || !recvPacket()) {
		return false;
	}
	if (resppackettype==ORA_PACKET_REFUSE) {
		setError("the listener refused the connection");
		return false;
	}
	if (resppackettype!=ORA_PACKET_ACCEPT || respsize<2) {
		setError("expected an accept packet");
		return false;
	}

	// the accept itself keeps the 16-bit header whatever version it
	// announces - only the packets after it switch.  sendAccept() flips
	// its own end at exactly this point, so flip this one here too
	uint16_t	acceptversion=
			(uint16_t)(((uint16_t)resppacket[0]<<8)|resppacket[1]);
	largeheader=(acceptversion>=ORA_PROTOCOL_VERSION_12);

	if (!sendProtocolNegotiation() || !recvPacket()) {
		return false;
	}
	if (resppackettype!=ORA_PACKET_DATA ||
			getResponseTtcCode()!=ORA_TTC_PROTOCOL_NEGOTIATION) {
		setError("bad protocol negotiation response");
		return false;
	}

	if (!sendDataTypeNegotiation() || !recvPacket()) {
		return false;
	}
	if (resppackettype!=ORA_PACKET_DATA) {
		setError("bad data type negotiation response");
		return false;
	}

	return true;
}


// ---- authentication ----

// one AUTH_ pair: the name's size, the name, the value's size, the value
// (left out entirely when the size is 0) and the flags ub4.  see
// sqlrprotocol_oracle::getAuthField()
void oracleprotocolclient::appendAuthField(const char *name,
						const char *value,
						uint32_t flags) {
	size_t	namesize=charstring::getLength(name);
	size_t	valuesize=charstring::getLength(value);
	appendLenPreInt((uint32_t)namesize);
	appendLenString(name,namesize);
	appendLenPreInt((uint32_t)valuesize);
	if (valuesize) {
		appendLenString(value,valuesize);
	}
	appendLenPreInt(flags);
}

// the fixed part of a login request, up to and including the user name.
// the pointers are single bytes because this client speaks the portable
// encoding - recvAuthenticationRequest() tells the two apart by whether
// the first pointer is 0xfe, and 0x01 is not
void oracleprotocolclient::sendAuthRequest(unsigned char ttifunction,
						const char *user,
						uint32_t authmode,
						uint32_t fieldcount) {

	size_t	usersize=charstring::getLength(user);

	beginTtiCall(ttifunction);
	appendByte(2);				// sequence number
	appendByte(1);				// pointer
	appendLenPreInt((uint32_t)usersize);
	appendLenPreInt(authmode);
	appendByte(1);				// pointer
	appendLenPreInt(fieldcount);
	appendByte(1);				// pointer
	appendByte(1);				// pointer
	appendLenString(user,usersize);
}

// pull one AUTH_ pair's value out of the response the listener just sent
bool oracleprotocolclient::findAuthField(const char *name, char **value) {

	*value=NULL;

	rewindResponse();

	unsigned char	dataflagshigh=0;
	unsigned char	dataflagslow=0;
	unsigned char	ttccode=0;
	uint32_t	paircount=0;
	if (!readByte(&dataflagshigh) || !readByte(&dataflagslow) ||
			!readByte(&ttccode) || !readLenPreInt(&paircount)) {
		return false;
	}

	for (uint32_t i=0; i<paircount; i++) {

		uint32_t	namesize=0;
		char		*fieldname=NULL;
		uint32_t	valuesize=0;
		char		*fieldvalue=NULL;
		uint32_t	flags=0;

		if (!readLenPreInt(&namesize) || !readLenString(&fieldname)) {
			return false;
		}
		if (!readLenPreInt(&valuesize)) {
			delete[] fieldname;
			return false;
		}
		if (valuesize && !readLenString(&fieldvalue)) {
			delete[] fieldname;
			return false;
		}
		if (!readLenPreInt(&flags)) {
			delete[] fieldname;
			delete[] fieldvalue;
			return false;
		}

		if (!charstring::compare(fieldname,name)) {
			delete[] fieldname;
			*value=fieldvalue;
			return true;
		}

		delete[] fieldname;
		delete[] fieldvalue;
	}

	return false;
}

// the o5logon exchange, client side.  the math mirrors
// src/auths/oracle_userlist.cpp - see passwordHash(), o5logonChallenge()
// and o5logonComboKey() there:
//
//	password hash	= sha1(password || vfrdata) || 4 zero bytes
//	session key A	= aes192-cbc-decrypt(password hash,
//					the listener's AUTH_SESSKEY)
//	session key B	= 40 random bytes || 8 bytes of 0x08 - the pkcs#7
//			  padding a real client's key carries, and the
//			  listener checks for
//	AUTH_SESSKEY	= aes192-cbc-encrypt(password hash, session key B)
//	b		= A[16..39] xor B[16..39]
//	combo key	= md5(b[0..15]) || md5(b[16..23])[0..7]
//	AUTH_PASSWORD	= aes192-cbc-encrypt(combo key,
//					16 random bytes || password || pad)
bool oracleprotocolclient::login(const char *user, const char *password) {

	// phase one names the user and asks for a challenge.  the listener
	// reads no field out of it, so it carries none
	sendAuthRequest(ORA_TTI_LOGON_PRESENT_USER_REQ_AUTH_SESSKEY,
							user,1,0);
	if (!sendPacket() || !recvPacket()) {
		return false;
	}
	if (getResponseTtcCode()!=ORA_TTC_OK) {
		setError("the listener refused the login at phase one");
		return false;
	}

	char	*vfrdatahex=NULL;
	char	*sesskeyhex=NULL;
	if (!findAuthField("AUTH_VFR_DATA",&vfrdatahex) ||
		!findAuthField("AUTH_SESSKEY",&sesskeyhex)) {
		delete[] vfrdatahex;
		delete[] sesskeyhex;
		setError("no challenge in the phase one response");
		return false;
	}

	unsigned char	*vfrdata=NULL;
	uint64_t	vfrdatasize=0;
	charstring::hexDecode(vfrdatahex,charstring::getLength(vfrdatahex),
						&vfrdata,&vfrdatasize);

	unsigned char	*encparta=NULL;
	uint64_t	encpartasize=0;
	charstring::hexDecode(sesskeyhex,charstring::getLength(sesskeyhex),
						&encparta,&encpartasize);

	delete[] vfrdatahex;
	delete[] sesskeyhex;

	if (encpartasize!=ORA_SESSION_KEY_SIZE_11G) {
		delete[] vfrdata;
		delete[] encparta;
		setError("the challenge isn't an 11g session key");
		return false;
	}

	// the password hash, which both aes keys below are derived from
	unsigned char	passwordhash[ORA_PASSWORD_HASH_SIZE_11G];
	bytestring::zero(passwordhash,sizeof(passwordhash));
	sha1		s;
	bool		ok=(s.append((const unsigned char *)password,
				(uint32_t)charstring::getLength(password)) &&
			s.append(vfrdata,(uint32_t)vfrdatasize));
	if (ok) {
		const unsigned char	*digest=s.getHash();
		ok=(digest!=NULL);
		if (ok) {
			bytestring::copy(passwordhash,digest,20);
		}
	}
	delete[] vfrdata;
	if (!ok) {
		delete[] encparta;
		setError("failed to hash the password");
		return false;
	}

	// session key part A, and this client's own part B
	unsigned char	parta[ORA_SESSION_KEY_SIZE_11G];
	unsigned char	partb[ORA_SESSION_KEY_SIZE_11G];
	unsigned char	encpartb[ORA_SESSION_KEY_SIZE_11G];
	size_t		materialsize=ORA_SESSION_KEY_SIZE_11G-
					ORA_SESSION_KEY_PAD_SIZE_11G;
	bytestring::set(partb+materialsize,
			(unsigned char)ORA_SESSION_KEY_PAD_SIZE_11G,
			ORA_SESSION_KEY_PAD_SIZE_11G);
	csprng	csr;
	ok=(oracleAes192Cbc(false,passwordhash,
				encparta,ORA_SESSION_KEY_SIZE_11G,parta) &&
		csr.generateBytes(partb,sizeof(partb),materialsize) &&
		oracleAes192Cbc(true,passwordhash,
				partb,ORA_SESSION_KEY_SIZE_11G,encpartb));
	delete[] encparta;
	if (!ok) {
		setError("failed to build a session key");
		return false;
	}

	// the combo key the password travels under
	unsigned char	b[ORA_COMBO_KEY_SIZE_11G];
	for (size_t i=0; i<sizeof(b); i++) {
		b[i]=parta[16+i]^partb[16+i];
	}
	unsigned char	combokey[ORA_COMBO_KEY_SIZE_11G];
	md5		m1;
	md5		m2;
	ok=(m1.append(b,16) && m2.append(b+16,8));
	if (ok) {
		const unsigned char	*part1=m1.getHash();
		const unsigned char	*part2=m2.getHash();
		ok=(part1!=NULL && part2!=NULL);
		if (ok) {
			bytestring::copy(combokey,part1,16);
			bytestring::copy(combokey+16,part2,8);
		}
	}
	if (!ok) {
		setError("failed to build a combo key");
		return false;
	}

	// AUTH_PASSWORD: a 16-byte salt, the password, and pkcs#7 padding
	size_t		passwordsize=charstring::getLength(password);
	size_t		padsize=16-((16+passwordsize)%16);
	size_t		plainsize=16+passwordsize+padsize;
	unsigned char	*plainpassword=new unsigned char[plainsize];
	unsigned char	*encpassword=new unsigned char[plainsize];
	bytestring::copy(plainpassword+16,password,passwordsize);
	bytestring::set(plainpassword+16+passwordsize,
				(unsigned char)padsize,padsize);
	ok=(csr.generateBytes(plainpassword,plainsize,16) &&
		oracleAes192Cbc(true,combokey,
				plainpassword,plainsize,encpassword));
	delete[] plainpassword;
	if (!ok) {
		delete[] encpassword;
		setError("failed to encrypt the password");
		return false;
	}

	char	*sesskey=oracleHexEncodeUpper(encpartb,sizeof(encpartb));
	char	*authpassword=oracleHexEncodeUpper(encpassword,plainsize);
	delete[] encpassword;

	// phase two.  the listener reads AUTH_SESSKEY and AUTH_PASSWORD out
	// of this and ignores everything else, so those two are all it
	// carries - a real client sends around twenty
	sendAuthRequest(ORA_TTI_LOGON_PRESENT_PWD_SEND_AUTH_PASSWORD,
						user,0x40000101,2);
	appendAuthField("AUTH_SESSKEY",sesskey,1);
	appendAuthField("AUTH_PASSWORD",authpassword,0);
	delete[] sesskey;
	delete[] authpassword;

	if (!sendPacket() || !recvPacket()) {
		return false;
	}
	if (getResponseTtcCode()!=ORA_TTC_OK) {
		setError("the listener refused the login");
		return false;
	}

	return true;
}

void oracleprotocolclient::disconnect() {
	if (!connected) {
		return;
	}
	beginTtiCall(ORA_TTI_DISCONNECT);
	sendPacket();
	sock.close();
	connected=false;
}


// ---- calls ----

// TTI_OPEN: a sequence number, a pointer flag for the cursor id the
// listener allocates, and an open size.  see open() in
// src/protocols/oracle.cpp
bool oracleprotocolclient::open(uint32_t *cursorid) {

	beginTtiCall(ORA_TTI_OPEN);
	appendByte(1);			// sequence number
	appendByte(1);			// cursor id pointer
	appendLenPreInt(0);		// open size

	if (!sendPacket() || !recvPacket()) {
		return false;
	}
	if (getResponseTtcCode()!=ORA_TTC_OK) {
		setError("the listener refused to open a cursor");
		return false;
	}

	rewindResponse();
	unsigned char	skip=0;
	if (!readByte(&skip) || !readByte(&skip) || !readByte(&skip) ||
					!readLenPreInt(cursorid)) {
		setError("bad open response");
		return false;
	}
	return true;
}

// TTI_QUERY3 - the modern combined open/parse/describe/execute call.  the
// field order below is exactly what getQuery3Request() reads, and it was
// checked against a real OCI request captured in
// test/testdetails-oracleprotocol.log: every field is either a ub4 in the
// count-then-bytes form or a single raw byte, so nothing sits at a fixed
// offset and the sequence has to be written out in full
bool oracleprotocolclient::query3(uint32_t options, uint32_t cursorid,
					uint32_t prefetchrows,
					const char *query) {

	size_t	querysize=charstring::getLength(query);

	// the al8i4 vector, which getQuery3Binds() only reads when there are
	// binds or defines - there are none here, so it's written to match
	// what a real client sends rather than because anything reads it
	static const unsigned char	al8i4[]={
		0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x01, 0x06, 0x00, 0x02, 0x80, 0x00, 0x00,
		0x00, 0x00
	};
	uint32_t	vectorsize=(querysize)?13:0;

	beginTtiCall(ORA_TTI_QUERY3);
	appendByte(1);				// sequence number
	appendLenPreInt(options);
	appendLenPreInt(cursorid);
	appendByte(1);				// pointer
	appendLenPreInt((uint32_t)querysize);
	appendByte(1);				// pointer
	appendLenPreInt(vectorsize);
	appendByte(1);				// pointer
	appendByte(1);				// pointer
	appendLenPreInt(0);			// prefetch buffer size
	appendLenPreInt(prefetchrows);
	appendLenPreInt(0);			// max long size
	appendByte(0);				// pointer
	appendLenPreInt(0);			// bind count
	appendByte(0);				// pointer
	appendByte(1);				// pointer
	appendByte(0);				// pointer
	appendByte(1);				// pointer
	appendByte(1);				// pointer
	appendByte(1);				// pointer
	appendLenPreInt(0);			// define count
	appendLenPreInt(0);
	appendByte(1);				// pointer
	appendByte(1);				// pointer
	appendByte(0);				// pointer
	appendLenPreInt(0);
	appendByte(0);				// pointer
	appendLenPreInt(0);
	appendLenPreInt(0);

	// the query text, behind a length byte that has to equal the
	// declared size - which is what getQuery3Request() believes for
	// every client but OCI, and this client isn't one
	if (querysize) {
		appendLenString(query,querysize);
		appendBytes(al8i4,sizeof(al8i4));
	}

	return sendPacket() && recvPacket();
}

// TTI_FETCH.  the session took the modern path with its first query3(), so
// fetch() in src/protocols/oracle.cpp hands this to fetch3()
bool oracleprotocolclient::fetch(uint32_t cursorid, uint32_t rowstofetch) {

	beginTtiCall(ORA_TTI_FETCH);
	appendByte(1);				// sequence number
	appendLenPreInt(cursorid);
	appendLenPreInt(rowstofetch);

	return sendPacket() && recvPacket();
}


// ---- the pre-query3 calls ----

// TTI_QUERY: the legacy parse.  query() in src/protocols/oracle.cpp reads a
// fixed 13-byte body and then querysize raw bytes of query text - no length
// byte of its own in front of it.  everything in the body is big-endian
// except the query size, which readLE() reads little-endian.  the five
// unexplained bytes around it are read and logged but never acted on, so
// they go out as zeros
bool oracleprotocolclient::legacyQuery(uint16_t options,
					uint16_t moreoptions,
					uint16_t cursorid,
					const char *query) {

	size_t	querysize=charstring::getLength(query);

	beginTtiCall(ORA_TTI_QUERY);
	appendBE16(options);
	appendBE16(moreoptions);
	appendBE16(cursorid);
	appendByte(0);			// unknown3
	appendByte(0);			// unknown4
	appendByte(0);			// unknown5
	appendLE16((uint16_t)querysize);
	appendByte(0);			// unknown6
	appendByte(0);			// unknown7
	if (querysize) {
		appendBytes((const unsigned char *)query,querysize);
	}

	return sendPacket() && recvPacket();
}

// TTI_EXECUTE: options, more options and a cursor id, and nothing else.
// execute() in src/protocols/oracle.cpp only takes this shape while
// query3session is false - once it's true the same function code means the
// modern re-execute instead
bool oracleprotocolclient::legacyExecute(uint16_t options,
					uint16_t moreoptions,
					uint16_t cursorid) {

	beginTtiCall(ORA_TTI_EXECUTE);
	appendBE16(options);
	appendBE16(moreoptions);
	appendBE16(cursorid);

	return sendPacket() && recvPacket();
}

// the legacy TTI_FETCH: options and more options, no cursor id.  the
// options pick the response shape - OPTION_DEFINE prepends column
// definitions, OPTION_SNDIOV swaps the two filler bytes for an iov, and
// OPTION_EXACTFETCH swaps the trailer.  those three bits make eight
// response shapes, and 0 asks for the plainest of them, which is the only
// one worth decoding by hand
bool oracleprotocolclient::legacyFetch(uint16_t options,
					uint16_t moreoptions) {

	beginTtiCall(ORA_TTI_FETCH);
	appendBE16(options);
	appendBE16(moreoptions);

	return sendPacket() && recvPacket();
}
