// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Regression coverage for #10004: an inbound request whose ttc payload runs
// past the negotiated sdu, and so arrives as more than one tns packet.
//
// recvPacket() in src/protocols/oracle.cpp used to read exactly one packet and
// stop - an 8 byte header and a single read of the body it declared - with no
// path at all for a request the client had to split.  A parse that ran out of
// bytes mid-structure failed "truncated", the leftover packets sat unread on
// the socket, and the session was finished.  refillPacket() and have() now pull
// the rest in on demand, so this test's request has to succeed end to end.
//
// It is the inbound mirror of #9989, which fixed the outbound half.  The
// framing is the same in both directions and is settled there: every fragment
// is a complete data packet with its own header, the same type and flags, and
// the two data flag bytes repeated - and nothing marks a fragment as a
// continuation or marks the last one, so a fragment may end at any byte offset
// with no alignment to ttc structure.  oracleprotocolclient::sendSplitPacket()
// writes that shape from the client side; sendSplitPacket() in
// src/protocols/oracle.cpp writes it from the listener's.
//
// Three things have to line up for the request to really span packets, and all
// three are pinned here rather than left to chance:
//
//	- a small sdu.  the client asks for 512 in its connect packet, which
//	  is the tns minimum recvConnectRequest() floors at - anything smaller
//	  lands on 512 anyway, and it is what a real client would be answered
//	  if it asked for less
//	- a bind value past 252 bytes, which travels in the clr's chunked long
//	  form (#9985) - a 0xfe marker, a run of 255 byte chunks, and an empty
//	  chunk to close it.  that is what lets one request grow past a small
//	  sdu without a megabyte of values behind it, and it is the shape the
//	  ticket names as the practical way in
//	- TTI_QUERY2, which is in the (B) scope the ticket settled on: the
//	  legacy combined bind/execute call, whose bind values are read by
//	  getQuery2BindValues()
//
// The two 600 byte values put the request at 1276 bytes against a 512 byte
// sdu, so it goes out as three packets, and both packet boundaries land in the
// middle of a chunk of a bind value - the exact place the old code had no way
// to read past.  The sizes here are all fixed, so where the boundaries fall is
// fixed too; runSplitQuery2() below prints them.
//
// The answer is the two values concatenated, which comes back the same way -
// past 512 bytes, so the listener splits it too, and the client reassembles it.
// Checking the whole 1200 bytes rather than a length or a slice is what makes a
// boundary read at the wrong place impossible to survive: the two values are
// built from different alphabets, so a fragment dropped, doubled or swapped
// shows up as text rather than as a plausible answer.

// TTI_QUERY2 in src/protocols/oracle.cpp.  oracleprotocolclient has no call of
// its own for it, so the request is built here out of that class's building
// blocks - the same way oraclemidfetchdescribe.cpp and oraclerefcursorreuse.cpp
// build theirs
static const unsigned char	ORA_TTI_QUERY2=0x47;

// what the session asks for, and so - since it is the floor
// recvConnectRequest() clamps to - what it gets
static const uint16_t	ORA_SPLIT_SDU=512;

// each bind value.  600 bytes is past the clr short form's 252 twice over, so
// each one takes three raw byte chunks (255, 255 and 90), and two of them are
// enough to put the request past the sdu twice
static const size_t	ORA_SPLIT_VALUE_SIZE=600;

// room for both values back to back, and for the clr framing around either one
static const size_t	ORA_SPLIT_BUFFER_SIZE=2048;

// the program variable width the bind descriptors claim.  nothing reads it on
// an in bind - it is only a floor under an out bind's buffer - so this is what
// a real client's own char[2000] would have put there
static const uint32_t	ORA_SPLIT_BIND_BUFFER_SIZE=2000;

// how many counts a query2 bind or define descriptor carries behind its four
// raw bytes - OCI7_DEFINE_COUNTS in src/protocols/oracle.cpp.  the first is the
// buffer size and the rest are read and dropped
static const uint16_t	ORA_OCI7_DESCRIPTOR_COUNTS=8;

// the marker sendFetchResponse() writes in front of every row
static const unsigned char	ORA_ROW_MARKER=0x07;

