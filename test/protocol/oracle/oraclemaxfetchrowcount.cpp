// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Regression coverage for #10092: a request that names the size of the row
// batch it wants, with nothing bounding what it may name.
//
// sendQuery3Response(), sendFetch3Response() and sendFetchResponse() in
// src/protocols/oracle.cpp build a whole batch into one in-memory buffer
// before any of it goes out, so the count a client sends is what decides how
// much the connection process allocates.  query3's prefetch count, fetch3's
// and the legacy fetch's rows to fetch, and query2's own row count were all
// taken off the wire unclamped, so one request naming a huge count against a
// large result set made the process build a response of whatever size the
// backend could fill - 305 MB at a million rows, measured against this
// backend - and the send buffer grows by halves and never gives the memory
// back, so the process held that footprint for the rest of its life.
//
// The counts are now refused rather than clamped, since a clamp hands back a
// short batch the client has no way to ask the rest of: a count past
// MAX_FETCH_ROW_COUNT is answered ORA-20004 before any row is fetched, and a
// batch that runs past MAX_RESPONSE_SIZE while it is being built - which a
// count inside the row limit can still do, row width being unbounded - is
// answered ORA-20005 instead of being sent.
//
// Six cases, each on its own session:
//
//	- a query3 prefetch inside the row limit, which is the control.  a
//	  batch of 200 rows on a 512 byte sdu that no single packet of it can
//	  fit, which has to come back whole and in order.  it is the normal
//	  path the refusals below sit beside, and it is what says this
//	  harness would see a batch come back short or empty
//	- a query3 prefetch count past the row limit, which has to come back
//	  ORA-20004 with no rows at all
//	- a fetch3 rows-to-fetch count past it, refused the same way.  the
//	  refusal happens before the cursor is touched, so the case fetches
//	  the whole batch afterwards on that same cursor and requires every
//	  row of it - a refusal that consumed rows would show up as a short
//	  batch here
//	- the same count through the legacy pre-query3 calls - a TTI_QUERY
//	  parse, a bindless TTI_EXECUTE and a TTI_FETCH - which reach
//	  fetch() and sendFetchResponse() rather than the modern pair.  a
//	  guard scoped to query3 and fetch3 alone would be bypassable here,
//	  so this is the case that says it isn't.  it fetches the batch
//	  afterwards too
//	- query2's own row count, the one an oci7 client's oexfet() carries
//	  in its descriptor block, past the same limit.  it is refused in
//	  query2()'s existing refusal block, ahead of the parse and the
//	  execute, and the case runs the same exact fetch again inside the
//	  limit afterwards and requires the whole batch
//	- a batch inside the row limit whose rows are wide enough to run past
//	  the response size limit while it is built: 9000 rows of 4000 bytes,
//	  around 36 MB against a 32 MB ceiling.  ORA-20005, and no rows
//
// The two refusals answer in whichever shape the session is in, the way
// sendQueryError() does, so the summary object a case reads back depends on
// the call it sent: the query3 and fetch3 cases get putSummary()'s, which
// carries an end to end sequence number the oci7 one doesn't, and the legacy
// fetch and query2 cases get putOci7Summary()'s.
//
// Every refusal case ends by opening a second cursor and running a statement
// on it.  A refusal that cost the session its place in the byte stream would
// show up there rather than in the answer itself.

// what every case's session asks for, and - since it is the floor
// recvConnectRequest() clamps to - what it gets.  small enough that an
// ordinary batch of rows can't fit one packet, which is what lets the
// control case tell a whole batch from one packet's worth of it
static const uint16_t	ORA_SPLIT_SDU=512;

// MAX_FETCH_ROW_COUNT in src/protocols/oracle.cpp, and the first count past
// it
static const uint32_t	ORA_MAX_FETCH_ROW_COUNT=100000;
static const uint32_t	ORA_OVER_LIMIT_ROWS=ORA_MAX_FETCH_ROW_COUNT+1;

// what the two refusals come back as - ORA_MAX_FETCH_ROW_COUNT_EXCEEDED and
// ORA_MAX_RESPONSE_SIZE_EXCEEDED in src/protocols/oracle.cpp
static const uint32_t	ORA_MAX_FETCH_ROW_COUNT_EXCEEDED=20004;
static const uint32_t	ORA_MAX_RESPONSE_SIZE_EXCEEDED=20005;
static const char	*ORA_MAX_FETCH_ROW_COUNT_EXCEEDED_TEXT=
			"ORA-20004: Maximum fetch row count exceeded.";
static const char	*ORA_MAX_RESPONSE_SIZE_EXCEEDED_TEXT=
			"ORA-20005: Maximum response size exceeded.";

// how many rows the batch every case asks for inside the limit has, and how
// many it asks for.  the two are deliberately equal: the point is a batch
// delivered whole, not one the result set happened to run out in the middle
// of.  it is nowhere near the limit on purpose - what it has to be able to
// show is a whole batch against a short or refused one, and 200 rows shows
// that in a fraction of a second
static const uint32_t	ORA_BATCH_ROWS=200;

// one row: four digits of row number and 56 bytes of padding behind them.
// 60 bytes goes out as 62 bytes on the wire - putRowData()'s row marker,
// then a short form clr's length byte and the value - so 200 of them are
// ~12400 bytes, twenty five packets' worth at this sdu
static const size_t	ORA_ROW_SIZE=60;
static const size_t	ORA_ROW_NUMBER_SIZE=4;
static const size_t	ORA_ROW_PAD_SIZE=56;

