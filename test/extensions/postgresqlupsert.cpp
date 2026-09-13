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
	// negative control: a bind variable embedded inside a value
	// expression (UPPER($3)) rather than standing alone should make
	// bind-to-column mapping unreliable, so the upsert trigger must
	// bail out with an error on the original insert cursor rather
	// than silently converting to a corrupted update
	stdoutput.printf("UPSERT WITH BIND INSIDE EXPRESSION FAILS:\n");
	cur->prepareQuery("insert into student values "
				"(nextval('student_id'),"
				"$1,$2,UPPER($3),$4,$5)");
	cur->inputBind("1","David");
	cur->inputBind("2","Muse");
	cur->inputBind("3","grad");
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
	// negative control: a bind variable immediately followed by more
	// text in the same value ($3||'+1') rather than standing alone is
	// the same unreliable-mapping case as above, just without a
	// wrapping function call - the upsert trigger must bail out here
	// too instead of treating $3 as a whole-column bind
	stdoutput.printf("UPSERT WITH BIND FOLLOWED BY MORE TEXT FAILS:\n");
	cur->prepareQuery("insert into student values "
				"(nextval('student_id'),"
				"$1,$2,$3||'+1',$4,$5)");
	cur->inputBind("1","David");
	cur->inputBind("2","Muse");
	cur->inputBind("3","grad");
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

	// upsert with a leading space before each bind marker; regression
	// test for #10124 - isBind() only skipped whitespace after the bind
	// marker, so " $1" (ordinary sql formatting - a space after each
	// comma) wasn't recognized as a whole-token bind, and internal
	// bind-to-column/where-clause map lookups keyed on the untrimmed
	// bind name failed too, so the upsert trigger bailed out with a
	// "bind variable was found inside a value expression" error even
	// though the value really is just a bind standing alone.  this
	// instance has no normalize translation, so the leading spaces
	// reach the trigger unstripped
	stdoutput.printf("UPSERT WITH LEADING SPACE BEFORE BIND MARKER:\n");
	con=new sqlrconnection("sqlrelay",9040,
					"/tmp/postgresqlupsertnonormalize.socket",
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
	stdoutput.printf("\n");
	// duplicate insert, with a leading space before each bind marker,
	// should still be converted to an update
	cur->prepareQuery("insert into student values "
				"(nextval('student_id'),"
				" $1, $2, $3, $4, $5)");
	cur->inputBind("1","David");
	cur->inputBind("2","Muse");
	cur->inputBind("3","Sophomore");
	cur->inputBind("4","ME");
	cur->inputBind("5","3.5");
	assertTrue(cur->executeQuery());
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
	assertTrue(cur->sendQuery("drop table student"));
	assertTrue(cur->sendQuery("drop sequence student_id"));
	delete secondcur;
	secondcur=NULL;
	delete cur;
	cur=NULL;
	delete con;
	con=NULL;
	stdoutput.printf("\n");

	// upsert with an explicit column list and a leading space after each
	// comma; regression test for #10132 - getColumnsFromInsertQuery()
	// didn't trim whitespace from parsed column-name tokens, so
	// "(id, firstname, lastname)" produced tokens " firstname"/
	// " lastname" that never exact-matched the trigger's configured
	// <column name="firstname"/> and <primarykey name="id"/> entries,
	// and the trigger failed to recognize the columns/primary key for
	// this insert.  this instance has no normalize translation, so the
	// leading spaces reach the trigger unstripped
	stdoutput.printf("UPSERT WITH EXPLICIT COLUMN LIST AND "
				"LEADING SPACE:\n");
	con=new sqlrconnection("sqlrelay",9040,
					"/tmp/postgresqlupsertnonormalize.socket",
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
	assertTrue(cur->sendQuery("insert into student "
				"(id, firstname, lastname, year, major, gpa) "
				"values (nextval('student_id'),"
				"'David','Muse','Freshman','ME','4.0')"));
	assertTrue(secondcur->sendQuery("select count(*) from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	// duplicate insert, with an explicit column list and a leading
	// space after each comma, should still be converted to an update
	cur->prepareQuery("insert into student "
				"(id, firstname, lastname, year, major, gpa) "
				"values (nextval('student_id'),"
				" $1, $2, $3, $4, $5)");
	cur->inputBind("1","David");
	cur->inputBind("2","Muse");
	cur->inputBind("3","Sophomore");
	cur->inputBind("4","ME");
	cur->inputBind("5","3.5");
	assertTrue(cur->executeQuery());
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
	assertTrue(cur->sendQuery("drop table student"));
	assertTrue(cur->sendQuery("drop sequence student_id"));
	delete secondcur;
	secondcur=NULL;
	delete cur;
	cur=NULL;
	delete con;
	con=NULL;
	stdoutput.printf("\n");

	// upsert with a postgres dollar-quoted string literal standing in for
	// one of the trigger's configured where-clause columns (firstname,
	// lastname - see the "student" table's <column> entries in
	// postgresqlupsert.conf); regression test for #10135.
	// wholeBindVariable() classified a whole value token that was
	// alphanumeric-and-'$' start to end as a bind name, and a
	// dollar-quoted literal like $$abc$$ or $tag$abc$tag$ matches that
	// shape too.  Since firstname/lastname feed the generated update's
	// where clause, misclassifying one of them as a bind sent
	// convertInsertToUpdate() (upsert.cpp) looking up a bind name that
	// was never actually bound, producing "firstname= and lastname=..."
	// with no value - a query syntax error - instead of a real update.
	// this instance has no normalize translation, so the dollar-quoted
	// text reaches the trigger with its case intact (normalize doesn't
	// recognize dollar-quoting and would otherwise lowercase it
	// character by character, same as any other unquoted text)
	stdoutput.printf("UPSERT WITH POSTGRES DOLLAR-QUOTED LITERAL "
				"WHERE-CLAUSE VALUE:\n");
	con=new sqlrconnection("sqlrelay",9040,
					"/tmp/postgresqlupsertnonormalize.socket",
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
	stdoutput.printf("\n");
	// duplicate insert (same firstname/lastname, so postgresql reports a
	// duplicate key and the trigger converts this to an update), with
	// firstname and lastname given as dollar-quoted literals - one $$...$$,
	// one $tag$...$tag$ - rather than binds, and bind numbering kept
	// contiguous (there's no bind for a where-clause column here at all)
	cur->prepareQuery("insert into student values "
				"(nextval('student_id'),"
				"$$David$$,$tag$Muse$tag$,$1,$2,$3)");
	cur->inputBind("1","Sophomore");
	cur->inputBind("2","ME");
	cur->inputBind("3","3.5");
	assertTrue(cur->executeQuery());
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
	assertTrue(cur->sendQuery("drop table student"));
	assertTrue(cur->sendQuery("drop sequence student_id"));
	delete secondcur;
	secondcur=NULL;
	delete cur;
	cur=NULL;
	delete con;
	con=NULL;
	stdoutput.printf("\n");

	// regression test for #10137: getFirstValuesFromInsertQuery() split the
	// VALUES list on top-level commas/close-parens without understanding
	// postgres dollar-quoting, so a comma or close-paren inside a
	// dollar-quoted value's body was treated as a real delimiter instead
	// of quoted text
	stdoutput.printf("UPSERT WITH POSTGRES DOLLAR-QUOTED LITERAL VALUE "
				"CONTAINING COMMA/PAREN:\n");
	con=new sqlrconnection("sqlrelay",9040,
					"/tmp/postgresqlupsertnonormalize.socket",
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
				"'Da,vid','Mu)se','Freshman','ME','4.0')"));
	assertTrue(secondcur->sendQuery("select count(*) from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	// duplicate insert (same firstname/lastname, so postgresql reports a
	// duplicate key and the trigger converts this to an update), with
	// firstname and lastname given as dollar-quoted literals whose bodies
	// contain a comma and a close-paren - one $$...$$, one $tag$...$tag$
	cur->prepareQuery("insert into student values "
				"(nextval('student_id'),"
				"$$Da,vid$$,$tag$Mu)se$tag$,$1,$2,$3)");
	cur->inputBind("1","Sophomore");
	cur->inputBind("2","ME");
	cur->inputBind("3","3.5");
	assertTrue(cur->executeQuery());
	assertTrue(secondcur->sendQuery("select count(*) from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertTrue(secondcur->sendQuery("select * from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertEquals(secondcur->getField(0,1),"Da,vid");
	assertEquals(secondcur->getField(0,2),"Mu)se");
	assertEquals(secondcur->getField(0,3),"Sophomore");
	assertEquals(secondcur->getField(0,4),"ME");
	assertEquals(secondcur->getField(0,5),"3.5");
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

	reportTestStatus();

	return status;
}
