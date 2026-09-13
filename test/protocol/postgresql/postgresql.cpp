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

#ifdef HAVE_POSTGRESQL_PQEXECPREPARED
// append a big-endian uint16 to a byte buffer, advancing pos
static void appendUint16BE(unsigned char *buf, size_t *pos, uint16_t val) {
	buf[(*pos)++]=(unsigned char)((val>>8)&0xff);
	buf[(*pos)++]=(unsigned char)(val&0xff);
}
#endif

int main(int argc, char **argv) {

#ifdef HAVE_POSTGRESQL_PQEXECPREPARED

	const char	*host;
	const char	*port;
	const char	*cleartextport;
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
	cleartextport="5433";
	user="testuser";
	password="testpassword";

	// neither postgresqlprotocol listener has to be on its default port -
	// two sessions running this suite at once can't both have 5432/5433.
	// POSTGRESQLPROTOCOLPORT1 and POSTGRESQLPROTOCOLPORT2 name the ports
	// they actually ended up on; they're the same variables
	// test/sqlrelay.conf.d/postgresqlprotocol.conf.in's
	// @POSTGRESQLPROTOCOLPORT1@ and @POSTGRESQLPROTOCOLPORT2@ are
	// generated from, so one pair of values drives both ends.  unset
	// means 5432 and 5433, as before.
	if (issqlrelay) {
		const char	*portoverride=
			environment::getValue("POSTGRESQLPROTOCOLPORT1");
		if (!charstring::isNullOrEmpty(portoverride)) {
			port=portoverride;
		}
		const char	*cleartextportoverride=
			environment::getValue("POSTGRESQLPROTOCOLPORT2");
		if (!charstring::isNullOrEmpty(cleartextportoverride)) {
			cleartextport=cleartextportoverride;
		}
	}

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
		PGconn	*ctok=PQsetdbLogin(host,cleartextport,NULL,NULL,
						db,user,password);
		assertEquals(PQstatus(ctok),CONNECTION_OK);
		PQfinish(ctok);
		stdoutput.printf("\n");

		stdoutput.printf("cleartext auth - wrong password:\n");
		PGconn	*ctbad=PQsetdbLogin(host,cleartextport,NULL,NULL,
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
	assertEquals(PQport(pgconn),port);
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

	// #9447: a null bind to a bytea/text column previously lost its
	// type on the wire between sqlr-listener and sqlr-connection.
	// exercising the fix requires the client to declare the param's
	// oid explicitly in PQprepare (sqlrelay's postgresql protocol
	// module never sends back a ParameterDescription, so it only knows
	// a param's type if the client states it) and to send an explicit
	// paramFormats array in PQexecPrepared (a NULL array sends a
	// format-code count of 0, a separate code path that skips the
	// oid-based null typing entirely)
	stdoutput.printf("PQprepare/PQexecPrepared: null bytea/text bind\n");
	query="alter table testtable add column testbytea bytea, "
				"add column testtext text";
	pgresult=PQexec(pgconn,query);
	assertEquals(PQresultStatus(pgresult),PGRES_COMMAND_OK);
	PQclear(pgresult);

	query="insert into testtable (testint,testbytea,testtext) "
					"values ($1,$2,$3)";
	Oid	nulltypes[]={23,17,25};
	pgresult=PQprepare(pgconn,"nullblobclob",query,3,nulltypes);
	assertEquals(PQresultStatus(pgresult),PGRES_COMMAND_OK);
	PQclear(pgresult);

	const char * const nullparamvalues[]={"99",NULL,NULL};
	int	nullparamformats[]={0,0,0};
	pgresult=PQexecPrepared(pgconn,"nullblobclob",3,
				nullparamvalues,NULL,nullparamformats,0);
	assertEquals(PQresultStatus(pgresult),PGRES_COMMAND_OK);
	assertEquals(PQcmdTuples(pgresult),"1");
	PQclear(pgresult);

	query="select testbytea,testtext from testtable where testint=99";
	pgresult=PQexec(pgconn,query);
	assertEquals(PQresultStatus(pgresult),PGRES_TUPLES_OK);
	assertEquals(PQntuples(pgresult),1);
	assertEquals(PQgetisnull(pgresult,0,0),1);
	assertEquals(PQgetisnull(pgresult,0,1),1);
	PQclear(pgresult);
	stdoutput.printf("\n");

	// #10133: the numeric/_numeric binary bind-parameter decoder used
	// to never place a decimal point and treated sign as a plain
	// boolean, losing scale and mishandling negative values, NaN and
	// the two infinities.  bind each case in postgresql's binary
	// numeric wire format and read the value back to confirm the
	// decoder built the right text.  the param is still declared as
	// oid 1700 (numeric) via PQprepare below, so sqlrelay's own
	// protocol-layer decoder runs, but the target column is text, not
	// numeric, so the value isn't re-validated as a numeric literal by
	// the backend - some backends in this shop's test fleet predate
	// postgresql 14 and reject "Infinity"/"-Infinity" as numeric input
	stdoutput.printf("PQprepare/PQexecPrepared: binary numeric bind\n");
	query="alter table testtable add column testnumeric text";
	pgresult=PQexec(pgconn,query);
	assertEquals(PQresultStatus(pgresult),PGRES_COMMAND_OK);
	PQclear(pgresult);

	query="insert into testtable (testint,testnumeric) values ($1,$2)";
	Oid	numerictypes[]={23,1700};
	pgresult=PQprepare(pgconn,"numericbind",query,2,numerictypes);
	assertEquals(PQresultStatus(pgresult),PGRES_COMMAND_OK);
	PQclear(pgresult);

	// digit groups are base-10000, big-endian, one group per 4 decimal
	// digits; weight is the power of 10000 the first group is worth
	uint16_t	digits_half[]={5000};
	uint16_t	digits_onetwothreefourfive[]={1,2345,6780};
	uint16_t	digits_fortytwohalf[]={42,5000};

	struct numericcase {
		int		testint;
		const uint16_t	*digits;
		uint16_t	ndigits;
		int16_t		weight;
		uint16_t	sign;
		uint16_t	dscale;
		const char	*expected;
	};
	numericcase	cases[]={
		{201,digits_half,1,-1,0x0000,1,"0.5"},
		{202,digits_onetwothreefourfive,3,1,0x0000,3,"12345.678"},
		{203,digits_fortytwohalf,2,0,0x4000,1,"-42.5"},
		{204,NULL,0,0,0xC000,0,"NaN"},
		{205,NULL,0,0,0xD000,0,"Infinity"},
		{206,NULL,0,0,0xF000,0,"-Infinity"}
	};
	unsigned int	ncases=sizeof(cases)/sizeof(cases[0]);

	for (unsigned int i=0; i<ncases; i++) {

		// pack the wire format: ndigits, weight, sign, dscale,
		// then ndigits big-endian uint16 digit groups
		unsigned char	buf[8+2*3];
		size_t		pos=0;
		appendUint16BE(buf,&pos,cases[i].ndigits);
		appendUint16BE(buf,&pos,(uint16_t)cases[i].weight);
		appendUint16BE(buf,&pos,cases[i].sign);
		appendUint16BE(buf,&pos,cases[i].dscale);
		for (uint16_t d=0; d<cases[i].ndigits; d++) {
			appendUint16BE(buf,&pos,cases[i].digits[d]);
		}

		char	testintstr[12];
		charstring::printf(testintstr,sizeof(testintstr),
						"%d",cases[i].testint);

		const char	*paramvalues[]={testintstr,(const char *)buf};
		int		paramlengths[]={0,(int)pos};
		int		paramformats[]={0,1};
		pgresult=PQexecPrepared(pgconn,"numericbind",2,
				paramvalues,paramlengths,paramformats,0);
		assertEquals(PQresultStatus(pgresult),PGRES_COMMAND_OK);
		assertEquals(PQcmdTuples(pgresult),"1");
		PQclear(pgresult);
	}

	query="select testint,testnumeric from testtable "
			"where testint>=201 and testint<=206 "
			"order by testint";
	pgresult=PQexec(pgconn,query);
	assertEquals(PQresultStatus(pgresult),PGRES_TUPLES_OK);
	assertEquals(PQntuples(pgresult),(int)ncases);
	for (unsigned int r=0; r<ncases; r++) {
		assertEquals(PQgetvalue(pgresult,(int)r,1),cases[r].expected);
	}
	PQclear(pgresult);
	stdoutput.printf("\n");

	query="drop table testtable";
	pgresult=PQexec(pgconn,query);
	PQclear(pgresult);

	PQfinish(pgconn);
#endif

	reportTestStatus();
	return status;
}
