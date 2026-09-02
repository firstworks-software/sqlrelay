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
// This test is what keeps both covered: it runs the same query twice over
// oracleprotocolclient, once with the client's bit clear and once with it
// set, and asserts the exact bytes each framing has to produce.
//
// Four branch sites are exercised, two per direction:
//
//	- getQuery3Request() reads the query text, which is over 252 bytes
//	  and so arrives chunked
//	- putLenBytes() writes the column value, which is over 252 bytes and
//	  so goes back chunked
//
// The query carries its own answer - "select '<600 bytes>' from dual" - so
// the value that comes back is the query text that went out.  One exact
// round trip therefore proves both directions at once: a chunk boundary read
// or written at the wrong place cannot survive it.

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

int	status=0;
const char	*success="\033[32msuccess\033[0m";
const char	*failure="\033[31mfailure\033[0m";

static void report(const char *label, bool ok) {
	stdoutput.printf("%s: %s\n",label,(ok)?success:failure);
	if (!ok) {
		status=1;
	}
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

// the exact bytes putLenBytes() has to write for a value past 252 bytes,
// built here rather than read back off the response - so what this test
// asserts is the framing itself, and not just the value that survived it.
// returns the size written, or 0 if it wouldn't fit
static size_t buildLongFormClr(const char *value, size_t valuesize,
					bool bigchunk,
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

	// the closing empty chunk - one zero byte either way, since a ub4
	// zero is a count byte of 0 and nothing after it
	if (outsize>=outmax) {
		return 0;
	}
	out[outsize++]=0;

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

// one whole session, from the connect to the disconnect, with the client's
// CCAP_TTC3_BIG_CHUNK_CLR bit whichever way "bigchunk" says.  the bit goes
// out in the data type negotiation, so each framing needs a session of its
// own
static void runOnce(const char *host, uint16_t port, const char *sid,
			const char *user, const char *password,
			const char *value, size_t valuesize,
			const char *query, bool bigchunk) {

	const char	*mode=(bigchunk)?"big chunk":"raw byte";
	char		label[128];

	stdoutput.printf("\n--- %s framing ---\n\n",mode);

	oracleprotocolclient	client;
	client.setBigChunkClr(bigchunk);

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
							bigchunk,expected,
							sizeof(expected));
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
			value,ORA_CLR_TEST_VALUE_SIZE,query,false);

	// and the framing every real client asks for now
	runOnce(host,port,sid,user,password,
			value,ORA_CLR_TEST_VALUE_SIZE,query,true);

	delete[] query;

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
