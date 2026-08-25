// Copyright (c) David Muse
// See the file COPYING for more information.

#include <ibase.h>
#include <config.h>
#include <time.h>
#include <rudiments/sys.h>
#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/process.h>
#include <rudiments/environment.h>
#include <rudiments/stringbuffer.h>
#include <rudiments/stdio.h>

#include "asserts.cpp"

ISC_STATUS_ARRAY	fbstatus;
isc_db_handle		db=0;
isc_tr_handle		tr=0;

char	dbpath[1024];
char	dpb[512];
short	dpbsize=0;

const char	*user="testuser";
const char	*password="testpassword";

// build a database parameter buffer by hand, the same buffer walk that
// src/connections/firebird.cpp:894-920 does
static short buildDpb(char *buffer, const char *dpbuser,
					const char *dpbpassword) {

	char	*dpbptr=buffer;

	// set the parameter buffer version
	*dpbptr=isc_dpb_version1;
	dpbptr++;

	// set the user name
	*dpbptr=isc_dpb_user_name;
	dpbptr++;
	*dpbptr=(char)charstring::getLength(dpbuser);
	dpbptr++;
	charstring::copy(dpbptr,dpbuser);
	dpbptr+=charstring::getLength(dpbuser);

	// set the password
	*dpbptr=isc_dpb_password;
	dpbptr++;
	*dpbptr=(char)charstring::getLength(dpbpassword);
	dpbptr++;
	charstring::copy(dpbptr,dpbpassword);
	dpbptr+=charstring::getLength(dpbpassword);

	// set the character set
	// NONE means no transliteration of returned data.  It does not
	// control sqllen or sqlsubtype - those are the column's declared
	// width in the column's charset, and that charset's id, both set by
	// the database's default charset.  The metadata section asserts
	// sqllen 50 and sqlsubtype 0, which hold only for a single-byte
	// database.  A UTF8 database gives sqllen 200 and a non-zero
	// subtype.  The test databases are single-byte - checked against
	// firebird 2.5, 3.0 and 4.0 under #8954.
	*dpbptr=isc_dpb_lc_ctype;
	dpbptr++;
	*dpbptr=4;
	dpbptr++;
	charstring::copy(dpbptr,"NONE");
	dpbptr+=4;

	// set the sql dialect
	*dpbptr=isc_dpb_sql_dialect;
	dpbptr++;
	*dpbptr=1;
	dpbptr++;
	*dpbptr=SQL_DIALECT_V6;
	dpbptr++;

	// set the connect timeout
	// (the firebird analogue of the CS_LOGIN_TIMEOUT that
	// test/protocol/tds/tds.cpp:146-150 sets - without it, a module that
	// never finishes the handshake hangs this test, and make tests along
	// with it)
	*dpbptr=isc_dpb_connect_timeout;
	dpbptr++;
	*dpbptr=1;
	dpbptr++;
	*dpbptr=30;
	dpbptr++;

	return (short)(dpbptr-buffer);
}

// A stubbed op returns false, which drops the socket rather than sending an
// error response (#7231), so every isc_* call after it fails too.  Each
// section below reattaches if the one before it killed the connection, so
// one stub does not cascade into ten thousand failures.
static void reattach() {
	if (db) {
		isc_detach_database(fbstatus,&db);
		db=0;
	}
	tr=0;
	isc_attach_database(fbstatus,(short)charstring::getLength(dbpath),
						dbpath,&db,dpbsize,dpb);
}

// pull the first message out of the status vector
static void firstErrorMessage(char *msg, unsigned int msgsize) {
	msg[0]='\0';
	const ISC_STATUS	*sv=fbstatus;
	fbInterpret(msg,msgsize,&sv);
}

// the read-committed/rec_version/write/wait tpb from
// src/connections/firebird.cpp:356-363
static char tpb[]={
	isc_tpb_version3,
	isc_tpb_write,
	isc_tpb_read_committed,
	isc_tpb_rec_version,
	isc_tpb_wait
};

// the autocommit tpb from src/connections/firebird.cpp:365-373
static char tpbac[]={
	isc_tpb_version3,
	isc_tpb_write,
	isc_tpb_read_committed,
	isc_tpb_rec_version,
	isc_tpb_wait,
	isc_tpb_autocommit
};

// a read-only variant of tpb
static char tpbro[]={
	isc_tpb_version3,
	isc_tpb_read,
	isc_tpb_read_committed,
	isc_tpb_rec_version,
	isc_tpb_wait
};

// isolation-level variants of tpb - isc_tpb_rec_version only applies under
// read committed (see src/connections/firebird.cpp's buildTpb()), so it's
// omitted for consistency and concurrency
static char tpbconsistency[]={
	isc_tpb_version3,
	isc_tpb_write,
	isc_tpb_consistency,
	isc_tpb_wait
};

static char tpbconcurrency[]={
	isc_tpb_version3,
	isc_tpb_write,
	isc_tpb_concurrency,
	isc_tpb_wait
};

static char tpbreadcommitted[]={
	isc_tpb_version3,
	isc_tpb_write,
	isc_tpb_read_committed,
	isc_tpb_rec_version,
	isc_tpb_wait
};

// count the rows in a table, so a commit or rollback can be shown to
// have taken
static int countRows(const char *table) {

	char	query[256];
	charstring::printf(query,sizeof(query),
				"select count(*) from %s",table);

	XSQLDA	*sqlda=(XSQLDA *)new char[XSQLDA_LENGTH(1)];
	bytestring::zero(sqlda,XSQLDA_LENGTH(1));
	sqlda->version=SQLDA_VERSION1;
	sqlda->sqln=1;

	// prepare, describing the count into the sqlda
	isc_stmt_handle	stmt=0;
	if (isc_dsql_allocate_statement(fbstatus,&db,&stmt) ||
		isc_dsql_prepare(fbstatus,&tr,&stmt,0,
					query,SQL_DIALECT_V6,sqlda)) {
		delete[] (char *)sqlda;
		return -1;
	}

	// point the count column at a buffer
	// (firebird 3+ returns count() as bigint, older versions as integer)
	ISC_INT64	countbuffer=0;
	short		countind=0;
	sqlda->sqlvar[0].sqldata=(char *)&countbuffer;
	sqlda->sqlvar[0].sqlind=&countind;

	// fetch the single row
	int	count=-1;
	if (!isc_dsql_execute(fbstatus,&tr,&stmt,SQL_DIALECT_V6,NULL) &&
		!isc_dsql_fetch(fbstatus,&stmt,SQL_DIALECT_V6,sqlda)) {
		count=((sqlda->sqlvar[0].sqltype&~1)==SQL_LONG)?
					(int)*((ISC_LONG *)&countbuffer):
					(int)countbuffer;
	}

	isc_dsql_free_statement(fbstatus,&stmt,DSQL_drop);

	delete[] (char *)sqlda;

	return count;
}

// run a query that selects a single integer column, in the current
// transaction, and return it (or -1 on error) - used to ask the backend
// itself, via mon$transactions, what it actually did
static int selectInt(const char *query) {

	XSQLDA	*sqlda=(XSQLDA *)new char[XSQLDA_LENGTH(1)];
	bytestring::zero(sqlda,XSQLDA_LENGTH(1));
	sqlda->version=SQLDA_VERSION1;
	sqlda->sqln=1;

	isc_stmt_handle	stmt=0;
	if (isc_dsql_allocate_statement(fbstatus,&db,&stmt) ||
		isc_dsql_prepare(fbstatus,&tr,&stmt,0,
					query,SQL_DIALECT_V6,sqlda)) {
		delete[] (char *)sqlda;
		return -1;
	}

	ISC_INT64	valbuffer=0;
	short		valind=0;
	sqlda->sqlvar[0].sqldata=(char *)&valbuffer;
	sqlda->sqlvar[0].sqlind=&valind;

	int	val=-1;
	if (!isc_dsql_execute(fbstatus,&tr,&stmt,SQL_DIALECT_V6,NULL) &&
		!isc_dsql_fetch(fbstatus,&stmt,SQL_DIALECT_V6,sqlda)) {
		val=((sqlda->sqlvar[0].sqltype&~1)==SQL_LONG)?
					(int)*((ISC_LONG *)&valbuffer):
					(int)valbuffer;
	}

	isc_dsql_free_statement(fbstatus,&stmt,DSQL_drop);

	delete[] (char *)sqlda;

	return val;
}

// point every column of an output sqlda at a correctly sized buffer and an
// indicator
static int bindOutputBuffers(XSQLDA *sqlda, short *ind, char **buffer) {

	int	colcount=(sqlda->sqld<sqlda->sqln)?sqlda->sqld:sqlda->sqln;
	if (colcount<0) {
		colcount=0;
	}

	for (int col=0; col<colcount; col++) {

		XSQLVAR	*var=&sqlda->sqlvar[col];

		int	buflen=var->sqllen;
		if ((var->sqltype&~1)==SQL_VARYING) {
			// 2-byte length prefix ahead of the data
			buflen=var->sqllen+2;
		} else if ((var->sqltype&~1)==SQL_TEXT) {
			// room for a null, so the value reads as a string
			buflen=var->sqllen+1;
		}
		if (buflen<1) {
			buflen=1;
		}

		buffer[col]=new char[buflen];
		bytestring::zero(buffer[col],buflen);

		var->sqldata=buffer[col];
		var->sqlind=&ind[col];

		// force the nullable bit on, so firebird always writes the
		// indicator, even for a column it thinks cannot be null
		var->sqltype|=1;

		ind[col]=0;
	}

	return colcount;
}

static void freeOutputBuffers(int colcount, char **buffer) {
	for (int col=0; col<colcount; col++) {
		delete[] buffer[col];
	}
}

// read every segment of an already-open blob into buffer, up to
// buffersize bytes, and return the number of bytes read.  Used where a
// test only cares whether the round-tripped content matches, not the
// segment-boundary mechanics themselves - those are exercised directly by
// the "isc_get_segment" test further down.
static int readBlob(isc_blob_handle *blobhandle, char *buffer,
					int buffersize) {

	bytestring::zero(buffer,buffersize);

	unsigned short	seglen=0;
	int		totalread=0;
	int		segreads=0;
	while (segreads<64 && totalread<buffersize) {

		ISC_STATUS	getresult=isc_get_segment(fbstatus,blobhandle,
					&seglen,
					(unsigned short)(buffersize-totalread),
					buffer+totalread);

		totalread+=seglen;
		segreads++;

		// a non-zero return with isc_segment in the vector means
		// the buffer was too small for the whole segment and there
		// is more of it to come - anything else non-zero means eof
		// (or an error, left for the caller to notice via fbstatus)
		if (getresult && fbstatus[1]!=isc_segment) {
			break;
		}
	}

	return totalread;
}

// point an input parameter at a newly allocated buffer holding the value,
// in whatever type the bind describe came back with, and hand the buffer
// back for the caller to delete[].
//
// the type has to be taken rather than assumed: sql relay's firebird
// protocol module gets a bind's real type - for a select, update or delete
// as well as an insert - from the backend's own prepare, when talking to a
// live firebird backend that actually ran one (see describeBinds() in
// src/protocols/firebird.cpp). under fakeinputbindvariables, or when the
// firebird protocol fronts a non-firebird backend, there was no prepare to
// ask, and the bind comes back as a generic stand-in - a wide nullable
// string - instead.
static char *bindInputValue(XSQLVAR *var, short *ind,
					ISC_LONG intval, const char *strval) {

	int	type=var->sqltype&~1;

	int	len=var->sqllen;
	if (len<1) {
		len=1;
	}

	int	strvallen=(int)charstring::getLength(strval);
	if (strvallen>len) {
		strvallen=len;
	}

	char	*buffer=NULL;
	if (type==SQL_LONG) {
		// ISC_LONG, not C long - on LP64 a plain long is 8 bytes
		// where SQL_LONG is 4, and the execute then fails with a
		// message-length error that looks like a module defect
		buffer=new char[sizeof(ISC_LONG)];
		*((ISC_LONG *)buffer)=intval;
	} else if (type==SQL_SHORT) {
		buffer=new char[sizeof(ISC_SHORT)];
		*((ISC_SHORT *)buffer)=(ISC_SHORT)intval;
	} else if (type==SQL_VARYING) {
		// a 2-byte host-order length ahead of the data
		buffer=new char[len+3];
		bytestring::zero(buffer,len+3);
		PARAMVARY	*vary=(PARAMVARY *)buffer;
		vary->vary_length=(ISC_USHORT)strvallen;
		charstring::copy((char *)vary->vary_string,strval,strvallen);
	} else {
		// SQL_TEXT, and anything else, as a blank-padded string
		buffer=new char[len+1];
		bytestring::set(buffer,' ',len);
		buffer[len]='\0';
		charstring::copy(buffer,strval,strvallen);
	}

	var->sqldata=buffer;
	var->sqlind=ind;
	// force the nullable bit on, so the indicator is honoured
	var->sqltype|=1;
	*ind=0;

	return buffer;
}

// pin a result-set column's metadata
static void assertColumn(XSQLVAR *var, const char *name, int type,
					int len, int scale, int subtype) {

	// the low bit of sqltype means nullable, and every testtable
	// column is nullable
	assertEquals(var->sqltype,type|1);
	assertEquals(var->sqltype&~1,type);
	assertEquals(var->sqllen,len);
	assertEquals(var->sqlscale,scale);
	assertEquals(var->sqlsubtype,subtype);

	// the column name, and the alias, which "select *" leaves the same
	assertEquals(var->sqlname,name);
	assertEquals(var->sqlname_length,(int)charstring::getLength(name));
	assertEquals(var->aliasname,name);
	assertEquals(var->aliasname_length,(int)charstring::getLength(name));

	// the table, and its owner, whose name depends on who built the db
	assertEquals(var->relname,"TESTTABLE");
	assertEquals(var->relname_length,9);
	assertTrue(var->ownname_length>0);
	assertEquals(var->ownname_length,
			(int)charstring::getLength(var->ownname));
}

