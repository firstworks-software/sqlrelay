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
// It also covers #10702, a 9i client reading a clob.  The module sent the
// clob as utf-16, which a client of a 9i era reads as garbage and breaks on
// (ORA-03120).  A real server sends such a client the clob in the database
// character set: one byte a character from a single-byte database, and from
// a multibyte one each character as a chunk of its own, behind its size.
// The select and the read are as 9.2.0.4 ociselect sent them, from
// samples/10538-redhat9x86-oci8-ociselect-lob-sqlrelay121.oraproxy, and the
// answers are what the two real servers sent a 9i client, from
// samples/10538-solaris8sparc-oci8-ociselect-lob-realserver-oracle122.oraproxy
// and ...-oracle102.oraproxy.  A listener with a multibyte character set is
// run with the framed read, and the single-byte one - the instance of
// ORACLEPROTOCOLPORT7 - with the plain one.
//
// And #10714, a 9i client's call that only fetches.  9.2.0.4 OCI8 defines a
// column after it has executed a statement with no iterations, and then
// fetches 15 rows with a TTI_QUERY3 that has no parse, no execute and no
// descriptors.  It names the 15 in the al8i4 vector, and a real server sends
// 15 rows for it.  The module sent one, and the client made up the rest with
// a TTI_FETCH for each.  Both calls are as 9.2.0.4 sent them, from
// samples/10714-redhat9x86-oci8-ociarray-fetchonly-sqlrelay102.oraproxy, and
// the 15 rows are what a real server sent, [0018] of
// samples/10714-redhat9x86-oci8-ociarray-fetchonly-realserver-oracle102.
// oraproxy.
//
// And #10716, temporary lobs for a 9i client.  The calls are 9.2.0.4 OCI8's
// (create, write, get length, read, erase, copy, append and free of two
// clobs, from samples/10716-redhat9x86-oci8-templobs-sqlrelay102.oraproxy),
// and the answers are what the real oracle102 server sent it, from
// samples/10716-redhat9x86-oci8-templobs-realserver-oracle102.oraproxy: a
// create answers the 86 byte buffer the client sent with the locator in the
// front of it, then the character set and a 1, a write the locator and the
// amount, a read the characters in the database character set - plain
// bytes from a single-byte database, each behind its size from a multibyte
// one - the locator and the amount.
//
// It runs against the oracleprotocol instance's 12.1 listener
// (ORACLEPROTOCOLPORT1), or with "10.2", its 10.2 one (ORACLEPROTOCOLPORT12),
// or with "singlebyte", the single-byte one.

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
static const unsigned char	ORA_TTI_LOB_OPERATIONS=0x60;
static const unsigned char	ORA_TTC_LOB_DATA=0x0e;

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

// the select of a 400 character clob, parsed, defined as a clob and executed
// as 9.2.0.4 ociselect sent its select - [0019] of the module capture - and
// then the read of the clob as it sent that - [0025].  returns the locator
// the row carried
static const unsigned char	clobselecthead[]={
	0x02, 0x80, 0x71, 0x00, 0xfc, 0xf1, 0x05, 0x08,
	0x01, 0x29, 0xc0, 0xcb, 0x05, 0x08, 0x01, 0x0c,
	0x00, 0x00, 0x00, 0x00, 0xf0, 0xcb, 0x05, 0x08,
	0x00, 0x01, 0x01, 0x01, 0x14, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xf2, 0xcb, 0x05, 0x08,
	0x70, 0xf2, 0x05, 0x08, 0x01, 0x03
};
static const unsigned char	clobselecttail[]={
	0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x70, 0x01, 0x00, 0x00, 0x01, 0x14, 0x00, 0x00,
	0x00, 0x00, 0x01, 0x1f, 0x01, 0x00,
	0x71, 0x01, 0x00, 0x00, 0x01, 0x14, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00,
	0x72, 0x01, 0x00, 0x00, 0x01, 0x14, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00
};
static const unsigned char	clobreadhead[]={
	0x01, 0x72, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
	0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7c, 0xe7,
	0xff, 0xbf, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00
};
static const unsigned char	clobreadtail[]={
	0x02, 0x01, 0x90
};
static const unsigned char	clobreadpointer[]={
	0xbc, 0x25, 0x07, 0x08
};

