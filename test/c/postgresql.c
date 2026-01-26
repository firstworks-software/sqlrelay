// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclientwrapper.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "assert.c"

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

	printf("IDENTIFY: \n");
	assertEqualsString(sqlrcon_identify(con),"postgresql");
	printf("\n");

	// ping
	printf("PING: \n");
	assertTrue(sqlrcon_ping(con));
	printf("\n");

	// isolation levels
	/*printf("ISOLATION LEVELS: \n");
	{
		const char	*isolationlevels[]={"read committed","read uncommitted","repeatable read","serializable",NULL};
		const char	**il;
		for (il=isolationlevels; *il; il++) {
			// postgresql requires the isolation level to
			// be the first query of the transaction
			sqlrcon_begin(con);
			assertTrue(sqlrcon_setIsolationLevel(con,*il));
			assertEqualsString(sqlrcon_getIsolationLevel(con),*il);
			sqlrcon_commit(con);
			printf("\n");
		}
		// reset to the default isolation level
		sqlrcon_begin(con);
		assertTrue(sqlrcon_setIsolationLevel(con,isolationlevels[0]));
		sqlrcon_commit(con);
		printf("\n");
	}*/

	// drop existing table
	sqlrcur_sendQuery(cur,"drop table testtable");

	printf("CREATE TEMPTABLE: \n");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable (testint int, testfloat float, testreal real, testsmallint smallint, testchar char(40), testvarchar varchar(40), testdate date, testtime time, testtimestamp timestamp)"));
	printf("\n");

	printf("BEGIN TRANSCTION: \n");
	assertTrue(sqlrcur_sendQuery(cur,"begin"));
	printf("\n");

	printf("INSERT: \n");
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (1,1.1,1.1,1,'testchar1','testvarchar1','01/01/2001','01:00:00',NULL)"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (2,2.2,2.2,2,'testchar2','testvarchar2','01/01/2002','02:00:00',NULL)"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (3,3.3,3.3,3,'testchar3','testvarchar3','01/01/2003','03:00:00',NULL)"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (4,4.4,4.4,4,'testchar4','testvarchar4','01/01/2004','04:00:00',NULL)"));
	printf("\n");

	printf("AFFECTED ROWS: \n");
	assertEqualsInt(sqlrcur_affectedRows(cur),1);
	printf("\n");

	printf("BIND BY POSITION: \n");
	sqlrcur_prepareQuery(cur,"insert into testtable values ($1,$2,$3,$4,$5,$6,$7,$8)");
	assertEqualsInt(sqlrcur_countBindVariables(cur),8);
	sqlrcur_inputBindLong(cur,"1",5);
	sqlrcur_inputBindDouble(cur,"2",5.5,4,2);
	sqlrcur_inputBindDouble(cur,"3",5.5,4,2);
	sqlrcur_inputBindLong(cur,"4",5);
	sqlrcur_inputBindString(cur,"5","testchar5");
	sqlrcur_inputBindString(cur,"6","testvarchar5");
	sqlrcur_inputBindString(cur,"7","01/01/2005");
	sqlrcur_inputBindString(cur,"8","05:00:00");
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",6);
	sqlrcur_inputBindDouble(cur,"2",6.6,4,2);
	sqlrcur_inputBindDouble(cur,"3",6.6,4,2);
	sqlrcur_inputBindLong(cur,"4",6);
	sqlrcur_inputBindString(cur,"5","testchar6");
	sqlrcur_inputBindString(cur,"6","testvarchar6");
	sqlrcur_inputBindString(cur,"7","01/01/2006");
	sqlrcur_inputBindString(cur,"8","06:00:00");
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",7);
	sqlrcur_inputBindDouble(cur,"2",7.7,4,2);
	sqlrcur_inputBindDouble(cur,"3",7.7,4,2);
	sqlrcur_inputBindLong(cur,"4",7);
	sqlrcur_inputBindString(cur,"5","testchar7");
	sqlrcur_inputBindString(cur,"6","testvarchar7");
	sqlrcur_inputBindString(cur,"7","01/01/2007");
	sqlrcur_inputBindString(cur,"8","07:00:00");
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");

	printf("BIND BY POSITION WITH VALIDATION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",8);
	sqlrcur_inputBindDouble(cur,"2",8.8,4,2);
	sqlrcur_inputBindDouble(cur,"3",8.8,4,2);
	sqlrcur_inputBindLong(cur,"4",8);
	sqlrcur_inputBindString(cur,"5","testchar8");
	sqlrcur_inputBindString(cur,"6","testvarchar8");
	sqlrcur_inputBindString(cur,"7","01/01/2008");
	sqlrcur_inputBindString(cur,"8","08:00:00");
	sqlrcur_validateBinds(cur);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");

	printf("SELECT: \n");
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
	printf("\n");

	printf("COLUMN COUNT: \n");
	assertEqualsInt(sqlrcur_colCount(cur),9);
	printf("\n");

	printf("COLUMN NAMES: \n");
	assertEqualsString(sqlrcur_getColumnName(cur,0),"testint");
	assertEqualsString(sqlrcur_getColumnName(cur,1),"testfloat");
	assertEqualsString(sqlrcur_getColumnName(cur,2),"testreal");
	assertEqualsString(sqlrcur_getColumnName(cur,3),"testsmallint");
	assertEqualsString(sqlrcur_getColumnName(cur,4),"testchar");
	assertEqualsString(sqlrcur_getColumnName(cur,5),"testvarchar");
	assertEqualsString(sqlrcur_getColumnName(cur,6),"testdate");
	assertEqualsString(sqlrcur_getColumnName(cur,7),"testtime");
	assertEqualsString(sqlrcur_getColumnName(cur,8),"testtimestamp");
	cols=sqlrcur_getColumnNames(cur);
	assertEqualsString(cols[0],"testint");
	assertEqualsString(cols[1],"testfloat");
	assertEqualsString(cols[2],"testreal");
	assertEqualsString(cols[3],"testsmallint");
	assertEqualsString(cols[4],"testchar");
	assertEqualsString(cols[5],"testvarchar");
	assertEqualsString(cols[6],"testdate");
	assertEqualsString(cols[7],"testtime");
	assertEqualsString(cols[8],"testtimestamp");
	printf("\n");

	printf("COLUMN TYPES: \n");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),"int4");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testint"),"int4");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,1),"float8");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testfloat"),"float8");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,2),"float4");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testreal"),"float4");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,3),"int2");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testsmallint"),"int2");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,4),"bpchar");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testchar"),"bpchar");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,5),"varchar");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testvarchar"),"varchar");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,6),"date");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testdate"),"date");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,7),"time");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testtime"),"time");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,8),"timestamp");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testtimestamp"),"timestamp");
	printf("\n");

	printf("COLUMN LENGTH: \n");
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,0),4);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testint"),4);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,1),8);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testfloat"),8);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,2),4);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testreal"),4);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,3),2);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testsmallint"),2);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,4),44);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testchar"),44);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,5),44);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testvarchar"),44);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,6),4);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testdate"),4);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,7),8);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testtime"),8);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,8),8);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testtimestamp"),8);
	printf("\n");

	printf("LONGEST COLUMN: \n");
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,0),1);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testint"),1);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,1),3);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testfloat"),3);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,2),3);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testreal"),3);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,3),1);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testsmallint"),1);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,4),40);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testchar"),40);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,5),12);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testvarchar"),12);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,6),10);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testdate"),10);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,7),8);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testtime"),8);
	printf("\n");

	printf("ROW COUNT: \n");
	assertEqualsInt(sqlrcur_rowCount(cur),8);
	printf("\n");

	/*printf("TOTAL ROWS: \n");
	assertEqualsInt(sqlrcur_totalRows(cur),8);
	printf("\n");*/

	printf("FIRST ROW INDEX: \n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),0);
	printf("\n");

	printf("END OF RESULT SET: \n");
	assertTrue(sqlrcur_endOfResultSet(cur));
	printf("\n");

	printf("FIELDS BY INDEX: \n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"1.1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"1.1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,3),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,4),"testchar1                               ");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,5),"testvarchar1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,6),"2001-01-01");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,7),"01:00:00");
	printf("\n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,1),"8.8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,2),"8.8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,3),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,4),"testchar8                               ");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,5),"testvarchar8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,6),"2008-01-01");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,7),"08:00:00");
	printf("\n");

	printf("FIELD LENGTHS BY INDEX: \n");
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,0),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,1),3);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,2),3);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,3),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,4),40);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,5),12);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,6),10);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,7),8);
	printf("\n");
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,0),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,1),3);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,2),3);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,3),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,4),40);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,5),12);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,6),10);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,7),8);
	printf("\n");

	printf("FIELDS BY NAME: \n");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testint"),"1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testfloat"),"1.1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testreal"),"1.1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testsmallint"),"1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testchar"),"testchar1                               ");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testvarchar"),"testvarchar1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testdate"),"2001-01-01");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testtime"),"01:00:00");
	printf("\n");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testint"),"8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testfloat"),"8.8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testreal"),"8.8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testsmallint"),"8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testchar"),"testchar8                               ");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testvarchar"),"testvarchar8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testdate"),"2008-01-01");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testtime"),"08:00:00");
	printf("\n");

	printf("FIELD LENGTHS BY NAME: \n");
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testint"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testfloat"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testreal"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testsmallint"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testchar"),40);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testvarchar"),12);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testdate"),10);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testtime"),8);
	printf("\n");
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testint"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testfloat"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testreal"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testsmallint"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testchar"),40);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testvarchar"),12);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testdate"),10);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testtime"),8);
	printf("\n");

	printf("FIELDS BY ARRAY: \n");
	fields=sqlrcur_getRow(cur,0);
	assertEqualsString(fields[0],"1");
	assertEqualsString(fields[1],"1.1");
	assertEqualsString(fields[2],"1.1");
	assertEqualsString(fields[3],"1");
	assertEqualsString(fields[4],"testchar1                               ");
	assertEqualsString(fields[5],"testvarchar1");
	assertEqualsString(fields[6],"2001-01-01");
	assertEqualsString(fields[7],"01:00:00");
	printf("\n");

	printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=sqlrcur_getRowLengths(cur,0);
	assertEqualsInt(fieldlens[0],1);
	assertEqualsInt(fieldlens[1],3);
	assertEqualsInt(fieldlens[2],3);
	assertEqualsInt(fieldlens[3],1);
	assertEqualsInt(fieldlens[4],40);
	assertEqualsInt(fieldlens[5],12);
	assertEqualsInt(fieldlens[6],10);
	assertEqualsInt(fieldlens[7],8);
	printf("\n");

	printf("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"select $(var1),'$(var2)',$(var3)");
	sqlrcur_subLong(cur,"var1",1);
	sqlrcur_subString(cur,"var2","hello");
	sqlrcur_subDouble(cur,"var3",10.5556,6,4);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");

	printf("FIELDS: \n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"10.5556");
	printf("\n");

	printf("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"select $(var1),$(var2),$(var3)");
	sqlrcur_subLongs(cur,subvars,subvallongs);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");
	
	printf("FIELDS: \n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"2");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"3");
	printf("\n");
	
	printf("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"select '$(var1)','$(var2)','$(var3)'");
	sqlrcur_subStrings(cur,subvars,subvalstrings);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");

	printf("FIELDS: \n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"hi");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"bye");
	printf("\n");

	printf("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"select $(var1),$(var2),$(var3)");
	sqlrcur_subDoubles(cur,subvars,subvaldoubles,precs,scales);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");

	printf("FIELDS: \n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"10.55");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"10.556");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"10.5556");
	printf("\n");

	printf("NULLS as Nulls: \n");
	sqlrcur_getNullsAsNulls(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select NULL,1,NULL"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),NULL);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select NULL,1,NULL"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"");
	sqlrcur_getNullsAsNulls(cur);
	printf("\n");

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

	printf("DONT GET COLUMN INFO: \n");
	sqlrcur_dontGetColumnInfo(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
	assertEqualsString(sqlrcur_getColumnName(cur,0),NULL);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,0),0);
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),NULL);
	sqlrcur_getColumnInfo(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
	assertEqualsString(sqlrcur_getColumnName(cur,0),"testint");
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,0),4);
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),"int4");
	printf("\n");

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

	printf("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqualsInt(sqlrcur_colCount(cur),9);
	printf("\n");

	printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqualsString(sqlrcur_getColumnName(cur,0),"testint");
	assertEqualsString(sqlrcur_getColumnName(cur,1),"testfloat");
	assertEqualsString(sqlrcur_getColumnName(cur,2),"testreal");
	assertEqualsString(sqlrcur_getColumnName(cur,3),"testsmallint");
	assertEqualsString(sqlrcur_getColumnName(cur,4),"testchar");
	assertEqualsString(sqlrcur_getColumnName(cur,5),"testvarchar");
	assertEqualsString(sqlrcur_getColumnName(cur,6),"testdate");
	assertEqualsString(sqlrcur_getColumnName(cur,7),"testtime");
	assertEqualsString(sqlrcur_getColumnName(cur,8),"testtimestamp");
	cols=sqlrcur_getColumnNames(cur);
	assertEqualsString(cols[0],"testint");
	assertEqualsString(cols[1],"testfloat");
	assertEqualsString(cols[2],"testreal");
	assertEqualsString(cols[3],"testsmallint");
	assertEqualsString(cols[4],"testchar");
	assertEqualsString(cols[5],"testvarchar");
	assertEqualsString(cols[6],"testdate");
	assertEqualsString(cols[7],"testtime");
	assertEqualsString(cols[8],"testtimestamp");
	printf("\n");

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

	printf("FROM ONE CACHE FILE TO ANOTHER: \n");
	sqlrcur_cacheToFile(cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile1"));
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile2"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	printf("\n");

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

	printf("COMMIT AND ROLLBACK: \n");
	secondcon=sqlrcon_alloc("sqlrelay",9000,
			"/tmp/test.socket","testuser","testpassword",0,1);
	secondcur=sqlrcur_alloc(secondcon);
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqualsString(sqlrcur_getFieldByIndex(secondcur,0,0),"0");
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqualsString(sqlrcur_getFieldByIndex(secondcur,0,0),"8");
	//assertTrue(sqlrcon_autoCommitOn(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (10,10.1,10.1,10,'testchar10','testvarchar10','01/01/2010','10:00:00',NULL)"));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqualsString(sqlrcur_getFieldByIndex(secondcur,0,0),"9");
	//assertTrue(sqlrcon_autoCommitOff(con));
	printf("\n");

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

	// temporary tables
	printf("TEMPORARY TABLES: \n");
	sqlrcur_sendQuery(cur,"drop table temptable\n");
	sqlrcur_sendQuery(cur,"create temporary table temptable (col1 int)");
	assertTrue(sqlrcur_sendQuery(cur,"insert into temptable values (1)"));
	assertTrue(sqlrcur_sendQuery(cur,"select count(*) from temptable"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	sqlrcon_endSession(con);
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,"select count(*) from temptable"));
	sqlrcur_sendQuery(cur,"drop table temptable\n");
	printf("\n");

	// stored procedures
	printf("STORED PROCEDURES: \n");
	// return no values
	sqlrcur_sendQuery(cur,"drop function testfunc(int,float,char(20))");
	assertTrue(sqlrcur_sendQuery(cur,"create function testfunc(int,float,char(20)) returns void as ' declare in1 int; in2 float; in3 char(20); begin in1:=$1; in2:=$2; in3:=$3; return; end;' language plpgsql"));
	sqlrcur_prepareQuery(cur,"select testfunc($1,$2,$3)");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",1.1,4,2);
	sqlrcur_inputBindString(cur,"3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_sendQuery(cur,"drop function testfunc(int,float,char(20))");
	printf("\n");
	// return single value
	sqlrcur_sendQuery(cur,"drop function testfunc(int,float,char(20))");
	assertTrue(sqlrcur_sendQuery(cur,"create function testfunc(int,float,char(20)) returns int as ' begin return $1; end;' language plpgsql"));
	sqlrcur_prepareQuery(cur,"select * from testfunc($1,$2,$3)");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",1.1,4,2);
	sqlrcur_inputBindString(cur,"3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	sqlrcur_sendQuery(cur,"drop function testfunc(int,float,char(20))");
	printf("\n");
	// return multiple values
	sqlrcur_sendQuery(cur,"drop function testfunc(int,char(20))");
	assertTrue(sqlrcur_sendQuery(cur,"create function testfunc(int,float,char(20)) returns record as ' declare output record; begin select $1,$2,$3 into output; return output; end;' language plpgsql"));
	sqlrcur_prepareQuery(cur,"select * from testfunc($1,$2,$3) as (col1 int, col2 float, col3 bpchar)");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",1.1,4,2);
	sqlrcur_inputBindString(cur,"3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	//assertEqualsInt(atof(sqlrcur_getFieldByIndex(cur,0,1)),1.1);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"hello");
	sqlrcur_sendQuery(cur,"drop function testfunc(int,float,char(20))");
	printf("\n");
	// return result set
	sqlrcur_sendQuery(cur,"drop function testfunc()");
	assertTrue(sqlrcur_sendQuery(cur,"create function testfunc() returns setof record as ' declare output record; begin for output in select * from testtable loop return next output; end loop; return; end;' language plpgsql"));
	assertTrue(sqlrcur_sendQuery(cur,"select * from testfunc() as (testint int, testfloat float, testreal real, testsmallint smallint, testchar char(40), testvarchar varchar(40), testdate date, testtime time, testtimestamp timestamp)"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,4,0),"5");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,5,0),"6");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	sqlrcur_sendQuery(cur,"drop function testfunc()");
	printf("\n");

	// drop existing table
	sqlrcur_sendQuery(cur,"drop table testtable");

	// invalid queries...
	printf("INVALID QUERIES: \n");
	assertFalse(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
	assertFalse(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
	assertFalse(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
	assertFalse(sqlrcur_sendQuery(cur,"select * from testtable order by testint"));
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

	return 0;
}
