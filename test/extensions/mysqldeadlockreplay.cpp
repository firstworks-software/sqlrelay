// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclient.h>
#include <rudiments/charstring.h>
#include <rudiments/semaphoreset.h>
#include <rudiments/file.h>
#include <rudiments/permissions.h>
#include <rudiments/process.h>
#include <rudiments/snooze.h>
#include <rudiments/stdio.h>

semaphoreset	*sem;
uint16_t	sessionid;

const char *success="\033[32msuccess\033[0m";
const char *failure="\033[31mfailure\033[0m";

// the replay trigger's <log file="..."/> for the deadlock condition.  it gets
// the transaction log the replay ran, which is the only place the queries
// replay rewrote are visible from outside the server
const char	*deadlocklog="/tmp/mysqldeadlockreplay-deadlock.log";

void assertEquals(const char *actual, const char *expected) {

	if (!expected) {
		if (!actual) {
			stdoutput.printf("%s ",success);
			return;
		} else {
			stdoutput.printf("%s!=%s\n",actual,expected);
			stdoutput.printf("%s ",failure);
			// #10114: session 1 may now fail anywhere across both
			// deadlock scenarios, and session 2 waits on more than
			// just semaphores 0 and 1, so signal everything session
			// 2 might be waiting on, not just the first two
			if (!sessionid) {
				delete sem;
				file::remove("semkey");
			} else if (sessionid==1) {
				sem->signal(0);
				sem->signal(1);
				sem->signal(2);
				sem->signal(3);
			} else if (sessionid==2) {
				sem->signal(4);
			}
			process::exit(1);
		}
	}

	if (!charstring::compare(actual,expected)) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s!=%s\n",actual,expected);
		stdoutput.printf("%s ",failure);
		if (!sessionid) {
			delete sem;
			file::remove("semkey");
		} else if (sessionid==1) {
			sem->signal(0);
			sem->signal(1);
			sem->signal(2);
			sem->signal(3);
		} else if (sessionid==2) {
			sem->signal(4);
		}
		process::exit(1);
	}
}

void assertEquals(int actual, int expected) {

	if (actual==expected) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%d!=%d\n",actual,expected);
		stdoutput.printf("%s ",failure);
		if (!sessionid) {
			delete sem;
			file::remove("semkey");
		} else if (sessionid==1) {
			sem->signal(0);
			sem->signal(1);
			sem->signal(2);
			sem->signal(3);
		} else if (sessionid==2) {
			sem->signal(4);
		}
		process::exit(1);
	}
}

void assertTrue(bool actual) {

	if (actual) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s!=true\n",(actual)?"true":"false");
		stdoutput.printf("%s ",failure);
		if (!sessionid) {
			delete sem;
			file::remove("semkey");
		} else if (sessionid==1) {
			sem->signal(0);
			sem->signal(1);
			sem->signal(2);
			sem->signal(3);
		} else if (sessionid==2) {
			sem->signal(4);
		}
		process::exit(1);
	}
}

void assertLogContains(const char *filename, const char *needle) {

	char	*contents=file::getContents(filename);
	if (contents && charstring::contains(contents,needle)) {
		stdoutput.printf("%s ",success);
		delete[] contents;
		return;
	}
	stdoutput.printf("%s does not contain \"%s\":\n%s\n",
				filename,needle,(contents)?contents:"(null)");
	stdoutput.printf("%s ",failure);
	delete[] contents;
	delete sem;
	file::remove("semkey");
	process::exit(1);
}

