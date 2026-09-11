// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Regression coverage for #10056: a row batch whose answer runs past the
// negotiated sdu, and so goes out as more than one tns packet.
//
// sendFetchResponse(), sendQuery3Response() and sendFetch3Response() in
// src/protocols/oracle.cpp each used to stop packing rows once the response
// reached the sdu and leave the rest of the batch sitting on the cursor for
// the client to come back for.  Nothing on the wire asks it to come back:
// a client handed fewer rows than it asked for performs one automatic
// continuation fetch per array fetch call and no more, so a batch that
// needed two continuations lost every row past the second answer outright,
// silently and with no error anywhere.  #10030 fixed the legacy path and
// #10056 the two modern ones - all three now pack the whole requested batch
// into one logical response and let sendPacket() hand it to
// sendSplitPacket() (#9989), which is what divides it at the sdu.
//
// oraclesplitrequest.cpp and oraclesplitquery3.cpp cover the receive side -
// a client request that arrives as more than one packet.  This is the send
// side, and reaching it takes a particular shape of result set.  A single
// big row will not do it: the old cap ran before the row was written and
// was guarded by "some row has already gone out", so a batch's first row
// was exempt however wide it was - which is why oraclesplitquery3.cpp
// already fetches a multi-kilobyte row at this same 512 byte sdu and finds
// nothing wrong.  The regression is only reachable with many ordinary width
// rows, so every case here asks for 20 rows of 120 bytes against a 512 byte
// sdu: 122 bytes a row on the wire, five packets of answer, and two rows
// before the old cap fired.
//
// Three cases, each on its own session:
//
//	- the fetch3 batch - a TTI_FETCH for 20 rows on a cursor a query3
//	  already parsed and executed.  sendFetch3Response()
//	- the query3 prefetch batch - the same 20 rows, asked for through an
//	  execute-only query3's prefetch count rather than through a fetch of
//	  their own.  sendQuery3Response()
//	- the legacy fetch batch - the same 20 rows again, through the
//	  pre-query3 calls: a TTI_QUERY parse, a bindless TTI_EXECUTE and a
//	  TTI_FETCH.  sendFetchResponse()
//
// The legacy case also guards #10030 and #10063. A bindless legacy execute
// used to fail with ORA-01036, "unrecognized bind variable :b1", once run
// after a bind-heavy session on the same pool: open() in
// src/protocols/oracle.cpp hands out a pooled cursor without clearing it,
// the legacy query() never called clearParams() the way query2() and
// query3() do, and execute() only cleared when the request carried binds
// of its own - so a bindless legacy execute re-applied input binds an
// earlier client session had left on that cursor. Running the case last,
// behind whatever the rest of the suite has already done to this pool by
// the time it gets here, is what originally caught this.
//
// Two more cases, unrelated to the row-batch split above, cover #10080: the
// same #10063 gap in osql7() and parseExecute(), the TTI_OSQL7 and
// TTI_PARSE_EXECUTE entry points, which were missing the same
// clearParams()/clearDefines() calls. Each dirties its own cursor with a
// bind-heavy parse and execute first, rather than relying on the pool state
// the legacy case above depends on, then reparses a bindless statement over
// it and confirms the reparse's own execute - separate for osql7(), combined
// for parseExecute() - no longer comes back ORA-01036.
//
// Each of the three row-batch cases decodes the whole batch out of the one
// logical response, checks every row against the value the statement built
// for it, and prints how
// many packets the answer needed - a run that stops covering what it
// claims to cover fails rather than passing quietly.  Each then asks for
// another 20 rows and requires none: nothing may be held back for a
// continuation fetch, which is how the old cap deferred the rest of a batch
// and is the one thing a real client would only ask for once.  And each
// opens a second cursor afterwards, which a session left out of step by a
// half-read answer couldn't answer.

// what the session asks for, and so - since it is the floor
// recvConnectRequest() clamps to - what it gets
static const uint16_t	ORA_SPLIT_SDU=512;

// how many rows every case asks for.  the statement below carries the same
// count, and the two have to agree: the point is a batch that is delivered
// whole, not one the result set happened to run out in the middle of
static const uint32_t	ORA_BATCH_ROWS=20;

