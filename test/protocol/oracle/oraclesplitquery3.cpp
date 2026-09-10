// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Regression coverage for #10036: a TTI_QUERY3 whose own request runs past the
// negotiated sdu, and so arrives as more than one tns packet.
//
// #10004 taught recvPacket() in src/protocols/oracle.cpp to pull the rest of a
// split request in on demand, but only the query/bind subtree it scoped itself
// to.  query3()'s own parse - getQuery3Request(), getQuery3Binds() and
// getQuery3BindDescriptor() - was outside that scope and still failed
// "truncated" the moment a request spanned packets.  #10036 armed reassembly
// there too, and this test is what says so: query3 is the modern OCI8+ path,
// so it is the one most likely to carry a request big enough to need it.
//
// oraclesplitrequest.cpp covers the same ground for TTI_QUERY2 and is this
// test's model, down to sendSplitPacket() and reportFragments().  Two things
// are deliberately different here:
//
//	- no legacy TTI_QUERY runs first.  oraclesplitrequest sends one to
//	  keep query3session false in src/protocols/oracle.cpp, since
//	  TTI_QUERY2 is the call it is for.  this test wants the opposite, so
//	  the first thing on the wire after the login is the split query3
//	- the split is aimed rather than just made.  three structures of a
//	  query3 request parse differently and each had its own truncation
//	  checks to convert, so a boundary landing in one says nothing about
//	  the other two
//
// So there are four cases, each on its own session:
//
//	- a boundary inside the al8i4 vector and bind descriptor walk, which
//	  is a long run of small count-prefixed fields.  twelve binds behind a
//	  300 byte statement puts one there
//	- a boundary inside the chunked query text.  a 700 byte statement is
//	  past the clr short form's 252 twice over, so it goes out as a long
//	  form of three raw byte chunks and the first boundary lands in the
//	  middle of one
//	- a boundary inside a bind value's own bytes.  a 260 byte statement
//	  and three binds put the row data block early enough that the first
//	  boundary falls inside the first 600 byte value
//	- two ordinary single-packet query3 requests written back to back
//	  before either answer is read.  that is the query3 analogue of the
//	  regression #10004 hit at its comment 9: a parse that refills where
//	  running out of bytes was the legitimate end of the message reads the
//	  next request as a continuation of this one, and both are lost
//
// Where the boundaries really fall is asserted, not assumed.  buildQuery3()
// hands back the offset of each part of the request it built, so each case
// checks that a boundary lands in the part it was built to split and prints
// where every boundary went - a run that stops covering what it claims to
// cover fails rather than passing quietly.
//
// Each case then fetches its row and compares the whole answer byte for byte
// against the values that went out, and opens a second cursor afterwards.
// That second open is the direct test for a false continuation: a parse that
// read past the end of its own request leaves the session out of step, and
// the answer to the open is the tail of something else rather than a cursor.

// what the session asks for, and so - since it is the floor
// recvConnectRequest() clamps to - what it gets
static const uint16_t	ORA_SPLIT_SDU=512;

// the last two binds of every case.  600 bytes is past the clr short form's
// 252 twice over, so each one goes out as three raw byte chunks (255, 255
// and 90), and two of them are enough to put any of these requests past the
// sdu twice
static const size_t	ORA_BIG_VALUE_SIZE=600;

// every other bind.  small enough that the number of binds moves the
// descriptor walk without moving the row data much
static const size_t	ORA_SMALL_VALUE_SIZE=8;

// how wide every bind is declared.  installQuery3Binds() in
// src/protocols/oracle.cpp sizes the buffer from this, so it has to be past
// the biggest value it carries
static const uint32_t	ORA_BIND_BUFFER_SIZE=1024;

// as many binds as any case here uses
static const uint32_t	ORA_MAX_BINDS=16;

// room for the longest statement any case builds
static const size_t	ORA_MAX_QUERY_SIZE=1024;

// room for every value concatenated - the answer to every case - with room
// to spare, so a wrong answer is compared rather than truncated into looking
// right
static const size_t	ORA_RESULT_BUFFER_SIZE=4096;

// putRowHeader()'s flags byte in the answer to a fetch
static const unsigned char	ORA_ROW_HEADER_FLAGS_FETCH=0x02;

