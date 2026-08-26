// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/inetsocketclient.h>
#include <rudiments/bytebuffer.h>
#include <rudiments/bytestring.h>
#include <rudiments/charstring.h>
#include <rudiments/stdio.h>

// Coverage for ticket #9476: the pre-tds7 (tds 5.0) path in
// src/protocols/tds.cpp used to write every multi-byte value
// little-endian, no matter what byte order the client declared in its
// login record.  preTds7ByteOrder() now decodes the login record's
// typeflags block and calls setProtocolIsBigEndian(), and every pre-tds7
// write goes through the byte-order-aware write() overloads from there
// on.
//
// No real client can drive the big-endian half of that.  SAP's ct-lib and
// FreeTDS both fill the typeflags block in from the machine they're built
// for, and every machine that runs either of them here is x86 - so
// tdssapprotocol, tdsfreetdssapprotocol and tdsfreetdsmssqlprotocol all
// declare little-endian and a bug in the big-endian path would be
// invisible to all three.  So this drives the wire protocol directly, the
// way test/protocol/tds/tdsdialectguard.cpp does for #9481: two
// hand-built pre-tds7 logins, identical except for the typeflags block,
// then the same query on each, then the raw response bytes of both
// compared against each other.
//
// The comparison is the point.  Checking that each response decodes to
// the right value in the order it declared would pass on a server that
// ignored the declaration and wrote little-endian both times, as long as
// the test decoded little-endian both times too.  So the fields that byte
// order applies to are also checked to be the exact byte-reversal of each
// other between the two connections, and the fields it doesn't apply to
// (the column name, the datatype byte, the size byte) are checked to be
// identical.

// packet header fields - see PACKET_HEADER_SIZE, STATUS_EOM and the packet
// type constants in src/protocols/tds.cpp
static const size_t		PACKET_HEADER_SIZE=8;
static const unsigned char	STATUS_EOM=0x01;
static const unsigned char	PRE_TDS7_LOGIN=0x02;
static const unsigned char	PRE_TDS7_NORMAL=0x0F;
static const unsigned char	TABULAR_RESULT=0x04;
static const unsigned char	TOKEN_LOGIN_ACK=0xAD;

// the tokens this test's response is made of - see preTds7RowFmt(),
// preTds7Rows() and done() in src/protocols/tds.cpp
static const unsigned char	TDS5_TOKEN_LANGUAGE=0x21;
static const unsigned char	TOKEN_ROW=0xD1;
static const unsigned char	TOKEN_DONE=0xFD;
static const unsigned char	TOKEN_ROWFMT=0xEE;

// the datatype byte pretds7typemap[] maps an integer column to
static const unsigned char	TDS5_TYPE_INTN=0x26;

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

// The two typeflags blocks, straight out of the TDS_* constants that
// preTds7ByteOrder() decodes against in src/protocols/tds.cpp.  Byte 0 is
// 2-byte ints, byte 1 is 4-byte ints, byte 2 is the character set family,
// byte 3 is floats, byte 4 is date/time, byte 5 is the usedb flag - see
// the PRE_TDS7_TYPE_FLAGS_* defines.
//
// little-endian: TDS_INT2_LSB_LO (3), TDS_INT4_LSB_LO (1),
// TDS_CHAR_ASCII (6), TDS_FLT_IEEE_LO (10), TDS_TWO_I4_LSB_LO (9), usedb.
// This is what a live capture of a real ct-lib client on x86 sends, and
// what tdsdialectguard.cpp sends too.
static const unsigned char	LE_TYPE_FLAGS[TYPE_FLAGS_SIZE]=
					{0x03,0x01,0x06,0x0a,0x09,0x01};

