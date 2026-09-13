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
			if (!sessionid) {
				delete sem;
				file::remove("semkey");
			} else if (sessionid==1) {
				sem->signal(0);
				sem->signal(1);
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
	int32_t	vals[]={0,0,0};
	if (!sem->create(file::generateKey("semkey",1),
			permissions::parsePermString("rw-------"),
			3,vals)) {
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
		if (!sem->attach(file::generateKey("semkey",1),3)) {
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

		// done
		process::exit(0);
	}

	pid_t	pid2=process::fork();
	if (!pid2) {

		sessionid=2;

		// attach to the semaphore
		if (!sem->attach(file::generateKey("semkey",1),3)) {
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
		// column.  The deadlock below rolls these rows back, but
		// innodb's auto_increment counter doesn't roll back with them,
		// so replaying them as written would land them on new, higher
		// ids.  The replay has to substitute the ids they got here.
		// The third one also carries a clause after the values list,
		// which the replay has to keep.
		assertTrue(sqlrcur.sendQuery(
			"insert into testtable "
			"values (null,10,'aaa','aaa')"));
		assertTrue(sqlrcur.sendQuery(
			"insert into testtable "
			"(col1,col2,col3,col4) "
			"values (null,20,'bbb','bbb')"));
		assertTrue(sqlrcur.sendQuery(
			"insert into testtable "
			"(col1,col2,col3,col4) "
			"values (null,30,'ccc','ccc') "
			"on duplicate key update col2=col2"));

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
	assertEquals((int)sqlrcur.rowCount(),5);
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
	stdoutput.printf("\n");

	// The replayed queries themselves.  The row state above can't tell
	// whether the clause after the values list survived the rewrite, since
	// no duplicate key ever collides here, so check the query text.
	stdoutput.printf("REPLAYED QUERIES: \n");
	assertLogContains(deadlocklog,
			"insert into testtable (col1,col2,col3,col4) "
			"values (3,10,'aaa','aaa')");
	assertLogContains(deadlocklog,
			"insert into testtable (col1,col2,col3,col4) "
			"values (4,20,'bbb','bbb')");
	assertLogContains(deadlocklog,
			"insert into testtable (col1,col2,col3,col4) "
			"values (5,30,'ccc','ccc') "
			"on duplicate key update col2=col2");
	stdoutput.printf("\n");

	// clean up
	delete sem;
	file::remove("semkey");

	// done
	process::exit(0);
}