// the statement.  one placeholder per bind, each named once, so the module's
// positional walk over the query text (getBindVariableName()) lines up with
// the positional descriptors on the wire.
//
// the spaces around the concatenation are load-bearing: afterBindVariable() in
// src/common/bindvariables.h ends a placeholder's name on whitespace and a
// short set of punctuation that doesn't include the pipe, so ":v1||:v2" reads
// as one placeholder named "v1||:v2" and the bind never matches
static const char	*splitquery="select :v1 || :v2 from dual";

int	status=0;
const char	*success="\033[32msuccess\033[0m";
const char	*failure="\033[31mfailure\033[0m";

static void report(const char *label, bool ok) {
	stdoutput.printf("%s: %s\n",label,(ok)?success:failure);
	if (!ok) {
		status=1;
	}
}

static void reportResponse(oracleprotocolclient *client) {
	stdoutput.printf("response (%d bytes):\n",
				(int)client->getResponseSize());
	stdoutput.safePrint(client->getResponse(),
				(int32_t)client->getResponseSize());
	stdoutput.printf("\n");
}

// two values from two alphabets, neither of which divides 255 or 502, so a
// chunk or a packet boundary read at the wrong place shows up as shifted or
// swapped text rather than as plausible bytes
static void buildValue(char *value, size_t valuesize, const char *pattern) {
	size_t	patternsize=charstring::getLength(pattern);
	for (size_t i=0; i<valuesize; i++) {
		value[i]=pattern[i%patternsize];
	}
	value[valuesize]='\0';
}

// where two byte strings first differ, or -1 if they don't
static int64_t firstDifference(const unsigned char *actual,
				size_t actualsize,
				const char *expected,
				size_t expectedsize) {
	size_t	size=(actualsize<expectedsize)?actualsize:expectedsize;
	for (size_t i=0; i<size; i++) {
		if (actual[i]!=(unsigned char)expected[i]) {
			return (int64_t)i;
		}
	}
	if (actualsize!=expectedsize) {
		return (int64_t)size;
	}
	return -1;
}

// say where each packet boundary of the request about to go out falls, so a
// run says outright that the boundaries really do land inside a bind value
// rather than leaving it to the sizes to imply
static void reportFragments(size_t requestsize, uint16_t sdu) {

	size_t	overhead=8+2;
	size_t	body=requestsize-overhead;
	size_t	maxbodysize=(size_t)sdu-overhead;
	size_t	fragments=(body+maxbodysize-1)/maxbodysize;

	stdoutput.printf("  request: %d bytes, %d packets of at most %d\n",
				(int)requestsize,(int)fragments,(int)sdu);
	for (size_t i=1; i<fragments; i++) {
		stdoutput.printf("    boundary %d at request byte %d\n",
				(int)i,(int)(overhead+i*maxbodysize));
	}
}

// a TTI_QUERY2 that binds and executes an already-parsed statement, written
// out as several packets rather than one.
//
// the field order is what query2() and getQuery2Descriptors() read in
// src/protocols/oracle.cpp: a raw sequence byte, the options bitmask and the
// cursor id as counts, then the descriptor block - twenty fields of header, one
// descriptor per define and then per bind, and last a single TTC_ROW_DATA byte
// and one clr per bind.  OPTION_PARSE is left clear, so no query text rides in
// this request; the statement was parsed by the TTI_QUERY ahead of it.
//
// the descriptor header's twenty fields are six pointers and fourteen counts,
// interleaved, of which only two carry anything: the eleventh field is the
// define count and the thirteenth the bind count.  a pointer is one byte here -
// pointersize follows the pointer data type's negotiated representation, and
// this client offers no pointer type at all, which leaves it at the universal
// one byte
static bool sendSplitQuery2(oracleprotocolclient *client,
				uint32_t cursorid,
				const oracleprotocolbindvalue *values,
				uint32_t bindcount) {

	client->beginTtiCall(ORA_TTI_QUERY2);
	client->appendByte(1);				// sequence number
	client->appendAuthCount(ORA_OPTION_BIND|ORA_OPTION_EXECUTE,4);
	client->appendAuthCount(cursorid,4);

	// the descriptor block header
	client->appendAuthPointer();
	client->appendAuthPointer();
	client->appendAuthCount(0,4);
	client->appendAuthCount(0,4);
	client->appendAuthPointer();
	client->appendAuthCount(0,4);
	client->appendAuthPointer();
	client->appendAuthCount(0,4);
	client->appendAuthPointer();
	client->appendAuthPointer();
	client->appendAuthCount(0,4);			// define count
	client->appendAuthPointer();
	client->appendAuthCount(bindcount,4);		// bind count
	for (uint16_t i=0; i<7; i++) {
		client->appendAuthCount(0,4);
	}

	// one bind descriptor per placeholder
	for (uint32_t i=0; i<bindcount; i++) {
		client->appendByte(ORA_TYPE_VARCHAR);	// wire data type
		client->appendByte(0);			// flag
		client->appendByte(0);			// precision
		client->appendByte(0);			// scale
		client->appendAuthCount(ORA_SPLIT_BIND_BUFFER_SIZE,4);
		for (uint16_t j=1; j<ORA_OCI7_DESCRIPTOR_COUNTS; j++) {
			client->appendAuthCount(0,4);
		}
	}

	// and the values, in bind order
	client->appendByte(ORA_TTC_ROW_DATA);
	for (uint32_t i=0; i<bindcount; i++) {
		client->appendLenBytes(values[i].value,values[i].size);
	}

	reportFragments(client->getRequestSize(),client->getSdu());

	return client->sendSplitPacket() && client->recvPacket();
}

