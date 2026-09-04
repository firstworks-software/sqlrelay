// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Coverage for the clr long form's two chunk framings in
// src/protocols/oracle.cpp.
//
// A clr past 252 bytes goes out chunked: a 0xfe marker, a run of chunks, and
// an empty chunk to close it.  How a chunk's length is written is negotiated
// per session, in recvDataTypeRequest():
//
//	- raw byte framing - one length byte per chunk, so a chunk is at
//	  most 255 bytes.  this is what the module always did
//	- big chunk framing - the length as a count prefixed ub4, so a chunk
//	  is at most 32767 bytes.  taken only when the tti version is 6 and
//	  both ends offer CCAP_TTC3 bit 0x20, CCAP_TTC3_BIG_CHUNK_CLR
//
// The module now offers the bit, and so does every real client - OCI, ojdbc,
// python-oracledb, node-oracledb and go-ora - which puts all of them on the
// big chunk framing and leaves the raw byte framing with no client at all.
// This test is what keeps both covered: it runs the same work three times
// over oracleprotocolclient - once with the client's bit clear, once with it
// set, and once with it set over a tti version 5 session, where the module's
// gate ignores it - and asserts the exact bytes each framing has to produce.
//
// Three branch sites are exercised, one reader and two writers, and every
// run takes all three:
//
//	- getQuery3Request() reads the query text, which is over 252 bytes
//	  and so arrives chunked
//	- putLenBytes() writes a varchar column value, which is over 252
//	  bytes and so goes back chunked
//	- putLongBytes() writes a LONG column value, which goes back in the
//	  long form whatever its size, and closes it with three zero bytes
//	  rather than one
//
// The first two are covered by a query that carries its own answer -
// "select '<600 bytes>' from dual" - so the value that comes back is the
// query text that went out.  One exact round trip therefore proves both
// directions at once: a chunk boundary read or written at the wrong place
// cannot survive it.
//
// putLongBytes() needs a real LONG column to reach, so each run creates a
// table with one LONG column, inserts that same value into it, selects it
// back and drops it again.  Its raw byte branch is the one nothing else
// reaches: OCI and every thin driver negotiate the bit on, so no client of
// any kind arrives at it, and no other test does either.

// the value the query carries and hands back.  600 bytes is well past the
// short form's 252, and past 255 twice over: with raw byte lengths it takes
// three chunks (255, 255 and 90), with count prefixed ub4s it takes one.  so
// one size covers the multi-chunk case and the single-chunk case, one per
// framing
static const size_t	ORA_CLR_TEST_VALUE_SIZE=600;

// room for the value, the marker, a length per chunk and the closing zero -
// far more than any framing needs
static const size_t	ORA_CLR_TEST_BUFFER_SIZE=1024;

// putRowHeader()'s flags byte in the answer to a fetch
static const unsigned char	ORA_ROW_HEADER_FLAGS_FETCH=0x02;

// how many zero bytes close the long form.  putLenBytes() writes the one
// empty chunk that ends the run of chunks; putLongBytes() writes two more
// behind it, which is what a LONG or a LONG RAW column's value carries and
// nothing else does
static const size_t	ORA_CLR_TRAILING_ZEROS=1;
static const size_t	ORA_LONG_TRAILING_ZEROS=3;

// the table the LONG case creates, fills, selects back and drops.  it is
// this test's own: the oracleprotocol tests run against the same schema as
// the rest of the test suite, and the LONG table already in there -
// protocoltestlong - belongs to test/protocol/oracle/oci8.cpp, which
// creates and drops it around its own assertions
static const char	*ORA_LONG_TABLE="oracleclrchunkslong";

// the one column the table has, which is the LONG the value goes into
static const char	*ORA_LONG_COLUMN="testlong";

int	status=0;
const char	*success="\033[32msuccess\033[0m";
const char	*failure="\033[31mfailure\033[0m";

static void report(const char *label, bool ok) {
	stdoutput.printf("%s: %s\n",label,(ok)?success:failure);
	if (!ok) {
		status=1;
	}
}

// what the listener said went wrong, for a call that answered with an oracle
// error rather than with what it was asked for
static void reportResponseError(oracleprotocolclient *client) {
	stdoutput.printf("the listener answered with an error:\n");
	stdoutput.safePrint(client->getResponse(),
				(int32_t)client->getResponseSize());
	stdoutput.printf("\n");
}