// and the batch the response size case asks for: a row count well inside the
// row limit, of rows wide enough that the batch runs past the 32 MB response
// ceiling while it is being built.  9000 rows of 4000 bytes is ~36 MB, which
// is over the ceiling by enough that the chunk framing and the objects around
// the rows can't decide it either way, and not so far over that the case
// spends its time fetching rows the module was always going to refuse.  4000
// is varchar2's own ceiling, so the width can't go higher and the count is
// what carries the size
static const uint32_t	ORA_WIDE_ROWS=9000;
static const uint32_t	ORA_WIDE_ROW_SIZE=4000;

// how many rows a case will decode, which is more than any of them asks
// for - an answer carrying rows nobody asked for is then counted rather than
// truncated into looking right
static const size_t	ORA_MAX_DECODED_ROWS=256;

// how wide a decoded value may be.  the short form clr's own ceiling, so a
// row that came back some other shape fails the decode rather than
// overrunning anything
static const size_t	ORA_MAX_DECODED_ROW_SIZE=ORA_CLR_MAX_SHORT_LENGTH;

// the alphabet the padding is cut from.  36 bytes long, and 36 divides
// neither the 56 byte pad nor the 502 bytes a fragment carries, so a row read
// at the wrong place shows up as shifted or swapped text rather than as
// plausible bytes
static const char	*ORA_PATTERN=
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

// 200 rows of exactly 60 characters: the row number, four digits wide and
// zero padded, then 56 bytes of the alphabet.  the row number is what makes
// every row different from every other one, so a batch that came back short,
// doubled or out of order is a wrong answer rather than the same answer two
// hundred times.
//
// "connect by level" rather than a table, so the case needs nothing set up
// ahead of it, and an alias rather than the expression's own text, so the
// column name in the describe block is one byte wide whatever oracle would
// otherwise have called it
static const char	*ORA_BATCH_QUERY=
	"select lpad(to_char(level),4,'0')||"
	"rpad('ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789',56,"
	"'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789') as v "
	"from dual connect by level<=200";

// and the wide rows, whose contents nothing reads - the whole point of them
// is their size
static const char	*ORA_WIDE_QUERY=
	"select rpad('ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789',4000,"
	"'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789') as v "
	"from dual connect by level<=9000";

// the statement every case runs on a second cursor at the end, whose value is
// unlike anything else on the wire here
static const char	*ORA_ALIVE_QUERY="select 'STILLALIVE' from dual";
static const char	*ORA_ALIVE_VALUE="STILLALIVE";

// putRowHeader()'s flags byte: 0x02 in the answer to a fetch and 0x22 in the
// answer to an execute
static const unsigned char	ORA_ROW_HEADER_FLAGS_FETCH=0x02;
static const unsigned char	ORA_ROW_HEADER_FLAGS_EXECUTE=0x22;

// TTI_QUERY2 - an oci7 client's oexec() and oexfet().  the call query2's own
// row count rides in
static const unsigned char	ORA_TTI_QUERY2=0x47;

// how wide a pointer field is on the wire.  getPointer() in
// src/protocols/oracle.cpp sizes it off the representation the client offered
// first for DATATYPE_POINTER, and oracleprotocolclient offers the universal
// one
static const size_t	ORA_POINTER_SIZE=1;

// a define descriptor's flag byte and the three fields the module keeps out
// of the counts behind it, the same values oraclemaxdescriptors.cpp builds
// its own from
static const unsigned char	ORA_DEFINE_DATATYPE=1;
static const unsigned char	ORA_DEFINE_FLAG_DEFINED=0x07;
static const uint32_t		ORA_DEFINE_BUFFER_SIZE=63;
static const uint32_t		ORA_DEFINE_CHARSET=31;

// how many counts ride behind a descriptor's four raw bytes, and how many
// fields sit behind the define count once the bind count and the pointer in
// front of it are written out
static const size_t	ORA_DESCRIPTOR_COUNTS=8;
static const size_t	ORA_DESCRIPTOR_HEADER_TAIL=7;

// one row of a decoded batch
struct oraclebatchrow {
	unsigned char	value[ORA_MAX_DECODED_ROW_SIZE];
	size_t		size;
	bool		isnull;
};

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

