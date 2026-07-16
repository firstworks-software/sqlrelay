// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclientwrapper.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
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

	const char	*subvars[]={"var1","var2","var3",NULL};
	const char	*subvalstrings[]={"hi","hello","bye"};
	int64_t		subvallongs[]={1,2,3};
	double		subvaldoubles[]={10.55,10.556,10.5556};
	uint32_t	precs[]={4,5,6};
	uint32_t	scales[]={2,3,4};
	const char * const *cols;
	const char * const *fields;
	uint16_t	port;
	const char	*socket;
	uint16_t	id;
	char		*filename;
	uint32_t	*fieldlens;
	const char	**il;
	uint64_t	i;
	unsigned char	buffer[256];
	uint16_t	j;
	char		querystr[1024];
	char		hex[3];

	#define	LARGE_BUFFER_LENGTH	8192
	char		largebuffer[LARGE_BUFFER_LENGTH+1];

	const char	*isolationlevels[]={"0","1",NULL};


	// instantiation
	con=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
			"testuser","testpassword",0,1);
	cur=sqlrcur_alloc(con);


	// identify
	printf("IDENTIFY: \n");
	assertEqStr(sqlrcon_identify(con),"sqlite");
	printf("\n");


	// db version
	printf("DB VERSION: \n");
	const char	*dbversion=sqlrcon_dbVersion(con);
	int		issqlite3=1;
	if (!dbversion ||
		!strcmp(dbversion,"unknown") ||
		atoi(dbversion)<3) {
		issqlite3=0;
	}
	// table-valued pragma functions were added in sqlite 3.16.0
	int		haspragmafuncs=0;
	if (issqlite3) {
		const char	*dot=strchr(dbversion,'.');
		int		majorversion=atoi(dbversion);
		int		minorversion=(dot)?atoi(dot+1):0;
		haspragmafuncs=(majorversion>3 ||
				(majorversion==3 && minorversion>=16));
	}
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
	assertEqStr(sqlrcon_bindFormat(con),":*");
	printf("\n");


	// nextval format
	printf("NEXTVAL FORMAT: \n");
	assertEqStr(sqlrcon_nextvalFormat(con),"");
	printf("\n");


	// isolation levels
	printf("ISOLATION LEVELS: \n");
	for (il=isolationlevels; *il; il++) {
		assertTrue(sqlrcon_setIsolationLevel(con,*il));
		assertEqStr(sqlrcon_getIsolationLevel(con),*il);
		printf("\n");
	}
	// reset to the default isolation level
	assertTrue(sqlrcon_setIsolationLevel(con,isolationlevels[0]));
	printf("\n");


	// create testtable
	printf("CREATE TESTTABLE: \n");
	sqlrcon_begin(con);
	sqlrcur_sendQuery(cur,"drop table if exists testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testint int, "
		"	testfloat float, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testclob clob, "
		"	testblob blob)"));
	sqlrcon_commit(con);
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
		"	'testchar1', "
		"	'testvarchar1', "
		"	'testclob1', "
		"	'testblob1')"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	2, "
		"	2.5, "
		"	'testchar2', "
		"	'testvarchar2', "
		"	'testclob2', "
		"	'testblob2')"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	3, "
		"	3.5, "
		"	'testchar3', "
		"	'testvarchar3', "
		"	'testclob3', "
		"	'testblob3')"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	4, "
		"	4.5, "
		"	'testchar4', "
		"	'testvarchar4', "
		"	'testclob4', "
		"	'testblob4')"));
	printf("\n");


	// affected rows
	printf("AFFECTED ROWS: \n");
	assertEqInt(sqlrcur_affectedRows(cur),1);
	printf("\n");


	// input bind by position
	// sqlite doesn't support bind by position


	// array of input binds by position
	// sqlite doesn't support bind by position


	// input bind by position with validation
	// sqlite doesn't support bind by position


	// input bind by name
	printf("INPUT BIND BY NAME: \n");
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	:var1, "
		"	:var2, "
		"	:var3, "
		"	:var4, "
		"	:var5, "
		"	:var6)");
	assertEqInt(sqlrcur_countBindVariables(cur),6);
	sqlrcur_inputBindLong(cur,"var1",5);
	sqlrcur_inputBindDouble(cur,"var2",5.5,4,1);
	sqlrcur_inputBindString(cur,"var3","testchar5");
	sqlrcur_inputBindString(cur,"var4","testvarchar5");
	sqlrcur_inputBindClob(cur,"var5","testclob5",9);
	sqlrcur_inputBindBlob(cur,"var6","testblob5",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",6);
	sqlrcur_inputBindDouble(cur,"var2",6.5,4,1);
	sqlrcur_inputBindString(cur,"var3","testchar6");
	sqlrcur_inputBindString(cur,"var4","testvarchar6");
	sqlrcur_inputBindClob(cur,"var5","testclob6",9);
	sqlrcur_inputBindBlob(cur,"var6","testblob6",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",7);
	sqlrcur_inputBindDouble(cur,"var2",7.5,4,1);
	sqlrcur_inputBindString(cur,"var3","testchar7");
	sqlrcur_inputBindString(cur,"var4","testvarchar7");
	sqlrcur_inputBindClob(cur,"var5","testclob7",9);
	sqlrcur_inputBindBlob(cur,"var6","testblob7",9);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// array of input binds by name
	// sqlite doesn't support implicit conversion
	// of string binds to other data types, so
	// arrays of binds don't generally work.


	// input bind by name with validation
	printf("INPUT BIND BY NAME ""WITH VALIDATION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",8);
	sqlrcur_inputBindDouble(cur,"var2",8.5,4,1);
	sqlrcur_inputBindString(cur,"var3","testchar8");
	sqlrcur_inputBindString(cur,"var4","testvarchar8");
	sqlrcur_inputBindClob(cur,"var5","testclob8",9);
	sqlrcur_inputBindBlob(cur,"var6","testblob8",9);
	sqlrcur_validateBinds(cur);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// select
	printf("SELECT: \n");
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	printf("\n");


	// column count
	printf("COLUMN COUNT: \n");
	assertEqInt(sqlrcur_colCount(cur),6);
	printf("\n");


	// column names
	printf("COLUMN NAMES: \n");
	assertEqStr(sqlrcur_getColumnName(cur,0),"testint");
	assertEqStr(sqlrcur_getColumnName(cur,1),"testfloat");
	assertEqStr(sqlrcur_getColumnName(cur,2),"testchar");
	assertEqStr(sqlrcur_getColumnName(cur,3),"testvarchar");
	cols=sqlrcur_getColumnNames(cur);
	assertEqStr(cols[0],"testint");
	assertEqStr(cols[1],"testfloat");
	assertEqStr(cols[2],"testchar");
	assertEqStr(cols[3],"testvarchar");
	printf("\n");


	// column types
	printf("COLUMN TYPES: \n");
	if (issqlite3) {
		assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),"INTEGER");
		assertEqStr(sqlrcur_getColumnTypeByName(cur,"testint"),"INTEGER");
		assertEqStr(sqlrcur_getColumnTypeByIndex(cur,1),"FLOAT");
		assertEqStr(sqlrcur_getColumnTypeByName(cur,"testfloat"),"FLOAT");
		assertEqStr(sqlrcur_getColumnTypeByIndex(cur,2),"STRING");
		assertEqStr(sqlrcur_getColumnTypeByName(cur,"testchar"),"STRING");
		assertEqStr(sqlrcur_getColumnTypeByIndex(cur,3),"STRING");
		assertEqStr(sqlrcur_getColumnTypeByName(cur,"testvarchar"),"STRING");
		assertEqStr(sqlrcur_getColumnTypeByIndex(cur,4),"STRING");
		assertEqStr(sqlrcur_getColumnTypeByName(cur,"testclob"),"STRING");
		assertEqStr(sqlrcur_getColumnTypeByIndex(cur,5),"STRING");
		assertEqStr(sqlrcur_getColumnTypeByName(cur,"testblob"),"STRING");
	} else {
		assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnTypeByName(cur,"testint"),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnTypeByIndex(cur,1),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnTypeByName(cur,"testfloat"),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnTypeByIndex(cur,2),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnTypeByName(cur,"testchar"),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnTypeByIndex(cur,3),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnTypeByName(cur,"testvarchar"),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnTypeByIndex(cur,4),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnTypeByName(cur,"testclob"),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnTypeByIndex(cur,5),"UNKNOWN");
		assertEqStr(sqlrcur_getColumnTypeByName(cur,"testblob"),"UNKNOWN");
	}
	printf("\n");


	// column length
	printf("COLUMN LENGTH: \n");
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),0);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testint"),0);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,1),0);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testfloat"),0);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,2),0);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testchar"),0);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,3),0);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testvarchar"),0);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,4),0);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testclob"),0);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,5),0);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testblob"),0);
	printf("\n");


	// longest column
	printf("LONGEST COLUMN: \n");
	assertEqInt(sqlrcur_getLongestByIndex(cur,0),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"testint"),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,1),3);
	assertEqInt(sqlrcur_getLongestByName(cur,"testfloat"),3);
	assertEqInt(sqlrcur_getLongestByIndex(cur,2),9);
	assertEqInt(sqlrcur_getLongestByName(cur,"testchar"),9);
	assertEqInt(sqlrcur_getLongestByIndex(cur,3),12);
	assertEqInt(sqlrcur_getLongestByName(cur,"testvarchar"),12);
	assertEqInt(sqlrcur_getLongestByIndex(cur,4),9);
	assertEqInt(sqlrcur_getLongestByName(cur,"testclob"),9);
	assertEqInt(sqlrcur_getLongestByIndex(cur,5),9);
	assertEqInt(sqlrcur_getLongestByName(cur,"testblob"),9);
	printf("\n");


	// row count
	printf("ROW COUNT: \n");
	assertEqInt(sqlrcur_rowCount(cur),8);
	printf("\n");


	// total rows
	printf("TOTAL ROWS: \n");
	assertEqInt(sqlrcur_totalRows(cur),(issqlite3)?0:8);
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
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"testchar1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,3),"testvarchar1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,4),"testclob1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,5),"testblob1");
	printf("\n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8.5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,2),"testchar8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,3),"testvarchar8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,4),"testclob8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,5),"testblob8");
	printf("\n");


	// field lengths by index
	printf("FIELD LENGTHS BY INDEX: \n");
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,1),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,2),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,3),12);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,4),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,5),9);
	printf("\n");
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,0),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,1),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,2),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,3),12);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,4),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,5),9);
	printf("\n");


	// fields by name
	printf("FIELDS BY NAME: \n");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testint"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testfloat"),"1.5");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testchar"),"testchar1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testvarchar"),"testvarchar1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testclob"),"testclob1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testblob"),"testblob1");
	printf("\n");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testint"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testfloat"),"8.5");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testchar"),"testchar8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testvarchar"),"testvarchar8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testclob"),"testclob8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testblob"),"testblob8");
	printf("\n");


	// field lengths by name
	printf("FIELD LENGTHS BY NAME: \n");
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testchar"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testvarchar"),12);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testclob"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testblob"),9);
	printf("\n");
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testchar"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testvarchar"),12);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testclob"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testblob"),9);
	printf("\n");


	// fields by array
	printf("FIELDS BY ARRAY: \n");
	fields=sqlrcur_getRow(cur,0);
	assertEqStr(fields[0],"1");
	assertEqStr(fields[1],"1.5");
	assertEqStr(fields[2],"testchar1");
	assertEqStr(fields[3],"testvarchar1");
	assertEqStr(fields[4],"testclob1");
	assertEqStr(fields[5],"testblob1");
	printf("\n");


	// field lengths by array
	printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=sqlrcur_getRowLengths(cur,0);
	assertEqInt(fieldlens[0],1);
	assertEqInt(fieldlens[1],3);
	assertEqInt(fieldlens[2],9);
	assertEqInt(fieldlens[3],12);
	assertEqInt(fieldlens[4],9);
	assertEqInt(fieldlens[5],9);
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
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),0);
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),
				(issqlite3)?"INTEGER":"UNKNOWN");
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
	sqlrcur_cacheToFile(cur,"cachefile1");
	sqlrcur_setCacheTtl(cur,200);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqStr(filename,"cachefile1");
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	free(filename);
	printf("\n");


	// column count for cached result set
	printf("COLUMN COUNT FOR ""CACHED RESULT SET: \n");
	assertEqInt(sqlrcur_colCount(cur),6);
	printf("\n");


	// column names for cached result set
	printf("COLUMN NAMES FOR ""CACHED RESULT SET: \n");
	assertEqStr(sqlrcur_getColumnName(cur,0),"testint");
	assertEqStr(sqlrcur_getColumnName(cur,1),"testfloat");
	assertEqStr(sqlrcur_getColumnName(cur,2),"testchar");
	assertEqStr(sqlrcur_getColumnName(cur,3),"testvarchar");
	assertEqStr(sqlrcur_getColumnName(cur,4),"testclob");
	assertEqStr(sqlrcur_getColumnName(cur,5),"testblob");
	cols=sqlrcur_getColumnNames(cur);
	assertEqStr(cols[0],"testint");
	assertEqStr(cols[1],"testfloat");
	assertEqStr(cols[2],"testchar");
	assertEqStr(cols[3],"testvarchar");
	assertEqStr(cols[4],"testclob");
	assertEqStr(cols[5],"testblob");
	printf("\n");


	// cached result set with result set
	// buffer size
	printf("CACHED RESULT SET ""WITH RESULT SET ""BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile1");
	sqlrcur_setCacheTtl(cur,200);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqStr(filename,"cachefile1");
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize(cur,0);
	free(filename);
	printf("\n");


	// from one cache file to another
	printf("FROM ONE CACHE FILE ""TO ANOTHER: \n");
	sqlrcur_cacheToFile(cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile1"));
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile2"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	printf("\n");


	// from one cache file to another
	// with result set buffer size
	printf("FROM ONE CACHE FILE TO ANOTHER ""WITH RESULT SET "
		"BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile1"));
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile2"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize(cur,0);
	printf("\n");


	// cached result set with suspend
	// and result set buffer size
	printf("CACHED RESULT SET WITH SUSPEND ""AND RESULT SET "
		"BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile1");
	sqlrcur_setCacheTtl(cur,200);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,2,0),"3");
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqStr(filename,"cachefile1");
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
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable"));
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
	assertTrue(sqlrcur_sendQuery(cur,"drop table if exists testtable"));
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
	// sqlite DDL is transactional; commit so the table is visible
	// to the second connection (the commit implicitly starts a new tx)
	assertTrue(sqlrcon_commit(con));
	secondcon=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
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
	sqlrcur_sendQuery(cur,"drop table if exists testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 int, "
		"	col2 char, "
		"	col3 float)"));
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	$(var1), "
		"	'$(var2)', "
		"	$(var3))");
	sqlrcur_subLong(cur,"var1",1);
	sqlrcur_subString(cur,"var2","hello");
	sqlrcur_subDouble(cur,"var3",10.5556,6,4);
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"10.5556");
	assertTrue(sqlrcur_sendQuery(cur,"delete from testtable"));
	printf("\n");


	// array substitutions
	printf("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	'$(var1)', "
		"	'$(var2)', "
		"	'$(var3)')");
	sqlrcur_subStrings(cur,subvars,subvalstrings);
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"hi");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"bye");
	assertTrue(sqlrcur_sendQuery(cur,"delete from testtable"));
	printf("\n");
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	$(var1), "
		"	'$(var2)', "
		"	$(var3))");
	sqlrcur_subLongs(cur,subvars,subvallongs);
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"2");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"3.0");
	assertTrue(sqlrcur_sendQuery(cur,"delete from testtable"));
	printf("\n");
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	$(var1), "
		"	'$(var2)', "
		"	$(var3))");
	sqlrcur_subDoubles(cur,subvars,subvaldoubles,precs,scales);
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"10.55");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"10.556");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"10.5556");
	assertTrue(sqlrcur_sendQuery(cur,"delete from testtable"));
	printf("\n");


	// nulls as nulls
	printf("NULLS AS NULLS: \n");
	sqlrcur_getNullsAsNulls(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	NULL, "
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"");
	assertTrue(sqlrcur_sendQuery(cur,"drop table if exists testtable"));
	printf("\n");


	// null and empty lobs
	printf("NULL AND EMPTY LOBS: \n");
	sqlrcur_getNullsAsNulls(cur);
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testclob1 clob, "
		"	testclob2 clob, "
		"	testblob1 blob, "
		"	testblob2 blob)"));
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	:var1, "
		"	:var2, "
		"	:var3, "
		"	:var4)");
	sqlrcur_inputBindClob(cur,"var1","",0);
	sqlrcur_inputBindClob(cur,"var2",NULL,0);
	sqlrcur_inputBindBlob(cur,"var3","",0);
	sqlrcur_inputBindBlob(cur,"var4",NULL,0);
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
		"	testclob clob, "
		"	testblob blob)"));
	sqlrcur_prepareQuery(cur,"insert into testtable "
		"values (:clobval,:blobval)");
	for (i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	sqlrcur_inputBindClob(cur,"clobval",largebuffer,LARGE_BUFFER_LENGTH);
	sqlrcur_inputBindBlob(cur,"blobval",largebuffer,LARGE_BUFFER_LENGTH);
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable"));
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testclob"),
		LARGE_BUFFER_LENGTH);
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testclob"),largebuffer);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testblob"),
		LARGE_BUFFER_LENGTH);
	assertEqStrLen(sqlrcur_getFieldByName(cur,0,"testblob"),largebuffer,
		LARGE_BUFFER_LENGTH);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// output bind by position
	// sqlite doesn't support output binds


	// output bind by name
	// sqlite doesn't support output binds


	// output bind by name with validation
	// sqlite doesn't support output binds


	// lob output bind
	// sqlite doesn't support output binds


	// long output bind
	// sqlite doesn't support output binds


	// negative input bind
	printf("NEGATIVE INPUT BIND: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable ""(testval int)"));
	sqlrcur_prepareQuery(cur,"insert into testtable ""values (:testval)");
	sqlrcur_inputBindLong(cur,"testval",-1);
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"select testval from testtable"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testval"),"-1");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// bind validation
	printf("BIND VALIDATION: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 varchar(20), "
		"	col2 varchar(20), "
		"	col3 varchar(20))"));
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	$(var1), "
		"	$(var2), "
		"	$(var3))");
	sqlrcur_inputBindString(cur,"var1","1");
	sqlrcur_inputBindString(cur,"var2","2");
	sqlrcur_inputBindString(cur,"var3","3");
	sqlrcur_subString(cur,"var1",":var1");
	assertTrue(sqlrcur_validBind(cur,"var1"));
	assertFalse(sqlrcur_validBind(cur,"var2"));
	assertFalse(sqlrcur_validBind(cur,"var3"));
	assertFalse(sqlrcur_validBind(cur,"var4"));
	printf("\n");
	sqlrcur_subString(cur,"var2",":var2");
	assertTrue(sqlrcur_validBind(cur,"var1"));
	assertTrue(sqlrcur_validBind(cur,"var2"));
	assertFalse(sqlrcur_validBind(cur,"var3"));
	assertFalse(sqlrcur_validBind(cur,"var4"));
	printf("\n");
	sqlrcur_subString(cur,"var3",":var3");
	assertTrue(sqlrcur_validBind(cur,"var1"));
	assertTrue(sqlrcur_validBind(cur,"var2"));
	assertTrue(sqlrcur_validBind(cur,"var3"));
	assertFalse(sqlrcur_validBind(cur,"var4"));
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// rebinding
	printf("REBINDING: \n");
	sqlrcur_prepareQuery(cur,"select :val");
	sqlrcur_inputBindLong(cur,"val",1);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	sqlrcur_inputBindLong(cur,"val",2);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"2");
	sqlrcur_inputBindLong(cur,"val",3);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"3");
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
	sqlrcur_prepareQuery(cur,"select :var");
	sqlrcur_inputBindLong(cur,"var",1);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	printf("\n");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	printf("\n");
	sqlrcur_inputBindLong(cur,"var",2);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"2");
	printf("\n");


	// stored procedure returning no value
	// sqlite doesn't support stored procedures


	// stored procedure returning single value
	// sqlite doesn't support stored procedures


	// stored procedure returning multiple values
	// sqlite doesn't support stored procedures


	// stored procedure returning result set
	// sqlite doesn't support stored procedures


	// temporary tables
	printf("TEMPORARY TABLES: \n");
	sqlrcur_sendQuery(cur,"drop table if exists temptable\n");
	assertTrue(sqlrcur_sendQuery(cur,"create temporary table ""temptable (col1 int)"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into temptable ""values (1)"));
	assertTrue(sqlrcur_sendQuery(cur,"select count(*) from temptable"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	sqlrcon_endSession(con);
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,"select count(*) from temptable"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table if exists temptable\n"));
	printf("\n");


	// encoded binary data
	printf("ENCODED BINARY DATA: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable "
		"(col1 blob)"));
	for (j=0; j<256; j++) {
		buffer[j]=(unsigned char)j;
	}
	strcpy(querystr,"insert into testtable ""values (X'");
	for (i=0; i<sizeof(buffer); i++) {
		snprintf(hex,sizeof(hex),"%02x",buffer[i]);
		strcat(querystr,hex);
	}
	strcat(querystr,"')");
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
		"	(col1 integer primary key "
		"	autoincrement, "
		"	col2 int)"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable "
		"values (null,1)"));
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
	assertEqInt(sqlrcur_rowCount(cur),0);
	printf("\n");


	// schema list
	printf("SCHEMA LIST: \n");
	assertTrue(sqlrcur_getSchemaList(cur,NULL));
	assertEqStr(sqlrcur_getColumnName(cur,0),"Database");
	printf("\n");


	// table type list
	printf("TABLE TYPE LIST: \n");
	assertTrue(sqlrcur_getTableTypeList(cur));
	assertEqStr(sqlrcur_getColumnName(cur,0),"table_type");
	assertInResultSet(cur,"table_type","TABLE");
	printf("\n");


	// table list
	printf("TABLE LIST: \n");
	sqlrcur_sendQuery(cur,"drop table if exists testtable1");
	sqlrcur_sendQuery(cur,"drop table if exists testtable2");
	sqlrcur_sendQuery(cur,"drop table if exists testtable3");
	sqlrcur_sendQuery(cur,"drop table if exists testtable4");
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
	assertTrue(sqlrcur_sendQuery(cur,"drop table if exists testtable1"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table if exists testtable2"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table if exists testtable3"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table if exists testtable4"));
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
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"19");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"INTEGER");
	assertTrue(sqlrcur_getTypeInfoList(cur,"char"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"CHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"2147483647");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"CHAR");
	assertTrue(sqlrcur_getTypeInfoList(cur,"varchar"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"VARCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"12");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"2147483647");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"VARCHAR");
	assertTrue(sqlrcur_getTypeInfoList(cur,"date"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"DATE");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"91");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"10");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"DATE");
	printf("\n");


	// column list
	printf("COLUMN LIST: \n");
	if (haspragmafuncs) {
		sqlrcur_sendQuery(cur,"drop table if exists testtable");
		assertTrue(sqlrcur_sendQuery(cur,
			"create table testtable ("
			"	testint int, "
			"	testfloat float, "
			"	testchar char(40), "
			"	testvarchar varchar(40), "
			"	testclob clob, "
			"	testblob blob)"));
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
		assertEqStr(sqlrcur_getFieldByName(cur,2,"column_name"),"testchar");
		assertEqStr(sqlrcur_getFieldByName(cur,3,"column_name"),"testvarchar");
		assertEqStr(sqlrcur_getFieldByName(cur,4,"column_name"),"testclob");
		assertEqStr(sqlrcur_getFieldByName(cur,5,"column_name"),"testblob");
		assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"INT");
		assertEqStr(sqlrcur_getFieldByName(cur,1,"data_type"),"FLOAT");
		assertEqStr(sqlrcur_getFieldByName(cur,2,"data_type"),"CHAR");
		assertEqStr(sqlrcur_getFieldByName(cur,3,"data_type"),"VARCHAR");
		assertEqStr(sqlrcur_getFieldByName(cur,4,"data_type"),"CLOB");
		assertEqStr(sqlrcur_getFieldByName(cur,5,"data_type"),"BLOB");
		assertTrue(sqlrcur_sendQuery(cur,"drop table if exists testtable"));
	}
	printf("\n");


	// column list - auto_increment,
	// primary key
	printf("COLUMN LIST - ""auto_increment, ""primary key: \n");
	if (haspragmafuncs) {
		sqlrcur_sendQuery(cur,"drop table if exists testtable");
		assertTrue(sqlrcur_sendQuery(cur,
			"create table testtable ("
			"	col1 integer primary key "
			"	autoincrement, "
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
		assertTrue(sqlrcur_sendQuery(cur,"drop table if exists testtable"));
		assertTrue(sqlrcur_sendQuery(cur,
			"create table testtable ("
			"	col1 int primary key, "
			"	col2 int)"));
		assertTrue(sqlrcur_getColumnList(cur,"testtable",NULL));
		assertNotContains(sqlrcur_getFieldByName(cur,0,"extra"),
				"auto_increment");
		assertContains(sqlrcur_getFieldByName(cur,0,"column_key"),
				"PRI");
		assertTrue(sqlrcur_sendQuery(cur,"drop table if exists testtable"));
	}
	printf("\n");


	// primary keys list
	printf("PRIMARY KEYS LIST: \n");
	if (haspragmafuncs) {
		sqlrcur_sendQuery(cur,"drop table if exists testtable");
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
		assertTrue(sqlrcur_sendQuery(cur,"drop table if exists testtable"));
	}
	printf("\n");


	// key and index list
	printf("KEY AND INDEX LIST: \n");
	if (haspragmafuncs) {
		sqlrcur_sendQuery(cur,"drop table if exists testtable");
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
		assertEqStr(sqlrcur_getFieldByName(cur,0,"non_unique"),"0");
		assertEqStr(sqlrcur_getFieldByName(cur,0,"seq_in_index"),"1");
		assertEqStr(sqlrcur_getFieldByName(cur,0,"column_name"),"col1");
		assertEqStr(sqlrcur_getFieldByName(cur,0,"collation"),"A");
		assertEqStr(sqlrcur_getFieldByName(cur,0,"index_type"),"3");
		{
			const char *kn=sqlrcur_getFieldByName(cur,0,"key_name");
			assertTrue(!((!kn) || (!kn[0])));
		}
		assertTrue(sqlrcur_sendQuery(cur,"drop table if exists testtable"));
	}
	printf("\n");


	// procedure list
	printf("PROCEDURE LIST: \n");
	assertTrue(sqlrcur_getProcedureList(cur,NULL));
	assertEqInt(sqlrcur_rowCount(cur),0);
	printf("\n");


	// procedure parameter list
	printf("PROCEDURE PARAMETER LIST: \n");
	assertTrue(sqlrcur_getProcedureParameterList(cur,"testproc1",NULL));
	assertEqStr(sqlrcur_getColumnName(cur,0),"parameter_name");
	assertEqStr(sqlrcur_getColumnName(cur,1),"parameter_mode");
	assertEqStr(sqlrcur_getColumnName(cur,2),"data_type");
	assertEqStr(sqlrcur_getColumnName(cur,3),"character_maximum_length");
	assertEqStr(sqlrcur_getColumnName(cur,4),"ordinal_position");
	assertEqInt(sqlrcur_rowCount(cur),0);
	printf("\n");


	// invalid queries
	printf("INVALID QUERIES: \n");
	assertFalse(sqlrcur_sendQuery(cur,"select * from testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"select * from testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"select * from testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"select * from testtable"));
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
