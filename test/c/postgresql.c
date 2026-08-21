// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclientwrapper.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
	#include <windows.h>
	#define sleep(x) Sleep((x)*1000)
	#define snprintf _snprintf
	static int win32gethostname(char *name, int len) {
		DWORD sz=(DWORD)len;
		DWORD i;
		if (!GetComputerNameA(name,&sz)) {
			return -1;
		}
		// GetComputerNameA returns the netbios name uppercased;
		// lowercase it to match gethostname on unix and the test db
		// name, which is the lowercase host name
		for (i=0; i<sz; i++) {
			if (name[i]>='A' && name[i]<='Z') {
				name[i]=name[i]+('a'-'A');
			}
		}
		return 0;
	}
	#define gethostname(name,len) win32gethostname((name),(len))
#else
	#include <strings.h>
	#include <unistd.h>
#endif
#include <stdint.h>

#include "asserts.c"

sqlrcon	con;
sqlrcur	cur;
sqlrcon	secondcon;
sqlrcur	secondcur;

int main(int argc, char **argv) {

	// keep stdout unbuffered so a crash is logged where it actually
	// happens, not at the last flushed line
	setvbuf(stdout,NULL,_IONBF,0);

	const char	*isolationlevels[]={
				"read committed","read uncommitted",
				"repeatable read","serializable",NULL};
	const char * const *cols;
	const char * const *fields;
	uint32_t	*fieldlens;
	const char	*subvars[]={"var1","var2","var3",NULL};
	int64_t		subvallongs[]={1,2,3};
	const char	*subvalstrings[]={"hi","hello","bye"};
	double		subvaldoubles[]={10.55,10.556,10.5556};
	uint32_t	precs[]={4,5,6};
	uint32_t	scales[]={2,3,4};
	uint16_t	port;
	const char	*socket;
	uint16_t	id;
	char		*filename;
	#define	LARGE_BUFFER_LENGTH	8192
	char		largebuffer[LARGE_BUFFER_LENGTH+1];
	uint64_t	i;
	unsigned char	buffer[256];
	char		querystr[1024];
	char		hex[3];


	// hostname
	char	hostname[256];
	char	*dot;
	gethostname(hostname,sizeof(hostname));
	dot=strchr(hostname,'.');
	if (dot) {
		*dot='\0';
	}


	// instantiation
	con=sqlrcon_alloc("sqlrelay",9003,"/tmp/postgresql.socket","testuser",
				"testpassword",0,1);
	cur=sqlrcur_alloc(con);


	// identify
	printf("IDENTIFY: \n");
	assertEqStr(sqlrcon_identify(con),"postgresql");
	printf("\n");


	// ping
	printf("PING: \n");
	assertTrue(sqlrcon_ping(con));
	printf("\n");


	// transaction state
	printf("TRANSACTION STATE: \n");
	assertEqStr(sqlrcon_getDefaultTransactionModel(con),"explicit");
	assertEqStr(sqlrcon_getTransactionModel(con),"explicit");
	assertFalse(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcon_getAutoCommit(con));
	printf("\n");


	// bind format
	printf("BIND FORMAT: \n");
	assertEqStr(sqlrcon_bindFormat(con),"$1");
	printf("\n");


	// nextval format
	printf("NEXTVAL FORMAT: \n");
	assertEqStr(sqlrcon_nextvalFormat(con),"nextval('%s')");
	printf("\n");


	// isolation levels
	printf("ISOLATION LEVELS: \n");
	{
		const char **il;
		for (il=isolationlevels; *il; il++) {
			// postgresql requires the
			// isolation level to be the first
			// query of the transaction
			sqlrcon_begin(con);
			assertTrue(sqlrcon_setIsolationLevel(con,*il));
			assertEqStr(sqlrcon_getIsolationLevel(con),*il);
			sqlrcon_commit(con);
			printf("\n");
		}
	}
	// reset to the default isolation level
	sqlrcon_begin(con);
	assertTrue(sqlrcon_setIsolationLevel(con,isolationlevels[0]));
	sqlrcon_commit(con);
	printf("\n");


	// create testtable
	printf("CREATE TESTTABLE: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testint int, "
		"	testfloat float, "
		"	testreal real, "
		"	testsmallint smallint, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testdate date, "
		"	testtime time, "
		"	testtimestamp timestamp, "
		"	testtext text, "
		"	testbytea bytea)"));
	printf("\n");


	// insert
	printf("INSERT: \n");
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	1.5, "
		"	1.5, "
		"	1, "
		"	'testchar1', "
		"	'testvarchar1', "
		"	'01/01/2001', "
		"	'01:00:00', "
		"	NULL, "
		"	'testtext1', "
		"	'testbytea1')"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	2, "
		"	2.5, "
		"	2.5, "
		"	2, "
		"	'testchar2', "
		"	'testvarchar2', "
		"	'01/01/2002', "
		"	'02:00:00', "
		"	NULL, "
		"	'testtext2', "
		"	'testbytea2')"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	3, "
		"	3.5, "
		"	3.5, "
		"	3, "
		"	'testchar3', "
		"	'testvarchar3', "
		"	'01/01/2003', "
		"	'03:00:00', "
		"	NULL, "
		"	'testtext3', "
		"	'testbytea3')"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	4, "
		"	4.5, "
		"	4.5, "
		"	4, "
		"	'testchar4', "
		"	'testvarchar4', "
		"	'01/01/2004', "
		"	'04:00:00', "
		"	NULL, "
		"	'testtext4', "
		"	'testbytea4')"));
	printf("\n");


	// affected rows
	printf("AFFECTED ROWS: \n");
	assertEqInt(sqlrcur_affectedRows(cur),1);
	printf("\n");


	// input bind by position
	printf("INPUT BIND BY POSITION: \n");
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	$1, "
		"	$2, "
		"	$3, "
		"	$4, "
		"	$5, "
		"	$6, "
		"	$7, "
		"	$8, "
		"	NULL, "
		"	$9, "
		"	$10)");
	assertEqInt(sqlrcur_countBindVariables(cur),10);
	sqlrcur_inputBindLong(cur,"1",5);
	sqlrcur_inputBindDouble(cur,"2",5.5,4,2);
	sqlrcur_inputBindDouble(cur,"3",5.5,4,2);
	sqlrcur_inputBindLong(cur,"4",5);
	sqlrcur_inputBindString(cur,"5","testchar5");
	sqlrcur_inputBindString(cur,"6","testvarchar5");
	sqlrcur_inputBindString(cur,"7","01/01/2005");
	sqlrcur_inputBindString(cur,"8","05:00:00");
	sqlrcur_inputBindClob(cur,"9","testtext5",9);
	sqlrcur_inputBindBlob(cur,"10","testbytea5",10);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",6);
	sqlrcur_inputBindDouble(cur,"2",6.5,4,2);
	sqlrcur_inputBindDouble(cur,"3",6.5,4,2);
	sqlrcur_inputBindLong(cur,"4",6);
	sqlrcur_inputBindString(cur,"5","testchar6");
	sqlrcur_inputBindString(cur,"6","testvarchar6");
	sqlrcur_inputBindString(cur,"7","01/01/2006");
	sqlrcur_inputBindString(cur,"8","06:00:00");
	sqlrcur_inputBindClob(cur,"9","testtext6",9);
	sqlrcur_inputBindBlob(cur,"10","testbytea6",10);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",7);
	sqlrcur_inputBindDouble(cur,"2",7.5,4,2);
	sqlrcur_inputBindDouble(cur,"3",7.5,4,2);
	sqlrcur_inputBindLong(cur,"4",7);
	sqlrcur_inputBindString(cur,"5","testchar7");
	sqlrcur_inputBindString(cur,"6","testvarchar7");
	sqlrcur_inputBindString(cur,"7","01/01/2007");
	sqlrcur_inputBindString(cur,"8","07:00:00");
	sqlrcur_inputBindClob(cur,"9","testtext7",9);
	sqlrcur_inputBindBlob(cur,"10","testbytea8",10);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// array of input binds by position
	// postgresql doesn't support implicit
	// conversion of string binds to other data
	// types, so arrays of binds don't generally
	// work.


	// input bind by name
	// postgresql doesn't support bind by name


	// input bind by position with validation
	printf("BIND BY POSITION ""WITH VALIDATION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",8);
	sqlrcur_inputBindDouble(cur,"2",8.5,4,2);
	sqlrcur_inputBindDouble(cur,"3",8.5,4,2);
	sqlrcur_inputBindLong(cur,"4",8);
	sqlrcur_inputBindString(cur,"5","testchar8");
	sqlrcur_inputBindString(cur,"6","testvarchar8");
	sqlrcur_inputBindString(cur,"7","01/01/2008");
	sqlrcur_inputBindString(cur,"8","08:00:00");
	sqlrcur_inputBindClob(cur,"9","testtext8",9);
	sqlrcur_inputBindClob(cur,"10","testbytea8",10);
	sqlrcur_validateBinds(cur);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// array of input binds by name
	// postgresql doesn't support bind by name


	// input bind by name with validation
	// postgresql doesn't support bind by name


	// select
	printf("SELECT: \n");
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	printf("\n");


	// column count
	printf("COLUMN COUNT: \n");
	assertEqInt(sqlrcur_colCount(cur),11);
	printf("\n");


	// column names
	printf("COLUMN NAMES: \n");
	assertEqStr(sqlrcur_getColumnName(cur,0),"testint");
	assertEqStr(sqlrcur_getColumnName(cur,1),"testfloat");
	assertEqStr(sqlrcur_getColumnName(cur,2),"testreal");
	assertEqStr(sqlrcur_getColumnName(cur,3),"testsmallint");
	assertEqStr(sqlrcur_getColumnName(cur,4),"testchar");
	assertEqStr(sqlrcur_getColumnName(cur,5),"testvarchar");
	assertEqStr(sqlrcur_getColumnName(cur,6),"testdate");
	assertEqStr(sqlrcur_getColumnName(cur,7),"testtime");
	assertEqStr(sqlrcur_getColumnName(cur,8),"testtimestamp");
	assertEqStr(sqlrcur_getColumnName(cur,9),"testtext");
	assertEqStr(sqlrcur_getColumnName(cur,10),"testbytea");
	cols=sqlrcur_getColumnNames(cur);
	assertEqStr(cols[0],"testint");
	assertEqStr(cols[1],"testfloat");
	assertEqStr(cols[2],"testreal");
	assertEqStr(cols[3],"testsmallint");
	assertEqStr(cols[4],"testchar");
	assertEqStr(cols[5],"testvarchar");
	assertEqStr(cols[6],"testdate");
	assertEqStr(cols[7],"testtime");
	assertEqStr(cols[8],"testtimestamp");
	assertEqStr(cols[9],"testtext");
	assertEqStr(cols[10],"testbytea");
	printf("\n");


	// column types
	printf("COLUMN TYPES: \n");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),"int4");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testint"),"int4");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,1),"float8");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testfloat"),"float8");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,2),"float4");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testreal"),"float4");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,3),"int2");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testsmallint"),"int2");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,4),"bpchar");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testchar"),"bpchar");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,5),"varchar");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testvarchar"),"varchar");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,6),"date");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testdate"),"date");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,7),"time");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testtime"),"time");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,8),"timestamp");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testtimestamp"),
		"timestamp");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,9),"text");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testtext"),"text");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,10),"bytea");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testbytea"),"bytea");
	printf("\n");


	// column length
	printf("COLUMN LENGTH: \n");
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),4);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testint"),4);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,1),8);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testfloat"),8);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,2),4);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testreal"),4);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,3),2);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testsmallint"),2);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,4),40);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testchar"),40);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,5),40);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testvarchar"),40);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,6),4);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testdate"),4);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,7),8);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testtime"),8);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,8),8);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testtimestamp"),8);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,9),0);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testtext"),0);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,10),0);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testbytea"),0);
	printf("\n");


	// longest column
	printf("LONGEST COLUMN: \n");
	assertEqInt(sqlrcur_getLongestByIndex(cur,0),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"testint"),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,1),3);
	assertEqInt(sqlrcur_getLongestByName(cur,"testfloat"),3);
	assertEqInt(sqlrcur_getLongestByIndex(cur,2),3);
	assertEqInt(sqlrcur_getLongestByName(cur,"testreal"),3);
	assertEqInt(sqlrcur_getLongestByIndex(cur,3),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"testsmallint"),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,4),40);
	assertEqInt(sqlrcur_getLongestByName(cur,"testchar"),40);
	assertEqInt(sqlrcur_getLongestByIndex(cur,5),12);
	assertEqInt(sqlrcur_getLongestByName(cur,"testvarchar"),12);
	assertEqInt(sqlrcur_getLongestByIndex(cur,6),10);
	assertEqInt(sqlrcur_getLongestByName(cur,"testdate"),10);
	assertEqInt(sqlrcur_getLongestByIndex(cur,7),8);
	assertEqInt(sqlrcur_getLongestByName(cur,"testtime"),8);
	assertEqInt(sqlrcur_getLongestByIndex(cur,9),9);
	assertEqInt(sqlrcur_getLongestByName(cur,"testtext"),9);
	assertEqInt(sqlrcur_getLongestByIndex(cur,10),10);
	assertEqInt(sqlrcur_getLongestByName(cur,"testbytea"),10);
	printf("\n");


	// row count
	printf("ROW COUNT: \n");
	assertEqInt(sqlrcur_rowCount(cur),8);
	printf("\n");


	// total rows
	printf("TOTAL ROWS: \n");
	assertEqInt(sqlrcur_totalRows(cur),8);
	printf("\n");


	// first row index
	printf("FIRST ROW INDEX: \n");
	assertEqInt(sqlrcur_firstRowIndex(cur),0);
	printf("\n");


	// end of result set
	printf("END OF RESULT SET: \n");
	assertTrue(sqlrcur_endOfResultSet(cur));
	printf("\n");


	// fields by index
	printf("FIELDS BY INDEX: \n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"1.5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"1.5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,3),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,4),"testchar1"
		"                               ");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,5),"testvarchar1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,6),"2001-01-01");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,7),"01:00:00");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,9),"testtext1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,10),"testbytea1");
	printf("\n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8.5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,2),"8.5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,3),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,4),"testchar8"
		"                               ");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,5),"testvarchar8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,6),"2008-01-01");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,7),"08:00:00");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,9),"testtext8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,10),"testbytea8");
	printf("\n");


	// field lengths by index
	printf("FIELD LENGTHS BY INDEX: \n");
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,1),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,2),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,3),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,4),40);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,5),12);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,6),10);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,7),8);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,9),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,10),10);
	printf("\n");
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,0),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,1),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,2),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,3),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,4),40);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,5),12);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,6),10);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,7),8);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,9),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,10),10);
	printf("\n");


	// fields by name
	printf("FIELDS BY NAME: \n");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testint"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testfloat"),"1.5");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testreal"),"1.5");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testsmallint"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testchar"),"testchar1"
		"                               ");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testvarchar"),"testvarchar1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testdate"),"2001-01-01");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testtime"),"01:00:00");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testtext"),"testtext1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testbytea"),"testbytea1");
	printf("\n");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testint"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testfloat"),"8.5");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testreal"),"8.5");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testsmallint"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testchar"),"testchar8"
		"                               ");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testvarchar"),"testvarchar8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testdate"),"2008-01-01");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testtime"),"08:00:00");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testtext"),"testtext8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testbytea"),"testbytea8");
	printf("\n");


	// field lengths by name
	printf("FIELD LENGTHS BY NAME: \n");
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testreal"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testsmallint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testchar"),40);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testvarchar"),12);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testdate"),10);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testtime"),8);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testtext"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testbytea"),10);
	printf("\n");
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testreal"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testsmallint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testchar"),40);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testvarchar"),12);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testdate"),10);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testtime"),8);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testtext"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testbytea"),10);
	printf("\n");


	// fields by array
	printf("FIELDS BY ARRAY: \n");
	fields=sqlrcur_getRow(cur,0);
	assertEqStr(fields[0],"1");
	assertEqStr(fields[1],"1.5");
	assertEqStr(fields[2],"1.5");
	assertEqStr(fields[3],"1");
	assertEqStr(fields[4],"testchar1""                               ");
	assertEqStr(fields[5],"testvarchar1");
	assertEqStr(fields[6],"2001-01-01");
	assertEqStr(fields[7],"01:00:00");
	assertEqStr(fields[9],"testtext1");
	assertEqStr(fields[10],"testbytea1");
	printf("\n");


	// field lengths by array
	printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=sqlrcur_getRowLengths(cur,0);
	assertEqInt(fieldlens[0],1);
	assertEqInt(fieldlens[1],3);
	assertEqInt(fieldlens[2],3);
	assertEqInt(fieldlens[3],1);
	assertEqInt(fieldlens[4],40);
	assertEqInt(fieldlens[5],12);
	assertEqInt(fieldlens[6],10);
	assertEqInt(fieldlens[7],8);
	assertEqInt(fieldlens[9],9);
	assertEqInt(fieldlens[10],10);
	printf("\n");


	// result set buffer size
	printf("RESULT SET BUFFER SIZE: \n");
	assertEqInt(sqlrcur_getResultSetBufferSize(cur),0);
	sqlrcur_setResultSetBufferSize(cur,2);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	assertEqInt(sqlrcur_getResultSetBufferSize(cur),2);
	printf("\n");
	assertEqInt(sqlrcur_firstRowIndex(cur),0);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),2);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,1,0),"2");
	assertEqStr(sqlrcur_getFieldByIndex(cur,2,0),"3");
	printf("\n");
	assertEqInt(sqlrcur_firstRowIndex(cur),2);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),4);
	assertEqStr(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	printf("\n");
	assertEqInt(sqlrcur_firstRowIndex(cur),6);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),8);
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	printf("\n");
	assertEqInt(sqlrcur_firstRowIndex(cur),8);
	assertTrue(sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),8);
	sqlrcur_setResultSetBufferSize(cur,0);
	printf("\n");


	// dont get column info
	printf("DONT GET COLUMN INFO: \n");
	sqlrcur_dontGetColumnInfo(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	assertEqStr(sqlrcur_getColumnName(cur,0),NULL);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),0);
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),NULL);
	sqlrcur_getColumnInfo(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	assertEqStr(sqlrcur_getColumnName(cur,0),"testint");
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),4);
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),"int4");
	printf("\n");


	// suspended session
	printf("SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	free((char *)socket);
	printf("\n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,1,0),"2");
	assertEqStr(sqlrcur_getFieldByIndex(cur,2,0),"3");
	assertEqStr(sqlrcur_getFieldByIndex(cur,3,0),"4");
	assertEqStr(sqlrcur_getFieldByIndex(cur,4,0),"5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,5,0),"6");
	assertEqStr(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	printf("\n");
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	free((char *)socket);
	printf("\n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,1,0),"2");
	assertEqStr(sqlrcur_getFieldByIndex(cur,2,0),"3");
	assertEqStr(sqlrcur_getFieldByIndex(cur,3,0),"4");
	assertEqStr(sqlrcur_getFieldByIndex(cur,4,0),"5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,5,0),"6");
	assertEqStr(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	printf("\n");
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	free((char *)socket);
	printf("\n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,1,0),"2");
	assertEqStr(sqlrcur_getFieldByIndex(cur,2,0),"3");
	assertEqStr(sqlrcur_getFieldByIndex(cur,3,0),"4");
	assertEqStr(sqlrcur_getFieldByIndex(cur,4,0),"5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,5,0),"6");
	assertEqStr(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	printf("\n");


	// suspended result set
	printf("SUSPENDED RESULT SET: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,2,0),"3");
	id=sqlrcur_getResultSetId(cur);
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	free((char *)socket);
	assertTrue(sqlrcur_resumeResultSet(cur,id));
	printf("\n");
	assertEqInt(sqlrcur_firstRowIndex(cur),4);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),6);
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	printf("\n");
	assertEqInt(sqlrcur_firstRowIndex(cur),6);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),8);
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	printf("\n");
	assertEqInt(sqlrcur_firstRowIndex(cur),8);
	assertTrue(sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),8);
	sqlrcur_setResultSetBufferSize(cur,0);
	printf("\n");


	// cached result set
	printf("CACHED RESULT SET: \n");
	sqlrcur_cacheToFile(cur,"cachefile1-postgresql");
	sqlrcur_setCacheTtl(cur,200);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqStr(filename,"cachefile1-postgresql");
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	free(filename);
	printf("\n");


	// column count for cached result set
	printf("COLUMN COUNT FOR ""CACHED RESULT SET: \n");
	assertEqInt(sqlrcur_colCount(cur),11);
	printf("\n");


	// column names for cached result set
	printf("COLUMN NAMES FOR ""CACHED RESULT SET: \n");
	assertEqStr(sqlrcur_getColumnName(cur,0),"testint");
	assertEqStr(sqlrcur_getColumnName(cur,1),"testfloat");
	assertEqStr(sqlrcur_getColumnName(cur,2),"testreal");
	assertEqStr(sqlrcur_getColumnName(cur,3),"testsmallint");
	assertEqStr(sqlrcur_getColumnName(cur,4),"testchar");
	assertEqStr(sqlrcur_getColumnName(cur,5),"testvarchar");
	assertEqStr(sqlrcur_getColumnName(cur,6),"testdate");
	assertEqStr(sqlrcur_getColumnName(cur,7),"testtime");
	assertEqStr(sqlrcur_getColumnName(cur,8),"testtimestamp");
	cols=sqlrcur_getColumnNames(cur);
	assertEqStr(cols[0],"testint");
	assertEqStr(cols[1],"testfloat");
	assertEqStr(cols[2],"testreal");
	assertEqStr(cols[3],"testsmallint");
	assertEqStr(cols[4],"testchar");
	assertEqStr(cols[5],"testvarchar");
	assertEqStr(cols[6],"testdate");
	assertEqStr(cols[7],"testtime");
	assertEqStr(cols[8],"testtimestamp");
	printf("\n");


	// cached result set with result set
	// buffer size
	printf("CACHED RESULT SET WITH ""RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile1-postgresql");
	sqlrcur_setCacheTtl(cur,200);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqStr(filename,"cachefile1-postgresql");
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize(cur,0);
	free(filename);
	printf("\n");


	// from one cache file to another
	printf("FROM ONE CACHE FILE ""TO ANOTHER: \n");
	sqlrcur_cacheToFile(cur,"cachefile2-postgresql");
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile1-postgresql"));
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile2-postgresql"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	printf("\n");


	// from one cache file to another
	// with result set buffer size
	printf("FROM ONE CACHE FILE TO ANOTHER "
		"WITH RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile2-postgresql");
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile1-postgresql"));
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile2-postgresql"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize(cur,0);
	printf("\n");


	// cached result set with suspend
	// and result set buffer size
	printf("CACHED RESULT SET WITH SUSPEND "
		"AND RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile1-postgresql");
	sqlrcur_setCacheTtl(cur,200);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,2,0),"3");
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqStr(filename,"cachefile1-postgresql");
	id=sqlrcur_getResultSetId(cur);
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	printf("\n");
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	free((char *)socket);
	assertTrue(sqlrcur_resumeCachedResultSet(cur,id,filename));
	printf("\n");
	assertEqInt(sqlrcur_firstRowIndex(cur),4);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),6);
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	printf("\n");
	assertEqInt(sqlrcur_firstRowIndex(cur),6);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),8);
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	printf("\n");
	assertEqInt(sqlrcur_firstRowIndex(cur),8);
	assertTrue(sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),8);
	sqlrcur_cacheOff(cur);
	printf("\n");
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize(cur,0);
	free(filename);
	printf("\n");


	// finished suspended session
	printf("FINISHED SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,4,0),"5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,5,0),"6");
	assertEqStr(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	id=sqlrcur_getResultSetId(cur);
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	free((char *)socket);
	assertTrue(sqlrcur_resumeResultSet(cur,id));
	assertEqStr(sqlrcur_getFieldByIndex(cur,4,0),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,5,0),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,6,0),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),NULL);
	printf("\n");


	// nested selects
	printf("NESTED SELECTS: \n");
	sqlrcur_setResultSetBufferSize(cur,1);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable"));
	secondcur=sqlrcur_alloc(con);
	sqlrcur_setResultSetBufferSize(secondcur,1);
	for (i=0; sqlrcur_getRow(cur,i); i++) {
		assertTrue(sqlrcur_sendQuery(secondcur,
			"select * from testtable"));
	}
	sqlrcur_free(secondcur);
	sqlrcur_setResultSetBufferSize(cur,0);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// reset transaction state
	printf("RESET TRANSACTION STATE: \n");
	assertTrue(sqlrcon_commit(con));
	assertEqStr(sqlrcon_getTransactionModel(con),"explicit");
	assertTrue(sqlrcon_getAutoCommit(con));
	printf("\n");


	// transaction behavior - implicit
	printf("TRANSACTION BEHAVIOR - implicit: \n");
	assertTrue(sqlrcon_setTransactionModel(con,"implicit"));
	assertEqStr(sqlrcon_getTransactionModel(con),"implicit");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable (col1 integer)"));
	// postgresql DDL is transactional; commit so the table is visible
	// to the second connection (the commit implicitly starts a new tx)
	assertTrue(sqlrcon_commit(con));
	secondcon=sqlrcon_alloc("sqlrelay",9003,"/tmp/postgresql.socket",
						"testuser","testpassword",0,1);
	secondcur=sqlrcur_alloc(secondcon);
	// session is in a transaction; insert is not visible until commit
	assertTrue(sqlrcon_getInTransaction(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (1)"));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"0");
	// commit makes it visible, and implicitly starts a new transaction
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
	// rollback discards, and implicitly starts a new transaction
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (2)"));
	assertTrue(sqlrcon_rollback(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
	// autoCommitOn takes effect immediately
	assertTrue(sqlrcon_autoCommitOn(con));
	assertTrue(sqlrcon_getAutoCommit(con));
	assertFalse(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (3)"));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"2");
	// autoCommitOff takes effect immediately
	assertTrue(sqlrcon_autoCommitOff(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcon_getInTransaction(con));
	sqlrcur_closeResultSet(secondcur);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// transaction behavior - explicit
	printf("TRANSACTION BEHAVIOR - explicit: \n");
	assertTrue(sqlrcon_setTransactionModel(con,"explicit"));
	assertEqStr(sqlrcon_getTransactionModel(con),"explicit");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable (col1 integer)"));
	// begin starts a new transaction; insert is not visible until commit
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (1)"));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"0");
	// commit makes it visible; no new transaction is started
	assertTrue(sqlrcon_commit(con));
	assertFalse(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
	// begin, insert, rollback discards; no new transaction is started
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (2)"));
	assertTrue(sqlrcon_rollback(con));
	assertFalse(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
	// autoCommitOn takes effect immediately
	assertTrue(sqlrcon_autoCommitOn(con));
	assertTrue(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (3)"));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"2");
	// autoCommitOff takes effect immediately
	assertTrue(sqlrcon_autoCommitOff(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	sqlrcur_closeResultSet(secondcur);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// transaction behavior - explicit-deferred
	printf("TRANSACTION BEHAVIOR - explicit-deferred: \n");
	assertTrue(sqlrcon_setTransactionModel(con,"explicit-deferred"));
	assertEqStr(sqlrcon_getTransactionModel(con),"explicit-deferred");
	// switch to autocommit-on so the begin/commit cycles below
	// bracket explicit transactions (autocommit-off semantics are
	// exercised at the end of this block)
	assertTrue(sqlrcon_autoCommitOn(con));
	assertTrue(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable (col1 integer)"));
	// begin starts a transaction; commit makes it visible
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (1)"));
	assertTrue(sqlrcon_commit(con));
	assertFalse(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
	// begin, insert, rollback discards
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (2)"));
	assertTrue(sqlrcon_rollback(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
	// during a transaction started by begin(), autoCommitOn is a
	// no-op: the autocommit setting takes effect after the user
	// explicitly commits/rollbacks the tx (mysql-native semantic)
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (3)"));
	assertTrue(sqlrcon_autoCommitOn(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
	// explicit commit ends the tx; autocommit-on now takes effect
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcon_getAutoCommit(con));
	assertFalse(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"2");
	// autocommit is on; subsequent inserts are visible immediately
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (4)"));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"3");
	// autoCommitOff takes effect immediately when not in a transaction
	assertTrue(sqlrcon_autoCommitOff(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	// autocommit-off persists across commit/rollback; each commit or
	// rollback ends the current implicit tx and a new one starts for
	// the next statement
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (5)"));
	assertTrue(sqlrcon_commit(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"4");
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (6)"));
	assertTrue(sqlrcon_rollback(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"4");
	// autoCommitOff during a transaction changes the variable
	// immediately but the in-flight tx continues; only after the
	// next explicit commit/rollback does the new autocommit-off
	// setting drop us into a new implicit tx (mysql-asymmetric
	// semantic)
	assertTrue(sqlrcon_autoCommitOn(con));
	assertTrue(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (7)"));
	assertTrue(sqlrcon_autoCommitOff(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"4");
	assertTrue(sqlrcon_commit(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"5");
	sqlrcur_closeResultSet(secondcur);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// transaction behavior - explicit-error
	printf("TRANSACTION BEHAVIOR - explicit-error: \n");
	assertTrue(sqlrcon_setTransactionModel(con,"explicit-error"));
	assertEqStr(sqlrcon_getTransactionModel(con),"explicit-error");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable (col1 integer)"));
	// begin, insert, commit
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (1)"));
	assertTrue(sqlrcon_commit(con));
	assertFalse(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
	// begin, insert, rollback
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (2)"));
	assertTrue(sqlrcon_rollback(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
	// while in a transaction, autoCommitOn/Off throw an error
	assertTrue(sqlrcon_begin(con));
	assertFalse(sqlrcon_autoCommitOn(con));
	assertFalse(sqlrcon_autoCommitOff(con));
	assertTrue(sqlrcon_commit(con));
	// outside of a transaction, autoCommitOn takes effect immediately
	assertTrue(sqlrcon_autoCommitOn(con));
	assertTrue(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (3)"));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"2");
	// autoCommitOff takes effect immediately
	assertTrue(sqlrcon_autoCommitOff(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	sqlrcur_closeResultSet(secondcur);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// transaction behavior - none
	printf("TRANSACTION BEHAVIOR - none: \n");
	assertTrue(sqlrcon_setTransactionModel(con,"none"));
	assertEqStr(sqlrcon_getTransactionModel(con),"none");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable (col1 integer)"));
	// no transactions; everything is visible immediately
	assertTrue(sqlrcon_getAutoCommit(con));
	assertFalse(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (1)"));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
	// commit and rollback are no-ops
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (2)"));
	assertTrue(sqlrcon_rollback(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"2");
	// autocommit is always on; autoCommitOff is an error
	assertFalse(sqlrcon_autoCommitOff(con));
	assertTrue(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcon_autoCommitOn(con));
	assertTrue(sqlrcon_getAutoCommit(con));
	sqlrcur_closeResultSet(secondcur);
	sqlrcur_free(secondcur);
	sqlrcon_free(secondcon);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// reset transaction behavior
	printf("RESET TRANSACTION BEHAVIOR: \n");
	assertTrue(sqlrcon_setTransactionModel(con,sqlrcon_getDefaultTransactionModel(con)));
	assertEqStr(sqlrcon_getTransactionModel(con),"explicit");
	assertTrue(sqlrcon_getAutoCommit(con));
	printf("\n");


	// individual substitutions
	printf("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"select $(var1),'$(var2)',$(var3)");
	sqlrcur_subLong(cur,"var1",1);
	sqlrcur_subString(cur,"var2","hello");
	sqlrcur_subDouble(cur,"var3",10.5556,6,4);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"10.5556");
	printf("\n");


	// array substitutions
	printf("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"select $(var1),$(var2),$(var3)");
	sqlrcur_subLongs(cur,subvars,subvallongs);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"2");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"3");
	printf("\n");
	sqlrcur_prepareQuery(cur,"select '$(var1)',""'$(var2)','$(var3)'");
	sqlrcur_subStrings(cur,subvars,subvalstrings);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"hi");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"bye");
	printf("\n");
	sqlrcur_prepareQuery(cur,"select $(var1),$(var2),$(var3)");
	sqlrcur_subDoubles(cur,subvars,subvaldoubles,precs,scales);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"10.55");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"10.556");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"10.5556");
	printf("\n");


	// nulls as nulls
	printf("NULLS AS NULLS: \n");
	sqlrcur_getNullsAsNulls(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select NULL,1,NULL"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select NULL,1,NULL"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"");
	printf("\n");


	// null and empty lobs
	printf("NULL AND EMPTY LOBS: \n");
	sqlrcur_getNullsAsNulls(cur);
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testclob1 text, "
		"	testclob2 text, "
		"	testblob1 bytea, "
		"	testblob2 bytea)"));
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	$1, "
		"	$2, "
		"	$3, "
		"	$4)");
	sqlrcur_inputBindClob(cur,"1","",0);
	sqlrcur_inputBindClob(cur,"2",NULL,0);
	sqlrcur_inputBindBlob(cur,"3","",0);
	sqlrcur_inputBindBlob(cur,"4",NULL,0);
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,3),NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// long lobs
	printf("LONG LOBS: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testtext text, "
		"	testbytea bytea)"));
	sqlrcur_prepareQuery(cur,"insert into testtable ""values ($1,$2)");
	for (i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	sqlrcur_inputBindClob(cur,"1",largebuffer,LARGE_BUFFER_LENGTH);
	sqlrcur_inputBindBlob(cur,"2",largebuffer,LARGE_BUFFER_LENGTH);
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable"));
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,
					"testtext"),LARGE_BUFFER_LENGTH);
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testtext"),largebuffer);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,
					"testbytea"),LARGE_BUFFER_LENGTH);
	assertEqStrLen(sqlrcur_getFieldByName(cur,0,"testbytea"),largebuffer,
		LARGE_BUFFER_LENGTH);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// output bind by position
	// postgresql doesn't support output binds


	// output bind by name
	// postgresql doesn't support output binds


	// output bind by name with validation
	// postgresql doesn't support output binds


	// lob output bind
	// postgresql doesn't support output binds


	// long output bind
	// postgresql doesn't support output binds


	// negative input bind
	printf("NEGATIVE INPUT BIND: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable ""(testval int)"));
	sqlrcur_prepareQuery(cur,"insert into testtable values ($1)");
	sqlrcur_inputBindLong(cur,"1",-1);
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"select testval from testtable"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testval"),"-1");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// bind validation
	// postgresql doesn't support bind by name


	// rebinding
	printf("REBINDING: \n");
	sqlrcur_sendQuery(cur,"drop function testfunc(int)");
	assertTrue(sqlrcur_sendQuery(cur,"create function testfunc(int) "
		"returns int as "
		"	' begin return $1; end;' ""language plpgsql"));
	sqlrcur_prepareQuery(cur,"select * from testfunc($1)");
	sqlrcur_inputBindLong(cur,"1",1);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	sqlrcur_inputBindLong(cur,"1",2);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"2");
	sqlrcur_inputBindLong(cur,"1",3);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"3");
	assertTrue(sqlrcur_sendQuery(cur,"drop function testfunc(int)"));
	printf("\n");


	// reexecute
	printf("REEXECUTE: \n");
	sqlrcur_prepareQuery(cur,"select 1");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	printf("\n");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	printf("\n");
	sqlrcur_prepareQuery(cur,"select $1::int");
	sqlrcur_inputBindLong(cur,"1",1);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	printf("\n");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	printf("\n");
	sqlrcur_inputBindLong(cur,"1",2);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"2");
	printf("\n");


	// stored procedure returning no value
	printf("STORED PROCEDURE ""RETURNING NO VALUE: \n");
	sqlrcur_sendQuery(cur,"drop function ""testfunc(int,float,char(20))");
	assertTrue(sqlrcur_sendQuery(cur,
		"create function testfunc("
		"	int,float,char(20)) "
		"returns void as ' "
		"	declare in1 int; "
		"	in2 float; "
		"	in3 char(20); "
		"begin "
		"	in1:=$1; "
		"	in2:=$2; "
		"	in3:=$3; "
		"	return; ""end;' language plpgsql"));
	sqlrcur_prepareQuery(cur,"select testfunc($1,$2,$3)");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",1.5,4,2);
	sqlrcur_inputBindString(cur,"3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"drop function "
		"testfunc(int,float,char(20))"));
	printf("\n");


	// stored procedure returning single value
	printf("STORED PROCEDURE ""RETURNING SINGLE VALUE: \n");
	sqlrcur_sendQuery(cur,"drop function ""testfunc(int,float,char(20))");
	assertTrue(sqlrcur_sendQuery(cur,"create function "
		"testfunc(int,float,char(20)) "
		"returns int as "
		"	' begin return $1; end;' ""language plpgsql"));
	sqlrcur_prepareQuery(cur,"select * from ""testfunc($1,$2,$3)");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",1.5,4,2);
	sqlrcur_inputBindString(cur,"3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertTrue(sqlrcur_sendQuery(cur,"drop function "
		"testfunc(int,float,char(20))"));
	printf("\n");


	// stored procedure returning
	// multiple values
	printf("STORED PROCEDURE ""RETURNING MULTIPLE VALUES: \n");
	sqlrcur_sendQuery(cur,"drop function ""testfunc(int,float,char(20))");
	assertTrue(sqlrcur_sendQuery(cur,
		"create function testfunc("
		"	int,float,char(20)) "
		"returns record as ' "
		"	declare output record; "
		"begin "
		"	select $1,$2,$3 "
		"	into output; "
		"	return output; ""end;' language plpgsql"));
	sqlrcur_prepareQuery(cur,
		"select "
		"	* "
		"from "
		"	testfunc($1,$2,$3) "
		"	as (col1 int, "
		"		col2 float, "
		"		col3 bpchar) ");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",1.5,4,2);
	sqlrcur_inputBindString(cur,"3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"hello");
	assertTrue(sqlrcur_sendQuery(cur,"drop function "
		"testfunc(int,float,char(20))"));
	printf("\n");


	// stored procedure returning result set
	printf("STORED PROCEDURE ""RETURNING RESULT SET: \n");
	sqlrcur_sendQuery(cur,"drop function testfunc()");
	assertTrue(sqlrcur_sendQuery(cur,"create function testfunc() "
		"returns setof record as ' "
		"	declare output record; "
		"begin "
		"	for output in "
		"		select 1 "
		"		union "
		"		select 2 "
		"		union "
		"		select 3 "
		"		union "
		"		select 4 "
		"		union "
		"		select 5 "
		"		union "
		"		select 6 "
		"		union "
		"		select 7 "
		"		union "
		"		select 8 "
		"	loop "
		"		return next output; "
		"	end loop; "
		"	return; ""end;' language plpgsql"));
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testfunc() "
		"	as (testint int)"));
	assertEqInt(sqlrcur_rowCount(cur),8);
	assertTrue(sqlrcur_sendQuery(cur,"drop function testfunc()"));
	printf("\n");


	// temporary tables
	printf("TEMPORARY TABLES: \n");
	sqlrcur_sendQuery(cur,"drop table temptable\n");
	assertTrue(sqlrcur_sendQuery(cur,"create temporary table temptable (col1 int)"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into temptable values (1)"));
	assertTrue(sqlrcur_sendQuery(cur,"select count(*) from temptable"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	sqlrcon_endSession(con);
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,"select count(*) from temptable"));
	printf("\n");


	// encoded binary data
	printf("ENCODED BINARY DATA: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable "
		"(col1 bytea)"));
	for (i=0; i<256; i++) {
		buffer[i]=(unsigned char)i;
	}
	strcpy(querystr,"insert into testtable ""values (decode('");
	for (i=0; i<sizeof(buffer); i++) {
		snprintf(hex,sizeof(hex),"%02x",buffer[i]);
		strcat(querystr,hex);
	}
	strcat(querystr,"','hex'))");
	assertTrue(sqlrcur_sendQuery(cur,querystr));
	assertTrue(sqlrcur_sendQuery(cur,"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),sizeof(buffer));
	assertEqBin(sqlrcur_getFieldByIndex(cur,0,0),
		buffer,sizeof(buffer));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// quotes
	printf("QUOTES: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable "
		"(col1 varchar(4))"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable "
		"values ('''''')"));
	assertTrue(sqlrcur_sendQuery(cur,"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),2);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"''");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// last insert id
	printf("LAST INSERT ID: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable "
		"	(col1 serial primary key, "
		"	col2 int)"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable "
		"(col2) values (1)"));
	assertEqInt(sqlrcon_getLastInsertId(con),1);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// database is schema
	printf("DATABASE IS SCHEMA: \n");
	assertFalse(sqlrcon_getDatabaseIsSchema(con));
	printf("\n");


	// catalog list
	printf("CATALOG LIST: \n");
	assertTrue(sqlrcur_getCatalogList(cur,NULL));
	assertEqStr(sqlrcur_getColumnName(cur,0),"Database");
	assertInResultSet(cur,"Database",hostname);
	printf("\n");


	// schema list
	printf("SCHEMA LIST: \n");
	assertTrue(sqlrcur_getSchemaList(cur,NULL));
	assertEqStr(sqlrcur_getColumnName(cur,0),"Database");
	assertInResultSet(cur,"Database","public");
	printf("\n");


	// table type list
	printf("TABLE TYPE LIST: \n");
	assertTrue(sqlrcur_getTableTypeList(cur));
	assertEqStr(sqlrcur_getColumnName(cur,0),"table_type");
	assertInResultSet(cur,"table_type","TABLE");
	printf("\n");


	// table list
	printf("TABLE LIST: \n");
	sqlrcur_sendQuery(cur,"drop table testtable1");
	sqlrcur_sendQuery(cur,"drop table testtable2");
	sqlrcur_sendQuery(cur,"drop table testtable3");
	sqlrcur_sendQuery(cur,"drop table testtable4");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable1 ("
		"	col1 int, "
		"	col2 int)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable2 ("
		"	col1 int, "
		"	col2 int)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable3 ("
		"	col1 int, "
		"	col2 int)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable4 ("
		"	col1 int, "
		"	col2 int)"));
	assertTrue(sqlrcur_getTableList(cur,NULL));
	assertInResultSet(cur,"Tables_in_xxx","testtable1");
	assertInResultSet(cur,"Tables_in_xxx","testtable2");
	assertInResultSet(cur,"Tables_in_xxx","testtable3");
	assertInResultSet(cur,"Tables_in_xxx","testtable4");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable1"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable2"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable3"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable4"));
	printf("\n");


	// type info list
	printf("TYPE INFO LIST: \n");
	assertTrue(sqlrcur_getTypeInfoList(cur,"integer"));
	assertEqStr(sqlrcur_getColumnName(cur,0),"type_name");
	assertEqStr(sqlrcur_getColumnName(cur,1),"data_type");
	assertEqStr(sqlrcur_getColumnName(cur,2),"precision");
	assertEqStr(sqlrcur_getColumnName(cur,3),"literal_prefix");
	assertEqStr(sqlrcur_getColumnName(cur,4),"literal_suffix");
	assertEqStr(sqlrcur_getColumnName(cur,5),"create_params");
	assertEqStr(sqlrcur_getColumnName(cur,6),"nullable");
	assertEqStr(sqlrcur_getColumnName(cur,7),"case_sensitive");
	assertEqStr(sqlrcur_getColumnName(cur,8),"searchable");
	assertEqStr(sqlrcur_getColumnName(cur,9),"unsigned_attribute");
	assertEqStr(sqlrcur_getColumnName(cur,10),"fixed_prec_scale");
	assertEqStr(sqlrcur_getColumnName(cur,11),"auto_increment");
	assertEqStr(sqlrcur_getColumnName(cur,12),"local_type_name");
	assertEqStr(sqlrcur_getColumnName(cur,13),"minumum_scale");
	assertEqStr(sqlrcur_getColumnName(cur,14),"maxiumm_scale");
	assertEqStr(sqlrcur_getColumnName(cur,15),"sql_data_type");
	assertEqStr(sqlrcur_getColumnName(cur,16),"sql_datetime_sub");
	assertEqStr(sqlrcur_getColumnName(cur,17),"num_prec_radix");
	assertEqStr(sqlrcur_getColumnName(cur,18),"interval_precision");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"INTEGER");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"4");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"10");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"INTEGER");
	assertTrue(sqlrcur_getTypeInfoList(cur,"char"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"CHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"255");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"CHAR");
	assertTrue(sqlrcur_getTypeInfoList(cur,"varchar"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"VARCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"12");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"255");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"VARCHAR");
	assertTrue(sqlrcur_getTypeInfoList(cur,"date"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"DATE");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"91");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"10");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"DATE");
	printf("\n");


	// column list
	printf("COLUMN LIST: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testint int, "
		"	testfloat float, "
		"	testreal real, "
		"	testsmallint smallint, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testdate date, "
		"	testtime time, "
		"	testtimestamp timestamp, "
		"	testtext text, "
		"	testbytea bytea)"));
	assertTrue(sqlrcur_getColumnList(cur,"testtable",NULL));
	assertEqStr(sqlrcur_getColumnName(cur,0),"column_name");
	assertEqStr(sqlrcur_getColumnName(cur,1),"data_type");
	assertEqStr(sqlrcur_getColumnName(cur,2),"character_maximum_length");
	assertEqStr(sqlrcur_getColumnName(cur,3),"numeric_precision");
	assertEqStr(sqlrcur_getColumnName(cur,4),"numeric_scale");
	assertEqStr(sqlrcur_getColumnName(cur,5),"is_nullable");
	assertEqStr(sqlrcur_getColumnName(cur,6),"column_key");
	assertEqStr(sqlrcur_getColumnName(cur,7),"column_default");
	assertEqStr(sqlrcur_getColumnName(cur,8),"extra");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"column_name"),"testint");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"column_name"),"testfloat");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"column_name"),"testreal");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"column_name"),"testsmallint");
	assertEqStr(sqlrcur_getFieldByName(cur,4,"column_name"),"testchar");
	assertEqStr(sqlrcur_getFieldByName(cur,5,"column_name"),"testvarchar");
	assertEqStr(sqlrcur_getFieldByName(cur,6,"column_name"),"testdate");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"column_name"),"testtime");
	assertEqStr(sqlrcur_getFieldByName(cur,8,"column_name"),
		"testtimestamp");
	assertEqStr(sqlrcur_getFieldByName(cur,9,"column_name"),"testtext");
	assertEqStr(sqlrcur_getFieldByName(cur,10,"column_name"),"testbytea");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"integer");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"data_type"),
		"double precision");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"data_type"),"real");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"data_type"),"smallint");
	assertEqStr(sqlrcur_getFieldByName(cur,4,"data_type"),"character");
	assertEqStr(sqlrcur_getFieldByName(cur,5,"data_type"),
		"character varying");
	assertEqStr(sqlrcur_getFieldByName(cur,6,"data_type"),"date");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"data_type"),
		"time without time zone");
	assertEqStr(sqlrcur_getFieldByName(cur,8,"data_type"),
		"timestamp without time zone");
	assertEqStr(sqlrcur_getFieldByName(cur,9,"data_type"),"text");
	assertEqStr(sqlrcur_getFieldByName(cur,10,"data_type"),"bytea");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// column list - auto_increment, primary key
	printf("COLUMN LIST - ""auto_increment, primary key: \n");
	sqlrcur_sendQuery(cur,"drop table if exists testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 serial primary key, "
		"	col2 int)"));
	assertTrue(sqlrcur_getColumnList(cur,"testtable",NULL));
	assertContains(sqlrcur_getFieldByName(cur,0,"extra"),
		"auto_increment");
	assertContains(sqlrcur_getFieldByName(cur,0,"column_key"),
		"PRI");
	assertNotContains(sqlrcur_getFieldByName(cur,1,"extra"),
		"auto_increment");
	assertNotContains(sqlrcur_getFieldByName(cur,1,"column_key"),
		"PRI");
	printf("\n");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)"));
	assertTrue(sqlrcur_getColumnList(cur,"testtable",NULL));
	assertNotContains(sqlrcur_getFieldByName(cur,0,"extra"),
		"auto_increment");
	assertContains(sqlrcur_getFieldByName(cur,0,"column_key"),
		"PRI");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// primary keys list
	printf("PRIMARY KEYS LIST: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)"));
	assertTrue(sqlrcur_getPrimaryKeysList(cur,"testtable",NULL));
	assertEqStr(sqlrcur_getColumnName(cur,0),"table");
	assertEqStr(sqlrcur_getColumnName(cur,1),"non_unique");
	assertEqStr(sqlrcur_getColumnName(cur,2),"key_name");
	assertEqStr(sqlrcur_getColumnName(cur,3),"seq_in_index");
	assertEqStr(sqlrcur_getColumnName(cur,4),"column_name");
	assertEqStr(sqlrcur_getColumnName(cur,5),"collation");
	assertEqStr(sqlrcur_getColumnName(cur,6),"cardinality");
	assertEqStr(sqlrcur_getColumnName(cur,7),"sub_part");
	assertEqStr(sqlrcur_getColumnName(cur,8),"packed");
	assertEqStr(sqlrcur_getColumnName(cur,9),"null");
	assertEqStr(sqlrcur_getColumnName(cur,10),"index_type");
	assertEqStr(sqlrcur_getColumnName(cur,11),"comment");
	assertEqStr(sqlrcur_getColumnName(cur,12),"index_comment");
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByName(cur,0,"table"),"testtable");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"seq_in_index"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"column_name"),"col1");
	{
		const char *keyname=sqlrcur_getFieldByName(cur,0,"key_name");
		assertTrue(keyname && keyname[0]);
	}
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// key and index list
	printf("KEY AND INDEX LIST: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)"));
	assertTrue(sqlrcur_getKeyAndIndexList(cur,"testtable",NULL));
	assertEqStr(sqlrcur_getColumnName(cur,0),"table");
	assertEqStr(sqlrcur_getColumnName(cur,1),"non_unique");
	assertEqStr(sqlrcur_getColumnName(cur,2),"key_name");
	assertEqStr(sqlrcur_getColumnName(cur,3),"seq_in_index");
	assertEqStr(sqlrcur_getColumnName(cur,4),"column_name");
	assertEqStr(sqlrcur_getColumnName(cur,5),"collation");
	assertEqStr(sqlrcur_getColumnName(cur,6),"cardinality");
	assertEqStr(sqlrcur_getColumnName(cur,7),"sub_part");
	assertEqStr(sqlrcur_getColumnName(cur,8),"packed");
	assertEqStr(sqlrcur_getColumnName(cur,9),"null");
	assertEqStr(sqlrcur_getColumnName(cur,10),"index_type");
	assertEqStr(sqlrcur_getColumnName(cur,11),"comment");
	assertEqStr(sqlrcur_getColumnName(cur,12),"index_comment");
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByName(cur,0,"table"),"testtable");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"non_unique"),"f");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"seq_in_index"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"column_name"),"col1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"collation"),"A");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"index_type"),"3");
	{
		const char *keyname=sqlrcur_getFieldByName(cur,0,"key_name");
		assertTrue(keyname && keyname[0]);
	}
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// procedure list
	printf("PROCEDURE LIST: \n");
	sqlrcur_sendQuery(cur,"drop function ""testproc1(int,char,"
		"varchar,date)");
	sqlrcur_sendQuery(cur,"drop function ""testproc2(int,char,"
		"varchar,date)");
	sqlrcur_sendQuery(cur,"drop function ""testproc3(int,char,"
		"varchar,date)");
	sqlrcur_sendQuery(cur,"drop function ""testproc4(int,char,"
		"varchar,date)");
	assertTrue(sqlrcur_sendQuery(cur,
		"create function testproc1("
		"	in1 int, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) ""returns void ""as 'begin end;' "
		"language plpgsql"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create function testproc2("
		"	in1 int, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) ""returns void ""as 'begin end;' "
		"language plpgsql"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create function testproc3("
		"	in1 int, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) ""returns void ""as 'begin end;' "
		"language plpgsql"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create function testproc4("
		"	in1 int, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) ""returns void ""as 'begin end;' "
		"language plpgsql"));
	assertTrue(sqlrcur_getProcedureList(cur,NULL));
	assertInResultSet(cur,"routine_name","testproc1");
	assertInResultSet(cur,"routine_name","testproc2");
	assertInResultSet(cur,"routine_name","testproc3");
	assertInResultSet(cur,"routine_name","testproc4");
	printf("\n");


	// procedure parameter list
	printf("PROCEDURE PARAMETER LIST: \n");
	assertTrue(sqlrcur_getProcedureParameterList(cur,"testproc1",NULL));
	assertEqStr(sqlrcur_getColumnName(cur,0),"parameter_name");
	assertEqStr(sqlrcur_getColumnName(cur,1),"parameter_mode");
	assertEqStr(sqlrcur_getColumnName(cur,2),"data_type");
	assertEqStr(sqlrcur_getColumnName(cur,3),"character_maximum_length");
	assertEqStr(sqlrcur_getColumnName(cur,4),"ordinal_position");
	assertEqInt(sqlrcur_rowCount(cur),4);
	assertEqStr(sqlrcur_getFieldByName(cur,0,"parameter_name"),"in1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"integer");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"ordinal_position"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"parameter_name"),"in2");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"data_type"),"character");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"ordinal_position"),"2");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"parameter_name"),"in3");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"data_type"),
		"character varying");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"ordinal_position"),"3");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"parameter_name"),"in4");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"data_type"),"date");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"ordinal_position"),"4");
	assertTrue(sqlrcur_sendQuery(cur,"drop function ""testproc1(int,char,"
		"varchar,date)"));
	assertTrue(sqlrcur_sendQuery(cur,"drop function ""testproc2(int,char,"
		"varchar,date)"));
	assertTrue(sqlrcur_sendQuery(cur,"drop function ""testproc3(int,char,"
		"varchar,date)"));
	assertTrue(sqlrcur_sendQuery(cur,"drop function ""testproc4(int,char,"
		"varchar,date)"));
	printf("\n");


	// invalid queries
	printf("INVALID QUERIES: \n");
	assertFalse(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	assertFalse(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	assertFalse(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	assertFalse(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,"insert into testtable "
		"values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(cur,"insert into testtable "
		"values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(cur,"insert into testtable "
		"values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(cur,"insert into testtable "
		"values (1,2,3,4)"));
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	printf("\n");

	reportTestStatus();

	return status;
}
