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

// the tti protocol versions the module implements - TTI_VERSION_MIN and
// TTI_VERSION_MAX in src/protocols/oracle.cpp.  sendProtocolNegotiation()
// offers version 6, the highest sendTtiResponse() implements, unless
// setTtiVersion() says otherwise
static const unsigned char	ORA_TTI_VERSION_5=5;
static const unsigned char	ORA_TTI_VERSION_6=6;

// query3 options - the OPTION_* defines in src/protocols/oracle.cpp
static const uint32_t	ORA_OPTION_PARSE=(1<<0);
static const uint32_t	ORA_OPTION_DEFINE=(1<<4);
static const uint32_t	ORA_OPTION_EXECUTE=(1<<5);
static const uint32_t	ORA_OPTION_FETCH=(1<<6);
static const uint32_t	ORA_OPTION_COMMIT=(1<<8);
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

// CCAP_FIELD_VERSION_12_2 in src/protocols/oracle.cpp.  a bind descriptor
// grows a thirteenth field at and above it - see getQuery3BindDescriptor()
// there - so a client pinned below it, which this one is, leaves that
// field out
static const unsigned char	ORA_CCAP_FIELD_VERSION_12_2=8;

// CCAP_TTC3 and CCAP_TTC3_BIG_CHUNK_CLR in src/protocols/oracle.cpp.  a
// client that offers the bit, against a module that also offers it, gets a
// long clr's chunk lengths as count prefixed ub4s rather than raw bytes -
// see recvDataTypeRequest() there, which is where bigchunkclr is decided
static const size_t		ORA_CCAP_TTC3=37;
static const unsigned char	ORA_CCAP_TTC3_BIG_CHUNK_CLR=0x20;

// the clr's two forms - the CLR_* defines in src/protocols/oracle.cpp
static const size_t		ORA_CLR_MAX_SHORT_LENGTH=252;
static const unsigned char	ORA_CLR_NULL_MARKER=0xfd;
static const unsigned char	ORA_CLR_LONG_FORM_MARKER=0xfe;
static const size_t		ORA_CLR_MAX_CHUNK_SIZE=255;
static const size_t		ORA_CLR_MAX_BIG_CHUNK_SIZE=32767;

// bind and define descriptor flags - the BIND_FLAG_* defines in
// src/protocols/oracle.cpp.  USE_INDICATORS is what a descriptor with a
// real value behind it carries; UNBOUND says the placeholder was never
// bound, and getQuery3Binds() there drops the row data for the whole
// request as soon as any one descriptor sets it
static const unsigned char	ORA_BIND_FLAG_USE_INDICATORS=0x01;
static const unsigned char	ORA_BIND_FLAG_UNBOUND=0x80;

// ORACLE_TYPE_VARCHAR in src/protocols/oracle.cpp - the only type a
// character bind needs
static const unsigned char	ORA_TYPE_VARCHAR=1;

// SQLCS_IMPLICIT, a character bind's character set form.  nothing in the
// listener reads it - getQuery3BindDescriptor() consumes the byte and
// throws it away - it is here so a capture of this client's request reads
// the way a real client's does
static const unsigned char	ORA_CSFRM_IMPLICIT=1;

// how many elements go in the al8i4 vector - see appendAl8i4Vector()
static const uint32_t		ORA_AL8I4_SIZE=13;

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

// one bind or define descriptor.  these are the twelve fields
// getQuery3BindDescriptor() in src/protocols/oracle.cpp reads, in its
// order, plus the thirteenth that only exists from field version 12.2 up.
// none of them says which placeholder the bind is for, or how many values
// follow, or which way the value travels - a bind is tied to its
// placeholder and to its value by position alone.
//
// varchar() fills the whole thing in for the common case, so a test that
// binds a string only has to say how big its buffer is; a test that needs
// some other shape starts from clear() and sets the fields itself
struct oracleprotocolbind {

	unsigned char		type;		// ORACLE_TYPE_*
	unsigned char		flags;		// ORA_BIND_FLAG_*
	unsigned char		precision;
	unsigned char		scale;
	uint32_t		buffersize;
	uint32_t		maxelements;
	uint32_t		contflags;
	uint32_t		oidlength;
	const unsigned char	*oid;		// oidlength bytes, or NULL
	uint32_t		version;
	uint32_t		charsetid;
	unsigned char		csfrm;
	uint32_t		maxdatasize;
	uint32_t		oaccolid;	// field version 12.2 and up

