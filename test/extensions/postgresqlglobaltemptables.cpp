// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/process.h>
#include <rudiments/datetime.h>
#include <rudiments/signalclasses.h>
#include <sqlrelay/sqlrclient.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../c++/asserts.cpp"

sqlrconnection	*con=NULL;
sqlrcursor	*cur=NULL;
sqlrconnection	*secondcon=NULL;
sqlrcursor	*secondcur=NULL;

int main(int argc, char **argv) {

	stdoutput.printf("GLOBALTEMPTABLES:\n");
	con=new sqlrconnection("sqlrelay",9022,"/tmp/postgresqlglobaltemptables.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);

	stdoutput.printf("LAZY CREATE ON FIRST INSERT:\n");
	assertTrue(cur->sendQuery("insert into gtttest1 values (1,'one')"));
	assertTrue(cur->sendQuery("insert into gtttest1 values (2,'two')"));
	assertTrue(cur->sendQuery("select count(*) from gtttest1"));
	assertEquals(cur->getField(0,(uint32_t)0),"2");
	stdoutput.printf("\n");


	stdoutput.printf("SUBSEQUENT INSERTS:\n");
	assertTrue(cur->sendQuery("insert into gtttest1 values (3,'three')"));
	assertTrue(cur->sendQuery("insert into gtttest1 values (4,'four')"));
	assertTrue(cur->sendQuery("select count(*) from gtttest1"));
	assertEquals(cur->getField(0,(uint32_t)0),"4");
	stdoutput.printf("\n");


	stdoutput.printf("INSERT WITH LEADING COMMENT:\n");
	assertTrue(cur->sendQuery(
		"-- leading comment\n"
		"insert into gtttest2 values (1,'one')"));
	assertTrue(cur->sendQuery("select count(*) from gtttest2"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");


	stdoutput.printf("UNCONFIGURED TABLE STILL FAILS:\n");
	assertFalse(cur->sendQuery("insert into nonexistent values (1,'x')"));
	stdoutput.printf("\n");


	stdoutput.printf("ALREADY-EXISTS HANDLING (PRE-EXISTING TABLE):\n");
	assertTrue(cur->sendQuery("drop table gtttest2"));
	assertTrue(cur->sendQuery("create temp table gtttest2 ("
					"id int primary key, "
					"value varchar(20) "
					")"));
	assertTrue(cur->sendQuery("insert into gtttest2 values (2,'two')"));
	assertTrue(cur->sendQuery("select count(*) from gtttest2"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");


	stdoutput.printf("TABLES DROPPED AT END-OF-SESSION:\n");
	con->endSession();
	assertFalse(cur->sendQuery("select * from gtttest1"));
	assertFalse(cur->sendQuery("select * from gtttest2"));
	stdoutput.printf("\n");

	delete cur;
	delete con;
	stdoutput.printf("\n");

	reportTestStatus();

	return status;
}