// one row: four digits of row number and 116 bytes of padding behind them.
// 120 bytes goes out as 122 bytes on the wire - putRowData()'s row data
// marker, then a short form clr's length byte and the value - so 20 of them
// and the objects around them are ~2500 bytes, five times what one packet
// can carry at this sdu
static const size_t	ORA_ROW_SIZE=120;
static const size_t	ORA_ROW_NUMBER_SIZE=4;
static const size_t	ORA_ROW_PAD_SIZE=116;

// how many rows a case will decode, which is more than any of them asks
// for - an answer carrying too many rows is then counted rather than
// truncated into looking right
static const size_t	ORA_MAX_DECODED_ROWS=40;

// how wide a decoded value may be.  the short form clr's own ceiling, so a
// row that came back some other shape fails the decode rather than
// overrunning anything
static const size_t	ORA_MAX_DECODED_ROW_SIZE=ORA_CLR_MAX_SHORT_LENGTH;

// the alphabet the padding is cut from.  36 bytes long, and 36 divides
// neither the 116 byte pad nor the 502 bytes a fragment carries, so a row
// read at the wrong place shows up as shifted or swapped text rather than
// as plausible bytes
static const char	*ORA_PATTERN=
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

// 20 rows of exactly 120 characters: the row number, four digits wide and
// zero padded, then 116 bytes of the alphabet.  the row number is what
// makes every row different from every other one, so a batch that came back
// short, doubled or out of order is a wrong answer rather than the same
// answer twenty times.
//
// "connect by level" rather than a table, so the case needs nothing set up
// ahead of it, and an alias rather than the expression's own text, so the
// column name in the describe block is one byte wide whatever oracle would
// otherwise have called it
static const char	*ORA_BATCH_QUERY=
	"select lpad(to_char(level),4,'0')||"
	"rpad('ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789',116,"
	"'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789') as v "
	"from dual connect by level<=20";

// putRowHeader()'s flags byte: 0x02 in the answer to a fetch and 0x22 in
// the answer to an execute
static const unsigned char	ORA_ROW_HEADER_FLAGS_FETCH=0x02;
static const unsigned char	ORA_ROW_HEADER_FLAGS_EXECUTE=0x22;

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
static size_t responsePacketSize(oracleprotocolclient *client) {
	return client->getResponseSize()+
			ORA_FRAGMENT_OVERHEAD-ORA_DATA_FLAGS_SIZE;
}

static size_t responseFragmentCount(oracleprotocolclient *client) {
	return fragmentCount(responsePacketSize(client),client->getSdu());
}

// say how big the answer was and how many packets that size needs, so a run
// says outright what it covered rather than leaving the sizes to imply it.
// the count itself is deliberately not asserted to any particular value -
// the describe and summary objects around the rows are computed rather than
// captured, so the last packet's fill is not something to pin
static void reportFragments(oracleprotocolclient *client) {
	stdoutput.printf("  response: %d bytes, %d packets of at most %d\n",
				(int)client->getResponseSize(),
				(int)responseFragmentCount(client),
				(int)client->getSdu());
}

// walk a whole batch out of one logical response - the data flags, a row
// header, one row data message per row, and then the trailer, which this
// stops short of.  see putRowHeader(), putRowData() and putRow() in
// src/protocols/oracle.cpp.
//
// the row header has no fixed size: six of its fields are length prefixed
// counts, so it has to be read field by field rather than skipped as a byte
// count.  its flags byte says which call is being answered - 0x02 a fetch
// and 0x22 an execute - and comes back for the caller to check rather than
// being asserted here, since both are legitimate.  so does its own count of
// the rows behind it.
//
// a response carrying no rows at all leads with the summary object instead
// of a row header, which is not a decode failure but an answer of zero rows
// - what the fetch behind a whole batch has to get
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

// the same walk, behind the legacy TTI_FETCH's row body instead of
// TTI_FETCH3's.  the row header is the one putRowHeader() builds either
// way, byte for byte, but putRow() marks a row the same 0x07 as
// putRowData() does and then, behind the value, writes two more
// length-prefixed fields putRowData() never does: the indicator and the
// return code odefin() gave the client a pointer for.  see putRow() in
// src/protocols/oracle.cpp and readLegacyFetchRows() in
// oraclelegacyfetch.cpp, which walks the same fields for a single row
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

// a readBatchRows()/readLegacyBatchRows() decode, picked at the call site
// to match whichever call built the response
typedef bool (*batchrowreader)(oracleprotocolclient *client,
				unsigned char *flags,
				uint32_t *colcount,
				uint32_t *headerrowcount,
				oraclebatchrow *rows,
				size_t maxrows,
				size_t *rowcount);

