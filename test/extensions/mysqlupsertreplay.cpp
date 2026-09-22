// Copyright (c) David Muse
// See the file COPYING for more information.

// replay and upsert configured on the same connection, with replay listed
// first (see the "replay must be listed before upsert" note in the config
// guide's upsert section).
//
// upsert runs the update it substitutes for a failed insert on a private
// cursor.  That update only reaches replay's log if upsert runs it with
// triggers enabled - otherwise the log holds the insert that failed and not
// the update that actually took effect, and a replay silently loses the
// update.
//
// The two-entry log only exists inside a transaction - outside of one,
// replay's logQuery() clears the log at the top of every call, so the update
// would just replace the insert - so everything here runs inside one.
//
// A deadlock is what makes the lost update visible.  Innodb rolls the whole
// transaction back, so the update upsert ran is gone from the database, and
// only the replay can put it back.

#include <rudiments/charstring.h>
#include <rudiments/file.h>
#include <rudiments/permissions.h>
#include <rudiments/process.h>
#include <rudiments/semaphoreset.h>
#include <rudiments/stdio.h>
#include <sqlrelay/sqlrclient.h>

sqlrconnection	*con=NULL;
sqlrcursor	*cur=NULL;
sqlrconnection	*secondcon=NULL;
sqlrcursor	*secondcur=NULL;

#include "../c++/asserts.cpp"

// the replay trigger's <log file="..."/> for the deadlock condition.  the
// transaction log it writes there is the only place the queries replay logged
// are visible from outside the server
const char	*deadlocklog="/tmp/mysqlupsertreplay-deadlock.log";

const char	*semkeyfile="upsertreplaysemkey";

semaphoreset	*sem=NULL;

// the insert that duplicates the seeded row, and the update upsert converts
// it into, exactly as they should appear in the transaction log
const char	*loggedinsert=
		"insert into student values "
		"(null,'David','Muse','Sophomore','ME','3.5')";
const char	*loggedupdate=
		"update student set "
		"firstname='David',lastname='Muse',"
		"year='Sophomore',major='ME',gpa='3.5' "
		"where firstname='David' and lastname='Muse'";

void assertLogContains(const char *filename, const char *needle) {

	char	*contents=file::getContents(filename);
	if (contents && charstring::contains(contents,needle)) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s does not contain \"%s\":\n%s\n",
				filename,needle,(contents)?contents:"(null)");
		status=1;
	}
	delete[] contents;
}

// asserts that "first" turns up in the log file before "second" does
void assertLogOrder(const char *filename,
				const char *first, const char *second) {

	char		*contents=file::getContents(filename);
	const char	*firstpos=(contents)?
				charstring::findFirst(contents,first):NULL;
	const char	*secondpos=(contents)?
				charstring::findFirst(contents,second):NULL;
	if (firstpos && secondpos && firstpos<secondpos) {
		stdoutput.printf("%s ",success);
	} else {
		stdoutput.printf("%s\n",failure);
		stdoutput.printf("%s does not contain \"%s\" "
					"followed by \"%s\":\n%s\n",
					filename,first,second,
					(contents)?contents:"(null)");
		status=1;
	}
	delete[] contents;
}

// session 1 - the session that wins the deadlock.  it locks one row, lets
// session 2 lock the other, then asks for session 2's row, which closes the
// cycle.  the twenty rows it inserts first make it the heavier of the two
// transactions, so innodb picks session 2 as the victim
void sessionOne() {

	sqlrconnection	sqlrcon("sqlrelay",9045,
				"/tmp/mysqlupsertreplay.socket",
				"testuser","testpassword",0,1);
	sqlrcursor	sqlrcur(&sqlrcon);
	con=&sqlrcon;
	cur=&sqlrcur;

	stdoutput.printf("SESSION 1...\n");

	bool	ok=sqlrcon.begin();
	ok=ok && sqlrcur.sendQuery("insert into weighttable values "
			"(1),(2),(3),(4),(5),(6),(7),(8),(9),(10),"
			"(11),(12),(13),(14),(15),(16),(17),(18),(19),(20)");
	ok=ok && sqlrcur.sendQuery("update locktable set "
					"col2=col2+1 where col1=1");

	// let session 2 go, whether or not any of that worked - otherwise it
	// would wait here forever
	sem->signal(0);

	assertTrue(ok);
	stdoutput.printf("\n");
	if (!ok) {
		con=NULL;
		cur=NULL;
		process::exit(1);
	}

	// Wait for session 2 to lock the other row.  Waiting a fixed amount of
	// time instead would let this session take that row first on a loaded
	// machine, and then no lock cycle forms and no deadlock happens.
	sem->wait(1);

	stdoutput.printf("SESSION 1...\n");

	// this one waits for session 2's row, which closes the cycle.  it goes
	// through once innodb rolls session 2 back
	assertTrue(sqlrcur.sendQuery("update locktable set "
					"col2=col2+1 where col1=2"));
	assertTrue(sqlrcon.commit());
	stdoutput.printf("\n");

	con=NULL;
	cur=NULL;
	process::exit(status);
}