#define LOCATOR_SIZE 114

static void runClobRead(oracleprotocolclient *client, bool framed) {

	const char	*label=(framed)?"clob read, multibyte database":
						"clob read, single-byte database";
	stdoutput.printf("\n%s\n",label);

	char		l[256];
	static const char	query[]=
			"select to_clob(rpad('C',400,'C')),to_blob(hextoraw("
			"'0102')),bfilename('DMP_DIR','x') from dual";
	client->beginTtiCall(ORA_TTI_QUERY3);
	client->appendByte(seqnumber++);

	// the statement's size is at offset 9 of the head, and again in front
	// of the text
	unsigned char	head[sizeof(clobselecthead)];
	bytestring::copy(head,clobselecthead,sizeof(head));
	head[9]=(unsigned char)(sizeof(query)-1);
	client->appendBytes(head,sizeof(head));
	client->appendByte((unsigned char)(sizeof(query)-1));
	client->appendBytes((const unsigned char *)query,sizeof(query)-1);
	client->appendBytes(clobselecttail,sizeof(clobselecttail));
	charstring::printf(l,sizeof(l),"%s: select",label);
	bool	ok=(client->sendPacket() && client->recvPacket());
	report(l,ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client->getError());
		return;
	}

	// the locator is the 114 bytes behind the 0x72 that ends the row's
	// lob length prefix.  the row carries nothing else a clob's
	// characters would be in
	static const unsigned char	locatorhead[]={
		0x00, 0x70, 0x00, 0x02, 0x02, 0x0c
	};
	const unsigned char	*response=client->getResponse();
	size_t			responsesize=client->getResponseSize();
	size_t			start=0;
	bool			found=false;
	for (size_t i=0; i+LOCATOR_SIZE<=responsesize; i++) {
		if (!bytestring::compare(response+i,locatorhead,
						sizeof(locatorhead))) {
			start=i;
			found=true;
			break;
		}
	}
	charstring::printf(l,sizeof(l),"%s: the row carries a locator",label);
	report(l,found);
	if (!found) {
		dumpResponse(client);
		return;
	}
	unsigned char	locator[LOCATOR_SIZE];
	bytestring::copy(locator,response+start,sizeof(locator));

	// the locator says whether the clob's characters are one byte each,
	// by whether it marks its character set present - flags 0x80 at
	// offset 6.  a real server's clob of a single-byte database leaves
	// it clear, [0018] of the 10.2 capture
	charstring::printf(l,sizeof(l),"%s: the locator says how the "
					"characters are sent",label);
	report(l,((locator[6]&0x80)!=0)==framed);

	// the read, of all 400 characters from the first
	client->beginTtiCall(ORA_TTI_LOB_OPERATIONS);
	client->appendByte(seqnumber++);
	client->appendBytes(clobreadpointer,sizeof(clobreadpointer));
	client->appendBytes(clobreadhead,sizeof(clobreadhead));
	client->appendBytes(locator,sizeof(locator));
	client->appendBytes(clobreadtail,sizeof(clobreadtail));
	charstring::printf(l,sizeof(l),"%s: read",label);
	ok=(client->sendPacket() && client->recvPacket());
	report(l,ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client->getError());
		return;
	}

	// the data, in the long form, and then the locator and the amount
	unsigned char	dataflags[2];
	unsigned char	ttccode=0;
	unsigned char	marker=0;
	client->rewindResponse();
	charstring::printf(l,sizeof(l),"%s: lob data follows",label);
	ok=(client->readBytes(dataflags,sizeof(dataflags)) &&
		client->readByte(&ttccode) && ttccode==ORA_TTC_LOB_DATA &&
		client->readByte(&marker) && marker==0xfe);
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
		return;
	}

	// a multibyte database sends each character as a chunk of its own,
	// its size plus one, its size and its byte - 02 01 43 - [0024] of the
	// 12.2 capture; a single-byte one sends the bytes in chunks of up to
	// 255 - ff, then 91 for the rest - [0024] of the 10.2 capture
	unsigned char	chunk[256];
	size_t		chars=0;
	bool		good=true;
	for (;;) {
		unsigned char	size=0;
		if (!client->readByte(&size)) {
			good=false;
			break;
		}
		if (!size) {
			break;
		}
		if (!client->readBytes(chunk,size)) {
			good=false;
			break;
		}
		if (framed) {
			good=(good && size==2 && chunk[0]==1 &&
							chunk[1]=='C');
			chars++;
		} else {
			for (unsigned char i=0; i<size; i++) {
				good=(good && chunk[i]=='C');
			}
			chars+=size;
			good=(good && (size==255 || chars==400));
		}
	}
	stdoutput.printf("    characters: %d\n",(int)chars);
	charstring::printf(l,sizeof(l),"%s: the 400 characters are "
					"sent the way a real server "
					"sends them",label);
	report(l,good && chars==400);

	// then 08, the locator as it came and the amount, 400
	unsigned char	okcode=0;
	unsigned char	echoed[LOCATOR_SIZE];
	charstring::printf(l,sizeof(l),"%s: the locator and the amount "
					"follow",label);
	ok=(client->readByte(&okcode) && okcode==ORA_TTC_OK &&
		client->readBytes(echoed,sizeof(echoed)) &&
		!bytestring::compare(echoed,locator,sizeof(locator)) &&
		expectBytes(client,clobreadtail,sizeof(clobreadtail)));
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
	}
}