// everything asked of a batch response, whichever call went out to get it:
// the rows themselves, the size and the packet count that say it was too big
// to have gone out as one packet, that nothing was held back for a
// continuation fetch, and that the session still works afterwards
static void checkBatch(oracleprotocolclient *client,
				const char *mode,
				unsigned char expectedflags,
				uint32_t cursorid,
				batchrowreader readrows) {

	char	label[192];

	reportFragments(client);

	oraclebatchrow	rows[ORA_MAX_DECODED_ROWS];
	unsigned char	flags=0;
	uint32_t	colcount=0;
	uint32_t	headerrowcount=0;
	size_t		rowcount=0;
	bool		decoded=readrows(client,&flags,&colcount,
						&headerrowcount,
						rows,ORA_MAX_DECODED_ROWS,
						&rowcount);
	charstring::printf(label,sizeof(label),
				"%s: the batch response decodes",mode);
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
	// on the decode rather than on the batch: a capped response claims the
	// whole batch in the header and ships two rows, so this reads 20
	// either way.  it says the fields in front of it were read at the
	// right offsets, which is what the rows are read at too
	charstring::printf(label,sizeof(label),
			"%s: the row header counts the whole batch",mode);
	report(label,headerrowcount==ORA_BATCH_ROWS);
	if (headerrowcount!=ORA_BATCH_ROWS) {
		stdoutput.printf("  the header counts %d rows, %d asked for\n",
					(int)headerrowcount,(int)ORA_BATCH_ROWS);
	}

	// the substantive assertion: the whole batch in one answer.  a
	// response capped at one packet's worth of rows carries 2 of the 20,
	// with a row header still claiming 20
	charstring::printf(label,sizeof(label),
			"%s: every row asked for came back in one response",
			mode);
	report(label,rowcount==(size_t)ORA_BATCH_ROWS);
	if (rowcount!=(size_t)ORA_BATCH_ROWS) {
		stdoutput.printf("  %d rows back, %d asked for\n",
					(int)rowcount,(int)ORA_BATCH_ROWS);
		if (!rowcount) {
			reportResponse(client);
		}
	}

	charstring::printf(label,sizeof(label),
			"%s: every row arrived whole and in order",mode);
	report(label,rowcount && checkBatchRows(rows,rowcount));

	// and that they could only have got here as more than one packet -
	// without this the case would pass just as well against a response
	// that never needed splitting, which is no coverage at all
	charstring::printf(label,sizeof(label),
			"%s: the answer is bigger than one packet",mode);
	report(label,client->getResponseSize()>(size_t)client->getSdu());

	// the same thing counted in packets rather than bytes.  both are
	// worked out from the size of the answer, so neither can tell a
	// response the module split from one it sent whole in a single
	// oversized packet - what they say is that the answer was too big to
	// have fitted in one packet at this sdu
	charstring::printf(label,sizeof(label),
			"%s: the answer needed more than one packet at this sdu",
			mode);
	report(label,responseFragmentCount(client)>1);

	// nothing may be waiting on the cursor for a continuation fetch -
	// deferring the rest of the batch to one is what the old cap did, and
	// a client only ever performs one of those per array fetch.
	//
	// a fetch past the end of the result set answers with the summary
	// object alone, so the answer has to lead with one and not just
	// decode: fetch() checks nothing it reads, and "no rows" on its own
	// is equally what a session that has stopped answering properly looks
	// like from here.  the summary carries any error in it, and an error
	// object leads with the same ttc code, so this rules out a truncated
	// or misshapen answer rather than a failed one
	charstring::printf(label,sizeof(label),
			"%s: nothing was held back for a continuation fetch",
			mode);
	if (!client->fetch(cursorid,ORA_BATCH_ROWS)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return;
	}

	unsigned char	moreflags=0;
	uint32_t	morecolcount=0;
	uint32_t	moreheaderrowcount=0;
	size_t		morerowcount=0;
	bool		moredecoded=readrows(client,&moreflags,
						&morecolcount,
						&moreheaderrowcount,rows,
						ORA_MAX_DECODED_ROWS,
						&morerowcount);
	bool		leadssummary=
			(client->getResponseTtcCode()==ORA_TTC_ERROR);
	report(label,moredecoded && leadssummary && !morerowcount);
	if (!moredecoded) {
		stdoutput.printf("  the answer didn't decode\n");
		reportResponse(client);
	} else if (morerowcount) {
		stdoutput.printf("  %d more rows came back\n",
							(int)morerowcount);
		reportResponse(client);
	} else if (!leadssummary) {
		stdoutput.printf("  the answer leads with ttc code 0x%02x, "
					"not a summary object\n",
					(int)client->getResponseTtcCode());
		reportResponse(client);
	}

	// and the session is still usable, which it wouldn't be if any packet
	// of the answer had been left unread on the socket - the answer to
	// this would be the tail of the last one rather than an open cursor
	uint32_t	secondcursorid=0;
	charstring::printf(label,sizeof(label),
			"%s: the session survived the split response",mode);
	report(label,client->open(&secondcursorid));
}

