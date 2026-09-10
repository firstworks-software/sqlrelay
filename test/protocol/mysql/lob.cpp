// Copyright (c) David Muse
// See the file COPYING for more information.

#include <mysql.h>
#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/stdio.h>

#include "asserts.cpp"

MYSQL	mysql;

// exercises buildLobField (src/protocols/mysql.cpp), which had zero live
// coverage under any backend.  it reads a field's lob value in 8192-char
// segments (sizeof(lobbuffer)/MAX_BYTES_PER_CHAR), looping over the
// connection module's getLobFieldSegment until the whole value is read.  a
// real mysql backend never takes this path (mysqlcursor::getField hands
// blob columns straight back in mysql's own wire format), so this pairs the
// mysql protocol frontend with the firebird connection module instead,
// against the firebirdmysqlprotocol instance
// (test/sqlrelay.conf.d/firebird.conf.in) - firebird BLOBs have no small
// size ceiling the way DB2's LONG VARCHAR does, so an ordinary BLOB
// SUB_TYPE 1 column reliably takes the chunked path.
int main(int argc, char **argv) {

	// well past the 8192-byte segment size, forcing buildLobField's loop
	// to run more than once (five times, at 8192 bytes per call)
	#define LOB_LENGTH	(40*1024)
	char		lobbuffer[LOB_LENGTH];

	stdoutput.printf("mysql_init\n");
	assertEquals((long)mysql_init(&mysql),(long)&mysql);
	stdoutput.printf("\n");

	stdoutput.printf("mysql_real_connect\n");
	assertEquals((long)mysql_real_connect(&mysql,
					"127.0.0.1","testuser","testpassword",
					NULL,9042,NULL,0),(long)&mysql);
	stdoutput.printf("\n");

	const char	*query="drop table mysqlprotocollobtest";
	mysql_real_query(&mysql,query,charstring::getLength(query));
	query="commit";
	mysql_real_query(&mysql,query,charstring::getLength(query));

	stdoutput.printf("mysql_real_query: create\n");
	query="create table mysqlprotocollobtest "
			"(id int, blobcol blob sub_type 1)";
	assertEquals(mysql_real_query(&mysql,query,
					charstring::getLength(query)),0);
	stdoutput.printf("\n");

	// firebird's implicit transaction model means the create table above
	// isn't visible to the prepare below without an explicit commit
	stdoutput.printf("mysql_real_query: commit\n");
	query="commit";
	assertEquals(mysql_real_query(&mysql,query,
					charstring::getLength(query)),0);
	stdoutput.printf("\n");

	// byte-position-dependent, not a uniform fill, so an overlapping or
	// duplicated segment read (not just truncation) would also mismatch
	for (int i=0; i<LOB_LENGTH; i++) {
		lobbuffer[i]='A'+(i%26);
	}

	// bound parameter, not a literal - firebird's MAX_STATEMENT_SIZE
	// (an unsigned short) rejects any literal insert text over 65535
	// bytes
	stdoutput.printf("mysql_stmt_prepare: insert\n");
	MYSQL_STMT	*stmt=mysql_stmt_init(&mysql);
	query="insert into mysqlprotocollobtest values (1,?)";
	assertEquals(mysql_stmt_prepare(stmt,query,
					charstring::getLength(query)),0);

	MYSQL_BIND	bind;
	bytestring::zero(&bind,sizeof(bind));
	unsigned long	bindlength=LOB_LENGTH;
	bind.buffer_type=MYSQL_TYPE_LONG_BLOB;
	bind.buffer=lobbuffer;
	bind.buffer_length=LOB_LENGTH;
	bind.length=&bindlength;
	assertEquals(mysql_stmt_bind_param(stmt,&bind),0);

	stdoutput.printf("mysql_stmt_execute: insert\n");
	assertEquals(mysql_stmt_execute(stmt),0);
	stdoutput.printf("\n");

	mysql_stmt_close(stmt);

	stdoutput.printf("mysql_real_query: select\n");
	query="select blobcol from mysqlprotocollobtest where id=1";
	assertEquals(mysql_real_query(&mysql,query,
					charstring::getLength(query)),0);
	stdoutput.printf("\n");

	stdoutput.printf("mysql_store_result/mysql_fetch_row: \n");
	MYSQL_RES	*result=mysql_store_result(&mysql);
	assertEquals((int)mysql_num_rows(result),1);
	MYSQL_ROW	row=mysql_fetch_row(result);
	unsigned long	*lengths=mysql_fetch_lengths(result);
	assertEquals((int)lengths[0],LOB_LENGTH);
	assertTrue(!bytestring::compare(row[0],lobbuffer,LOB_LENGTH));
	stdoutput.printf("\n");

	mysql_free_result(result);

	query="drop table mysqlprotocollobtest";
	mysql_real_query(&mysql,query,charstring::getLength(query));

	mysql_close(&mysql);

	reportTestStatus();

	return status;
}
