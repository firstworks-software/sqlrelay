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
	con=new sqlrconnection("sqlrelay",9020,"/tmp/postgresqlupsert.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);
	secondcur=new sqlrcursor(con);
	cur->sendQuery("drop table student");
	cur->sendQuery("drop sequence student_id");
	assertTrue(cur->sendQuery("create sequence student_id"));
	assertTrue(cur->sendQuery("create table student ("
					"id int, "
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
				"(nextval('student_id'),"
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
				"(nextval('student_id'),"
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
	cur->prepareQuery("insert into student values "
				"(nextval('student_id'),"
				"$1,$2,$3,$4,$5)");
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
	assertTrue(cur->sendQuery("drop table student"));
	assertTrue(cur->sendQuery("drop sequence student_id"));
	delete secondcur;
	secondcur=NULL;
	delete cur;
	cur=NULL;
	delete con;
	con=NULL;
	stdoutput.printf("\n");

	// upsert via sqlstate - this instance's trigger is configured with a
	// string= attribute that can never match ("this string never
	// matches"), so recognizing the duplicate-key error below relies
	// entirely on the new sqlstate= attribute (23505, postgresql's
	// unique_violation)
	stdoutput.printf("UPSERT VIA SQLSTATE:\n");
	con=new sqlrconnection("sqlrelay",9036,
					"/tmp/postgresqlupsertsqlstate.socket",
					"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);
	secondcur=new sqlrcursor(con);
	cur->sendQuery("drop table student2");
	cur->sendQuery("drop sequence student2_id");
	assertTrue(cur->sendQuery("create sequence student2_id"));
	assertTrue(cur->sendQuery("create table student2 ("
					"id int, "
					"firstname varchar(20) not null, "
					"lastname varchar(20), "
					"year varchar(20), "
					"major varchar(20), "
					"gpa varchar(20), "
					"primary key (id), "
					"unique (firstname,lastname) "
					")"));
	stdoutput.printf("\n");
	// initial insert
	assertTrue(cur->sendQuery("insert into student2 values "
				"(nextval('student2_id'),"
				"'David','Muse','Freshman','ME','4.0')"));
	assertTrue(secondcur->sendQuery("select count(*) from student2"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	// duplicate insert, reports sqlstate 23505 - should be converted to
	// an update via the sqlstate match alone
	assertTrue(cur->sendQuery("insert into student2 values "
				"(nextval('student2_id'),"
				"'David','Muse','Sophomore','ME','3.5')"));
	assertTrue(secondcur->sendQuery("select count(*) from student2"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertTrue(secondcur->sendQuery("select * from student2"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertEquals(secondcur->getField(0,1),"David");
	assertEquals(secondcur->getField(0,2),"Muse");
	assertEquals(secondcur->getField(0,3),"Sophomore");
	assertEquals(secondcur->getField(0,4),"ME");
	assertEquals(secondcur->getField(0,5),"3.5");
	stdoutput.printf("\n");

	// negative control: a not-null violation (sqlstate 23502) on the
	// same table must never be mistaken for the configured 23505
	// (unique_violation) - the insert should just fail, not be
	// converted to an update
	stdoutput.printf("UPSERT VIA SQLSTATE DOES NOT MASK "
				"UNRELATED ERRORS:\n");
	assertFalse(cur->sendQuery("insert into student2 values "
				"(nextval('student2_id'),"
				"NULL,'Nobody','Freshman','ME','4.0')"));
	assertTrue(secondcur->sendQuery("select count(*) from student2"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");

	assertTrue(cur->sendQuery("drop table student2"));
	assertTrue(cur->sendQuery("drop sequence student2_id"));
	delete secondcur;
	secondcur=NULL;
	delete cur;
	cur=NULL;
	delete con;
	con=NULL;
	stdoutput.printf("\n");

	reportTestStatus();

	return status;
}
