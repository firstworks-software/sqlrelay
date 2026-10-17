// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>

#include "oracleprotocolclient.cpp"

// Regression coverage for #10538: genuine 9i sqlplus logged in, sent a break
// right after the first fetch of its bootstrap, and lost the session.
//
// sqlplus's bootstrap is TTI_QUERY, TTI_QUERY2 and TTI_FETCH of "SELECT USER
// FROM DUAL", with a define whose flag byte is 0x00.  The flag's bits say
// which of the indicator (0x01), return length (0x02) and return code (0x04)
// pointers odefin() passed, and a real server sends a value's indicator and
// return code only when their bits are set.  The module sent both whatever
// the flag said, so sqlplus read two bytes it never asked for and broke the
// call.  The next call, a 10g style OALL8 of "SELECT NULL FROM DUAL FOR
// UPDATE NOWAIT", was answered in the shape a 10g client reads, with fields a
// real 10.2 server leaves out below field version 4.
//
// The genuine 9i clients only run on the buildfarm, so this plays one.  It
// sends 9.2.0.4 sqlplus's data type negotiation, logs in with o3logon and the
// 9i verifier, the way a 0x76 portable 9i client does, and then sends the
// bootstrap and the OALL8 as that sqlplus sent them, pointers and all.  It
// checks each answer against what a real 10.2 server sends a 9i client.  It
// does the same fetch with a define flag of 0x01 and of 0x07, which have to
// keep getting what they ask for.
//
// Its requests are 9.2.0.4 sqlplus's own, from
// samples/10538-redhat9x86-sqlplus-select-sqlrelay121-after.oraproxy - "the
// capture" below.
//
// It runs against the oracleprotocol instance's 12.1 listener
// (ORACLEPROTOCOLPORT1), or with "10.2", its 10.2 one (ORACLEPROTOCOLPORT12).

static const char	*host="127.0.0.1";
static const char	*sid="ora1";
static const char	*user="testuser";
static const char	*password="testpassword";

// the ttc and tti codes oracleprotocolclient.cpp doesn't name
static const unsigned char	ORA_TTI_QUERY2=0x47;
static const unsigned char	ORA_TTI_CLOSE=0x08;
static const unsigned char	ORA_TTC_STATUS=0x09;
static const unsigned char	ORA_TTC_PIGGYBACK_TTI_FUNCTION=0x11;
static const unsigned char	ORA_TTI_SWITCH_SESSION=0x6b;

// the define flag bits - OCI7_DEFINE_* in src/protocols/oracle.cpp
static const unsigned char	DEFINE_INDICATOR=0x01;
static const unsigned char	DEFINE_RETURN_CODE=0x04;

// 9.2.0.4 sqlplus's data type negotiation, behind its ttc code, from [0009]
// of the capture.
// the character sets, the encoding flags, the compile capabilities - logon
// types 0x05 at 4, field version 2 at 7 - the runtime capabilities and the db
// time zone group are as it sent them.  its ub1 data type list is cut down to
// the character, number and date types and the pointer type, whose 0x0a
// representation is what makes every pointer field after it four raw bytes
static const unsigned char	datatyperequest9i[]={
	0x1f, 0x00, 0x1f, 0x00,
	0x12,
	0x15,
	0x06, 0x01, 0x01, 0x01, 0x05, 0x01, 0x01, 0x02,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x7f,
	0x0f, 0x03, 0x06, 0x03, 0x00,
	0x02,
	0x02, 0x01,
	0x80, 0x00, 0x00, 0x00, 0x3c, 0x3c, 0x3c, 0x80,
	0x00, 0x00, 0x00,
	0x01, 0x01, 0x01, 0x00,
	0x02, 0x02, 0x0a, 0x00,
	0x0c, 0x0c, 0x0a, 0x00,
	0x20, 0x20, 0x0a, 0x20, 0x01, 0x00,
	0x60, 0x60, 0x01, 0x00,
	0x00
};