// whether a legacy parse or execute summary came back clean.
// readLegacySummary() in oraclelegacyfetch.cpp walks the whole object;
// this stops after the error number, the one field #10063 turns nonzero -
// a bindless execute on a cursor still holding another session's input
// binds answers ORA-01036 here rather than with anything a decode failure
// would catch
static bool legacySummarySucceeded(oracleprotocolclient *client) {

	client->rewindResponse();

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	uint32_t	skip=0;
	uint32_t	errornumber=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode) ||
		ttccode!=ORA_TTC_ERROR ||
		!client->readLenPreInt(&skip) ||	// end of call status
		!client->readLenPreInt(&skip) ||	// rows processed
		!client->readLenPreInt(&errornumber)) {
		return false;
	}
	return errornumber==0;
}

// ---- #10080: osql7() and parseExecute() clearing stale binds ----
//
// TTI_OSQL7 (an oci7 client's oparse()) and TTI_PARSE_EXECUTE (0x27, its
// pre-8.0 oparsex()) had the same gap #10063 fixed in open() and the legacy
// query(): neither called clearParams(), so a bind an earlier statement left
// on the cursor rode into whatever they reparsed next, and a later bindless
// execute failed ORA-01036 re-applying it.  #10080 added the missing calls
// to both.
//
// unlike runLegacyCase() above, which leans on the rest of the suite having
// already left binds on the pool's one connection, the two cases below dirty
// their own cursor first - a parse and a bound execute of their own - so
// each is a complete repro on its own and doesn't depend on run order.

static const unsigned char	ORA_TTI_OSQL7=0x4a;
static const unsigned char	ORA_TTI_QUERY2=0x47;
static const unsigned char	ORA_TTI_PARSE_EXECUTE=0x27;

// the width of a "pointer" field in this session's negotiated encoding -
// this module only ever negotiates the portable one, where a pointer is one
// byte wide.  a real value is opaque to whatever reads it back, so a nonzero
// byte stands in for one
static const size_t	ORA_POINTER_SIZE=1;

// one bind's wire datatype, buffer size and character set - VARCHAR2, wide
// enough for the values below, and the AL32UTF8 charset the module actually
// runs on.  none of the three affect whether the bind is accepted, only how
// it's decoded, so nothing here needs to match a real client's capture the
// way the query3 offsets elsewhere in this file do
static const unsigned char	ORA_BIND_DATATYPE=1;		// SQLT_CHR
static const unsigned char	ORA_BIND_FLAG=0x07;
static const uint32_t		ORA_BIND_BUFFER_SIZE=40;
static const uint32_t		ORA_BIND_CHARSET=31;
static const size_t		ORA_BIND_DESCRIPTOR_COUNTS=8;

// a statement with one placeholder, and the value bound into it - what
// dirties a cursor before each of the two cases below reparses over it.
// "dirty" is 5 bytes so it fits the short clr form with room to spare
static const char	*ORA_BIND_DIRTY_QUERY="select :b1 as v from dual";
static const char	*ORA_BIND_DIRTY_VALUE="dirty";

// the statement reparsed afterward, on the same cursor - bindless, so any
// bind still installed on the cursor has nothing to attach to and a
// bindless execute of it is what would come back ORA-01036
static const char	*ORA_BIND_CLEAN_QUERY="select 1 as v from dual";