// the OALL8 9.2.0.4 OCI8 sends for "select level from dual connect by
// level<=40" with no iterations, behind the piggyback's header, and the
// fetch of 15 rows that only fetches, behind its sequence number and the
// cursor id.  [0015] and [0017] of the capture cited above
static const unsigned char	fetchonlyselecttail[]={
	0x02, 0x80, 0x61, 0x00, 0x44, 0x15, 0x06, 0x08,
	0x01, 0x2b, 0xf8, 0xcc, 0x05, 0x08, 0x01, 0x0c,
	0x00, 0x00, 0x00, 0x00, 0x28, 0xcd, 0x05, 0x08,
	0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x2a, 0xcd, 0x05, 0x08, 0xb8, 0x15, 0x06,
	0x08, 0x00, 0x2b,
	's', 'e', 'l', 'e', 'c', 't', ' ', 'l',
	'e', 'v', 'e', 'l', ' ', 'f', 'r', 'o',
	'm', ' ', 'd', 'u', 'a', 'l', ' ', 'c',
	'o', 'n', 'n', 'e', 'c', 't', ' ', 'b',
	'y', ' ', 'l', 'e', 'v', 'e', 'l', '<',
	'=', '4', '0',
	0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x01, 0x00, 0x00, 0x00, 0x00
};
static const unsigned char	fetchonlyhead[]={
	0x01, 0x40, 0x01
};
static const unsigned char	fetchonlytail[]={
	0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xcc, 0x05,
	0x08, 0x01, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x28,
	0xcd, 0x05, 0x08, 0x00, 0x00, 0x01, 0x40, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x2a, 0xcd, 0x05, 0x08,
	0xb8, 0x15, 0x06, 0x08, 0x00, 0x00, 0x01, 0x0f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00,
	0x00, 0x00, 0x00
};

