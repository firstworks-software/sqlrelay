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

	stdoutput.printf("CONVERTSLASHESCAPE:\n");

	con=new sqlrconnection("sqlrelay",9030,
				"/tmp/postgresqlconvertslashescape.socket",
				"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);

	cur->sendQuery("drop table ceescapetest");
	assertTrue(cur->sendQuery("create table ceescapetest ("
					"val varchar(20)"
					")"));
	stdoutput.printf("\n");


	// a query written assuming mysql/mariadb-style backslash-escaping
	// (\'), proxied to postgresql, which doesn't accept backslash as a
	// quote-escape.  Without convertslashescape, the backend would read
	// the backslash as an ordinary character and the quote right after
	// it as the real closing quote, corrupting the rest of the query
	// (see the plain postgresqlslashescape test).  With
	// convertslashescape="yes" on this instance's normalize
	// translation, the \' should be rewritten to '' before it reaches
	// postgresql, so the statement runs as intended and the value comes
	// back with a real single quote in it.
	stdoutput.printf("INSERT WITH BACKSLASH-ESCAPED QUOTE:\n");
	assertTrue(cur->sendQuery(
			"insert into ceescapetest values ('a\\'b')"));
	assertTrue(cur->sendQuery(
			"select val from ceescapetest"));
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"a'b");
	stdoutput.printf("\n");


	// a query that already uses the doubled-quote form should be left
	// alone - it's already the form every backend accepts, there's
	// nothing to convert
	stdoutput.printf("INSERT WITH DOUBLED-QUOTE ESCAPE:\n");
	assertTrue(cur->sendQuery(
			"insert into ceescapetest values ('c''d')"));
	assertTrue(cur->sendQuery(
			"select val from ceescapetest where val='c''d'"));
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"c'd");
	stdoutput.printf("\n");


	// an escaped backslash (\\, two literal backslash characters in the
	// query text) isn't a quote-escape - convertslashescape recognizes
	// it only to leave it alone, the same two characters going in as
	// coming out, and postgresql (which doesn't treat backslash
	// specially in a literal) stores both of them
	stdoutput.printf("INSERT WITH ESCAPED BACKSLASH:\n");
	assertTrue(cur->sendQuery(
			"insert into ceescapetest values ('e\\\\f')"));
	assertTrue(cur->sendQuery(
			"select val from ceescapetest where val like 'e%f'"));
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"e\\\\f");
	stdoutput.printf("\n");


	assertTrue(cur->sendQuery("drop table ceescapetest"));
	delete cur;
	delete con;
	stdoutput.printf("\n");

	reportTestStatus();

	return status;
}