// session 2 - the session that loses the deadlock, and the one the upsert and
// replay triggers act on
void sessionTwo() {

	sqlrconnection	sqlrcon("sqlrelay",9045,
				"/tmp/mysqlupsertreplay.socket",
				"testuser","testpassword",0,1);
	sqlrcursor	sqlrcur(&sqlrcon);
	con=&sqlrcon;
	cur=&sqlrcur;

	// wait for session 1 to lock its row
	sem->wait(0);

	stdoutput.printf("SESSION 2...\n");

	assertTrue(sqlrcon.begin());

	// duplicates the seeded row on the (firstname,lastname) unique key, so
	// upsert converts it to an update.  the client sees success, not the
	// duplicate-key error, and replay logs both the insert and the update
	assertTrue(sqlrcur.sendQuery(loggedinsert));
	assertFalse(charstring::contains(sqlrcur.errorMessage(),
						"Duplicate entry"));

	// lock the row session 1 waits for
	assertTrue(sqlrcur.sendQuery("update locktable set "
					"col2=col2+1 where col1=2"));
	stdoutput.printf("\n");

	// Let session 1 close the cycle, now that this row is locked.  This
	// has to come before the query below, which blocks until innodb breaks
	// the deadlock, or the two sessions would just wait on each other.
	sem->signal(1);

	// This one waits for the row session 1 locked, until session 1 asks
	// for the row above and innodb breaks the cycle by rolling this
	// session back.  The replay trigger re-runs the whole logged
	// transaction inside this sendQuery, so it comes back successful.
	bool	qr=sqlrcur.sendQuery("update locktable set "
					"col2=col2+1 where col1=1");
	bool	cr=sqlrcon.commit();

	stdoutput.printf("SESSION 2...\n");
	assertTrue(qr);
	assertTrue(cr);
	stdoutput.printf("\n");

	con=NULL;
	cur=NULL;
	process::exit(status);
}