// TTI_OSQL7 - an oci7 client's oparse().  the field order is osql7()'s read
// order in src/protocols/oracle.cpp: a sequence byte, two counts, then
// alternating pointers and counts, then a fixed run of bytes it skips
// without reading, and last the sql text as a clr
static bool osql7(oracleprotocolclient *client, unsigned char sequence,
					uint32_t cursorid, const char *query) {

	size_t	querysize=charstring::getLength(query);

	client->beginTtiCall(ORA_TTI_OSQL7);
	client->appendByte(sequence);
	client->appendLenPreInt(1);
	client->appendLenPreInt(cursorid);
	client->appendByte((unsigned char)ORA_POINTER_SIZE);
	client->appendLenPreInt((uint32_t)querysize);
	client->appendByte((unsigned char)ORA_POINTER_SIZE);
	client->appendLenPreInt(0);
	client->appendByte((unsigned char)ORA_POINTER_SIZE);
	client->appendLenPreInt(0);

	// four pointers and three counts that are zero in every capture, and
	// that osql7() skips as a run rather than parsing
	for (size_t i=0; i<ORA_POINTER_SIZE*4+3; i++) {
		client->appendByte(0);
	}

	client->appendLenBytes(query,querysize);

	return client->sendPacket() && client->recvPacket();
}

// TTI_QUERY2 - an oci7 client's oexec(), bindless and with no descriptor
// block behind the header at all.  query2() in src/protocols/oracle.cpp
// reads three fields and skips the rest of the request, so three fields are
// all this writes
static bool query2(oracleprotocolclient *client, unsigned char sequence,
					uint32_t options, uint32_t cursorid) {

	client->beginTtiCall(ORA_TTI_QUERY2);
	client->appendByte(sequence);
	client->appendAuthCount(options,4);
	client->appendAuthCount(cursorid,4);

	return client->sendPacket() && client->recvPacket();
}

// a null pointer field inside a query2 descriptor block - getQuery2Descriptors()
// in src/protocols/oracle.cpp never looks at what one of these holds, only
// where it sits, so zero does as well as any other value
static void appendQuery2Pointer(oracleprotocolclient *client) {
	for (size_t i=0; i<ORA_POINTER_SIZE; i++) {
		client->appendByte(0);
	}
}

// TTI_QUERY2 with one bind descriptor and its value, and no define block -
// what an oci7 client's obndrv()+oexec() send to bind a single value into
// the statement osql7() already parsed and run it.  the header is query2()'s
// above; behind it comes getQuery2Descriptors()'s own shape: ten fields, a
// zero define count, one more pointer, the bind count, seven more counts -
// of which only the bind count is read back into anything used here - one
// bind descriptor, and its value behind the TTC_ROW_DATA marker every bind
// value block starts with
static bool query2Bind(oracleprotocolclient *client, unsigned char sequence,
					uint32_t options, uint32_t cursorid,
					const char *value) {

	size_t	valuesize=charstring::getLength(value);

	client->beginTtiCall(ORA_TTI_QUERY2);
	client->appendByte(sequence);
	client->appendAuthCount(options,4);
	client->appendAuthCount(cursorid,4);

	// the ten fields ahead of the define count
	appendQuery2Pointer(client);
	appendQuery2Pointer(client);
	client->appendAuthCount(0,4);
	client->appendAuthCount(0,4);
	appendQuery2Pointer(client);
	client->appendAuthCount(0,4);
	appendQuery2Pointer(client);
	client->appendAuthCount(0,4);
	appendQuery2Pointer(client);
	appendQuery2Pointer(client);

	client->appendAuthCount(0,4);			// define count: none

	// the pointer and seven counts behind it, of which the first is the
	// bind count and the rest - including the row count, unused since
	// this call doesn't fetch - stay zero
	appendQuery2Pointer(client);
	client->appendAuthCount(1,4);
	for (size_t i=0; i<7; i++) {
		client->appendAuthCount(0,4);
	}

	// the one bind descriptor
	client->appendByte(ORA_BIND_DATATYPE);
	client->appendByte(ORA_BIND_FLAG);
	client->appendByte(0);
	client->appendByte(0);
	for (size_t i=0; i<ORA_BIND_DESCRIPTOR_COUNTS; i++) {
		uint32_t	count=0;
		if (i==0) {
			count=ORA_BIND_BUFFER_SIZE;
		} else if (i==5) {
			count=ORA_BIND_CHARSET;
		}
		client->appendAuthCount(count,4);
	}

	// and its value
	client->appendByte(ORA_TTC_ROW_DATA);
	client->appendLenBytes(value,valuesize);

	return client->sendPacket() && client->recvPacket();
}