// big-endian: TDS_INT2_LSB_HI (2), TDS_INT4_LSB_HI (0), TDS_CHAR_ASCII
// (6), TDS_FLT_IEEE_HI (4), TDS_TWO_I4_LSB_HI (8), usedb.  Only the four
// byte-order bytes change - the character set stays ascii, and the usedb
// flag isn't a byte order at all.  The block still has to be non-zero as
// a whole: preTds7ByteOrder() catches an all-zero block up front and
// reads it as little-endian, because 0 is what TDS_INT4_LSB_HI happens to
// be.
static const unsigned char	BE_TYPE_FLAGS[TYPE_FLAGS_SIZE]=
					{0x02,0x00,0x06,0x04,0x08,0x01};

// The query.  0x00000102 rather than something symmetric: reversed it's
// 0x02010000, so a byte-swapped value can't be mistaken for an
// unswapped one, in a comparison or in a hex dump.  "convert(int,...)"
// pins the column to a 4 byte integer rather than leaving the width up to
// how the backend types a bare constant, and the alias pins the column
// name so both connections get a rowfmt of the same length.
static const char	*QUERY="select convert(int,258) as intcol";
static const uint32_t	EXPECTED_VALUE=258;

// a response is expected within this long; used on every read below so a
// server that hangs instead of answering fails the test instead of hanging
// it
static const int32_t	RESPONSE_TIMEOUT_SEC=10;

int	status=0;
const char	*success="\033[32msuccess\033[0m";
const char	*failure="\033[31mfailure\033[0m";

static void report(const char *label, bool ok) {
	stdoutput.printf("%s: %s\n",label,(ok)?success:failure);
	if (!ok) {
		status=1;
	}
}

