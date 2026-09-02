// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Coverage for getQuery3BindValues() in src/protocols/oracle.cpp, which
// reads the bind values off a request.
//
// The al8i4 vector's second element is the execution iteration count, and
// the module used to read one TTC_ROW_DATA block per iteration the request
// claimed.  python-oracledb's thin driver claims 0 on both of the calls that
// carry bind values - the TTI_QUERY3 that parses, binds and executes, and
// the TTI_EXECUTE that re-executes a statement it already parsed - and sends
// a real block for each of them anyway.  a loop bounded by the claim never
// ran its body for one of those, so every bind value was dropped without a
// word:
//
//	- a select's binds were never installed, and the backend answered
//	  ORA-01008, "not all variables bound"
//	- a re-execute's fresh values never reached the statement, which ran
//	  again on whatever the execute before it had left installed.  no
//	  error at all, just the wrong answer
//
// getQuery3BindValues() now takes the block count from what is left in the
// packet, rather than from the claim, when the claim is 0 and there are
// binds to fill.  a nonzero claim still bounds the loop the way it always
// did, which is what the third scenario below is here to show.
//
// oracleprotocolclient's query3() and reexecute() take the iteration count
// and the block count as two separate arguments for this - a scenario says
// "claim 0 iterations, send 1 block" by passing exactly that.

// putRowHeader()'s flags byte in the answer to a fetch
static const unsigned char	ORA_ROW_HEADER_FLAGS_FETCH=0x02;

// what putIoVector() leads an execute response's out bind values with
static const unsigned char	ORA_TTC_IO_VECTOR=0x0b;
static const unsigned char	ORA_IO_VECTOR_CONSTANT=0x05;

// the six count prefixed ub4s putIoVector() writes behind its constant: the
// bind count, and then five whose meaning no capture explains
static const uint32_t		ORA_IO_VECTOR_FILLER_COUNT=5;

// putOutBindValues() writes a signed indicator behind every value it sends:
// one zero byte for a value, and these two behind a null's zero length
static const size_t		ORA_OUT_BIND_INDICATOR_SIZE=1;
static const size_t		ORA_OUT_BIND_NULL_INDICATOR_SIZE=2;

// how wide the binds here are declared.  installQuery3Binds() sizes an out
// bind's buffer from this, and it is well past anything these scenarios bind
static const uint32_t	ORA_BIND_BUFFER_SIZE=512;

// room for a value that comes back, and its terminator
static const size_t	ORA_VALUE_BUFFER_SIZE=1024;

// the pl/sql block's placeholders - :o, :i and :l
static const uint32_t	ORA_PLSQL_BIND_COUNT=3;

// no scenario here sends more row data blocks than this
static const uint32_t	ORA_MAX_BLOCKS=4;

int	status=0;
const char	*success="\033[32msuccess\033[0m";
const char	*failure="\033[31mfailure\033[0m";

static void report(const char *label, bool ok) {
	stdoutput.printf("%s: %s\n",label,(ok)?success:failure);
	if (!ok) {
		status=1;
	}
}

// a dropped bind value comes back as an oracle error rather than as
// anything this can decode - ORA-01008 is the one to expect - so print the
// response whole and let the message speak for itself
static void reportResponseError(oracleprotocolclient *client) {
	stdoutput.printf("the listener answered with an error:\n");
	stdoutput.safePrint(client->getResponse(),
				(int32_t)client->getResponseSize());
	stdoutput.printf("\n");
}

