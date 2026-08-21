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

	stdoutput.printf("LISTAGG TO STRING_AGG:\n");

	con=new sqlrconnection("sqlrelay",9023,"/tmp/postgresqlstringagg.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);


	// create test table
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery("create table testtable ("
					"id int, "
					"grp varchar(20), "
					"name varchar(20), "
					"nullable varchar(20) "
					")"));
	assertTrue(cur->sendQuery("insert into testtable values "
					"(1,'a','alpha','x')"));
	assertTrue(cur->sendQuery("insert into testtable values "
					"(2,'a','bravo',null)"));
	assertTrue(cur->sendQuery("insert into testtable values "
					"(3,'a','charlie','y')"));
	assertTrue(cur->sendQuery("insert into testtable values "
					"(4,'b','delta','z')"));
	assertTrue(cur->sendQuery("insert into testtable values "
					"(5,'b','echo',null)"));
	stdoutput.printf("\n");


	stdoutput.printf("BASIC ASC:\n");
	assertTrue(cur->sendQuery(
		"select grp,"
		" listagg(name,',') within group (order by id asc) "
		"from testtable group by grp order by grp"));
	assertEquals(cur->getField(0,(uint32_t)0),"a");
	assertEquals(cur->getField(0,1),"alpha,bravo,charlie");
	assertEquals(cur->getField(1,(uint32_t)0),"b");
	assertEquals(cur->getField(1,1),"delta,echo");
	stdoutput.printf("\n");


	stdoutput.printf("BASIC DESC:\n");
	assertTrue(cur->sendQuery(
		"select grp,"
		" listagg(name,',') within group (order by id desc) "
		"from testtable group by grp order by grp"));
	assertEquals(cur->getField(0,(uint32_t)0),"a");
	assertEquals(cur->getField(0,1),"charlie,bravo,alpha");
	assertEquals(cur->getField(1,(uint32_t)0),"b");
	assertEquals(cur->getField(1,1),"echo,delta");
	stdoutput.printf("\n");


	stdoutput.printf("NON-STRING EXPRESSION:\n");
	assertTrue(cur->sendQuery(
		"select grp,"
		" listagg(id,'-') within group (order by id asc) "
		"from testtable group by grp order by grp"));
	assertEquals(cur->getField(0,(uint32_t)0),"a");
	assertEquals(cur->getField(0,1),"1-2-3");
	assertEquals(cur->getField(1,(uint32_t)0),"b");
	assertEquals(cur->getField(1,1),"4-5");
	stdoutput.printf("\n");


	stdoutput.printf("EXPRESSION WITH EMBEDDED COMMA:\n");
	assertTrue(cur->sendQuery(
		"select grp,"
		" listagg(coalesce(nullable,'?'),',') "
		" within group (order by id asc) "
		"from testtable group by grp order by grp"));
	assertEquals(cur->getField(0,(uint32_t)0),"a");
	assertEquals(cur->getField(0,1),"x,?,y");
	assertEquals(cur->getField(1,(uint32_t)0),"b");
	assertEquals(cur->getField(1,1),"z,?");
	stdoutput.printf("\n");


	stdoutput.printf("MULTIPLE LISTAGG:\n");
	assertTrue(cur->sendQuery(
		"select grp,"
		" listagg(name,',') within group (order by id asc),"
		" listagg(id,'|') within group (order by id desc) "
		"from testtable group by grp order by grp"));
	assertEquals(cur->getField(0,(uint32_t)0),"a");
	assertEquals(cur->getField(0,1),"alpha,bravo,charlie");
	assertEquals(cur->getField(0,2),"3|2|1");
	assertEquals(cur->getField(1,(uint32_t)0),"b");
	assertEquals(cur->getField(1,1),"delta,echo");
	assertEquals(cur->getField(1,2),"5|4");
	stdoutput.printf("\n");


	stdoutput.printf("NO SEPARATOR:\n");
	assertTrue(cur->sendQuery(
		"select grp,"
		" listagg(name) within group (order by id asc) "
		"from testtable group by grp order by grp"));
	assertEquals(cur->getField(0,(uint32_t)0),"a");
	assertEquals(cur->getField(0,1),"alphabravocharlie");
	assertEquals(cur->getField(1,(uint32_t)0),"b");
	assertEquals(cur->getField(1,1),"deltaecho");
	stdoutput.printf("\n");


	stdoutput.printf("DISTINCT:\n");
	assertTrue(cur->sendQuery(
		"select listagg(distinct grp,',') "
		" within group (order by grp) "
		"from testtable"));
	assertEquals(cur->getField(0,(uint32_t)0),"a,b");
	stdoutput.printf("\n");


	stdoutput.printf("DISTINCT DESC:\n");
	assertTrue(cur->sendQuery(
		"select listagg(distinct grp,',') "
		" within group (order by grp desc) "
		"from testtable"));
	assertEquals(cur->getField(0,(uint32_t)0),"b,a");
	stdoutput.printf("\n");


	stdoutput.printf("DISTINCT NO SEPARATOR:\n");
	assertTrue(cur->sendQuery(
		"select listagg(distinct grp) "
		" within group (order by grp) "
		"from testtable"));
	assertEquals(cur->getField(0,(uint32_t)0),"ab");
	stdoutput.printf("\n");


	stdoutput.printf("DISTINCT EXPRESSION:\n");
	assertTrue(cur->sendQuery(
		"select grp,"
		" listagg(distinct coalesce(nullable,'?'),',') "
		" within group (order by coalesce(nullable,'?')) "
		"from testtable group by grp order by grp"));
	assertEquals(cur->getField(0,(uint32_t)0),"a");
	assertEquals(cur->getField(0,1),"?,x,y");
	assertEquals(cur->getField(1,(uint32_t)0),"b");
	assertEquals(cur->getField(1,1),"?,z");
	stdoutput.printf("\n");


	stdoutput.printf("LISTAGG INSIDE STRING LITERAL:\n");
	assertTrue(cur->sendQuery(
		"select 'listagg(foo,bar) within group (order by baz)' "
		"as lit"));
	assertEquals(cur->getField(0,(uint32_t)0),
		"listagg(foo,bar) within group (order by baz)");
	stdoutput.printf("\n");


	// clean up
	assertTrue(cur->sendQuery("drop table testtable"));
	delete cur;
	delete con;
	stdoutput.printf("\n");

	reportTestStatus();

	return status;
}