int main(int argc, char **argv) {

	// the deadlock needs a transactional storage engine, and the query
	// that gets the column info the upsert trigger needs doesn't work
	// before mysql 5, so skip the whole thing there
	{
		sqlrconnection	versioncon("sqlrelay",9045,
					"/tmp/mysqlupsertreplay.socket",
					"testuser","testpassword",0,1);
		const char	*dbversion=versioncon.dbVersion();
		uint32_t	majorversion=dbversion[0]-'0';
		if (majorversion<5) {
			stdoutput.printf("MySQL version < 5, "
						"skipping tests\n");
			return 0;
		}
	}

	// set up the tables, then drop the connection, so that the two
	// sessions below can both have one of the instance's two connections
	stdoutput.printf("SETUP:\n");
	{
		sqlrconnection	setupcon("sqlrelay",9045,
					"/tmp/mysqlupsertreplay.socket",
					"testuser","testpassword",0,1);
		sqlrcursor	setupcur(&setupcon);
		con=&setupcon;
		cur=&setupcur;

		setupcur.sendQuery("drop table student");
		assertTrue(setupcur.sendQuery("create table student ("
						"id int auto_increment, "
						"firstname varchar(20), "
						"lastname varchar(20), "
						"year varchar(20), "
						"major varchar(20), "
						"gpa varchar(20), "
						"primary key (id), "
						"unique (firstname,lastname) "
						")"));
		assertTrue(setupcur.sendQuery("insert into student values "
					"(null,"
					"'David','Muse','Freshman','ME','4.0')"));

		// the two rows the sessions deadlock over
		setupcur.sendQuery("drop table locktable");
		assertTrue(setupcur.sendQuery("create table locktable "
					"(col1 int primary key, col2 int)"));
		assertTrue(setupcur.sendQuery("insert into locktable "
						"values (1,0)"));
		assertTrue(setupcur.sendQuery("insert into locktable "
						"values (2,0)"));

		// innodb rolls back whichever transaction changed fewer rows,
		// so session 1 pads itself with rows here to stay the heavier
		// one and keep session 2 the victim
		setupcur.sendQuery("drop table weighttable");
		assertTrue(setupcur.sendQuery("create table weighttable "
						"(col1 int)"));

		con=NULL;
		cur=NULL;
	}
	stdoutput.printf("\n");

	// the trigger appends to the log file, so one left behind by an
	// earlier run would make the checks at the end a false pass
	file::remove(deadlocklog);

	// create the semaphores the sessions hand off with - one that lets
	// session 2 start once session 1 has locked its row, and one that lets
	// session 1 go on once session 2 has locked the other
	file::remove(semkeyfile);
	file	fd;
	if (!fd.create(semkeyfile,permissions::parsePermString("rw-------"))) {
		stdoutput.printf("failed to create %s file\n",semkeyfile);
		process::exit(1);
	}
	fd.close();

	sem=new semaphoreset();
	int32_t	vals[]={0,0};
	if (!sem->create(file::generateKey(semkeyfile,1),
			permissions::parsePermString("rw-------"),
			2,vals)) {
		stdoutput.printf("failed to create semaphoreset\n");
		process::exit(1);
	}

	pid_t	pid1=process::fork();
	if (!pid1) {
		if (!sem->attach(file::generateKey(semkeyfile,1),2)) {
			stdoutput.printf("failed to attach to "
						"the semaphoreset\n");
			process::exit(1);
		}
		sessionOne();
	}

	pid_t	pid2=process::fork();
	if (!pid2) {
		if (!sem->attach(file::generateKey(semkeyfile,1),2)) {
			stdoutput.printf("failed to attach to "
						"the semaphoreset\n");
			process::exit(1);
		}
		sessionTwo();
	}

	// wait for the sessions to finish, and fail if either of them did
	int32_t	status1=0;
	int32_t	status2=0;
	process::wait(pid1,&status1);
	process::wait(pid2,&status2);
	stdoutput.printf("SESSION RESULTS:\n");
	assertEquals(status1,0);
	assertEquals(status2,0);
	stdoutput.printf("\n");

	delete sem;
	file::remove(semkeyfile);

	sqlrconnection	sqlrcon("sqlrelay",9045,
				"/tmp/mysqlupsertreplay.socket",
				"testuser","testpassword",0,1);
	sqlrcursor	sqlrcur(&sqlrcon);
	con=&sqlrcon;
	cur=&sqlrcur;

	// The deadlock rolled the update upsert ran back with the rest of
	// session 2's transaction, so the row only shows the upserted values
	// if the replay ran the update again.  Without the update in the log,
	// the replay re-runs the insert, which fails as a duplicate again, and
	// the row keeps the values it was seeded with.
	stdoutput.printf("REPLAYED UPSERT:\n");
	assertTrue(sqlrcur.sendQuery("select * from student"));
	assertEquals((int)sqlrcur.rowCount(),1);
	assertEquals(sqlrcur.getField(0,"id"),"1");
	assertEquals(sqlrcur.getField(0,"firstname"),"David");
	assertEquals(sqlrcur.getField(0,"lastname"),"Muse");
	assertEquals(sqlrcur.getField(0,"year"),"Sophomore");
	assertEquals(sqlrcur.getField(0,"major"),"ME");
	assertEquals(sqlrcur.getField(0,"gpa"),"3.5");
	stdoutput.printf("\n");

	// the log holds the insert that failed and the update upsert
	// substituted for it, in that order
	stdoutput.printf("REPLAY LOG:\n");
	assertLogContains(deadlocklog,loggedinsert);
	assertLogContains(deadlocklog,loggedupdate);
	assertLogOrder(deadlocklog,loggedinsert,loggedupdate);
	stdoutput.printf("\n");

	// clean up
	sqlrcur.sendQuery("drop table student");
	sqlrcur.sendQuery("drop table locktable");
	sqlrcur.sendQuery("drop table weighttable");

	con=NULL;
	cur=NULL;

	reportTestStatus();

	return status;
}