// connect, log in and open a cursor.  every scenario needs all three and
// none of them is what it is testing, so they report under the scenario's
// name and a failure in any of them ends it
static bool beginSession(oracleprotocolclient *client,
				const char *scenario,
				const char *host, uint16_t port,
				const char *sid,
				const char *user, const char *password,
				uint32_t *cursorid) {

	char	label[128];

	charstring::printf(label,sizeof(label),"%s: connect",scenario);
	if (!client->connect(host,port,sid)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report(label,true);

	charstring::printf(label,sizeof(label),"%s: login",scenario);
	if (!client->login(user,password)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report(label,true);

	charstring::printf(label,sizeof(label),"%s: open cursor",scenario);
	if (!client->open(cursorid)) {
		report(label,false);
		stdoutput.printf("%s\n",client->getError());
		return false;
	}
	report(label,true);

	return true;
}

// walk a fetch response - the data flags, a row header, one row data message
// carrying the single column, and then the summary object, which this stops
// short of.  see sendFetch3Response(), putRowHeader() and putRowData() in
// src/protocols/oracle.cpp.  the same walk oracleclrchunks does
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

// walk what a statement with out binds answers with, which is the two
// response shapes putOutBindValues() gets written into:
//
//	- a full execute's, from sendQuery3Response(), where an io vector
//	  giving each bind's direction comes first
//	- a re-execute's, from sendReexecuteResponse(), where it doesn't -
//	  the client learned the directions from the execute before it
//
// every bind of the pl/sql block below is classified in-out, so there is one
// value here per descriptor, in descriptor order.  "values" is bindcount
// slots of "valuesize" bytes each
static bool readOutBindValues(oracleprotocolclient *client,
				bool iovector,
				uint32_t bindcount,
				unsigned char *values,
				size_t valuesize,
				size_t *sizes,
				bool *isnulls) {

	client->rewindResponse();

	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	if (!client->readBytes(dataflags,sizeof(dataflags))) {
		return false;
	}

	if (iovector) {

		unsigned char	constant=0;
		uint32_t	iovbindcount=0;
		uint32_t	skip=0;
		if (!client->readByte(&ttccode) ||
			ttccode!=ORA_TTC_IO_VECTOR ||
			!client->readByte(&constant) ||
			constant!=ORA_IO_VECTOR_CONSTANT ||
			!client->readLenPreInt(&iovbindcount) ||
			iovbindcount!=bindcount) {
			return false;
		}
		for (uint32_t i=0; i<ORA_IO_VECTOR_FILLER_COUNT; i++) {
			if (!client->readLenPreInt(&skip)) {
				return false;
			}
		}

		// one raw direction byte per descriptor
		for (uint32_t i=0; i<bindcount; i++) {
			unsigned char	direction=0;
			if (!client->readByte(&direction)) {
				return false;
			}
		}
	}

	if (!client->readByte(&ttccode) || ttccode!=ORA_TTC_ROW_DATA) {
		return false;
	}

	for (uint32_t i=0; i<bindcount; i++) {

		// leave room for a terminator, so the value can be read back
		// as a string
		if (!client->readLenBytes(values+i*valuesize,valuesize-1,
						&(sizes[i]),&(isnulls[i]))) {
			return false;
		}
		values[i*valuesize+sizes[i]]='\0';

		size_t		indicatorsize=(isnulls[i])?
					ORA_OUT_BIND_NULL_INDICATOR_SIZE:
					ORA_OUT_BIND_INDICATOR_SIZE;
		unsigned char	indicator[ORA_OUT_BIND_NULL_INDICATOR_SIZE];
		if (!client->readBytes(indicator,indicatorsize)) {
			return false;
		}
	}

	return true;
}

// one whole session that binds a value into a select and reads it back out
// of the result set.  "iterations" is what the al8i4 vector claims and
// "blockcount" is how many row data blocks really go out, one value each -
// the two are what each scenario varies.
//
// a block is a complete execution, so with more than one the last block's
// value is the one the result set left behind, and that is what the fetch
// has to come back with
static void runSelectBind(const char *scenario,
				const char *host, uint16_t port,
				const char *sid,
				const char *user, const char *password,
				uint32_t iterations,
				const char * const *blockvalues,
				uint32_t blockcount) {

	char	label[128];

	stdoutput.printf("\n--- %s ---\n\n",scenario);
	stdoutput.printf("iterations claimed: %d, row data blocks sent: %d\n\n",
					(int)iterations,(int)blockcount);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!beginSession(&client,scenario,host,port,sid,
					user,password,&cursorid)) {
		client.disconnect();
		return;
	}

	// dual hands the bind straight back, so the value that comes out of
	// the fetch is the value that went in - which is the whole assertion
	const char	*query="select :b from dual";

	oracleprotocolbind	bind;
	bind.varchar(ORA_BIND_BUFFER_SIZE);

	// one bind, so the flat block-major array is one value per block
	oracleprotocolbindvalue	values[ORA_MAX_BLOCKS];
	for (uint32_t i=0; i<blockcount; i++) {
		values[i].set(blockvalues[i]);
	}

	// parse and execute, asking for no rows up front, the way
	// oracleclrchunks does: sendQuery3Response() answers with the
	// describe alone and the row comes back on its own in answer to the
	// fetch below
	charstring::printf(label,sizeof(label),"%s: execute",scenario);
	if (!client.query3(ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_NOPLSQL,
				cursorid,0,query,&bind,1,
				iterations,values,blockcount)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,client.getResponseTtcCode()==ORA_TTC_DESCRIBE_INFO);

	// a bind value the module dropped is a placeholder the backend never
	// got a value for, and it says so: ORA-01008
	if (client.getResponseTtcCode()==ORA_TTC_ERROR) {
		reportResponseError(&client);
		client.disconnect();
		return;
	}

	charstring::printf(label,sizeof(label),"%s: fetch",scenario);
	if (!client.fetch(cursorid,1)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,true);

	uint32_t	colcount=0;
	unsigned char	value[ORA_VALUE_BUFFER_SIZE];
	size_t		valuesize=0;
	bool		isnull=false;
	bool		decoded=readFetch3Row(&client,&colcount,
						value,sizeof(value)-1,
						&valuesize,&isnull);
	charstring::printf(label,sizeof(label),
				"%s: fetch response decodes",scenario);
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
	value[valuesize]='\0';

	const char	*expected=blockvalues[blockcount-1];
	charstring::printf(label,sizeof(label),
				"%s: the bound value comes back",scenario);
	report(label,!isnull && colcount==1 &&
			!charstring::compare((const char *)value,expected));
	if (charstring::compare((const char *)value,expected)) {
		stdoutput.printf("  expected: %s\n",expected);
		stdoutput.printf("  got:      %s\n",
					(isnull)?"NULL":(const char *)value);
	}

	client.disconnect();
}

// one whole session that parses and executes a pl/sql block with in and out
// binds, and then re-executes it with a fresh value.
//
// the block is the ticket's own repro.  classifyQuery3Binds() reads every
// character placeholder in a block as in-out, so all three come back as out
// binds: :o is what the block copied, and :l is how long it found the input
// to be - a second, independent reading of the same value.
//
// the first execute claims one iteration and works either way.  the
// re-execute is the one that matters: it claims 0 and sends a block anyway,
// which is what a thin driver's every re-execute looks like.  with its
// values dropped the statement ran again on the ones the first execute
// installed, so the failure to watch for is not an error but the previous
// value coming back
static void runPlsqlReexecute(const char *scenario,
				const char *host, uint16_t port,
				const char *sid,
				const char *user, const char *password) {

	char	label[128];

	stdoutput.printf("\n--- %s ---\n\n",scenario);

	oracleprotocolclient	client;

	uint32_t	cursorid=0;
	if (!beginSession(&client,scenario,host,port,sid,
					user,password,&cursorid)) {
		client.disconnect();
		return;
	}

	const char	*block="begin :o := :i; :l := length(:i); end;";

	const char	*firstvalue="first value";
	const char	*secondvalue="a longer second value";

	oracleprotocolbind	binds[ORA_PLSQL_BIND_COUNT];
	for (uint32_t i=0; i<ORA_PLSQL_BIND_COUNT; i++) {
		binds[i].varchar(ORA_BIND_BUFFER_SIZE);
	}

	// an out-only placeholder still takes a value slot on the wire, and
	// a real client's carries whatever was in its buffer - a null is the
	// tidiest way to say "nothing going in here"
	oracleprotocolbindvalue	values[ORA_PLSQL_BIND_COUNT];
	values[0].setNull();			// :o
	values[1].set(firstvalue);		// :i
	values[2].setNull();			// :l

	unsigned char	outvalues[ORA_PLSQL_BIND_COUNT*ORA_VALUE_BUFFER_SIZE];
	size_t		outsizes[ORA_PLSQL_BIND_COUNT];
	bool		outisnulls[ORA_PLSQL_BIND_COUNT];

	// OPTION_SNDIOV is what asks for the out bind values back, and
	// classifyQuery3Binds() reads nothing as an out bind without it.  no
	// OPTION_NOPLSQL either - that says the statement is not a block
	charstring::printf(label,sizeof(label),
			"%s: first execute (iterations 1)",scenario);
	if (!client.query3(ORA_OPTION_PARSE|
				ORA_OPTION_EXECUTE|
				ORA_OPTION_SNDIOV,
				cursorid,0,block,
				binds,ORA_PLSQL_BIND_COUNT,
				1,values,1)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,client.getResponseTtcCode()==ORA_TTC_IO_VECTOR);

	if (client.getResponseTtcCode()==ORA_TTC_ERROR) {
		reportResponseError(&client);
		client.disconnect();
		return;
	}

	bool	decoded=readOutBindValues(&client,true,ORA_PLSQL_BIND_COUNT,
						outvalues,
						ORA_VALUE_BUFFER_SIZE,
						outsizes,outisnulls);
	charstring::printf(label,sizeof(label),
			"%s: first execute out binds decode",scenario);
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

	// the control half of this scenario: at one claimed iteration the
	// value has always been read, so this says the request itself is
	// well formed before the interesting one goes out
	const char	*out=(const char *)outvalues;
	charstring::printf(label,sizeof(label),
			"%s: first execute bound the value",scenario);
	report(label,!outisnulls[0] &&
			!charstring::compare(out,firstvalue));
	if (charstring::compare(out,firstvalue)) {
		stdoutput.printf("  expected: %s\n",firstvalue);
		stdoutput.printf("  got:      %s\n",
					(outisnulls[0])?"NULL":out);
	}

	// the re-execute: fresh values, no descriptors and no query text -
	// restoreQuery3Binds() remembers the rest - and 0 claimed iterations
	// with a real block behind it
	values[1].set(secondvalue);

	charstring::printf(label,sizeof(label),
			"%s: re-execute at iterations 0",scenario);
	if (!client.reexecute(cursorid,0,ORA_OPTION_EXECUTE,0,
					ORA_PLSQL_BIND_COUNT,values,1)) {
		report(label,false);
		stdoutput.printf("%s\n",client.getError());
		client.disconnect();
		return;
	}
	report(label,client.getResponseTtcCode()==ORA_TTC_ROW_DATA);

	if (client.getResponseTtcCode()==ORA_TTC_ERROR) {
		reportResponseError(&client);
		client.disconnect();
		return;
	}

	// no io vector this time - sendReexecuteResponse() sends the values
	// on their own
	decoded=readOutBindValues(&client,false,ORA_PLSQL_BIND_COUNT,
					outvalues,ORA_VALUE_BUFFER_SIZE,
					outsizes,outisnulls);
	charstring::printf(label,sizeof(label),
			"%s: re-execute out binds decode",scenario);
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

	// the assertion the whole scenario is for.  a dropped block leaves
	// the previous execute's value installed, so "first value" coming
	// back here is the bug, and so is a null
	charstring::printf(label,sizeof(label),
			"%s: re-executed value bound (not NULL, not stale)",
			scenario);
	report(label,!outisnulls[0] &&
			!charstring::compare(out,secondvalue));
	if (charstring::compare(out,secondvalue)) {
		stdoutput.printf("  expected: %s\n",secondvalue);
		stdoutput.printf("  got:      %s\n",
					(outisnulls[0])?"NULL":out);
	}

	// and the same value read a second way, by the block rather than
	// copied by it - a length that still matches the first execute's
	// input would say the copy above came from somewhere else
	const char	*lengthout=(const char *)
				(outvalues+2*ORA_VALUE_BUFFER_SIZE);
	int64_t		expectedlength=(int64_t)
				charstring::getLength(secondvalue);
	charstring::printf(label,sizeof(label),
			"%s: the block saw the re-executed value",scenario);
	report(label,!outisnulls[2] &&
		charstring::convertToInteger(lengthout)==expectedlength);
	if (outisnulls[2] ||
		charstring::convertToInteger(lengthout)!=expectedlength) {
		stdoutput.printf("  expected: %d\n",(int)expectedlength);
		stdoutput.printf("  got:      %s\n",
					(outisnulls[2])?"NULL":lengthout);
	}

	client.disconnect();
}

int main(int argc, char **argv) {

	stdoutput.printf("\n====== bind values at 0 iterations ======\n");

	// the oracleprotocol test instance - see
	// test/sqlrelay.conf.d/oracleprotocol.conf.  it isn't a real oracle
	// server, it's a listener speaking oracle's wire protocol, so 1521
	// is just the port it was configured with rather than anything a
	// real database is on.  ORACLEPROTOCOLPORT1 names the port it
	// actually ended up on, the same way oracleclrchunks reads it
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

	// what a thin driver's parse-bind-execute looks like: 0 iterations
	// claimed, one real block behind it.  the old loop never ran its
	// body for this, and the backend answered ORA-01008
	const char	*onevalue[1];
	onevalue[0]="bound at zero iterations";
	runSelectBind("select at iterations 0",
			host,port,sid,user,password,0,onevalue,1);

	// the same claim with two blocks behind it, which is where the
	// count derived from the packet has to land on the right number
	// rather than merely on a nonzero one.  each block is its own
	// execution, so the second one's value is what the result set holds
	const char	*twovalues[2];
	twovalues[0]="first block";
	twovalues[1]="second block";
	runSelectBind("select at iterations 0, two blocks",
			host,port,sid,user,password,0,twovalues,2);

	// the control: a claim that matches what's really there, which is
	// the path every other client takes and the one the fix leaves
	// alone.  it passes with or without the fix, so a failure here is a
	// broken request rather than a broken read
	const char	*controlvalue[1];
	controlvalue[0]="bound at one iteration";
	runSelectBind("select at iterations 1 (control)",
			host,port,sid,user,password,1,controlvalue,1);

	// and the re-execute, which fails silently rather than loudly
	runPlsqlReexecute("pl/sql re-execute",
				host,port,sid,user,password);

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
