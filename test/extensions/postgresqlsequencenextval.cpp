// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/process.h>
#include <sqlrelay/sqlrclient.h>
#include <stdlib.h>
#include <stdio.h>

#include "../c++/asserts.cpp"

sqlrconnection	*con=NULL;
sqlrcursor	*cur=NULL;
sqlrconnection	*secondcon=NULL;
sqlrcursor	*secondcur=NULL;

int main(int argc, char **argv) {

	stdoutput.printf("SEQUENCE NEXTVAL:\n");

	con=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);


	// create test sequences
	cur->sendQuery("drop sequence seqa");
	cur->sendQuery("drop sequence seqb");
	assertTrue(cur->sendQuery("create sequence seqa"));
	assertTrue(cur->sendQuery("create sequence seqb"));
	stdoutput.printf("\n");


	// the module is configured with style="nextval()", so .nextval and
	// "next value for" queries get rewritten to nextval(), which is what
	// postgresql understands

	stdoutput.printf("DOT NEXTVAL:\n");
	assertTrue(cur->sendQuery("select seqa.nextval"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertTrue(cur->sendQuery("select seqa.nextval"));
	assertEquals(cur->getField(0,(uint32_t)0),"2");
	stdoutput.printf("\n");


	stdoutput.printf("NEXT VALUE FOR:\n");
	assertTrue(cur->sendQuery("select next value for seqa"));
	assertEquals(cur->getField(0,(uint32_t)0),"3");
	stdoutput.printf("\n");


	stdoutput.printf("NEXTVAL PASSTHROUGH:\n");
	assertTrue(cur->sendQuery("select nextval('seqa')"));
	assertEquals(cur->getField(0,(uint32_t)0),"4");
	stdoutput.printf("\n");


	stdoutput.printf("SCHEMA-QUALIFIED DOT NEXTVAL:\n");
	assertTrue(cur->sendQuery("select public.seqb.nextval"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");


	stdoutput.printf("MULTIPLE STYLES IN ONE QUERY:\n");
	assertTrue(cur->sendQuery(
		"select seqa.nextval,next value for seqb"));
	assertEquals(cur->getField(0,(uint32_t)0),"5");
	assertEquals(cur->getField(0,1),"2");
	stdoutput.printf("\n");


	stdoutput.printf("TOKENS INSIDE STRING LITERAL:\n");
	assertTrue(cur->sendQuery("select 'seqa.nextval' as lit"));
	assertEquals(cur->getField(0,(uint32_t)0),"seqa.nextval");
	assertTrue(cur->sendQuery("select 'next value for seqb' as lit"));
	assertEquals(cur->getField(0,(uint32_t)0),"next value for seqb");
	stdoutput.printf("\n");


	// clean up
	assertTrue(cur->sendQuery("drop sequence seqa"));
	assertTrue(cur->sendQuery("drop sequence seqb"));
	delete cur;
	delete con;
	stdoutput.printf("\n");

	reportTestStatus();

	return status;
}