static void runFetchOnly(oracleprotocolclient *client) {

	const char	*label="fetch only";
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
	client->appendBytes(fetchonlyselecttail,sizeof(fetchonlyselecttail));
	char	l[256];
	charstring::printf(l,sizeof(l),"%s: execute with no iterations",label);
	bool	ok=(client->sendPacket() && client->recvPacket());
	report(l,ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client->getError());
		return;
	}

	// the cursor id is the third al8o4 value, behind the TTC_OK
	static const unsigned char	al8o4head[]={
		ORA_TTC_OK, 0x01, 0x06, 0x00, 0x00, 0x01
	};
	const unsigned char	*response=client->getResponse();
	size_t			responsesize=client->getResponseSize();
	unsigned char		cursorid=0;
	for (size_t i=0; i+sizeof(al8o4head)<responsesize; i++) {
		if (!bytestring::compare(response+i,al8o4head,
						sizeof(al8o4head))) {
			cursorid=response[i+sizeof(al8o4head)];
			break;
		}
	}
	charstring::printf(l,sizeof(l),"%s: the answer carries a cursor id",
									label);
	report(l,cursorid!=0);
	if (!cursorid) {
		dumpResponse(client);
		return;
	}

	// the fetch.  it names 15 rows in the al8i4 vector
	client->beginTtiCall(ORA_TTI_QUERY3);
	client->appendByte(seqnumber++);
	client->appendBytes(fetchonlyhead,sizeof(fetchonlyhead));
	client->appendByte(cursorid);
	client->appendBytes(fetchonlytail,sizeof(fetchonlytail));
	charstring::printf(l,sizeof(l),"%s: fetch 15 rows",label);
	ok=(client->sendPacket() && client->recvPacket());
	report(l,ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client->getError());
		return;
	}

	// a real server sends all 15, in one row header that says so, the
	// rows being 2 through 16 because the execute sent the first
	static const unsigned char	rowheader[]={
		ORA_TTC_ROW_HEADER,
		0x02, 0x01, 0x01, 0x00, 0x01, 0x0f, 0x00, 0x00, 0x00
	};
	unsigned char	dataflags[2];
	client->rewindResponse();
	charstring::printf(l,sizeof(l),"%s: the row header says 15 rows",label);
	ok=(client->readBytes(dataflags,sizeof(dataflags)) &&
		expectBytes(client,rowheader,sizeof(rowheader)));
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
		return;
	}
	bool	rowsok=true;
	for (unsigned char i=3; i<=17 && rowsok; i++) {
		const unsigned char	row[]={
			ORA_TTC_ROW_DATA, 0x02, 0xc1, i
		};
		rowsok=expectBytes(client,row,sizeof(row));
	}
	charstring::printf(l,sizeof(l),"%s: 15 rows follow, 2 through 16",
									label);
	report(l,rowsok);
	if (!rowsok) {
		dumpResponse(client);
		return;
	}

	// then the return parameters
	unsigned char	okcode=0;
	charstring::printf(l,sizeof(l),"%s: the return parameters follow",
									label);
	report(l,client->readByte(&okcode) && okcode==ORA_TTC_OK);
}

// the fixed part of a pre-10g lob operation request, behind its sequence
// number: the pointers are the client's own and mean nothing here but
// whether they are null.  [0017] and the ones after it of the capture cited
// above
static void beginLegacyLobOperation(oracleprotocolclient *client,
					const unsigned char *source,
					uint32_t sourcesize,
					const unsigned char *dest,
					uint32_t destsize,
					uint32_t sourceoffset,
					uint32_t destoffset,
					bool amountpointer,
					uint32_t operation) {
	static const unsigned char	pointer[]={0x30, 0x18, 0x06, 0x08};
	static const unsigned char	amountptr[]={0x8c, 0xef, 0xff, 0xbf};
	static const unsigned char	null[]={0x00, 0x00, 0x00, 0x00};
	client->beginTtiCall(ORA_TTI_LOB_OPERATIONS);
	client->appendByte(seqnumber++);
	client->appendBytes(pointer,sizeof(pointer));
	client->appendLenPreInt(sourcesize);
	client->appendBytes((dest)?pointer:null,sizeof(null));
	client->appendLenPreInt(destsize);
	client->appendLenPreInt(sourceoffset);
	client->appendLenPreInt(destoffset);
	client->appendBytes(null,sizeof(null));
	client->appendBytes((amountpointer)?amountptr:null,sizeof(null));
	client->appendBytes(null,sizeof(null));
	client->appendLenPreInt(operation);
	client->appendBytes(null,sizeof(null));
	client->appendLenPreInt(0);
	if (source) {
		client->appendBytes(source,sourcesize);
	}
	if (dest) {
		client->appendBytes(dest,destsize);
	}
}

// the answer to a lob operation that quotes a locator: the locator back, and
// the amount
static bool expectLobAnswer(oracleprotocolclient *client,
				const unsigned char *locator,
				uint32_t locatorsize,
				uint32_t amount) {
	unsigned char	echoed[64];
	unsigned char	okcode=0;
	uint32_t	value=0;
	client->rewindResponse();
	unsigned char	dataflags[2];
	return (locatorsize<=sizeof(echoed) &&
		client->readBytes(dataflags,sizeof(dataflags)) &&
		client->readByte(&okcode) && okcode==ORA_TTC_OK &&
		client->readBytes(echoed,locatorsize) &&
		!bytestring::compare(echoed,locator,locatorsize) &&
		client->readLenPreInt(&value) && value==amount);
}