// TTI_PARSE_EXECUTE (0x27) - the pre-8.0 combined parse-and-execute
// (oparsex()).  the field order is parseExecute()'s read order in
// src/protocols/oracle.cpp: a sequence byte, the cursor id and a query
// pointer as counts, then the sql text as a clr.  there is no bind step of
// its own - it prepares and executes in the one call - which is exactly why
// a bind an earlier statement left on the cursor has nowhere else to be
// cleared from
static bool parseExecute(oracleprotocolclient *client, unsigned char sequence,
					uint32_t cursorid, const char *query) {

	size_t	querysize=charstring::getLength(query);

	client->beginTtiCall(ORA_TTI_PARSE_EXECUTE);
	client->appendByte(sequence);
	client->appendAuthCount(cursorid,4);
	client->appendByte((unsigned char)ORA_POINTER_SIZE);
	client->appendAuthCount((uint32_t)querysize,4);
	client->appendLenBytes(query,querysize);

	return client->sendPacket() && client->recvPacket();
}

// whether a bindless TTI_QUERY2 execute-only call came back clean.
// sendQuery2Response() answers success with a TTC_OK wrapper and a lead-in
// ahead of the same summary object legacySummarySucceeded() reads, so that
// decode doesn't apply here; sendQueryError() (query3session is never set in
// any of these cases, so this is always the oci7 branch) answers a failure
// with the summary object directly, the same as osql7()'s and
// parseExecute()'s own responses.  so the two shapes are told apart by
// whether the code right behind the data flags is the error object at all
static bool query2ExecuteSucceeded(oracleprotocolclient *client) {

	client->rewindResponse();

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	if (!client->readBytes(dataflags,sizeof(dataflags)) ||
		!client->readByte(&ttccode)) {
		return false;
	}

	if (ttccode==ORA_TTC_ERROR) {
		uint32_t	skip=0;
		uint32_t	errornumber=0;
		return client->readLenPreInt(&skip) &&		// end of call status
			client->readLenPreInt(&skip) &&	// rows processed
			client->readLenPreInt(&errornumber) &&
			errornumber==0;
	}

	return ttccode==ORA_TTC_OK;
}

// a parse and a bound execute of a statement of their own, on the cursor a
// case is about to reparse over - what an earlier client session leaving
// binds behind looks like, built to order rather than relied on from
// whatever the rest of the suite has already done to the pool
static bool dirtyCursorWithBind(oracleprotocolclient *client,
					const char *mode, uint32_t cursorid) {

	char	label[192];

	charstring::printf(label,sizeof(label),
				"%s: dirty the cursor with a bound execute",mode);

	bool	dirtied=osql7(client,1,cursorid,ORA_BIND_DIRTY_QUERY) &&
			query2Bind(client,2,
					ORA_OPTION_BIND|ORA_OPTION_EXECUTE|
					ORA_OPTION_NOPLSQL,
					cursorid,ORA_BIND_DIRTY_VALUE) &&
			query2ExecuteSucceeded(client);
	report(label,dirtied);
	if (!dirtied) {
		reportResponse(client);
	}
	return dirtied;
}

// the login and the cursor every case starts with, on a session small
// enough that an ordinary batch of rows can't fit one packet
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

// the batch a modern client asks for with a fetch of its own: a query3 that
// parses and executes but prefetches nothing, then a TTI_FETCH for the
// whole 20 rows.  sendFetch3Response() answers it, and the answer is five
// packets long
static void runFetch3Case(const char *mode,
				const char *host, uint16_t port,
				const char *sid,
				const char *user, const char *password) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!startSession(&client,mode,host,port,sid,user,password,
								&cursorid)) {
		return;
	}

	// a prefetch count of 0 and no OPTION_FETCH, so this answers with the
	// describe alone and leaves the whole result set for the fetch below
	charstring::printf(label,sizeof(label),"%s: parse and execute",mode);
	if (!client.query3(ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,
				cursorid,0,ORA_BATCH_QUERY)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}

	bool	executed=(client.getResponseTtcCode()==ORA_TTC_DESCRIBE_INFO);
	report(label,executed);
	if (!executed) {
		reportResponse(&client);
		client.disconnect();
		return;
	}

	charstring::printf(label,sizeof(label),"%s: fetch the batch",mode);
	if (!client.fetch(cursorid,ORA_BATCH_ROWS)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	checkBatch(&client,mode,ORA_ROW_HEADER_FLAGS_FETCH,cursorid,
					readBatchRows);

	client.disconnect();
}