// what the OALL8 carries behind the piggyback's sequence number, up to the
// embedded call's own, then behind that, from [0027] of the capture:
// the session id and serial number, the mode, then the embedded TTI_QUERY3
// with its options (parse, execute, fetch, no pl/sql), cursor 0, sqlplus's
// own pointers, the statement and the al8i4 vector
static const unsigned char	oall8head[]={
	0x00, 0x00, 0x01, 0x01, 0x03, 0x5e
};
static const unsigned char	oall8tail[]={
	0x02, 0x80, 0x61, 0x00, 0x34, 0x3b, 0x11, 0x08,
	0x01, 0x28, 0x38, 0x15, 0x10, 0x08, 0x01, 0x0c,
	0x00, 0x00, 0x00, 0x00, 0x68, 0x15, 0x10, 0x08,
	0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x6a, 0x15, 0x10, 0x08, 0xa8, 0x3b, 0x11,
	0x08, 0x00, 0x28,
	'S', 'E', 'L', 'E', 'C', 'T', ' ', 'N',
	'U', 'L', 'L', ' ', 'F', 'R', 'O', 'M',
	' ', 'D', 'U', 'A', 'L', ' ', 'F', 'O',
	'R', ' ', 'U', 'P', 'D', 'A', 'T', 'E',
	' ', 'N', 'O', 'W', 'A', 'I', 'T', 0x00,
	0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x01, 0x00, 0x00, 0x00, 0x00
};

int	status=0;
const char	*success="\033[32msuccess\033[0m";
const char	*failure="\033[31mfailure\033[0m";

// the next call's sequence number.  the login took 2 and 3
static unsigned char	seqnumber=4;

static void report(const char *label, bool ok) {
	stdoutput.printf("%s: %s\n",label,(ok)?success:failure);
	if (!ok) {
		status=1;
	}
}

static void dumpResponse(oracleprotocolclient *client) {
	stdoutput.printf("    response (%d bytes):",
				(int)client->getResponseSize());
	for (size_t i=0; i<client->getResponseSize(); i++) {
		stdoutput.printf(" %02x",client->getResponse()[i]);
	}
	stdoutput.printf("\n");
}

// whether the next bytes of the response are exactly these
static bool expectBytes(oracleprotocolclient *client,
				const unsigned char *bytes, size_t size) {
	unsigned char	actual[64];
	return (size<=sizeof(actual) &&
		client->readBytes(actual,size) &&
		!bytestring::compare(actual,bytes,size));
}

// whether the response ends with exactly these bytes, from where the walk is
static bool expectRest(oracleprotocolclient *client,
				const unsigned char *bytes, size_t size) {
	unsigned char	extra=0;
	return (expectBytes(client,bytes,size) && !client->readByte(&extra));
}

// the oci7 summary object a real 10.2 server ends a 9i client's fetch and
// OALL8 answers with: the ttc code, the end of call status, the rows
// processed, the error number, a reserved field, the array error number, the
// cursor id, the parse error offset, the command type 3, twelve zero bytes
// that include the rowid, the call number, a zero, the success iterations 1,
// and four zeros.  [0028] and [0032] of
// samples/10538-solaris8sparc-sqlplus-select-realserver-oracle102.oraproxy
static size_t buildSummary(unsigned char *summary,
				unsigned char callstatus,
				unsigned char rowsprocessed,
				unsigned char cursorid,
				unsigned char callnumber) {
	size_t	i=0;
	summary[i++]=ORA_TTC_ERROR;
	summary[i++]=0x01;
	summary[i++]=callstatus;
	if (rowsprocessed) {
		summary[i++]=0x01;
		summary[i++]=rowsprocessed;
	} else {
		summary[i++]=0x00;
	}
	summary[i++]=0x00;
	summary[i++]=0x00;
	summary[i++]=0x00;
	summary[i++]=0x01;
	summary[i++]=cursorid;
	summary[i++]=0x00;
	summary[i++]=0x03;
	for (size_t j=0; j<12; j++) {
		summary[i++]=0x00;
	}
	summary[i++]=callnumber;
	summary[i++]=0x00;
	summary[i++]=0x01;
	summary[i++]=0x01;
	for (size_t j=0; j<4; j++) {
		summary[i++]=0x00;
	}
	return i;
}

// TTI_OPEN, as 9.2.0.4 sqlplus sends it - [0017] of the capture
static bool openCursor(oracleprotocolclient *client, uint32_t *cursorid) {

	static const unsigned char	cursorptr[]={0x14, 0xdd, 0xff, 0xbf};

	client->beginTtiCall(ORA_TTI_OPEN);
	client->appendByte(seqnumber++);
	client->appendBytes(cursorptr,sizeof(cursorptr));
	client->appendLenPreInt(0);
	if (!client->sendPacket() || !client->recvPacket()) {
		return false;
	}

	client->rewindResponse();
	unsigned char	skip[3];
	return (client->getResponseTtcCode()==ORA_TTC_OK &&
		client->readBytes(skip,sizeof(skip)) &&
		client->readLenPreInt(cursorid) &&
		*cursorid && *cursorid<=0xff);
}

