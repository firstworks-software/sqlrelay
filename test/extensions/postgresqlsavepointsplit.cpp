// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/stdio.h>
#include <sqlrelay/sqlrclient.h>

#include "../c++/asserts.cpp"

// printErrors() in asserts.cpp reports whatever these point at
sqlrconnection	*con=NULL;
sqlrcursor	*cur=NULL;
sqlrconnection	*secondcon=NULL;
sqlrcursor	*secondcur=NULL;

// The savepoints trigger wraps each query in a savepoint so a failure can be
// rolled back to that savepoint instead of poisoning the transaction.  The
// splitmultiinsert trigger replaces a multi-insert with one insert per row,
// run on a private cursor with triggers enabled - so savepoints runs again,
// nested inside the savepoint it already took on the outer cursor.
//
// The trigger module is instantiated once per connection, not per cursor, so
// the live savepoint has to be tracked per cursor.  When it wasn't, the nested
// run reset the shared state and the outer cursor's savepoint was never rolled
// back: the rows the successful single-inserts had already written survived a
// multi-insert that failed as a whole (#10252).
//
// The nesting doesn't depend on the order the two triggers are listed in.
// splitmultiinsert does its work in a before-execute trigger, and savepoints
// has already taken the outer savepoint in its before-prepare trigger by then,
// so both orders are run here.

static void runTest(const char *testname, uint16_t port, const char *socket) {

	stdoutput.printf("%s:\n",testname);

	con=new sqlrconnection("sqlrelay",port,socket,
					"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);

	// set up - outside of a transaction, both triggers stand aside
	cur->sendQuery("drop table sptest");
	assertTrue(cur->sendQuery("create table sptest ("
					"id int primary key, "
					"value varchar(20) "
					")"));

	assertTrue(con->begin());

	// splitmultiinsert turns this into three single-inserts.  The first
	// two succeed, the third duplicates the first row's primary key and
	// fails, and splitmultiinsert copies that error onto this cursor.
	assertFalse(cur->sendQuery("insert into sptest values "
					"(1,'one'),"
					"(2,'two'),"
					"(1,'dup')"));

	// The failed multi-insert has to be all-or-nothing.  Rolling back to
	// the outer cursor's savepoint undoes the two rows the successful
	// single-inserts wrote, so nothing is left behind.  With the savepoint
	// state shared across cursors, the nested run clobbered it and this
	// returned 2.
	assertTrue(cur->sendQuery("select count(*) from sptest"));
	assertEquals(cur->getField(0,(uint32_t)0),"0");

	// the session is still usable - the failure was contained rather than
	// aborting the transaction
	assertTrue(cur->sendQuery("insert into sptest values (3,'three')"));

	assertTrue(con->commit());

	// only the row inserted after the failure survives the commit
	assertTrue(cur->sendQuery("select count(*) from sptest"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertTrue(cur->sendQuery("select id,value from sptest order by id"));
	assertEquals(cur->getField(0,(uint32_t)0),"3");
	assertEquals(cur->getField(0,1),"three");

	// clean up
	assertTrue(cur->sendQuery("drop table sptest"));

	delete cur;
	delete con;
	cur=NULL;
	con=NULL;

	stdoutput.printf("\n");
}

int main(int argc, char **argv) {

	runTest("SAVEPOINTS BEFORE SPLITMULTIINSERT",
			9046,"/tmp/postgresqlsavepointsplit.socket");

	runTest("SPLITMULTIINSERT BEFORE SAVEPOINTS",
			9047,"/tmp/postgresqlsplitsavepoint.socket");

	reportTestStatus();

	return status;
}
