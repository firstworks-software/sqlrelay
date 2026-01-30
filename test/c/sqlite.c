// Copyright (c) David Muse
// See the file COPYING for more information.

#include "../../config.h"
#include <sqlrelay/sqlrclientwrapper.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "asserts.c"

sqlrcon	con;
sqlrcur	cur;
sqlrcon	secondcon;
sqlrcur	secondcur;

int main(int argc, char **argv) {

	const char	*subvars[4]={"var1","var2","var3",NULL};
	const char	*subvalstrings[3]={"hi","hello","bye"};
	int64_t		subvallongs[3]={1,2,3};
	double		subvaldoubles[3]={10.55,10.556,10.5556};
	uint32_t	precs[3]={4,5,6};
	uint32_t	scales[3]={2,3,4};
	const char * const *cols;
	const char * const *fields;
	uint16_t	port;
	const char	*socket;
	uint16_t	id;
	char		*filename;
	uint32_t	*fieldlens;

	// instantiation
	con=sqlrcon_alloc("sqlrelay",9000,
			"/tmp/test.socket","testuser","testpassword",0,1);
	cur=sqlrcur_alloc(con);

	// get database type


	// identify
	printf("IDENTIFY: \n");
	assertEqualsString(sqlrcon_identify(con),"sqlite");
	printf("\n");


	// ping
	printf("PING: \n");
	assertTrue(sqlrcon_ping(con));
	printf("\n");


	// isolation levels
	printf("ISOLATION LEVELS: \n");
	{
		const char	*isolationlevels[]={"0","1",NULL};
		const char	**il;
		for (il=isolationlevels; *il; il++) {
			assertTrue(sqlrcon_setIsolationLevel(con,*il));
			assertEqualsString(sqlrcon_getIsolationLevel(con),*il);
			printf("\n");
		}
		// reset to the default isolation level
		assertTrue(sqlrcon_setIsolationLevel(con,isolationlevels[0]));
		printf("\n");
	}

	// drop existing table
	sqlrcur_sendQuery(cur,"begin transaction");
	sqlrcur_sendQuery(cur,"drop table testtable");
	sqlrcon_commit(con);

	// create a new table


	// create temptable
	printf("CREATE TEMPTABLE: \n");
	sqlrcur_sendQuery(cur,"begin transaction");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable (testint int, testfloat float, testchar char(40), testvarchar varchar(40))"));
	sqlrcon_commit(con);
	printf("\n");


	// insert
	printf("INSERT: \n");
	sqlrcur_sendQuery(cur,"begin transaction");
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (1,1.1,'testchar1','testvarchar1')"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (2,2.2,'testchar2','testvarchar2')"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (3,3.3,'testchar3','testvarchar3')"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (4,4.4,'testchar4','testvarchar4')"));
	printf("\n");


	// affected rows
	printf("AFFECTED ROWS: \n");
	assertEqualsInt(sqlrcur_affectedRows(cur),0);
	printf("\n");


	// bind by name
	printf("BIND BY NAME: \n");
	sqlrcur_prepareQuery(cur,"insert into testtable values (:var1,:var2,:var3,:var4)");
	assertEqualsInt(sqlrcur_countBindVariables(cur),4);
	sqlrcur_inputBindLong(cur,"var1",5);
	sqlrcur_inputBindDouble(cur,"var2",5.5,4,1);
	sqlrcur_inputBindString(cur,"var3","testchar5");
	sqlrcur_inputBindString(cur,"var4","testvarchar5");
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",6);
	sqlrcur_inputBindDouble(cur,"var2",6.6,4,1);
	sqlrcur_inputBindString(cur,"var3","testchar6");
	sqlrcur_inputBindString(cur,"var4","testvarchar6");
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",7);
	sqlrcur_inputBindDouble(cur,"var2",7.7,4,1);
	sqlrcur_inputBindString(cur,"var3","testchar7");
	sqlrcur_inputBindString(cur,"var4","testvarchar7");
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// bind by name with validation
	printf("BIND BY NAME WITH VALIDATION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",8);
	sqlrcur_inputBindDouble(cur,"var2",8.8,4,1);
	sqlrcur_inputBindString(cur,"var3","testchar8");
	sqlrcur_inputBindString(cur,"var4","testvarchar8");
	sqlrcur_validateBinds(cur);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// select
	printf("SELECT: \n");
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
	printf("\n");


	// column count
	printf("COLUMN COUNT: \n");
	assertEqualsInt(sqlrcur_colCount(cur),4);
	printf("\n");


	// column names
	printf("COLUMN NAMES: \n");
	assertEqualsString(sqlrcur_getColumnName(cur,0),"testint");
	assertEqualsString(sqlrcur_getColumnName(cur,1),"testfloat");
	assertEqualsString(sqlrcur_getColumnName(cur,2),"testchar");
	assertEqualsString(sqlrcur_getColumnName(cur,3),"testvarchar");
	cols=sqlrcur_getColumnNames(cur);
	assertEqualsString(cols[0],"testint");
	assertEqualsString(cols[1],"testfloat");
	assertEqualsString(cols[2],"testchar");
	assertEqualsString(cols[3],"testvarchar");
	printf("\n");


	// column types
	printf("COLUMN TYPES: \n");
	#ifdef HAVE_SQLITE3_STMT
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),"INTEGER");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testint"),"INTEGER");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,1),"FLOAT");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testfloat"),"FLOAT");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,2),"STRING");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testchar"),"STRING");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,3),"STRING");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testvarchar"),"STRING");
	#else
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),"UNKNOWN");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testint"),"UNKNOWN");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,1),"UNKNOWN");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testfloat"),"UNKNOWN");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,2),"UNKNOWN");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testchar"),"UNKNOWN");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,3),"UNKNOWN");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testvarchar"),"UNKNOWN");
	#endif
	printf("\n");


	// column length
	printf("COLUMN LENGTH: \n");
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,0),0);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testint"),0);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,1),0);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testfloat"),0);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,2),0);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testchar"),0);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,3),0);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testvarchar"),0);
	printf("\n");


	// longest column
	printf("LONGEST COLUMN: \n");
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,0),1);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testint"),1);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,1),3);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testfloat"),3);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,2),9);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testchar"),9);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,3),12);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testvarchar"),12);
	printf("\n");


	// row count
	printf("ROW COUNT: \n");
	assertEqualsInt(sqlrcur_rowCount(cur),8);
	printf("\n");


	// total rows
	printf("TOTAL ROWS: \n");
	#ifdef HAVE_SQLITE3_STMT
	assertEqualsInt(sqlrcur_totalRows(cur),0);
	#else
	assertEqualsInt(sqlrcur_totalRows(cur),8);
	#endif
	printf("\n");


	// first row index
	printf("FIRST ROW INDEX: \n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),0);
	printf("\n");


	// end of result set
	printf("END OF RESULT SET: \n");
	assertTrue(sqlrcur_endOfResultSet(cur));
	printf("\n");


	// fields by index
	printf("FIELDS BY INDEX: \n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"1.1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"testchar1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,3),"testvarchar1");
	printf("\n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,1),"8.8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,2),"testchar8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,3),"testvarchar8");
	printf("\n");


	// field lengths by index
	printf("FIELD LENGTHS BY INDEX: \n");
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,0),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,1),3);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,2),9);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,3),12);
	printf("\n");
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,0),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,1),3);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,2),9);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,3),12);
	printf("\n");


	// fields by name
	printf("FIELDS BY NAME: \n");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testint"),"1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testfloat"),"1.1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testchar"),"testchar1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testvarchar"),"testvarchar1");
	printf("\n");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testint"),"8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testfloat"),"8.8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testchar"),"testchar8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testvarchar"),"testvarchar8");
	printf("\n");


	// field lengths by name
	printf("FIELD LENGTHS BY NAME: \n");
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testint"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testfloat"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testchar"),9);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testvarchar"),12);
	printf("\n");
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testint"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testfloat"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testchar"),9);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testvarchar"),12);
	printf("\n");


	// fields by array
	printf("FIELDS BY ARRAY: \n");
	fields=sqlrcur_getRow(cur,0);
	assertEqualsString(fields[0],"1");
	assertEqualsString(fields[1],"1.1");
	assertEqualsString(fields[2],"testchar1");
	assertEqualsString(fields[3],"testvarchar1");
	printf("\n");


	// field lengths by array
	printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=sqlrcur_getRowLengths(cur,0);
	assertEqualsInt(fieldlens[0],1);
	assertEqualsInt(fieldlens[1],3);
	assertEqualsInt(fieldlens[2],9);
	assertEqualsInt(fieldlens[3],12);
	printf("\n");


	// individual substitutions
	printf("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_sendQuery(cur,"drop table testtable1");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable1 (col1 int, col2 char, col3 float)"));
	sqlrcur_prepareQuery(cur,"insert into testtable1 values ($(var1),'$(var2)',$(var3))");
	sqlrcur_subLong(cur,"var1",1);
	sqlrcur_subString(cur,"var2","hello");
	sqlrcur_subDouble(cur,"var3",10.5556,6,4);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// fields
	printf("FIELDS: \n");
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable1"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"10.5556");
	assertTrue(sqlrcur_sendQuery(cur,"delete from testtable1"));
	printf("\n");


	// array substitutions
	printf("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"insert into testtable1 values ('$(var1)','$(var2)','$(var3)')");
	sqlrcur_subStrings(cur,subvars,subvalstrings);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// fields
	printf("FIELDS: \n");
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable1"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"hi");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"bye");
	assertTrue(sqlrcur_sendQuery(cur,"delete from testtable1"));
	printf("\n");


	// array substitutions
	printf("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"insert into testtable1 values ($(var1),'$(var2)',$(var3))");
	sqlrcur_subLongs(cur,subvars,subvallongs);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// fields
	printf("FIELDS: \n");
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable1"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"2");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"3.0");
	assertTrue(sqlrcur_sendQuery(cur,"delete from testtable1"));
	printf("\n");


	// array substitutions
	printf("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"insert into testtable1 values ($(var1),'$(var2)',$(var3))");
	sqlrcur_subDoubles(cur,subvars,subvaldoubles,precs,scales);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// fields
	printf("FIELDS: \n");
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable1"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"10.55");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"10.556");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"10.5556");
	assertTrue(sqlrcur_sendQuery(cur,"delete from testtable1"));
	printf("\n");


	// nulls as nulls
	printf("NULLS as Nulls: \n");
	sqlrcur_getNullsAsNulls(cur);
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable1 values (1,NULL,NULL)"));
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable1"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),NULL);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable1"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"");
	sqlrcur_getNullsAsNulls(cur);
	printf("\n");


	// result set buffer size
	printf("RESULT SET BUFFER SIZE: \n");
	assertEqualsInt(sqlrcur_getResultSetBufferSize(cur),0);
	sqlrcur_setResultSetBufferSize(cur,2);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
	assertEqualsInt(sqlrcur_getResultSetBufferSize(cur),2);
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),0);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),2);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,1,0),"2");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,2,0),"3");
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),2);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),4);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),6);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),8);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),8);
	assertTrue(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),8);
	printf("\n");


	// dont get column info
	printf("DONT GET COLUMN INFO: \n");
	sqlrcur_dontGetColumnInfo(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
	assertEqualsString(sqlrcur_getColumnName(cur,0),NULL);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,0),0);
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),NULL);
	sqlrcur_getColumnInfo(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
	assertEqualsString(sqlrcur_getColumnName(cur,0),"testint");
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,0),0);
	#ifdef HAVE_SQLITE3_STMT
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),"INTEGER");
	#else
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),"UNKNOWN");
	#endif
	printf("\n");


	// suspended session
	printf("SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	printf("\n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,1,0),"2");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,2,0),"3");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,3,0),"4");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,4,0),"5");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,5,0),"6");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	printf("\n");
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	printf("\n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,1,0),"2");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,2,0),"3");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,3,0),"4");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,4,0),"5");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,5,0),"6");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	printf("\n");
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	printf("\n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,1,0),"2");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,2,0),"3");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,3,0),"4");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,4,0),"5");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,5,0),"6");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	printf("\n");


	// suspended result set
	printf("SUSPENDED RESULT SET: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,2,0),"3");
	id=sqlrcur_getResultSetId(cur);
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	assertTrue(sqlrcur_resumeResultSet(cur,id));
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),4);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),6);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),6);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),8);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),8);
	assertTrue(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),8);
	sqlrcur_setResultSetBufferSize(cur,0);
	printf("\n");


	// cached result set
	printf("CACHED RESULT SET: \n");
	sqlrcur_cacheToFile(cur,"cachefile1");
	sqlrcur_setCacheTtl(cur,200);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqualsString(filename,"cachefile1");
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	free(filename);
	printf("\n");


	// column count for cached result set
	printf("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqualsInt(sqlrcur_colCount(cur),4);
	printf("\n");


	// column names for cached result set
	printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqualsString(sqlrcur_getColumnName(cur,0),"testint");
	assertEqualsString(sqlrcur_getColumnName(cur,1),"testfloat");
	assertEqualsString(sqlrcur_getColumnName(cur,2),"testchar");
	assertEqualsString(sqlrcur_getColumnName(cur,3),"testvarchar");
	cols=sqlrcur_getColumnNames(cur);
	assertEqualsString(cols[0],"testint");
	assertEqualsString(cols[1],"testfloat");
	assertEqualsString(cols[2],"testchar");
	assertEqualsString(cols[3],"testvarchar");
	printf("\n");


	// cached result set with result set buffer size
	printf("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile1");
	sqlrcur_setCacheTtl(cur,200);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqualsString(filename,"cachefile1");
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize(cur,0);
	free(filename);
	printf("\n");


	// from one cache file to another
	printf("FROM ONE CACHE FILE TO ANOTHER: \n");
	sqlrcur_cacheToFile(cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile1"));
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile2"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	printf("\n");


	// from one cache file to another with result set buffer size
	printf("FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile1"));
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile2"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize(cur,0);
	printf("\n");


	// cached result set with suspend and result set buffer size
	printf("CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile1");
	sqlrcur_setCacheTtl(cur,200);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,2,0),"3");
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqualsString(filename,"cachefile1");
	id=sqlrcur_getResultSetId(cur);
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	printf("\n");
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	assertTrue(sqlrcur_resumeCachedResultSet(cur,id,filename));
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),4);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),6);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),6);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),8);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),8);
	assertTrue(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),8);
	sqlrcur_cacheOff(cur);
	printf("\n");
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize(cur,0);
	free(filename);
	printf("\n");


	// commit and rollback
	printf("COMMIT AND ROLLBACK: \n");
	secondcon=sqlrcon_alloc("sqlrelay",9000,
			"/tmp/test.socket","testuser","testpassword",0,1);
	secondcur=sqlrcur_alloc(secondcon);
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqualsString(sqlrcur_getFieldByIndex(secondcur,0,0),"0");
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqualsString(sqlrcur_getFieldByIndex(secondcur,0,0),"8");
	assertTrue(sqlrcur_sendQuery(secondcur,"insert into testtable values (10,10.1,'testchar10','testvarchar10')"));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqualsString(sqlrcur_getFieldByIndex(secondcur,0,0),"9");
	printf("\n");


	// finished suspended session
	printf("FINISHED SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,4,0),"5");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,5,0),"6");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	id=sqlrcur_getResultSetId(cur);
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	assertTrue(sqlrcur_resumeResultSet(cur,id));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,4,0),NULL);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,5,0),NULL);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,6,0),NULL);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),NULL);
	printf("\n");

	// drop existing table
	sqlrcur_sendQuery(cur,"drop table testtable");

	// invalid queries...


	// invalid queries
	printf("INVALID QUERIES: \n");
	assertFalse(sqlrcur_sendQuery(cur,"select * from testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"select * from testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"select * from testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"select * from testtable"));
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,"insert into testtable values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(cur,"insert into testtable values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(cur,"insert into testtable values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(cur,"insert into testtable values (1,2,3,4)"));
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	printf("\n");

	reportTestStatus();

	return status;
}