// TTI_CLOSE - [0025] of the capture
static bool closeCursor(oracleprotocolclient *client, uint32_t cursorid) {

	client->beginTtiCall(ORA_TTI_CLOSE);
	client->appendByte(seqnumber++);
	client->appendLenPreInt(cursorid);
	return (client->sendPacket() && client->recvPacket() &&
			client->getResponseTtcCode()==ORA_TTC_STATUS);
}

// the bootstrap select, parsed, defined, executed and fetched as sqlplus
// does it - [0019] to [0024] of the capture - with "defineflag" in the define
static void runSelect(oracleprotocolclient *client,
				const char *label, unsigned char defineflag) {

	stdoutput.printf("\n%s, define flag 0x%02x\n",label,defineflag);

	char	l[256];
	uint32_t	cursorid=0;
	charstring::printf(l,sizeof(l),"%s: open",label);
	if (!openCursor(client,&cursorid)) {
		report(l,false);
		stdoutput.printf("    %s\n",client->getError());
		return;
	}
	report(l,true);

	// TTI_QUERY.  sqlplus's text carries its own nul, which the size
	// counts
	static const char		query[]="SELECT USER FROM DUAL";
	static const unsigned char	textptr[]={0x5c, 0xb6, 0x0c, 0x08};
	client->beginTtiCall(ORA_TTI_QUERY);
	client->appendByte(seqnumber++);
	client->appendLenPreInt(cursorid);
	client->appendBytes(textptr,sizeof(textptr));
	client->appendLenPreInt(sizeof(query));
	client->appendByte((unsigned char)sizeof(query));
	client->appendBytes((const unsigned char *)query,sizeof(query));
	uint32_t	curid=0;
	unsigned char	commandtype=0;
	uint32_t	rowsprocessed=0;
	uint32_t	successiterations=0;
	bool		leftover=false;
	charstring::printf(l,sizeof(l),"%s: parse",label);
	bool	ok=(client->sendPacket() && client->recvPacket() &&
			readLegacySummary(client,&curid,&commandtype,
						&rowsprocessed,
						&successiterations,
						&leftover) &&
			curid==cursorid && commandtype==3 && !leftover);
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
		return;
	}

	// TTI_QUERY2: define and execute, one define of a 29 byte varchar
	// in character set 31, the way sqlplus sends it
	static const unsigned char	query2head[]={
		0x02, 0x80, 0x30
	};
	static const unsigned char	query2tail[]={
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x38, 0x15, 0x10, 0x08, 0x01, 0x07,
		0x88, 0xae, 0x16, 0x08, 0x01, 0x02, 0x00, 0x00,
		0x00, 0x00, 0xd0, 0xad, 0x16, 0x08, 0x01, 0x01,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x01
	};
	static const unsigned char	definetail[]={
		0x00, 0x00, 0x01, 0x1d, 0x00, 0x00, 0x00, 0x00,
		0x01, 0x1f, 0x00, 0x00
	};
	client->beginTtiCall(ORA_TTI_QUERY2);
	client->appendByte(seqnumber++);
	client->appendBytes(query2head,sizeof(query2head));
	client->appendLenPreInt(cursorid);
	client->appendBytes(query2tail,sizeof(query2tail));
	client->appendByte(defineflag);
	client->appendBytes(definetail,sizeof(definetail));
	charstring::printf(l,sizeof(l),"%s: define and execute",label);
	ok=(client->sendPacket() && client->recvPacket() &&
			client->getResponseTtcCode()==ORA_TTC_OK);
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
		return;
	}

	// TTI_FETCH of one row
	unsigned char	fetchseqnumber=seqnumber++;
	client->beginTtiCall(ORA_TTI_FETCH);
	client->appendByte(fetchseqnumber);
	client->appendLenPreInt(cursorid);
	client->appendLenPreInt(1);
	charstring::printf(l,sizeof(l),"%s: fetch",label);
	ok=(client->sendPacket() && client->recvPacket());
	report(l,ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client->getError());
		return;
	}

	// the row header a real 10.2 server sends a 9i client: flags 0x02,
	// one column, iteration 0, one row and three zero lengths
	static const unsigned char	rowheader[]={
		ORA_TTC_ROW_HEADER,
		0x02, 0x01, 0x01, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00,
		ORA_TTC_ROW_DATA
	};
	unsigned char	dataflags[2];
	unsigned char	valuesize=0;
	unsigned char	value[256];
	client->rewindResponse();
	charstring::printf(l,sizeof(l),"%s: the row header is a real "
						"server's",label);
	ok=(client->readBytes(dataflags,sizeof(dataflags)) &&
		expectBytes(client,rowheader,sizeof(rowheader)));
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
		return;
	}

	charstring::printf(l,sizeof(l),"%s: the row carries the user",label);
	ok=(client->readByte(&valuesize) && valuesize &&
		valuesize<=252 && client->readBytes(value,valuesize));
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
		return;
	}
	value[valuesize]='\0';
	stdoutput.printf("    value: %s\n",value);

	// behind the value, the indicator and the return code, each only if
	// the define asked for it - 00 for a value that isn't null - then the
	// summary.  a flag of 0x00 gets neither, [0028] of samples/10538-
	// solaris8sparc-sqlplus-select-realserver-oracle102.oraproxy.  0x01
	// gets the indicator alone, [0030] of samples/10538-solaris8sparc-
	// oci7-defineflags-multirow-mask1-realserver-oracle102.oraproxy.  0x07
	// gets both, [0024] of samples/oracle102-oci7-portable-login-
	// select.cap
	unsigned char	expected[64];
	size_t		expectedsize=0;
	if (defineflag&DEFINE_INDICATOR) {
		expected[expectedsize++]=0x00;
	}
	if (defineflag&DEFINE_RETURN_CODE) {
		expected[expectedsize++]=0x00;
	}
	expectedsize+=buildSummary(expected+expectedsize,
					0x01,0x01,(unsigned char)cursorid,
					fetchseqnumber);
	charstring::printf(l,sizeof(l),"%s: the value is followed by "
					"what the define asked for, then a "
					"real server's summary",label);
	ok=expectRest(client,expected,expectedsize);
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
	}

	charstring::printf(l,sizeof(l),"%s: close",label);
	report(l,closeCursor(client,cursorid));
}

