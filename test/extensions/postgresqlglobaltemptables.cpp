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


	stdoutput.printf("LAZY CREATE ON FIRST DELETE:\n");
	assertTrue(cur->sendQuery("delete from gtttest3 where id=1"));
	assertTrue(cur->sendQuery("select count(*) from gtttest3"));
	assertEquals(cur->getField(0,(uint32_t)0),"0");
	stdoutput.printf("\n");


	stdoutput.printf("LAZY CREATE ON FIRST SELECT:\n");
	assertTrue(cur->sendQuery("select * from gtttest4"));
	assertEquals(cur->rowCount(),0);
	stdoutput.printf("\n");


	stdoutput.printf("LAZY CREATE ON FIRST UPDATE:\n");
	assertTrue(cur->sendQuery("update gtttest5 set value='x' where id=1"));
	assertEquals(cur->affectedRows(),0);
	stdoutput.printf("\n");


	stdoutput.printf("LAZY CREATE ON FIRST REFERENCE IN JOIN:\n");
	assertTrue(cur->sendQuery("select gtttest1.id from gtttest1 "
			"left join gtttest6 on gtttest1.id=gtttest6.id"));
	assertTrue(cur->sendQuery("select count(*) from gtttest6"));
	assertEquals(cur->getField(0,(uint32_t)0),"0");
	stdoutput.printf("\n");


	stdoutput.printf("LAZY CREATE ON FIRST REFERENCE IN SUBQUERY:\n");
	assertTrue(cur->sendQuery("select gtttest1.id from gtttest1 "
			"where gtttest1.id in (select id from gtttest7)"));
	assertTrue(cur->sendQuery("select count(*) from gtttest7"));
	assertEquals(cur->getField(0,(uint32_t)0),"0");
	stdoutput.printf("\n");


	stdoutput.printf("TABLES RE-CREATED AFTER END-OF-SESSION:\n");
	con->endSession();
	assertTrue(cur->sendQuery("select * from gtttest1"));
	assertEquals(cur->rowCount(),0);
	assertTrue(cur->sendQuery("select * from gtttest2"));
	assertEquals(cur->rowCount(),0);
	stdoutput.printf("\n");


	// The instance under test is configured with connections="1"
	// maxconnections="1", so every session below - and every session
	// above, for that matter - is served by the same single pooled
	// backend connection.  That's what exercises the bug fixed by
	// #9470: the trigger's per-table "created" flags live on that one
	// pooled connection and outlive any single client session, but each
	// postgresql "create temp table" is scoped to the backend session
	// that issued it.  gtttest3 was already created earlier in this
	// file (its "created" flag is already set), so before the fix,
	// every iteration below - including the first - would fail with
	// "relation ... does not exist": postgresql dropped the table when
	// the earlier session ended, but the flag never got cleared to
	// say so.  Every test above this point runs within a single
	// session, which is why the bug wasn't caught by them.
	stdoutput.printf("TABLES RE-CREATED ACROSS MORE CLIENT SESSIONS "
				"THAN THERE ARE POOLED CONNECTIONS:\n");
	for (uint16_t session=0; session<3; session++) {
		con->endSession();
		assertTrue(cur->sendQuery(
				"insert into gtttest3 values (1,'one')"));
		assertTrue(cur->sendQuery("select count(*) from gtttest3"));
		assertEquals(cur->getField(0,(uint32_t)0),"1");
		assertTrue(cur->sendQuery("delete from gtttest3"));
	}
	stdoutput.printf("\n");


	stdoutput.printf("TABLES RE-CREATED FOR A SEPARATE CLIENT "
				"CONNECTION SHARING THE SAME REUSED POOLED "
				"CONNECTION:\n");
	con->endSession();
	secondcon=new sqlrconnection("sqlrelay",9022,
			"/tmp/postgresqlglobaltemptables.socket",
			"testuser","testpassword",0,1);
	secondcur=new sqlrcursor(secondcon);
	assertTrue(secondcur->sendQuery(
				"insert into gtttest4 values (1,'one')"));
	assertTrue(secondcur->sendQuery("select count(*) from gtttest4"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	delete secondcur;
	delete secondcon;
	secondcur=NULL;
	secondcon=NULL;
	stdoutput.printf("\n");

	delete cur;
	delete con;
	stdoutput.printf("\n");

	reportTestStatus();

	return status;
}
