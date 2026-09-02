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
char		*backendpid=NULL;

// The instance under test is configured with connections="1"
// maxconnections="1", so every session in this file is expected to be served
// by the same single pooled backend connection.  That's what exercises the
// bug this file targets: the trigger's per-table "created" flags live on that
// one pooled connection and outlive any single client session.  Compare the
// backend process id after each end-of-session to prove that assumption.
void assertSameBackend(sqlrcursor *c) {
	assertTrue(c->sendQuery("select pg_backend_pid()"));
	assertEquals(c->getField(0,(uint32_t)0),backendpid);
}

int main(int argc, char **argv) {

	stdoutput.printf("GLOBALTEMPTABLES:\n");
	con=new sqlrconnection("sqlrelay",9022,"/tmp/postgresqlglobaltemptables.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);

	// capture the backend process id
	assertTrue(cur->sendQuery("select pg_backend_pid()"));
	backendpid=charstring::duplicate(cur->getField(0,(uint32_t)0));

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
	assertTrue(cur->sendQuery("select count(*) from gtttest1"));
	assertEquals(cur->getField(0,(uint32_t)0),"4");
	con->endSession();
	assertSameBackend(cur);
	assertTrue(cur->sendQuery("select * from gtttest1"));
	assertEquals(cur->rowCount(),0);
	assertTrue(cur->sendQuery("select * from gtttest2"));
	assertEquals(cur->rowCount(),0);
	stdoutput.printf("\n");


	// gtttest13-16 aren't in the trigger's table list.  The client
	// creates them directly, exercising sql relay's own create-temp-table
	// tracking (checkForTempTable() in src/server/sqlrservercursor.cpp),
	// which registers every table it sees created for automatic drop at
	// end-of-session, whether a trigger manages it or not.  Nothing
	// re-creates these, so if the table name was captured correctly then
	// the table is gone after end-of-session and the select fails.
	// (trac #9625)

	// This instance runs the normalize translation, which lowercases the
	// query outside of quoted strings before sql relay looks for a
	// create-temp-table.  So the next two cases prove that a mixed-case or
	// uppercase create still gets its table registered for drop when a
	// case-normalizing translation is in the pipeline - they don't
	// exercise the case-insensitive pattern itself, which never sees
	// anything but lowercase here.  test/c++/postgresql.cpp's "MIXED-CASE
	// TEMPORARY TABLE" case covers that, against an instance that runs no
	// translations.
	stdoutput.printf("MIXED-CASE CREATE TEMP TABLE IF NOT EXISTS "
				"(NORMALIZED):\n");
	assertTrue(cur->sendQuery("Create Temp Table If Not Exists gtttest13 ("
					"id int primary key, "
					"value varchar(20) "
					")"));
	assertTrue(cur->sendQuery("insert into gtttest13 values (1,'one')"));
	assertTrue(cur->sendQuery("select count(*) from gtttest13"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	con->endSession();
	assertSameBackend(cur);
	assertFalse(cur->sendQuery("select count(*) from gtttest13"));
	stdoutput.printf("\n");


	stdoutput.printf("UPPERCASE CREATE TEMP TABLE IF NOT EXISTS "
				"(NORMALIZED):\n");
	assertTrue(cur->sendQuery("CREATE TEMP TABLE IF NOT EXISTS gtttest14 ("
					"id int primary key, "
					"value varchar(20) "
					")"));
	assertTrue(cur->sendQuery("insert into gtttest14 values (1,'one')"));
	assertTrue(cur->sendQuery("select count(*) from gtttest14"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	con->endSession();
	assertSameBackend(cur);
	assertFalse(cur->sendQuery("select count(*) from gtttest14"));
	stdoutput.printf("\n");


	// The table name used to be collected up to the next whitespace
	// character only, so with no space before the column list it came out
	// "gtttest15(id" and the drop at end-of-session silently missed the
	// real table - the create itself succeeded either way, so only the
	// drop below catches this.
	stdoutput.printf("NO SPACE BEFORE THE COLUMN LIST, "
				"IF NOT EXISTS:\n");
	assertTrue(cur->sendQuery("create temp table if not exists gtttest15("
					"id int primary key, "
					"value varchar(20) "
					")"));
	assertTrue(cur->sendQuery("insert into gtttest15 values (1,'one')"));
	assertTrue(cur->sendQuery("select count(*) from gtttest15"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	con->endSession();
	assertSameBackend(cur);
	assertFalse(cur->sendQuery("select count(*) from gtttest15"));
	stdoutput.printf("\n");


	stdoutput.printf("NO SPACE BEFORE THE COLUMN LIST, "
				"NO IF NOT EXISTS:\n");
	assertTrue(cur->sendQuery("create temp table gtttest16("
					"id int primary key, "
					"value varchar(20) "
					")"));
	assertTrue(cur->sendQuery("insert into gtttest16 values (1,'one')"));
	assertTrue(cur->sendQuery("select count(*) from gtttest16"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	con->endSession();
	assertSameBackend(cur);
	assertFalse(cur->sendQuery("select count(*) from gtttest16"));
	stdoutput.printf("\n");


	// Postgresql doesn't drop these tables, sql relay does.  It records
	// every successful "create temp table" it sees and drops those
	// tables itself at end-of-session, before running the triggers'
	// endSession()s.  The trigger's part is just to notice that its
	// "created" flag is stale and re-create the table on next use.
	// No iteration below deletes its row, so if the table weren't
	// actually dropped and re-created, the next insert of id=1 would
	// fail on the primary key.
	stdoutput.printf("TABLES RE-CREATED ACROSS MORE CLIENT SESSIONS "
				"THAN THERE ARE POOLED CONNECTIONS:\n");
	for (uint16_t session=0; session<3; session++) {
		con->endSession();
		assertSameBackend(cur);
		assertTrue(cur->sendQuery(
				"insert into gtttest3 values (1,'one')"));
		assertTrue(cur->sendQuery("select count(*) from gtttest3"));
		assertEquals(cur->getField(0,(uint32_t)0),"1");
	}
	stdoutput.printf("\n");


	stdoutput.printf("TABLES RE-CREATED FOR A SEPARATE CLIENT "
				"CONNECTION SHARING THE SAME REUSED POOLED "
				"CONNECTION:\n");
	// leave a row behind, so the insert below collides on the primary
	// key if the table isn't re-created
	assertTrue(cur->sendQuery("insert into gtttest4 values (1,'one')"));
	assertTrue(cur->sendQuery("select count(*) from gtttest4"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	con->endSession();
	secondcon=new sqlrconnection("sqlrelay",9022,
			"/tmp/postgresqlglobaltemptables.socket",
			"testuser","testpassword",0,1);
	secondcur=new sqlrcursor(secondcon);
	assertSameBackend(secondcur);
	assertTrue(secondcur->sendQuery(
				"insert into gtttest4 values (1,'one')"));
	assertTrue(secondcur->sendQuery("select count(*) from gtttest4"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	delete secondcur;
	delete secondcon;
	secondcur=NULL;
	secondcon=NULL;
	stdoutput.printf("\n");


	stdoutput.printf("TABLES RE-CREATED AFTER END-OF-SESSION FOR A "
				"BOTH-PREPARE TRIGGER:\n");
	assertTrue(cur->sendQuery("insert into gtttest8 values (1,'one')"));
	assertTrue(cur->sendQuery("select count(*) from gtttest8"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	con->endSession();
	assertSameBackend(cur);
	assertTrue(cur->sendQuery("insert into gtttest8 values (1,'one')"));
	assertTrue(cur->sendQuery("select count(*) from gtttest8"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");


	// gtttest9 is created with a plain "create table", which sql relay's
	// create-temp-table pattern doesn't match, so it's never registered
	// for automatic drop.  The table survives end-of-session but the
	// trigger's "created" flag doesn't, so the next reference re-runs
	// the create against a table that's still there.  On postgresql the
	// trigger now inserts "if not exists" into the create automatically,
	// so this succeeds directly rather than failing and falling through
	// the trigger's already-exists handling - that handling still exists
	// and is still exercised on databases where "if not exists" isn't
	// available (eg. Oracle's "create global temporary table").
	stdoutput.printf("ALREADY-EXISTS HANDLING (TABLE SURVIVES "
				"END-OF-SESSION):\n");
	assertTrue(cur->sendQuery("delete from gtttest9"));
	assertTrue(cur->sendQuery("insert into gtttest9 values (1,'one')"));
	assertTrue(cur->sendQuery("select count(*) from gtttest9"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	con->endSession();
	assertSameBackend(cur);
	assertTrue(cur->sendQuery("select count(*) from gtttest9"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	// drop gtttest9 last - nothing may reference it after this, or the
	// trigger would re-create it and leave it behind
	assertTrue(cur->sendQuery("drop table gtttest9"));
	stdoutput.printf("\n");

	// postgresql's "create temp table" is DDL, and DDL is transactional
	// there - a rollback of the transaction it ran in undoes it, same as
	// any other statement.  Before the endTransaction() fix, the
	// trigger's "created" flag had no way to learn that, so the next
	// reference skipped the create and failed against a table that was
	// rolled back out of existence.
	stdoutput.printf("TABLE RE-CREATED AFTER A CLIENT ROLLBACK:\n");
	assertTrue(cur->sendQuery("begin"));
	assertTrue(cur->sendQuery("insert into gtttest10 values (1,'one')"));
	assertTrue(cur->sendQuery("select count(*) from gtttest10"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertTrue(con->rollback());
	assertTrue(cur->sendQuery("select count(*) from gtttest10"));
	assertEquals(cur->getField(0,(uint32_t)0),"0");
	stdoutput.printf("\n");


	// gtttest11's trigger is configured with an error= string that can
	// never match ("this string never matches"), so recognizing the
	// already-exists error below relies entirely on the built-in
	// postgresql sqlstate default (42P07) - this models the ticket's
	// real scenario, a configured locale-specific error= string that's
	// wrong for the host it's actually running on.  The create statement
	// uses "select ... into temp" rather than "create temp table":
	// it has no "table" keyword, so it isn't registered for sql relay's
	// own automatic drop, and postgresql has no "if not exists"
	// equivalent for it, so - unlike gtttest9's plain "create table" -
	// the re-create below genuinely fails with a real already-exists
	// error (at execute time) rather than succeeding outright.
	stdoutput.printf("ALREADY-EXISTS HANDLING VIA SQLSTATE "
				"(LOCALE-INDEPENDENT):\n");
	assertTrue(cur->sendQuery("select count(*) from gtttest11"));
	assertEquals(cur->getField(0,(uint32_t)0),"0");
	con->endSession();
	assertSameBackend(cur);
	assertTrue(cur->sendQuery("select count(*) from gtttest11"));
	assertEquals(cur->getField(0,(uint32_t)0),"0");
	stdoutput.printf("\n");


	// gtttest12's create always fails (undefined function, sqlstate
	// 42883) and never succeeds, so it must never be mistaken for
	// already-exists.  Unlike gtttest11 above, this fails at prepare
	// time rather than execute time, exercising a different
	// sqlstate-capture code path.
	stdoutput.printf("ALREADY-EXISTS HANDLING DOES NOT MASK UNRELATED "
				"ERRORS:\n");
	assertFalse(cur->sendQuery("select count(*) from gtttest12"));
	stdoutput.printf("\n");


	delete cur;
	delete con;
	delete[] backendpid;
	stdoutput.printf("\n");

	reportTestStatus();

	return status;
}