// what row "rownumber" of the batch holds - the same bytes the statement
// builds, built again here rather than read back off the wire
static void buildExpectedRow(char *row, uint32_t rownumber) {

	charstring::printf(row,ORA_ROW_SIZE+1,"%04d",(int)rownumber);

	size_t	patternsize=charstring::getLength(ORA_PATTERN);
	for (size_t i=0; i<ORA_ROW_PAD_SIZE; i++) {
		row[ORA_ROW_NUMBER_SIZE+i]=ORA_PATTERN[i%patternsize];
	}
	row[ORA_ROW_SIZE]='\0';
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

	stdoutput.printf("  %d bytes back, %d expected\n",
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
	stdoutput.printf("  want: ");
	stdoutput.safePrint(expected+from,(int32_t)count);
	stdoutput.printf("\n");
}

// every row of the batch against the value the statement built for it, in
// order, reporting the first row that doesn't match and where inside it the
// bytes part
static bool checkBatchRows(const oraclebatchrow *rows, size_t rowcount) {

	for (size_t i=0; i<rowcount; i++) {

		char	expected[ORA_ROW_SIZE+1];
		buildExpectedRow(expected,(uint32_t)(i+1));

		int64_t	difference=firstDifference(rows[i].value,rows[i].size,
							expected,ORA_ROW_SIZE);
		if (difference<0 && !rows[i].isnull) {
			continue;
		}

		stdoutput.printf("  row %d is wrong\n",(int)(i+1));
		if (rows[i].isnull) {
			stdoutput.printf("  it came back null\n");
			return false;
		}
		reportDifference(rows[i].value,rows[i].size,
					expected,ORA_ROW_SIZE,difference);
		return false;
	}
	return true;
}

// how many packets a message that size had to be split into - the mirror of
// sendSplitPacket() in src/protocols/oracle.cpp: every fragment repeats the
// eight byte header and the two data flag bytes, so a fragment carries
// sdu-10 bytes of body and the boundaries fall a fixed distance apart.
//
// it is arithmetic on the size and not a count of what came off the socket.
// an observed count would take a fragment counter inside
// oracleprotocolclient::recvPacket(), which keeps none, so a body that
// arrived whole in one oversized packet counts the same as one that arrived
// in pieces
static size_t fragmentCount(size_t packetsize, uint16_t sdu) {

	// too small to carry a body at all, so one packet
	if (packetsize<=ORA_FRAGMENT_OVERHEAD ||
			(size_t)sdu<=ORA_FRAGMENT_OVERHEAD) {
		return 1;
	}

	size_t	body=packetsize-ORA_FRAGMENT_OVERHEAD;
	size_t	maxbodysize=(size_t)sdu-ORA_FRAGMENT_OVERHEAD;
	return (body+maxbodysize-1)/maxbodysize;
}

// the answer measured the way the module measured it before splitting it.
// getResponseSize() counts the data flags and the body recvPacket()
// reassembled, but not the eight byte header, which is what the module's own
// "bigger than the sdu" test is against
static size_t responseFragmentCount(oracleprotocolclient *client) {
	return fragmentCount(client->getResponseSize()+
				ORA_FRAGMENT_OVERHEAD-ORA_DATA_FLAGS_SIZE,
				client->getSdu());
}


// ---- reading the answers ----

// walk a whole batch out of one logical response - the data flags, a row
// header, one row data message per row, and then the trailer, which this
// stops short of.  see putRowHeader(), putRowData() and putRow() in
// src/protocols/oracle.cpp.
//
// the row header has no fixed size: six of its fields are length prefixed
// counts, so it has to be read field by field rather than skipped as a byte
// count.  its flags byte says which call is being answered - 0x02 a fetch and
// 0x22 an execute - and comes back for the caller to check rather than being
// asserted here, since both are legitimate.
//
// a response carrying no rows at all leads with the summary object instead of
// a row header, which is not a decode failure but an answer of zero rows -
// what every refusal case here has to get
static bool readBatchRows(oracleprotocolclient *client,
				unsigned char *flags,
				uint32_t *colcount,
				uint32_t *headerrowcount,
				oraclebatchrow *rows,
				size_t maxrows,
				size_t *rowcount) {

	client->rewindResponse();

	*flags=0;
	*colcount=0;
	*headerrowcount=0;
	*rowcount=0;

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode)) {
		return false;
	}

	if (ttccode!=ORA_TTC_ROW_HEADER) {
		return true;
	}

	uint32_t	skip=0;
	if (!client->readByte(flags) ||
		(*flags!=ORA_ROW_HEADER_FLAGS_FETCH &&
			*flags!=ORA_ROW_HEADER_FLAGS_EXECUTE) ||
		!client->readLenPreInt(colcount) ||	// column count
		!client->readLenPreInt(&skip) ||	// iteration number
		!client->readLenPreInt(headerrowcount) ||
		!client->readLenPreInt(&skip) ||	// uac buffer length
		!client->readLenPreInt(&skip) ||	// bit vector size
		!client->readLenPreInt(&skip)) {	// meaning unknown
		return false;
	}

	for (;;) {

		unsigned char	marker=0;
		if (!client->readByte(&marker)) {
			return false;
		}
		if (marker!=ORA_TTC_ROW_DATA) {
			// the trailer, so the rows are done
			return true;
		}

		if (*rowcount>=maxrows) {
			return false;
		}

		oraclebatchrow	*row=&(rows[*rowcount]);
		if (!client->readLenBytes(row->value,sizeof(row->value),
					&(row->size),&(row->isnull))) {
			return false;
		}

		(*rowcount)++;
	}
}

// the same walk, behind the legacy row body instead of the modern one.  the
// row header is the one putRowHeader() builds either way, byte for byte, but
// putRow() writes two more length-prefixed fields behind each value that
// putRowData() never does: the indicator and the return code odefin() gave
// the client a pointer for.  see putRow() in src/protocols/oracle.cpp
static bool readLegacyBatchRows(oracleprotocolclient *client,
				unsigned char *flags,
				uint32_t *colcount,
				uint32_t *headerrowcount,
				oraclebatchrow *rows,
				size_t maxrows,
				size_t *rowcount) {

	client->rewindResponse();

	*flags=0;
	*colcount=0;
	*headerrowcount=0;
	*rowcount=0;

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode)) {
		return false;
	}

	if (ttccode!=ORA_TTC_ROW_HEADER) {
		return true;
	}

	uint32_t	skip=0;
	if (!client->readByte(flags) ||
		(*flags!=ORA_ROW_HEADER_FLAGS_FETCH &&
			*flags!=ORA_ROW_HEADER_FLAGS_EXECUTE) ||
		!client->readLenPreInt(colcount) ||	// column count
		!client->readLenPreInt(&skip) ||	// iteration number
		!client->readLenPreInt(headerrowcount) ||
		!client->readLenPreInt(&skip) ||	// uac buffer length
		!client->readLenPreInt(&skip) ||	// bit vector size
		!client->readLenPreInt(&skip)) {	// meaning unknown
		return false;
	}

	for (;;) {

		unsigned char	marker=0;
		if (!client->readByte(&marker)) {
			return false;
		}
		if (marker!=ORA_TTC_ROW_DATA) {
			// the trailer, so the rows are done
			return true;
		}

		if (*rowcount>=maxrows) {
			return false;
		}

		oraclebatchrow	*row=&(rows[*rowcount]);
		uint32_t	indicator=0;
		uint32_t	returncode=0;
		if (!client->readLenBytes(row->value,sizeof(row->value),
						&(row->size),&(row->isnull)) ||
			!client->readLenPreInt(&indicator) ||
			!client->readLenPreInt(&returncode)) {
			return false;
		}

		(*rowcount)++;
	}
}

