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

	const char	*isolationlevels[]={"CS","UR","RS","RR",NULL};
	const char	**il;
	const char	*bindvars[]={"1","2","3","4","5","6",
				"7","8","9","10","11","12",NULL};
	const char	*bindvals[]={"7","7","7","7.5","7.5","7.5",
				"testchar7","testvarchar7",
				"01/01/2007","07:00:00","testclob7",NULL};
	const char * const *cols;
	const char * const *fields;
	uint32_t	*fieldlens;
	const char	*subvars[]={"var1","var2","var3",NULL};
	int64_t		subvallongs[]={1,2,3};
	const char	*subvalstrings[]={"hi","hello","bye"};
	double		subvaldoubles[]={10.55,10.556,10.5556};
	uint32_t	precs[]={4,5,6};
	uint32_t	scales[]={2,3,4};
	int64_t		numvar;
	const char	*stringvar;
	const char	*nullvar;
	double		floatvar;
	int16_t		year=0;
	int16_t		month=0;
	int16_t		day=0;
	int16_t		hour=0;
	int16_t		minute=0;
	int16_t		second=0;
	int32_t		microsecond=0;
	const char	*tz=NULL;
	int		isnegative=0;
	const char	*clobvar;
	uint32_t	clobvarlength;
	const char	*blobvar;
	uint32_t	blobvarlength;
	uint16_t	port;
	char		*socket;
	uint16_t	id;
	char		*filename;
	uint64_t	i;
	unsigned char	buffer[256];
	uint16_t	j;
	char		querystr[1024];
	char		hex[3];

	#define	LARGE_BUFFER_LENGTH	(20*1024)
	char		largebuffer[LARGE_BUFFER_LENGTH+1];


	// instantiation
	con=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
				"db2inst1","testpassword",0,1);
	cur=sqlrcur_alloc(con);


	// identify
	printf("IDENTIFY: \n");
	assertEqStr(sqlrcon_identify(con),"db2");
	printf("\n");


	// ping
	printf("PING: \n");
	assertTrue(sqlrcon_ping(con));
	printf("\n");


	// transaction state
	printf("TRANSACTION STATE: \n");
	assertEqStr(sqlrcon_getDefaultTransactionModel(con),"implicit");
	assertEqStr(sqlrcon_getTransactionModel(con),"implicit");
	assertTrue(sqlrcon_getInTransaction(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	printf("\n");


	// bind format
	printf("BIND FORMAT: \n");
	assertEqStr(sqlrcon_bindFormat(con),"?");
	printf("\n");


	// nextval format
	printf("NEXTVAL FORMAT: \n");
	assertEqStr(sqlrcon_nextvalFormat(con),"(nextval for %s)");
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
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testsmallint smallint, "
		"	testint integer, "
		"	testbigint bigint, "
		"	testdecimal decimal(10,2), "
		"	testreal real, "
		"	testdouble double, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testdate date, "
		"	testtime time, "
		"	testtimestamp timestamp, "
		"	testclob clob, "
		"	testblob blob)"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// insert
	printf("INSERT: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	1, "
		"	1, "
		"	1.5, "
		"	1.5, "
		"	1.5, "
		"	'testchar1', "
		"	'testvarchar1', "
		"	'01/01/2001', "
		"	'01:00:00', "
		"	NULL, "
		"	'testclob1', "
		"	blob('testblob1'))"));
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
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	NULL, "
		"	?, "
		"	?)");
	assertEqInt(sqlrcur_countBindVariables(cur),12);
	sqlrcur_inputBindLong(cur,"1",2);
	sqlrcur_inputBindLong(cur,"2",2);
	sqlrcur_inputBindLong(cur,"3",2);
	sqlrcur_inputBindDouble(cur,"4",2.5,4,2);
	sqlrcur_inputBindDouble(cur,"5",2.5,4,2);
	sqlrcur_inputBindDouble(cur,"6",2.5,4,2);
	sqlrcur_inputBindString(cur,"7","testchar2");
	sqlrcur_inputBindString(cur,"8","testvarchar2");
	sqlrcur_inputBindDate(cur,"9",2002,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate(cur,"10",-1,-1,-1,2,0,0,0,NULL,0);
	sqlrcur_inputBindClob(cur,"11","testclob2",9);
	sqlrcur_inputBindBlob(cur,"12","testblob2",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",3);
	sqlrcur_inputBindLong(cur,"2",3);
	sqlrcur_inputBindLong(cur,"3",3);
	sqlrcur_inputBindDouble(cur,"4",3.5,4,2);
	sqlrcur_inputBindDouble(cur,"5",3.5,4,2);
	sqlrcur_inputBindDouble(cur,"6",3.5,4,2);
	sqlrcur_inputBindString(cur,"7","testchar3");
	sqlrcur_inputBindString(cur,"8","testvarchar3");
	sqlrcur_inputBindDate(cur,"9",2003,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate(cur,"10",-1,-1,-1,3,0,0,0,NULL,0);
	sqlrcur_inputBindClob(cur,"11","testclob3",9);
	sqlrcur_inputBindBlob(cur,"12","testblob3",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",4);
	sqlrcur_inputBindLong(cur,"2",4);
	sqlrcur_inputBindLong(cur,"3",4);
	sqlrcur_inputBindDouble(cur,"4",4.5,4,2);
	sqlrcur_inputBindDouble(cur,"5",4.5,4,2);
	sqlrcur_inputBindDouble(cur,"6",4.5,4,2);
	sqlrcur_inputBindString(cur,"7","testchar4");
	sqlrcur_inputBindString(cur,"8","testvarchar4");
	sqlrcur_inputBindDate(cur,"9",2004,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate(cur,"10",-1,-1,-1,4,0,0,0,NULL,0);
	sqlrcur_inputBindClob(cur,"11","testclob4",9);
	sqlrcur_inputBindBlob(cur,"12","testblob4",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",5);
	sqlrcur_inputBindLong(cur,"2",5);
	sqlrcur_inputBindLong(cur,"3",5);
	sqlrcur_inputBindDouble(cur,"4",5.5,4,2);
	sqlrcur_inputBindDouble(cur,"5",5.5,4,2);
	sqlrcur_inputBindDouble(cur,"6",5.5,4,2);
	sqlrcur_inputBindString(cur,"7","testchar5");
	sqlrcur_inputBindString(cur,"8","testvarchar5");
	sqlrcur_inputBindDate(cur,"9",2005,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate(cur,"10",-1,-1,-1,5,0,0,0,NULL,0);
	sqlrcur_inputBindClob(cur,"11","testclob5",9);
	sqlrcur_inputBindBlob(cur,"12","testblob5",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",6);
	sqlrcur_inputBindLong(cur,"2",6);
	sqlrcur_inputBindLong(cur,"3",6);
	sqlrcur_inputBindDouble(cur,"4",6.5,4,2);
	sqlrcur_inputBindDouble(cur,"5",6.5,4,2);
	sqlrcur_inputBindDouble(cur,"6",6.5,4,2);
	sqlrcur_inputBindString(cur,"7","testchar6");
	sqlrcur_inputBindString(cur,"8","testvarchar6");
	sqlrcur_inputBindDate(cur,"9",2006,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate(cur,"10",-1,-1,-1,6,0,0,0,NULL,0);
	sqlrcur_inputBindClob(cur,"11","testclob6",9);
	sqlrcur_inputBindBlob(cur,"12","testblob6",9);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// array of input binds by position
	printf("ARRAY OF INPUT BINDS ""BY POSITION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindStrings(cur,bindvars,bindvals);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// input bind by position with validation
	printf("INPUT BIND BY POSITION ""WITH VALIDATION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",8);
	sqlrcur_inputBindLong(cur,"2",8);
	sqlrcur_inputBindLong(cur,"3",8);
	sqlrcur_inputBindDouble(cur,"4",8.5,4,2);
	sqlrcur_inputBindDouble(cur,"5",8.5,4,2);
	sqlrcur_inputBindDouble(cur,"6",8.5,4,2);
	sqlrcur_inputBindString(cur,"7","testchar8");
	sqlrcur_inputBindString(cur,"8","testvarchar8");
	sqlrcur_inputBindDate(cur,"9",2008,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate(cur,"10",-1,-1,-1,8,0,0,0,NULL,0);
	sqlrcur_inputBindClob(cur,"11","testclob8",9);
	sqlrcur_inputBindBlob(cur,"12","testblob8",9);
	sqlrcur_validateBinds(cur);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");

	// input bind by name
	// db2 doesn't support bind by name


	// array of input binds by name
	// db2 doesn't support bind by name


	// input bind by name with validation
	// db2 doesn't support bind by name


	// select
	printf("SELECT: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	printf("\n");


	// column count
	printf("COLUMN COUNT: \n");
	assertEqInt(sqlrcur_colCount(cur),13);
	printf("\n");


	// column names
	printf("COLUMN NAMES: \n");
	assertEqStr(sqlrcur_getColumnName(cur,0),"TESTSMALLINT");
	assertEqStr(sqlrcur_getColumnName(cur,1),"TESTINT");
	assertEqStr(sqlrcur_getColumnName(cur,2),"TESTBIGINT");
	assertEqStr(sqlrcur_getColumnName(cur,3),"TESTDECIMAL");
	assertEqStr(sqlrcur_getColumnName(cur,4),"TESTREAL");
	assertEqStr(sqlrcur_getColumnName(cur,5),"TESTDOUBLE");
	assertEqStr(sqlrcur_getColumnName(cur,6),"TESTCHAR");
	assertEqStr(sqlrcur_getColumnName(cur,7),"TESTVARCHAR");
	assertEqStr(sqlrcur_getColumnName(cur,8),"TESTDATE");
	assertEqStr(sqlrcur_getColumnName(cur,9),"TESTTIME");
	assertEqStr(sqlrcur_getColumnName(cur,10),"TESTTIMESTAMP");
	cols=sqlrcur_getColumnNames(cur);
	assertEqStr(cols[0],"TESTSMALLINT");
	assertEqStr(cols[1],"TESTINT");
	assertEqStr(cols[2],"TESTBIGINT");
	assertEqStr(cols[3],"TESTDECIMAL");
	assertEqStr(cols[4],"TESTREAL");
	assertEqStr(cols[5],"TESTDOUBLE");
	assertEqStr(cols[6],"TESTCHAR");
	assertEqStr(cols[7],"TESTVARCHAR");
	assertEqStr(cols[8],"TESTDATE");
	assertEqStr(cols[9],"TESTTIME");
	assertEqStr(cols[10],"TESTTIMESTAMP");
	printf("\n");


	// column types
	printf("COLUMN TYPES: \n");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),"SMALLINT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTSMALLINT"),"SMALLINT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,1),"INTEGER");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTINT"),"INTEGER");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,2),"BIGINT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTBIGINT"),"BIGINT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,3),"DECIMAL");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTDECIMAL"),"DECIMAL");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,4),"REAL");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTREAL"),"REAL");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,5),"DOUBLE");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTDOUBLE"),"DOUBLE");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,6),"CHAR");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTCHAR"),"CHAR");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,7),"VARCHAR");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTVARCHAR"),"VARCHAR");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,8),"DATE");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTDATE"),"DATE");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,9),"TIME");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTTIME"),"TIME");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,10),"TIMESTAMP");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTTIMESTAMP"),
		"TIMESTAMP");
	printf("\n");


	// column length
	printf("COLUMN LENGTH: \n");
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),2);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTSMALLINT"),2);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,1),4);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTINT"),4);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,2),8);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTBIGINT"),8);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,3),12);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTDECIMAL"),12);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,4),4);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTREAL"),4);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,5),8);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTDOUBLE"),8);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,6),40);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTCHAR"),40);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,7),40);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTVARCHAR"),40);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,8),6);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTDATE"),6);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,9),6);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTTIME"),6);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,10),16);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTTIMESTAMP"),16);
	printf("\n");


	// longest column
	printf("LONGEST COLUMN: \n");
	assertEqInt(sqlrcur_getLongestByIndex(cur,0),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTSMALLINT"),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,1),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTINT"),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,2),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTBIGINT"),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,3),4);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTDECIMAL"),4);
	assertEqInt(sqlrcur_getLongestByIndex(cur,4),12);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTREAL"),12);
	assertEqInt(sqlrcur_getLongestByIndex(cur,5),21);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTDOUBLE"),21);
	assertEqInt(sqlrcur_getLongestByIndex(cur,6),40);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTCHAR"),40);
	assertEqInt(sqlrcur_getLongestByIndex(cur,7),12);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTVARCHAR"),12);
	assertEqInt(sqlrcur_getLongestByIndex(cur,8),10);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTDATE"),10);
	assertEqInt(sqlrcur_getLongestByIndex(cur,9),8);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTTIME"),8);
	printf("\n");


	// row count
	printf("ROW COUNT: \n");
	assertEqInt(sqlrcur_rowCount(cur),8);
	printf("\n");


	// total rows
	printf("TOTAL ROWS: \n");
	assertEqInt(sqlrcur_totalRows(cur),0);
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
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,3),"1.50");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,4),"1.500000E+00");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,5),"1.50000000000000E+000");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,6),"testchar1"
					"                               ");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,7),"testvarchar1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,8),"2001-01-01");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,9),"01:00:00");
	printf("\n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,2),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,3),"8.50");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,4),"8.500000E+00");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,5),"8.50000000000000E+000");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,6),"testchar8"
					"                               ");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,7),"testvarchar8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,8),"2008-01-01");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,9),"08:00:00");
	printf("\n");


	// field lengths by index
	printf("FIELD LENGTHS BY INDEX: \n");
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,1),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,2),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,3),4);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,4),12);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,5),21);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,6),40);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,7),12);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,8),10);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,9),8);
	printf("\n");
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,0),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,1),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,2),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,3),4);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,4),12);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,5),21);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,6),40);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,7),12);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,8),10);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,9),8);
	printf("\n");


	// fields by name
	printf("FIELDS BY NAME: \n");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTSMALLINT"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTINT"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTBIGINT"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTDECIMAL"),"1.50");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTREAL"),"1.500000E+00");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTDOUBLE"),"1.50000000000000E+000");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTCHAR"),"testchar1"
					"                               ");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTVARCHAR"),"testvarchar1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTDATE"),"2001-01-01");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTTIME"),"01:00:00");
	printf("\n");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTSMALLINT"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTINT"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTBIGINT"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTDECIMAL"),"8.50");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTREAL"),"8.500000E+00");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTDOUBLE"),"8.50000000000000E+000");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTCHAR"),"testchar8"
					"                               ");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTVARCHAR"),"testvarchar8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTDATE"),"2008-01-01");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTTIME"),"08:00:00");
	printf("\n");


	// field lengths by name
	printf("FIELD LENGTHS BY NAME: \n");
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTSMALLINT"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTINT"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTBIGINT"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTDECIMAL"),4);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTREAL"),12);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTDOUBLE"),21);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTCHAR"),40);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTVARCHAR"),12);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTDATE"),10);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTTIME"),8);
	printf("\n");
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTSMALLINT"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTINT"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTBIGINT"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTDECIMAL"),4);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTREAL"),12);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTDOUBLE"),21);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTCHAR"),40);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTVARCHAR"),12);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTDATE"),10);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTTIME"),8);
	printf("\n");


	// fields by array
	printf("FIELDS BY ARRAY: \n");
	fields=sqlrcur_getRow(cur,0);
	assertEqStr(fields[0],"1");
	assertEqStr(fields[1],"1");
	assertEqStr(fields[2],"1");
	assertEqStr(fields[3],"1.50");
	assertEqStr(fields[4],"1.500000E+00");
	assertEqStr(fields[5],"1.50000000000000E+000");
	assertEqStr(fields[6],"testchar1""                               ");
	assertEqStr(fields[7],"testvarchar1");
	assertEqStr(fields[8],"2001-01-01");
	assertEqStr(fields[9],"01:00:00");
	printf("\n");


	// field lengths by array
	printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=sqlrcur_getRowLengths(cur,0);
	assertEqInt(fieldlens[0],1);
	assertEqInt(fieldlens[1],1);
	assertEqInt(fieldlens[2],1);
	assertEqInt(fieldlens[3],4);
	assertEqInt(fieldlens[4],12);
	assertEqInt(fieldlens[5],21);
	assertEqInt(fieldlens[6],40);
	assertEqInt(fieldlens[7],12);
	assertEqInt(fieldlens[8],10);
	assertEqInt(fieldlens[9],8);
	printf("\n");


	// result set buffer size
	printf("RESULT SET BUFFER SIZE: \n");
	assertEqInt(sqlrcur_getResultSetBufferSize(cur),0);
	sqlrcur_setResultSetBufferSize(cur,2);
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
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
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	assertEqStr(sqlrcur_getColumnName(cur,0),NULL);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),0);
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),NULL);
	sqlrcur_getColumnInfo(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	assertEqStr(sqlrcur_getColumnName(cur,0),"TESTSMALLINT");
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),2);
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),"SMALLINT");
	printf("\n");


	// suspended session
	printf("SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	free(socket);
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
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	free(socket);
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
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	free(socket);
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
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	assertEqStr(sqlrcur_getFieldByIndex(cur,2,0),"3");
	id=sqlrcur_getResultSetId(cur);
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	free(socket);
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
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqStr(filename,"cachefile1");
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	free(filename);
	printf("\n");


	// column count for cached result set
	printf("COLUMN COUNT FOR ""CACHED RESULT SET: \n");
	assertEqInt(sqlrcur_colCount(cur),13);
	printf("\n");


	// column names for cached result set
	printf("COLUMN NAMES FOR ""CACHED RESULT SET: \n");
	assertEqStr(sqlrcur_getColumnName(cur,0),"TESTSMALLINT");
	assertEqStr(sqlrcur_getColumnName(cur,1),"TESTINT");
	assertEqStr(sqlrcur_getColumnName(cur,2),"TESTBIGINT");
	assertEqStr(sqlrcur_getColumnName(cur,3),"TESTDECIMAL");
	assertEqStr(sqlrcur_getColumnName(cur,4),"TESTREAL");
	assertEqStr(sqlrcur_getColumnName(cur,5),"TESTDOUBLE");
	assertEqStr(sqlrcur_getColumnName(cur,6),"TESTCHAR");
	assertEqStr(sqlrcur_getColumnName(cur,7),"TESTVARCHAR");
	assertEqStr(sqlrcur_getColumnName(cur,8),"TESTDATE");
	assertEqStr(sqlrcur_getColumnName(cur,9),"TESTTIME");
	assertEqStr(sqlrcur_getColumnName(cur,10),"TESTTIMESTAMP");
	cols=sqlrcur_getColumnNames(cur);
	assertEqStr(cols[0],"TESTSMALLINT");
	assertEqStr(cols[1],"TESTINT");
	assertEqStr(cols[2],"TESTBIGINT");
	assertEqStr(cols[3],"TESTDECIMAL");
	assertEqStr(cols[4],"TESTREAL");
	assertEqStr(cols[5],"TESTDOUBLE");
	assertEqStr(cols[6],"TESTCHAR");
	assertEqStr(cols[7],"TESTVARCHAR");
	assertEqStr(cols[8],"TESTDATE");
	assertEqStr(cols[9],"TESTTIME");
	assertEqStr(cols[10],"TESTTIMESTAMP");
	printf("\n");


	// cached result set with result set
	// buffer size
	printf("CACHED RESULT SET ""WITH RESULT SET ""BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile1");
	sqlrcur_setCacheTtl(cur,200);
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
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
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
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
	free(socket);
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
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testint"));
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
	free(socket);
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
	assertEqStr(sqlrcon_getTransactionModel(con),"implicit");
	assertFalse(sqlrcon_getAutoCommit(con));
	printf("\n");


	// transaction behavior - implicit
	printf("TRANSACTION BEHAVIOR - implicit: \n");
	assertTrue(sqlrcon_setTransactionModel(con,"implicit"));
	assertEqStr(sqlrcon_getTransactionModel(con),"implicit");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable (col1 integer)"));
	// db2 DDL is transactional; commit so the table is visible to the
	// second connection (the commit implicitly starts a new tx)
	assertTrue(sqlrcon_commit(con));
	secondcon=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
						"db2inst1","testpassword",0,1);
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
	assertEqStr(sqlrcon_getTransactionModel(con),"implicit");
	assertFalse(sqlrcon_getAutoCommit(con));
	printf("\n");


	// individual substitutions
	printf("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"values ""($(var1),'$(var2)','$(var3)')");
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
	sqlrcur_prepareQuery(cur,"values ""('$(var1)','$(var2)','$(var3)')");
	sqlrcur_subStrings(cur,subvars,subvalstrings);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"hi");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"bye");
	printf("\n");
	sqlrcur_prepareQuery(cur,"values ""($(var1),$(var2),$(var3))");
	sqlrcur_subLongs(cur,subvars,subvallongs);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"2");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"3");
	printf("\n");
	sqlrcur_prepareQuery(cur,"values ""($(var1),$(var2),$(var3))");
	sqlrcur_subDoubles(cur,subvars,subvaldoubles,precs,scales);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"10.55");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"10.556");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"10.5556");
	printf("\n");


	// nulls as nulls
	printf("NULLS AS NULLS: \n");
	sqlrcur_getNullsAsNulls(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select NULL,1,NULL "
		"from sysibm.sysdummy1"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select NULL,1,NULL "
		"from sysibm.sysdummy1"));
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
		"	testclob1 clob, "
		"	testclob2 clob, "
		"	testblob1 blob, "
		"	testblob2 blob)"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	?, "
		"	?, "
		"	?, "
		"	?)");
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
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// long lobs
	printf("LONG LOBS: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testclob clob, "
		"	testblob blob)"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,"insert into testtable ""values (?,?)");
	for (i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	sqlrcur_inputBindClob(cur,"1",largebuffer,LARGE_BUFFER_LENGTH);
	sqlrcur_inputBindBlob(cur,"2",largebuffer,LARGE_BUFFER_LENGTH);
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable"));
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTCLOB"),
		LARGE_BUFFER_LENGTH);
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTCLOB"),largebuffer);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTBLOB"),
		LARGE_BUFFER_LENGTH);
	assertEqStrLen(sqlrcur_getFieldByName(cur,0,"TESTBLOB"),largebuffer,
		LARGE_BUFFER_LENGTH);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// output bind by position
	printf("OUTPUT BIND BY POSITION: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	sqlrcur_getNullsAsNulls(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc("
		"	out out1 int, "
		"	out out2 varchar(20), "
		"	out out3 double, "
		"	out out4 date, "
		"	out out5 varchar(20)) ""language sql "
		"begin "
		"	set out1 = 1; "
		"	set out2 = 'hello'; "
		"	set out3 = 2.5; "
		"	set out4 = '2001-02-03'; "
		"	set out5 = null; ""end"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,"call testproc(?,?,?,?,?)");
	assertEqInt(sqlrcur_countBindVariables(cur),5);
	sqlrcur_defineOutputBindInteger(cur,"1");
	sqlrcur_defineOutputBindString(cur,"2",20);
	sqlrcur_defineOutputBindDouble(cur,"3");
	sqlrcur_defineOutputBindDate(cur,"4");
	sqlrcur_defineOutputBindString(cur,"5",20);
	assertTrue(sqlrcur_executeQuery(cur));
	numvar=sqlrcur_getOutputBindInteger(cur,"1");
	stringvar=sqlrcur_getOutputBindString(cur,"2");
	floatvar=sqlrcur_getOutputBindDouble(cur,"3");
	sqlrcur_getOutputBindDate(cur,"4",&year,&month,&day,
		&hour,&minute,&second,&microsecond,&tz,&isnegative);
	assertEqInt(numvar,1);
	assertEqStr(stringvar,"hello");
	assertEqDbl(floatvar,2.5);
	assertEqInt(year,2001);
	assertEqInt(month,2);
	assertEqInt(day,3);
	assertEqInt(hour,0);
	assertEqInt(minute,0);
	assertEqInt(second,0);
	assertEqInt(microsecond,0);
	assertEqStr(tz,"");
	assertFalse(isnegative);
	nullvar=sqlrcur_getOutputBindString(cur,"5");
	assertEqStr(nullvar,NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// output bind by name
	// db2 doesn't support bind by name


	// output bind by name with validation
	// db2 doesn't support bind by name


	// lob output bind
	printf("LOB OUTPUT BIND: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testclob clob, "
		"	testblob blob)"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,"insert into testtable ""values ('hello',?)");
	sqlrcur_inputBindBlob(cur,"1","hello",5);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc("
		"	out out1 clob, "
		"	out out2 blob) ""language sql "
		"begin "
		"	select testclob into out1 "
		"		from testtable; "
		"	select testblob into out2 "
		"		from testtable; ""end"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,"call testproc(?,?)");
	sqlrcur_defineOutputBindClob(cur,"1");
	sqlrcur_defineOutputBindBlob(cur,"2");
	assertTrue(sqlrcur_executeQuery(cur));
	clobvar=sqlrcur_getOutputBindClob(cur,"1");
	clobvarlength=sqlrcur_getOutputBindLength(cur,"1");
	blobvar=sqlrcur_getOutputBindBlob(cur,"2");
	blobvarlength=sqlrcur_getOutputBindLength(cur,"2");
	assertEqStrLen(clobvar,"hello",5);
	assertEqInt(clobvarlength,5);
	assertEqStrLen(blobvar,"hello",5);
	assertEqInt(blobvarlength,5);
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// long output bind
	printf("LONG OUTPUT BIND: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc("
		"	in in1 clob, "
		"	out out1 clob) ""language sql "
		"begin "
		"	set out1 = in1; ""end"));
	assertTrue(sqlrcon_commit(con));
	for (i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	sqlrcur_prepareQuery(cur,"call testproc(?,?)");
	sqlrcur_inputBindClob(cur,"1",largebuffer,LARGE_BUFFER_LENGTH);
	sqlrcur_defineOutputBindClob(cur,"2");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindLength(cur,"2"),LARGE_BUFFER_LENGTH);
	assertEqStr(sqlrcur_getOutputBindClob(cur,"2"),largebuffer);
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// negative input bind
	printf("NEGATIVE INPUT BIND: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable ""(testval integer)"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,"insert into testtable ""values (?)");
	sqlrcur_inputBindLong(cur,"1",-1);
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"select testval from testtable"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTVAL"),"-1");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// bind validation
	// db2 doesn't support bind by name


	// rebinding
	printf("REBINDING: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc("
		"	in in1 int, "
		"	out out1 int) ""language sql "
		"begin "
		"	set out1 = in1; ""end"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,"call testproc(?,?)");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_defineOutputBindInteger(cur,"2");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"2"),1);
	sqlrcur_inputBindLong(cur,"1",2);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"2"),2);
	sqlrcur_inputBindLong(cur,"1",3);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"2"),3);
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// reexecute
	printf("REEXECUTE: \n");
	sqlrcur_prepareQuery(cur,"select 1 from sysibm.sysdummy1");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	printf("\n");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	printf("\n");
	sqlrcur_prepareQuery(cur,"select cast(? as integer) "
		"from sysibm.sysdummy1");
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
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc("
		"	in in1 int, "
		"	in in2 double, "
		"	in in3 varchar(20)) ""language sql "
		"begin "
		"	return; ""end"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,"call testproc(?,?,?)");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",2.5,2,1);
	sqlrcur_inputBindString(cur,"3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// stored procedure returning single value
	printf("STORED PROCEDURE ""RETURNING SINGLE VALUE: \n");
	sqlrcur_sendQuery(cur,"drop function testfunc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create function testfunc("
		"	in1 int, "
		"	in2 double, "
		"	in3 varchar(20)) ""returns int ""language sql "
		"begin "
		"	return in1; ""end"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,"select testfunc(?,?,?) "
		"from sysibm.sysdummy1");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",2.5,2,1);
	sqlrcur_inputBindString(cur,"3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertTrue(sqlrcur_sendQuery(cur,"drop function testfunc"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// stored procedure returning
	// multiple values
	printf("STORED PROCEDURE ""RETURNING MULTIPLE VALUES: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc("
		"	in in1 int, "
		"	in in2 double, "
		"	in in3 varchar(20), "
		"	in in4 clob, "
		"	in in5 blob, "
		"	out out1 int, "
		"	out out2 double, "
		"	out out3 varchar(20), "
		"	out out4 clob, "
		"	out out5 blob) ""language sql "
		"begin "
		"	set out1 = in1; "
		"	set out2 = in2; "
		"	set out3 = in3; "
		"	set out4 = in4; "
		"	set out5 = in5; ""end"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,"call testproc(""?,?,?,?,?,?,?,?,?,?)");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",2.5,2,1);
	sqlrcur_inputBindString(cur,"3","hello");
	sqlrcur_inputBindClob(cur,"4","clob",4);
	sqlrcur_inputBindBlob(cur,"5","blob",4);
	sqlrcur_defineOutputBindInteger(cur,"6");
	sqlrcur_defineOutputBindDouble(cur,"7");
	sqlrcur_defineOutputBindString(cur,"8",20);
	sqlrcur_defineOutputBindClob(cur,"9");
	sqlrcur_defineOutputBindBlob(cur,"10");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"6"),1);
	assertEqDbl(sqlrcur_getOutputBindDouble(cur,"7"),2.5);
	assertEqStr(sqlrcur_getOutputBindString(cur,"8"),"hello");
	assertEqStr(sqlrcur_getOutputBindClob(cur,"9"),"clob");
	assertEqStr(sqlrcur_getOutputBindBlob(cur,"10"),"blob");
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// stored procedure returning result set
	printf("STORED PROCEDURE ""RETURNING RESULT SET: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,"create procedure testproc() "
		"result set 1 ""language sql "
		"begin "
		"	declare c1 cursor "
		"		with return for "
		"		select 1 "
		"		from sysibm.sysdummy1 "
		"		union "
		"		select 2 "
		"		from sysibm.sysdummy1 "
		"		union "
		"		select 3 "
		"		from sysibm.sysdummy1 "
		"		union "
		"		select 4 "
		"		from sysibm.sysdummy1 "
		"		union "
		"		select 5 "
		"		from sysibm.sysdummy1 "
		"		union "
		"		select 6 "
		"		from sysibm.sysdummy1 "
		"		union "
		"		select 7 "
		"		from sysibm.sysdummy1 "
		"		union "
		"		select 8 "
		"		from sysibm.sysdummy1; "
		"	open c1; ""end"));
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_sendQuery(cur,"call testproc()"));
	assertEqInt(sqlrcur_rowCount(cur),8);
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// temporary tables
	printf("TEMPORARY TABLES: \n");
	sqlrcur_sendQuery(cur,"drop table session.temptable");
	assertTrue(sqlrcur_sendQuery(cur,
		"declare global temporary table session.temptable ("
		"	col1 int "
		") not logged"));
	assertTrue(sqlrcur_sendQuery(
			cur,"insert into session.temptable values (1)"));
	assertTrue(sqlrcur_sendQuery(
			cur,"select count(*) from session.temptable"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	sqlrcon_endSession(con);
	printf("\n");
	assertFalse(sqlrcur_sendQuery(
			cur,"select count(*) from session.temptable"));
	printf("\n");


	// encoded binary data
	printf("ENCODED BINARY DATA: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable (col1 blob)"));
	for (j=0; j<256; j++) {
		buffer[j]=(unsigned char)j;
	}
	strcpy(querystr,"insert into testtable ""values (blob(X'");
	for (i=0; i<sizeof(buffer); i++) {
		snprintf(hex,sizeof(hex),"%02x",buffer[i]);
		strcat(querystr,hex);
	}
	strcat(querystr,"'))");
	assertTrue(sqlrcur_sendQuery(cur,querystr));
	assertTrue(sqlrcur_sendQuery(cur,"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),sizeof(buffer));
	assertEqInt(memcmp(sqlrcur_getFieldByIndex(cur,0,0),
					buffer,sizeof(buffer)),0);
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
	assertEqInt(strcmp(sqlrcur_getFieldByIndex(cur,0,0),"''"),0);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// last insert id
	printf("LAST INSERT ID: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable "
		"	(col1 int not null "
		"	generated always "
		"	as identity, "
		"	col2 int, "
		"	primary key(col1))"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable "
		"(col2) values (1)"));
	assertEqInt(sqlrcon_getLastInsertId(con),(uint64_t)1);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// database is schema
	printf("DATABASE IS SCHEMA: \n");
	assertTrue(sqlrcon_getDatabaseIsSchema(con));
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
	assertInResultSet(cur,"Database","DB2INST1");
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
		"	col1 integer, "
		"	col2 integer)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable2 ("
		"	col1 integer, "
		"	col2 integer)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable3 ("
		"	col1 integer, "
		"	col2 integer)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable4 ("
		"	col1 integer, "
		"	col2 integer)"));
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_getTableList(cur,NULL));
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE1");
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE2");
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE3");
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE4");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable1"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable2"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable3"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable4"));
	assertTrue(sqlrcon_commit(con));
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
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"254");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"CHAR");
	assertTrue(sqlrcur_getTypeInfoList(cur,"varchar"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"VARCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"12");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"32672");
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
		"	testsmallint smallint, "
		"	testint integer, "
		"	testbigint bigint, "
		"	testdecimal decimal(10,2), "
		"	testreal real, "
		"	testdouble double, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testdate date, "
		"	testtime time, "
		"	testtimestamp timestamp, "
		"	testclob clob, "
		"	testblob blob)"));
	assertTrue(sqlrcon_commit(con));
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
	assertEqStr(sqlrcur_getFieldByName(cur,0,"column_name"),"TESTSMALLINT");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"column_name"),"TESTINT");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"column_name"),"TESTBIGINT");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"column_name"),"TESTDECIMAL");
	assertEqStr(sqlrcur_getFieldByName(cur,4,"column_name"),"TESTREAL");
	assertEqStr(sqlrcur_getFieldByName(cur,5,"column_name"),"TESTDOUBLE");
	assertEqStr(sqlrcur_getFieldByName(cur,6,"column_name"),"TESTCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"column_name"),"TESTVARCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,8,"column_name"),"TESTDATE");
	assertEqStr(sqlrcur_getFieldByName(cur,9,"column_name"),"TESTTIME");
	assertEqStr(sqlrcur_getFieldByName(cur,10,"column_name"),
							"TESTTIMESTAMP");
	assertEqStr(sqlrcur_getFieldByName(cur,11,"column_name"),"TESTCLOB");
	assertEqStr(sqlrcur_getFieldByName(cur,12,"column_name"),"TESTBLOB");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"SMALLINT");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"data_type"),"INTEGER");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"data_type"),"BIGINT");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"data_type"),"DECIMAL");
	assertEqStr(sqlrcur_getFieldByName(cur,4,"data_type"),"REAL");
	assertEqStr(sqlrcur_getFieldByName(cur,5,"data_type"),"DOUBLE");
	assertEqStr(sqlrcur_getFieldByName(cur,6,"data_type"),"CHARACTER");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"data_type"),"VARCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,8,"data_type"),"DATE");
	assertEqStr(sqlrcur_getFieldByName(cur,9,"data_type"),"TIME");
	assertEqStr(sqlrcur_getFieldByName(cur,10,"data_type"),"TIMESTAMP");
	assertEqStr(sqlrcur_getFieldByName(cur,11,"data_type"),"CLOB");
	assertEqStr(sqlrcur_getFieldByName(cur,12,"data_type"),"BLOB");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// column list - auto_increment,
	// primary key
	printf("COLUMN LIST - ""auto_increment, ""primary key: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 int generated always "
		"	as identity primary key, "
		"	col2 int)"));
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_getColumnList(cur,"testtable",NULL));
	assertTrue(strstr(sqlrcur_getFieldByName(cur,0,"extra"),
		"auto_increment")!=NULL);
	assertTrue(strstr(sqlrcur_getFieldByName(cur,0,"column_key"),
		"PRI")!=NULL);
	assertFalse(strstr(sqlrcur_getFieldByName(cur,1,"extra"),
		"auto_increment")!=NULL);
	assertFalse(strstr(sqlrcur_getFieldByName(cur,1,"column_key"),
		"PRI")!=NULL);
	printf("\n");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 int not null "
		"	primary key, "
		"	col2 int)"));
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_getColumnList(cur,"testtable",NULL));
	assertFalse(strstr(sqlrcur_getFieldByName(cur,0,"extra"),
		"auto_increment")!=NULL);
	assertTrue(strstr(sqlrcur_getFieldByName(cur,0,"column_key"),
		"PRI")!=NULL);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// primary keys list
	printf("PRIMARY KEYS LIST: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 int not null "
		"	primary key, "
		"	col2 int)"));
	assertTrue(sqlrcon_commit(con));
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
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,0,"table"),"TESTTABLE"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"seq_in_index"),"1");
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,0,"column_name"),"COL1"));
	assertTrue(!(!sqlrcur_getFieldByName(cur,0,"key_name") ||
		!sqlrcur_getFieldByName(cur,0,"key_name")[0]));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// key and index list
	printf("KEY AND INDEX LIST: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 int not null "
		"	primary key, "
		"	col2 int)"));
	assertTrue(sqlrcon_commit(con));
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
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,0,"table"),"TESTTABLE"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"non_unique"),"0");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"seq_in_index"),"1");
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,0,"column_name"),"COL1"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"collation"),"A");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"index_type"),"3");
	assertTrue(!(!sqlrcur_getFieldByName(cur,0,"key_name") ||
		!sqlrcur_getFieldByName(cur,0,"key_name")[0]));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// procedure list
	printf("PROCEDURE LIST: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc1");
	sqlrcur_sendQuery(cur,"drop procedure testproc2");
	sqlrcur_sendQuery(cur,"drop procedure testproc3");
	sqlrcur_sendQuery(cur,"drop procedure testproc4");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc1("
		"	in in1 integer, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) ""language sql begin end"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc2("
		"	in in1 integer, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) ""language sql begin end"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc3("
		"	in in1 integer, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) ""language sql begin end"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc4("
		"	in in1 integer, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) ""language sql begin end"));
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_getProcedureList(cur,NULL));
	assertInResultSet(cur,"routine_name","TESTPROC1");
	assertInResultSet(cur,"routine_name","TESTPROC2");
	assertInResultSet(cur,"routine_name","TESTPROC3");
	assertInResultSet(cur,"routine_name","TESTPROC4");
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
	assertEqStr(sqlrcur_getFieldByName(cur,0,"parameter_name"),"IN1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"INTEGER");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"ordinal_position"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"parameter_name"),"IN2");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"data_type"),"CHARACTER");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"ordinal_position"),"2");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"parameter_name"),"IN3");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"data_type"),"VARCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"ordinal_position"),"3");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"parameter_name"),"IN4");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"data_type"),"DATE");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"ordinal_position"),"4");
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc1"));
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc2"));
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc3"));
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc4"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// invalid queries
	printf("INVALID QUERIES: \n");
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
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