// the OALL8 sqlplus sends behind the bootstrap, piggybacked on a switch
// session as sqlplus sends it - [0027] of the capture - and its answer,
// against a real 10.2 server's - [0032] of samples/10538-solaris8sparc-
// sqlplus-select-realserver-oracle102.oraproxy
static void runOall8(oracleprotocolclient *client) {

	const char	*label="for update";
	stdoutput.printf("\n%s\n",label);

	unsigned char	piggybackseqnumber=seqnumber++;
	unsigned char	callseqnumber=seqnumber++;
	client->beginPacket(ORA_PACKET_DATA);
	client->appendBE16(0);
	client->appendByte(ORA_TTC_PIGGYBACK_TTI_FUNCTION);
	client->appendByte(ORA_TTI_SWITCH_SESSION);
	client->appendByte(piggybackseqnumber);
	client->appendBytes(oall8head,sizeof(oall8head));
	client->appendByte(callseqnumber);
	client->appendBytes(oall8tail,sizeof(oall8tail));
	char	l[256];
	charstring::printf(l,sizeof(l),"%s: execute",label);
	bool	ok=(client->sendPacket() && client->recvPacket());
	report(l,ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client->getError());
		return;
	}

	// the describe info, with the query hash and date skipped, then no
	// max row size, one column and 10.2's constant 0x39
	static const unsigned char	describehead[]={
		0x00, 0x01, 0x01, 0x39
	};
	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	unsigned char	hashsize=0;
	unsigned char	hash[256];
	client->rewindResponse();
	charstring::printf(l,sizeof(l),"%s: describe info with 10.2's "
						"constant",label);
	ok=(client->readBytes(dataflags,sizeof(dataflags)) &&
		client->readByte(&ttccode) && ttccode==ORA_TTC_DESCRIBE_INFO &&
		client->readByte(&hashsize) &&
		client->readBytes(hash,hashsize) &&
		expectBytes(client,describehead,sizeof(describehead)));
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
		return;
	}

	// the column: a varchar2 0 bytes wide, in character set 1, named
	// NULL, and nothing behind the type name length - no column position
	static const unsigned char	column[]={
		0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x01, 0x01, 0x01, 0x00, 0x01, 0x04, 0x01,
		0x04, 0x04, 'N', 'U', 'L', 'L', 0x00, 0x00
	};
	charstring::printf(l,sizeof(l),"%s: the column is described as "
					"a real server describes select "
					"null to a 9i client",label);
	ok=expectBytes(client,column,sizeof(column));
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
		return;
	}

	// the date, and then straight into the return parameters
	static const unsigned char	datesize[]={0x01, 0x07, 0x07};
	unsigned char	date[7];
	unsigned char	okcode=0;
	charstring::printf(l,sizeof(l),"%s: nothing follows the date but "
					"the return parameters",label);
	ok=(expectBytes(client,datesize,sizeof(datesize)) &&
		client->readBytes(date,sizeof(date)) &&
		client->readByte(&okcode) && okcode==ORA_TTC_OK);
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
		return;
	}

	// six al8o4 values, the third of them the cursor id, then the al8txl
	// size and the session's key/value pairs
	uint32_t	al8o4count=0;
	uint32_t	al8o4[6];
	uint32_t	al8txl=0;
	uint32_t	paircount=0;
	ok=(client->readLenPreInt(&al8o4count) && al8o4count==6);
	for (uint32_t i=0; ok && i<6; i++) {
		ok=client->readLenPreInt(&al8o4[i]);
	}
	ok=(ok && client->readLenPreInt(&al8txl) && !al8txl &&
		client->readLenPreInt(&paircount));
	for (uint32_t i=0; ok && i<paircount; i++) {
		uint32_t	keysize=0;
		uint32_t	valuesize=0;
		uint32_t	flags=0;
		unsigned char	size=0;
		unsigned char	bytes[256];
		ok=(client->readLenPreInt(&keysize) &&
			(!keysize || (client->readByte(&size) &&
				client->readBytes(bytes,size))) &&
			client->readLenPreInt(&valuesize) &&
			(!valuesize || (client->readByte(&size) &&
				client->readBytes(bytes,size))) &&
			client->readLenPreInt(&flags));
	}
	charstring::printf(l,sizeof(l),"%s: return parameters decode",label);
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
		return;
	}
	stdoutput.printf("    cursor id: %d\n",(int)al8o4[2]);
	charstring::printf(l,sizeof(l),"%s: the third al8o4 value is a "
						"cursor id",label);
	ok=(al8o4[2] && al8o4[2]<=0xff);
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
		return;
	}

	// then the summary, with no registration id ahead of it, no end to
	// end sequence number in it and no rows behind it.  sqlplus never
	// fetches this one's row, and a real server sends none
	unsigned char	expected[64];
	size_t		expectedsize=buildSummary(expected,0x02,0x00,
						(unsigned char)al8o4[2],
						callseqnumber);
	charstring::printf(l,sizeof(l),"%s: a real server's summary "
					"follows, and nothing else",label);
	ok=expectRest(client,expected,expectedsize);
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
	}
}