// which part of a query3 request a byte falls in - the parts
// oracleprotocolquery3offsets in oracleprotocolclient.cpp measures
static const int	ORA_PART_HEADER=0;
static const int	ORA_PART_QUERY_TEXT=1;
static const int	ORA_PART_AL8I4_VECTOR=2;
static const int	ORA_PART_BIND_DESCRIPTORS=3;
static const int	ORA_PART_DEFINE_DESCRIPTORS=4;
static const int	ORA_PART_ROW_DATA=5;

static const char	*ORA_PART_NAMES[]={
	"the request header",
	"the chunked query text",
	"the al8i4 vector",
	"the bind descriptor walk",
	"the define descriptor walk",
	"the row data block"
};

// the alphabet every value and every run of padding is cut from.  36 bytes
// long, and 36 divides neither 255 nor 502, so a chunk or a packet boundary
// read at the wrong place shows up as shifted or swapped text rather than as
// plausible bytes
static const char	*ORA_PATTERN=
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

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

// a run of the alphabet, started "from" bytes into it so no two values in a
// request carry the same bytes - a value swapped for another one is then a
// wrong answer rather than the same answer twice
static void buildValue(char *value, size_t valuesize, size_t from) {
	size_t	patternsize=charstring::getLength(ORA_PATTERN);
	for (size_t i=0; i<valuesize; i++) {
		value[i]=ORA_PATTERN[(from+i)%patternsize];
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

static void reportDifference(const unsigned char *actual, size_t actualsize,
				const char *expected, size_t expectedsize,
				int64_t difference) {

	stdoutput.printf("  %d bytes back, %d sent\n",
				(int)actualsize,(int)expectedsize);
	stdoutput.printf("  first difference at byte %d\n",(int)difference);

	size_t	from=(difference>16)?(size_t)(difference-16):0;
	size_t	count=48;
	if (from+count>actualsize) {
		count=actualsize-from;
	}
	stdoutput.printf("  back: ");
	stdoutput.safePrint(actual+from,(int32_t)count);
	stdoutput.printf("\n");

	count=48;
	if (from+count>expectedsize) {
		count=expectedsize-from;
	}
	stdoutput.printf("  sent: ");
	stdoutput.safePrint(expected+from,(int32_t)count);
	stdoutput.printf("\n");
}

// "select :b1 || :b2 || ... || :bN from dual", padded out to exactly
// "querysize" bytes with a trailing comment.  the size is what the case is
// really choosing - it is what moves everything behind the query text, and
// so what decides which structure a packet boundary lands in.
//
// the padding is a comment carrying the alphabet rather than a run of
// blanks, so a chunk of the text read at the wrong place breaks the comment
// and comes back an oracle syntax error rather than something the backend
// still runs.  it carries no colon, so nothing in it reads as a placeholder.
//
// the spaces around the concatenation are load-bearing: afterBindVariable()
// in src/common/bindvariables.h ends a placeholder's name on whitespace and
// a short set of punctuation that doesn't include the pipe, so ":b1||:b2"
// reads as one placeholder named "b1||:b2" and the bind never matches
static bool buildQuery(char *query, size_t querysize, uint32_t bindcount) {

	query[0]='\0';
	charstring::append(query,"select ");
	for (uint32_t i=0; i<bindcount; i++) {
		if (i) {
			charstring::append(query," || ");
		}
		charstring::append(query,":b");
		charstring::append(query,(uint64_t)(i+1));
	}
	charstring::append(query," from dual ");

	size_t	length=charstring::getLength(query);
	if (length+4>querysize) {
		return false;
	}

	size_t	patternsize=charstring::getLength(ORA_PATTERN);
	query[length++]='/';
	query[length++]='*';
	while (length<querysize-2) {
		query[length]=ORA_PATTERN[length%patternsize];
		length++;
	}
	query[length++]='*';
	query[length++]='/';
	query[length]='\0';
	return true;
}

// which part of the request a byte offset falls in
static int partAt(const oracleprotocolquery3offsets *offsets, size_t offset) {
	if (offset<offsets->querytext) {
		return ORA_PART_HEADER;
	}
	if (offset<offsets->al8i4vector) {
		return ORA_PART_QUERY_TEXT;
	}
	if (offset<offsets->binddescriptors) {
		return ORA_PART_AL8I4_VECTOR;
	}
	if (offset<offsets->definedescriptors) {
		return ORA_PART_BIND_DESCRIPTORS;
	}
	if (offset<offsets->rowdata) {
		return ORA_PART_DEFINE_DESCRIPTORS;
	}
	return ORA_PART_ROW_DATA;
}

// which bind's value a byte of the row data block falls inside, counted from
// 1, and whether it landed on the value's own bytes rather than on the clr
// framing around them.  the long form interleaves a length byte with every
// 255 byte chunk, so this walks the block rather than measuring it.  the
// walk mirrors appendLenBytes() in oracleprotocolclient.cpp, which is what
// wrote the block
static uint32_t bindValueAt(const oracleprotocolbindvalue *values,
				uint32_t bindcount,
				size_t offset,
				bool *invaluebytes) {

	*invaluebytes=false;

	// the block's own marker byte
	size_t	at=1;

	for (uint32_t i=0; i<bindcount; i++) {

		size_t	size=values[i].size;
		size_t	start=at;

		if (size<=ORA_CLR_MAX_SHORT_LENGTH) {
			at=at+1+size;
			if (offset>=start && offset<at) {
				*invaluebytes=(offset>start);
				return i+1;
			}
			continue;
		}

		// the long form marker
		at++;

		size_t	done=0;
		while (done<size) {
			size_t	chunk=size-done;
			if (chunk>ORA_CLR_MAX_CHUNK_SIZE) {
				chunk=ORA_CLR_MAX_CHUNK_SIZE;
			}
			// the chunk's length byte, then the chunk
			at++;
			if (offset>=at && offset<at+chunk) {
				*invaluebytes=true;
				return i+1;
			}
			at=at+chunk;
			done=done+chunk;
		}

		// the closing empty chunk
		at++;
		if (offset>=start && offset<at) {
			return i+1;
		}
	}
	return 0;
}

// where the request about to go out is going to be cut, counted from the
// front of the packet.  the mirror of sendSplitPacket() in
// oracleprotocolclient.cpp: every fragment repeats the eight byte header and
// the two data flag bytes, so a fragment carries sdu-10 bytes of body and
// the boundaries fall a fixed distance apart
static size_t fragmentCount(size_t requestsize, uint16_t sdu) {
	size_t	body=requestsize-ORA_FRAGMENT_OVERHEAD;
	size_t	maxbodysize=(size_t)sdu-ORA_FRAGMENT_OVERHEAD;
	return (body+maxbodysize-1)/maxbodysize;
}

static size_t boundaryAt(size_t fragment, uint16_t sdu) {
	return ORA_FRAGMENT_OVERHEAD+
		fragment*((size_t)sdu-ORA_FRAGMENT_OVERHEAD);
}

// say where each packet boundary of the request about to go out falls and
// what it lands in the middle of, so a run says outright what it covered
// rather than leaving the sizes to imply it
static void reportFragments(size_t requestsize, uint16_t sdu,
				const oracleprotocolquery3offsets *offsets,
				const oracleprotocolbindvalue *values,
				uint32_t bindcount) {

	size_t	fragments=fragmentCount(requestsize,sdu);

	stdoutput.printf("  request: %d bytes, %d packets of at most %d\n",
				(int)requestsize,(int)fragments,(int)sdu);
	stdoutput.printf("    query text at %d, al8i4 vector at %d, "
				"bind descriptors at %d, row data at %d\n",
				(int)offsets->querytext,
				(int)offsets->al8i4vector,
				(int)offsets->binddescriptors,
				(int)offsets->rowdata);

	for (size_t i=1; i<fragments; i++) {

		size_t	at=boundaryAt(i,sdu);
		int	part=partAt(offsets,at);

		stdoutput.printf("    boundary %d at request byte %d, "
					"inside %s",
					(int)i,(int)at,ORA_PART_NAMES[part]);

		if (part==ORA_PART_ROW_DATA) {
			bool		invaluebytes=false;
			uint32_t	bind=bindValueAt(values,bindcount,
						at-offsets->rowdata,
						&invaluebytes);
			if (bind) {
				stdoutput.printf(" - bind %d's %s",(int)bind,
					(invaluebytes)?
						"value bytes":"clr framing");
			}
		}
		stdoutput.printf("\n");
	}
}

// whether some boundary really does land in the part the case was built to
// split - what makes a case's coverage an assertion rather than a claim in a
// comment.  a boundary in the row data block only counts where it lands on a
// value's own bytes, since a boundary on the clr framing exercises a
// different reader
static bool boundaryLandsIn(size_t requestsize, uint16_t sdu,
				const oracleprotocolquery3offsets *offsets,
				int part,
				const oracleprotocolbindvalue *values,
				uint32_t bindcount) {

	size_t	fragments=fragmentCount(requestsize,sdu);

	for (size_t i=1; i<fragments; i++) {

		size_t	at=boundaryAt(i,sdu);
		if (partAt(offsets,at)!=part) {
			continue;
		}
		if (part!=ORA_PART_ROW_DATA) {
			return true;
		}

		bool		invaluebytes=false;
		uint32_t	bind=bindValueAt(values,bindcount,
						at-offsets->rowdata,
						&invaluebytes);
		if (bind && invaluebytes) {
			return true;
		}
	}
	return false;
}

// walk a fetch response - the data flags, a row header, one row data message
// carrying the single column, and then the summary object, which this stops
// short of.  see sendFetch3Response(), putRowHeader() and putRowData() in
// src/protocols/oracle.cpp.  the same walk oraclebigchunkclroff.cpp does
static bool readFetch3Row(oracleprotocolclient *client,
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
		flags!=ORA_ROW_HEADER_FLAGS_FETCH ||
		!client->readLenPreInt(colcount) ||
		!client->readLenPreInt(&skip) ||	// iteration number
		!client->readLenPreInt(&skip) ||	// row count
		!client->readLenPreInt(&skip) ||	// uac buffer length
		!client->readLenPreInt(&skip) ||	// bit vector size
		!client->readLenPreInt(&skip)) {	// meaning unknown
		return false;
	}

	if (!client->readByte(&ttccode) || ttccode!=ORA_TTC_ROW_DATA) {
		return false;
	}

	return client->readLenBytes(value,maxsize,valuesize,isnull);
}

// the values every case binds: "bindcount" of them, the last two 600 bytes
// and the rest 8, each cut from a different place in the alphabet.  the
// answer is all of them concatenated, which is what the statement selects
static void buildValues(char *storage, size_t storagesize,
				oracleprotocolbindvalue *values,
				uint32_t bindcount,
				char *expected, size_t *expectedsize) {

	size_t	at=0;
	*expectedsize=0;

	for (uint32_t i=0; i<bindcount; i++) {

		size_t	size=(i+2>=bindcount)?
				ORA_BIG_VALUE_SIZE:ORA_SMALL_VALUE_SIZE;
		if (at+size+1>storagesize) {
			size=0;
		}

		buildValue(storage+at,size,i);
		values[i].set(storage+at,size);

		bytestring::copy(expected+(*expectedsize),storage+at,size);
		*expectedsize=*expectedsize+size;
		at=at+size+1;
	}
	expected[*expectedsize]='\0';
}

// one whole session: a login, a cursor, and one query3 carrying a statement
// past the clr short form, "bindcount" bind descriptors and two chunked
// values - written out as several packets none of which exceeds the sdu.
//
// "querysize" is what aims the split.  everything behind the query text
// moves with it, so it is what puts a packet boundary in "part" rather than
// somewhere else, and the case fails if it doesn't
static void runSplitCase(const char *mode,
				const char *host, uint16_t port,
				const char *sid,
				const char *user, const char *password,
				uint32_t bindcount,
				size_t querysize,
				int part) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	char	query[ORA_MAX_QUERY_SIZE+1];
	if (!buildQuery(query,querysize,bindcount)) {
		charstring::printf(label,sizeof(label),
					"%s: build the statement",mode);
		report(label,false);
		return;
	}
	stdoutput.printf("  statement: %d bytes, %d binds\n",
				(int)charstring::getLength(query),
				(int)bindcount);

	char				storage[ORA_RESULT_BUFFER_SIZE];
	char				expected[ORA_RESULT_BUFFER_SIZE];
	size_t				expectedsize=0;
	oracleprotocolbindvalue		values[ORA_MAX_BINDS];
	buildValues(storage,sizeof(storage),values,bindcount,
					expected,&expectedsize);

	oracleprotocolbind	binds[ORA_MAX_BINDS];
	for (uint32_t i=0; i<bindcount; i++) {
		binds[i].varchar(ORA_BIND_BUFFER_SIZE);
	}

	oracleprotocolclient	client;
	client.setSdu(ORA_SPLIT_SDU);

	charstring::printf(label,sizeof(label),"%s: connect",mode);
	if (!client.connect(host,port,sid)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		return;
	}
	report(label,true);

	charstring::printf(label,sizeof(label),
			"%s: the session runs on the small sdu",mode);
	report(label,client.getSdu()==ORA_SPLIT_SDU);

	charstring::printf(label,sizeof(label),"%s: login",mode);
	if (!client.login(user,password)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		return;
	}
	report(label,true);

	uint32_t	cursorid=0;
	charstring::printf(label,sizeof(label),"%s: open cursor",mode);
	if (!client.open(&cursorid)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		return;
	}
	report(label,true);

	// the request this case exists for, built but not sent - so where its
	// packet boundaries fall can be reported and asserted before it goes
	// anywhere.  no legacy call ran ahead of it, so this is the query3
	// that sets query3session in src/protocols/oracle.cpp as well as the
	// one that gets split
	oracleprotocolquery3offsets	offsets;
	charstring::printf(label,sizeof(label),"%s: build the request",mode);
	if (!client.buildQuery3(ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,
				cursorid,0,query,binds,bindcount,1,
				values,1,NULL,0,&offsets)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}

	reportFragments(client.getRequestSize(),client.getSdu(),
					&offsets,values,bindcount);

	charstring::printf(label,sizeof(label),
			"%s: the request spans more than one packet",mode);
	report(label,fragmentCount(client.getRequestSize(),
					client.getSdu())>1);

	charstring::printf(label,sizeof(label),
			"%s: a packet boundary lands inside %s",
			mode,ORA_PART_NAMES[part]);
	report(label,boundaryLandsIn(client.getRequestSize(),client.getSdu(),
					&offsets,part,values,bindcount));

	charstring::printf(label,sizeof(label),"%s: parse and execute",mode);
	if (!client.sendSplitPacket() || !client.recvPacket()) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}

	// a request the listener couldn't reassemble answers with an oracle
	// error - the parse fails "truncated" and the session gets an
	// ORA-01008 or an ORA-00904 - rather than with the describe a real
	// parse and execute sends
	bool	executed=(client.getResponseTtcCode()==ORA_TTC_DESCRIBE_INFO);
	report(label,executed);
	if (!executed) {
		reportResponse(&client);
		client.disconnect();
		return;
	}

	charstring::printf(label,sizeof(label),"%s: fetch",mode);
	if (!client.fetch(cursorid,1)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	uint32_t	colcount=0;
	unsigned char	actual[ORA_RESULT_BUFFER_SIZE];
	size_t		actualsize=0;
	bool		isnull=false;
	bool		decoded=readFetch3Row(&client,&colcount,
						actual,sizeof(actual),
						&actualsize,&isnull);
	charstring::printf(label,sizeof(label),
				"%s: fetch response decodes",mode);
	report(label,decoded);
	if (!decoded) {
		reportResponse(&client);
		client.disconnect();
		return;
	}

	charstring::printf(label,sizeof(label),
				"%s: one column, not null",mode);
	report(label,colcount==1 && !isnull);

	// the substantive assertion: every bind value, whole and in order.  a
	// boundary fell inside one of the structures that had to be read past
	// to get here, so this can only pass if the listener read past it
	charstring::printf(label,sizeof(label),
			"%s: every bind value arrived whole and in order",
			mode);
	int64_t	difference=firstDifference(actual,actualsize,
						expected,expectedsize);
	report(label,difference<0);
	if (difference>=0) {
		reportDifference(actual,actualsize,
					expected,expectedsize,difference);
	}

	// and the session is still usable, which it wouldn't be if any packet
	// of the request had been left unread on the socket, or if the parse
	// had read past the end of it - the answer to this would be the tail
	// of something else rather than an open cursor
	uint32_t	secondcursorid=0;
	charstring::printf(label,sizeof(label),
			"%s: the session survived the split request",mode);
	report(label,client.open(&secondcursorid));

	client.disconnect();
}

