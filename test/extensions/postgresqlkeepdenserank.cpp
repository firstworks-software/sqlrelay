// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/process.h>
#include <sqlrelay/sqlrclient.h>
#include <stdlib.h>
#include <stdio.h>

#include "../c++/asserts.cpp"

sqlrconnection	*con=NULL;
sqlrcursor	*cur=NULL;
sqlrconnection	*secondcon=NULL;
sqlrcursor	*secondcur=NULL;

int main(int argc, char **argv) {

	stdoutput.printf("KEEP DENSE_RANK TO ARRAY_AGG:\n");

	con=new sqlrconnection("sqlrelay",9034,
					"/tmp/postgresqlkeepdenserank.socket",
					"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);


	// create test table
	//
	// group 1 has a two-row tie at its highest sortkey (10), with
	// different measures, so "aggregate over the whole tied set" is
	// what's being tested, rather than "pick one of the tied rows"
	//
	// group 2 has a two-row tie at its highest sortkey (7) and another
	// at its lowest (3), each with one null measure, so the null-handling
	// of the tie-break can't wrongly win over the real value
	cur->sendQuery("drop table kdrtest");
	assertTrue(cur->sendQuery("create table kdrtest ("
					"grp int, "
					"measure varchar(20), "
					"sortkey int "
					")"));
	assertTrue(cur->sendQuery("insert into kdrtest values (1,'a',10)"));
	assertTrue(cur->sendQuery("insert into kdrtest values (1,'b',10)"));
	assertTrue(cur->sendQuery("insert into kdrtest values (1,'c',5)"));
	assertTrue(cur->sendQuery("insert into kdrtest values (2,'d',7)"));
	assertTrue(cur->sendQuery("insert into kdrtest values (2,null,7)"));
	assertTrue(cur->sendQuery("insert into kdrtest values (2,'e',3)"));
	assertTrue(cur->sendQuery("insert into kdrtest values (2,null,3)"));
	stdoutput.printf("\n");


	// group 1's highest sortkey is 10, kept rows are 'a' and 'b'
	// group 2's highest sortkey is 7, kept rows are 'd' and null,
	// and max ignores the null
	stdoutput.printf("MAX ... DENSE_RANK LAST:\n");
	assertTrue(cur->sendQuery(
		"select grp,"
		" max(measure) keep (dense_rank last order by sortkey) "
		"from kdrtest group by grp order by grp"));
	assertEquals(cur->rowCount(),2);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"b");
	assertEquals(cur->getField(1,(uint32_t)0),"2");
	assertEquals(cur->getField(1,1),"d");
	stdoutput.printf("\n");


	// same kept rows, but min of 'a','b' is 'a', and min of 'd',null
	// is 'd', since min ignores the null too
	stdoutput.printf("MIN ... DENSE_RANK LAST:\n");
	assertTrue(cur->sendQuery(
		"select grp,"
		" min(measure) keep (dense_rank last order by sortkey) "
		"from kdrtest group by grp order by grp"));
	assertEquals(cur->rowCount(),2);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"a");
	assertEquals(cur->getField(1,(uint32_t)0),"2");
	assertEquals(cur->getField(1,1),"d");
	stdoutput.printf("\n");


	// group 1's lowest sortkey is 5, the only kept row is 'c'
	// group 2's lowest sortkey is 3, kept rows are 'e' and null
	stdoutput.printf("MAX ... DENSE_RANK FIRST:\n");
	assertTrue(cur->sendQuery(
		"select grp,"
		" max(measure) keep (dense_rank first order by sortkey) "
		"from kdrtest group by grp order by grp"));
	assertEquals(cur->rowCount(),2);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"c");
	assertEquals(cur->getField(1,(uint32_t)0),"2");
	assertEquals(cur->getField(1,1),"e");
	stdoutput.printf("\n");


	stdoutput.printf("MIN ... DENSE_RANK FIRST:\n");
	assertTrue(cur->sendQuery(
		"select grp,"
		" min(measure) keep (dense_rank first order by sortkey) "
		"from kdrtest group by grp order by grp"));
	assertEquals(cur->rowCount(),2);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"c");
	assertEquals(cur->getField(1,(uint32_t)0),"2");
	assertEquals(cur->getField(1,1),"e");
	stdoutput.printf("\n");


	// an explicit asc is the same as no direction at all
	stdoutput.printf("MAX ... DENSE_RANK LAST ... ASC:\n");
	assertTrue(cur->sendQuery(
		"select grp,"
		" max(measure) keep (dense_rank last order by sortkey asc) "
		"from kdrtest group by grp order by grp"));
	assertEquals(cur->rowCount(),2);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"b");
	assertEquals(cur->getField(1,(uint32_t)0),"2");
	assertEquals(cur->getField(1,1),"d");
	stdoutput.printf("\n");


	// with the sort inverted, "last" keeps the rows with the lowest
	// sortkey - 5 for group 1 and 3 for group 2
	stdoutput.printf("MAX ... DENSE_RANK LAST ... DESC:\n");
	assertTrue(cur->sendQuery(
		"select grp,"
		" max(measure) keep (dense_rank last order by sortkey desc) "
		"from kdrtest group by grp order by grp"));
	assertEquals(cur->rowCount(),2);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"c");
	assertEquals(cur->getField(1,(uint32_t)0),"2");
	assertEquals(cur->getField(1,1),"e");
	stdoutput.printf("\n");


	stdoutput.printf("MIN ... DENSE_RANK LAST ... DESC:\n");
	assertTrue(cur->sendQuery(
		"select grp,"
		" min(measure) keep (dense_rank last order by sortkey desc) "
		"from kdrtest group by grp order by grp"));
	assertEquals(cur->rowCount(),2);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"c");
	assertEquals(cur->getField(1,(uint32_t)0),"2");
	assertEquals(cur->getField(1,1),"e");
	stdoutput.printf("\n");


	// with the sort inverted, "first" keeps the rows with the highest
	// sortkey - 10 for group 1 and 7 for group 2
	stdoutput.printf("MAX ... DENSE_RANK FIRST ... DESC:\n");
	assertTrue(cur->sendQuery(
		"select grp,"
		" max(measure) keep (dense_rank first order by sortkey desc) "
		"from kdrtest group by grp order by grp"));
	assertEquals(cur->rowCount(),2);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"b");
	assertEquals(cur->getField(1,(uint32_t)0),"2");
	assertEquals(cur->getField(1,1),"d");
	stdoutput.printf("\n");


	stdoutput.printf("MIN ... DENSE_RANK FIRST ... DESC:\n");
	assertTrue(cur->sendQuery(
		"select grp,"
		" min(measure) keep (dense_rank first order by sortkey desc) "
		"from kdrtest group by grp order by grp"));
	assertEquals(cur->rowCount(),2);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"a");
	assertEquals(cur->getField(1,(uint32_t)0),"2");
	assertEquals(cur->getField(1,1),"d");
	stdoutput.printf("\n");


	// two of them in one statement, one in the select list and one in
	// the having clause.  group 1's min-keep-last is 'a' and group 2's
	// is 'd', so only group 1 survives the having clause, and its
	// max-keep-last is 'b'
	stdoutput.printf("SELECT LIST AND HAVING CLAUSE:\n");
	assertTrue(cur->sendQuery(
		"select grp,"
		" max(measure) keep (dense_rank last order by sortkey) as m "
		"from kdrtest "
		"group by grp "
		"having min(measure) "
		" keep (dense_rank last order by sortkey) = 'a' "
		"order by grp"));
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"b");
	stdoutput.printf("\n");


	// mymax isn't max - if the identifier boundary is respected then
	// this is left alone, and postgresql has no keep clause of its own
	// to run it against
	stdoutput.printf("PASSTHROUGH - MYMAX:\n");
	assertFalse(cur->sendQuery(
		"select grp,"
		" mymax(measure) keep (dense_rank last order by sortkey) "
		"from kdrtest group by grp order by grp"));
	stdoutput.printf("\n");


	// sum is outside of the max/min scope of the translation, so this
	// is left alone too, and postgresql has no keep clause of its own
	// to run it against
	stdoutput.printf("PASSTHROUGH - SUM:\n");
	assertFalse(cur->sendQuery(
		"select grp,"
		" sum(sortkey) keep (dense_rank last order by sortkey) "
		"from kdrtest group by grp order by grp"));
	stdoutput.printf("\n");


	// clean up
	assertTrue(cur->sendQuery("drop table kdrtest"));
	delete cur;
	delete con;
	stdoutput.printf("\n");

	reportTestStatus();

	return status;
}