// a readBatchRows()/readLegacyBatchRows() decode, picked at the call site to
// match whichever call built the response
typedef bool (*batchrowreader)(oracleprotocolclient *client,
				unsigned char *flags,
				uint32_t *colcount,
				uint32_t *headerrowcount,
				oraclebatchrow *rows,
				size_t maxrows,
				size_t *rowcount);

// the oracle error number out of the summary object an oci7 call is answered
// with.  the fields are putOci7Summary()'s in src/protocols/oracle.cpp: the
// end of call status, the rows processed, and then the number.
//
// there is an end to end sequence number between the first two, but only for
// an o5logonclient, and no session here is one: that flag takes a listener on
// verifiertype="9i", where this test's listener runs on the 11g verifier
// serverversion="11.2" defaults to
static bool readSummaryOraNumber(oracleprotocolclient *client,
						uint32_t *oranum) {

	client->rewindResponse();

	*oranum=0;

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode) || ttccode!=ORA_TTC_ERROR) {
		return false;
	}

	uint32_t	skip=0;
	return (client->readLenPreInt(&skip) &&		// end of call status
		client->readLenPreInt(&skip) &&		// rows processed
		client->readLenPreInt(oranum));
}

// the same number out of the summary object a query3 session's calls are
// answered with, which is a different object: putSummary() writes the end to
// end sequence number putOci7Summary() only writes for an o5logonclient, so
// the number sits one field further in
static bool readQuery3SummaryOraNumber(oracleprotocolclient *client,
						uint32_t *oranum) {

	client->rewindResponse();

	*oranum=0;

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode) || ttccode!=ORA_TTC_ERROR) {
		return false;
	}

	uint32_t	skip=0;
	return (client->readLenPreInt(&skip) &&		// end of call status
		client->readLenPreInt(&skip) &&		// sequence number
		client->readLenPreInt(&skip) &&		// rows processed
		client->readLenPreInt(oranum));
}

// a readSummaryOraNumber()/readQuery3SummaryOraNumber() decode, picked at the
// call site the same way the row readers are
typedef bool (*summaryreader)(oracleprotocolclient *client, uint32_t *oranum);

// whether a legacy parse or execute summary came back clean - the same walk
// readSummaryOraNumber() does, stopping at the error number
static bool legacySummarySucceeded(oracleprotocolclient *client) {

	uint32_t	oranum=0;
	return readSummaryOraNumber(client,&oranum) && !oranum;
}


// ---- the calls oracleprotocolclient doesn't have ----

// a pointer field, whose contents the module reads past without looking at
static void appendPointer(oracleprotocolclient *client) {
	for (size_t i=0; i<ORA_POINTER_SIZE; i++) {
		client->appendByte(0);
	}
}

// TTI_QUERY2 carrying a one define descriptor block and a row count - an oci7
// client's oexfet(), an exact fetch of "rowcount" rows.
//
// behind the header come ten fields, the number of positions the define list
// names, a pointer and the bind count, seven more counts, and then one
// descriptor per define.  the whole shape is packet [0027] of samples/
// 9810-redhat9x86-portable-midfetch-defines3-sqlrelay.oraproxy, the same one
// oraclemaxdescriptors.cpp and oraclemidfetchdescribe.cpp build theirs from.
//
// "rowcount" is the second of those seven counts - oexfet()'s own nrows, and
// the field this test is about.  every other field in the preamble is a zero
// the module reads past
static bool query2ExactFetch(oracleprotocolclient *client,
					unsigned char sequence,
					uint32_t cursorid,
					uint32_t rowcount) {

	client->beginTtiCall(ORA_TTI_QUERY2);
	client->appendByte(sequence);
	client->appendAuthCount(ORA_OPTION_DEFINE|ORA_OPTION_FETCH,4);
	client->appendAuthCount(cursorid,4);

	// the ten fields ahead of the define count.  the pointer right in
	// front of it is the client's address of its own define array
	appendPointer(client);
	appendPointer(client);
	client->appendAuthCount(0,4);
	client->appendAuthCount(0,4);
	appendPointer(client);
	client->appendAuthCount(0,4);
	appendPointer(client);
	client->appendAuthCount(0,4);
	appendPointer(client);
	appendPointer(client);

	client->appendAuthCount(1,4);			// one define

	// the client's address of its own bind array, a bind count of none,
	// and the seven counts the row count rides in
	appendPointer(client);
	client->appendAuthCount(0,4);
	for (size_t i=0; i<ORA_DESCRIPTOR_HEADER_TAIL; i++) {
		client->appendAuthCount((i==1)?rowcount:0,4);
	}

	// the one define descriptor - four raw bytes, then eight counts of
	// which the first is the client's buffer size and the sixth its
	// character set
	client->appendByte(ORA_DEFINE_DATATYPE);
	client->appendByte(ORA_DEFINE_FLAG_DEFINED);
	client->appendByte(0);
	client->appendByte(0);
	for (size_t i=0; i<ORA_DESCRIPTOR_COUNTS; i++) {
		uint32_t	value=0;
		if (!i) {
			value=ORA_DEFINE_BUFFER_SIZE;
		} else if (i==5) {
			value=ORA_DEFINE_CHARSET;
		}
		client->appendAuthCount(value,4);
	}

	return client->sendPacket() && client->recvPacket();
}


