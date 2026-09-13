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


	// upsert
	stdoutput.printf("UPSERT:\n");
	con=new sqlrconnection("sqlrelay",9016,"/tmp/mysqlupsert.socket",
						"testuser","testpassword",0,1);

	// get the db version and bail for < 5, as the query to get the column
	// info doesn't work for < 5, making upserts also not work
	const char      *dbversion=con->dbVersion();
	uint32_t	majorversion=dbversion[0]-'0';
	if (majorversion<5) {
		stdoutput.printf("MySQL version < 5, skipping tests\n");
		delete con;
		return 0;
	}

	cur=new sqlrcursor(con);
	secondcur=new sqlrcursor(con);
	cur->sendQuery("drop table student");
	assertTrue(cur->sendQuery("create table student ("
					"id int auto_increment, "
					"firstname varchar(20), "
					"lastname varchar(20), "
					"year varchar(20), "
					"major varchar(20), "
					"gpa varchar(20), "
					"primary key (id), "
					"unique (firstname,lastname) "
					")"));
	stdoutput.printf("\n");
	// initial insert
	assertTrue(cur->sendQuery("insert into student values "
				"(null,"
				"'David','Muse','Freshman','ME','4.0')"));
	assertTrue(secondcur->sendQuery("select count(*) from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertTrue(secondcur->sendQuery("select * from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertEquals(secondcur->getField(0,1),"David");
	assertEquals(secondcur->getField(0,2),"Muse");
	assertEquals(secondcur->getField(0,3),"Freshman");
	assertEquals(secondcur->getField(0,4),"ME");
	assertEquals(secondcur->getField(0,5),"4.0");
	stdoutput.printf("\n");
	// should be converted to an update
	assertTrue(cur->sendQuery("insert into student values "
				"(null,"
				"'David','Muse','Sophomore','ME','3.5')"));
	assertTrue(secondcur->sendQuery("select count(*) from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertTrue(secondcur->sendQuery("select * from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertEquals(secondcur->getField(0,1),"David");
	assertEquals(secondcur->getField(0,2),"Muse");
	assertEquals(secondcur->getField(0,3),"Sophomore");
	assertEquals(secondcur->getField(0,4),"ME");
	assertEquals(secondcur->getField(0,5),"3.5");
	stdoutput.printf("\n");
	// with bind variables, should also be converted to an update
	cur->prepareQuery("insert into student values (null,?,?,?,?,?)");
	cur->inputBind("1","David");
	cur->inputBind("2","Muse");
	cur->inputBind("3","Junior");
	cur->inputBind("4","CS");
	cur->inputBind("5","3.0");
	assertTrue(cur->executeQuery());
	assertTrue(secondcur->sendQuery("select count(*) from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertTrue(secondcur->sendQuery("select * from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertEquals(secondcur->getField(0,1),"David");
	assertEquals(secondcur->getField(0,2),"Muse");
	assertEquals(secondcur->getField(0,3),"Junior");
	assertEquals(secondcur->getField(0,4),"CS");
	assertEquals(secondcur->getField(0,5),"3.0");
	stdoutput.printf("\n");
	// reexecute with bind variables, should also be converted to an update
	cur->inputBind("1","David");
	cur->inputBind("2","Muse");
	cur->inputBind("3","Senior");
	cur->inputBind("4","CS");
	cur->inputBind("5","2.5");
	assertTrue(cur->executeQuery());
	assertTrue(secondcur->sendQuery("select count(*) from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertTrue(secondcur->sendQuery("select * from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertEquals(secondcur->getField(0,1),"David");
	assertEquals(secondcur->getField(0,2),"Muse");
	assertEquals(secondcur->getField(0,3),"Senior");
	assertEquals(secondcur->getField(0,4),"CS");
	assertEquals(secondcur->getField(0,5),"2.5");
	stdoutput.printf("\n");
	// negative control: a bind variable immediately followed by more
	// text in the same value (?+1) rather than standing alone makes
	// bind-to-column mapping unreliable, so the upsert trigger must
	// bail out with an error on the original insert cursor rather
	// than silently converting to a corrupted update
	stdoutput.printf("UPSERT WITH BIND FOLLOWED BY MORE TEXT FAILS:\n");
	cur->prepareQuery("insert into student values (null,?,?,?+1,?,?)");
	cur->inputBind("1","David");
	cur->inputBind("2","Muse");
	cur->inputBind("3","2");
	cur->inputBind("4","CS");
	cur->inputBind("5","2.0");
	assertFalse(cur->executeQuery());
	assertTrue(secondcur->sendQuery("select count(*) from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertTrue(secondcur->sendQuery("select * from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertEquals(secondcur->getField(0,1),"David");
	assertEquals(secondcur->getField(0,2),"Muse");
	assertEquals(secondcur->getField(0,3),"Senior");
	assertEquals(secondcur->getField(0,4),"CS");
	assertEquals(secondcur->getField(0,5),"2.5");
	stdoutput.printf("\n");

	// upsert on a table whose name is quoted and contains a space
	// (regression test for #10127 - parseInsert() used to scan for the
	// first space to find the end of the table name, with no regard
	// for quoting, so a quoted table name that itself contains a space
	// got truncated; the upsert trigger looks up its config by that
	// extracted name at upsert.cpp:124, so a truncated name misses the
	// lookup and the trigger bails, leaving the raw duplicate-key error
	// from the second insert in place instead of acting on it)
	stdoutput.printf("UPSERT ON TABLE NAME WITH A SPACE:\n");
	cur->sendQuery("drop table `exam grades`");
	assertTrue(cur->sendQuery("create table `exam grades` ("
					"id int auto_increment, "
					"student varchar(20), "
					"course varchar(20), "
					"term varchar(20), "
					"score varchar(20), "
					"primary key (id), "
					"unique (student,course) "
					")"));
	stdoutput.printf("\n");
	// initial insert
	assertTrue(cur->sendQuery("insert into `exam grades` values "
				"(null,"
				"'Jane','Biology','Fall','88')"));
	assertTrue(secondcur->sendQuery("select count(*) from `exam grades`"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertTrue(secondcur->sendQuery("select * from `exam grades`"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertEquals(secondcur->getField(0,1),"Jane");
	assertEquals(secondcur->getField(0,2),"Biology");
	assertEquals(secondcur->getField(0,3),"Fall");
	assertEquals(secondcur->getField(0,4),"88");
	stdoutput.printf("\n");
	// a duplicate on (student,course) triggers the upsert trigger; with
	// the table name correctly extracted, the trigger finds "exam grades"
	// in its config and acts on the duplicate instead of leaving the raw
	// duplicate-key error in place
	assertFalse(cur->sendQuery("insert into `exam grades` values "
				"(null,"
				"'Jane','Biology','Spring','93')"));
	assertFalse(charstring::contains(cur->errorMessage(),"Duplicate entry"));
	assertTrue(secondcur->sendQuery("select count(*) from `exam grades`"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	assertTrue(cur->sendQuery("drop table `exam grades`"));

	assertTrue(cur->sendQuery("drop table student"));
	delete secondcur;
	secondcur=NULL;
	delete cur;
	delete con;
	stdoutput.printf("\n");

	reportTestStatus();

	return status;
}