// walk the plainest legacy fetch response and pull the single column of the
// single row out of it - the data flags, TTC_ROW_HEADER and its flags byte,
// six counts, then the row marker and the value, with the indicator and return
// code putRow() writes behind it.  see sendFetchResponse() and putRowHeader()
// in src/protocols/oracle.cpp, and readLegacyFetchRows() in
// oraclelegacyfetch.cpp, which walks the same shape for a NUMBER column
static bool readLegacyFetchRow(oracleprotocolclient *client,
				uint32_t *colcount,
				unsigned char *value,
				size_t maxsize,
				size_t *valuesize,
				bool *isnull) {

	client->rewindResponse();

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	unsigned char	flags=0;
	uint32_t	skip=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode) ||
		ttccode!=ORA_TTC_ROW_HEADER ||
		!client->readByte(&flags) ||
		!client->readLenPreInt(colcount) ||	// column count
		!client->readLenPreInt(&skip) ||	// iteration number
		!client->readLenPreInt(&skip) ||	// row count
		!client->readLenPreInt(&skip) ||	// uac buffer length
		!client->readLenPreInt(&skip) ||	// bit vector size
		!client->readLenPreInt(&skip)) {	// meaning unknown
		return false;
	}

	unsigned char	marker=0;
	if (!client->readByte(&marker) || marker!=ORA_ROW_MARKER) {
		return false;
	}

	uint32_t	indicator=0;
	uint32_t	returncode=0;
	return client->readLenBytes(value,maxsize,valuesize,isnull) &&
		client->readLenPreInt(&indicator) &&
		client->readLenPreInt(&returncode);
}