// ---- the pieces every case is built from ----

// the login and the cursor every case starts with, on a session small enough
// that an ordinary batch of rows can't fit one packet
static bool startSession(oracleprotocolclient *client,
				const char *mode,
				const char *host, uint16_t port,
				const char *sid,
				const char *user, const char *password,
				uint32_t *cursorid) {

	char	label[192];

	client->setSdu(ORA_SPLIT_SDU);

	charstring::printf(label,sizeof(label),"%s: connect",mode);
	if (!client->connect(host,port,sid)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report(label,true);

	charstring::printf(label,sizeof(label),
			"%s: the session runs on the small sdu",mode);
	report(label,client->getSdu()==ORA_SPLIT_SDU);

	charstring::printf(label,sizeof(label),"%s: login",mode);
	if (!client->login(user,password)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report(label,true);

	charstring::printf(label,sizeof(label),"%s: open cursor",mode);
	if (!client->open(cursorid)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report(label,true);

	return true;
}

// parse and execute one statement, leaving its whole result set for a request
// of its own.  a prefetch count of 0 and no OPTION_FETCH, so the answer is
// the describe alone
static bool parseAndExecute(oracleprotocolclient *client,
				const char *mode, const char *what,
				uint32_t cursorid, const char *query) {

	char	label[192];

	charstring::printf(label,sizeof(label),
				"%s: parse and execute %s",mode,what);
	if (!client->query3(ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,
				cursorid,0,query)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}

	bool	executed=(client->getResponseTtcCode()==ORA_TTC_DESCRIBE_INFO);
	report(label,executed);
	if (!executed) {
		reportResponse(client);
	}
	return executed;
}

// the same through the pre-query3 calls - a TTI_QUERY parse and a bindless
// TTI_EXECUTE - which is what keeps query3session false, and so what keeps
// the fetch behind it on fetch() and sendFetchResponse()
static bool legacyParseAndExecute(oracleprotocolclient *client,
					const char *mode,
					uint32_t cursorid, const char *query) {

	char	label[192];

	charstring::printf(label,sizeof(label),"%s: parse",mode);
	if (!client->legacyQuery(cursorid,query)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report(label,legacySummarySucceeded(client));

	charstring::printf(label,sizeof(label),"%s: execute",mode);
	if (!client->legacyExecute(cursorid,1,0)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	bool	executed=legacySummarySucceeded(client);
	report(label,executed);
	if (!executed) {
		reportResponse(client);
	}
	return executed;
}

// the session is still where the byte stream says it is: a fresh cursor, a
// statement on it and the row it selects.  a session left out of step by a
// half-read request or a half-written answer fails here rather than in the
// answer the case was really about
static void checkSessionSurvived(oracleprotocolclient *client,
						const char *mode) {

	char	label[192];

	uint32_t	cursorid=0;
	charstring::printf(label,sizeof(label),
			"%s: the session answers the next request",mode);
	if (!client->open(&cursorid)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return;
	}

	if (!parseAndExecute(client,mode,"on a second cursor",
						cursorid,ORA_ALIVE_QUERY)) {
		report(label,false);
		return;
	}

	if (!client->fetch(cursorid,1)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return;
	}
	report(label,client->responseContains(ORA_ALIVE_VALUE));
}

// everything a refused request's answer has to be: the summary object, the
// ora number and message that name the limit, and not one row.  the row count
// is printed rather than only asserted, since what a pre-fix run sends here is
// a count worth reading
static void checkRefusal(oracleprotocolclient *client,
				const char *mode,
				uint32_t expectedoranum,
				const char *expectedtext,
				batchrowreader readrows,
				summaryreader readoranum) {

	char	label[192];

	oraclebatchrow	rows[ORA_MAX_DECODED_ROWS];
	unsigned char	flags=0;
	uint32_t	colcount=0;
	uint32_t	headerrowcount=0;
	size_t		rowsback=0;
	bool		decoded=readrows(client,&flags,&colcount,
						&headerrowcount,rows,
						ORA_MAX_DECODED_ROWS,&rowsback);

	stdoutput.printf("  answer: %d bytes, ttc code 0x%02x, %d rows\n",
				(int)client->getResponseSize(),
				(int)client->getResponseTtcCode(),
				(int)rowsback);

	// the substantive assertion.  pre-fix the module answered a request
	// like this by building every row it asked for into memory and
	// sending the lot
	charstring::printf(label,sizeof(label),"%s: no rows came back",mode);
	report(label,decoded && !rowsback);
	if (!decoded) {
		stdoutput.printf("  the answer didn't decode\n");
		reportResponse(client);
	}

	charstring::printf(label,sizeof(label),
				"%s: the answer is a summary object",mode);
	report(label,client->getResponseTtcCode()==ORA_TTC_ERROR);

	uint32_t	oranum=0;
	bool		readnumber=readoranum(client,&oranum);
	charstring::printf(label,sizeof(label),
				"%s: the answer says ORA-%05d",
				mode,(int)expectedoranum);
	report(label,readnumber && oranum==expectedoranum);
	if (!readnumber) {
		stdoutput.printf("  the summary object didn't decode\n");
		reportResponse(client);
	} else if (oranum!=expectedoranum) {
		stdoutput.printf("  it says ORA-%05d\n",(int)oranum);
		reportResponse(client);
	}

	charstring::printf(label,sizeof(label),
				"%s: the message names the limit",mode);
	report(label,client->responseContains(expectedtext));
}

// everything a batch inside the limit has to be: every row, in order, in one
// answer too big to have fitted a single packet at this sdu.  a case that
// can't tell that from a refusal or a short batch proves nothing about the
// refusals
static void checkWholeBatch(oracleprotocolclient *client,
				const char *mode, const char *what,
				unsigned char expectedflags,
				batchrowreader readrows) {

	char	label[192];

	oraclebatchrow	rows[ORA_MAX_DECODED_ROWS];
	unsigned char	flags=0;
	uint32_t	colcount=0;
	uint32_t	headerrowcount=0;
	size_t		rowcount=0;
	bool		decoded=readrows(client,&flags,&colcount,
						&headerrowcount,rows,
						ORA_MAX_DECODED_ROWS,&rowcount);

	stdoutput.printf("  answer: %d bytes, %d packets of at most %d, "
				"%d rows\n",
				(int)client->getResponseSize(),
				(int)responseFragmentCount(client),
				(int)client->getSdu(),
				(int)rowcount);

	charstring::printf(label,sizeof(label),
				"%s: the %s response decodes",mode,what);
	report(label,decoded);
	if (!decoded) {
		reportResponse(client);
		return;
	}

	charstring::printf(label,sizeof(label),
			"%s: one column, behind a row header flagged 0x%02x",
			mode,(int)expectedflags);
	report(label,colcount==1 && flags==expectedflags);

	// the row header's own count of the rows behind it, which is a check
	// on the decode rather than on the batch: it carries the count the
	// client asked for whatever follows it, so it reads 200 either way.
	// it says the fields in front of it were read at the right offsets,
	// which is what the rows are read at too
	charstring::printf(label,sizeof(label),
			"%s: the row header counts the whole %s",mode,what);
	report(label,headerrowcount==ORA_BATCH_ROWS);
	if (headerrowcount!=ORA_BATCH_ROWS) {
		stdoutput.printf("  the header counts %d rows, %d asked for\n",
					(int)headerrowcount,(int)ORA_BATCH_ROWS);
	}

	charstring::printf(label,sizeof(label),
			"%s: every row of the %s came back in one response",
			mode,what);
	report(label,rowcount==(size_t)ORA_BATCH_ROWS);
	if (rowcount!=(size_t)ORA_BATCH_ROWS) {
		stdoutput.printf("  %d rows back, %d asked for\n",
					(int)rowcount,(int)ORA_BATCH_ROWS);
		if (!rowcount) {
			reportResponse(client);
		}
	}

	charstring::printf(label,sizeof(label),
			"%s: every row of the %s arrived whole and in order",
			mode,what);
	report(label,rowcount && checkBatchRows(rows,rowcount));

	// and that the answer could only have got here as more than one
	// packet - without this the case would pass just as well against a
	// batch that fitted one, which is about the size a refusal is
	charstring::printf(label,sizeof(label),
			"%s: the %s needed more than one packet at this sdu",
			mode,what);
	report(label,responseFragmentCount(client)>1);
}


// ---- the cases ----

// the control: a query3 execute whose prefetch count asks for the whole 200
// row batch, which is inside the limit and has to come back whole.  it is the
// shape every refusal below is a refusal of, and it is what says a batch that
// came back short or empty would be seen here.
//
// one query3 call parses and executes the statement and a second executes it
// again with the batch as its prefetch.  what keeps the describe off the
// second answer, so that it leads with the row header, is that the second one
// leaves OPTION_PARSE off - sendQuery3Response() writes a describe for
// OPTION_PARSE or OPTION_DESCRIBE and for nothing else
static void runInLimitQuery3PrefetchCase(const char *mode,
					const char *host, uint16_t port,
					const char *sid,
					const char *user, const char *password) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!startSession(&client,mode,host,port,sid,user,password,&cursorid)) {
		return;
	}

	if (!parseAndExecute(&client,mode,"the batch statement",
						cursorid,ORA_BATCH_QUERY)) {
		client.disconnect();
		return;
	}

	charstring::printf(label,sizeof(label),
				"%s: execute and prefetch the batch",mode);
	if (!client.query3(ORA_OPTION_EXECUTE|ORA_OPTION_NOPLSQL,
				cursorid,ORA_BATCH_ROWS,NULL)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	checkWholeBatch(&client,mode,"batch",ORA_ROW_HEADER_FLAGS_EXECUTE,
							readBatchRows);

	checkSessionSurvived(&client,mode);

	client.disconnect();
}

// the regression: the same execute with a prefetch count one past the row
// limit.  query3() refuses it ahead of the parse and the execute, so nothing
// is fetched and no response is built.  pre-fix the count was taken as it came
// and sendQuery3Response() built as much of it as the backend could fill
static void runOverLimitQuery3PrefetchCase(const char *mode,
					const char *host, uint16_t port,
					const char *sid,
					const char *user, const char *password) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!startSession(&client,mode,host,port,sid,user,password,&cursorid)) {
		return;
	}

	if (!parseAndExecute(&client,mode,"the batch statement",
						cursorid,ORA_BATCH_QUERY)) {
		client.disconnect();
		return;
	}

	stdoutput.printf("  request: prefetch %d rows, limit %d\n",
				(int)ORA_OVER_LIMIT_ROWS,
				(int)ORA_MAX_FETCH_ROW_COUNT);

	charstring::printf(label,sizeof(label),
			"%s: execute and prefetch past the limit",mode);
	if (!client.query3(ORA_OPTION_EXECUTE|ORA_OPTION_NOPLSQL,
				cursorid,ORA_OVER_LIMIT_ROWS,NULL)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	checkRefusal(&client,mode,ORA_MAX_FETCH_ROW_COUNT_EXCEEDED,
				ORA_MAX_FETCH_ROW_COUNT_EXCEEDED_TEXT,
				readBatchRows,readQuery3SummaryOraNumber);

	checkSessionSurvived(&client,mode);

	client.disconnect();
}

// the same count on the modern fetch: a TTI_FETCH for one row past the limit,
// on a cursor a query3 already parsed and executed, which fetch3() refuses
// before it touches the cursor at all.
//
// so the case goes on to fetch the whole batch on that same cursor and
// requires every row of it - the refusal is supposed to have cost the cursor
// nothing, and a batch that came back short here is what it would cost
static void runOverLimitFetch3Case(const char *mode,
					const char *host, uint16_t port,
					const char *sid,
					const char *user, const char *password) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!startSession(&client,mode,host,port,sid,user,password,&cursorid)) {
		return;
	}

	if (!parseAndExecute(&client,mode,"the batch statement",
						cursorid,ORA_BATCH_QUERY)) {
		client.disconnect();
		return;
	}

	stdoutput.printf("  request: fetch %d rows, limit %d\n",
				(int)ORA_OVER_LIMIT_ROWS,
				(int)ORA_MAX_FETCH_ROW_COUNT);

	charstring::printf(label,sizeof(label),
				"%s: fetch past the limit",mode);
	if (!client.fetch(cursorid,ORA_OVER_LIMIT_ROWS)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	checkRefusal(&client,mode,ORA_MAX_FETCH_ROW_COUNT_EXCEEDED,
				ORA_MAX_FETCH_ROW_COUNT_EXCEEDED_TEXT,
				readBatchRows,readQuery3SummaryOraNumber);

	charstring::printf(label,sizeof(label),
			"%s: fetch the batch on the same cursor",mode);
	if (!client.fetch(cursorid,ORA_BATCH_ROWS)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	checkWholeBatch(&client,mode,"batch",ORA_ROW_HEADER_FLAGS_FETCH,
							readBatchRows);

	checkSessionSurvived(&client,mode);

	client.disconnect();
}

// the same count again through the pre-query3 calls - a TTI_QUERY parse, a
// bindless TTI_EXECUTE and a legacy TTI_FETCH - which reach fetch() and
// sendFetchResponse() rather than fetch3() and sendFetch3Response().  a guard
// scoped to the modern pair alone would leave this path to be asked for the
// same unbounded batch, so this is the case that says it isn't
static void runOverLimitLegacyFetchCase(const char *mode,
					const char *host, uint16_t port,
					const char *sid,
					const char *user, const char *password) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!startSession(&client,mode,host,port,sid,user,password,&cursorid)) {
		return;
	}

	if (!legacyParseAndExecute(&client,mode,cursorid,ORA_BATCH_QUERY)) {
		client.disconnect();
		return;
	}

	stdoutput.printf("  request: fetch %d rows, limit %d\n",
				(int)ORA_OVER_LIMIT_ROWS,
				(int)ORA_MAX_FETCH_ROW_COUNT);

	charstring::printf(label,sizeof(label),
				"%s: fetch past the limit",mode);
	if (!client.legacyFetch(cursorid,ORA_OVER_LIMIT_ROWS)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	// a legacy session never sets query3session, so the refusal comes
	// back in the oci7 summary object rather than the one the two cases
	// above read
	checkRefusal(&client,mode,ORA_MAX_FETCH_ROW_COUNT_EXCEEDED,
				ORA_MAX_FETCH_ROW_COUNT_EXCEEDED_TEXT,
				readLegacyBatchRows,readSummaryOraNumber);

	charstring::printf(label,sizeof(label),
			"%s: fetch the batch on the same cursor",mode);
	if (!client.legacyFetch(cursorid,ORA_BATCH_ROWS)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	checkWholeBatch(&client,mode,"batch",ORA_ROW_HEADER_FLAGS_FETCH,
							readLegacyBatchRows);

	checkSessionSurvived(&client,mode);

	client.disconnect();
}

// the fourth count: the one an oci7 client's oexfet() carries in its
// descriptor block rather than in a fetch of its own.  query2() refuses it in
// the block it already refuses an over-wide define or bind list in, which is
// ahead of the parse and the execute, so the statement never runs for a
// request that was never going to be answered.
//
// the exact fetch is sent again inside the limit afterwards, and has to come
// back as the whole batch - the same pairing the two cases above use
static void runOverLimitQuery2Case(const char *mode,
					const char *host, uint16_t port,
					const char *sid,
					const char *user, const char *password) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!startSession(&client,mode,host,port,sid,user,password,&cursorid)) {
		return;
	}

	if (!legacyParseAndExecute(&client,mode,cursorid,ORA_BATCH_QUERY)) {
		client.disconnect();
		return;
	}

	stdoutput.printf("  request: exact fetch %d rows, limit %d\n",
				(int)ORA_OVER_LIMIT_ROWS,
				(int)ORA_MAX_FETCH_ROW_COUNT);

	charstring::printf(label,sizeof(label),
			"%s: exact fetch past the limit",mode);
	if (!query2ExactFetch(&client,2,cursorid,ORA_OVER_LIMIT_ROWS)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	checkRefusal(&client,mode,ORA_MAX_FETCH_ROW_COUNT_EXCEEDED,
				ORA_MAX_FETCH_ROW_COUNT_EXCEEDED_TEXT,
				readLegacyBatchRows,readSummaryOraNumber);

	charstring::printf(label,sizeof(label),
			"%s: exact fetch the batch on the same cursor",mode);
	if (!query2ExactFetch(&client,3,cursorid,ORA_BATCH_ROWS)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	checkWholeBatch(&client,mode,"exact fetch",ORA_ROW_HEADER_FLAGS_FETCH,
							readLegacyBatchRows);

	checkSessionSurvived(&client,mode);

	client.disconnect();
}

// the other limit: a row count well inside MAX_FETCH_ROW_COUNT whose rows are
// wide enough that the batch runs past MAX_RESPONSE_SIZE while it is being
// built.  the row count alone can't catch this - row width is unbounded, so a
// count the module is happy to accept still names a response of any size -
// which is why the batch loops measure the buffer as they fill it and bail
// with ORA-20005 rather than going on.
//
// this is the one case here that really does make the module fetch rows: the
// refusal is mid-batch by definition, so ~32 MB of response is built before
// the ceiling is reached.  nothing of it goes out - sendMaxResponseError()
// resets the send buffer the way sendQueryError() does - so what comes back
// is the summary object alone
static void runOverLimitResponseSizeCase(const char *mode,
					const char *host, uint16_t port,
					const char *sid,
					const char *user, const char *password) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!startSession(&client,mode,host,port,sid,user,password,&cursorid)) {
		return;
	}

	if (!parseAndExecute(&client,mode,"the wide statement",
						cursorid,ORA_WIDE_QUERY)) {
		client.disconnect();
		return;
	}

	stdoutput.printf("  request: prefetch %d rows of %d bytes, "
				"%d bytes of response\n",
				(int)ORA_WIDE_ROWS,(int)ORA_WIDE_ROW_SIZE,
				(int)(ORA_WIDE_ROWS*ORA_WIDE_ROW_SIZE));

	charstring::printf(label,sizeof(label),
			"%s: execute and prefetch the wide batch",mode);
	if (!client.query3(ORA_OPTION_EXECUTE|ORA_OPTION_NOPLSQL,
				cursorid,ORA_WIDE_ROWS,NULL)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	// the row count itself has to be inside the limit, or this case is
	// the row count case over again and says nothing about the bytes
	charstring::printf(label,sizeof(label),
			"%s: the row count asked for is inside the row limit",
			mode);
	report(label,ORA_WIDE_ROWS<=ORA_MAX_FETCH_ROW_COUNT);

	checkRefusal(&client,mode,ORA_MAX_RESPONSE_SIZE_EXCEEDED,
				ORA_MAX_RESPONSE_SIZE_EXCEEDED_TEXT,
				readBatchRows,readQuery3SummaryOraNumber);

	checkSessionSurvived(&client,mode);

	client.disconnect();
}

// the port a listener ended up on.  the default is what
// test/sqlrelay.conf.d/oracleprotocol.conf.in's @ORACLEPROTOCOLPORT1@ token
// defaults to; test/test.sh exports the real one, the same way
// oraclesplitresponse reads it
static uint16_t portFromEnvironment(const char *name, uint16_t fallback) {
	const char	*value=environment::getValue(name);
	if (charstring::isNullOrEmpty(value)) {
		return fallback;
	}
	return (uint16_t)charstring::convertToInteger(value);
}

int main(int argc, char **argv) {

	stdoutput.printf("\n====== #10092 fetch row count and response size "
							"limits ======\n\n");

	// the oracleprotocol test instance - see
	// test/sqlrelay.conf.d/oracleprotocol.conf.  it isn't a real oracle
	// server, it's a listener speaking oracle's wire protocol, so 1521 is
	// just the port it was configured with
	const char	*host="127.0.0.1";
	const char	*sid="ora1";
	const char	*user="testuser";
	const char	*password="testpassword";

	uint16_t	port=portFromEnvironment("ORACLEPROTOCOLPORT1",1521);

	// the batch the module has to honor
	runInLimitQuery3PrefetchCase("query3 prefetch inside the limit",
						host,port,sid,user,password);

	// and the four counts it has to refuse, one per entry point
	runOverLimitQuery3PrefetchCase("query3 prefetch over the limit",
						host,port,sid,user,password);
	runOverLimitFetch3Case("fetch3 over the limit",
						host,port,sid,user,password);
	runOverLimitLegacyFetchCase("legacy fetch over the limit",
						host,port,sid,user,password);
	runOverLimitQuery2Case("query2 row count over the limit",
						host,port,sid,user,password);

	// and a count inside the row limit whose rows carry the response past
	// the byte limit instead
	runOverLimitResponseSizeCase("response size over the limit",
						host,port,sid,user,password);

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