	void	clear();
	void	varchar(uint32_t buffersize);
};

// one bind's value for one execution iteration.  a size of 0 is how a null
// goes out: appendLenBytes() writes a lone zero length byte for it, which
// is what getLenBytes() in src/protocols/oracle.cpp reads back as a null -
// and what oracle itself makes of an empty string anyway
struct oracleprotocolbindvalue {

	const char	*value;
	size_t		size;

	void	set(const char *value);
	void	set(const char *value, size_t size);
	void	setNull();
};

class oracleprotocolclient {
	public:
			oracleprotocolclient();
			~oracleprotocolclient();

		// whether this client offers CCAP_TTC3_BIG_CHUNK_CLR in
		// its compile capabilities.  off by default, so a test
		// that says nothing keeps the raw-byte chunk framing every
		// test here used before the bit existed.  it has to be set
		// before connect() - the bit goes out in the data type
		// negotiation, and both ends decide there and then
		void	setBigChunkClr(bool bigchunkclr);

		// which tti protocol version this client offers in its
		// protocol negotiation.  version 6 by default, the highest
		// the module implements.  version 5 is the other one it
		// implements, and it is the version, not the bit, that
		// decides the chunk framing there: recvDataTypeRequest()
		// only ever honors CCAP_TTC3_BIG_CHUNK_CLR at version 6 and
		// above, so a version 5 client gets the raw byte framing
		// whatever it offered.  like the bit, this has to be set
		// before connect()
		void	setTtiVersion(unsigned char ttiversion);

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
		// cursor sends.  this form sends no binds and no defines
		bool	query3(uint32_t options, uint32_t cursorid,
					uint32_t prefetchrows,
					const char *query);

		// TTI_QUERY3 with binds.  "binds" carries one descriptor
		// per placeholder and "defines" one per define - the
		// listener reads a define's descriptor with the same code
		// and throws the result away, so 0 defines is the normal
		// case here.
		//
		// "values" is one flat array of blockcount*bindcount
		// values, block by block: the first bindcount entries are
		// the first execution iteration's, the next bindcount the
		// second's, and so on.
		//
		// "iterations" is what goes in the al8i4 vector, and it is
		// deliberately independent of blockcount - a real thin
		// client can claim 0 iterations and still send row data,
		// and telling the listener one thing while doing another
		// is the whole point of some of the tests here
		bool	query3(uint32_t options, uint32_t cursorid,
					uint32_t prefetchrows,
					const char *query,
					const oracleprotocolbind *binds,
					uint32_t bindcount,
					uint32_t iterations,
					const oracleprotocolbindvalue *values,
					uint32_t blockcount,
					const oracleprotocolbind *defines=NULL,
					uint32_t definecount=0);

		// TTI_EXECUTE, the modern shape: the second and later
		// executes of a statement one query3() with query text
		// already parsed.  only fresh values go out - the listener
		// remembers the descriptors - so this takes the bind count
		// and the values but no descriptors of its own.
		// "iterations" is independent of blockcount here too.
		// legacyExecute() writes the pre-query3 shape instead, and
		// which of the two the listener reads is decided by
		// query3session there, not by the call
		bool	reexecute(uint32_t cursorid, uint32_t iterations,
					uint32_t options, uint32_t moreoptions,
					uint32_t bindcount,
					const oracleprotocolbindvalue *values,
					uint32_t blockcount);

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
		void	appendLenBytes(const char *value, size_t size);
		void	appendAl8i4Vector(uint32_t iterations);
		void	appendBindDescriptor(const oracleprotocolbind *bind);
		void	appendRowDataBlocks(
					const oracleprotocolbindvalue *values,
					uint32_t bindcount,
					uint32_t blockcount);
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
		bool	readLenBytes(unsigned char *value, size_t maxsize,
						size_t *size, bool *isnull);

	private:
		void	setError(const char *message);
		void	setError(const char *message, const char *detail);

		bool	bigChunkClrFraming();

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
		bool			bigchunkclr;
		unsigned char		ttiversion;