int main(int argc, char **argv) {

	stdoutput.printf("\n====== #10004 multi-packet inbound request "
							"======\n\n");

	// the oracleprotocol test instance - see
	// test/sqlrelay.conf.d/oracleprotocol.conf.  it isn't a real oracle
	// server, it's a listener speaking oracle's wire protocol, so 1521 is
	// just the port it was configured with.  ORACLEPROTOCOLPORT1 names the
	// port it actually ended up on, the same way oracleclrchunks reads it
	const char	*host="127.0.0.1";
	uint16_t	port=1521;
	const char	*sid="ora1";
	const char	*user="testuser";
	const char	*password="testpassword";

	const char	*portoverride=
			environment::getValue("ORACLEPROTOCOLPORT1");
	if (!charstring::isNullOrEmpty(portoverride)) {
		port=(uint16_t)charstring::convertToInteger(portoverride);
	}

	char	value1[ORA_SPLIT_VALUE_SIZE+1];
	char	value2[ORA_SPLIT_VALUE_SIZE+1];
	buildValue(value1,ORA_SPLIT_VALUE_SIZE,
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
	buildValue(value2,ORA_SPLIT_VALUE_SIZE,
				"abcdefghijklmnopqrstuvwxyz9876543210");

	char	expected[2*ORA_SPLIT_VALUE_SIZE+1];
	bytestring::copy(expected,value1,ORA_SPLIT_VALUE_SIZE);
	bytestring::copy(expected+ORA_SPLIT_VALUE_SIZE,
				value2,ORA_SPLIT_VALUE_SIZE);
	expected[2*ORA_SPLIT_VALUE_SIZE]='\0';

	oracleprotocolclient	client;
	client.setSdu(ORA_SPLIT_SDU);

	if (!client.connect(host,port,sid)) {
		report("connect",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("connect",true);

	stdoutput.printf("  negotiated sdu: %d\n",(int)client.getSdu());
	report("the session runs on the small sdu",
			client.getSdu()==ORA_SPLIT_SDU);

	if (!client.login(user,password)) {
		report("login",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("login",true);

	uint32_t	cursorid=0;
	if (!client.open(&cursorid)) {
		report("open cursor",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("open cursor",true);

	// the parse, which fits in one packet and is only here so the bind and
	// execute below have a statement to run.  it stays on the legacy
	// TTI_QUERY: a TTI_QUERY3 anywhere in the session would set
	// query3session in src/protocols/oracle.cpp and hand every call behind
	// it to the modern path, and TTI_QUERY2 is the call this test is for
	if (!client.legacyQuery(cursorid,splitquery)) {
		report("parse",false);
		stdoutput.printf("%s\n",client.getError());
		return status;
	}
	report("parse",true);

	// and the request this test exists for
	oracleprotocolbindvalue	values[2];
	values[0].set(value1,ORA_SPLIT_VALUE_SIZE);
	values[1].set(value2,ORA_SPLIT_VALUE_SIZE);

	if (!sendSplitQuery2(&client,cursorid,values,2)) {
		report("bind and execute",false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return status;
	}

	// a request the listener couldn't reassemble answers with an oracle
	// error - ORA-01008 where the value block ran out, ORA-00904 where the
	// descriptor walk did - rather than with the ok a real execute sends
	bool	executed=(client.getResponseTtcCode()==ORA_TTC_OK);
	report("bind and execute",executed);
	if (!executed) {
		reportResponse(&client);
		client.disconnect();
		return status;
	}

	if (!client.legacyFetch(cursorid,0)) {
		report("fetch",false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return status;
	}
	report("fetch",true);

	uint32_t	colcount=0;
	unsigned char	actual[ORA_SPLIT_BUFFER_SIZE];
	size_t		actualsize=0;
	bool		isnull=false;
	bool		decoded=readLegacyFetchRow(&client,&colcount,
						actual,sizeof(actual),
						&actualsize,&isnull);
	report("fetch response decodes",decoded);
	if (!decoded) {
		reportResponse(&client);
		client.disconnect();
		return status;
	}

	report("one column, not null",colcount==1 && !isnull);

	// the substantive assertion: both values, whole, in bind order.  the
	// two packet boundaries fell inside a chunk of one of them, so this
	// can only pass if the listener read past both of them
	int64_t	difference=firstDifference(actual,actualsize,
					expected,2*ORA_SPLIT_VALUE_SIZE);
	report("both bind values arrived whole and in order",difference<0);
	if (difference>=0) {
		stdoutput.printf("  %d bytes back, %d sent\n",
				(int)actualsize,(int)(2*ORA_SPLIT_VALUE_SIZE));
		stdoutput.printf("  first difference at byte %d\n",
				(int)difference);
		size_t	from=(difference>16)?(size_t)(difference-16):0;
		size_t	count=48;
		if (from+count>actualsize) {
			count=actualsize-from;
		}
		stdoutput.printf("  back: ");
		stdoutput.safePrint(actual+from,(int32_t)count);
		stdoutput.printf("\n");
		count=48;
		if (from+count>2*ORA_SPLIT_VALUE_SIZE) {
			count=2*ORA_SPLIT_VALUE_SIZE-from;
		}
		stdoutput.printf("  sent: ");
		stdoutput.safePrint(expected+from,(int32_t)count);
		stdoutput.printf("\n");
	}

	// and the session is still usable, which it wouldn't be if any packet
	// of the request had been left unread on the socket - the answer to
	// this would be the tail of the last one rather than an open cursor
	uint32_t	secondcursorid=0;
	report("the session survived the split request",
			client.open(&secondcursorid));

	client.disconnect();

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
