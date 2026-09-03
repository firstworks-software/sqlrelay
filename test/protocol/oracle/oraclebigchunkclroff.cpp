// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Coverage for the "bigchunkclr" listener attribute's "off" state in
// src/protocols/oracle.cpp - #9633.
//
// A clr past 252 bytes goes out chunked, and how a chunk's length is written
// is negotiated per session in recvDataTypeRequest(): a raw byte, capped at
// 255, or a count prefixed ub4, capped at 32767.  The module offers
// CCAP_TTC3 bit 0x20, CCAP_TTC3_BIG_CHUNK_CLR, and takes big chunks for a
// tti 6 client that offers the bit back.
//
// go-ora v1 breaks that.  It advertises the bit - byte 37 is 0xb3 there -
// and then frames raw bytes anyway, having never implemented big chunks at
// all.  Nothing on the wire separates it from v2, which advertises the same
// bit and does implement them, so no gate could serve both and the
// deployment has to say instead.  bigchunkclr="off" is how it says so, and
// it does two things at once:
//
//	- advertiseBigChunkClr() returns false, so putTti6Response() clears
//	  CCAP_TTC3 bit 0x20 out of the array the module advertises.  this
//	  half is for ojdbc, which takes big chunks from the server's bit
//	  alone without regard to its own - framing raw bytes while still
//	  advertising the bit would break ojdbc instead of fixing go-ora v1
//	- the per-session flag is forced false, so every long clr is framed
//	  raw in both directions
//
// No real client can cover this.  Reaching it needs a client that offers the
// bit and frames raw, and go-ora v1 is the only one there has ever been - OCI,
// ojdbc, python-oracledb, node-oracledb and go-ora v2/v3 all frame big when
// they offer the bit, so pointing any of them at the "off" listener exercises
// the ordinary agreement rather than the disagreement.  oracleprotocolclient's
// setBigChunkClr() and setBigChunkClrFraming() split the two apart, which is
// what lets this test be go-ora v1.
//
// Two legs, and the second is what makes the first mean anything:
//
//	- the "off" listener, ORACLEPROTOCOLPORT6.  the advertised bit has to
//	  be clear, and a raw framed bind over 252 bytes has to be understood
//	  and answered in raw framing
//	- the default listener, ORACLEPROTOCOLPORT1, with the same query and
//	  the same client but the big chunk framing.  the advertised bit has
//	  to be set and the answer has to come back in big chunk framing -
//	  so a failure on the first leg is the attribute, and not the wiring
//
// One statement covers both directions.  "select :b from dual" with a 600
// byte varchar bind sends a clr over 252 bytes in the request, which
// getQuery3Request() has to read, and dual hands it straight back as a
// column value over 252 bytes, which putLenBytes() has to write.  A chunk
// boundary read or written the wrong way at either end cannot survive it: a
// mis-framed bind fails the query outright with "malformed clr: bad chunk
// length", and a mis-framed answer fails the byte-for-byte comparison.

// the value the bind carries and the query hands back.  600 bytes is well
// past the short form's 252, and past 255 twice over: raw framed it takes
// three chunks (255, 255 and 90), big chunk framed it takes one
static const size_t	ORA_CLR_TEST_VALUE_SIZE=600;

// room for the value, the marker, a length per chunk and the closing zero -
// far more than either framing needs
static const size_t	ORA_CLR_TEST_BUFFER_SIZE=1024;

// how wide the bind is declared.  installQuery3Binds() sizes the buffer from
// this, so it has to be past the value it carries
static const uint32_t	ORA_BIND_BUFFER_SIZE=1024;

// putRowHeader()'s flags byte in the answer to a fetch
static const unsigned char	ORA_ROW_HEADER_FLAGS_FETCH=0x02;

// how many zero bytes close the long form putLenBytes() writes: the one
// empty chunk that ends the run of chunks.  putLongBytes() writes two more
// behind it, and nothing here goes through putLongBytes()
static const size_t	ORA_CLR_TRAILING_ZEROS=1;

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
// error rather than with what it was asked for.  a bind the module couldn't
// reassemble is the failure this test is watching for, and it lands here
static void reportResponseError(oracleprotocolclient *client) {
	stdoutput.printf("the listener answered with an error:\n");
	stdoutput.safePrint(client->getResponse(),
				(int32_t)client->getResponseSize());
	stdoutput.printf("\n");
}