		// what this client offers, and so - since the listener
		// negotiates down to the lower of the two ends - what it
		// gets.  a bind descriptor's shape depends on it
		unsigned char		fieldversion;

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
	bigchunkclr=false;
	ttiversion=ORA_TTI_VERSION_6;
	fieldversion=ORA_CCAP_FIELD_VERSION_11_2;
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

void oracleprotocolclient::setBigChunkClr(bool bigchunkclr) {
	this->bigchunkclr=bigchunkclr;
}

void oracleprotocolclient::setTtiVersion(unsigned char ttiversion) {
	this->ttiversion=ttiversion;
}

// which framing this client has to read and write - which is not the same
// question as which bit it offers.  recvDataTypeRequest() in
// src/protocols/oracle.cpp gates the big chunk framing on the tti version as
// well as on both ends' bits:
//
//	bigchunkclr=(ttiversion>=6 && ...)
//
// so a client that offers the bit at version 5 is answered in raw bytes
// anyway, and has to frame its own requests in raw bytes to be understood.
// this mirrors that gate, and it is only the plumbing - what the test
// asserts is the bytes the module actually wrote, which it builds itself
bool oracleprotocolclient::bigChunkClrFraming() {
	return (bigchunkclr && ttiversion>=ORA_TTI_VERSION_6);
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

// a text - one length byte, then that many bytes.  there is no long form; a
// value past 252 bytes takes a clr instead.  the mirror of putLenString() in
// src/protocols/oracle.cpp
void oracleprotocolclient::appendLenString(const char *value, size_t size) {
	appendByte((unsigned char)size);
	reqpacket.append(value,size);
}

// a clr - the text-shaped short form up to 252 bytes, the chunked long form
// above that.  the mirror of putLenBytes() in src/protocols/oracle.cpp, and
// the exact shape getLenBytes() and getQuery3Request() read back there:
// a 0xfe marker, then a run of chunks, then an empty chunk to close it.
//
// a chunk's length goes out as a raw byte, capped at 255, unless this client
// negotiated CCAP_TTC3_BIG_CHUNK_CLR - see bigChunkClrFraming() - then it
// goes out as a count prefixed ub4, capped at 32767.  the closing chunk is
// one zero byte either way, since a ub4 zero is a count byte of 0 and
// nothing after it
void oracleprotocolclient::appendLenBytes(const char *value, size_t size) {

	if (size<=ORA_CLR_MAX_SHORT_LENGTH) {
		appendByte((unsigned char)size);
		if (size) {
			reqpacket.append(value,size);
		}
		return;
	}

	appendByte(ORA_CLR_LONG_FORM_MARKER);
	bool	bigchunk=bigChunkClrFraming();
	size_t	maxchunk=(bigchunk)?
			ORA_CLR_MAX_BIG_CHUNK_SIZE:ORA_CLR_MAX_CHUNK_SIZE;
	size_t	offset=0;
	while (offset<size) {
		size_t	chunk=size-offset;
		if (chunk>maxchunk) {
			chunk=maxchunk;
		}
		if (bigchunk) {
			appendLenPreInt((uint32_t)chunk);
		} else {
			appendByte((unsigned char)chunk);
		}
		reqpacket.append(value+offset,chunk);
		offset+=chunk;
	}
	appendByte(0);
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

// a clr, the other way round - the mirror of getLenBytes() in
// src/protocols/oracle.cpp, and the inverse of appendLenBytes() above.  a
// zero length and the 0xfd marker are both nulls, a length up to 252 is the
// short form, and 0xfe introduces the chunked long form, whose chunks are
// concatenated into "value".  a chunk's length is a raw byte, or a count
// prefixed ub4 if this client negotiated CCAP_TTC3_BIG_CHUNK_CLR - see
// bigChunkClrFraming().
//
// a long form written by putLongBytes() rather than putLenBytes() - which
// is what a LONG or a LONG RAW column goes out through - has two more zero
// bytes behind its closing empty chunk.  this stops at the empty chunk
// either way and leaves whatever follows it alone
bool oracleprotocolclient::readLenBytes(unsigned char *value,
						size_t maxsize,
						size_t *size,
						bool *isnull) {

	*size=0;
	*isnull=false;

	unsigned char	length=0;
	if (!readByte(&length)) {
		return false;
	}

	if (!length) {
		*isnull=true;
		return true;
	}
	if (length==ORA_CLR_NULL_MARKER) {
		unsigned char	nullcount=0;
		if (!readByte(&nullcount)) {
			return false;
		}
		*isnull=true;
		return true;
	}

	if (length<=ORA_CLR_MAX_SHORT_LENGTH) {
		if ((size_t)length>maxsize ||
				!readBytes(value,(size_t)length)) {
			return false;
		}
		*size=length;
		return true;
	}

	if (length!=ORA_CLR_LONG_FORM_MARKER) {
		return false;
	}

	for (;;) {

		uint32_t	chunksize=0;
		if (bigChunkClrFraming()) {
			if (!readLenPreInt(&chunksize)) {
				return false;
			}
		} else {
			unsigned char	rawchunksize=0;
			if (!readByte(&rawchunksize)) {
				return false;
			}
			chunksize=rawchunksize;
		}
		if (!chunksize) {
			return true;
		}

		if ((size_t)chunksize>maxsize-(*size) ||
				!readBytes(value+(*size),(size_t)chunksize)) {
			return false;
		}
		*size+=chunksize;
	}
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

// one version, whichever setTtiVersion() named - 6 by default, which is the
// highest sendTtiResponse() implements
bool oracleprotocolclient::sendProtocolNegotiation() {

	beginPacket(ORA_PACKET_DATA);
	appendBE16(0);
	appendByte(ORA_TTC_PROTOCOL_NEGOTIATION);
	appendByte(ttiversion);
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

	// the compile capabilities.  everything but the field version and
	// CCAP_TTC3 is zero: recvDataTypeRequest() reads no other index out
	// of a client's array, and a zero at CCAP_TTC3 leaves the time zone
	// version out of the request, which keeps this simple.
	//
	// CCAP_TTC3_BIG_CHUNK_CLR is the one bit a test can turn on there.
	// clear - the default, and what every test here sent before the bit
	// existed - the listener frames a long clr's chunks as a raw length
	// byte apiece; set, it frames them as count prefixed ub4s.  every
	// real client sets it, so the raw-byte framing is only reachable
	// from a client like this one that deliberately leaves it clear.
	// CCAP_TTC3_TZ_VERSION is not in the bit, so the time zone version
	// stays out of the request either way
	unsigned char	compilecaps[ORA_CCAP_SIZE];
	bytestring::zero(compilecaps,sizeof(compilecaps));
	compilecaps[ORA_CCAP_FIELD_VERSION]=fieldversion;
	if (bigchunkclr) {
		compilecaps[ORA_CCAP_TTC3]|=ORA_CCAP_TTC3_BIG_CHUNK_CLR;
	}
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


// ---- binds ----

// every field zero - a descriptor that claims nothing at all, and the
// starting point for anything that isn't a plain character bind
void oracleprotocolbind::clear() {
	type=0;
	flags=0;
	precision=0;
	scale=0;
	buffersize=0;
	maxelements=0;
	contflags=0;
	oidlength=0;
	oid=NULL;
	version=0;
	charsetid=0;
	csfrm=0;
	maxdatasize=0;
	oaccolid=0;
}

// a character bind of at most "buffersize" bytes.  only two of these
// fields reach anything: getQuery3Binds() in src/protocols/oracle.cpp
// keeps the type, the flags and the buffer size and reads past the rest,
// and of those it acts on the flags - BIND_FLAG_USE_INDICATORS is what
// says a real value follows, where BIND_FLAG_UNBOUND would say the
// placeholder was never bound and cost the whole request its row data.
// the character set fields carry what this client negotiated, so the
// request reads the way a real one does
void oracleprotocolbind::varchar(uint32_t buffersize) {
	clear();
	type=ORA_TYPE_VARCHAR;
	flags=ORA_BIND_FLAG_USE_INDICATORS;
	this->buffersize=buffersize;
	charsetid=ORA_CHARSET_AL32UTF8;
	csfrm=ORA_CSFRM_IMPLICIT;
}

// a value, sized by its own null terminator
void oracleprotocolbindvalue::set(const char *value) {
	this->value=value;
	size=charstring::getLength(value);
}

// a value of a size the caller states, which is what a value with a zero
// byte in it - or one built to be longer than a clr's short form - needs
void oracleprotocolbindvalue::set(const char *value, size_t size) {
	this->value=value;
	this->size=size;
}

void oracleprotocolbindvalue::setNull() {
	value=NULL;
	size=0;
}

// the al8i4 vector: thirteen count prefixed ub4s, back to back, right
// after the query text.  getQuery3Binds() in src/protocols/oracle.cpp
// reads as many elements as the request's vector size field declared and
// keeps exactly one of them - the second, which is the iteration count.
// every other element is read and dropped, so the twelve constants below
// are only what a real client's vector carries, not something the
// listener acts on.  they are also the bytes this client sent as a canned
// array before there was anything to vary, so a no-bind request's vector
// is byte for byte what it always was
void oracleprotocolclient::appendAl8i4Vector(uint32_t iterations) {
	appendLenPreInt(1);		// element 0
	appendLenPreInt(iterations);	// element 1 - the iteration count
	appendLenPreInt(0);		// element 2
	appendLenPreInt(0);		// element 3
	appendLenPreInt(0);		// element 4
	appendLenPreInt(0);		// element 5
	appendLenPreInt(0);		// element 6
	appendLenPreInt(6);		// element 7
	appendLenPreInt(0);		// element 8
	appendLenPreInt(32768);		// element 9
	appendLenPreInt(0);		// element 10
	appendLenPreInt(0);		// element 11
	appendLenPreInt(0);		// element 12
}

// one bind or define descriptor: four raw bytes, then a run of count
// prefixed ub4s with a lone raw byte sitting in the middle of it.  the
// order and the widths are getQuery3BindDescriptor()'s read order in
// src/protocols/oracle.cpp, and two of them are worth naming: csfrm,
// which is a plain byte between two ub4s rather than a ub4 of its own,
// and oaccolid, which only exists once the negotiated field version
// reaches 12.2 - this client negotiates 11.2, so it stays out.
//
// the oid bytes only go out behind a nonzero oid length, since that
// length is what the listener reads them by.  a define takes the
// identical shape: the listener reads it with the same function and
// throws the results away
void oracleprotocolclient::appendBindDescriptor(
				const oracleprotocolbind *bind) {

	appendByte(bind->type);				// type
	appendByte(bind->flags);			// flags
	appendByte(bind->precision);			// precision
	appendByte(bind->scale);			// scale
	appendLenPreInt(bind->buffersize);		// buffer size
	appendLenPreInt(bind->maxelements);		// max elements
	appendLenPreInt(bind->contflags);		// cont flags
	appendLenPreInt(bind->oidlength);		// oid length
	if (bind->oidlength && bind->oid) {
		appendBytes(bind->oid,bind->oidlength);	// oid
	}
	appendLenPreInt(bind->version);			// version
	appendLenPreInt(bind->charsetid);		// charset id
	appendByte(bind->csfrm);			// csfrm
	appendLenPreInt(bind->maxdatasize);		// max data size
	if (fieldversion>=ORA_CCAP_FIELD_VERSION_12_2) {
		appendLenPreInt(bind->oaccolid);	// oaccolid
	}
}

// the bind values: one TTC_ROW_DATA block per execution iteration, each a
// marker byte and then one clr per bind, in descriptor order.  there is no
// length in front of a block, nothing between two of them and nothing
// after the last - getQuery3BindValues() in src/protocols/oracle.cpp
// reads blocks until the packet runs out or it has as many as it was
// expecting.  a value carries no type of its own either; the type comes
// from the descriptor it lines up with.
//
// "values" is one flat array of blockcount*bindcount values, block by
// block, which is the order they go out in
void oracleprotocolclient::appendRowDataBlocks(
				const oracleprotocolbindvalue *values,
				uint32_t bindcount,
				uint32_t blockcount) {

	for (uint32_t block=0; block<blockcount; block++) {
		appendByte(ORA_TTC_ROW_DATA);		// row data marker
		for (uint32_t i=0; i<bindcount; i++) {
			const oracleprotocolbindvalue	*v=
					&(values[block*bindcount+i]);
			appendLenBytes(v->value,v->size);	// value
		}
	}
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

// TTI_QUERY3 without binds, which is what a call that only parses,
// describes, executes or fetches sends
bool oracleprotocolclient::query3(uint32_t options, uint32_t cursorid,
					uint32_t prefetchrows,
					const char *query) {
	// 1 iteration, the count this call has always claimed - with no
	// binds and no defines the listener never reads it anyway
	return query3(options,cursorid,prefetchrows,query,NULL,0,1,NULL,0);
}

// TTI_QUERY3 - the modern combined open/parse/describe/execute call.  the
// field order below is exactly what getQuery3Request() reads, and it was
// checked against a real OCI request captured in
// test/testdetails-oracleprotocol.log: every field is either a ub4 in the
// count-then-bytes form or a single raw byte, so nothing sits at a fixed
// offset and the sequence has to be written out in full.
//
// four things follow the header, in this order, which is the order
// getQuery3Request() and then getQuery3Binds() read them in: the query
// text, the al8i4 vector, every bind descriptor and then every define
// descriptor, and last one row data block per execution iteration
bool oracleprotocolclient::query3(uint32_t options, uint32_t cursorid,
					uint32_t prefetchrows,
					const char *query,
					const oracleprotocolbind *binds,
					uint32_t bindcount,
					uint32_t iterations,
					const oracleprotocolbindvalue *values,
					uint32_t blockcount,
					const oracleprotocolbind *defines,
					uint32_t definecount) {

	if ((bindcount && !binds) || (definecount && !defines)) {
		setError("query3 needs a descriptor per bind and define");
		return false;
	}
	if (blockcount && (!bindcount || !values)) {
		setError("query3 needs binds and values for a row data block");
		return false;
	}

	size_t	querysize=charstring::getLength(query);

	// getQuery3Binds() skips the al8i4 vector whole unless there is at
	// least one bind or define, so with neither it is only there because
	// a real client's request has one - which is why it also goes out
	// with a query and no binds, exactly as it always did
	bool		sendvector=(querysize || bindcount || definecount);
	uint32_t	vectorsize=(sendvector)?ORA_AL8I4_SIZE:0;

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
	appendLenPreInt(bindcount);		// bind count
	appendByte(0);				// pointer
	appendByte(1);				// pointer
	appendByte(0);				// pointer
	appendByte(1);				// pointer
	appendByte(1);				// pointer
	appendByte(1);				// pointer
	appendLenPreInt(definecount);		// define count
	appendLenPreInt(0);
	appendByte(1);				// pointer
	appendByte(1);				// pointer
	appendByte(0);				// pointer
	appendLenPreInt(0);
	appendByte(0);				// pointer
	appendLenPreInt(0);
	appendLenPreInt(0);

	// the query text, as a clr: up to 252 bytes, a length byte that has
	// to equal the declared size - which is what getQuery3Request()
	// believes for every client but OCI, and this client isn't one -
	// and past that the chunked long form, which getQuery3Request()
	// reads on its own branch
	if (querysize) {
		appendLenBytes(query,querysize);
	}

	if (sendvector) {
		appendAl8i4Vector(iterations);
	}

	// every bind descriptor, then every define descriptor - they are
	// one run of identically shaped descriptors as far as the wire is
	// concerned, and only the two counts in the header tell them apart
	for (uint32_t i=0; i<bindcount; i++) {
		appendBindDescriptor(&(binds[i]));
	}
	for (uint32_t i=0; i<definecount; i++) {
		appendBindDescriptor(&(defines[i]));
	}

	// and the values.  the listener reads no row data at all for a
	// request whose descriptors are all unbound, but that is its call to
	// make from the flags - whatever the caller asked for goes out
	appendRowDataBlocks(values,bindcount,blockcount);

	return sendPacket() && recvPacket();
}

// TTI_EXECUTE, the modern shape.  once one query3() carrying query text
// has gone out, query3session is true in src/protocols/oracle.cpp and the
// same function code reaches reexecute() there rather than the legacy
// execute() that legacyExecute() writes - so this only works on a
// connection that has already parsed a statement, and legacyExecute() only
// works on one that has not.
//
// five fields, a raw sequence byte and four ub4s, and then fresh values
// for the binds the statement was parsed with.  no descriptors go out:
// restoreQuery3Binds() there remembers how many binds the original query3()
// declared and what types they were, so the caller only has to know the
// count and supply the values
bool oracleprotocolclient::reexecute(uint32_t cursorid,
					uint32_t iterations,
					uint32_t options,
					uint32_t moreoptions,
					uint32_t bindcount,
					const oracleprotocolbindvalue *values,
					uint32_t blockcount) {

	if (blockcount && (!bindcount || !values)) {
		setError("reexecute needs binds and values "
					"for a row data block");
		return false;
	}

	beginTtiCall(ORA_TTI_EXECUTE);
	appendByte(1);				// sequence number
	appendLenPreInt(cursorid);
	appendLenPreInt(iterations);
	appendLenPreInt(options);
	appendLenPreInt(moreoptions);

	appendRowDataBlocks(values,bindcount,blockcount);

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