static void runTempLobs(oracleprotocolclient *client, bool framed) {

	const char	*label=(framed)?"temporary clob, multibyte database":
					"temporary clob, single-byte database";
	stdoutput.printf("\n%s\n",label);
	char	l[256];

	// create: a blank 86 byte locator, a duration of 10, a clob (csfrm 1,
	// type 0x70), and the character set, the duration and a 0 behind it
	unsigned char	blank[86];
	bytestring::zero(blank,sizeof(blank));
	blank[1]=0x54;
	beginLegacyLobOperation(client,blank,sizeof(blank),NULL,10,1,0x70,
					true,0x110);
	client->appendLenPreInt(31);
	client->appendLenPreInt(10);
	client->appendLenPreInt(0);
	charstring::printf(l,sizeof(l),"%s: create",label);
	bool	ok=(client->sendPacket() && client->recvPacket());
	report(l,ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client->getError());
		return;
	}

	// the locator is in front of the zeros that fill the buffer, which
	// is 86 bytes in all, and a clob's character set and a 1 follow it
	unsigned char	dataflags[2];
	unsigned char	okcode=0;
	unsigned char	buffer[86];
	uint32_t	csid=0;
	uint32_t	one=0;
	client->rewindResponse();
	charstring::printf(l,sizeof(l),"%s: the buffer comes back with a "
					"locator in it, then the character "
					"set and a 1",label);
	ok=(client->readBytes(dataflags,sizeof(dataflags)) &&
		client->readByte(&okcode) && okcode==ORA_TTC_OK &&
		client->readBytes(buffer,sizeof(buffer)) &&
		client->readLenPreInt(&csid) && csid==((framed)?873:31) &&
		client->readLenPreInt(&one) && one==1);
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
		return;
	}
	unsigned char	locator[40];
	bytestring::copy(locator,buffer,sizeof(locator));
	charstring::printf(l,sizeof(l),"%s: the rest of the buffer is "
						"zeros",label);
	bool	zeros=true;
	for (size_t i=sizeof(locator); i<sizeof(buffer); i++) {
		zeros=(zeros && !buffer[i]);
	}
	report(l,zeros);

	// the locator says how the characters are sent
	charstring::printf(l,sizeof(l),"%s: the locator says how the "
					"characters are sent",label);
	report(l,((locator[6]&0x80)!=0)==framed);

	// write 10 characters at 1
	if (framed) {
		beginLegacyLobOperation(client,locator,sizeof(locator),NULL,0,
						1,0,true,0x40);
		client->appendLenPreInt(10);
		client->appendByte(ORA_TTC_LOB_DATA);
		client->appendByte(0xfe);
		for (const char *c="0123456789"; *c; c++) {
			client->appendByte(2);
			client->appendByte(1);
			client->appendByte((unsigned char)*c);
		}
		client->appendByte(0);
	} else {
		beginLegacyLobOperation(client,locator,sizeof(locator),NULL,0,
						1,0,true,0x40);
		client->appendLenPreInt(10);
		client->appendByte(ORA_TTC_LOB_DATA);
		client->appendByte(10);
		client->appendBytes((const unsigned char *)"0123456789",10);
	}
	charstring::printf(l,sizeof(l),"%s: write",label);
	ok=(client->sendPacket() && client->recvPacket() &&
		expectLobAnswer(client,locator,sizeof(locator),10));
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
		return;
	}

	// the length
	beginLegacyLobOperation(client,locator,sizeof(locator),NULL,0,0,0,
								true,1);
	client->appendLenPreInt(0);
	charstring::printf(l,sizeof(l),"%s: get length",label);
	ok=(client->sendPacket() && client->recvPacket() &&
		expectLobAnswer(client,locator,sizeof(locator),10));
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
		return;
	}

	// erase 3 characters at 2
	beginLegacyLobOperation(client,locator,sizeof(locator),NULL,0,2,0,
								true,8);
	client->appendLenPreInt(3);
	charstring::printf(l,sizeof(l),"%s: erase",label);
	ok=(client->sendPacket() && client->recvPacket() &&
		expectLobAnswer(client,locator,sizeof(locator),3));
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
		return;
	}

	// read it all: 0, three spaces and 456789
	beginLegacyLobOperation(client,locator,sizeof(locator),NULL,0,1,0,
								true,2);
	client->appendLenPreInt(10);
	charstring::printf(l,sizeof(l),"%s: read",label);
	ok=(client->sendPacket() && client->recvPacket());
	report(l,ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client->getError());
		return;
	}
	const char	*expected="0   456789";
	unsigned char	marker=0;
	unsigned char	chunk[256];
	size_t		chars=0;
	bool		good=true;
	unsigned char	size=0;
	client->rewindResponse();
	charstring::printf(l,sizeof(l),"%s: the characters are sent the way "
					"a real server sends them",label);
	good=(client->readBytes(dataflags,sizeof(dataflags)) &&
		client->readByte(&okcode) && okcode==ORA_TTC_LOB_DATA &&
		client->readByte(&marker) && marker==0xfe);
	while (good) {
		if (!client->readByte(&size)) {
			good=false;
			break;
		}
		if (!size) {
			break;
		}
		if (!client->readBytes(chunk,size)) {
			good=false;
			break;
		}
		if (framed) {
			good=(good && size==2 && chunk[0]==1 && chars<10 &&
						chunk[1]==expected[chars]);
			chars++;
		} else {
			for (unsigned char i=0; i<size; i++) {
				good=(good && chars<10 &&
						chunk[i]==expected[chars]);
				chars++;
			}
		}
	}
	report(l,good && chars==10);
	if (!good || chars!=10) {
		dumpResponse(client);
		return;
	}
	charstring::printf(l,sizeof(l),"%s: the locator and the amount "
					"follow the characters",label);
	unsigned char	okcode2=0;
	unsigned char	echoed[40];
	uint32_t	amount=0;
	ok=(client->readByte(&okcode2) && okcode2==ORA_TTC_OK &&
		client->readBytes(echoed,sizeof(echoed)) &&
		!bytestring::compare(echoed,locator,sizeof(locator)) &&
		client->readLenPreInt(&amount) && amount==10);
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
		return;
	}

	// free
	beginLegacyLobOperation(client,locator,sizeof(locator),NULL,0,0,0,
								false,0x111);
	charstring::printf(l,sizeof(l),"%s: free",label);
	ok=(client->sendPacket() && client->recvPacket());
	report(l,ok);
	if (!ok) {
		stdoutput.printf("    %s\n",client->getError());
		return;
	}
	client->rewindResponse();
	unsigned char	freed[40];
	charstring::printf(l,sizeof(l),"%s: the locator comes back freed",
									label);
	ok=(client->readBytes(dataflags,sizeof(dataflags)) &&
		client->readByte(&okcode) && okcode==ORA_TTC_OK &&
		client->readBytes(freed,sizeof(freed)) &&
		freed[5]==0 && freed[4]==locator[4]);
	report(l,ok);
	if (!ok) {
		dumpResponse(client);
	}
}

