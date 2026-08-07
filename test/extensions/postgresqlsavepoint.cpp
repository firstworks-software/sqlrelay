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

	stdoutput.printf("SAVEPOINT:\n");
	con=new sqlrconnection("sqlrelay",9021,"/tmp/postgresqlsavepointtest.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);

	// set up - autocommit, no transaction yet, the trigger should be
	// inactive here
	cur->sendQuery("drop table sptest");
	assertTrue(cur->sendQuery("create table sptest ("
					"id int primary key, "
					"value varchar(20) "
					")"));
	stdoutput.printf("\n");


	// Postgresql's default behavior is that any error in a transaction
	// aborts the whole transaction - subsequent queries fail with
	// "current transaction is aborted, commands ignored until end of
	// transaction block" until rollback.  The savepoints trigger should
	// contain the failure to just the failed query by rolling back to a
	// savepoint it took before the query, leaving the rest of the
	// transaction intact.

	// failing query in the middle of a transaction
	stdoutput.printf("CONTAIN FAILURE WITHIN TX:\n");
	assertTrue(con->begin());
	// successful insert
	assertTrue(cur->sendQuery("insert into sptest values (1,'one')"));
	// duplicate primary key - this fails
	assertFalse(cur->sendQuery("insert into sptest values (1,'dup')"));
	// without the savepoints trigger this would fail with
	// "current transaction is aborted..."; with the trigger it should
	// succeed because the failed query was rolled back to a savepoint
	assertTrue(cur->sendQuery("insert into sptest values (2,'two')"));
	assertTrue(con->commit());
	// rows 1 and 2 should be there, "dup" should not
	assertTrue(cur->sendQuery("select count(*) from sptest"));
	assertEquals(cur->getField(0,(uint32_t)0),"2");
	assertTrue(cur->sendQuery("select id,value from sptest order by id"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"one");
	assertEquals(cur->getField(1,(uint32_t)0),"2");
	assertEquals(cur->getField(1,1),"two");
	stdoutput.printf("\n");


	// Multiple failing queries in the same transaction.  Each one
	// should be contained individually and shouldn't poison the
	// transaction.
	stdoutput.printf("MULTIPLE FAILURES WITHIN TX:\n");
	assertTrue(con->begin());
	assertFalse(cur->sendQuery("insert into sptest values (1,'dup')"));
	assertTrue(cur->sendQuery("insert into sptest values (3,'three')"));
	assertFalse(cur->sendQuery("insert into sptest values (2,'dup')"));
	assertTrue(cur->sendQuery("insert into sptest values (4,'four')"));
	assertFalse(cur->sendQuery("insert into sptest values (3,'dup')"));
	assertTrue(con->commit());
	assertTrue(cur->sendQuery("select count(*) from sptest"));
	assertEquals(cur->getField(0,(uint32_t)0),"4");
	stdoutput.printf("\n");


	// PostgreSQL resolves references during prepare, so queries against
	// nonexistent tables (or other reference-related problems) fail at
	// prepare time rather than execute time.  Without a savepoint
	// bracketing the prepare, the prepare-time failure poisons the
	// transaction.  The savepoints trigger should contain prepare-time
	// failures the same way it contains execute-time failures.
	stdoutput.printf("CONTAIN PREPARE FAILURE WITHIN TX:\n");
	assertTrue(con->begin());
	assertTrue(cur->sendQuery("insert into sptest values (11,'eleven')"));
	// fails at prepare time - the table doesn't exist
	assertFalse(cur->sendQuery("select * from nonexistent_table"));
	// without containment of the prepare failure this would fail with
	// "current transaction is aborted..."; with the trigger's
	// before/after-prepare savepoint it should succeed
	assertTrue(cur->sendQuery("insert into sptest values (12,'twelve')"));
	assertTrue(con->commit());
	// rows 11 and 12 should now be present
	assertTrue(cur->sendQuery("select count(*) from sptest"));
	assertEquals(cur->getField(0,(uint32_t)0),"6");
	stdoutput.printf("\n");


	// Mix of prepare-time and execute-time failures within the same
	// transaction.  Both kinds should be contained without poisoning
	// the surrounding transaction.
	stdoutput.printf("MIXED PREPARE/EXECUTE FAILURES WITHIN TX:\n");
	assertTrue(con->begin());
	// prepare-time failure (nonexistent column)
	assertFalse(cur->sendQuery(
			"select nonexistent_column from sptest"));
	assertTrue(cur->sendQuery("insert into sptest values (13,'thirteen')"));
	// execute-time failure (duplicate primary key)
	assertFalse(cur->sendQuery("insert into sptest values (13,'dup')"));
	// prepare-time failure (nonexistent table)
	assertFalse(cur->sendQuery("update nonexistent_table set x=1"));
	assertTrue(cur->sendQuery("insert into sptest values (14,'fourteen')"));
	assertTrue(con->commit());
	assertTrue(cur->sendQuery("select count(*) from sptest"));
	assertEquals(cur->getField(0,(uint32_t)0),"8");
	stdoutput.printf("\n");


	// A successful query inside a transaction followed by an explicit
	// rollback should undo everything - including queries the trigger
	// committed savepoints for.  Releasing the savepoint must not have
	// caused the inserted row to escape the surrounding transaction.
	stdoutput.printf("ROLLBACK STILL UNDOES SUCCESSFUL QUERIES:\n");
	assertTrue(con->begin());
	assertTrue(cur->sendQuery("insert into sptest values (5,'five')"));
	assertTrue(cur->sendQuery("insert into sptest values (6,'six')"));
	assertTrue(con->rollback());
	assertTrue(cur->sendQuery("select count(*) from sptest"));
	// still 8 - the begin/inserts/rollback added nothing
	assertEquals(cur->getField(0,(uint32_t)0),"8");
	stdoutput.printf("\n");


	// Queries outside of a transaction (in autocommit mode) should be
	// untouched by the trigger - the trigger bails out when not in a
	// transaction since savepoints have no meaning there.
	stdoutput.printf("AUTOCOMMIT QUERIES UNAFFECTED:\n");
	assertTrue(cur->sendQuery("insert into sptest values (7,'seven')"));
	assertFalse(cur->sendQuery("insert into sptest values (7,'dup')"));
	assertTrue(cur->sendQuery("insert into sptest values (8,'eight')"));
	assertTrue(cur->sendQuery("select count(*) from sptest"));
	assertEquals(cur->getField(0,(uint32_t)0),"10");
	stdoutput.printf("\n");


	// autoCommitOff + begin-required mode (postgresql default for sqlr)
	stdoutput.printf("AUTOCOMMIT OFF MODE:\n");
	assertTrue(con->autoCommitOff());
	assertTrue(con->begin());
	assertTrue(cur->sendQuery("insert into sptest values (9,'nine')"));
	assertFalse(cur->sendQuery("insert into sptest values (9,'dup')"));
	assertTrue(cur->sendQuery("insert into sptest values (10,'ten')"));
	assertTrue(con->commit());
	assertTrue(con->autoCommitOn());
	assertTrue(cur->sendQuery("select count(*) from sptest"));
	assertEquals(cur->getField(0,(uint32_t)0),"12");
	stdoutput.printf("\n");


	// clean up
	assertTrue(cur->sendQuery("drop table sptest"));
	delete cur;
	delete con;
	stdoutput.printf("\n");

	reportTestStatus();

	return status;
}
