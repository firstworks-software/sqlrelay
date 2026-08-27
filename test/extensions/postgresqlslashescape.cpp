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

	stdoutput.printf("SLASHESCAPE:\n");

	con=new sqlrconnection("sqlrelay",9035,
					"/tmp/postgresqlslashescape.socket",
					"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);


	// this instance configures normalize with no explicit slashescape
	// attribute, so postgresql should get its own default (backslash
	// is an ordinary character, not a quote-escape)
	cur->sendQuery("drop table seescapetest");
	assertTrue(cur->sendQuery("create table seescapetest ("
					"val varchar(20)"
					")"));
	assertTrue(cur->sendQuery(
			"insert into seescapetest values ('a_b')"));
	assertTrue(cur->sendQuery(
			"insert into seescapetest values ('axb')"));
	stdoutput.printf("\n");


	// oracle's "like ... escape '\'" construct - backslash as the
	// escape character for the "_" wildcard.  Before this fix,
	// normalize would consume the escape clause's own closing quote as
	// an escaped quote, leaving the literal open and swallowing the
	// rest of the query.  If that happened here, this query would fail
	// outright rather than return the wrong row.
	stdoutput.printf("LIKE ... ESCAPE '\\':\n");
	assertTrue(cur->sendQuery(
			"select val from seescapetest "
			"where val like 'a\\_b' escape '\\' "
			"order by val"));
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"a_b");
	stdoutput.printf("\n");


	// the like operand can be a bind variable instead of a literal - the
	// mangled part is the escape clause's own literal, which doesn't
	// care what precedes it
	stdoutput.printf("LIKE (BIND VARIABLE) ... ESCAPE '\\':\n");
	cur->prepareQuery(
			"select val from seescapetest "
			"where val like :pat escape '\\' "
			"order by val");
	cur->inputBind("pat","a\\_b");
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"a_b");
	stdoutput.printf("\n");


	// a literal ending in a backslash, independent of the escape
	// keyword - the general shape of the bug.  Before the fix, the
	// closing quote below would have been consumed as an escaped
	// quote, so the query would never find a real closing quote and
	// would fail rather than round-trip the value.
	stdoutput.printf("VALUE ENDING IN A BACKSLASH:\n");
	assertTrue(cur->sendQuery(
			"insert into seescapetest values ('trailing\\')"));
	assertTrue(cur->sendQuery(
			"select val from seescapetest "
			"where val='trailing\\'"));
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"trailing\\");
	stdoutput.printf("\n");


	assertTrue(cur->sendQuery("drop table seescapetest"));
	delete cur;
	delete con;
	stdoutput.printf("\n");

	reportTestStatus();

	return status;
}