static void dump(const char *label, const unsigned char *bytes, size_t size) {
	stdoutput.printf("    %s:",label);
	for (size_t i=0; i<size; i++) {
		stdoutput.printf(" %02x",bytes[i]);
	}
	stdoutput.printf("\n");
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
// mirror the reads in sqlrprotocol_tds::preTds7Login().  Only the
// typeflags block differs between the two logins this test sends; the
// two multi-byte fields that preTds7Login() reads in the declared order
// after it (deprecated and oldsecure) are both zero, so the rest of the
// record is byte-for-byte identical either way.
static void buildPreTds7Login(bytebuffer *body,
				const char *username, const char *password,
				const unsigned char *typeflags) {

	appendField(body,"",NAME_SIZE);			// hostname
	appendField(body,username,NAME_SIZE);
	appendField(body,password,NAME_SIZE);
	appendField(body,"",NAME_SIZE);			// hostproc

	body->append(typeflags,TYPE_FLAGS_SIZE);

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

// lay a 32-bit value out in the byte order the login declared, the way
// the server's write(bytebuffer *,uint32_t) does
static void appendUint32(bytebuffer *body, uint32_t value, bool bigendian) {
	unsigned char	bytes[4];
	bytes[0]=(unsigned char)((value>>24)&0xff);
	bytes[1]=(unsigned char)((value>>16)&0xff);
	bytes[2]=(unsigned char)((value>>8)&0xff);
	bytes[3]=(unsigned char)(value&0xff);
	if (bigendian) {
		body->append(bytes,sizeof(bytes));
		return;
	}
	for (size_t i=0; i<sizeof(bytes); i++) {
		body->append(bytes[sizeof(bytes)-1-i]);
	}
}

// build a tds 5.0 language token - the token byte, a 32-bit length in the
// declared byte order covering everything after it, a status byte, then
// the sql as single-byte characters with no terminator.  See
// preTds7Language() in src/protocols/tds.cpp.
//
// The length field is the first thing the server reads in the order the
// login declared, so a big-endian session that got this wrong wouldn't
// even reach the query - it's part of what's under test here.
static void buildLanguageToken(bytebuffer *body,
					const char *sql, bool bigendian) {

	size_t	sqllen=charstring::getLength(sql);

	body->append((unsigned char)TDS5_TOKEN_LANGUAGE);
	appendUint32(body,(uint32_t)(sqllen+1),bigendian);
	body->append((unsigned char)0);			// status: no params
	body->append((const unsigned char *)sql,sqllen);
}

// write one tds packet: an 8 byte header (type, status, big-endian size,
// spid, packet id, window) followed by "datasize" bytes of payload - see
// sqlrprotocol_tds::sendPacket()/recvPacket().  The header's size field
// is big-endian in every tds dialect, whatever the login declared.
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

// read a whole tds response - every packet up to and including the one
// with STATUS_EOM set - into "payload", and hand back the first packet's
// type.  The login ack and this test's result set both fit in one packet,
// but reading until end-of-message rather than assuming that keeps a
// larger response from being read as a truncated one.
static bool readTdsResponse(inetsocketclient *sock,
				unsigned char *packettype,
				bytebuffer *payload) {

	payload->clear();

	// a response this test can provoke is never more packets than this;
	// the cap keeps a server that never sets STATUS_EOM from looping here
	// forever
	const uint32_t	maxpackets=64;

	for (uint32_t packet=0; packet<maxpackets; packet++) {

		unsigned char	header[PACKET_HEADER_SIZE];
		if (sock->read(header,sizeof(header),RESPONSE_TIMEOUT_SEC,0)!=
						(ssize_t)sizeof(header)) {
			return false;
		}
		if (!packet) {
			*packettype=header[0];
		}
		size_t	packetsize=((size_t)header[2]<<8)|(size_t)header[3];
		if (packetsize<PACKET_HEADER_SIZE) {
			return false;
		}

		size_t	datasize=packetsize-PACKET_HEADER_SIZE;
		unsigned char	chunk[4096];
		while (datasize) {
			size_t	size=(datasize>sizeof(chunk))?
						sizeof(chunk):datasize;
			if (sock->read(chunk,size,RESPONSE_TIMEOUT_SEC,0)!=
							(ssize_t)size) {
				return false;
			}
			payload->append(chunk,size);
			datasize-=size;
		}

		if (header[1]&STATUS_EOM) {
			return true;
		}
	}
	return false;
}

// Log in and run the query, handing back the raw bytes of the query's
// response.  "bigendian" picks which typeflags block the login declares
// and which order the language token's length field goes out in - nothing
// else about the session differs between the two calls.
static bool runSession(const char *host, uint16_t port,
				const char *user, const char *password,
				bool bigendian, bytebuffer *response) {

	const char	*label=(bigendian)?"big-endian":"little-endian";

	inetsocketclient	sock;
	sock.setHost(host);
	sock.setPort(port);
	if (!sock.connect()) {
		stdoutput.printf("%s: connect failed\n",label);
		return false;
	}

	bytebuffer	loginbody;
	buildPreTds7Login(&loginbody,user,password,
			(bigendian)?BE_TYPE_FLAGS:LE_TYPE_FLAGS);
	if (loginbody.getSize()!=PRE_TDS7_LOGIN_SIZE) {
		stdoutput.printf("%s: built a %d byte login record, "
					"expected %d\n",label,
					(int)loginbody.getSize(),
					(int)PRE_TDS7_LOGIN_SIZE);
		sock.close();
		return false;
	}

	if (!sendTdsPacket(&sock,PRE_TDS7_LOGIN,
				(const unsigned char *)loginbody.getBuffer(),
				loginbody.getSize())) {
		stdoutput.printf("%s: sending the login failed\n",label);
		sock.close();
		return false;
	}

	unsigned char	packettype=0;
	bytebuffer	loginresponse;
	if (!readTdsResponse(&sock,&packettype,&loginresponse)) {
		stdoutput.printf("%s: no login response\n",label);
		sock.close();
		return false;
	}
	if (packettype!=TABULAR_RESULT || !loginresponse.getSize() ||
			((const unsigned char *)loginresponse.getBuffer())[0]!=
							TOKEN_LOGIN_ACK) {
		stdoutput.printf("%s: login refused\n",label);
		sock.close();
		return false;
	}

	bytebuffer	querybody;
	buildLanguageToken(&querybody,QUERY,bigendian);
	if (!sendTdsPacket(&sock,PRE_TDS7_NORMAL,
				(const unsigned char *)querybody.getBuffer(),
				querybody.getSize())) {
		stdoutput.printf("%s: sending the query failed\n",label);
		sock.close();
		return false;
	}

	if (!readTdsResponse(&sock,&packettype,response)) {
		stdoutput.printf("%s: no query response\n",label);
		sock.close();
		return false;
	}
	if (packettype!=TABULAR_RESULT) {
		stdoutput.printf("%s: query response packet type 0x%02x, "
					"expected 0x%02x\n",label,
					packettype,TABULAR_RESULT);
		sock.close();
		return false;
	}

	// sqlrservercontroller::closeClientConnection() waits for the client
	// to close its end before it closes its own, so close rather than
	// read for a server-side close
	sock.close();
	return true;
}

// The pieces of one response this test compares - the raw bytes of each
// field, kept in the order they arrived rather than decoded, plus what
// they decode to in the order the session declared.
struct rowfmtandrow {
	unsigned char	rowfmtlength[2];
	unsigned char	colcount[2];
	unsigned char	usertype[4];
	unsigned char	value[4];
	unsigned char	donerowcount[4];

	uint32_t	decodedrowfmtlength;
	uint32_t	decodedcolcount;
	uint32_t	decodedvalue;
	uint32_t	decodeddonerowcount;

	// the fields byte order doesn't apply to, kept so the two responses
	// can be checked to differ only where they're supposed to
	unsigned char	name[256];
	unsigned char	namelength;
	unsigned char	flags;
	unsigned char	datatype;
	unsigned char	valuesize;
};

static uint32_t decode16(const unsigned char *bytes, bool bigendian) {
	if (bigendian) {
		return ((uint32_t)bytes[0]<<8)|(uint32_t)bytes[1];
	}
	return ((uint32_t)bytes[1]<<8)|(uint32_t)bytes[0];
}

static uint32_t decode32(const unsigned char *bytes, bool bigendian) {
	if (bigendian) {
		return ((uint32_t)bytes[0]<<24)|((uint32_t)bytes[1]<<16)|
			((uint32_t)bytes[2]<<8)|(uint32_t)bytes[3];
	}
	return ((uint32_t)bytes[3]<<24)|((uint32_t)bytes[2]<<16)|
			((uint32_t)bytes[1]<<8)|(uint32_t)bytes[0];
}

// Walk the whole response - rowfmt, row, done - copying out the fields
// this test compares.  The walk is deliberately exact rather than a scan
// for the tokens: every field of every token is stepped over at the width
// the writers in src/protocols/tds.cpp give it, and the parse fails if
// anything doesn't land where it should.  So a response that parses at
// all is a response whose layout is already known to be right, and the
// offsets the fields were copied out of are the right ones.
//
// The layout, from preTds7RowFmt(), preTds7Field() and done():
//
//	0xEE			rowfmt token
//	uint16			token length - everything after it
//	uint16			column count
//	per column:
//		byte		name length
//		bytes		name
//		byte		flags
//		uint32		usertype
//		byte		datatype
//		byte		size (an intn is varint 1)
//		byte		locale length
//	0xD1			row token
//	per column:
//		byte		value size
//		bytes		value
//	0xFD			done token
//	uint16			status
//	uint16			transaction state
//	uint32			row count
static bool parseResponse(const unsigned char *rp, size_t size,
					bool bigendian, rowfmtandrow *out) {

	const char	*label=(bigendian)?"big-endian":"little-endian";
	size_t		pos=0;

	// rowfmt token
	if (size<5 || rp[pos]!=TOKEN_ROWFMT) {
		stdoutput.printf("%s: no rowfmt token\n",label);
		return false;
	}
	pos++;

	bytestring::copy(out->rowfmtlength,rp+pos,2);
	out->decodedrowfmtlength=decode16(out->rowfmtlength,bigendian);
	pos+=2;

	// the token length covers the column count and the column blocks
	size_t	rowfmtend=pos+out->decodedrowfmtlength;
	if (rowfmtend>size) {
		stdoutput.printf("%s: rowfmt token length %d runs past the "
					"end of a %d byte response\n",label,
					(int)out->decodedrowfmtlength,
					(int)size);
		return false;
	}

	bytestring::copy(out->colcount,rp+pos,2);
	out->decodedcolcount=decode16(out->colcount,bigendian);
	pos+=2;

	// one column - anything else and the offsets below are somebody
	// else's result set
	if (out->decodedcolcount!=1) {
		stdoutput.printf("%s: %d columns, expected 1\n",label,
						(int)out->decodedcolcount);
		return false;
	}

	// the column block
	if (pos>=rowfmtend) {
		stdoutput.printf("%s: truncated column block\n",label);
		return false;
	}
	out->namelength=rp[pos];
	pos++;
	if (pos+out->namelength>rowfmtend) {
		stdoutput.printf("%s: truncated column name\n",label);
		return false;
	}
	bytestring::copy(out->name,rp+pos,out->namelength);
	pos+=out->namelength;

	if (pos+1+4+1+1+1>rowfmtend) {
		stdoutput.printf("%s: truncated column block\n",label);
		return false;
	}
	out->flags=rp[pos];
	pos++;
	bytestring::copy(out->usertype,rp+pos,4);
	pos+=4;
	out->datatype=rp[pos];
	pos++;
	if (out->datatype!=TDS5_TYPE_INTN) {
		stdoutput.printf("%s: datatype 0x%02x, expected 0x%02x "
					"(intn)\n",label,
					out->datatype,TDS5_TYPE_INTN);
		return false;
	}
	out->valuesize=rp[pos];
	pos++;
	if (out->valuesize!=4) {
		stdoutput.printf("%s: column size %d, expected 4\n",label,
						(int)out->valuesize);
		return false;
	}
	if (rp[pos]) {
		stdoutput.printf("%s: locale length %d, expected 0\n",label,
						(int)rp[pos]);
		return false;
	}
	pos++;

	// the column block has to end exactly where the token length said
	// the token does
	if (pos!=rowfmtend) {
		stdoutput.printf("%s: column block ends at %d, rowfmt token "
					"ends at %d\n",label,
					(int)pos,(int)rowfmtend);
		return false;
	}

	// row token
	if (pos>=size || rp[pos]!=TOKEN_ROW) {
		stdoutput.printf("%s: no row token behind the rowfmt\n",label);
		return false;
	}
	pos++;

	// the field: a size byte, then the value at that width
	if (pos>=size || rp[pos]!=out->valuesize) {
		stdoutput.printf("%s: field size doesn't match the rowfmt\n",
									label);
		return false;
	}
	pos++;
	if (pos+4>size) {
		stdoutput.printf("%s: truncated field\n",label);
		return false;
	}
	bytestring::copy(out->value,rp+pos,4);
	out->decodedvalue=decode32(out->value,bigendian);
	pos+=4;

	// done token
	if (pos>=size || rp[pos]!=TOKEN_DONE) {
		stdoutput.printf("%s: no done token behind the row\n",label);
		return false;
	}
	pos++;
	if (pos+2+2+4>size) {
		stdoutput.printf("%s: truncated done token\n",label);
		return false;
	}
	pos+=2;		// status
	pos+=2;		// transaction state
	bytestring::copy(out->donerowcount,rp+pos,4);
	out->decodeddonerowcount=decode32(out->donerowcount,bigendian);
	pos+=4;

	// one row, so one done, so the response ends here
	if (pos!=size) {
		stdoutput.printf("%s: %d bytes left over behind the done "
					"token\n",label,(int)(size-pos));
		return false;
	}
	return true;
}

// The check the rest of this test exists for: one field, laid out by each
// of the two connections, has to be the exact byte-reversal of the other
// - and has to actually differ, or a symmetric value would pass this
// without the server having done anything at all.
static void reportReversed(const char *label,
				const unsigned char *le,
				const unsigned char *be,
				size_t size) {

	bool	reversed=true;
	bool	differ=false;
	for (size_t i=0; i<size; i++) {
		if (le[i]!=be[size-1-i]) {
			reversed=false;
		}
		if (le[i]!=be[i]) {
			differ=true;
		}
	}
	report(label,reversed && differ);
	if (!reversed || !differ) {
		dump("little-endian",le,size);
		dump("big-endian",be,size);
	}
}

static void reportIdentical(const char *label,
				const unsigned char *le,
				const unsigned char *be,
				size_t size) {

	bool	identical=!bytestring::compare(le,be,size);
	report(label,identical);
	if (!identical) {
		dump("little-endian",le,size);
		dump("big-endian",be,size);
	}
}

int main(int argc, char **argv) {

	stdoutput.printf("\n====== #9476 pre-tds7 byte order ======\n\n");

	// the tdssapprotocol test instance - see
	// test/sqlrelay.conf.d/tdssapprotocol.conf; it has no <auths> block,
	// so it falls back to a "tds_connectstrings" auth, which accepts the
	// user/password embedded in its <connection> string
	const char	*host="127.0.0.1";
	uint16_t	port=9031;
	const char	*user="testuser";
	const char	*password="testpassword";

	bytebuffer	leresponse;
	bool	leok=runSession(host,port,user,password,false,&leresponse);
	report("little-endian login and query",leok);

	bytebuffer	beresponse;
	bool	beok=runSession(host,port,user,password,true,&beresponse);
	report("big-endian login and query",beok);

	if (!leok || !beok) {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
		return 1;
	}

	rowfmtandrow	le;
	rowfmtandrow	be;
	bool	leparsed=parseResponse(
				(const unsigned char *)leresponse.getBuffer(),
				leresponse.getSize(),false,&le);
	report("little-endian response parsed",leparsed);
	bool	beparsed=parseResponse(
				(const unsigned char *)beresponse.getBuffer(),
				beresponse.getSize(),true,&be);
	report("big-endian response parsed",beparsed);

	if (!leparsed || !beparsed) {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
		return 1;
	}

	// each response decodes correctly in the order its login declared
	report("little-endian column count",le.decodedcolcount==1);
	report("big-endian column count",be.decodedcolcount==1);
	report("little-endian value",le.decodedvalue==EXPECTED_VALUE);
	report("big-endian value",be.decodedvalue==EXPECTED_VALUE);
	report("little-endian done row count",le.decodeddonerowcount==1);
	report("big-endian done row count",be.decodeddonerowcount==1);

	// both describe the same result set, so the decoded rowfmt token
	// length has to be the same number in both, whatever bytes it went
	// out as
	report("rowfmt token lengths agree",
		le.decodedrowfmtlength==be.decodedrowfmtlength);

	// and the raw bytes of every field byte order applies to are the
	// reverse of each other
	reportReversed("rowfmt token length byte-reversed",
			le.rowfmtlength,be.rowfmtlength,
			sizeof(le.rowfmtlength));
	reportReversed("column count byte-reversed",
			le.colcount,be.colcount,sizeof(le.colcount));
	reportReversed("value byte-reversed",
			le.value,be.value,sizeof(le.value));
	reportReversed("done row count byte-reversed",
			le.donerowcount,be.donerowcount,
			sizeof(le.donerowcount));

	// while the fields it doesn't apply to are identical.  A server that
	// byte-swapped something it shouldn't have would fail here rather
	// than pass the reversal checks above and look correct.
	report("column name lengths agree",le.namelength==be.namelength);
	if (le.namelength==be.namelength) {
		reportIdentical("column name identical",
				le.name,be.name,le.namelength);
	}
	reportIdentical("column flags identical",&le.flags,&be.flags,1);
	reportIdentical("column datatype identical",
			&le.datatype,&be.datatype,1);
	reportIdentical("column size identical",
			&le.valuesize,&be.valuesize,1);

	// usertype is a 32-bit field in the declared order, but it's always
	// 0, so it's the same bytes either way - checked as such rather than
	// left out, since a nonzero one would have to be reversed instead
	reportIdentical("usertype identical (always 0)",
			le.usertype,be.usertype,sizeof(le.usertype));

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
