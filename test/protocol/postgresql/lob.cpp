// Copyright (c) David Muse
// See the file COPYING for more information.

#include <libpq-fe.h>
#include <rudiments/charstring.h>
#include <rudiments/stringbuffer.h>
#include <rudiments/bytestring.h>
#include <rudiments/stdio.h>

#include "asserts.cpp"

PGconn	*pgconn;

// exercises buildLobField (src/protocols/postgresql.cpp, #10041), which had
// zero live coverage under any backend.  it reads a field's lob value in
// 8192-char segments (sizeof(lobbuffer)/MAX_BYTES_PER_CHAR), looping over the
// connection module's getLobFieldSegment until the whole value is read.  a
// real postgresql backend never takes this path (it hands back its own
// bytea/text wire format directly), so this pairs the postgresql protocol
// frontend with the mysql connection module instead, against the
// mysqlpostgresqlprotocol instance (test/sqlrelay.conf.d/mysqlprotocol.conf)
// - mysqlcursor::getField (src/connections/mysql.cpp) sets a BLOB column's
// lob flag unconditionally, with no size floor, so an ordinary blob column
// reliably takes the chunked path.  db2/odbc, this file's counterpart for
// #10038, would also work here (DB2's ~32700-byte LONG VARCHAR ceiling is
// well past the 8192-byte segment size), but src/connections/db2.cpp is
// under active work on a separate ticket (#10040) as this is written, so
// mysql sidesteps that entirely.
int main(int argc, char **argv) {

	// 40K - well past the 8192-byte segment size, forcing
	// buildLobField's loop to run more than once (five times, at
	// 8192 bytes per call).  it also has to stay under 65536: past
	// that, writeByteaField's hex encoding of the returned value runs
	// into an unrelated bug in rudiments' charstring::hexEncode (its
	// loop counter is a uint16_t, so it never reaches a uint64_t
	// inputsize bigger than 65535 and the encode never terminates)
	#define LOB_LENGTH	(40*1024)
	char		lobbuffer[LOB_LENGTH+1];

	stdoutput.printf("PQstatus:\n");
	pgconn=PQsetdbLogin("127.0.0.1","9041",NULL,NULL,
					"testuser","testuser","testpassword");
	assertEquals(PQstatus(pgconn),CONNECTION_OK);
	stdoutput.printf("\n");

	PGresult	*pgresult=PQexec(pgconn,"drop table if exists testtable");
	PQclear(pgresult);

	stdoutput.printf("PQexec: create\n");
	pgresult=PQexec(pgconn,
			"create table testtable (id int, blobcol longblob)");
	assertEquals(PQresultStatus(pgresult),PGRES_COMMAND_OK);
	PQclear(pgresult);
	stdoutput.printf("\n");

	// byte-position-dependent, not a uniform fill, so an overlapping or
	// duplicated segment read (not just truncation) would also mismatch
	for (int i=0; i<LOB_LENGTH; i++) {
		lobbuffer[i]='A'+(i%26);
	}
	lobbuffer[LOB_LENGTH]='\0';

	// a literal insert, not a bound parameter - the postgresql protocol
	// module passes $1-style placeholders straight through to the
	// backend as query text, and mysql doesn't understand them.  a
	// literal is also simpler here than in #10038's db2 counterpart:
	// mysql has no ~32K string-literal ceiling to work around
	stringbuffer	query;
	query.append("insert into testtable values (1,'")
			->append(lobbuffer,LOB_LENGTH)->append("')");

	stdoutput.printf("PQexec: insert\n");
	pgresult=PQexec(pgconn,query.getString());
	assertEquals(PQresultStatus(pgresult),PGRES_COMMAND_OK);
	PQclear(pgresult);
	stdoutput.printf("\n");

	stdoutput.printf("PQexec: select\n");
	pgresult=PQexec(pgconn,
			"select blobcol from testtable where id=1");
	assertEquals(PQresultStatus(pgresult),PGRES_TUPLES_OK);
	assertEquals(PQntuples(pgresult),1);
	stdoutput.printf("\n");

	// the mysql blob column maps to bytea (oid 17), which
	// writeByteaField always sends back hex-encoded ("\x..."), so
	// PQunescapeBytea is what a real bytea client would use to get the
	// raw bytes back
	stdoutput.printf("PQgetvalue/PQunescapeBytea: \n");
	size_t		lobsize=0;
	unsigned char	*lob=PQunescapeBytea(
			(const unsigned char *)PQgetvalue(pgresult,0,0),
			&lobsize);
	assertEquals((int)lobsize,LOB_LENGTH);
	assertTrue(!bytestring::compare(lob,lobbuffer,LOB_LENGTH));
	stdoutput.printf("\n");
	PQfreemem(lob);

	PQclear(pgresult);

	pgresult=PQexec(pgconn,"drop table testtable");
	PQclear(pgresult);

	PQfinish(pgconn);

	reportTestStatus();

	return status;
}