// the pattern is 36 bytes long, and 36 divides neither 255 nor 600, so a
// chunk boundary read or written at the wrong place shows up as shifted text
// rather than as plausible bytes
static void buildTestValue(char *value, size_t valuesize) {
	static const char	*pattern=
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	size_t	patternsize=charstring::getLength(pattern);
	for (size_t i=0; i<valuesize; i++) {
		value[i]=pattern[i%patternsize];
	}
	value[valuesize]='\0';
}

// the exact bytes the long form has to carry for a value the module writes
// chunked, built here rather than read back off the response - so what this
// test asserts is the framing itself, and not just the value that survived
// it.  putLenBytes() and putLongBytes() write the same marker, the same
// chunks and the same lengths, and differ only in how many zero bytes close
// the run, which is what "trailingzeros" says - so both are built from this
// one function and cannot drift apart.  returns the size written, or 0 if
// it wouldn't fit
static size_t buildLongFormClr(const char *value, size_t valuesize,
					bool bigchunk, size_t trailingzeros,
					unsigned char *out, size_t outmax) {

	size_t	maxchunk=(bigchunk)?
			ORA_CLR_MAX_BIG_CHUNK_SIZE:ORA_CLR_MAX_CHUNK_SIZE;
	size_t	outsize=0;

	if (outsize>=outmax) {
		return 0;
	}
	out[outsize++]=ORA_CLR_LONG_FORM_MARKER;

	size_t	offset=0;
	while (offset<valuesize) {

		size_t	chunk=valuesize-offset;
		if (chunk>maxchunk) {
			chunk=maxchunk;
		}

		// a count byte and that many big-endian bytes, or one raw
		// byte - see appendLenPreInt() in oracleprotocolclient.cpp
		// for the count prefixed form
		if (bigchunk) {
			if (outsize+3>outmax) {
				return 0;
			}
			if (chunk<=0xff) {
				out[outsize++]=1;
				out[outsize++]=(unsigned char)chunk;
			} else {
				out[outsize++]=2;
				out[outsize++]=(unsigned char)
							((chunk>>8)&0xff);
				out[outsize++]=(unsigned char)(chunk&0xff);
			}
		} else {
			if (outsize+1>outmax) {
				return 0;
			}
			out[outsize++]=(unsigned char)chunk;
		}

		if (outsize+chunk>outmax) {
			return 0;
		}
		bytestring::copy(out+outsize,value+offset,chunk);
		outsize+=chunk;
		offset+=chunk;
	}

	// the closing empty chunk - one zero byte in either framing, since a
	// ub4 zero is a count byte of 0 and nothing after it - and, for a
	// value putLongBytes() wrote, the two zero bytes it puts behind it
	if (outsize+trailingzeros>outmax) {
		return 0;
	}
	for (size_t i=0; i<trailingzeros; i++) {
		out[outsize++]=0;
	}

	return outsize;
}

// a substring search over raw bytes, which responseContains() can't do - the
// framing this looks for isn't text
static bool responseContainsBytes(oracleprotocolclient *client,
					const unsigned char *bytes,
					size_t size) {
	const unsigned char	*response=client->getResponse();
	size_t			responsesize=client->getResponseSize();
	if (!size || responsesize<size) {
		return false;
	}
	for (size_t i=0; i<=responsesize-size; i++) {
		if (!bytestring::compare(response+i,bytes,size)) {
			return true;
		}
	}
	return false;
}

// walk a fetch response - the data flags, a row header, one row data message
// carrying the single column, and then the summary object, which this stops
// short of.  see sendFetch3Response(), putRowHeader() and putRowData() in
// src/protocols/oracle.cpp
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

// a ddl or dml statement that has to work.  a statement with no result set
// answers with the return parameters putReturnParameters() writes, whose
// first byte is TTC_OK - anything else is either an oracle error or a
// response shape this doesn't understand, and both are failures here
static bool runOkStatement(oracleprotocolclient *client, const char *mode,
				const char *what, uint32_t cursorid,
				const char *query, bool commit) {

	char	label[128];
	charstring::printf(label,sizeof(label),"%s: long: %s",mode,what);

	uint32_t	options=ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL;
	if (commit) {
		options|=ORA_OPTION_COMMIT;
	}

	if (!client->query3(options,cursorid,0,query)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}

	bool	ok=(client->getResponseTtcCode()==ORA_TTC_OK);
	report(label,ok);
	if (!ok) {
		reportResponseError(client);
	}
	return ok;
}

