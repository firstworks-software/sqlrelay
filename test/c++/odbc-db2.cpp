// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclient.h>
#include <rudiments/charstring.h>
#include <rudiments/stdio.h>

#include "asserts.cpp"

sqlrconnection	*con=NULL;
sqlrcursor	*cur=NULL;
sqlrconnection	*secondcon=NULL;
sqlrcursor	*secondcur=NULL;

// exercises odbc.cpp's getLobFieldSegment (#10038) - identical code to
// informix.cpp's, and informix itself isn't reachable on every dev host.
// db2, reached here through the odbc connection module rather than db2's
// own, gives that shared code path a live backend to run against instead.
int main(int argc, char **argv) {

	#define	LARGE_BUFFER_LENGTH	(20*1024)
	char		largebuffer[LARGE_BUFFER_LENGTH+1];

	// character count; the utf-8 encoded bind buffer is twice this
	// many bytes.  db2's long varchar type tops out at 32700 bytes, so
	// this has to stay under half that, while still spanning multiple
	// of getLobFieldSegment's 8192-character fetch chunks
	#define	MULTIBYTE_BUFFER_LENGTH	16000
	char		multibytebuffer[MULTIBYTE_BUFFER_LENGTH*2+1];


	// instantiation
	con=new sqlrconnection("sqlrelay",9013,"/tmp/odbc-db2.socket",
						"db2inst1","testpassword",0,1);
	cur=new sqlrcursor(con);


	// identify
	stdoutput.printf("IDENTIFY: \n");
	assertEquals(con->identify(),"odbc");
	stdoutput.printf("\n");


	// ping
	stdoutput.printf("PING: \n");
	assertTrue(con->ping());
	stdoutput.printf("\n");


	// null and empty lobs
	//
	// long varchar, not clob - see the LONG LOBS comment below for why
	stdoutput.printf("NULL AND EMPTY LOBS: \n");
	cur->getNullsAsNulls();
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testclob1 long varchar, "
		"	testclob2 long varchar)"));
	cur->prepareQuery(
		"insert into "
		"	testtable "
		"values ("
		"	?, "
		"	?)");
	cur->inputBindClob("1","",0);
	cur->inputBindClob("2",NULL,0);
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("select * from testtable"));
	assertEquals(cur->getField(0,(uint32_t)0),"");
	assertEquals(cur->getField(0,1),NULL);
	cur->getNullsAsEmptyStrings();
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// long lobs - big enough to force multiple top-level
	// getLobFieldSegment calls.  sqlrprotocol_sqlrclient::sendLobField
	// (src/protocols/sqlrclient.cpp) requests 8192 chars per call, so a
	// 20K lob forces three: 8192, 8192, 4096
	//
	// long varchar / long varchar for bit data, not clob/blob.  DB2 clob
	// columns report as SQL_CLOB over odbc, which
	// odbccursor::isLob() (src/connections/odbc.cpp) doesn't recognize
	// as a lob type, so they take the ordinary non-lob fetch path and
	// never reach getLobFieldSegment at all.  long varchar and long
	// varchar for bit data report as SQL_LONGVARCHAR/SQL_LONGVARBINARY,
	// which isLob() does recognize, so these are what actually exercise
	// the code this file is after.
	stdoutput.printf("LONG LOBS: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testclob long varchar, "
		"	testblob long varchar for bit data)"));
	cur->prepareQuery("insert into testtable values (?,?)");
	// byte-position-dependent, not a uniform fill, so an overlapping or
	// duplicated segment read (not just truncation) would also mismatch
	for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='A'+(i%26);
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	cur->inputBindClob("1",largebuffer,LARGE_BUFFER_LENGTH);
	cur->inputBindBlob("2",largebuffer,LARGE_BUFFER_LENGTH);
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("select * from testtable"));
	assertEquals(cur->getFieldLength(0,(uint32_t)0),LARGE_BUFFER_LENGTH);
	assertEquals(cur->getField(0,(uint32_t)0),largebuffer);
	assertEquals(cur->getFieldLength(0,1),LARGE_BUFFER_LENGTH);
	assertEquals(cur->getField(0,1),largebuffer,LARGE_BUFFER_LENGTH);
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// multibyte lobs (#10040) - long varchar, not clob, for the same
	// reason as LONG LOBS above.  odbc.cpp's getLobFieldSegment only
	// uses offset for bounds checking, not as a position argument to a
	// db2 api call the way db2.cpp's does, so this isn't proving that
	// fix itself - it gives the odbc connection module the same
	// multi-chunk utf-8 coverage #10038 already gave it for
	// single-byte data
	stdoutput.printf("MULTIBYTE LOBS: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	testclob long varchar)"));
	cur->prepareQuery("insert into testtable values (?)");
	// cycle through 95 distinct two-byte utf-8 characters
	// (u+00a1-u+00ff), byte-position-dependent so an overlapping or
	// duplicated segment read would also mismatch
	for (int i=0; i<MULTIBYTE_BUFFER_LENGTH; i++) {
		uint16_t	codepoint=0xa1+(i%95);
		multibytebuffer[i*2]=(char)(0xc0|(codepoint>>6));
		multibytebuffer[i*2+1]=(char)(0x80|(codepoint&0x3f));
	}
	multibytebuffer[MULTIBYTE_BUFFER_LENGTH*2]='\0';
	cur->inputBindClob("1",multibytebuffer,MULTIBYTE_BUFFER_LENGTH*2);
	assertTrue(cur->executeQuery());
	assertTrue(cur->sendQuery("select * from testtable"));
	assertEquals(cur->getFieldLength(0,(uint32_t)0),
					MULTIBYTE_BUFFER_LENGTH*2);
	assertEquals(cur->getField(0,(uint32_t)0),multibytebuffer,
					MULTIBYTE_BUFFER_LENGTH*2);
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// long lobs, small maxfieldsize - the default maxfieldsize (32768)
	// is bigger than the 20K value above, so an ordinary bound fetch
	// would round-trip it without ever forcing getLobFieldSegment to
	// run, proving nothing about that code path specifically.  this
	// connects to a second instance whose maxfieldsize is reduced to
	// 1024, well under the value's length, where only the chunked lob
	// path can bring it back intact (#10038)
	stdoutput.printf("LONG LOBS, SMALL MAXFIELDSIZE: \n");
	secondcon=new sqlrconnection("sqlrelay",9038,
					"/tmp/odbcdb2maxfieldsize.socket",
						"db2inst1","testpassword",0,1);
	secondcur=new sqlrcursor(secondcon);
	secondcur->sendQuery("drop table testtable");
	assertTrue(secondcur->sendQuery(
		"create table testtable ("
		"	testclob long varchar, "
		"	testblob long varchar for bit data)"));
	secondcur->prepareQuery("insert into testtable values (?,?)");
	secondcur->inputBindClob("1",largebuffer,LARGE_BUFFER_LENGTH);
	secondcur->inputBindBlob("2",largebuffer,LARGE_BUFFER_LENGTH);
	assertTrue(secondcur->executeQuery());
	assertTrue(secondcur->sendQuery("select * from testtable"));
	assertEquals(secondcur->getFieldLength(0,(uint32_t)0),
							LARGE_BUFFER_LENGTH);
	assertEquals(secondcur->getField(0,(uint32_t)0),largebuffer);
	assertEquals(secondcur->getFieldLength(0,1),LARGE_BUFFER_LENGTH);
	assertEquals(secondcur->getField(0,1),largebuffer,
							LARGE_BUFFER_LENGTH);
	assertTrue(secondcur->sendQuery("drop table testtable"));
	delete secondcur;
	secondcur=NULL;
	delete secondcon;
	secondcon=NULL;
	stdoutput.printf("\n");


	reportTestStatus();

	return status;
}