// name a column's sql type for the metadata dump
static void printSqlType(XSQLVAR *var) {
	switch (var->sqltype&~1) {
		case SQL_TEXT:
			stdoutput.printf("SQL_TEXT");
			break;
		case SQL_VARYING:
			stdoutput.printf("SQL_VARYING");
			break;
		case SQL_SHORT:
			stdoutput.printf("SQL_SHORT");
			break;
		case SQL_LONG:
			stdoutput.printf("SQL_LONG");
			break;
		case SQL_INT64:
			stdoutput.printf("SQL_INT64");
			break;
		case SQL_FLOAT:
			stdoutput.printf("SQL_FLOAT");
			break;
		case SQL_DOUBLE:
			stdoutput.printf("SQL_DOUBLE");
			break;
		case SQL_TYPE_DATE:
			stdoutput.printf("SQL_TYPE_DATE");
			break;
		case SQL_TYPE_TIME:
			stdoutput.printf("SQL_TYPE_TIME");
			break;
		case SQL_TIMESTAMP:
			stdoutput.printf("SQL_TIMESTAMP");
			break;
		case SQL_BLOB:
			stdoutput.printf("SQL_BLOB");
			break;
		default:
			// SQL_INT128, SQL_DEC16, SQL_DEC34, SQL_TIME_TZ,
			// SQL_TIMESTAMP_TZ and SQL_BOOLEAN are firebird 3/4
			// additions that testtable does not use, so print the
			// raw code rather than assert on it
			stdoutput.printf("%d",var->sqltype&~1);
			break;
	}
}