// the same batch, asked for through an execute's prefetch count instead of
// through a fetch of its own - which is how a modern client gets its first
// rows, and a different responder: sendQuery3Response().
//
// one query3 call parses and executes the statement and a second executes it
// again, asking for the whole batch as its prefetch.  both calls execute -
// what keeps the describe off the second answer, so that it leads with the
// row header and nothing in front of it, is that the second one leaves
// OPTION_PARSE off: sendQuery3Response() writes a describe for OPTION_PARSE
// or OPTION_DESCRIBE and for nothing else.  OPTION_DESCRIBE stays off both
// calls, since sendQuery3Response() forces the prefetch count to 0 for a
// describe, on the grounds that a describe asks about the statement rather
// than for its data
static void runQuery3Case(const char *mode,
				const char *host, uint16_t port,
				const char *sid,
				const char *user, const char *password) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!startSession(&client,mode,host,port,sid,user,password,
								&cursorid)) {
		return;
	}

	// a prefetch count of 0 and no OPTION_FETCH, so this answers with the
	// describe alone and leaves the whole result set for the execute below
	charstring::printf(label,sizeof(label),"%s: parse and execute",mode);
	if (!client.query3(ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,
				cursorid,0,ORA_BATCH_QUERY)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}

	bool	executed=(client.getResponseTtcCode()==ORA_TTC_DESCRIBE_INFO);
	report(label,executed);
	if (!executed) {
		reportResponse(&client);
		client.disconnect();
		return;
	}

	// no query text and no parse - an execute of what the call above
	// parsed, asking for the whole batch as its prefetch
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

	charstring::printf(label,sizeof(label),
				"%s: the answer leads with the rows",mode);
	report(label,client.getResponseTtcCode()==ORA_TTC_ROW_HEADER);

	checkBatch(&client,mode,ORA_ROW_HEADER_FLAGS_EXECUTE,cursorid,
					readBatchRows);

	client.disconnect();
}

// the same batch again, this time through the pre-query3 calls: a legacy
// TTI_QUERY parse, a bindless TTI_EXECUTE and a TTI_FETCH for the whole 20
// rows.  sendFetchResponse() answers the fetch - the same row header
// putRowHeader() builds for the other two cases, behind a row body of its
// own that readLegacyBatchRows() decodes instead of readBatchRows().
//
// this is also the regression case for #10063: open() hands out a pooled
// cursor without clearing it, and a bindless legacy execute() never clears
// one either, so it silently re-applies whatever input binds an earlier
// client session left behind on that cursor.  the two cases above bind
// nothing themselves, but running third puts this one behind whatever the
// rest of the suite has already done to this connection pool by the time
// it gets here - which is what originally caught the bug and is worth
// keeping this case last for
static void runLegacyCase(const char *mode,
				const char *host, uint16_t port,
				const char *sid,
				const char *user, const char *password) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!startSession(&client,mode,host,port,sid,user,password,
								&cursorid)) {
		return;
	}

	charstring::printf(label,sizeof(label),"%s: parse",mode);
	if (!client.legacyQuery(cursorid,ORA_BATCH_QUERY)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,legacySummarySucceeded(&client));

	// no binds of its own - the shape #10063 needs, since a bindless
	// execute is the one that never clears the cursor's params
	charstring::printf(label,sizeof(label),"%s: execute",mode);
	if (!client.legacyExecute(cursorid,1,0)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	bool	executed=legacySummarySucceeded(&client);
	report(label,executed);
	if (!executed) {
		reportResponse(&client);
		client.disconnect();
		return;
	}

	charstring::printf(label,sizeof(label),"%s: fetch the batch",mode);
	if (!client.legacyFetch(cursorid,ORA_BATCH_ROWS)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	checkBatch(&client,mode,ORA_ROW_HEADER_FLAGS_FETCH,cursorid,
					readLegacyBatchRows);

	client.disconnect();
}

// #10080: a TTI_OSQL7 reparse of a bindless statement, on a cursor this
// case has just bound and executed a different one on, followed by a
// bindless TTI_QUERY2 execute of what was reparsed.  before #10080, osql7()
// left the cursor's binds in place across the reparse, so the execute here
// re-applied ":b1" to a statement with no placeholder of its own and failed
// ORA-01036
static void runOsql7ClearsBindsCase(const char *mode,
					const char *host, uint16_t port,
					const char *sid,
					const char *user, const char *password) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!startSession(&client,mode,host,port,sid,user,password,
								&cursorid)) {
		return;
	}

	if (!dirtyCursorWithBind(&client,mode,cursorid)) {
		client.disconnect();
		return;
	}

	charstring::printf(label,sizeof(label),
				"%s: reparse a bindless statement",mode);
	if (!osql7(&client,3,cursorid,ORA_BIND_CLEAN_QUERY)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,legacySummarySucceeded(&client));

	charstring::printf(label,sizeof(label),
				"%s: the bindless execute doesn't see the "
				"earlier bind",mode);
	if (!query2(&client,4,ORA_OPTION_EXECUTE|ORA_OPTION_NOPLSQL,
								cursorid)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	bool	succeeded=query2ExecuteSucceeded(&client);
	report(label,succeeded);
	if (!succeeded) {
		reportResponse(&client);
	}

	client.disconnect();
}

