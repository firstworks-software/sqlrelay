// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclient.h>
#include <rudiments/charstring.h>
#include <rudiments/process.h>
#include <rudiments/stdio.h>

#include "asserts.cpp"

sqlrconnection	*con=NULL;
sqlrcursor	*cur=NULL;
sqlrconnection	*secondcon=NULL;
sqlrcursor	*secondcur=NULL;

// utf-8
const byte_t yo8[]={'y','o','\0'};
const byte_t grin8[]={0xF0,0x9F,0x98,0x84,'\0'};
const byte_t horn8[]={0xF0,0x9F,0x91,0xBF,'\0'};
const byte_t cool8[]={0xF0,0x9F,0x98,0x8E,'\0'};
const byte_t *emoji8[]={yo8,grin8,horn8,cool8,NULL};

// utf-16
const byte_t yo16[]={'y','\0','o','\0','\0','\0'};
const byte_t grin16[]={0x3D,0xD8,0x04,0xDE,'\0','\0'};
const byte_t horn16[]={0x3D,0xD8,0x7F,0xDC,'\0','\0'};
const byte_t cool16[]={0x3D,0xD8,0x0E,0xDE,'\0','\0'};
const byte_t *emoji16[]={yo16,grin16,horn16,cool16,NULL};


int main(int argc, char **argv) {

	// instantiation
	con=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);


	// create temptable
	stdoutput.printf("CREATE TEMPTABLE: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	i int identity, "
		"	emojidirect nvarchar(64), "
		"	emojifrombase64 nvarchar(64), "
		"	base64 varchar(64))"));
	stdoutput.printf("\n\n");


	// insert
	stdoutput.printf("INSERT: \n");
	cur->prepareQuery("insert into testtable values (:1,null,:2)");
	const byte_t **e16=emoji16;
	const byte_t **e8=emoji8;
	for (; *e16; e16++,e8++) {

		cur->inputBind("1",(const char *)*e8);

		// FIXME: should use an encoding-aware len()
		// function instead of hardcoding to 6
		char	*b64e=charstring::base64Encode(*e16,6);
		cur->inputBind("2",b64e);
		assertTrue(cur->executeQuery());
		delete[] b64e;
	}
	stdoutput.printf("\n\n");


	// update
	stdoutput.printf("UPDATE: \n");
	assertTrue(cur->sendQuery(
		"update testtable set "
		"	emojifrombase64=cast(cast(N'' as xml).value('xs:base64Binary(sql:column(\"base64\"))','VARBINARY(MAX)') AS NVARCHAR(MAX))"));
	stdoutput.printf("\n\n");


	// select
	stdoutput.printf("SELECT: \n");
	assertTrue(cur->sendQuery("select * from testtable"));
	stdoutput.printf("\n");
	uint64_t	row=0;
	for (const byte_t **e=emoji8; *e; e++) {
		assertEquals(cur->getField(row,"emojidirect"),
							(const char *)*e);
		assertEquals(cur->getField(row,"emojifrombase64"),
							(const char *)*e);
		row++;
	}
	stdoutput.printf("\n\n");


	// output bind
	stdoutput.printf("OUTPUT BIND: \n");
	row=1;
	for (const byte_t **e=emoji8; *e; e++) {

		cur->prepareQuery(
			"set :output=(select emojidirect "
			"	from testtable where i=$(row))");
		cur->substitution("row",row);
		cur->defineOutputBindString("output",100);
		assertTrue(cur->executeQuery());
		assertEquals(cur->getOutputBindString("output"),
						(const char *)*e);

		cur->prepareQuery(
			"set :output=(select emojifrombase64 "
			"	from testtable where i=$(row))");
		cur->substitution("row",row);
		cur->defineOutputBindString("output",100);
		assertTrue(cur->executeQuery());
		assertEquals(cur->getOutputBindString("output"),
						(const char *)*e);
		row++;
	}
	stdoutput.printf("\n\n");

	delete cur;
	delete con;

	reportTestStatus();

	return status;
}