// two ordinary query3 requests, each inside one packet, written back to back
// before either answer is read.
//
// this is the other half of arming reassembly, and the half that regresses
// quietly.  every refill point added to the query3 parse is a place where
// running out of bytes now means "wait for more" instead of "stop", so a
// point where running out was the legitimate end of the message reads the
// next request as a continuation of this one.  #10004 hit exactly that in
// getQuery2Descriptors() and had to make the site a deliberate non-refill
// point again.
//
// two cursors rather than one, so both answers can be fetched and compared -
// a first request that swallowed the second's bytes cannot produce two right
// answers
static void runBackToBackCase(const char *mode,
				const char *host, uint16_t port,
				const char *sid,
				const char *user, const char *password) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	// two binds and a statement well under the clr short form's 252
	// bytes, so each request is a few hundred bytes and goes out whole
	const char	*query="select :b1 || :b2 from dual";

	oracleprotocolbind	binds[2];
	binds[0].varchar(ORA_BIND_BUFFER_SIZE);
	binds[1].varchar(ORA_BIND_BUFFER_SIZE);

	char	first1[ORA_SMALL_VALUE_SIZE+1];
	char	first2[ORA_SMALL_VALUE_SIZE+1];
	char	second1[ORA_SMALL_VALUE_SIZE+1];
	char	second2[ORA_SMALL_VALUE_SIZE+1];
	buildValue(first1,ORA_SMALL_VALUE_SIZE,0);
	buildValue(first2,ORA_SMALL_VALUE_SIZE,5);
	buildValue(second1,ORA_SMALL_VALUE_SIZE,11);
	buildValue(second2,ORA_SMALL_VALUE_SIZE,17);

	oracleprotocolbindvalue	firstvalues[2];
	oracleprotocolbindvalue	secondvalues[2];
	firstvalues[0].set(first1,ORA_SMALL_VALUE_SIZE);
	firstvalues[1].set(first2,ORA_SMALL_VALUE_SIZE);
	secondvalues[0].set(second1,ORA_SMALL_VALUE_SIZE);
	secondvalues[1].set(second2,ORA_SMALL_VALUE_SIZE);

	char	firstexpected[2*ORA_SMALL_VALUE_SIZE+1];
	char	secondexpected[2*ORA_SMALL_VALUE_SIZE+1];
	charstring::copy(firstexpected,first1);
	charstring::append(firstexpected,first2);
	charstring::copy(secondexpected,second1);
	charstring::append(secondexpected,second2);

	oracleprotocolclient	client;
	client.setSdu(ORA_SPLIT_SDU);

	charstring::printf(label,sizeof(label),"%s: connect",mode);
	if (!client.connect(host,port,sid)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		return;
	}
	report(label,true);

	charstring::printf(label,sizeof(label),"%s: login",mode);
	if (!client.login(user,password)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		return;
	}
	report(label,true);

	uint32_t	firstcursorid=0;
	uint32_t	secondcursorid=0;
	charstring::printf(label,sizeof(label),"%s: open two cursors",mode);
	if (!client.open(&firstcursorid) || !client.open(&secondcursorid)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	uint32_t	options=ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL;

	charstring::printf(label,sizeof(label),
			"%s: both requests fit one packet each",mode);
	bool	built=client.buildQuery3(options,firstcursorid,0,query,
					binds,2,1,firstvalues,1);
	size_t	firstsize=client.getRequestSize();
	report(label,built && firstsize<=(size_t)client.getSdu());
	if (!built) {
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	stdoutput.printf("  request: %d bytes against a %d byte sdu\n",
				(int)firstsize,(int)client.getSdu());

	charstring::printf(label,sizeof(label),
			"%s: write both requests before reading either",mode);
	if (!client.sendPacket() ||
		!client.buildQuery3(options,secondcursorid,0,query,
						binds,2,1,secondvalues,1) ||
		!client.sendPacket()) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	// a parse that ran past the end of the first request took the second
	// one's packet with it, and there is then only one answer to be had
	charstring::printf(label,sizeof(label),
			"%s: the first request is answered on its own",mode);
	if (!client.recvPacket()) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,client.getResponseTtcCode()==ORA_TTC_DESCRIBE_INFO);
	stdoutput.printf("  response: %d bytes\n",
				(int)client.getResponseSize());

	charstring::printf(label,sizeof(label),
			"%s: the second request is answered too",mode);
	if (!client.recvPacket()) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,client.getResponseTtcCode()==ORA_TTC_DESCRIBE_INFO);
	stdoutput.printf("  response: %d bytes\n",
				(int)client.getResponseSize());

	// and both rows, which is what says the two requests' bytes stayed
	// apart rather than just that two answers came back
	for (uint32_t i=0; i<2; i++) {

		uint32_t	cursorid=(i)?secondcursorid:firstcursorid;
		const char	*expected=(i)?secondexpected:firstexpected;

		charstring::printf(label,sizeof(label),
					"%s: fetch %d",mode,(int)(i+1));
		if (!client.fetch(cursorid,1)) {
			report(label,false);
			stdoutput.printf("%s\n",client.getError());
			client.disconnect();
			return;
		}
		report(label,true);

		uint32_t	colcount=0;
		unsigned char	actual[ORA_RESULT_BUFFER_SIZE];
		size_t		actualsize=0;
		bool		isnull=false;
		bool		decoded=readFetch3Row(&client,&colcount,
						actual,sizeof(actual),
						&actualsize,&isnull);
		charstring::printf(label,sizeof(label),
				"%s: fetch %d response decodes",
				mode,(int)(i+1));
		report(label,decoded);
		if (!decoded) {
			reportResponse(&client);
			client.disconnect();
			return;
		}

		charstring::printf(label,sizeof(label),
			"%s: request %d's own values came back",
			mode,(int)(i+1));
		int64_t	difference=firstDifference(actual,actualsize,expected,
					2*ORA_SMALL_VALUE_SIZE);
		report(label,colcount==1 && !isnull && difference<0);
		if (difference>=0) {
			reportDifference(actual,actualsize,expected,
					2*ORA_SMALL_VALUE_SIZE,difference);
		}
	}

	uint32_t	thirdcursorid=0;
	charstring::printf(label,sizeof(label),
			"%s: the session survived both requests",mode);
	report(label,client.open(&thirdcursorid));

	client.disconnect();
}

