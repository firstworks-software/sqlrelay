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

	const char	*bindvars[]={"1","2","3","4","5","6","7","8","9","10",
				"11","12",NULL};
	const char	*bindvals[]={"7","7","7.5","7.5","7.5","7.5",
				"01-JAN-2007","07:00:00",
				"testchar7","testvarchar7",NULL,"testblob7"};
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
	char		*socket;
	uint16_t	id;
	char		*filename;
	uint64_t	i;

	#define	LARGE_BUFFER_LENGTH	(20*1024)
	char		largebuffer[LARGE_BUFFER_LENGTH+1];



	// instantiation
	con=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
			"testuser","testpassword",0,1);
	cur=sqlrcur_alloc(con);


	// identify
	printf("IDENTIFY: \n");
	assertEqStr(sqlrcon_identify(con),"firebird");
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
	assertEqStr(sqlrcon_nextvalFormat(con),"next value for %s");
	printf("\n");


	// isolation levels
	printf("ISOLATION LEVELS: \n");
	// though firebird does support a
	// "set transaction ..." statement to set the
	// isolation level, it looks like, in firebird,
	// you can really only set it through the TPB at
	// the start of a transaction, so attempts to set
	// it should fail
	assertFalse(sqlrcon_setIsolationLevel(con,"read committed"));
	assertEqStr(sqlrcon_getIsolationLevel(con),"read committed");
	printf("\n");


	// insert
	printf("INSERT: \n");
	sqlrcur_sendQuery(cur,"delete from testtable");
	sqlrcon_commit(con);
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	1, "
		"	1.5, "
		"	1.5, "
		"	1.5, "
		"	1.5, "
		"	'01-JAN-2001', "
		"	'01:00:00', "
		"	'testchar1', "
		"	'testvarchar1', "
		"	NULL, "
		"	'testblob1')"));
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
		"	?, "
		"	?)");
	assertEqInt(sqlrcur_countBindVariables(cur),12);
	sqlrcur_inputBindLong(cur,"1",2);
	sqlrcur_inputBindLong(cur,"2",2);
	sqlrcur_inputBindDouble(cur,"3",2.5,2,1);
	sqlrcur_inputBindDouble(cur,"4",2.5,2,1);
	sqlrcur_inputBindDouble(cur,"5",2.5,2,1);
	sqlrcur_inputBindDouble(cur,"6",2.5,2,1);
	sqlrcur_inputBindDate(cur,"7",2002,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate(cur,"8",-1,-1,-1,2,0,0,0,NULL,0);
	sqlrcur_inputBindString(cur,"9","testchar2");
	sqlrcur_inputBindString(cur,"10","testvarchar2");
	sqlrcur_inputBindString(cur,"11",(char *)NULL);
	sqlrcur_inputBindBlob(cur,"12","testblob2",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",3);
	sqlrcur_inputBindLong(cur,"2",3);
	sqlrcur_inputBindDouble(cur,"3",3.5,2,1);
	sqlrcur_inputBindDouble(cur,"4",3.5,2,1);
	sqlrcur_inputBindDouble(cur,"5",3.5,2,1);
	sqlrcur_inputBindDouble(cur,"6",3.5,2,1);
	sqlrcur_inputBindDate(cur,"7",2003,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate(cur,"8",-1,-1,-1,3,0,0,0,NULL,0);
	sqlrcur_inputBindString(cur,"9","testchar3");
	sqlrcur_inputBindString(cur,"10","testvarchar3");
	sqlrcur_inputBindString(cur,"11",(char *)NULL);
	sqlrcur_inputBindBlob(cur,"12","testblob3",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",4);
	sqlrcur_inputBindLong(cur,"2",4);
	sqlrcur_inputBindDouble(cur,"3",4.5,2,1);
	sqlrcur_inputBindDouble(cur,"4",4.5,2,1);
	sqlrcur_inputBindDouble(cur,"5",4.5,2,1);
	sqlrcur_inputBindDouble(cur,"6",4.5,2,1);
	sqlrcur_inputBindDate(cur,"7",2004,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate(cur,"8",-1,-1,-1,4,0,0,0,NULL,0);
	sqlrcur_inputBindString(cur,"9","testchar4");
	sqlrcur_inputBindString(cur,"10","testvarchar4");
	sqlrcur_inputBindString(cur,"11",(char *)NULL);
	sqlrcur_inputBindBlob(cur,"12","testblob4",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",5);
	sqlrcur_inputBindLong(cur,"2",5);
	sqlrcur_inputBindDouble(cur,"3",5.5,2,1);
	sqlrcur_inputBindDouble(cur,"4",5.5,2,1);
	sqlrcur_inputBindDouble(cur,"5",5.5,2,1);
	sqlrcur_inputBindDouble(cur,"6",5.5,2,1);
	sqlrcur_inputBindDate(cur,"7",2005,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate(cur,"8",-1,-1,-1,5,0,0,0,NULL,0);
	sqlrcur_inputBindString(cur,"9","testchar5");
	sqlrcur_inputBindString(cur,"10","testvarchar5");
	sqlrcur_inputBindString(cur,"11",(char *)NULL);
	sqlrcur_inputBindBlob(cur,"12","testblob5",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",6);
	sqlrcur_inputBindLong(cur,"2",6);
	sqlrcur_inputBindDouble(cur,"3",6.5,2,1);
	sqlrcur_inputBindDouble(cur,"4",6.5,2,1);
	sqlrcur_inputBindDouble(cur,"5",6.5,2,1);
	sqlrcur_inputBindDouble(cur,"6",6.5,2,1);
	sqlrcur_inputBindDate(cur,"7",2006,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate(cur,"8",-1,-1,-1,6,0,0,0,NULL,0);
	sqlrcur_inputBindString(cur,"9","testchar6");
	sqlrcur_inputBindString(cur,"10","testvarchar6");
	sqlrcur_inputBindString(cur,"11",(char *)NULL);
	sqlrcur_inputBindBlob(cur,"12","testblob6",9);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// array of input binds by position
	printf("ARRAY OF INPUT BINDS BY POSITION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindStrings(cur,bindvars,bindvals);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// input bind by position with validation
	printf("INPUT BIND BY POSITION ""WITH VALIDATION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",8);
	sqlrcur_inputBindLong(cur,"2",8);
	sqlrcur_inputBindDouble(cur,"3",8.5,2,1);
	sqlrcur_inputBindDouble(cur,"4",8.5,2,1);
	sqlrcur_inputBindDouble(cur,"5",8.5,2,1);
	sqlrcur_inputBindDouble(cur,"6",8.5,2,1);
	sqlrcur_inputBindDate(cur,"7",2008,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate(cur,"8",-1,-1,-1,8,0,0,0,NULL,0);
	sqlrcur_inputBindString(cur,"9","testchar8");
	sqlrcur_inputBindString(cur,"10","testvarchar8");
	sqlrcur_inputBindString(cur,"11",(char *)NULL);
	sqlrcur_inputBindBlob(cur,"12","testblob8",9);
	sqlrcur_validateBinds(cur);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// input bind by name
	// firebird doesn't support bind by name


	// array of input binds by name
	// firebird doesn't support bind by name


	// input bind by name with validation
	// firebird doesn't support bind by name


	// select
	printf("SELECT: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testinteger "));
	printf("\n");


	// column count
	printf("COLUMN COUNT: \n");
	assertEqInt(sqlrcur_colCount(cur),12);
	printf("\n");


	// column names
	printf("COLUMN NAMES: \n");
	assertEqStr(sqlrcur_getColumnName(cur,0),"TESTINTEGER");
	assertEqStr(sqlrcur_getColumnName(cur,1),"TESTSMALLINT");
	assertEqStr(sqlrcur_getColumnName(cur,2),"TESTDECIMAL");
	assertEqStr(sqlrcur_getColumnName(cur,3),"TESTNUMERIC");
	assertEqStr(sqlrcur_getColumnName(cur,4),"TESTFLOAT");
	assertEqStr(sqlrcur_getColumnName(cur,5),"TESTDOUBLE");
	assertEqStr(sqlrcur_getColumnName(cur,6),"TESTDATE");
	assertEqStr(sqlrcur_getColumnName(cur,7),"TESTTIME");
	assertEqStr(sqlrcur_getColumnName(cur,8),"TESTCHAR");
	assertEqStr(sqlrcur_getColumnName(cur,9),"TESTVARCHAR");
	assertEqStr(sqlrcur_getColumnName(cur,10),"TESTTIMESTAMP");
	assertEqStr(sqlrcur_getColumnName(cur,11),"TESTBLOB");
	cols=sqlrcur_getColumnNames(cur);
	assertEqStr(cols[0],"TESTINTEGER");
	assertEqStr(cols[1],"TESTSMALLINT");
	assertEqStr(cols[2],"TESTDECIMAL");
	assertEqStr(cols[3],"TESTNUMERIC");
	assertEqStr(cols[4],"TESTFLOAT");
	assertEqStr(cols[5],"TESTDOUBLE");
	assertEqStr(cols[6],"TESTDATE");
	assertEqStr(cols[7],"TESTTIME");
	assertEqStr(cols[8],"TESTCHAR");
	assertEqStr(cols[9],"TESTVARCHAR");
	assertEqStr(cols[10],"TESTTIMESTAMP");
	assertEqStr(cols[11],"TESTBLOB");
	printf("\n");


	// column types
	printf("COLUMN TYPES: \n");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),"INTEGER");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTINTEGER"),"INTEGER");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,1),"SMALLINT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTSMALLINT"),"SMALLINT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,2),"DECIMAL");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTDECIMAL"),"DECIMAL");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,3),"NUMERIC");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTNUMERIC"),"NUMERIC");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,4),"FLOAT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTFLOAT"),"FLOAT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,5),"DOUBLE PRECISION");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTDOUBLE"),
		"DOUBLE PRECISION");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,6),"DATE");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTDATE"),"DATE");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,7),"TIME");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTTIME"),"TIME");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,8),"CHAR");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTCHAR"),"CHAR");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,9),"VARCHAR");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTVARCHAR"),"VARCHAR");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,10),"TIMESTAMP");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTTIMESTAMP"),
		"TIMESTAMP");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,11),"BLOB");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTBLOB"),"BLOB");
	printf("\n");


	// column length
	printf("COLUMN LENGTH: \n");
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),4);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTINTEGER"),4);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,1),2);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTSMALLINT"),2);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,2),8);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTDECIMAL"),8);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,3),8);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTNUMERIC"),8);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,4),4);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTFLOAT"),4);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,5),8);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTDOUBLE"),8);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,6),4);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTDATE"),4);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,7),4);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTTIME"),4);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,8),50);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTCHAR"),50);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,9),50);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTVARCHAR"),50);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,10),8);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTTIMESTAMP"),8);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,11),8);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTBLOB"),8);
	printf("\n");


	// longest column
	printf("LONGEST COLUMN: \n");
	assertEqInt(sqlrcur_getLongestByIndex(cur,0),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTINTEGER"),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,1),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTSMALLINT"),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,2),4);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTDECIMAL"),4);
	assertEqInt(sqlrcur_getLongestByIndex(cur,3),4);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTNUMERIC"),4);
	assertEqInt(sqlrcur_getLongestByIndex(cur,4),6);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTFLOAT"),6);
	assertEqInt(sqlrcur_getLongestByIndex(cur,5),6);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTDOUBLE"),6);
	assertEqInt(sqlrcur_getLongestByIndex(cur,6),10);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTDATE"),10);
	assertEqInt(sqlrcur_getLongestByIndex(cur,7),8);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTTIME"),8);
	assertEqInt(sqlrcur_getLongestByIndex(cur,8),50);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTCHAR"),50);
	assertEqInt(sqlrcur_getLongestByIndex(cur,9),12);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTVARCHAR"),12);
	assertEqInt(sqlrcur_getLongestByIndex(cur,10),0);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTTIMESTAMP"),0);
	assertEqInt(sqlrcur_getLongestByIndex(cur,11),9);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTBLOB"),9);
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
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"1.50");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,3),"1.50");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,4),"1.5000");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,5),"1.5000");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,6),"2001:01:01");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,7),"01:00:00");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,8),"testchar1"
		"                                         ");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,9),"testvarchar1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,11),"testblob1");
	printf("\n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,2),"8.50");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,3),"8.50");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,4),"8.5000");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,5),"8.5000");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,6),"2008:01:01");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,7),"08:00:00");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,8),"testchar8"
		"                                         ");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,9),"testvarchar8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,11),"testblob8");
	printf("\n");


	// field lengths by index
	printf("FIELD LENGTHS BY INDEX: \n");
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,1),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,2),4);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,3),4);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,4),6);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,5),6);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,6),10);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,7),8);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,8),50);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,9),12);
	printf("\n");
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,0),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,1),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,2),4);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,3),4);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,4),6);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,5),6);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,6),10);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,7),8);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,8),50);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,9),12);
	printf("\n");


	// fields by name
	printf("FIELDS BY NAME: \n");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTINTEGER"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTSMALLINT"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTDECIMAL"),"1.50");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTNUMERIC"),"1.50");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTFLOAT"),"1.5000");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTDOUBLE"),"1.5000");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTDATE"),"2001:01:01");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTTIME"),"01:00:00");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTCHAR"),"testchar1"
		"                                         ");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTVARCHAR"),"testvarchar1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTBLOB"),"testblob1");
	printf("\n");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTINTEGER"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTSMALLINT"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTDECIMAL"),"8.50");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTNUMERIC"),"8.50");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTFLOAT"),"8.5000");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTDOUBLE"),"8.5000");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTDATE"),"2008:01:01");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTTIME"),"08:00:00");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTCHAR"),"testchar8"
		"                                         ");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTVARCHAR"),"testvarchar8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTBLOB"),"testblob8");
	printf("\n");


	// field lengths by name
	printf("FIELD LENGTHS BY NAME: \n");
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTINTEGER"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTSMALLINT"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTDECIMAL"),4);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTNUMERIC"),4);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTFLOAT"),6);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTDOUBLE"),6);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTDATE"),10);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTTIME"),8);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTCHAR"),50);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTVARCHAR"),12);
	printf("\n");
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTINTEGER"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTSMALLINT"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTDECIMAL"),4);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTNUMERIC"),4);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTFLOAT"),6);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTDOUBLE"),6);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTDATE"),10);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTTIME"),8);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTCHAR"),50);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTVARCHAR"),12);
	printf("\n");


	// fields by array
	printf("FIELDS BY ARRAY: \n");
	fields=sqlrcur_getRow(cur,0);
	assertEqStr(fields[0],"1");
	assertEqStr(fields[1],"1");
	assertEqStr(fields[2],"1.50");
	assertEqStr(fields[3],"1.50");
	assertEqStr(fields[4],"1.5000");
	assertEqStr(fields[5],"1.5000");
	assertEqStr(fields[6],"2001:01:01");
	assertEqStr(fields[7],"01:00:00");
	assertEqStr(fields[8],"testchar1"
		"                                         ");
	assertEqStr(fields[9],"testvarchar1");
	assertEqStr(fields[11],"testblob1");
	printf("\n");


	// field lengths by array
	printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=sqlrcur_getRowLengths(cur,0);
	assertEqInt(fieldlens[0],1);
	assertEqInt(fieldlens[1],1);
	assertEqInt(fieldlens[2],4);
	assertEqInt(fieldlens[3],4);
	assertEqInt(fieldlens[4],6);
	assertEqInt(fieldlens[5],6);
	assertEqInt(fieldlens[6],10);
	assertEqInt(fieldlens[7],8);
	assertEqInt(fieldlens[8],50);
	assertEqInt(fieldlens[9],12);
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
		"	testinteger "));
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
		"	testinteger "));
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
		"	testinteger "));
	assertEqStr(sqlrcur_getColumnName(cur,0),"TESTINTEGER");
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),4);
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),"INTEGER");
	printf("\n");


	// suspended session
	printf("SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testinteger "));
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
		"	testinteger "));
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
		"	testinteger "));
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
		"	testinteger "));
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
		"	testinteger "));
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqStr(filename,"cachefile1");
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	free(filename);
	printf("\n");


	// column count for cached result set
	printf("COLUMN COUNT FOR ""CACHED RESULT SET: \n");
	assertEqInt(sqlrcur_colCount(cur),12);
	printf("\n");


	// column names for cached result set
	printf("COLUMN NAMES FOR ""CACHED RESULT SET: \n");
	assertEqStr(sqlrcur_getColumnName(cur,0),"TESTINTEGER");
	assertEqStr(sqlrcur_getColumnName(cur,1),"TESTSMALLINT");
	assertEqStr(sqlrcur_getColumnName(cur,2),"TESTDECIMAL");
	assertEqStr(sqlrcur_getColumnName(cur,3),"TESTNUMERIC");
	assertEqStr(sqlrcur_getColumnName(cur,4),"TESTFLOAT");
	assertEqStr(sqlrcur_getColumnName(cur,5),"TESTDOUBLE");
	assertEqStr(sqlrcur_getColumnName(cur,6),"TESTDATE");
	assertEqStr(sqlrcur_getColumnName(cur,7),"TESTTIME");
	assertEqStr(sqlrcur_getColumnName(cur,8),"TESTCHAR");
	assertEqStr(sqlrcur_getColumnName(cur,9),"TESTVARCHAR");
	assertEqStr(sqlrcur_getColumnName(cur,10),"TESTTIMESTAMP");
	assertEqStr(sqlrcur_getColumnName(cur,11),"TESTBLOB");
	cols=sqlrcur_getColumnNames(cur);
	assertEqStr(cols[0],"TESTINTEGER");
	assertEqStr(cols[1],"TESTSMALLINT");
	assertEqStr(cols[2],"TESTDECIMAL");
	assertEqStr(cols[3],"TESTNUMERIC");
	assertEqStr(cols[4],"TESTFLOAT");
	assertEqStr(cols[5],"TESTDOUBLE");
	assertEqStr(cols[6],"TESTDATE");
	assertEqStr(cols[7],"TESTTIME");
	assertEqStr(cols[8],"TESTCHAR");
	assertEqStr(cols[9],"TESTVARCHAR");
	assertEqStr(cols[10],"TESTTIMESTAMP");
	assertEqStr(cols[11],"TESTBLOB");
	printf("\n");


	// cached result set with result set buffer size
	printf("CACHED RESULT SET WITH ""RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile1");
	sqlrcur_setCacheTtl(cur,200);
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testinteger "));
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
		"	testinteger "));
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
		"	testinteger "));
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
	// truncate testtable so this section starts with it empty;
	// firebird DDL on the table here would otherwise hit cursor-state
	// issues at the next commit, so we reuse the existing schema and
	// just write to one column (testinteger)
	assertTrue(sqlrcur_sendQuery(cur,"delete from testtable"));
	// commit so the truncation is visible to the second connection
	// (the commit implicitly starts a new tx)
	assertTrue(sqlrcon_commit(con));
	secondcon=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	secondcur=sqlrcur_alloc(secondcon);
	// session is in a transaction; insert is not visible until commit
	assertTrue(sqlrcon_getInTransaction(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable (testinteger) values (1)"));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"0");
	// commit makes it visible, and implicitly starts a new transaction
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
	// rollback discards, and implicitly starts a new transaction
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable (testinteger) values (2)"));
	assertTrue(sqlrcon_rollback(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
	// autoCommitOn takes effect immediately
	assertTrue(sqlrcon_autoCommitOn(con));
	assertTrue(sqlrcon_getAutoCommit(con));
	assertFalse(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable (testinteger) values (3)"));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"2");
	// autoCommitOff takes effect immediately
	assertTrue(sqlrcon_autoCommitOff(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcon_getInTransaction(con));
	sqlrcur_closeResultSet(secondcur);
	printf("\n");


	// transaction behavior - explicit
	printf("TRANSACTION BEHAVIOR - explicit: \n");
	assertTrue(sqlrcon_setTransactionModel(con,"explicit"));
	assertEqStr(sqlrcon_getTransactionModel(con),"explicit");
	// truncate testtable so this section starts with it empty (delete
	// autocommits here since explicit-model defaults to autocommit-on)
	assertTrue(sqlrcur_sendQuery(cur,"delete from testtable"));
	// begin starts a new transaction; insert is not visible until commit
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable (testinteger) values (1)"));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"0");
	// commit makes it visible; no new transaction is started
	assertTrue(sqlrcon_commit(con));
	assertFalse(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
	// begin, insert, rollback discards; no new transaction is started
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable (testinteger) values (2)"));
	assertTrue(sqlrcon_rollback(con));
	assertFalse(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
	// autoCommitOn takes effect immediately
	assertTrue(sqlrcon_autoCommitOn(con));
	assertTrue(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable (testinteger) values (3)"));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"2");
	// autoCommitOff takes effect immediately
	assertTrue(sqlrcon_autoCommitOff(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	sqlrcur_closeResultSet(secondcur);
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
	// truncate testtable so this section starts with it empty
	assertTrue(sqlrcur_sendQuery(cur,"delete from testtable"));
	// begin starts a transaction; commit makes it visible
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable (testinteger) values (1)"));
	assertTrue(sqlrcon_commit(con));
	assertFalse(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
	// begin, insert, rollback discards
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable (testinteger) values (2)"));
	assertTrue(sqlrcon_rollback(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
	// during a transaction started by begin(), autoCommitOn is a
	// no-op: the autocommit setting takes effect after the user
	// explicitly commits/rollbacks the tx (mysql-native semantic)
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable (testinteger) values (3)"));
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
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable (testinteger) values (4)"));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"3");
	// autoCommitOff takes effect immediately when not in a transaction
	assertTrue(sqlrcon_autoCommitOff(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	// autocommit-off persists across commit/rollback; each commit or
	// rollback ends the current implicit tx and a new one starts for
	// the next statement
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable (testinteger) values (5)"));
	assertTrue(sqlrcon_commit(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"4");
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable (testinteger) values (6)"));
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
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable (testinteger) values (7)"));
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
	printf("\n");


	// transaction behavior - explicit-error
	printf("TRANSACTION BEHAVIOR - explicit-error: \n");
	assertTrue(sqlrcon_setTransactionModel(con,"explicit-error"));
	assertEqStr(sqlrcon_getTransactionModel(con),"explicit-error");
	// truncate testtable so this section starts with it empty
	assertTrue(sqlrcur_sendQuery(cur,"delete from testtable"));
	// begin, insert, commit
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable (testinteger) values (1)"));
	assertTrue(sqlrcon_commit(con));
	assertFalse(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
	// begin, insert, rollback
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable (testinteger) values (2)"));
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
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable (testinteger) values (3)"));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"2");
	// autoCommitOff takes effect immediately
	assertTrue(sqlrcon_autoCommitOff(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	sqlrcur_closeResultSet(secondcur);
	printf("\n");


	// transaction behavior - none
	printf("TRANSACTION BEHAVIOR - none: \n");
	assertTrue(sqlrcon_setTransactionModel(con,"none"));
	assertEqStr(sqlrcon_getTransactionModel(con),"none");
	// truncate testtable so this section starts with it empty
	assertTrue(sqlrcur_sendQuery(cur,"delete from testtable"));
	// no transactions; everything is visible immediately
	assertTrue(sqlrcon_getAutoCommit(con));
	assertFalse(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable (testinteger) values (1)"));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
	// commit and rollback are no-ops
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable (testinteger) values (2)"));
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
	printf("\n");


	// reset transaction behavior
	printf("RESET TRANSACTION BEHAVIOR: \n");
	assertTrue(sqlrcon_setTransactionModel(con,sqlrcon_getDefaultTransactionModel(con)));
	assertEqStr(sqlrcon_getTransactionModel(con),"implicit");
	assertFalse(sqlrcon_getAutoCommit(con));
	printf("\n");


	// individual substitutions
	printf("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"select $(var1),'$(var2)',$(var3) "
		"from rdb$database");
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
	sqlrcur_prepareQuery(cur,
		"select "
		"	'$(var1)', "
		"	'$(var2)', "
		"	'$(var3)' "
		"from "
		"	rdb$database ");
	sqlrcur_subStrings(cur,subvars,subvalstrings);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"hi");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"bye");
	printf("\n");
	sqlrcur_prepareQuery(cur,"select $(var1),$(var2),$(var3) "
		"from rdb$database");
	sqlrcur_subLongs(cur,subvars,subvallongs);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"2");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"3");
	printf("\n");
	sqlrcur_prepareQuery(cur,"select $(var1),$(var2),$(var3) "
		"from rdb$database");
	sqlrcur_subDoubles(cur,subvars,subvaldoubles,precs,scales);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"10.55");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"10.556");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"10.5556");
	printf("\n");


	// nulls as nulls
	printf("NULLS AS NULLS: \n");
	sqlrcur_getNullsAsNulls(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select 1,NULL,NULL "
		"from rdb$database"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select 1,NULL,NULL "
		"from rdb$database"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"");
	printf("\n");


	// null and empty lobs
	printf("NULL AND EMPTY LOBS: \n");
	sqlrcur_getNullsAsNulls(cur);
	sqlrcur_sendQuery(cur,"delete from testtable1");
	sqlrcur_prepareQuery(cur,"insert into testtable1 values (?)");
	sqlrcur_inputBindBlob(cur,"1","",0);
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"select testblob from testtable1"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTBLOB"),"");
	sqlrcur_sendQuery(cur,"delete from testtable1");
	sqlrcur_prepareQuery(cur,"insert into testtable1 values (?)");
	sqlrcur_inputBindBlob(cur,"1",NULL,0);
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"select testblob from testtable1"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTBLOB"),NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,"delete from testtable1"));
	printf("\n");


	// long lobs
	printf("LONG LOBS: \n");
	sqlrcur_sendQuery(cur,"delete from testtable1");
	sqlrcur_prepareQuery(cur,"insert into testtable1 values (?)");
	for (i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	sqlrcur_inputBindClob(cur,"1",largebuffer,LARGE_BUFFER_LENGTH);
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"select testblob from testtable1"));
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,
					"TESTBLOB"),LARGE_BUFFER_LENGTH);
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTBLOB"),largebuffer);
	assertTrue(sqlrcur_sendQuery(cur,"delete from testtable1"));
	printf("\n");


	// output bind by position
	printf("OUTPUT BIND BY POSITION: \n");
	sqlrcur_getNullsAsNulls(cur);
	sqlrcur_prepareQuery(cur,"execute procedure ""testproc ?, ?, ?, ?");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",1.5,2,1);
	sqlrcur_inputBindString(cur,"3","hello");
	sqlrcur_inputBindBlob(cur,"4","blob",4);
	sqlrcur_defineOutputBindInteger(cur,"1");
	sqlrcur_defineOutputBindDouble(cur,"2");
	sqlrcur_defineOutputBindString(cur,"3",20);
	sqlrcur_defineOutputBindBlob(cur,"4");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"1"),1);
	double d=sqlrcur_getOutputBindDouble(cur,"2");
	assertEqDbl(d,1.5);
	assertEqStr(sqlrcur_getOutputBindString(cur,"3"),
		"hello               ");
	assertEqStr(sqlrcur_getOutputBindBlob(cur,"4"),"blob");
	sqlrcur_getNullsAsEmptyStrings(cur);
	printf("\n");


	// output bind by name
	// firebird doesn't support bind by name


	// output bind by name with validation
	// firebird doesn't support bind by name


	// lob output bind
	printf("LOB OUTPUT BIND: \n");
	sqlrcur_prepareQuery(cur,"execute procedure testproc1 ?");
	sqlrcur_inputBindBlob(cur,"1","hello",5);
	sqlrcur_defineOutputBindBlob(cur,"1");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStrLen(sqlrcur_getOutputBindBlob(cur,"1"),"hello",5);
	assertEqInt(sqlrcur_getOutputBindLength(cur,"1"),5);
	printf("\n");


	// long output bind
	printf("LONG OUTPUT BIND: \n");
	for (i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	sqlrcur_prepareQuery(cur,"execute procedure testproc1 ?");
	sqlrcur_inputBindBlob(cur,"1",largebuffer,LARGE_BUFFER_LENGTH);
	sqlrcur_defineOutputBindBlob(cur,"1");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindLength(cur,"1"),LARGE_BUFFER_LENGTH);
	assertEqStrLen(sqlrcur_getOutputBindBlob(cur,"1"),largebuffer,
		LARGE_BUFFER_LENGTH);
	printf("\n");


	// negative input bind
	printf("NEGATIVE INPUT BIND: \n");
	sqlrcur_prepareQuery(cur,"select cast(? as integer) "
		"from rdb$database");
	sqlrcur_inputBindLong(cur,"1",-1);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"-1");
	printf("\n");


	// bind validation
	// firebird doesn't support bind by name


	// rebinding
	printf("REBINDING: \n");
	sqlrcur_prepareQuery(cur,"execute procedure ""testproc ?, ?, ?, ?");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",1.5,2,1);
	sqlrcur_inputBindString(cur,"3","hello");
	sqlrcur_inputBindBlob(cur,"4","blob",4);
	sqlrcur_defineOutputBindInteger(cur,"1");
	sqlrcur_defineOutputBindDouble(cur,"2");
	sqlrcur_defineOutputBindString(cur,"3",20);
	sqlrcur_defineOutputBindBlob(cur,"4");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"1"),1);
	sqlrcur_inputBindLong(cur,"1",2);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"1"),2);
	sqlrcur_inputBindLong(cur,"1",3);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"1"),3);
	printf("\n");


	// reexecute
	printf("REEXECUTE: \n");
	sqlrcur_prepareQuery(cur,"select 1 from rdb$database");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	printf("\n");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	printf("\n");
	sqlrcur_prepareQuery(cur,"select cast(? as int) ""from rdb$database");
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
	sqlrcur_prepareQuery(cur,
		"execute block (in1 int = ?, "
		"	in2 double precision = ?, "
		"	in3 varchar(20) = ?) ""as ""begin ""end");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",1.5,2,1);
	sqlrcur_inputBindString(cur,"3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// stored procedure returning single value
	printf("STORED PROCEDURE ""RETURNING SINGLE VALUE: \n");
	sqlrcur_prepareQuery(cur,
		"execute block (in1 int = ?, "
		"	in2 double precision = ?, "
		"	in3 varchar(20) = ?) ""returns (out1 int) ""as "
		"begin "
		"	out1 = in1; "
		"	suspend; ""end");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",1.5,2,1);
	sqlrcur_inputBindString(cur,"3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	printf("\n");


	// stored procedure returning multiple values
	printf("STORED PROCEDURE ""RETURNING MULTIPLE VALUES: \n");
	sqlrcur_prepareQuery(cur,
		"execute block (in1 int = ?, "
		"	in2 double precision = ?, "
		"	in3 varchar(20) = ?) "
		"returns (out1 int, "
		"	out2 double precision, "
		"	out3 varchar(20)) ""as "
		"begin "
		"	out1 = in1; "
		"	out2 = in2; "
		"	out3 = in3; "
		"	suspend; ""end");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",1.5,2,1);
	sqlrcur_inputBindString(cur,"3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"1.5000");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"hello");
	printf("\n");


	// stored procedure returning result set
	printf("STORED PROCEDURE ""RETURNING RESULT SET: \n");
	sqlrcur_prepareQuery(cur,"execute block ""returns (out1 int) ""as "
		"declare i int; "
		"begin "
		"	i = 1; "
		"	while (i <= 8) do "
		"	begin "
		"		out1 = i; "
		"		suspend; "
		"		i = i + 1; "
		"	end ""end");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),8);
	printf("\n");


	// temporary tables
	// firebird supports temporary tables,
	// but we're omitting this for now


	// encoded binary data
	// firebird doesn't support encoded binary data


	// quotes
	printf("QUOTES: \n");
	sqlrcur_sendQuery(cur,"delete from table testtable1");
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable1 "
		"values ('''''')"));
	assertTrue(sqlrcur_sendQuery(cur,"select testblob from testtable1"));
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),2);
	assertTrue(!memcmp(sqlrcur_getFieldByIndex(cur,0,0),"''",2));
	assertTrue(sqlrcur_sendQuery(cur,"delete from testtable1"));
	printf("\n");


	// last insert id
	// firebird doesn't support auto-increment


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
	assertInResultSet(cur,"Database","TESTUSER");
	printf("\n");


	// table type list
	printf("TABLE TYPE LIST: \n");
	assertTrue(sqlrcur_getTableTypeList(cur));
	assertEqStr(sqlrcur_getColumnName(cur,0),"table_type");
	assertInResultSet(cur,"table_type","TABLE");
	printf("\n");


	// table list
	printf("TABLE LIST: \n");
	assertTrue(sqlrcur_getTableList(cur,NULL));
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE1");
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE2");
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE3");
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
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"32767");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"CHAR");
	assertTrue(sqlrcur_getTypeInfoList(cur,"varchar"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"VARCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"12");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"32765");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"VARCHAR");
	assertTrue(sqlrcur_getTypeInfoList(cur,"date"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"DATE");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"91");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"10");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"DATE");
	printf("\n");


	// column list
	printf("COLUMN LIST: \n");
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
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,0,"column_name"),
		"TESTINTEGER"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,1,"column_name"),
		"TESTSMALLINT"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,2,"column_name"),
		"TESTDECIMAL"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,3,"column_name"),
		"TESTNUMERIC"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,4,"column_name"),
		"TESTFLOAT"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,5,"column_name"),
		"TESTDOUBLE"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,6,"column_name"),
		"TESTDATE"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,7,"column_name"),
		"TESTTIME"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,8,"column_name"),
		"TESTCHAR"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,9,"column_name"),
		"TESTVARCHAR"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,10,"column_name"),
		"TESTTIMESTAMP"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,11,"column_name"),
		"TESTBLOB"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,0,"data_type"),
		"INTEGER"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,1,"data_type"),
		"SMALLINT"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,2,"data_type"),
		"DECIMAL"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,3,"data_type"),
		"NUMERIC"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,4,"data_type"),"FLOAT"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,5,"data_type"),
		"DOUBLE PRECISION"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,6,"data_type"),"DATE"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,7,"data_type"),"TIME"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,8,"data_type"),"CHAR"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,9,"data_type"),
		"VARCHAR"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,10,"data_type"),
		"TIMESTAMP"));
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,11,"data_type"),
		"BLOB SUB_TYPE BINARY"));
	printf("\n");


	// column list - auto_increment, primary key
	printf("COLUMN LIST - ""auto_increment, primary key: \n");
	assertTrue(sqlrcur_getColumnList(cur,"testtable2",NULL));
	assertTrue(strstr(sqlrcur_getFieldByName(cur,0,"extra"),
		"auto_increment")!=NULL);
	assertTrue(strstr(sqlrcur_getFieldByName(cur,0,"column_key"),
		"PRI")!=NULL);
	assertFalse(strstr(sqlrcur_getFieldByName(cur,1,"extra"),
		"auto_increment")!=NULL);
	assertFalse(strstr(sqlrcur_getFieldByName(cur,1,"column_key"),
		"PRI")!=NULL);
	printf("\n");
	assertTrue(sqlrcur_getColumnList(cur,"testtable3",NULL));
	assertFalse(strstr(sqlrcur_getFieldByName(cur,0,"extra"),
		"auto_increment")!=NULL);
	assertTrue(strstr(sqlrcur_getFieldByName(cur,0,"column_key"),
		"PRI")!=NULL);
	printf("\n");


	// primary keys list
	printf("PRIMARY KEYS LIST: \n");
	assertTrue(sqlrcur_getPrimaryKeysList(cur,"testtable2",NULL));
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
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,0,"table"),"TESTTABLE2"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"seq_in_index"),"1");
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,0,"column_name"),"COL1"));
	assertTrue(sqlrcur_getFieldByName(cur,0,"key_name")!=NULL &&strlen(
		sqlrcur_getFieldByName(cur,0,"key_name"))>0);
	printf("\n");


	// key and index list
	printf("KEY AND INDEX LIST: \n");
	assertTrue(sqlrcur_getKeyAndIndexList(cur,"testtable2",NULL));
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
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,0,"table"),"TESTTABLE2"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"non_unique"),"0");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"seq_in_index"),"1");
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,0,"column_name"),"COL1"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"collation"),"A");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"index_type"),"3");
	assertTrue(sqlrcur_getFieldByName(cur,0,"key_name")!=NULL &&strlen(
		sqlrcur_getFieldByName(cur,0,"key_name"))>0);
	printf("\n");


	// procedure list
	printf("PROCEDURE LIST: \n");
	assertTrue(sqlrcur_getProcedureList(cur,NULL));
	assertInResultSet(cur,"routine_name","TESTPROC");
	assertInResultSet(cur,"routine_name","TESTPROC1");
	printf("\n");


	// procedure parameter list
	printf("PROCEDURE PARAMETER LIST: \n");
	assertTrue(sqlrcur_getProcedureParameterList(cur,"testproc",NULL));
	assertEqStr(sqlrcur_getColumnName(cur,0),"parameter_name");
	assertEqStr(sqlrcur_getColumnName(cur,1),"parameter_mode");
	assertEqStr(sqlrcur_getColumnName(cur,2),"data_type");
	assertEqStr(sqlrcur_getColumnName(cur,3),"character_maximum_length");
	assertEqStr(sqlrcur_getColumnName(cur,4),"ordinal_position");
	assertEqInt(sqlrcur_rowCount(cur),8);
	assertEqStr(sqlrcur_getFieldByName(cur,0,"parameter_name"),"OUT1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"parameter_mode"),"4");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"INTEGER");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"ordinal_position"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"parameter_name"),"OUT2");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"parameter_mode"),"4");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"data_type"),"FLOAT");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"ordinal_position"),"2");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"parameter_name"),"OUT3");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"parameter_mode"),"4");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"data_type"),"VARCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"ordinal_position"),"3");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"parameter_name"),"OUT4");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"parameter_mode"),"4");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"data_type"),
		"BLOB SUB_TYPE BINARY");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"ordinal_position"),"4");
	assertEqStr(sqlrcur_getFieldByName(cur,4,"parameter_name"),"IN1");
	assertEqStr(sqlrcur_getFieldByName(cur,4,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,4,"data_type"),"INTEGER");
	assertEqStr(sqlrcur_getFieldByName(cur,4,"ordinal_position"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,5,"parameter_name"),"IN2");
	assertEqStr(sqlrcur_getFieldByName(cur,5,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,5,"data_type"),"FLOAT");
	assertEqStr(sqlrcur_getFieldByName(cur,5,"ordinal_position"),"2");
	assertEqStr(sqlrcur_getFieldByName(cur,6,"parameter_name"),"IN3");
	assertEqStr(sqlrcur_getFieldByName(cur,6,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,6,"data_type"),"VARCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,6,"ordinal_position"),"3");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"parameter_name"),"IN4");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"data_type"),
		"BLOB SUB_TYPE BINARY");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"ordinal_position"),"4");
	printf("\n");


	// invalid queries
	printf("INVALID QUERIES: \n");
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable1 "
		"order by "
		"	testinteger "));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable1 "
		"order by "
		"	testinteger "));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable1 "
		"order by "
		"	testinteger "));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable1 "
		"order by "
		"	testinteger "));
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,"insert into testtable1 "
		"values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(cur,"insert into testtable1 "
		"values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(cur,"insert into testtable1 "
		"values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(cur,"insert into testtable1 "
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