// the LONG column half of a session.  the varchar the select above hands
// back goes out through putLenBytes(); a LONG column goes out through
// putLongBytes() instead, which is the same two framings, but always in the
// long form - there is no short form branch there at all - and closed with
// three zero bytes rather than one.
//
// a LONG column is the only way in: putLongBytes() is reached from a LONG,
// from a LONG RAW, and from nothing else.  the table is created, filled,
// selected back and dropped here, once per framing, so a run that died
// before its drop leaves nothing behind that the next one can't clean up
static void runLongCase(oracleprotocolclient *client, const char *mode,
				const char *value, size_t valuesize,
				bool bigchunk) {

	char	label[128];
	char	*query=NULL;

	stdoutput.printf("\n-- %s framing, a LONG column, "
				"through putLongBytes() --\n\n",mode);

	// one cursor for the ddl and the insert, and a second for the select
	// below - the setup statements are not what this is testing, and a
	// client with a result set open doesn't reparse over it
	uint32_t	setupcursorid=0;
	charstring::printf(label,sizeof(label),
				"%s: long: open the setup cursor",mode);
	if (!client->open(&setupcursorid)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return;
	}
	report(label,true);

	// a table a previous run died holding.  the answer is deliberately
	// not looked at: on a schema that hasn't seen this test the table
	// isn't there, and ORA-00942 is the right answer rather than a
	// failure
	charstring::printf(&query,"drop table %s",ORA_LONG_TABLE);
	client->query3(ORA_OPTION_PARSE|
			ORA_OPTION_EXECUTE|
			ORA_OPTION_NOPLSQL,
			setupcursorid,0,query);
	delete[] query;
	query=NULL;

	charstring::printf(&query,"create table %s (%s long)",
					ORA_LONG_TABLE,ORA_LONG_COLUMN);
	bool	ok=runOkStatement(client,mode,"create the table",
					setupcursorid,query,false);
	delete[] query;
	query=NULL;
	if (!ok) {
		return;
	}

	// the same value the varchar case sends, so both framings are
	// asserted over the same bytes.  the insert commits, since the
	// select below runs on a cursor of its own
	charstring::printf(&query,"insert into %s values ('%s')",
						ORA_LONG_TABLE,value);
	ok=runOkStatement(client,mode,"insert the value",
					setupcursorid,query,true);
	delete[] query;
	query=NULL;
	if (!ok) {
		return;
	}

	uint32_t	selectcursorid=0;
	charstring::printf(label,sizeof(label),
				"%s: long: open the select cursor",mode);
	if (!client->open(&selectcursorid)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return;
	}
	report(label,true);

	// query3() sends a max long size of 0, and sendQuery3Response() sends
	// no rows at all for a result set with a long column until the client
	// says how much of one it has room for - so this answers with the
	// describe alone and the row comes back on its own in answer to the
	// fetch below, exactly as it does for the varchar case
	charstring::printf(&query,"select %s from %s",
					ORA_LONG_COLUMN,ORA_LONG_TABLE);
	charstring::printf(label,sizeof(label),"%s: long: execute",mode);
	bool	sent=client->query3(ORA_OPTION_PARSE|
					ORA_OPTION_EXECUTE|
					ORA_OPTION_NOPLSQL,
					selectcursorid,0,query);
	delete[] query;
	query=NULL;
	if (!sent) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return;
	}
	report(label,client->getResponseTtcCode()==ORA_TTC_DESCRIBE_INFO);
	if (client->getResponseTtcCode()==ORA_TTC_ERROR) {
		reportResponseError(client);
		return;
	}

	charstring::printf(label,sizeof(label),"%s: long: fetch",mode);
	if (!client->fetch(selectcursorid,1)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return;
	}
	report(label,true);

	// the framing assertion, built from the value that was inserted and
	// closed with putLongBytes()' three zero bytes rather than
	// putLenBytes()' one
	unsigned char	expected[ORA_CLR_TEST_BUFFER_SIZE];
	size_t		expectedsize=buildLongFormClr(value,valuesize,
						bigchunk,
						ORA_LONG_TRAILING_ZEROS,
						expected,sizeof(expected));
	size_t	maxchunk=(bigchunk)?
			ORA_CLR_MAX_BIG_CHUNK_SIZE:ORA_CLR_MAX_CHUNK_SIZE;
	size_t	chunks=(valuesize+maxchunk-1)/maxchunk;
	charstring::printf(label,sizeof(label),
			"%s: long: the value goes out as %d chunk(s)",
			mode,(int)chunks);
	report(label,expectedsize>0 &&
			responseContainsBytes(client,expected,expectedsize));

	// and the round trip, walked field by field rather than searched for
	uint32_t	colcount=0;
	unsigned char	actual[ORA_CLR_TEST_BUFFER_SIZE];
	size_t		actualsize=0;
	bool		isnull=false;
	bool		decoded=readFetch3Row(client,&colcount,
						actual,sizeof(actual),
						&actualsize,&isnull);
	charstring::printf(label,sizeof(label),
				"%s: long: fetch response decodes",mode);
	report(label,decoded);
	if (!decoded) {
		stdoutput.printf("response (%d bytes):\n",
					(int)client->getResponseSize());
		stdoutput.safePrint(client->getResponse(),
					(int32_t)client->getResponseSize());
		stdoutput.printf("\n");
		return;
	}

	charstring::printf(label,sizeof(label),
				"%s: long: one column, not null",mode);
	report(label,colcount==1 && !isnull);

	charstring::printf(label,sizeof(label),
			"%s: long: the value round trips exactly",mode);
	int64_t	difference=firstDifference(actual,actualsize,
						value,valuesize);
	report(label,difference<0);
	if (difference>=0) {
		stdoutput.printf("  %d bytes back, %d sent\n",
					(int)actualsize,(int)valuesize);
		stdoutput.printf("  first difference at byte %d\n",
					(int)difference);
	}

	// and put the schema back the way it was found.  the table is there
	// this time, so this one has to work
	charstring::printf(&query,"drop table %s",ORA_LONG_TABLE);
	runOkStatement(client,mode,"drop the table",setupcursorid,query,true);
	delete[] query;
}