// the port a listener ended up on.  the default is what
// test/sqlrelay.conf.d/oracleprotocol.conf.in's @ORACLEPROTOCOLPORT1@ token
// defaults to; test/test.sh exports the real one, the same way
// oraclesplitrequest reads it
static uint16_t portFromEnvironment(const char *name, uint16_t fallback) {
	const char	*value=environment::getValue(name);
	if (charstring::isNullOrEmpty(value)) {
		return fallback;
	}
	return (uint16_t)charstring::convertToInteger(value);
}

int main(int argc, char **argv) {

	stdoutput.printf("\n====== #10036 multi-packet query3 request "
							"======\n\n");

	// the oracleprotocol test instance - see
	// test/sqlrelay.conf.d/oracleprotocol.conf.  it isn't a real oracle
	// server, it's a listener speaking oracle's wire protocol, so 1521 is
	// just the port it was configured with
	const char	*host="127.0.0.1";
	const char	*sid="ora1";
	const char	*user="testuser";
	const char	*password="testpassword";

	uint16_t	port=portFromEnvironment("ORACLEPROTOCOLPORT1",1521);

	// twelve binds behind a 300 byte statement: the query text ends well
	// before the first boundary and the descriptor walk is long enough to
	// still be running when it arrives
	runSplitCase("split in the descriptor walk",
			host,port,sid,user,password,
			12,300,ORA_PART_BIND_DESCRIPTORS);

	// a 700 byte statement, so the chunked long form of the query text is
	// still being read when the first boundary arrives
	runSplitCase("split in the query text",
			host,port,sid,user,password,
			8,700,ORA_PART_QUERY_TEXT);

	// a 260 byte statement and three binds: everything ahead of the row
	// data is short, so the first boundary falls inside the first 600
	// byte value
	runSplitCase("split in a bind value",
			host,port,sid,user,password,
			3,260,ORA_PART_ROW_DATA);

	// and the other direction - two whole requests, back to back
	runBackToBackCase("back to back requests",
			host,port,sid,user,password);

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