// the pattern is 36 bytes long, and 36 divides neither 255 nor 600, so a
// chunk boundary read or written at the wrong place shows up as shifted text
// rather than as plausible bytes.  the same pattern oracleclrchunks.cpp uses
static void buildTestValue(char *value, size_t valuesize) {
	static const char	*pattern=
				"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	size_t	patternsize=charstring::getLength(pattern);
	for (size_t i=0; i<valuesize; i++) {
		value[i]=pattern[i%patternsize];
	}
	value[valuesize]='\0';
}

// the exact bytes the long form has to carry for a value the module wrote
// chunked, built here rather than read back off the response - so what this
// asserts is the framing itself, and not just the value that survived it.
// the same builder oracleclrchunks.cpp carries, "trailingzeros" and all, so
// the two tests can't drift apart on what a framing looks like.  returns the
// size written, or 0 if it wouldn't fit
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
	// ub4 zero is a count byte of 0 and nothing after it
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

// one whole session against one listener, as go-ora v1: the bit offered
// either way, and the framing said outright rather than worked out from it.
//
// "framebigchunk" is the framing the client reads and writes, which is the
// framing the listener under test has to be using, and "expectcapbit" is
// whether that listener has to be advertising CCAP_TTC3 bit 0x20 as well.
// the "off" listener says false to both and the default listener says true
// to both - and the point of running the two is that the query is otherwise
// identical, so the attribute is the only thing that differs
static void runLeg(const char *mode, const char *host, uint16_t port,
			const char *sid, const char *user, const char *password,
			const char *query,
			const char *value, size_t valuesize,
			bool framebigchunk, bool expectcapbit) {

	char	label[128];

	stdoutput.printf("\n--- %s, port %d ---\n\n",mode,(int)port);

	oracleprotocolclient	client;

	// what go-ora v1 does, and what nothing else does: offer the bit,
	// then frame raw anyway.  the second call is the whole reason this
	// test can exist - without it the framing follows the bit
	client.setBigChunkClr(true);
	client.setBigChunkClrFraming(framebigchunk);

	charstring::printf(label,sizeof(label),"%s: connect",mode);
	if (!client.connect(host,port,sid)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		return;
	}
	report(label,true);

	// the ojdbc half of the attribute, and the half that would regress
	// silently: ojdbc takes big chunks from the server's bit alone, so
	// the "off" listener has to stop advertising it rather than just
	// stop using it
	const unsigned char	*caps=client.getServerCompileCaps();
	size_t			capssize=client.getServerCompileCapsSize();
	charstring::printf(label,sizeof(label),
			"%s: the listener advertised a compile "
			"capability array",mode);
	if (capssize<=ORA_CCAP_TTC3) {
		report(label,false);
		stdoutput.printf("  %d bytes, need more than %d\n",
					(int)capssize,(int)ORA_CCAP_TTC3);
		client.disconnect();
		return;
	}
	report(label,true);

	bool	capbit=((caps[ORA_CCAP_TTC3]&ORA_CCAP_TTC3_BIG_CHUNK_CLR)!=0);
	charstring::printf(label,sizeof(label),
			"%s: CCAP_TTC3 bit 0x20 is %s",
			mode,(expectcapbit)?"set":"clear");
	report(label,capbit==expectcapbit);
	if (capbit!=expectcapbit) {
		stdoutput.printf("  byte %d is 0x%02x\n",
					(int)ORA_CCAP_TTC3,
					(int)caps[ORA_CCAP_TTC3]);
	}

	charstring::printf(label,sizeof(label),"%s: login",mode);
	if (!client.login(user,password)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	uint32_t	cursorid=0;
	charstring::printf(label,sizeof(label),"%s: open cursor",mode);
	if (!client.open(&cursorid)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	// the bind is over 252 bytes, so it goes out chunked and
	// getQuery3Request() has to read it in this leg's framing.  dual
	// hands it straight back, so the column value is over 252 bytes too
	// and comes back chunked through putLenBytes() - one statement, both
	// directions
	oracleprotocolbind	bind;
	bind.varchar(ORA_BIND_BUFFER_SIZE);

	oracleprotocolbindvalue	values[1];
	values[0].set(value,valuesize);

	// parse and execute, asking for no rows up front: with a prefetch of
	// 0 and no OPTION_FETCH, sendQuery3Response() answers with the
	// describe alone, and the row comes back on its own in answer to the
	// fetch below - which is the plainest response shape there is to walk
	charstring::printf(label,sizeof(label),"%s: execute",mode);
	if (!client.query3(ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,
				cursorid,0,query,&bind,1,1,values,1)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}

	// a bind the module framed the other way is a clr it can't
	// reassemble - "malformed clr: bad chunk length" - and the query
	// fails outright rather than coming back wrong, so this assertion is
	// the request half of the leg
	bool	executed=(client.getResponseTtcCode()==ORA_TTC_DESCRIBE_INFO);
	report(label,executed);
	if (!executed) {
		if (client.getResponseTtcCode()==ORA_TTC_ERROR) {
			reportResponseError(&client);
		}
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

	// the response half: the whole long form built here from the value
	// that was sent, which the response has to carry verbatim
	unsigned char	expected[ORA_CLR_TEST_BUFFER_SIZE];
	size_t		expectedsize=buildLongFormClr(value,valuesize,
						framebigchunk,
						ORA_CLR_TRAILING_ZEROS,
						expected,sizeof(expected));
	size_t	maxchunk=(framebigchunk)?
			ORA_CLR_MAX_BIG_CHUNK_SIZE:ORA_CLR_MAX_CHUNK_SIZE;
	size_t	chunks=(valuesize+maxchunk-1)/maxchunk;
	charstring::printf(label,sizeof(label),
			"%s: the value comes back as %d %s chunk(s)",
			mode,(int)chunks,
			(framebigchunk)?"big":"raw byte");
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
			"%s: the bound value round trips exactly",mode);
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

// the port a listener ended up on.  the defaults are what
// test/sqlrelay.conf.d/oracleprotocol.conf.in's @ORACLEPROTOCOLPORTn@ tokens
// default to; test/test.sh exports the real ones, the same way
// oracleclrchunks reads ORACLEPROTOCOLPORT1
static uint16_t portFromEnvironment(const char *name, uint16_t fallback) {
	const char	*value=environment::getValue(name);
	if (charstring::isNullOrEmpty(value)) {
		return fallback;
	}
	return (uint16_t)charstring::convertToInteger(value);
}

int main(int argc, char **argv) {

	stdoutput.printf("\n====== bigchunkclr off ======\n\n");

	// the oracleprotocol test instance - see
	// test/sqlrelay.conf.d/oracleprotocol.conf.  it isn't a real oracle
	// server, it's a listener speaking oracle's wire protocol, so these
	// are just the ports it was configured with rather than anything a
	// real database is on
	const char	*host="127.0.0.1";
	const char	*sid="ora1";
	const char	*user="testuser";
	const char	*password="testpassword";

	uint16_t	offport=portFromEnvironment("ORACLEPROTOCOLPORT6",1526);
	uint16_t	defaultport=
			portFromEnvironment("ORACLEPROTOCOLPORT1",1521);

	char	value[ORA_CLR_TEST_VALUE_SIZE+1];
	buildTestValue(value,ORA_CLR_TEST_VALUE_SIZE);

	// dual hands the bind straight back, so the value that comes out of
	// the fetch is the value that went in
	const char	*query="select :b from dual";

	stdoutput.printf("bind and column value: %d bytes\n",
				(int)ORA_CLR_TEST_VALUE_SIZE);

	// the listener with bigchunkclr="off".  the bit has to be gone from
	// what it advertises, and everything has to be framed raw
	runLeg("bigchunkclr off",host,offport,sid,user,password,
			query,value,ORA_CLR_TEST_VALUE_SIZE,false,false);

	// and the listener without the attribute, which is the control: same
	// client, same query, big chunk framing.  a failure here says the
	// query or the harness is wrong rather than the attribute
	runLeg("default listener",host,defaultport,sid,user,password,
			query,value,ORA_CLR_TEST_VALUE_SIZE,true,true);

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