// one whole session, from the connect to the disconnect, with the client
// offering its CCAP_TTC3_BIG_CHUNK_CLR bit whichever way "offerbigchunk"
// says over a tti version "ttiversion" session, and with "bigchunk" saying
// which framing that combination has to come back as.  the bit goes out in
// the data type negotiation and the version in the protocol negotiation, so
// each combination needs a session of its own
static void runOnce(const char *host, uint16_t port, const char *sid,
			const char *user, const char *password,
			const char *value, size_t valuesize,
			const char *query, const char *mode,
			unsigned char ttiversion, bool offerbigchunk,
			bool bigchunk) {

	char		label[128];

	stdoutput.printf("\n--- %s framing ---\n\n",mode);

	oracleprotocolclient	client;
	client.setTtiVersion(ttiversion);
	client.setBigChunkClr(offerbigchunk);

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

	uint32_t	cursorid=0;
	charstring::printf(label,sizeof(label),"%s: open cursor",mode);
	if (!client.open(&cursorid)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		return;
	}
	report(label,true);

	stdoutput.printf("\n-- %s framing, a varchar value, "
				"through putLenBytes() --\n\n",mode);

	// parse and execute, asking for no rows up front: with a prefetch of
	// 0 and no OPTION_FETCH, sendQuery3Response() answers with the
	// describe alone, and the row comes back on its own in answer to the
	// fetch below - which is the plainest response shape there is to
	// walk.  the query text is over 252 bytes, so this is what
	// getQuery3Request()'s chunked branch reads
	charstring::printf(label,sizeof(label),"%s: execute",mode);
	if (!client.query3(ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,
				cursorid,0,query)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,client.getResponseTtcCode()==ORA_TTC_DESCRIBE_INFO);

	// a query the listener couldn't reassemble would come back as an
	// oracle error rather than as a describe, so say so plainly
	if (client.getResponseTtcCode()==ORA_TTC_ERROR) {
		stdoutput.printf("the listener answered the query with an "
					"error:\n");
		stdoutput.safePrint(client.getResponse(),
					(int32_t)client.getResponseSize());
		stdoutput.printf("\n");
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

	// the framing assertion.  the whole long form is built here from the
	// value that was sent, and the response has to carry it verbatim
	unsigned char	expected[ORA_CLR_TEST_BUFFER_SIZE];
	size_t		expectedsize=buildLongFormClr(value,valuesize,
						bigchunk,
						ORA_CLR_TRAILING_ZEROS,
						expected,sizeof(expected));
	size_t	maxchunk=(bigchunk)?
			ORA_CLR_MAX_BIG_CHUNK_SIZE:ORA_CLR_MAX_CHUNK_SIZE;
	size_t	chunks=(valuesize+maxchunk-1)/maxchunk;
	charstring::printf(label,sizeof(label),
			"%s: the value goes out as %d chunk(s)",
			mode,(int)chunks);
	report(label,expectedsize>0 &&
			responseContainsBytes(&client,expected,expectedsize));

	// and the round trip, walked field by field rather than searched for
	uint32_t	colcount=0;
	unsigned char	actual[ORA_CLR_TEST_BUFFER_SIZE];
	size_t		actualsize=0;
	bool		isnull=false;
	bool		decoded=readFetch3Row(&client,&colcount,
						actual,sizeof(actual),
						&actualsize,&isnull);
	charstring::printf(label,sizeof(label),
				"%s: fetch response decodes",mode);
	report(label,decoded);
	if (!decoded) {
		stdoutput.printf("response (%d bytes):\n",
					(int)client.getResponseSize());
		stdoutput.safePrint(client.getResponse(),
					(int32_t)client.getResponseSize());
		stdoutput.printf("\n");
		client.disconnect();
		return;
	}

	charstring::printf(label,sizeof(label),
				"%s: one column, not null",mode);
	report(label,colcount==1 && !isnull);

	charstring::printf(label,sizeof(label),
				"%s: the value round trips exactly",mode);
	int64_t	difference=firstDifference(actual,actualsize,
						value,valuesize);
	report(label,difference<0);
	if (difference>=0) {
		stdoutput.printf("  %d bytes back, %d sent\n",
					(int)actualsize,(int)valuesize);
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
		if (from+count>valuesize) {
			count=valuesize-from;
		}
		stdoutput.printf("  sent: ");
		stdoutput.safePrint(value+from,(int32_t)count);
		stdoutput.printf("\n");
	}

	// and the same framing again, through the other writer
	runLongCase(&client,mode,value,valuesize,bigchunk);

	client.disconnect();
}

int main(int argc, char **argv) {

	stdoutput.printf("\n====== clr long form chunk framing ======\n\n");

	// the oracleprotocol test instance - see
	// test/sqlrelay.conf.d/oracleprotocol.conf.  it isn't a real oracle
	// server, it's a listener speaking oracle's wire protocol, so 1521
	// is just the port it was configured with rather than anything a
	// real database is on.  ORACLEPROTOCOLPORT1 names the port it
	// actually ended up on, the same way oracledescribeonly reads it
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

	char	value[ORA_CLR_TEST_VALUE_SIZE+1];
	buildTestValue(value,ORA_CLR_TEST_VALUE_SIZE);

	char	*query=NULL;
	charstring::printf(&query,"select '%s' from dual",value);

	stdoutput.printf("query text: %d bytes\n",
				(int)charstring::getLength(query));
	stdoutput.printf("column value: %d bytes\n",
				(int)ORA_CLR_TEST_VALUE_SIZE);

	// the framing every test here used before the module offered the
	// bit, and the one no real client asks for any more
	runOnce(host,port,sid,user,password,
			value,ORA_CLR_TEST_VALUE_SIZE,query,"raw byte",
			ORA_TTI_VERSION_6,false,false);

	// and the framing every real client asks for now
	runOnce(host,port,sid,user,password,
			value,ORA_CLR_TEST_VALUE_SIZE,query,"big chunk",
			ORA_TTI_VERSION_6,true,true);

	// the tti version leg of the gate, on its own.  recvDataTypeRequest()
	// only honors the bit at tti version 6 and above:
	//
	//	bigchunkclr=(ttiversion>=6 && ... )
	//
	// so a version 5 client that offers the bit - which this run is, and
	// which nothing else here or anywhere else is - still has to be
	// answered in raw bytes.  that is the false leg of the version test,
	// and no other run reaches it: the two above leave the version at 6
	// and only move the bit
	runOnce(host,port,sid,user,password,
			value,ORA_CLR_TEST_VALUE_SIZE,query,"tti 5 raw byte",
			ORA_TTI_VERSION_5,true,false);

	delete[] query;

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
