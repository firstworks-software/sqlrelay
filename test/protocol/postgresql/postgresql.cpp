// Copyright (c) David Muse
// See the file COPYING for more information.

#include <libpq-fe.h>
#include <config.h>
#include <rudiments/sys.h>
#include <rudiments/charstring.h>
#include <rudiments/process.h>
#include <rudiments/environment.h>
#include <rudiments/stringbuffer.h>
#include <rudiments/stdio.h>

#include "asserts.cpp"

PGconn	*pgconn;

int	main(int argc, char **argv) {

#ifdef HAVE_POSTGRESQL_PQEXECPREPARED

	const char	*host;
	const char	*port;
	const char	*user;
	const char	*password;
	const char	*db;

	// pass "native" to test a real postgresql instance instead of
	// sqlrelay's postgresql protocol
	bool	issqlrelay=!(argc==2 && !charstring::compare(argv[1],"native"));

	if (issqlrelay) {
		host="127.0.0.1";
		db="testuser";
	} else {
		// short hostname, matching the db the native odbc tests use
		char	*hostname=sys::getHostName();
		char	*dot=(char *)charstring::findFirstOrEnd(hostname,'.');
		*dot='\0';
		host="postgresql";
		db=hostname;
	}
	port="5432";
	user="testuser";
	password="testpassword";

	stdoutput.printf("PQresStatus:\n");
	assertEquals(PQresStatus(PGRES_EMPTY_QUERY),"PGRES_EMPTY_QUERY");
	assertEquals(PQresStatus(PGRES_COMMAND_OK),"PGRES_COMMAND_OK");
	assertEquals(PQresStatus(PGRES_TUPLES_OK),"PGRES_TUPLES_OK");
	assertEquals(PQresStatus(PGRES_COPY_OUT),"PGRES_COPY_OUT");
	assertEquals(PQresStatus(PGRES_COPY_IN),"PGRES_COPY_IN");
	assertEquals(PQresStatus(PGRES_BAD_RESPONSE),"PGRES_BAD_RESPONSE");
	assertEquals(PQresStatus(PGRES_NONFATAL_ERROR),"PGRES_NONFATAL_ERROR");
	assertEquals(PQresStatus(PGRES_FATAL_ERROR),"PGRES_FATAL_ERROR");
	stdoutput.printf("\n");

	// verify cleartext-method authentication (#8620): on the
	// cleartext listener the right password is accepted and a
	// wrong password is rejected
	if (issqlrelay) {
		stdoutput.printf("cleartext auth - right password:\n");
		PGconn	*ctok=PQsetdbLogin(host,"5433",NULL,NULL,
						db,user,password);
		assertEquals(PQstatus(ctok),CONNECTION_OK);
		PQfinish(ctok);
		stdoutput.printf("\n");

		stdoutput.printf("cleartext auth - wrong password:\n");
		PGconn	*ctbad=PQsetdbLogin(host,"5433",NULL,NULL,
						db,user,"wrongpassword");
		assertEquals(PQstatus(ctbad),CONNECTION_BAD);
		PQfinish(ctbad);
		stdoutput.printf("\n");
	}

	stdoutput.printf("PQstatus:\n");
	pgconn=PQsetdbLogin(host,port,NULL,NULL,db,user,password);
	assertEquals(PQstatus(pgconn),CONNECTION_OK);
	stdoutput.printf("\n");

	stdoutput.printf("PQdb:\n");
	assertEquals(PQdb(pgconn),db);
	stdoutput.printf("\n");

	stdoutput.printf("PQuser:\n");
	assertEquals(PQuser(pgconn),user);
	stdoutput.printf("\n");

	stdoutput.printf("PQpass:\n");
	assertEquals(PQpass(pgconn),password);
	stdoutput.printf("\n");

	stdoutput.printf("PQhost:\n");
	assertEquals(PQhost(pgconn),host);
	stdoutput.printf("\n");

	stdoutput.printf("PQport:\n");
	assertEquals(PQport(pgconn),
			(charstring::getLength(port)?port:(char *)"5432"));
	stdoutput.printf("\n");

	stdoutput.printf("PQtty:\n");
	assertEquals(PQtty(pgconn),"");
	stdoutput.printf("\n");

	stdoutput.printf("PQoptions:\n");
	assertEquals(PQoptions(pgconn),"");
	stdoutput.printf("\n");

	stdoutput.printf("PQstatus:\n");
	PQfinish(pgconn);
	char	conninfo[1024];
	charstring::printf(conninfo,sizeof(conninfo),
		"host='%s' port='%s' user='%s' password='%s' dbname='%s'",
						host,port,user,password,db);
	pgconn=PQconnectdb(conninfo);
	assertEquals(PQstatus(pgconn),CONNECTION_OK);
	stdoutput.printf("\n");

	stdoutput.printf("PQdb:\n");
	assertEquals(PQdb(pgconn),db);
	stdoutput.printf("\n");

	stdoutput.printf("PQuser:\n");
	assertEquals(PQuser(pgconn),user);
	stdoutput.printf("\n");

	stdoutput.printf("PQpass:\n");
	assertEquals(PQpass(pgconn),password);
	stdoutput.printf("\n");

	stdoutput.printf("PQhost:\n");
	assertEquals(PQhost(pgconn),host);
	stdoutput.printf("\n");

	stdoutput.printf("PQport:\n");
	assertEquals(PQport(pgconn),port);
	stdoutput.printf("\n");

	stdoutput.printf("PQtty:\n");
	assertEquals(PQtty(pgconn),"");
	stdoutput.printf("\n");

	stdoutput.printf("PQoptions:\n");
	assertEquals(PQoptions(pgconn),"");
	stdoutput.printf("\n");

#if 0
	stdoutput.printf("PQresetStart:\n");
	PQresetStart(pgconn);
	pgconn=PQconnectdb(conninfo);
	assertEquals(PQstatus(pgconn),CONNECTION_OK);
	stdoutput.printf("\n");
#endif

	const char	*query="drop table testtable";
	PGresult	*pgresult=PQexec(pgconn,query);
	PQclear(pgresult);

	stdoutput.printf("PQexec: create\n");
	query="create table testtable (testint int, testfloat float, testreal real, testsmallint smallint, testchar char(40), testvarchar varchar(40), testdate date, testtime time, testtimestamp timestamp)";
	pgresult=PQexec(pgconn,query);
	assertEquals(PQresultStatus(pgresult),PGRES_COMMAND_OK);
	PQclear(pgresult);
	stdoutput.printf("\n");

	stdoutput.printf("PQexec: insert\n");
	query="insert into testtable values (1,1.5,1.5,1,'testchar1','testvarchar1','01/01/2001','01:00:00',NULL)";
	pgresult=PQexec(pgconn,query);
	assertEquals(PQresultStatus(pgresult),PGRES_COMMAND_OK);
	assertEquals(PQcmdTuples(pgresult),"1");
	PQclear(pgresult);
	stdoutput.printf("\n");

	stdoutput.printf("PQprepare/PQexecPrepared: insert\n");
	//query="insert into testtable values (2,2.5,2.5,2,'testchar2','testvarchar2','01/01/2002','02:00:00',NULL)";
	query="insert into testtable values ($1,$2,$3,$4,$5,$6,$7,$8,$9)";
	pgresult=PQprepare(pgconn,"",query,9,NULL);
	assertEquals(PQresultStatus(pgresult),PGRES_COMMAND_OK);
	PQclear(pgresult);
	const char * const paramvalues[]={"2","2.5","2.5","2","testchar2","testvarchar2","01/01/2002","02:00:00",NULL};
	pgresult=PQexecPrepared(pgconn,"",9,paramvalues,NULL,NULL,0);
	assertEquals(PQresultStatus(pgresult),PGRES_COMMAND_OK);
	assertEquals(PQcmdTuples(pgresult),"1");
	PQclear(pgresult);
	stdoutput.printf("\n");

	stdoutput.printf("PQexec: select\n");
	query="select * from testtable";
	pgresult=PQexec(pgconn,query);
	assertEquals(PQresultStatus(pgresult),PGRES_TUPLES_OK);
	stdoutput.printf("\n");

	stdoutput.printf("PQnfields:\n");
	assertEquals(PQnfields(pgresult),9);
	stdoutput.printf("\n");

	stdoutput.printf("PQntuples:\n");
	assertEquals(PQntuples(pgresult),2);
	stdoutput.printf("\n");
	
	stdoutput.printf("PQfname:\n");
	assertEquals(PQfname(pgresult,0),"testint");
	assertEquals(PQfname(pgresult,1),"testfloat");
	assertEquals(PQfname(pgresult,2),"testreal");
	assertEquals(PQfname(pgresult,3),"testsmallint");
	assertEquals(PQfname(pgresult,4),"testchar");
	assertEquals(PQfname(pgresult,5),"testvarchar");
	assertEquals(PQfname(pgresult,6),"testdate");
	assertEquals(PQfname(pgresult,7),"testtime");
	assertEquals(PQfname(pgresult,8),"testtimestamp");
	stdoutput.printf("\n");
	
	stdoutput.printf("PQftype:\n");
	assertEquals(PQftype(pgresult,0),23);
	assertEquals(PQftype(pgresult,1),701);
	assertEquals(PQftype(pgresult,2),700);
	assertEquals(PQftype(pgresult,3),21);
	assertEquals(PQftype(pgresult,4),1042);
	assertEquals(PQftype(pgresult,5),1043);
	assertEquals(PQftype(pgresult,6),1082);
	assertEquals(PQftype(pgresult,7),1083);
	assertEquals(PQftype(pgresult,8),1114);
	stdoutput.printf("\n");
	
	stdoutput.printf("PQfsize:\n");
	assertEquals(PQfsize(pgresult,0),4);
	assertEquals(PQfsize(pgresult,1),8);
	assertEquals(PQfsize(pgresult,2),4);
	assertEquals(PQfsize(pgresult,3),2);
	assertEquals(PQfsize(pgresult,4),-1);
	assertEquals(PQfsize(pgresult,5),-1);
	assertEquals(PQfsize(pgresult,6),4);
	assertEquals(PQfsize(pgresult,7),8);
	assertEquals(PQfsize(pgresult,8),8);
	stdoutput.printf("\n");
	
	stdoutput.printf("PQfmod:\n");
	assertEquals(PQfmod(pgresult,0),-1);
	assertEquals(PQfmod(pgresult,1),-1);
	assertEquals(PQfmod(pgresult,2),-1);
	assertEquals(PQfmod(pgresult,3),-1);
	assertEquals(PQfmod(pgresult,4),44);
	assertEquals(PQfmod(pgresult,5),44);
	assertEquals(PQfmod(pgresult,6),-1);
	assertEquals(PQfmod(pgresult,7),-1);
	assertEquals(PQfmod(pgresult,8),-1);
	stdoutput.printf("\n");
	
	stdoutput.printf("PQbinaryTuples:\n");
	assertEquals(PQbinaryTuples(pgresult),0);
	stdoutput.printf("\n");

	stdoutput.printf("PQgetisnull:\n");
	assertEquals(PQgetisnull(pgresult,0,0),0);
	assertEquals(PQgetisnull(pgresult,0,1),0);
	assertEquals(PQgetisnull(pgresult,0,2),0);
	assertEquals(PQgetisnull(pgresult,0,3),0);
	assertEquals(PQgetisnull(pgresult,0,4),0);
	assertEquals(PQgetisnull(pgresult,0,5),0);
	assertEquals(PQgetisnull(pgresult,0,6),0);
	assertEquals(PQgetisnull(pgresult,0,7),0);
	// testtimestamp (column 8) is the one column inserted as NULL
	assertEquals(PQgetisnull(pgresult,0,8),1);
	assertEquals(PQgetisnull(pgresult,1,0),0);
	assertEquals(PQgetisnull(pgresult,1,1),0);
	assertEquals(PQgetisnull(pgresult,1,2),0);
	assertEquals(PQgetisnull(pgresult,1,3),0);
	assertEquals(PQgetisnull(pgresult,1,4),0);
	assertEquals(PQgetisnull(pgresult,1,5),0);
	assertEquals(PQgetisnull(pgresult,1,6),0);
	assertEquals(PQgetisnull(pgresult,1,7),0);
	assertEquals(PQgetisnull(pgresult,1,8),1);
	stdoutput.printf("\n");

	stdoutput.printf("PQgetvalue:\n");
	assertEquals(PQgetvalue(pgresult,0,0),"1");
	assertEquals(PQgetvalue(pgresult,0,1),"1.5");
	assertEquals(PQgetvalue(pgresult,0,2),"1.5");
	assertEquals(PQgetvalue(pgresult,0,3),"1");
	assertEquals(PQgetvalue(pgresult,0,4),"testchar1                               ");
	assertEquals(PQgetvalue(pgresult,0,5),"testvarchar1");
	assertEquals(PQgetvalue(pgresult,0,6),"2001-01-01");
	assertEquals(PQgetvalue(pgresult,0,7),"01:00:00");
	assertEquals(PQgetvalue(pgresult,1,0),"2");
	assertEquals(PQgetvalue(pgresult,1,1),"2.5");
	assertEquals(PQgetvalue(pgresult,1,2),"2.5");
	assertEquals(PQgetvalue(pgresult,1,3),"2");
	assertEquals(PQgetvalue(pgresult,1,4),"testchar2                               ");
	assertEquals(PQgetvalue(pgresult,1,5),"testvarchar2");
	assertEquals(PQgetvalue(pgresult,1,6),"2002-01-01");
	assertEquals(PQgetvalue(pgresult,1,7),"02:00:00");
	stdoutput.printf("\n");

	stdoutput.printf("PQgetlength:\n");
	assertEquals(PQgetlength(pgresult,0,0),1);
	assertEquals(PQgetlength(pgresult,0,1),3);
	assertEquals(PQgetlength(pgresult,0,2),3);
	assertEquals(PQgetlength(pgresult,0,3),1);
	assertEquals(PQgetlength(pgresult,0,4),40);
	assertEquals(PQgetlength(pgresult,0,5),12);
	assertEquals(PQgetlength(pgresult,0,6),10);
	assertEquals(PQgetlength(pgresult,0,7),8);
	assertEquals(PQgetlength(pgresult,1,0),1);
	assertEquals(PQgetlength(pgresult,1,1),3);
	assertEquals(PQgetlength(pgresult,1,2),3);
	assertEquals(PQgetlength(pgresult,1,3),1);
	assertEquals(PQgetlength(pgresult,1,4),40);
	assertEquals(PQgetlength(pgresult,1,5),12);
	assertEquals(PQgetlength(pgresult,1,6),10);
	assertEquals(PQgetlength(pgresult,1,7),8);
	stdoutput.printf("\n");

	PQclear(pgresult);

	query="drop table testtable";
	pgresult=PQexec(pgconn,query);
	PQclear(pgresult);

	PQfinish(pgconn);
#endif

	reportTestStatus();
	return status;
}