int main(int argc, char **argv) {

	// deadlock replay needs a transactional storage engine, which the
	// mysql 3.23 backend lacks; skip on mysql before 5
	{
		sqlrconnection	versioncon("sqlrelay",9017,"/tmp/mysqldeadlockreplay.socket",
					"testuser","testpassword",0,1);
		const char	*dbversion=versioncon.dbVersion();
		uint32_t	majorversion=dbversion[0]-'0';
		if (majorversion<5) {
			stdoutput.printf("MySQL version < 5, "
						"skipping tests\n");
			return 0;
		}
	}

	sessionid=0;

	sem=new semaphoreset();

	// create the key file
	file::remove("semkey");
	file	fd;
	if (!fd.create("semkey",permissions::parsePermString("rw-------"))) {
		stdoutput.printf("failed to create semkey file\n");
		process::exit(1);
	}
	fd.close();

	// create the semaphore
	int32_t	vals[]={0,0,0,0,0};
	if (!sem->create(file::generateKey("semkey",1),
			permissions::parsePermString("rw-------"),
			5,vals)) {
		stdoutput.printf("failed to create semaphoreset\n");
		process::exit(1);
	}

	// The trigger appends to the log file, so one left behind by a previous
	// run would make the replayed-query checks at the end a false pass.
	file::remove(deadlocklog);

	pid_t	pid1=process::fork();
	if (!pid1) {

		sessionid=1;

		// attach to the semaphore
		if (!sem->attach(file::generateKey("semkey",1),5)) {
			stdoutput.printf("failed to attach to "
						"the semaphoreset\n");
			process::exit(1);
		}

		// connect to relay
		sqlrconnection	sqlrcon("sqlrelay",9017,
			"/tmp/mysqldeadlockreplay.socket","testuser","testpassword",0,1);
		sqlrcursor	sqlrcur(&sqlrcon);

		stdoutput.printf("SESSION 1...\n");

		// set things up
		sqlrcur.sendQuery("drop table testtable");
		assertTrue(sqlrcur.sendQuery("create table testtable "
			"(col1 int primary key auto_increment, col2 int, col3 varchar(20), col4 varchar(20))"));
		assertTrue(sqlrcur.sendQuery(
			"insert into testtable "
			"(col2,col3,col4) "
			"values (1,'hello','hello')"));
		assertTrue(sqlrcur.sendQuery(
			"insert into testtable "
			"(col2,col3,col4) "
			"values (1,'hello','hello')"));
		sqlrcur.sendQuery("drop table weighttable");
		assertTrue(sqlrcur.sendQuery("create table weighttable "
						"(col1 int)"));
		// #10114: a table to deadlock a null-autoincrement insert on
		// directly, instead of a later query in the transaction.
		// lockcol is the unique key the two sessions contend for, and
		// the two rows seeded here bound the gap they contend in
		sqlrcur.sendQuery("drop table nullautoinctable");
		assertTrue(sqlrcur.sendQuery(
			"create table nullautoinctable "
			"(col1 int primary key auto_increment, col2 int, "
			"col3 varchar(20), lockcol int unique)"));
		assertTrue(sqlrcur.sendQuery(
			"insert into nullautoinctable "
			"(col2,col3,lockcol) "
			"values (0,'seed1',1)"));
		assertTrue(sqlrcur.sendQuery(
			"insert into nullautoinctable "
			"(col2,col3,lockcol) "
			"values (0,'seed2',10)"));
		stdoutput.printf("\n");

		assertTrue(sqlrcon.begin());

		// Innodb rolls back whichever of the two transactions changed
		// fewer rows, so the inserts session 2 runs below would
		// otherwise make session 1 the victim instead.  These rows
		// outweigh them and keep session 2 the one that gets replayed.
		assertTrue(sqlrcur.sendQuery("insert into weighttable values "
			"(1),(2),(3),(4),(5),(6),(7),(8),(9),(10),"
			"(11),(12),(13),(14),(15),(16),(17),(18),(19),(20)"));

		// execute the initial update
		assertTrue(sqlrcur.sendQuery("update testtable set "
						"col2=col2+1 where col1=1"));
		stdoutput.printf("\n");

		// signal the second session to go
		sem->signal(0);

		// wait for the second session to do its update
		snooze::macrosnooze(3);

		stdoutput.printf("SESSION 1...\n");

		// execute the final update
		assertTrue(sqlrcur.sendQuery("update testtable set "
						"col2=col2+1 where col1=2"));
		assertTrue(sqlrcon.commit());
		stdoutput.printf("\n");

		// signal the second session to go
		sem->signal(1);

		// #10114: a second deadlock, using the same weighttable trick,
		// but this time session 2's losing statement is a
		// null-autoincrement insert into nullautoinctable itself,
		// rather than a later query in the transaction

		// The second session replays the deadlock above inside the
		// commit it just got out of, and the replay re-runs the same
		// updates.  Waiting for it to finish keeps it from contending
		// with the locks below and making this session the victim.
		sem->wait(4);

		assertTrue(sqlrcon.begin());
		assertTrue(sqlrcur.sendQuery("insert into weighttable values "
			"(21),(22),(23),(24),(25),(26),(27),(28),(29),(30),"
			"(31),(32),(33),(34),(35),(36),(37),(38),(39),(40)"));

		// Searching the unique index for a lockcol that isn't there
		// locks the gap below the next row, lockcol=10, without
		// locking that row itself.  Session 2's insert of lockcol=5
		// lands in that gap, so it has to wait for this lock, while
		// this session waits below for the lockcol=10 row session 2
		// updates.
		assertTrue(sqlrcur.sendQuery("select * from nullautoinctable "
						"where lockcol=5 for update"));
		stdoutput.printf("\n");

		// signal the second session to go
		sem->signal(2);

		// wait for the second session to do its insert
		snooze::macrosnooze(3);

		stdoutput.printf("SESSION 1...\n");

		// this one should hang and wait for session 2 to release the
		// lockcol=10 row
		assertTrue(sqlrcur.sendQuery("update nullautoinctable set "
						"col2=col2 where lockcol=10"));
		assertTrue(sqlrcon.commit());
		stdoutput.printf("\n");

		// signal the second session to go
		sem->signal(3);

		// done
		process::exit(0);
	}

	pid_t	pid2=process::fork();
	if (!pid2) {

		sessionid=2;

		// attach to the semaphore
		if (!sem->attach(file::generateKey("semkey",1),5)) {
			stdoutput.printf("failed to attach to "
						"the semaphoreset\n");
			process::exit(1);
		}

		// connect to relay
		sqlrconnection	sqlrcon("sqlrelay",9017,
			"/tmp/mysqldeadlockreplay.socket","testuser","testpassword",0,1);
		sqlrcursor	sqlrcur(&sqlrcon);

		// wait for the first session to let us go
		sem->wait(0);

		stdoutput.printf("SESSION 2...\n");

		assertTrue(sqlrcon.begin());

		// Inserts that supply a literal null for the auto-increment
		// column, in null, NULL, and Null case, since detection has
		// to be case-insensitive.  The deadlock below rolls these
		// rows back, but innodb's auto_increment counter doesn't roll
		// back with them, so replaying them as written would land
		// them on new, higher ids.  The replay has to substitute the
		// ids they got here.  The third one also carries a clause
		// after the values list, which the replay has to keep.
		// #10115: the first one also uses uppercase INSERT/VALUES,
		// since keyword detection has to be case-insensitive too.
		assertTrue(sqlrcur.sendQuery(
			"INSERT INTO testtable "
			"VALUES (NULL,10,'aaa','aaa')"));
		assertTrue(sqlrcur.sendQuery(
			"insert into testtable "
			"(col1,col2,col3,col4) "
			"values (null,20,'bbb','bbb')"));
		assertTrue(sqlrcur.sendQuery(
			"insert into testtable "
			"(col1,col2,col3,col4) "
			"values (null,30,'ccc','ccc') "
			"on duplicate key update col2=col2"));
		assertTrue(sqlrcur.sendQuery(
			"insert into testtable "
			"values (NULL,40,'ddd','ddd')"));
		assertTrue(sqlrcur.sendQuery(
			"insert into testtable "
			"(col1,col2,col3,col4) "
			"values (Null,50,'eee','eee')"));

		// #10113: col3's value is a function call with its own
		// comma-separated arguments, and col3 comes before col1 in
		// the column list, so a paren-blind comma split would
		// desync the value-to-column mapping and land the
		// autoincrement null on the wrong column
		assertTrue(sqlrcur.sendQuery(
			"insert into testtable "
			"(col3,col1,col2,col4) "
			"values (COALESCE(NULL,'ggg','hhh'),null,60,'iii')"));

		// #10117: col4's value is a double-quoted string literal
		// containing both a comma and a close-paren, and col4
		// comes before col1 in the column list, so a quote-blind
		// comma/close-paren scan would desync the value-to-column
		// mapping and land the autoincrement null on the wrong
		// column
		assertTrue(sqlrcur.sendQuery(
			"insert into testtable "
			"(col4,col1,col2,col3) "
			"values (\"x,y)z\",null,70,'nnn')"));

		// execute the conflicting updates
		assertTrue(sqlrcur.sendQuery("update testtable set "
						"col2=col2+1 where col1=2"));
		stdoutput.printf("\n");


		// this one should hang and wait for session 1 to commit
		bool	qr=sqlrcur.sendQuery("update testtable set "
						"col2=col2+1 where col1=1");

		// commit
		bool	cr=sqlrcon.commit();

		// wait for the first session to let us go
		sem->wait(1);

		stdoutput.printf("SESSION 2...\n");
		assertEquals(qr,1);
		assertEquals(cr,1);

		// #10114: the replay above is done, so let session 1 start the
		// second deadlock
		sem->signal(4);

		// wait for session 1 to lock the gap below lockcol=10
		sem->wait(2);

		stdoutput.printf("SESSION 2...\n");

		assertTrue(sqlrcon.begin());

		// succeeds, and its id gets cached as the connection's
		// last-insert-id.  lockcol=1000 is outside the gap session 1
		// locked, so this one doesn't contend for anything.
		assertTrue(sqlrcur.sendQuery(
			"insert into nullautoinctable "
			"(col1,col2,col3,lockcol) "
			"values (null,1,'kkk',1000)"));

		// lock the row session 1 waits for
		assertTrue(sqlrcur.sendQuery("update nullautoinctable set "
						"col2=col2 where lockcol=10"));
		stdoutput.printf("\n");

		// This one lands in the gap session 1 locked, so it hangs
		// there until session 1 asks for the lockcol=10 row and innodb
		// rolls this session back to break the cycle.  Before the fix,
		// the insert above's id got spliced into this one's null, so
		// the replay collided with that insert on the primary key
		// instead of this one keeping its null and getting a fresh id
		// of its own.
		bool	qr2=sqlrcur.sendQuery(
			"insert into nullautoinctable "
			"(col1,col2,col3,lockcol) "
			"values (null,2,'lll',5)");

		// commit
		bool	cr2=sqlrcon.commit();

		// wait for the first session to let us go
		sem->wait(3);

		stdoutput.printf("SESSION 2...\n");
		assertEquals(qr2,1);
		assertEquals(cr2,1);

		// done
		process::exit(0);
	}

	// wait for children to exit
	process::wait(pid1);
	process::wait(pid2);
	stdoutput.printf("\n");

	// connect to relay
	sqlrconnection	sqlrcon("sqlrelay",9017,
			"/tmp/mysqldeadlockreplay.socket","testuser","testpassword",0,1);
	sqlrcursor	sqlrcur(&sqlrcon);


	// results
	stdoutput.printf("RESULTS: \n");
	sqlrcur.sendQuery("select * from testtable order by col1");
	assertEquals(sqlrcur.getField(0,"col1"),"1");
	assertEquals(sqlrcur.getField(0,"col2"),"3");
	assertEquals(sqlrcur.getField(1,"col1"),"2");
	assertEquals(sqlrcur.getField(1,"col2"),"3");
	assertEquals((int)sqlrcur.rowCount(),9);
	stdoutput.printf("\n");

	// The rows session 2 inserted kept the ids they got before the deadlock
	// rolled them back, instead of the higher ones the auto_increment
	// counter would have handed out when the replay re-ran the inserts.
	stdoutput.printf("REPLAYED INSERT IDS: \n");
	assertEquals(sqlrcur.getField(2,"col1"),"3");
	assertEquals(sqlrcur.getField(2,"col2"),"10");
	assertEquals(sqlrcur.getField(2,"col3"),"aaa");
	assertEquals(sqlrcur.getField(3,"col1"),"4");
	assertEquals(sqlrcur.getField(3,"col2"),"20");
	assertEquals(sqlrcur.getField(3,"col3"),"bbb");
	assertEquals(sqlrcur.getField(4,"col1"),"5");
	assertEquals(sqlrcur.getField(4,"col2"),"30");
	assertEquals(sqlrcur.getField(4,"col3"),"ccc");
	assertEquals(sqlrcur.getField(5,"col1"),"6");
	assertEquals(sqlrcur.getField(5,"col2"),"40");
	assertEquals(sqlrcur.getField(5,"col3"),"ddd");
	assertEquals(sqlrcur.getField(6,"col1"),"7");
	assertEquals(sqlrcur.getField(6,"col2"),"50");
	assertEquals(sqlrcur.getField(6,"col3"),"eee");
	assertEquals(sqlrcur.getField(7,"col1"),"8");
	assertEquals(sqlrcur.getField(7,"col2"),"60");
	assertEquals(sqlrcur.getField(7,"col3"),"ggg");
	assertEquals(sqlrcur.getField(7,"col4"),"iii");
	assertEquals(sqlrcur.getField(8,"col1"),"9");
	assertEquals(sqlrcur.getField(8,"col2"),"70");
	assertEquals(sqlrcur.getField(8,"col3"),"nnn");
	assertEquals(sqlrcur.getField(8,"col4"),"x,y)z");
	stdoutput.printf("\n");

	// The replayed queries themselves.  The row state above can't tell
	// whether the clause after the values list survived the rewrite, since
	// no duplicate key ever collides here, so check the query text.
	stdoutput.printf("REPLAYED QUERIES: \n");
	assertLogContains(deadlocklog,
			"INSERT INTO testtable (col1,col2,col3,col4) "
			"values (3,10,'aaa','aaa')");
	assertLogContains(deadlocklog,
			"insert into testtable (col1,col2,col3,col4) "
			"values (4,20,'bbb','bbb')");
	assertLogContains(deadlocklog,
			"insert into testtable (col1,col2,col3,col4) "
			"values (5,30,'ccc','ccc') "
			"on duplicate key update col2=col2");
	assertLogContains(deadlocklog,
			"insert into testtable (col1,col2,col3,col4) "
			"values (6,40,'ddd','ddd')");
	assertLogContains(deadlocklog,
			"insert into testtable (col1,col2,col3,col4) "
			"values (7,50,'eee','eee')");
	// the function call's value must come through unmolested, and
	// the substituted id must land on col1, not on a column the
	// paren-blind bug would have shifted it to
	assertLogContains(deadlocklog,
			"insert into testtable (col3,col1,col2,col4) "
			"values (COALESCE(NULL,'ggg','hhh'),8,60,'iii')");
	// the double-quoted literal's embedded comma and close-paren
	// must come through unmolested, and the substituted id must
	// land on col1, not on a column a quote-blind scan would have
	// shifted it to
	assertLogContains(deadlocklog,
			"insert into testtable (col4,col1,col2,col3) "
			"values (\"x,y)z\",9,70,'nnn')");
	stdoutput.printf("\n");

	// #10114: the insert that lost the second deadlock was itself a
	// null-autoincrement insert.  its row must have gotten a fresh id
	// of its own, not the other insert's id
	stdoutput.printf("REPLAYED NULL-AUTOINCREMENT INSERT, FAILING QUERY ITSELF: \n");
	sqlrcur.sendQuery("select * from nullautoinctable order by col1");
	assertEquals((int)sqlrcur.rowCount(),4);
	assertEquals(sqlrcur.getField(2,"col3"),"kkk");
	assertEquals(sqlrcur.getField(3,"col3"),"lll");
	assertTrue(charstring::convertToInteger(sqlrcur.getField(3,"col1")) >
			charstring::convertToInteger(sqlrcur.getField(2,"col1")));
	stdoutput.printf("\n");

	// The insert that succeeded still gets the id it got here substituted
	// for its null, but the one that lost the deadlock keeps its null,
	// rather than the other insert's id.
	stdoutput.printf("REPLAYED NULL-AUTOINCREMENT INSERT QUERY TEXT: \n");
	assertLogContains(deadlocklog,
			"insert into nullautoinctable (col1,col2,col3,lockcol) "
			"values (3,1,'kkk',1000)");
	assertLogContains(deadlocklog,
			"insert into nullautoinctable (col1,col2,col3,lockcol) "
			"values (null,2,'lll',5)");
	stdoutput.printf("\n");

	// clean up
	delete sem;
	file::remove("semkey");

	// done
	process::exit(0);
}