int main(int argc, char **argv) {

	// pass "native" to test a real firebird instance instead of
	// sqlrelay's firebird protocol.  a second argument names which one,
	// as a host name or dns alias.  the aliases are not versioned the way
	// they read - "firebird" and "firebird3" are both the 3.0 server and
	// "firebird2" is the 2.5 server, with no alias for 4.0 - so prefer
	// the full host name.
	// in sqlrelay mode, an argument names the listener, in firebird's
	// "host" or "host/port" form, so an instance on a port other than 3050
	// can be reached.
	bool		issqlrelay=!(argc>=2 &&
					!charstring::compare(argv[1],"native"));
	const char	*server="127.0.0.1";
	if (!issqlrelay) {
		server=(argc>=3)?argv[2]:"firebird";
	} else if (argc>=2) {
		server=argv[1];
	}

	// short hostname, matching the db the native odbc tests use
	char	*hostname=sys::getHostName();
	char	*dot=(char *)charstring::findFirstOrEnd(hostname,'.');
	*dot='\0';

	charstring::printf(dbpath,sizeof(dbpath),
				"%s:/u02/%s.gdb",server,hostname);

	dpbsize=buildDpb(dpb,user,password);


	stdoutput.printf("\n=============== Attach ===============\n\n");

	stdoutput.printf("isc_attach_database\n");
	ISC_STATUS	attached=isc_attach_database(fbstatus,
					(short)charstring::getLength(dbpath),
					dbpath,&db,dpbsize,dpb);
	assertEquals((int)attached,0);
	assertEquals((int)fbstatus[1],0);
	assertTrue(db!=0);
	stdoutput.printf("\n\n");

	// Nothing below here can work without an attachment.  Running on
	// anyway leaves sqld unset and spins the describe loop for millions
	// of iterations.
	if (attached) {
		reportTestStatus();
		return status;
	}


	stdoutput.printf("isc_attach_database - bad password\n");
	char		badpwdpb[512];
	short		badpwdpbsize=buildDpb(badpwdpb,user,"wrongpassword");
	isc_db_handle	badpwdb=0;
	assertTrue(isc_attach_database(fbstatus,
					(short)charstring::getLength(dbpath),
					dbpath,&badpwdb,
					badpwdpbsize,badpwdpb)!=0);
	assertTrue(badpwdb==0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_attach_database - bad db path\n");
	char		baddbpath[1024];
	charstring::printf(baddbpath,sizeof(baddbpath),
				"%s:/u02/nosuchdatabase.gdb",server);
	isc_db_handle	baddb=0;
	assertTrue(isc_attach_database(fbstatus,
					(short)charstring::getLength(baddbpath),
					baddbpath,&baddb,dpbsize,dpb)!=0);
	assertTrue(baddb==0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_database_info\n");
	char	dbinfoitems[]={
			isc_info_page_size,
			isc_info_num_buffers,
			isc_info_db_sql_dialect,
			isc_info_ods_version,
			isc_info_isc_version,
			isc_info_base_level,
			isc_info_end
			};
	char	dbinfobuffer[2048];
	bytestring::zero(dbinfobuffer,sizeof(dbinfobuffer));
	assertEquals((int)isc_database_info(fbstatus,&db,
				(short)sizeof(dbinfoitems),dbinfoitems,
				(short)sizeof(dbinfobuffer),dbinfobuffer),0);

	// walk the info buffer - item byte, 2-byte length, then value
	const char	*dbinfoptr=dbinfobuffer;
	const char	*dbinfoendptr=dbinfobuffer+sizeof(dbinfobuffer);
	int		dbinfocount=0;
	while (dbinfoptr<dbinfoendptr && *dbinfoptr!=(char)isc_info_end &&
					*dbinfoptr!=(char)isc_info_truncated) {

		char	dbinfoitem=*dbinfoptr;
		dbinfoptr++;
		short	dbinfolen=(short)isc_vax_integer(dbinfoptr,2);
		dbinfoptr+=2;

		switch (dbinfoitem) {
			case isc_info_page_size:
				// firebird pages are 1k-32k
				assertTrue(isc_vax_integer(
						dbinfoptr,dbinfolen)>=1024);
				break;
			case isc_info_num_buffers:
				assertTrue(isc_vax_integer(
						dbinfoptr,dbinfolen)>0);
				break;
			case isc_info_db_sql_dialect:
				assertEquals((int)isc_vax_integer(
						dbinfoptr,dbinfolen),
						SQL_DIALECT_V6);
				break;
			case isc_info_ods_version:
				// 11 is firebird 2.5, 12 is 3.0, 13 is 4.0
				assertTrue(isc_vax_integer(
						dbinfoptr,dbinfolen)>=11);
				break;
			case isc_info_isc_version:
				// count byte, then a counted string
				assertEquals((int)dbinfoptr[0],1);
				assertTrue(dbinfoptr[1]>0);
				break;
			case isc_info_base_level:
				// count byte, then the level, always 6.
				// The count is 3, not the 1 the server sent.
				// fbclient merges its own entry into the
				// caller's buffer after the response arrives,
				// so the count the caller sees is one more
				// than the wire carried.  Measured as 3 on
				// firebird 2.5, 3.0 and 4.0 and on sqlrelay
				// under #8954.
				assertEquals((int)dbinfoptr[0],3);
				assertEquals((int)dbinfoptr[1],6);
				break;
			default:
				stdoutput.printf("unexpected db info item %d\n",
							(int)dbinfoitem);
				break;
		}

		dbinfoptr+=dbinfolen;
		dbinfocount++;
	}
	assertEquals(dbinfocount,6);
	assertEquals((int)*dbinfoptr,(int)isc_info_end);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_database_info - counters, sizes and dates\n");
	char	countinfoitems[]={
			isc_info_reads,
			isc_info_writes,
			isc_info_fetches,
			isc_info_marks,
			isc_info_allocation,
			isc_info_current_memory,
			isc_info_max_memory,
			isc_info_page_errors,
			isc_info_record_errors,
			isc_info_bpage_errors,
			isc_info_dpage_errors,
			isc_info_ipage_errors,
			isc_info_ppage_errors,
			isc_info_tpage_errors,
			isc_info_set_page_buffers,
			isc_info_db_size_in_pages,
			isc_info_db_file_size,
			isc_info_oldest_transaction,
			isc_info_oldest_active,
			isc_info_oldest_snapshot,
			isc_info_next_transaction,
			isc_info_active_tran_count,
			isc_info_read_seq_count,
			isc_info_read_idx_count,
			isc_info_insert_count,
			isc_info_update_count,
			isc_info_delete_count,
			isc_info_backout_count,
			isc_info_purge_count,
			isc_info_expunge_count,
			isc_info_creation_date,
			isc_info_end
			};
	char	countinfobuffer[2048];
	bytestring::zero(countinfobuffer,sizeof(countinfobuffer));
	assertEquals((int)isc_database_info(fbstatus,&db,
				(short)sizeof(countinfoitems),countinfoitems,
				(short)sizeof(countinfobuffer),countinfobuffer),0);

	// These are shape assertions rather than value assertions, on purpose.
	// The same code runs against a real server in native mode, where the
	// counters are whatever that server has been doing.
	const char	*countinfoptr=countinfobuffer;
	const char	*countinfoendptr=countinfobuffer+sizeof(countinfobuffer);
	int		countinfocount=0;
	while (countinfoptr<countinfoendptr &&
			*countinfoptr!=(char)isc_info_end &&
			*countinfoptr!=(char)isc_info_truncated) {

		char	countinfoitem=*countinfoptr;
		countinfoptr++;
		short	countinfolen=(short)isc_vax_integer(countinfoptr,2);
		countinfoptr+=2;

		switch (countinfoitem) {
			case isc_info_read_seq_count:
			case isc_info_read_idx_count:
			case isc_info_insert_count:
			case isc_info_update_count:
			case isc_info_delete_count:
			case isc_info_backout_count:
			case isc_info_purge_count:
			case isc_info_expunge_count:
				// a vector of (2-byte relation id, 4-byte
				// count) pairs, empty for relations the
				// server hasn't touched
				assertEquals((int)(countinfolen%6),0);
				break;
			case isc_info_creation_date:
				// an ISC_DATE then an ISC_TIME.  The date has
				// to be a real one - a zero decodes to 17
				// November 1858, the modified julian day
				// epoch, which is what a client shows when the
				// server has nothing to say.
				assertEquals((int)countinfolen,8);
				assertTrue(isc_vax_integer(countinfoptr,4)>0);
				break;
			default:
				// everything else is a plain counter
				assertEquals((int)countinfolen,4);
				break;
		}

		countinfoptr+=countinfolen;
		countinfocount++;
	}
	assertEquals(countinfocount,31);
	assertEquals((int)*countinfoptr,(int)isc_info_end);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_database_info - repeating and refused items\n");
	char	repinfoitems[]={
			isc_info_user_names,
			isc_info_limbo,
			isc_info_active_transactions,
			isc_info_window_turns,
			isc_info_license,
			isc_info_end
			};
	char	repinfobuffer[2048];
	bytestring::zero(repinfobuffer,sizeof(repinfobuffer));
	assertEquals((int)isc_database_info(fbstatus,&db,
				(short)sizeof(repinfoitems),repinfoitems,
				(short)sizeof(repinfobuffer),repinfobuffer),0);

	const char	*repinfoptr=repinfobuffer;
	const char	*repinfoendptr=repinfobuffer+sizeof(repinfobuffer);
	int		usernamecount=0;
	int		limbocount=0;
	int		errorcount=0;
	// the isc_info_user_names and isc_info_active_transactions clusters
	// repeat once per attachment and per active transaction, both of
	// which vary with whatever else is connected to the server at the
	// time - asserting inside the walk would make the number of
	// assertions this section runs non-reproducible between runs, so
	// instead track whether every cluster was well-formed here and
	// assert that once, after the walk
	bool		usernamesok=true;
	bool		activetransactionsok=true;
	while (repinfoptr<repinfoendptr &&
			*repinfoptr!=(char)isc_info_end &&
			*repinfoptr!=(char)isc_info_truncated) {

		char	repinfoitem=*repinfoptr;
		repinfoptr++;
		short	repinfolen=(short)isc_vax_integer(repinfoptr,2);
		repinfoptr+=2;

		switch (repinfoitem) {
			case isc_info_user_names:
				// a counted string, and the whole cluster is
				// repeated per attached user - there's no
				// count at the front
				if (repinfolen!=(short)(repinfoptr[0]+1) ||
						repinfoptr[0]<=0) {
					usernamesok=false;
				}
				usernamecount++;
				break;
			case isc_info_limbo:
				limbocount++;
				break;
			case isc_info_active_transactions:
				// one transaction id per cluster
				if (repinfolen!=4) {
					activetransactionsok=false;
				}
				break;
			case isc_info_error:
				// a real server refuses these two, and only
				// these two, with isc_infunk
				assertEquals((int)repinfolen,5);
				assertTrue(repinfoptr[0]==
						(char)isc_info_window_turns ||
					repinfoptr[0]==(char)isc_info_license);
				assertEquals((int)isc_vax_integer(
							repinfoptr+1,4),
							335544341);
				errorcount++;
				break;
			default:
				stdoutput.printf("unexpected db info item %d\n",
							(int)repinfoitem);
				break;
		}

		repinfoptr+=repinfolen;
	}
	assertTrue(usernamecount>=1);
	assertTrue(usernamesok);
	assertTrue(activetransactionsok);
	// a server with nothing in limbo sends no cluster at all rather than
	// an empty one
	assertEquals(limbocount,0);
	assertEquals(errorcount,2);
	assertEquals((int)*repinfoptr,(int)isc_info_end);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_detach_database\n");
	assertEquals((int)isc_detach_database(fbstatus,&db),0);
	assertTrue(db==0);
	stdoutput.printf("\n\n");


	// reattach for the rest of the run
	stdoutput.printf("isc_attach_database - reattach\n");
	assertEquals((int)isc_attach_database(fbstatus,
					(short)charstring::getLength(dbpath),
					dbpath,&db,dpbsize,dpb),0);
	assertTrue(db!=0);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============ Transactions ============\n\n");

	reattach();

	stdoutput.printf("isc_start_transaction\n");
	tr=0;
	assertEquals((int)isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb),0);
	assertTrue(tr!=0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_commit_transaction\n");
	assertEquals((int)isc_commit_transaction(fbstatus,&tr),0);
	assertTrue(tr==0);
	stdoutput.printf("\n\n");


	// scratch table for the commit/rollback cases
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
					"drop table testtran",
					SQL_DIALECT_V6,NULL);
	isc_commit_transaction(fbstatus,&tr);
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
			"create table testtran (testinteger integer)",
					SQL_DIALECT_V6,NULL);
	isc_commit_transaction(fbstatus,&tr);


	stdoutput.printf("isc_rollback_transaction\n");
	tr=0;
	assertEquals((int)isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb),0);
	assertEquals((int)isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
					"insert into testtran values (1)",
					SQL_DIALECT_V6,NULL),0);
	assertEquals(countRows("testtran"),1);
	assertEquals((int)isc_rollback_transaction(fbstatus,&tr),0);
	assertTrue(tr==0);
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	assertEquals(countRows("testtran"),0);
	isc_commit_transaction(fbstatus,&tr);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_commit_retaining\n");
	tr=0;
	assertEquals((int)isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb),0);
	assertEquals((int)isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
					"insert into testtran values (2)",
					SQL_DIALECT_V6,NULL),0);
	assertEquals((int)isc_commit_retaining(fbstatus,&tr),0);
	// the handle stays live, so a rollback after it loses nothing
	assertTrue(tr!=0);
	assertEquals((int)isc_rollback_transaction(fbstatus,&tr),0);
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	assertEquals(countRows("testtran"),1);
	isc_commit_transaction(fbstatus,&tr);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_rollback_retaining\n");
	tr=0;
	assertEquals((int)isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb),0);
	assertEquals((int)isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
					"insert into testtran values (3)",
					SQL_DIALECT_V6,NULL),0);
	assertEquals((int)isc_rollback_retaining(fbstatus,&tr),0);
	// the handle stays live, and the insert is gone
	assertTrue(tr!=0);
	assertEquals(countRows("testtran"),1);
	assertEquals((int)isc_commit_transaction(fbstatus,&tr),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_transaction_info\n");
	tr=0;
	assertEquals((int)isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb),0);
	char	trinfoitems[]={
			isc_info_tra_id,
			isc_info_end
			};
	char	trinfobuffer[256];
	bytestring::zero(trinfobuffer,sizeof(trinfobuffer));
	assertEquals((int)isc_transaction_info(fbstatus,&tr,
				(short)sizeof(trinfoitems),trinfoitems,
				(short)sizeof(trinfobuffer),trinfobuffer),0);
	assertEquals((int)trinfobuffer[0],(int)isc_info_tra_id);
	short	trinfolen=(short)isc_vax_integer(trinfobuffer+1,2);
	assertTrue(isc_vax_integer(trinfobuffer+3,trinfolen)>0);
	assertEquals((int)trinfobuffer[3+trinfolen],(int)isc_info_end);
	assertEquals((int)isc_commit_transaction(fbstatus,&tr),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_start_transaction - read only\n");
	tr=0;
	assertEquals((int)isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpbro),tpbro),0);
	assertTrue(tr!=0);
	// A write in a read-only transaction has to fail.  A real server sends
	// nothing but a bare isc_read_only_trans, and the client turns that one
	// code into the sqlcode, the sqlstate and the message below, so these
	// assertions hold against the module and a real server alike.
	char	roerrmsg[512];
	char	rosqlstate[6];
	assertTrue(isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
					"insert into testtran values (4)",
					SQL_DIALECT_V6,NULL)!=0);
	firstErrorMessage(roerrmsg,sizeof(roerrmsg));
	assertEquals(roerrmsg,
			"attempted update during read-only transaction");
	assertEquals((int)isc_sqlcode(fbstatus),-817);
	fb_sqlstate(rosqlstate,fbstatus);
	assertEquals(rosqlstate,"42000");
	// an update and a delete that match rows have to fail the same way
	assertTrue(isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
					"update testtran set testinteger=99",
					SQL_DIALECT_V6,NULL)!=0);
	assertEquals((int)isc_sqlcode(fbstatus),-817);
	assertTrue(isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
					"delete from testtran",
					SQL_DIALECT_V6,NULL)!=0);
	assertEquals((int)isc_sqlcode(fbstatus),-817);
	// a write against a table that doesn't exist still fails at prepare,
	// with the backend's own error rather than the read-only one
	assertTrue(isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
					"insert into nosuchtable values (1)",
					SQL_DIALECT_V6,NULL)!=0);
	assertEquals((int)isc_sqlcode(fbstatus),-204);
	// reads still work, and nothing was written
	assertEquals(countRows("testtran"),1);
	// ddl fails too, but with the compound reply a real server sends for
	// it - isc_no_meta_update, then a dyn code specific to the verb
	// carrying the object name, then isc_read_only_trans - rather than
	// the bare isc_read_only_trans a write gets
	assertTrue(isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
					"create table t9107tmp (a integer)",
					SQL_DIALECT_V6,NULL)!=0);
	char	ddlmsg1[512];
	char	ddlmsg2[512];
	char	ddlmsg3[512];
	const ISC_STATUS	*ddlsv=fbstatus;
	fbInterpret(ddlmsg1,sizeof(ddlmsg1),&ddlsv);
	fbInterpret(ddlmsg2,sizeof(ddlmsg2),&ddlsv);
	fbInterpret(ddlmsg3,sizeof(ddlmsg3),&ddlsv);
	assertEquals(ddlmsg1,"unsuccessful metadata update");
	assertEquals(ddlmsg2,"CREATE TABLE T9107TMP failed");
	assertEquals(ddlmsg3,"attempted update during read-only transaction");
	assertEquals((int)isc_sqlcode(fbstatus),-607);
	char	ddlsqlstate[6];
	fb_sqlstate(ddlsqlstate,fbstatus);
	assertEquals(ddlsqlstate,"42000");
	// a different verb gets its own dyn code and its own message
	assertTrue(isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
					"drop table testtran",
					SQL_DIALECT_V6,NULL)!=0);
	ddlsv=fbstatus;
	fbInterpret(ddlmsg1,sizeof(ddlmsg1),&ddlsv);
	fbInterpret(ddlmsg2,sizeof(ddlmsg2),&ddlsv);
	assertEquals(ddlmsg2,"DROP TABLE TESTTRAN failed");
	assertEquals((int)isc_sqlcode(fbstatus),-607);
	// an "execute block" that only selects isn't refused - it lands in
	// statementType()'s ddl catch-all, but a real server allows a
	// read-only one, and so does the module
	assertEquals((int)isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
					"execute block as begin end",
					SQL_DIALECT_V6,NULL),0);
	// isc_info_tra_access says read only
	char	roinfoitems[]={
			isc_info_tra_access,
			isc_info_end
			};
	char	roinfobuffer[64];
	bytestring::zero(roinfobuffer,sizeof(roinfobuffer));
	assertEquals((int)isc_transaction_info(fbstatus,&tr,
				(short)sizeof(roinfoitems),roinfoitems,
				(short)sizeof(roinfobuffer),roinfobuffer),0);
	assertEquals((int)roinfobuffer[0],(int)isc_info_tra_access);
	assertEquals((int)isc_vax_integer(roinfobuffer+1,2),1);
	assertEquals((int)roinfobuffer[3],(int)isc_info_tra_readonly);
	assertEquals((int)isc_rollback_transaction(fbstatus,&tr),0);
	// and read write for a normal tpb
	tr=0;
	assertEquals((int)isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb),0);
	bytestring::zero(roinfobuffer,sizeof(roinfobuffer));
	assertEquals((int)isc_transaction_info(fbstatus,&tr,
				(short)sizeof(roinfoitems),roinfoitems,
				(short)sizeof(roinfobuffer),roinfobuffer),0);
	assertEquals((int)roinfobuffer[3],(int)isc_info_tra_readwrite);
	assertEquals((int)isc_rollback_transaction(fbstatus,&tr),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_start_transaction - isolation level\n");
	{
	char	isoinfoitems[]={
			isc_info_tra_isolation,
			isc_info_end
			};
	char	isoinfobuffer[64];

	// consistency (snapshot table stability) - one transaction, then a
	// second and third at different levels in the same session, to prove
	// each isc_start_transaction is honored on its own, not just the
	// first one of the session
	tr=0;
	assertEquals((int)isc_start_transaction(fbstatus,&tr,1,&db,
			(unsigned short)sizeof(tpbconsistency),
			tpbconsistency),0);
	bytestring::zero(isoinfobuffer,sizeof(isoinfobuffer));
	assertEquals((int)isc_transaction_info(fbstatus,&tr,
				(short)sizeof(isoinfoitems),isoinfoitems,
				(short)sizeof(isoinfobuffer),isoinfobuffer),0);
	assertEquals((int)isoinfobuffer[0],(int)isc_info_tra_isolation);
	assertEquals((int)isoinfobuffer[3],(int)isc_info_tra_consistency);
	// ask the backend itself, not just the module, what it got
	assertEquals(selectInt("select mon$isolation_mode from "
				"mon$transactions where "
				"mon$transaction_id=current_transaction"),0);
	assertEquals((int)isc_rollback_transaction(fbstatus,&tr),0);

	// concurrency (snapshot)
	tr=0;
	assertEquals((int)isc_start_transaction(fbstatus,&tr,1,&db,
			(unsigned short)sizeof(tpbconcurrency),
			tpbconcurrency),0);
	bytestring::zero(isoinfobuffer,sizeof(isoinfobuffer));
	assertEquals((int)isc_transaction_info(fbstatus,&tr,
				(short)sizeof(isoinfoitems),isoinfoitems,
				(short)sizeof(isoinfobuffer),isoinfobuffer),0);
	assertEquals((int)isoinfobuffer[3],(int)isc_info_tra_concurrency);
	assertEquals(selectInt("select mon$isolation_mode from "
				"mon$transactions where "
				"mon$transaction_id=current_transaction"),1);
	assertEquals((int)isc_rollback_transaction(fbstatus,&tr),0);

	// read committed (record version) - the default, requested
	// explicitly this time
	tr=0;
	assertEquals((int)isc_start_transaction(fbstatus,&tr,1,&db,
			(unsigned short)sizeof(tpbreadcommitted),
			tpbreadcommitted),0);
	bytestring::zero(isoinfobuffer,sizeof(isoinfobuffer));
	assertEquals((int)isc_transaction_info(fbstatus,&tr,
				(short)sizeof(isoinfoitems),isoinfoitems,
				(short)sizeof(isoinfobuffer),isoinfobuffer),0);
	assertEquals((int)isoinfobuffer[3],(int)isc_info_tra_read_committed);
	assertEquals((int)isoinfobuffer[4],(int)isc_info_tra_rec_version);
	assertEquals(selectInt("select mon$isolation_mode from "
				"mon$transactions where "
				"mon$transaction_id=current_transaction"),2);
	assertEquals((int)isc_rollback_transaction(fbstatus,&tr),0);

	// readonly and a non-default isolation level combine
	char	tpbroconsistency[]={
		isc_tpb_version3,
		isc_tpb_read,
		isc_tpb_consistency,
		isc_tpb_wait
		};
	tr=0;
	assertEquals((int)isc_start_transaction(fbstatus,&tr,1,&db,
			(unsigned short)sizeof(tpbroconsistency),
			tpbroconsistency),0);
	char	roisoinfoitems[]={
			isc_info_tra_access,
			isc_info_tra_isolation,
			isc_info_end
			};
	bytestring::zero(isoinfobuffer,sizeof(isoinfobuffer));
	assertEquals((int)isc_transaction_info(fbstatus,&tr,
				(short)sizeof(roisoinfoitems),roisoinfoitems,
				(short)sizeof(isoinfobuffer),isoinfobuffer),0);
	assertEquals((int)isoinfobuffer[0],(int)isc_info_tra_access);
	assertEquals((int)isoinfobuffer[3],(int)isc_info_tra_readonly);
	assertEquals((int)isoinfobuffer[4],(int)isc_info_tra_isolation);
	assertEquals((int)isoinfobuffer[7],(int)isc_info_tra_consistency);
	assertEquals(selectInt("select mon$isolation_mode from "
				"mon$transactions where "
				"mon$transaction_id=current_transaction"),0);
	assertEquals((int)isc_rollback_transaction(fbstatus,&tr),0);

	// back to a plain write transaction, with no isolation level in the
	// tpb at all - falls back to read committed, same as before any of
	// this ran
	tr=0;
	assertEquals((int)isc_start_transaction(fbstatus,&tr,1,&db,
			(unsigned short)sizeof(tpb),tpb),0);
	assertEquals(selectInt("select mon$isolation_mode from "
				"mon$transactions where "
				"mon$transaction_id=current_transaction"),2);
	assertEquals((int)isc_rollback_transaction(fbstatus,&tr),0);
	}
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_start_transaction - autocommit\n");
	tr=0;
	assertEquals((int)isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpbac),tpbac),0);
	assertEquals((int)isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
					"insert into testtran values (5)",
					SQL_DIALECT_V6,NULL),0);
	// autocommit commits each statement, so the rollback loses nothing
	assertEquals((int)isc_rollback_transaction(fbstatus,&tr),0);
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	assertEquals(countRows("testtran"),2);
	isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
					"drop table testtran",
					SQL_DIALECT_V6,NULL);
	isc_commit_transaction(fbstatus,&tr);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============= Statements =============\n\n");

	reattach();

	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);

	stdoutput.printf("isc_dsql_execute_immediate - create table\n");
	// The table can't just be dropped and recreated.  Pooled
	// sqlr-connection backends autocommit with isc_commit_retaining
	// (src/connections/firebird.cpp:1171-1173), which keeps testtable
	// "in use" for DDL until they truly commit, roll back or detach, and
	// they stay attached indefinitely.  So a rerun against a live
	// instance would fail the create with -607.  Plain DML isn't blocked
	// that way, so probe with a delete instead: it clears the table if a
	// previous run left it behind, and says -204 if it isn't there yet,
	// which is the only case that needs the DDL.  Both paths run the same
	// two assertions, so the score doesn't move (#9153).
	int	tableresult=(int)isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
					"delete from testtable",
					SQL_DIALECT_V6,NULL);
	if (tableresult && isc_sqlcode(fbstatus)==-204) {
		// the table test/c++/firebird.cpp, test/c/firebird.c and
		// test/odbc/firebird.cpp share, so this test is diffable
		// against them
		tableresult=(int)isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
				"create table testtable ("
					"testinteger integer, "
					"testsmallint smallint, "
					"testdecimal decimal(10,2), "
					"testnumeric numeric(10,2), "
					"testfloat float, "
					"testdouble double precision, "
					"testdate date, "
					"testtime time, "
					"testchar char(50), "
					"testvarchar varchar(50), "
					"testtimestamp timestamp, "
					"testblob blob)",
				SQL_DIALECT_V6,NULL);
	}
	assertEquals(tableresult,0);
	assertEquals((int)isc_commit_transaction(fbstatus,&tr),0);
	stdoutput.printf("\n\n");


	tr=0;
	assertEquals((int)isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb),0);


	stdoutput.printf("isc_dsql_allocate_statement\n");
	isc_stmt_handle	stmt=0;
	assertEquals((int)isc_dsql_allocate_statement(fbstatus,&db,&stmt),0);
	assertTrue(stmt!=0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_dsql_prepare - insert\n");
	// row 1, exactly as test/c++/firebird.cpp inserts it, with
	// testtimestamp as the one NULL column
	assertEquals((int)isc_dsql_prepare(fbstatus,&tr,&stmt,0,
			"insert into testtable values "
			"(1,1,1.5,1.5,1.5,1.5,"
			"'01-JAN-2001','01:00:00',"
			"'testchar1','testvarchar1',NULL,'testblob1')",
			SQL_DIALECT_V6,NULL),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_dsql_sql_info - isc_info_sql_stmt_type\n");
	char	stmtinfoitems[]={
			isc_info_sql_stmt_type
			};
	char	stmtinfobuffer[64];
	bytestring::zero(stmtinfobuffer,sizeof(stmtinfobuffer));
	assertEquals((int)isc_dsql_sql_info(fbstatus,&stmt,
				(short)sizeof(stmtinfoitems),stmtinfoitems,
				(short)sizeof(stmtinfobuffer),stmtinfobuffer),0);
	assertEquals((int)stmtinfobuffer[0],(int)isc_info_sql_stmt_type);
	short	stmtinfolen=(short)isc_vax_integer(stmtinfobuffer+1,2);
	assertEquals((int)isc_vax_integer(stmtinfobuffer+3,stmtinfolen),
					isc_info_sql_stmt_insert);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_dsql_execute\n");
	assertEquals((int)isc_dsql_execute(fbstatus,&tr,&stmt,
						SQL_DIALECT_V6,NULL),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_dsql_sql_info - isc_info_sql_records\n");
	char	recinfoitems[]={
			isc_info_sql_records
			};
	char	recinfobuffer[128];
	bytestring::zero(recinfobuffer,sizeof(recinfobuffer));
	assertEquals((int)isc_dsql_sql_info(fbstatus,&stmt,
				(short)sizeof(recinfoitems),recinfoitems,
				(short)sizeof(recinfobuffer),recinfobuffer),0);
	assertEquals((int)recinfobuffer[0],(int)isc_info_sql_records);

	// the records reply is a cluster of sub-items, each with its own
	// item byte and 2-byte length
	const char	*recptr=recinfobuffer+3;
	int		insertcount=0;
	int		reccount=0;
	while (*recptr!=(char)isc_info_end && reccount<8) {
		char	recitem=*recptr;
		recptr++;
		short	reclen=(short)isc_vax_integer(recptr,2);
		recptr+=2;
		if (recitem==isc_info_req_insert_count) {
			insertcount=(int)isc_vax_integer(recptr,reclen);
		}
		recptr+=reclen;
		reccount++;
	}
	assertEquals(insertcount,1);
	stdoutput.printf("\n\n");


	// DSQL_close on a statement with no open cursor is an error, not a
	// no-op.  This is an insert, so executing it never opened one.
	// Measured identically on firebird 2.5, 3.0 and 4.0 under #8954.
	stdoutput.printf("isc_dsql_free_statement - DSQL_close\n");
	assertEquals((int)isc_dsql_free_statement(fbstatus,&stmt,DSQL_close),
						isc_dsql_cursor_close_err);
	assertEquals((int)isc_sqlcode(fbstatus),-501);
	assertTrue(stmt!=0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_dsql_free_statement - DSQL_drop\n");
	assertEquals((int)isc_dsql_free_statement(fbstatus,&stmt,DSQL_drop),0);
	assertTrue(stmt==0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_dsql_execute_immediate - rows 2 through 8\n");
	// the same rows test/c++/firebird.cpp binds in, one per year and
	// hour, with testtimestamp NULL throughout
	for (int row=2; row<=8; row++) {
		char	rowquery[512];
		charstring::printf(rowquery,sizeof(rowquery),
			"insert into testtable values "
			"(%d,%d,%d.5,%d.5,%d.5,%d.5,"
			"'01-JAN-200%d','0%d:00:00',"
			"'testchar%d','testvarchar%d',NULL,'testblob%d')",
			row,row,row,row,row,row,row,row,row,row,row);
		assertEquals((int)isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
					rowquery,SQL_DIALECT_V6,NULL),0);
	}
	assertEquals((int)isc_commit_transaction(fbstatus,&tr),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("row count\n");
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	assertEquals(countRows("testtable"),8);
	isc_commit_transaction(fbstatus,&tr);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============== Metadata ==============\n\n");

	reattach();

	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);

	const char	*selectquery=
			"select * from testtable order by testinteger";


	stdoutput.printf("isc_dsql_prepare - undersized XSQLDA\n");
	XSQLDA	*sqlda=(XSQLDA *)new char[XSQLDA_LENGTH(1)];
	bytestring::zero(sqlda,XSQLDA_LENGTH(1));
	sqlda->version=SQLDA_VERSION1;
	sqlda->sqln=1;
	stmt=0;
	assertEquals((int)isc_dsql_allocate_statement(fbstatus,&db,&stmt),0);
	assertEquals((int)isc_dsql_prepare(fbstatus,&tr,&stmt,0,
					selectquery,SQL_DIALECT_V6,sqlda),0);
	assertEquals(sqlda->sqld,12);
	assertTrue(sqlda->sqld>sqlda->sqln);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_dsql_prepare - resized XSQLDA\n");
	// clamp, so a dead connection's junk sqld does not blow up the
	// allocation or spin the column walk below
	int	colcount=sqlda->sqld;
	if (colcount<1 || colcount>64) {
		colcount=12;
	}
	delete[] (char *)sqlda;
	sqlda=(XSQLDA *)new char[XSQLDA_LENGTH(colcount)];
	bytestring::zero(sqlda,XSQLDA_LENGTH(colcount));
	sqlda->version=SQLDA_VERSION1;
	sqlda->sqln=(ISC_SHORT)colcount;
	assertEquals((int)isc_dsql_prepare(fbstatus,&tr,&stmt,0,
					selectquery,SQL_DIALECT_V6,sqlda),0);
	assertEquals(sqlda->sqld,12);
	assertEquals(sqlda->sqln,12);
	stdoutput.printf("\n\n");


	stdoutput.printf("column metadata\n");
	// decimal(10,2) and numeric(10,2) both land on int64 with scale -2 -
	// firebird stores 10-18 digits of precision in a bigint.  the
	// XSQLVAR carries no precision field at all, only sqltype, sqllen,
	// sqlscale and sqlsubtype, which is why the connection module has to
	// derive precision as 18+(-scale) and reports 16 rather than 10 (see
	// test/odbc/firebird.cpp:5827-5828).  sqlsubtype tells decimal (2)
	// from numeric (1).
	// only walk fixed indexes if the sqlda actually has all 12 columns -
	// a prepare against a testtable recreated with fewer columns would
	// otherwise read sqlvar entries past the allocation
	if (colcount==12) {
		assertColumn(&sqlda->sqlvar[0],"TESTINTEGER",SQL_LONG,4,0,0);
		assertColumn(&sqlda->sqlvar[1],"TESTSMALLINT",SQL_SHORT,2,0,0);
		assertColumn(&sqlda->sqlvar[2],"TESTDECIMAL",SQL_INT64,8,-2,2);
		assertColumn(&sqlda->sqlvar[3],"TESTNUMERIC",SQL_INT64,8,-2,1);
		assertColumn(&sqlda->sqlvar[4],"TESTFLOAT",SQL_FLOAT,4,0,0);
		assertColumn(&sqlda->sqlvar[5],"TESTDOUBLE",SQL_DOUBLE,8,0,0);
		assertColumn(&sqlda->sqlvar[6],"TESTDATE",SQL_TYPE_DATE,4,0,0);
		assertColumn(&sqlda->sqlvar[7],"TESTTIME",SQL_TYPE_TIME,4,0,0);
		// for SQL_TEXT and SQL_VARYING, sqlsubtype is the column's
		// character set id, and 0 is NONE.  sqllen 50 and sqlsubtype 0
		// confirmed against firebird 2.5, 3.0 and 4.0 under #8954.
		assertColumn(&sqlda->sqlvar[8],"TESTCHAR",SQL_TEXT,50,0,0);
		assertColumn(&sqlda->sqlvar[9],"TESTVARCHAR",SQL_VARYING,50,0,0);
		assertColumn(&sqlda->sqlvar[10],"TESTTIMESTAMP",
						SQL_TIMESTAMP,8,0,0);
		// for SQL_BLOB, sqllen is the size of the blob id, not the
		// data, and sqlsubtype 0 is a binary blob
		assertColumn(&sqlda->sqlvar[11],"TESTBLOB",SQL_BLOB,8,0,0);
	}
	stdoutput.printf("\n\n");


	stdoutput.printf("column types\n");
	for (int col=0; col<colcount; col++) {
		stdoutput.printf("	%s: ",sqlda->sqlvar[col].sqlname);
		printSqlType(&sqlda->sqlvar[col]);
		stdoutput.printf("\n");
	}
	stdoutput.printf("\n");


	stdoutput.printf("isc_dsql_describe\n");
	XSQLDA	*descsqlda=(XSQLDA *)new char[XSQLDA_LENGTH(colcount)];
	bytestring::zero(descsqlda,XSQLDA_LENGTH(colcount));
	descsqlda->version=SQLDA_VERSION1;
	descsqlda->sqln=(ISC_SHORT)colcount;
	assertEquals((int)isc_dsql_describe(fbstatus,&stmt,
					SQL_DIALECT_V6,descsqlda),0);
	assertEquals(descsqlda->sqld,12);
	// describe has to agree with what prepare already said
	for (int col=0; col<colcount; col++) {
		assertEquals(descsqlda->sqlvar[col].sqltype,
					sqlda->sqlvar[col].sqltype);
		assertEquals(descsqlda->sqlvar[col].sqllen,
					sqlda->sqlvar[col].sqllen);
		assertEquals(descsqlda->sqlvar[col].sqlscale,
					sqlda->sqlvar[col].sqlscale);
		assertEquals(descsqlda->sqlvar[col].sqlsubtype,
					sqlda->sqlvar[col].sqlsubtype);
		assertEquals(descsqlda->sqlvar[col].sqlname,
					sqlda->sqlvar[col].sqlname);
	}
	delete[] (char *)descsqlda;
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_dsql_sql_info - isc_info_sql_select\n");
	char	selinfoitems[]={
			isc_info_sql_select,
			isc_info_sql_describe_vars,
			isc_info_sql_sqlda_seq,
			isc_info_sql_type,
			isc_info_sql_sub_type,
			isc_info_sql_scale,
			isc_info_sql_length,
			isc_info_sql_field,
			isc_info_sql_relation,
			isc_info_sql_owner,
			isc_info_sql_alias,
			isc_info_sql_describe_end
			};
	char	selinfobuffer[8192];
	bytestring::zero(selinfobuffer,sizeof(selinfobuffer));
	assertEquals((int)isc_dsql_sql_info(fbstatus,&stmt,
				(short)sizeof(selinfoitems),selinfoitems,
				(short)sizeof(selinfobuffer),selinfobuffer),0);
	// the reply opens with isc_info_sql_select, then
	// isc_info_sql_describe_vars and a 2-byte length, then the column
	// count, then one isc_info_sql_sqlda_seq..isc_info_sql_describe_end
	// group per column
	assertEquals((int)selinfobuffer[0],(int)isc_info_sql_select);
	assertEquals((int)selinfobuffer[1],(int)isc_info_sql_describe_vars);
	short	selinfolen=(short)isc_vax_integer(selinfobuffer+2,2);
	assertEquals((int)isc_vax_integer(selinfobuffer+4,selinfolen),12);
	assertEquals((int)selinfobuffer[4+selinfolen],
					(int)isc_info_sql_sqlda_seq);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n================ Fetch ===============\n\n");

	reattach();

	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);

	stmt=0;
	assertEquals((int)isc_dsql_allocate_statement(fbstatus,&db,&stmt),0);
	bytestring::zero(sqlda,XSQLDA_LENGTH(colcount));
	sqlda->version=SQLDA_VERSION1;
	sqlda->sqln=(ISC_SHORT)colcount;
	assertEquals((int)isc_dsql_prepare(fbstatus,&tr,&stmt,0,
					selectquery,SQL_DIALECT_V6,sqlda),0);


	stdoutput.printf("output buffers\n");
	short	sqlind[64];
	char	*databuffer[64];
	colcount=bindOutputBuffers(sqlda,sqlind,databuffer);
	assertEquals(colcount,12);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_dsql_execute\n");
	assertEquals((int)isc_dsql_execute(fbstatus,&tr,&stmt,
						SQL_DIALECT_V6,NULL),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_dsql_fetch\n");
	int		row=0;
	ISC_STATUS	fetchresult=0;
	// a describe that came back with fewer columns than expected leaves
	// the later sqlvar sqldata unset - stop rather than read past it
	while (row<8 && colcount==12) {

		fetchresult=isc_dsql_fetch(fbstatus,&stmt,
						SQL_DIALECT_V6,sqlda);
		if (fetchresult) {
			break;
		}
		row++;

		// testinteger
		assertEquals((int)sqlind[0],0);
		assertEquals((int)*((ISC_LONG *)sqlda->sqlvar[0].sqldata),row);

		// testsmallint
		assertEquals((int)sqlind[1],0);
		assertEquals((int)*((ISC_SHORT *)sqlda->sqlvar[1].sqldata),row);

		// testdecimal - int64 with sqlscale -2, so N.5 is stored as
		// N*100+50
		assertEquals((int)sqlind[2],0);
		assertEquals((int)sqlda->sqlvar[2].sqlscale,-2);
		assertEquals((int)*((ISC_INT64 *)sqlda->sqlvar[2].sqldata),
								row*100+50);

		// testnumeric
		assertEquals((int)sqlind[3],0);
		assertEquals((int)sqlda->sqlvar[3].sqlscale,-2);
		assertEquals((int)*((ISC_INT64 *)sqlda->sqlvar[3].sqldata),
								row*100+50);

		// testfloat
		assertEquals((int)sqlind[4],0);
		assertTrue(*((float *)sqlda->sqlvar[4].sqldata)==
						(float)(row+0.5));

		// testdouble
		assertEquals((int)sqlind[5],0);
		assertTrue(*((double *)sqlda->sqlvar[5].sqldata)==row+0.5);

		// testdate
		assertEquals((int)sqlind[6],0);
		struct tm	dateval;
		isc_decode_sql_date((ISC_DATE *)sqlda->sqlvar[6].sqldata,
								&dateval);
		assertEquals(dateval.tm_year+1900,2000+row);
		assertEquals(dateval.tm_mon+1,1);
		assertEquals(dateval.tm_mday,1);

		// testtime
		assertEquals((int)sqlind[7],0);
		struct tm	timeval;
		isc_decode_sql_time((ISC_TIME *)sqlda->sqlvar[7].sqldata,
								&timeval);
		assertEquals(timeval.tm_hour,row);
		assertEquals(timeval.tm_min,0);
		assertEquals(timeval.tm_sec,0);

		// testchar - blank padded out to the column's 50 characters
		assertEquals((int)sqlind[8],0);
		char	expectedchar[51];
		charstring::printf(expectedchar,sizeof(expectedchar),
						"testchar%d",row);
		size_t	charlen=charstring::getLength(expectedchar);
		bytestring::set(expectedchar+charlen,' ',50-charlen);
		expectedchar[50]='\0';
		assertEquals(sqlda->sqlvar[8].sqldata,expectedchar);

		// testvarchar - a PARAMVARY, so a 2-byte host-order length
		// then the data, unpadded
		assertEquals((int)sqlind[9],0);
		PARAMVARY	*vary=
			(PARAMVARY *)sqlda->sqlvar[9].sqldata;
		char	varcharval[64];
		charstring::copy(varcharval,(char *)vary->vary_string,
						vary->vary_length);
		varcharval[vary->vary_length]='\0';
		char	expectedvarchar[64];
		charstring::printf(expectedvarchar,sizeof(expectedvarchar),
						"testvarchar%d",row);
		assertEquals((int)vary->vary_length,12);
		assertEquals(varcharval,expectedvarchar);

		// testtimestamp - the one column inserted as NULL
		assertEquals((int)sqlind[10],-1);

		// testblob - an ISC_QUAD blob id, not the data
		assertEquals((int)sqlind[11],0);
		ISC_QUAD	*blobid=
			(ISC_QUAD *)sqlda->sqlvar[11].sqldata;
		assertTrue(blobid->gds_quad_high!=0 ||
					blobid->gds_quad_low!=0);
	}
	assertEquals(row,8);

	// the fetch past the last row returns 100
	if (!fetchresult) {
		fetchresult=isc_dsql_fetch(fbstatus,&stmt,
						SQL_DIALECT_V6,sqlda);
	}
	assertEquals((int)fetchresult,100);
	stdoutput.printf("\n\n");


	isc_dsql_free_statement(fbstatus,&stmt,DSQL_drop);
	isc_commit_transaction(fbstatus,&tr);

	freeOutputBuffers(colcount,databuffer);



	stdoutput.printf("\n=============== Binds ================\n\n");

	reattach();

	// this whole section rolls back at the end, so testtable is still
	// the 8 rows the sections below expect
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);

	const char	*insertquery=
			"insert into testtable values (?,?,?,?,?,?,?,?,?,?,?,?)";


	stdoutput.printf("isc_dsql_describe_bind\n");
	XSQLDA	*insqlda=(XSQLDA *)new char[XSQLDA_LENGTH(12)];
	bytestring::zero(insqlda,XSQLDA_LENGTH(12));
	insqlda->version=SQLDA_VERSION1;
	insqlda->sqln=12;
	stmt=0;
	assertEquals((int)isc_dsql_allocate_statement(fbstatus,&db,&stmt),0);
	assertEquals((int)isc_dsql_prepare(fbstatus,&tr,&stmt,0,
					insertquery,SQL_DIALECT_V6,NULL),0);
	assertEquals((int)isc_dsql_describe_bind(fbstatus,&stmt,
					SQL_DIALECT_V6,insqlda),0);
	assertEquals(insqlda->sqld,12);
	stdoutput.printf("\n\n");


	stdoutput.printf("input values\n");
	// row 7's values, from test/c++/firebird.cpp:17-22
	ISC_LONG	binteger=7;
	ISC_SHORT	bsmallint=7;
	// decimal(10,2) and numeric(10,2) carry sqlscale -2, so 7.5 goes in
	// as 750
	ISC_INT64	bdecimal=750;
	ISC_INT64	bnumeric=750;
	float		bfloat=7.5;
	double		bdouble=7.5;

	struct tm	btm;
	bytestring::zero(&btm,sizeof(btm));
	btm.tm_year=2007-1900;
	btm.tm_mday=1;
	ISC_DATE	bdate;
	isc_encode_sql_date(&btm,&bdate);

	bytestring::zero(&btm,sizeof(btm));
	btm.tm_hour=7;
	ISC_TIME	btime;
	isc_encode_sql_time(&btm,&btime);

	char	bchar[51];
	charstring::printf(bchar,sizeof(bchar),"testchar7");
	bytestring::set(bchar+charstring::getLength(bchar),' ',
				50-charstring::getLength(bchar));
	bchar[50]='\0';

	char		bvarchar[54];
	PARAMVARY	*bvary=(PARAMVARY *)bvarchar;
	bvary->vary_length=12;
	charstring::copy((char *)bvary->vary_string,"testvarchar7",12);

	// somewhere for the null columns to point
	char	bnull[16];
	bytestring::zero(bnull,sizeof(bnull));

	short	inind[12];
	bytestring::zero(inind,sizeof(inind));

	insqlda->sqlvar[0].sqldata=(char *)&binteger;
	insqlda->sqlvar[1].sqldata=(char *)&bsmallint;
	insqlda->sqlvar[2].sqldata=(char *)&bdecimal;
	insqlda->sqlvar[3].sqldata=(char *)&bnumeric;
	insqlda->sqlvar[4].sqldata=(char *)&bfloat;
	insqlda->sqlvar[5].sqldata=(char *)&bdouble;
	insqlda->sqlvar[6].sqldata=(char *)&bdate;
	insqlda->sqlvar[7].sqldata=(char *)&btime;
	insqlda->sqlvar[8].sqldata=bchar;
	insqlda->sqlvar[9].sqldata=bvarchar;
	insqlda->sqlvar[10].sqldata=bnull;
	insqlda->sqlvar[11].sqldata=bnull;
	for (int col=0; col<12; col++) {
		insqlda->sqlvar[col].sqlind=&inind[col];
		// force the nullable bit on, so the indicator is honoured
		insqlda->sqlvar[col].sqltype|=1;
	}

	// testtimestamp is row 7's NULL column, and testblob is bound in
	// the blobs section, which has an ISC_QUAD to bind
	inind[10]=-1;
	inind[11]=-1;

	// the bind describe has to agree with the table
	assertEquals(insqlda->sqlvar[0].sqltype&~1,SQL_LONG);
	assertEquals(insqlda->sqlvar[1].sqltype&~1,SQL_SHORT);
	assertEquals(insqlda->sqlvar[2].sqltype&~1,SQL_INT64);
	assertEquals(insqlda->sqlvar[2].sqlscale,-2);
	assertEquals(insqlda->sqlvar[3].sqltype&~1,SQL_INT64);
	assertEquals(insqlda->sqlvar[4].sqltype&~1,SQL_FLOAT);
	assertEquals(insqlda->sqlvar[5].sqltype&~1,SQL_DOUBLE);
	assertEquals(insqlda->sqlvar[6].sqltype&~1,SQL_TYPE_DATE);
	assertEquals(insqlda->sqlvar[7].sqltype&~1,SQL_TYPE_TIME);
	assertEquals(insqlda->sqlvar[8].sqltype&~1,SQL_TEXT);
	assertEquals(insqlda->sqlvar[9].sqltype&~1,SQL_VARYING);
	assertEquals(insqlda->sqlvar[10].sqltype&~1,SQL_TIMESTAMP);
	assertEquals(insqlda->sqlvar[11].sqltype&~1,SQL_BLOB);
	stdoutput.printf("\n\n");


	// firebird binds by position only.  test/c++/firebird.cpp:236-247
	// records that it has no bind by name, so there is no named-bind
	// case here.


	stdoutput.printf("isc_dsql_execute - bound insert\n");
	assertEquals((int)isc_dsql_execute(fbstatus,&tr,&stmt,
					SQL_DIALECT_V6,insqlda),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("round trip\n");
	XSQLDA	*rtsqlda=(XSQLDA *)new char[XSQLDA_LENGTH(12)];
	bytestring::zero(rtsqlda,XSQLDA_LENGTH(12));
	rtsqlda->version=SQLDA_VERSION1;
	rtsqlda->sqln=12;
	isc_stmt_handle	rtstmt=0;
	assertEquals((int)isc_dsql_allocate_statement(fbstatus,&db,&rtstmt),0);
	assertEquals((int)isc_dsql_prepare(fbstatus,&tr,&rtstmt,0,
			"select * from testtable where testinteger=7",
			SQL_DIALECT_V6,rtsqlda),0);
	short	rtind[64];
	char	*rtbuffer[64];
	int	rtcolcount=bindOutputBuffers(rtsqlda,rtind,rtbuffer);
	assertEquals(rtcolcount,12);
	assertEquals((int)isc_dsql_execute(fbstatus,&tr,&rtstmt,
						SQL_DIALECT_V6,NULL),0);
	assertEquals((int)isc_dsql_fetch(fbstatus,&rtstmt,
						SQL_DIALECT_V6,rtsqlda),0);
	// only read the sqlda if the prepare filled it in - a failed
	// prepare leaves every sqldata NULL
	if (rtcolcount==12) {
		assertEquals((int)*((ISC_LONG *)rtsqlda->sqlvar[0].sqldata),7);
		assertEquals((int)*((ISC_SHORT *)rtsqlda->sqlvar[1].sqldata),7);
		assertEquals((int)*((ISC_INT64 *)rtsqlda->sqlvar[2].sqldata),750);
		assertEquals((int)*((ISC_INT64 *)rtsqlda->sqlvar[3].sqldata),750);
		assertTrue(*((float *)rtsqlda->sqlvar[4].sqldata)==(float)7.5);
		assertTrue(*((double *)rtsqlda->sqlvar[5].sqldata)==7.5);
		struct tm	rtdate;
		isc_decode_sql_date((ISC_DATE *)rtsqlda->sqlvar[6].sqldata,
								&rtdate);
		assertEquals(rtdate.tm_year+1900,2007);
		assertEquals(rtdate.tm_mon+1,1);
		assertEquals(rtdate.tm_mday,1);
		struct tm	rttime;
		isc_decode_sql_time((ISC_TIME *)rtsqlda->sqlvar[7].sqldata,
								&rttime);
		assertEquals(rttime.tm_hour,7);
		assertEquals(rttime.tm_min,0);
		assertEquals(rttime.tm_sec,0);
		assertEquals(rtsqlda->sqlvar[8].sqldata,bchar);
		PARAMVARY	*rtvary=(PARAMVARY *)rtsqlda->sqlvar[9].sqldata;
		assertEquals((int)rtvary->vary_length,12);
		char	rtvarchar[64];
		charstring::copy(rtvarchar,(char *)rtvary->vary_string,
							rtvary->vary_length);
		rtvarchar[rtvary->vary_length]='\0';
		assertEquals(rtvarchar,"testvarchar7");
		assertEquals((int)rtind[10],-1);
	}
	// two rows now - the one the statements section inserted and the one
	// just bound in
	assertEquals((int)isc_dsql_fetch(fbstatus,&rtstmt,
						SQL_DIALECT_V6,rtsqlda),0);
	assertEquals((int)isc_dsql_fetch(fbstatus,&rtstmt,
						SQL_DIALECT_V6,rtsqlda),100);
	isc_dsql_free_statement(fbstatus,&rtstmt,DSQL_drop);
	freeOutputBuffers(rtcolcount,rtbuffer);
	delete[] (char *)rtsqlda;
	stdoutput.printf("\n\n");


	// #9069 and #9144: a select with a bind variable used to describe as
	// 0 columns, because nothing ran it at prepare time.  isc_dsql_prepare
	// now has to answer with the real shape.
	stdoutput.printf("select with bind variable - prepare and describe\n");
	const char	*bindselectquery=
			"select testinteger, testvarchar from testtable "
			"where testinteger=?";
	XSQLDA	*bselsqlda=(XSQLDA *)new char[XSQLDA_LENGTH(2)];
	bytestring::zero(bselsqlda,XSQLDA_LENGTH(2));
	bselsqlda->version=SQLDA_VERSION1;
	bselsqlda->sqln=2;
	isc_stmt_handle	bselstmt=0;
	assertEquals((int)isc_dsql_allocate_statement(fbstatus,&db,
							&bselstmt),0);
	assertEquals((int)isc_dsql_prepare(fbstatus,&tr,&bselstmt,0,
					bindselectquery,SQL_DIALECT_V6,
					bselsqlda),0);
	assertEquals(bselsqlda->sqld,2);
	assertEquals(bselsqlda->sqlvar[0].aliasname,"TESTINTEGER");
	assertEquals(bselsqlda->sqlvar[0].sqltype&~1,SQL_LONG);
	assertEquals(bselsqlda->sqlvar[1].aliasname,"TESTVARCHAR");
	assertEquals(bselsqlda->sqlvar[1].sqltype&~1,SQL_VARYING);
	stdoutput.printf("\n\n");


	stdoutput.printf("select with bind variable - describe_bind\n");
	XSQLDA	*bindselinsqlda=(XSQLDA *)new char[XSQLDA_LENGTH(1)];
	bytestring::zero(bindselinsqlda,XSQLDA_LENGTH(1));
	bindselinsqlda->version=SQLDA_VERSION1;
	bindselinsqlda->sqln=1;
	assertEquals((int)isc_dsql_describe_bind(fbstatus,&bselstmt,
					SQL_DIALECT_V6,bindselinsqlda),0);
	assertEquals(bindselinsqlda->sqld,1);
	// no assert on the parameter's own type - see bindInputValue()
	stdoutput.printf("\n\n");


	stdoutput.printf("select with bind variable - execute and fetch\n");
	short	bselvalind=0;
	char	*bselinbuffer=bindInputValue(&bindselinsqlda->sqlvar[0],
							&bselvalind,7,"7");
	short	bseloutind[2];
	char	*bseloutbuffer[2];
	int	bseloutcolcount=bindOutputBuffers(bselsqlda,bseloutind,
							bseloutbuffer);
	assertEquals(bseloutcolcount,2);
	assertEquals((int)isc_dsql_execute(fbstatus,&tr,&bselstmt,
					SQL_DIALECT_V6,bindselinsqlda),0);
	assertEquals((int)isc_dsql_fetch(fbstatus,&bselstmt,
					SQL_DIALECT_V6,bselsqlda),0);
	// only read the sqlda if the prepare filled it in - a failed
	// prepare leaves every sqldata NULL
	if (bseloutcolcount==2) {
		assertEquals((int)bseloutind[0],0);
		assertEquals((int)*((ISC_LONG *)bselsqlda->sqlvar[0].sqldata),
									7);
		assertEquals((int)bseloutind[1],0);
		PARAMVARY	*bselvary=
				(PARAMVARY *)bselsqlda->sqlvar[1].sqldata;
		char	bselvarchar[64];
		charstring::copy(bselvarchar,(char *)bselvary->vary_string,
						bselvary->vary_length);
		bselvarchar[bselvary->vary_length]='\0';
		assertEquals(bselvarchar,"testvarchar7");
	}
	// two rows now match testinteger=7 - the one the statements section
	// inserted and the one the binds section's round trip bound in
	// above, both with identical column values
	assertEquals((int)isc_dsql_fetch(fbstatus,&bselstmt,
					SQL_DIALECT_V6,bselsqlda),0);
	if (bseloutcolcount==2) {
		assertEquals((int)bseloutind[0],0);
		assertEquals((int)*((ISC_LONG *)bselsqlda->sqlvar[0].sqldata),
									7);
		assertEquals((int)bseloutind[1],0);
		PARAMVARY	*bselvary=
				(PARAMVARY *)bselsqlda->sqlvar[1].sqldata;
		char	bselvarchar[64];
		charstring::copy(bselvarchar,(char *)bselvary->vary_string,
						bselvary->vary_length);
		bselvarchar[bselvary->vary_length]='\0';
		assertEquals(bselvarchar,"testvarchar7");
	}
	assertEquals((int)isc_dsql_fetch(fbstatus,&bselstmt,
					SQL_DIALECT_V6,bselsqlda),100);
	freeOutputBuffers(bseloutcolcount,bseloutbuffer);
	isc_dsql_free_statement(fbstatus,&bselstmt,DSQL_drop);
	delete[] bselinbuffer;
	delete[] (char *)bindselinsqlda;
	delete[] (char *)bselsqlda;
	stdoutput.printf("\n\n");


	// #9167: the same shape with two bind variables.  Under
	// fakeinputbindvariables=yes the server prepares nothing at prepare
	// time, so the protocol module has to probe the shape (with NULL
	// substituted for every bind) to answer this at all.  The default
	// instance answers it from the real prepare (#9144), and a native
	// firebird from its own, so the block reads the same against all
	// three - the probe only widens where a correct describe comes back.
	stdoutput.printf("select with two bind variables - "
					"prepare and describe\n");
	const char	*bindselect2query=
			"select testinteger, testvarchar from testtable "
			"where testinteger=? and testvarchar=?";
	XSQLDA	*bsel2sqlda=(XSQLDA *)new char[XSQLDA_LENGTH(2)];
	bytestring::zero(bsel2sqlda,XSQLDA_LENGTH(2));
	bsel2sqlda->version=SQLDA_VERSION1;
	bsel2sqlda->sqln=2;
	isc_stmt_handle	bsel2stmt=0;
	assertEquals((int)isc_dsql_allocate_statement(fbstatus,&db,
							&bsel2stmt),0);
	assertEquals((int)isc_dsql_prepare(fbstatus,&tr,&bsel2stmt,0,
					bindselect2query,SQL_DIALECT_V6,
					bsel2sqlda),0);
	assertEquals(bsel2sqlda->sqld,2);
	assertEquals(bsel2sqlda->sqlvar[0].aliasname,"TESTINTEGER");
	assertEquals(bsel2sqlda->sqlvar[0].sqltype&~1,SQL_LONG);
	assertEquals(bsel2sqlda->sqlvar[1].aliasname,"TESTVARCHAR");
	assertEquals(bsel2sqlda->sqlvar[1].sqltype&~1,SQL_VARYING);
	stdoutput.printf("\n\n");


	stdoutput.printf("select with two bind variables - describe_bind\n");
	XSQLDA	*bsel2insqlda=(XSQLDA *)new char[XSQLDA_LENGTH(2)];
	bytestring::zero(bsel2insqlda,XSQLDA_LENGTH(2));
	bsel2insqlda->version=SQLDA_VERSION1;
	bsel2insqlda->sqln=2;
	assertEquals((int)isc_dsql_describe_bind(fbstatus,&bsel2stmt,
					SQL_DIALECT_V6,bsel2insqlda),0);
	assertEquals(bsel2insqlda->sqld,2);
	// no assert on the parameters' own types - see bindInputValue()
	stdoutput.printf("\n\n");


	stdoutput.printf("select with two bind variables - execute and fetch\n");
	short	bsel2inind[2];
	bytestring::zero(bsel2inind,sizeof(bsel2inind));
	char	*bsel2inbuffer[2];
	bsel2inbuffer[0]=bindInputValue(&bsel2insqlda->sqlvar[0],
						&bsel2inind[0],7,"7");
	bsel2inbuffer[1]=bindInputValue(&bsel2insqlda->sqlvar[1],
						&bsel2inind[1],0,"testvarchar7");
	short	bsel2outind[2];
	char	*bsel2outbuffer[2];
	int	bsel2outcolcount=bindOutputBuffers(bsel2sqlda,bsel2outind,
							bsel2outbuffer);
	assertEquals(bsel2outcolcount,2);
	assertEquals((int)isc_dsql_execute(fbstatus,&tr,&bsel2stmt,
					SQL_DIALECT_V6,bsel2insqlda),0);
	// two rows match testinteger=7 and testvarchar='testvarchar7' - the
	// one the statements section inserted and the one the round trip
	// above bound in, both with identical column values
	for (int row=0; row<2; row++) {
		assertEquals((int)isc_dsql_fetch(fbstatus,&bsel2stmt,
						SQL_DIALECT_V6,bsel2sqlda),0);
		// only read the sqlda if the prepare filled it in - a failed
		// prepare leaves every sqldata NULL
		if (bsel2outcolcount==2) {
			assertEquals((int)bsel2outind[0],0);
			assertEquals((int)*((ISC_LONG *)
					bsel2sqlda->sqlvar[0].sqldata),7);
			assertEquals((int)bsel2outind[1],0);
			PARAMVARY	*bsel2vary=
				(PARAMVARY *)bsel2sqlda->sqlvar[1].sqldata;
			char	bsel2varchar[64];
			charstring::copy(bsel2varchar,
					(char *)bsel2vary->vary_string,
					bsel2vary->vary_length);
			bsel2varchar[bsel2vary->vary_length]='\0';
			assertEquals(bsel2varchar,"testvarchar7");
		}
	}
	assertEquals((int)isc_dsql_fetch(fbstatus,&bsel2stmt,
					SQL_DIALECT_V6,bsel2sqlda),100);
	freeOutputBuffers(bsel2outcolcount,bsel2outbuffer);
	isc_dsql_free_statement(fbstatus,&bsel2stmt,DSQL_drop);
	delete[] bsel2inbuffer[0];
	delete[] bsel2inbuffer[1];
	delete[] (char *)bsel2insqlda;
	delete[] (char *)bsel2sqlda;
	stdoutput.printf("\n\n");


	// #9167: a bind variable in the select list itself is the one shape
	// the probe cannot handle - firebird rejects a bare untyped NULL
	// there, so the NULL-substituted probe fails to prepare, nothing gets
	// cached, and the describe still answers 0 columns.  That is accepted,
	// so nothing is asserted about the prepare itself: a native firebird
	// and the default instance fail it outright with a datatype error,
	// while the faked-bind instance has nothing prepared to fail.  What
	// this pins is only that it degrades rather than killing the session -
	// no crash, no hang, and an ordinary query still runs afterwards.
	stdoutput.printf("select with a bind variable in the select list\n");
	XSQLDA	*slsqlda=(XSQLDA *)new char[XSQLDA_LENGTH(2)];
	bytestring::zero(slsqlda,XSQLDA_LENGTH(2));
	slsqlda->version=SQLDA_VERSION1;
	slsqlda->sqln=2;
	isc_stmt_handle	slstmt=0;
	assertEquals((int)isc_dsql_allocate_statement(fbstatus,&db,&slstmt),0);
	ISC_STATUS	slprepared=isc_dsql_prepare(fbstatus,&tr,&slstmt,0,
			"select ?, testvarchar from testtable "
			"where testinteger=1",
			SQL_DIALECT_V6,slsqlda);
	stdoutput.printf("	prepare returned %d, describes %d columns\n",
					(int)slprepared,(int)slsqlda->sqld);
	if (slprepared) {
		char	slerrmsg[512];
		firstErrorMessage(slerrmsg,sizeof(slerrmsg));
		stdoutput.printf("	error: %s\n",slerrmsg);
	}
	isc_dsql_free_statement(fbstatus,&slstmt,DSQL_drop);
	delete[] (char *)slsqlda;
	stdoutput.printf("\n\n");


	stdoutput.printf("ordinary query after the select-list bind\n");
	XSQLDA	*aftersqlda=(XSQLDA *)new char[XSQLDA_LENGTH(1)];
	bytestring::zero(aftersqlda,XSQLDA_LENGTH(1));
	aftersqlda->version=SQLDA_VERSION1;
	aftersqlda->sqln=1;
	isc_stmt_handle	afterstmt=0;
	assertEquals((int)isc_dsql_allocate_statement(fbstatus,&db,
							&afterstmt),0);
	assertEquals((int)isc_dsql_prepare(fbstatus,&tr,&afterstmt,0,
			"select testinteger from testtable where testinteger=1",
			SQL_DIALECT_V6,aftersqlda),0);
	assertEquals(aftersqlda->sqld,1);
	short	afterind[1];
	char	*afterbuffer[1];
	int	aftercolcount=bindOutputBuffers(aftersqlda,afterind,afterbuffer);
	assertEquals(aftercolcount,1);
	assertEquals((int)isc_dsql_execute(fbstatus,&tr,&afterstmt,
						SQL_DIALECT_V6,NULL),0);
	assertEquals((int)isc_dsql_fetch(fbstatus,&afterstmt,
						SQL_DIALECT_V6,aftersqlda),0);
	if (aftercolcount==1) {
		assertEquals((int)*((ISC_LONG *)aftersqlda->sqlvar[0].sqldata),
										1);
	}
	// only row 1 has testinteger=1
	assertEquals((int)isc_dsql_fetch(fbstatus,&afterstmt,
						SQL_DIALECT_V6,aftersqlda),100);
	freeOutputBuffers(aftercolcount,afterbuffer);
	isc_dsql_free_statement(fbstatus,&afterstmt,DSQL_drop);
	delete[] (char *)aftersqlda;
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_dsql_execute - null binds\n");
	for (int col=0; col<12; col++) {
		// null this column, leave the rest as row 7's values
		short	savedind=inind[col];
		inind[col]=-1;
		assertEquals((int)isc_dsql_execute(fbstatus,&tr,&stmt,
						SQL_DIALECT_V6,insqlda),0);
		inind[col]=savedind;
	}
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_dsql_exec_immed2\n");
	assertEquals((int)isc_dsql_exec_immed2(fbstatus,&db,&tr,0,
					insertquery,SQL_DIALECT_V6,
					insqlda,NULL),0);
	stdoutput.printf("\n\n");


	// isc_dsql_insert is the legacy interbase insert-through-a-cursor
	// entry point, op_insert on the wire.  fbclient 3.0 and up refuses it
	// outright with isc_feature_removed, so nothing reaches the server and
	// nothing reaches a protocol module either.  Measured identically
	// against firebird 2.5, 3.0 and 4.0 under #8954, and 2.5 servers do
	// still implement op_insert, which is what places the refusal in the
	// client.
	stdoutput.printf("isc_dsql_insert\n");
	assertEquals((int)isc_dsql_insert(fbstatus,&stmt,
					SQL_DIALECT_V6,insqlda),
					isc_feature_removed);
	assertEquals((int)isc_sqlcode(fbstatus),-901);
	stdoutput.printf("\n\n");


	isc_dsql_free_statement(fbstatus,&stmt,DSQL_drop);
	delete[] (char *)insqlda;

	// back to the 8 rows
	isc_rollback_transaction(fbstatus,&tr);
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	assertEquals(countRows("testtable"),8);
	isc_commit_transaction(fbstatus,&tr);



	stdoutput.printf("\n=============== Blobs ================\n\n");

	reattach();

	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);

	// big enough to need more than one segment
	const int	blobsegmentsize=4096;
	const int	blobsegmentcount=5;
	const int	blobsize=blobsegmentsize*blobsegmentcount;
	char		*blobdata=new char[blobsize];
	for (int i=0; i<blobsize; i++) {
		blobdata[i]=(char)('a'+(i%26));
	}


	stdoutput.printf("isc_create_blob2\n");
	isc_blob_handle	blobhandle=0;
	ISC_QUAD	blobid;
	bytestring::zero(&blobid,sizeof(blobid));
	// no blob parameter buffer - testblob is a plain binary blob
	assertEquals((int)isc_create_blob2(fbstatus,&db,&tr,&blobhandle,
						&blobid,0,NULL),0);
	assertTrue(blobhandle!=0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_put_segment\n");
	// five segments, so the client sends more than one, which is what
	// makes the module's op_batch_segment path matter
	for (int seg=0; seg<blobsegmentcount; seg++) {
		assertEquals((int)isc_put_segment(fbstatus,&blobhandle,
				(unsigned short)blobsegmentsize,
				blobdata+seg*blobsegmentsize),0);
	}
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_close_blob\n");
	assertEquals((int)isc_close_blob(fbstatus,&blobhandle),0);
	assertTrue(blobhandle==0);
	stdoutput.printf("\n\n");


	stdoutput.printf("bind the blob id\n");
	XSQLDA	*blobsqlda=(XSQLDA *)new char[XSQLDA_LENGTH(2)];
	bytestring::zero(blobsqlda,XSQLDA_LENGTH(2));
	blobsqlda->version=SQLDA_VERSION1;
	blobsqlda->sqln=2;
	isc_stmt_handle	blobstmt=0;
	assertEquals((int)isc_dsql_allocate_statement(fbstatus,&db,
							&blobstmt),0);
	assertEquals((int)isc_dsql_prepare(fbstatus,&tr,&blobstmt,0,
		"insert into testtable (testinteger,testblob) values (?,?)",
		SQL_DIALECT_V6,NULL),0);
	assertEquals((int)isc_dsql_describe_bind(fbstatus,&blobstmt,
						SQL_DIALECT_V6,blobsqlda),0);
	assertEquals(blobsqlda->sqld,2);
	assertEquals(blobsqlda->sqlvar[1].sqltype&~1,SQL_BLOB);
	// row 9, so it can be deleted again and leave the 8 rows alone
	ISC_LONG	blobrowid=9;
	short		blobind[2];
	blobind[0]=0;
	blobind[1]=0;
	blobsqlda->sqlvar[0].sqldata=(char *)&blobrowid;
	blobsqlda->sqlvar[0].sqlind=&blobind[0];
	blobsqlda->sqlvar[0].sqltype|=1;
	blobsqlda->sqlvar[1].sqldata=(char *)&blobid;
	blobsqlda->sqlvar[1].sqlind=&blobind[1];
	blobsqlda->sqlvar[1].sqltype|=1;
	assertEquals((int)isc_dsql_execute(fbstatus,&tr,&blobstmt,
						SQL_DIALECT_V6,blobsqlda),0);
	isc_dsql_free_statement(fbstatus,&blobstmt,DSQL_drop);
	delete[] (char *)blobsqlda;
	assertEquals((int)isc_commit_transaction(fbstatus,&tr),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("select the blob id back\n");
	tr=0;
	assertEquals((int)isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb),0);
	XSQLDA	*rdsqlda=(XSQLDA *)new char[XSQLDA_LENGTH(1)];
	bytestring::zero(rdsqlda,XSQLDA_LENGTH(1));
	rdsqlda->version=SQLDA_VERSION1;
	rdsqlda->sqln=1;
	isc_stmt_handle	rdstmt=0;
	assertEquals((int)isc_dsql_allocate_statement(fbstatus,&db,&rdstmt),0);
	assertEquals((int)isc_dsql_prepare(fbstatus,&tr,&rdstmt,0,
			"select testblob from testtable where testinteger=9",
			SQL_DIALECT_V6,rdsqlda),0);
	short	rdind[64];
	char	*rdbuffer[64];
	int	rdcolcount=bindOutputBuffers(rdsqlda,rdind,rdbuffer);
	assertEquals(rdcolcount,1);
	assertEquals((int)isc_dsql_execute(fbstatus,&tr,&rdstmt,
						SQL_DIALECT_V6,NULL),0);
	assertEquals((int)isc_dsql_fetch(fbstatus,&rdstmt,
						SQL_DIALECT_V6,rdsqlda),0);
	ISC_QUAD	readblobid;
	bytestring::zero(&readblobid,sizeof(readblobid));
	if (rdcolcount==1) {
		readblobid=*((ISC_QUAD *)rdsqlda->sqlvar[0].sqldata);
	}
	assertTrue(readblobid.gds_quad_high!=0 || readblobid.gds_quad_low!=0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_open_blob2\n");
	blobhandle=0;
	assertEquals((int)isc_open_blob2(fbstatus,&db,&tr,&blobhandle,
						&readblobid,0,NULL),0);
	assertTrue(blobhandle!=0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_blob_info\n");
	char	blobinfoitems[]={
			isc_info_blob_total_length,
			isc_info_blob_num_segments
			};
	char	blobinfobuffer[256];
	bytestring::zero(blobinfobuffer,sizeof(blobinfobuffer));
	assertEquals((int)isc_blob_info(fbstatus,&blobhandle,
				(short)sizeof(blobinfoitems),blobinfoitems,
				(short)sizeof(blobinfobuffer),
				blobinfobuffer),0);
	const char	*blobinfoptr=blobinfobuffer;
	int		bloblength=0;
	int		blobsegments=0;
	int		blobinfocount=0;
	while (*blobinfoptr!=(char)isc_info_end && blobinfocount<8) {
		char	blobinfoitem=*blobinfoptr;
		blobinfoptr++;
		short	blobinfolen=(short)isc_vax_integer(blobinfoptr,2);
		blobinfoptr+=2;
		if (blobinfoitem==isc_info_blob_total_length) {
			bloblength=(int)isc_vax_integer(blobinfoptr,
							blobinfolen);
		} else if (blobinfoitem==isc_info_blob_num_segments) {
			blobsegments=(int)isc_vax_integer(blobinfoptr,
							blobinfolen);
		}
		blobinfoptr+=blobinfolen;
		blobinfocount++;
	}
	assertEquals(bloblength,blobsize);
	assertEquals(blobsegments,blobsegmentcount);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_get_segment\n");
	// a read buffer smaller than a segment, so isc_segment shows up
	char		segbuffer[1024];
	char		*blobread=new char[blobsize];
	bytestring::zero(blobread,blobsize);
	unsigned short	seglen=0;
	int		totalread=0;
	int		segreads=0;
	int		partialreads=0;
	while (segreads<64) {

		ISC_STATUS	getresult=isc_get_segment(fbstatus,&blobhandle,
					&seglen,
					(unsigned short)sizeof(segbuffer),
					segbuffer);

		// a non-zero return with isc_segment in the vector means the
		// buffer was too small for the whole segment and there is
		// more of it to come
		if (getresult && fbstatus[1]!=isc_segment) {
			break;
		}
		if (getresult) {
			partialreads++;
		}
		if (totalread+seglen<=blobsize) {
			bytestring::copy(blobread+totalread,segbuffer,seglen);
		}
		totalread+=seglen;
		segreads++;
	}
	assertEquals((int)fbstatus[1],(int)isc_segstr_eof);
	assertEquals(totalread,blobsize);
	assertTrue(segreads>1);
	assertTrue(partialreads>0);
	assertEquals((int)bytestring::compare(blobread,blobdata,blobsize),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_close_blob\n");
	assertEquals((int)isc_close_blob(fbstatus,&blobhandle),0);
	assertTrue(blobhandle==0);
	stdoutput.printf("\n\n");


	isc_dsql_free_statement(fbstatus,&rdstmt,DSQL_drop);
	freeOutputBuffers(rdcolcount,rdbuffer);
	delete[] (char *)rdsqlda;
	delete[] blobread;
	delete[] blobdata;

	// back to the 8 rows
	isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
				"delete from testtable where testinteger=9",
				SQL_DIALECT_V6,NULL);
	assertEquals(countRows("testtable"),8);
	isc_commit_transaction(fbstatus,&tr);



	stdoutput.printf("\n======== Blob handle collision (#9457) ========"
								"\n\n");

	// #9457: "invalid BLOB ID" (sqlcode -901, isc_bad_segstr_id) turned
	// out not to be about a NULL blob at all.  Firebird's client library
	// keeps every object - statements, transactions, blobs, requests -
	// in one flat table keyed purely by handle number.  The module's
	// blob handle allocator and its statement handle (assigned at
	// prepare time) both start counting near 1, so a blob created AFTER
	// a statement is prepared could land on the same client-visible
	// handle number as that statement, corrupting the table.  Creating
	// the blob BEFORE the prepare, like the test above does, happens to
	// dodge the collision - every test below creates it after, on
	// purpose, since that ordering is what the fix (newBlobHandle() and
	// nextTransactionHandle() in src/protocols/firebird.cpp) addresses.
	//
	// This needs its own table: testtable's blob column is used by the
	// test above, and a select-all test further up asserts an exact
	// column count against testtable, so it can't be widened with a
	// clob column here.
	reattach();

	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);

	isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
				"drop table testblobhandle",
				SQL_DIALECT_V6,NULL);
	isc_commit_transaction(fbstatus,&tr);
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	assertEquals((int)isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
		"create table testblobhandle ("
			"testinteger integer, "
			"testblob blob, "
			"testclob blob sub_type 1)",
				SQL_DIALECT_V6,NULL),0);
	isc_commit_transaction(fbstatus,&tr);
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);


	stdoutput.printf("isc_dsql_prepare - insert, before the blob and "
				"clob exist\n");
	XSQLDA	*bhinsqlda=(XSQLDA *)new char[XSQLDA_LENGTH(3)];
	bytestring::zero(bhinsqlda,XSQLDA_LENGTH(3));
	bhinsqlda->version=SQLDA_VERSION1;
	bhinsqlda->sqln=3;
	isc_stmt_handle	bhstmt=0;
	assertEquals((int)isc_dsql_allocate_statement(fbstatus,&db,
							&bhstmt),0);
	assertTrue(bhstmt!=0);
	assertEquals((int)isc_dsql_prepare(fbstatus,&tr,&bhstmt,0,
		"insert into testblobhandle "
		"(testinteger,testblob,testclob) values (?,?,?)",
		SQL_DIALECT_V6,NULL),0);
	assertEquals((int)isc_dsql_describe_bind(fbstatus,&bhstmt,
						SQL_DIALECT_V6,bhinsqlda),0);
	assertEquals(bhinsqlda->sqld,3);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_create_blob2 - blob, after the prepare\n");
	// short - the point here is the handle number, not the segment
	// path, which the "isc_get_segment" test above already covers
	const char	*bhblobtext="testblob after prepare";
	isc_blob_handle	bhblobhandle=0;
	ISC_QUAD	bhblobid;
	bytestring::zero(&bhblobid,sizeof(bhblobid));
	assertEquals((int)isc_create_blob2(fbstatus,&db,&tr,&bhblobhandle,
						&bhblobid,0,NULL),0);
	assertTrue(bhblobhandle!=0);
	assertEquals((int)isc_put_segment(fbstatus,&bhblobhandle,
			(unsigned short)charstring::getLength(bhblobtext),
			(char *)bhblobtext),0);
	assertEquals((int)isc_close_blob(fbstatus,&bhblobhandle),0);
	assertTrue(bhblobhandle==0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_create_blob2 - clob, after the prepare\n");
	const char	*bhclobtext="testclob after prepare";
	isc_blob_handle	bhclobhandle=0;
	ISC_QUAD	bhclobid;
	bytestring::zero(&bhclobid,sizeof(bhclobid));
	assertEquals((int)isc_create_blob2(fbstatus,&db,&tr,&bhclobhandle,
						&bhclobid,0,NULL),0);
	assertTrue(bhclobhandle!=0);
	assertEquals((int)isc_put_segment(fbstatus,&bhclobhandle,
			(unsigned short)charstring::getLength(bhclobtext),
			(char *)bhclobtext),0);
	assertEquals((int)isc_close_blob(fbstatus,&bhclobhandle),0);
	assertTrue(bhclobhandle==0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_dsql_execute - insert with a blob and clob "
				"created after the prepare\n");
	ISC_LONG	bhrowid=1;
	short		bhind[3];
	bhind[0]=0;
	bhind[1]=0;
	bhind[2]=0;
	bhinsqlda->sqlvar[0].sqldata=(char *)&bhrowid;
	bhinsqlda->sqlvar[0].sqlind=&bhind[0];
	bhinsqlda->sqlvar[0].sqltype|=1;
	bhinsqlda->sqlvar[1].sqldata=(char *)&bhblobid;
	bhinsqlda->sqlvar[1].sqlind=&bhind[1];
	bhinsqlda->sqlvar[1].sqltype|=1;
	bhinsqlda->sqlvar[2].sqldata=(char *)&bhclobid;
	bhinsqlda->sqlvar[2].sqlind=&bhind[2];
	bhinsqlda->sqlvar[2].sqltype|=1;
	assertEquals((int)isc_dsql_execute(fbstatus,&tr,&bhstmt,
						SQL_DIALECT_V6,bhinsqlda),0);
	// the defect this guards against surfaces here specifically: -901 /
	// isc_bad_segstr_id, from a corrupted client-side handle table
	// rather than a missing blob
	assertTrue(isc_sqlcode(fbstatus)!=-901);
	isc_dsql_free_statement(fbstatus,&bhstmt,DSQL_drop);
	delete[] (char *)bhinsqlda;
	assertEquals((int)isc_commit_transaction(fbstatus,&tr),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("select the row back - blob and clob round-trip\n");
	tr=0;
	assertEquals((int)isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb),0);
	XSQLDA	*bhrdsqlda=(XSQLDA *)new char[XSQLDA_LENGTH(2)];
	bytestring::zero(bhrdsqlda,XSQLDA_LENGTH(2));
	bhrdsqlda->version=SQLDA_VERSION1;
	bhrdsqlda->sqln=2;
	isc_stmt_handle	bhrdstmt=0;
	assertEquals((int)isc_dsql_allocate_statement(fbstatus,&db,
							&bhrdstmt),0);
	assertEquals((int)isc_dsql_prepare(fbstatus,&tr,&bhrdstmt,0,
		"select testblob,testclob from testblobhandle "
		"where testinteger=1",
		SQL_DIALECT_V6,bhrdsqlda),0);
	short	bhrdind[64];
	char	*bhrdbuffer[64];
	int	bhrdcolcount=bindOutputBuffers(bhrdsqlda,bhrdind,bhrdbuffer);
	assertEquals(bhrdcolcount,2);
	assertEquals((int)isc_dsql_execute(fbstatus,&tr,&bhrdstmt,
						SQL_DIALECT_V6,NULL),0);
	assertEquals((int)isc_dsql_fetch(fbstatus,&bhrdstmt,
						SQL_DIALECT_V6,bhrdsqlda),0);
	ISC_QUAD	bhreadblobid;
	ISC_QUAD	bhreadclobid;
	bytestring::zero(&bhreadblobid,sizeof(bhreadblobid));
	bytestring::zero(&bhreadclobid,sizeof(bhreadclobid));
	if (bhrdcolcount==2) {
		bhreadblobid=*((ISC_QUAD *)bhrdsqlda->sqlvar[0].sqldata);
		bhreadclobid=*((ISC_QUAD *)bhrdsqlda->sqlvar[1].sqldata);
	}
	assertTrue(bhreadblobid.gds_quad_high!=0 ||
					bhreadblobid.gds_quad_low!=0);
	assertTrue(bhreadclobid.gds_quad_high!=0 ||
					bhreadclobid.gds_quad_low!=0);
	stdoutput.printf("\n\n");


	stdoutput.printf("read back the blob content\n");
	isc_blob_handle	bhreadblobhandle=0;
	assertEquals((int)isc_open_blob2(fbstatus,&db,&tr,&bhreadblobhandle,
						&bhreadblobid,0,NULL),0);
	assertTrue(bhreadblobhandle!=0);
	char	bhreadblobbuffer[256];
	int	bhreadbloblen=readBlob(&bhreadblobhandle,bhreadblobbuffer,
						sizeof(bhreadblobbuffer));
	assertEquals((int)fbstatus[1],(int)isc_segstr_eof);
	assertEquals(bhreadbloblen,(int)charstring::getLength(bhblobtext));
	assertEquals(bhreadblobbuffer,bhblobtext);
	assertEquals((int)isc_close_blob(fbstatus,&bhreadblobhandle),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("read back the clob content\n");
	isc_blob_handle	bhreadclobhandle=0;
	assertEquals((int)isc_open_blob2(fbstatus,&db,&tr,&bhreadclobhandle,
						&bhreadclobid,0,NULL),0);
	assertTrue(bhreadclobhandle!=0);
	char	bhreadclobbuffer[256];
	int	bhreadcloblen=readBlob(&bhreadclobhandle,bhreadclobbuffer,
						sizeof(bhreadclobbuffer));
	assertEquals((int)fbstatus[1],(int)isc_segstr_eof);
	assertEquals(bhreadcloblen,(int)charstring::getLength(bhclobtext));
	assertEquals(bhreadclobbuffer,bhclobtext);
	assertEquals((int)isc_close_blob(fbstatus,&bhreadclobhandle),0);
	stdoutput.printf("\n\n");


	isc_dsql_free_statement(fbstatus,&bhrdstmt,DSQL_drop);
	freeOutputBuffers(bhrdcolcount,bhrdbuffer);
	delete[] (char *)bhrdsqlda;
	isc_commit_transaction(fbstatus,&tr);


	// defensive regression coverage for the symptom #9457 was originally
	// filed under - a NULL blob/clob select-back.  The literal repro no
	// longer reproduces (root-caused to the handle collision above
	// instead), but this guards against it resurfacing.
	stdoutput.printf("insert a row with a NULL blob and NULL clob\n");
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	assertEquals((int)isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
		"insert into testblobhandle "
		"(testinteger,testblob,testclob) values (2,NULL,NULL)",
				SQL_DIALECT_V6,NULL),0);
	assertEquals((int)isc_commit_transaction(fbstatus,&tr),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("select the NULL blob and NULL clob back\n");
	tr=0;
	assertEquals((int)isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb),0);
	XSQLDA	*bhnullsqlda=(XSQLDA *)new char[XSQLDA_LENGTH(2)];
	bytestring::zero(bhnullsqlda,XSQLDA_LENGTH(2));
	bhnullsqlda->version=SQLDA_VERSION1;
	bhnullsqlda->sqln=2;
	isc_stmt_handle	bhnullstmt=0;
	assertEquals((int)isc_dsql_allocate_statement(fbstatus,&db,
							&bhnullstmt),0);
	assertEquals((int)isc_dsql_prepare(fbstatus,&tr,&bhnullstmt,0,
		"select testblob,testclob from testblobhandle "
		"where testinteger=2",
		SQL_DIALECT_V6,bhnullsqlda),0);
	short	bhnullind[64];
	char	*bhnullbuffer[64];
	int	bhnullcolcount=bindOutputBuffers(bhnullsqlda,
						bhnullind,bhnullbuffer);
	assertEquals(bhnullcolcount,2);
	// the originally-reported symptom: selecting a NULL blob/clob output
	// column must not fail with -901
	assertEquals((int)isc_dsql_execute(fbstatus,&tr,&bhnullstmt,
						SQL_DIALECT_V6,NULL),0);
	assertTrue(isc_sqlcode(fbstatus)!=-901);
	assertEquals((int)isc_dsql_fetch(fbstatus,&bhnullstmt,
						SQL_DIALECT_V6,bhnullsqlda),0);
	if (bhnullcolcount==2) {
		assertEquals((int)bhnullind[0],-1);
		assertEquals((int)bhnullind[1],-1);
	}
	isc_dsql_free_statement(fbstatus,&bhnullstmt,DSQL_drop);
	freeOutputBuffers(bhnullcolcount,bhnullbuffer);
	delete[] (char *)bhnullsqlda;
	stdoutput.printf("\n\n");


	isc_commit_transaction(fbstatus,&tr);
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
				"drop table testblobhandle",
				SQL_DIALECT_V6,NULL);
	isc_commit_transaction(fbstatus,&tr);



	stdoutput.printf("\n=============== Errors ===============\n\n");

	reattach();

	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);

	// the sqlstates below are fb_sqlstate's mapping, confirmed against
	// firebird 2.5, 3.0 and 4.0 under #8954
	char	errmsg[512];
	char	sqlstate[6];


	stdoutput.printf("syntax error\n");
	assertTrue(isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
				"selec * from testtable",
				SQL_DIALECT_V6,NULL)!=0);
	firstErrorMessage(errmsg,sizeof(errmsg));
	assertEquals(errmsg,"Dynamic SQL Error");
	assertEquals((int)isc_sqlcode(fbstatus),-104);
	fb_sqlstate(sqlstate,fbstatus);
	assertEquals(sqlstate,"42000");
	stdoutput.printf("\n\n");


	stdoutput.printf("unknown table\n");
	reattach();
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	assertTrue(isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
				"select * from nosuchtable",
				SQL_DIALECT_V6,NULL)!=0);
	firstErrorMessage(errmsg,sizeof(errmsg));
	assertEquals(errmsg,"Dynamic SQL Error");
	assertEquals((int)isc_sqlcode(fbstatus),-204);
	fb_sqlstate(sqlstate,fbstatus);
	assertEquals(sqlstate,"42S02");
	stdoutput.printf("\n\n");


	stdoutput.printf("unknown column\n");
	reattach();
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	assertTrue(isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
				"select nosuchcolumn from testtable",
				SQL_DIALECT_V6,NULL)!=0);
	firstErrorMessage(errmsg,sizeof(errmsg));
	assertEquals(errmsg,"Dynamic SQL Error");
	assertEquals((int)isc_sqlcode(fbstatus),-206);
	fb_sqlstate(sqlstate,fbstatus);
	assertEquals(sqlstate,"42S22");
	stdoutput.printf("\n\n");


	stdoutput.printf("constraint violation\n");
	reattach();
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	// testtable has no constraints, so this needs its own table
	isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
				"drop table testconstraint",
				SQL_DIALECT_V6,NULL);
	isc_commit_transaction(fbstatus,&tr);
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
		"create table testconstraint "
		"(testinteger integer not null primary key)",
				SQL_DIALECT_V6,NULL);
	isc_commit_transaction(fbstatus,&tr);
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	assertEquals((int)isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
				"insert into testconstraint values (1)",
				SQL_DIALECT_V6,NULL),0);
	assertTrue(isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
				"insert into testconstraint values (1)",
				SQL_DIALECT_V6,NULL)!=0);
	firstErrorMessage(errmsg,sizeof(errmsg));
	// the message names the generated constraint, so only the fixed part
	// of it can be pinned
	assertTrue(charstring::contains(errmsg,
			"violation of PRIMARY or UNIQUE KEY constraint"));
	assertEquals((int)isc_sqlcode(fbstatus),-803);
	fb_sqlstate(sqlstate,fbstatus);
	assertEquals(sqlstate,"23000");
	isc_rollback_transaction(fbstatus,&tr);
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
				"drop table testconstraint",
				SQL_DIALECT_V6,NULL);
	isc_commit_transaction(fbstatus,&tr);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_dsql_sql_info - unprepared handle\n");
	reattach();
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	isc_stmt_handle	errstmt=0;
	assertEquals((int)isc_dsql_allocate_statement(fbstatus,&db,&errstmt),0);
	char	errinfoitems[]={
			isc_info_sql_stmt_type
			};
	char	errinfobuffer[64];
	bytestring::zero(errinfobuffer,sizeof(errinfobuffer));
	assertTrue(isc_dsql_sql_info(fbstatus,&errstmt,
				(short)sizeof(errinfoitems),errinfoitems,
				(short)sizeof(errinfobuffer),
				errinfobuffer)!=0);
	// the exact code depends on the server version, so pin only that it
	// is an error and that it carries a sqlstate
	assertTrue(isc_sqlcode(fbstatus)<0);
	fb_sqlstate(sqlstate,fbstatus);
	assertEquals((int)charstring::getLength(sqlstate),5);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_dsql_execute - freed handle\n");
	assertEquals((int)isc_dsql_free_statement(fbstatus,&errstmt,
							DSQL_drop),0);
	assertTrue(errstmt==0);
	assertTrue(isc_dsql_execute(fbstatus,&tr,&errstmt,
					SQL_DIALECT_V6,NULL)!=0);
	assertTrue(isc_sqlcode(fbstatus)<0);
	fb_sqlstate(sqlstate,fbstatus);
	assertEquals((int)charstring::getLength(sqlstate),5);
	isc_commit_transaction(fbstatus,&tr);
	stdoutput.printf("\n\n");


	stdoutput.printf("errors, not a dropped socket\n");
	// Against sqlrelay today every assert above fails the same way, and
	// not because firebird disagrees.  45 of the module's 50 handlers,
	// from detach() at src/protocols/firebird.cpp:1584 through
	// cancelEvents() at :2387, are a bare "return false;".  The session
	// loop at :611-763 reads that as "stop looping", so clientSession()
	// falls through to closeClientConnection() at :766 and the socket
	// goes away without an error response ever being written.  The
	// client then reports a lost connection - isc_net_read_err or
	// isc_lost_db_connection with sqlcode -902 - instead of the
	// -104/-204/-206/-803 above.  Finishing #7231 is what turns these
	// into real firebird errors.
	reattach();
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	assertTrue(isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
				"selec * from testtable",
				SQL_DIALECT_V6,NULL)!=0);
	// a stubbed op must not be what ends the conversation (#7231)
	assertTrue(fbstatus[1]!=isc_net_read_err);
	assertTrue(fbstatus[1]!=isc_net_write_err);
	assertTrue(fbstatus[1]!=isc_lost_db_connection);
	assertTrue(isc_sqlcode(fbstatus)!=-902);
	isc_rollback_transaction(fbstatus,&tr);
	stdoutput.printf("\n\n");



	stdoutput.printf("\n============== Cursors ===============\n\n");

	reattach();

	// this section rolls back at the end too, so testtable is left as
	// the 8 rows
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);


	stdoutput.printf("isc_dsql_set_cursor_name\n");
	XSQLDA	*cursqlda=(XSQLDA *)new char[XSQLDA_LENGTH(2)];
	bytestring::zero(cursqlda,XSQLDA_LENGTH(2));
	cursqlda->version=SQLDA_VERSION1;
	cursqlda->sqln=2;
	isc_stmt_handle	curstmt=0;
	assertEquals((int)isc_dsql_allocate_statement(fbstatus,&db,&curstmt),0);
	assertEquals((int)isc_dsql_prepare(fbstatus,&tr,&curstmt,0,
			"select testinteger,testvarchar from testtable "
			"where testinteger=8 for update",
			SQL_DIALECT_V6,cursqlda),0);
	assertEquals(cursqlda->sqld,2);
	assertEquals((int)isc_dsql_set_cursor_name(fbstatus,&curstmt,
						"testcursor",0),0);
	stdoutput.printf("\n\n");


	stdoutput.printf("isc_dsql_execute - cursor\n");
	short	curind[64];
	char	*curbuffer[64];
	int	curcolcount=bindOutputBuffers(cursqlda,curind,curbuffer);
	assertEquals(curcolcount,2);
	assertEquals((int)isc_dsql_execute(fbstatus,&tr,&curstmt,
						SQL_DIALECT_V6,NULL),0);
	assertEquals((int)isc_dsql_fetch(fbstatus,&curstmt,
						SQL_DIALECT_V6,cursqlda),0);
	// only read the sqlda if the prepare filled it in - a failed
	// prepare leaves every sqldata NULL
	if (curcolcount==2) {
		assertEquals((int)*((ISC_LONG *)cursqlda->sqlvar[0].sqldata),
									8);
	}
	stdoutput.printf("\n\n");


	// A positioned update needs the backend's cursor to carry the name the
	// client gave it, and op_set_cursor can't hand the name down - see
	// #9087.  So this works against a real server and fails against the
	// module, with the -504 a real server sends for a cursor that doesn't
	// exist.  The sqlrelay half of each branch below goes back to
	// asserting success when #9087 lands.
	stdoutput.printf("update where current of\n");
	const char	*posupdate="update testtable set testvarchar='updated8' "
					"where current of testcursor";
	if (issqlrelay) {
		assertTrue(isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
				posupdate,SQL_DIALECT_V6,NULL)!=0);
		firstErrorMessage(errmsg,sizeof(errmsg));
		assertEquals(errmsg,"Dynamic SQL Error");
		assertEquals((int)isc_sqlcode(fbstatus),-504);
		fb_sqlstate(sqlstate,fbstatus);
		assertEquals(sqlstate,"24000");
	} else {
		assertEquals((int)isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
				posupdate,SQL_DIALECT_V6,NULL),0);
	}
	XSQLDA	*vfsqlda=(XSQLDA *)new char[XSQLDA_LENGTH(1)];
	bytestring::zero(vfsqlda,XSQLDA_LENGTH(1));
	vfsqlda->version=SQLDA_VERSION1;
	vfsqlda->sqln=1;
	isc_stmt_handle	vfstmt=0;
	assertEquals((int)isc_dsql_allocate_statement(fbstatus,&db,&vfstmt),0);
	assertEquals((int)isc_dsql_prepare(fbstatus,&tr,&vfstmt,0,
		"select testvarchar from testtable where testinteger=8",
		SQL_DIALECT_V6,vfsqlda),0);
	short	vfind[64];
	char	*vfbuffer[64];
	int	vfcolcount=bindOutputBuffers(vfsqlda,vfind,vfbuffer);
	assertEquals(vfcolcount,1);
	assertEquals((int)isc_dsql_execute(fbstatus,&tr,&vfstmt,
						SQL_DIALECT_V6,NULL),0);
	assertEquals((int)isc_dsql_fetch(fbstatus,&vfstmt,
						SQL_DIALECT_V6,vfsqlda),0);
	// only read the sqlda if the prepare filled it in - a failed
	// prepare leaves every sqldata NULL
	if (vfcolcount==1) {
		PARAMVARY	*vfvary=
				(PARAMVARY *)vfsqlda->sqlvar[0].sqldata;
		char	vfvarchar[64];
		charstring::copy(vfvarchar,(char *)vfvary->vary_string,
							vfvary->vary_length);
		vfvarchar[vfvary->vary_length]='\0';
		// nothing half-applied - the row is untouched when the
		// update failed
		assertEquals(vfvarchar,(issqlrelay)?"testvarchar8":"updated8");
	}
	isc_dsql_free_statement(fbstatus,&vfstmt,DSQL_drop);
	freeOutputBuffers(vfcolcount,vfbuffer);
	delete[] (char *)vfsqlda;
	stdoutput.printf("\n\n");


	stdoutput.printf("delete where current of\n");
	const char	*posdelete="delete from testtable "
					"where current of testcursor";
	if (issqlrelay) {
		assertTrue(isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
				posdelete,SQL_DIALECT_V6,NULL)!=0);
		firstErrorMessage(errmsg,sizeof(errmsg));
		assertEquals(errmsg,"Dynamic SQL Error");
		assertEquals((int)isc_sqlcode(fbstatus),-504);
		fb_sqlstate(sqlstate,fbstatus);
		assertEquals(sqlstate,"24000");
	} else {
		assertEquals((int)isc_dsql_execute_immediate(fbstatus,&db,&tr,0,
				posdelete,SQL_DIALECT_V6,NULL),0);
	}
	// the row survives when the delete failed
	assertEquals(countRows("testtable"),(issqlrelay)?8:7);
	stdoutput.printf("\n\n");


	isc_dsql_free_statement(fbstatus,&curstmt,DSQL_drop);
	freeOutputBuffers(curcolcount,curbuffer);
	delete[] (char *)cursqlda;

	// back to the 8 rows
	isc_rollback_transaction(fbstatus,&tr);
	tr=0;
	isc_start_transaction(fbstatus,&tr,1,&db,
					(unsigned short)sizeof(tpb),tpb);
	assertEquals(countRows("testtable"),8);
	isc_commit_transaction(fbstatus,&tr);

	delete[] (char *)sqlda;


	reportTestStatus();
	return status;
}