int main(int argc, char **argv) {

	bool	at102=(argc>1 && !charstring::compare(argv[1],"10.2"));
	bool	singlebyte=(argc>1 && !charstring::compare(argv[1],
							"singlebyte"));

	stdoutput.printf("\n====== #10538 9i sqlplus bootstrap and #10702 "
				"clob read (%s) ======\n",
				(singlebyte)?"single-byte database":
				(at102)?"serverversion 10.2":
					"serverversion 12.1");

	const char	*portvariable=(singlebyte)?"ORACLEPROTOCOLPORT7":
					(at102)?"ORACLEPROTOCOLPORT12":
						"ORACLEPROTOCOLPORT1";
	uint16_t	port=(singlebyte)?1527:(at102)?1532:1521;
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

	if (!singlebyte) {
		runSelect(&client,"sqlplus bootstrap",0x00);
		runSelect(&client,"indicator",DEFINE_INDICATOR);
		runSelect(&client,"indicator, return length and return code",
									0x07);
		runOall8(&client);
		runFetchOnly(&client);
	}
	runClobRead(&client,!singlebyte);
	runTempLobs(&client,!singlebyte);

	client.disconnect();

	if (status==0) {
		stdoutput.printf("\n\033[34mAll tests succeeded\033[0m\n");
	} else {
		stdoutput.printf("\n\033[38;5;208mSome tests failed\033[0m\n");
	}

	return status;
}