// #10080: a TTI_PARSE_EXECUTE of a bindless statement, on a cursor this case
// has just bound and executed a different one on.  parseExecute() has no
// bind step of its own - it prepares and executes together - so before
// #10080 a bind left on the cursor rode straight into it and failed
// ORA-01036 the same way the osql7 case above's follow-on execute did
static void runParseExecuteClearsBindsCase(const char *mode,
					const char *host, uint16_t port,
					const char *sid,
					const char *user, const char *password) {

	char	label[192];

	stdoutput.printf("\n--- %s ---\n\n",mode);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!startSession(&client,mode,host,port,sid,user,password,
								&cursorid)) {
		return;
	}

	if (!dirtyCursorWithBind(&client,mode,cursorid)) {
		client.disconnect();
		return;
	}

	charstring::printf(label,sizeof(label),
				"%s: parse-execute a bindless statement "
				"doesn't see the earlier bind",mode);
	if (!parseExecute(&client,3,cursorid,ORA_BIND_CLEAN_QUERY)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	bool	succeeded=legacySummarySucceeded(&client);
	report(label,succeeded);
	if (!succeeded) {
		reportResponse(&client);
	}

	client.disconnect();
}

// the port a listener ended up on.  the default is what
// test/sqlrelay.conf.d/oracleprotocol.conf.in's @ORACLEPROTOCOLPORT1@ token
// defaults to; test/test.sh exports the real one, the same way
// oraclesplitquery3 reads it
static uint16_t portFromEnvironment(const char *name, uint16_t fallback) {
	const char	*value=environment::getValue(name);
	if (charstring::isNullOrEmpty(value)) {
		return fallback;
	}
	return (uint16_t)charstring::convertToInteger(value);
}

int main(int argc, char **argv) {

	stdoutput.printf("\n====== #10056 multi-packet row batch, "
						"#10080 stale binds ======\n\n");

	// the oracleprotocol test instance - see
	// test/sqlrelay.conf.d/oracleprotocol.conf.  it isn't a real oracle
	// server, it's a listener speaking oracle's wire protocol, so 1521 is
	// just the port it was configured with
	const char	*host="127.0.0.1";
	const char	*sid="ora1";
	const char	*user="testuser";
	const char	*password="testpassword";

	uint16_t	port=portFromEnvironment("ORACLEPROTOCOLPORT1",1521);

	// the batch a fetch of its own asks for
	runFetch3Case("fetch3 batch",host,port,sid,user,password);

	// the batch an execute's prefetch count asks for
	runQuery3Case("query3 prefetch batch",host,port,sid,user,password);

	// the same batch through the legacy pre-query3 calls, run last so the
	// pool is already whatever the rest of the suite has left it as
	runLegacyCase("legacy fetch batch",host,port,sid,user,password);

	// #10080: osql7() and parseExecute() clearing binds an earlier
	// statement on the same cursor left behind, each dirtying its own
	// cursor rather than relying on the case above's pool state
	runOsql7ClearsBindsCase("osql7 clears binds",host,port,sid,
							user,password);
	runParseExecuteClearsBindsCase("parse-execute clears binds",host,port,
							sid,user,password);

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