int main(int argc, char **argv) {

	bool	at102=(argc>1 && !charstring::compare(argv[1],"10.2"));

	stdoutput.printf("\n====== #10538 9i sqlplus bootstrap "
				"(serverversion %s) ======\n",
				(at102)?"10.2":"12.1");

	const char	*portvariable=(at102)?"ORACLEPROTOCOLPORT12":
						"ORACLEPROTOCOLPORT1";
	uint16_t	port=(at102)?1532:1521;
	const char	*portoverride=environment::getValue(portvariable);
	if (!charstring::isNullOrEmpty(portoverride)) {
		port=(uint16_t)charstring::convertToInteger(portoverride);
	}

	oracleprotocolclient	client;
	client.setDataTypeRequest(datatyperequest9i,
					sizeof(datatyperequest9i));
	client.setAuthSequenceNumbers(2,3);

	if (!client.connect(host,port,sid)) {
		report("connect",false);
		stdoutput.printf("    %s\n",client.getError());
		return status;
	}
	report("connect",true);

	bool	ok=(client.requestChallenge(user) &&
			client.getResponseTtcCode()==ORA_TTC_OK &&
			!client.gotBreak() &&
			client.answerO3LogonChallenge(user,password));
	report("o3logon login",ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client.getError());
		dumpResponse(&client);
		return status;
	}

	runSelect(&client,"sqlplus bootstrap",0x00);
	runSelect(&client,"indicator",DEFINE_INDICATOR);
	runSelect(&client,"indicator, return length and return code",0x07);
	runOall8(&client);

	client.disconnect();

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
