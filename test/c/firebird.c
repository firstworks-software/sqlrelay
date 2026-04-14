// Copyright (c) David Muse
// See the file COPYING for more information.

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

	const char	*bindvars[]={"1","2","3","4","5","6",
				"7","8","9","10","11",NULL};
	const char	*bindvals[]={"4","4","4.4","4.4","4.4","4.4",
				"01-JAN-2004","04:00:00",
				"testchar4","testvarchar4",NULL};
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
	int16_t		id;
	char		*filename;

	// instantiation
	con=sqlrcon_alloc("sqlrelay",9000,
			"/tmp/test.socket","testuser","testpassword",0,1);
	cur=sqlrcur_alloc(con);

	// get database type


	// identify
	printf("IDENTIFY: \n");
	assertEqStr(sqlrcon_identify(con),"firebird");
	printf("\n");


	// ping
	printf("PING: \n");
	assertTrue(sqlrcon_ping(con));
	printf("\n");


	// isolation levels
	printf("ISOLATION LEVELS: \n");
	// though firebird does support a "set transaction ..." statement to
	// set the isolation level, it looks like, in firebird, you can really
	// only set it through the TPB at the start of a transaction, so
	// attempts to set it should fail
	assertFalse(sqlrcon_setIsolationLevel(con,"read committed"));
	assertEqStr(sqlrcon_getIsolationLevel(con),"read committed");
	printf("\n");

	// clean up table
	sqlrcur_sendQuery(cur,"delete from testtable");
	sqlrcon_commit(con);


	// insert
	printf("INSERT: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	1, "
		"	1.1, "
		"	1.1, "
		"	1.1, "
		"	1.1, "
		"	'01-JAN-2001', "
		"	'01:00:00', "
		"	'testchar1', "
		"	'testvarchar1', "
		"	NULL, "
		"	NULL)"));
	printf("\n");


	// bind by position
	printf("BIND BY POSITION: \n");
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
		"	NULL)");
	assertEqInt(sqlrcur_countBindVariables(cur),11);
	sqlrcur_inputBindLong(cur,"1",2);
	sqlrcur_inputBindLong(cur,"2",2);
	sqlrcur_inputBindDouble(cur,"3",2.2,2,1);
	sqlrcur_inputBindDouble(cur,"4",2.2,2,1);
	sqlrcur_inputBindDouble(cur,"5",2.2,2,1);
	sqlrcur_inputBindDouble(cur,"6",2.2,2,1);
	sqlrcur_inputBindString(cur,"7","01-JAN-2002");
	sqlrcur_inputBindString(cur,"8","02:00:00");
	sqlrcur_inputBindString(cur,"9","testchar2");
	sqlrcur_inputBindString(cur,"10","testvarchar2");
	sqlrcur_inputBindString(cur,"11",(char *)NULL);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",3);
	sqlrcur_inputBindLong(cur,"2",3);
	sqlrcur_inputBindDouble(cur,"3",3.3,2,1);
	sqlrcur_inputBindDouble(cur,"4",3.3,2,1);
	sqlrcur_inputBindDouble(cur,"5",3.3,2,1);
	sqlrcur_inputBindDouble(cur,"6",3.3,2,1);
	sqlrcur_inputBindString(cur,"7","01-JAN-2003");
	sqlrcur_inputBindString(cur,"8","03:00:00");
	sqlrcur_inputBindString(cur,"9","testchar3");
	sqlrcur_inputBindString(cur,"10","testvarchar3");
	sqlrcur_inputBindString(cur,"11",(char *)NULL);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// array of binds by position
	printf("ARRAY OF BINDS BY POSITION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindStrings(cur,bindvars,bindvals);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// insert
	printf("INSERT: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	5, "
		"	5, "
		"	5.5, "
		"	5.5, "
		"	5.5, "
		"	5.5, "
		"	'01-JAN-2005', "
		"	'05:00:00', "
		"	'testchar5', "
		"	'testvarchar5', "
		"	NULL, "
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	6, "
		"	6, "
		"	6.6, "
		"	6.6, "
		"	6.6, "
		"	6.6, "
		"	'01-JAN-2006', "
		"	'06:00:00', "
		"	'testchar6', "
		"	'testvarchar6', "
		"	NULL, "
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	7, "
		"	7, "
		"	7.7, "
		"	7.7, "
		"	7.7, "
		"	7.7, "
		"	'01-JAN-2007', "
		"	'07:00:00', "
		"	'testchar7', "
		"	'testvarchar7', "
		"	NULL, "
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	8, "
		"	8, "
		"	8.8, "
		"	8.8, "
		"	8.8, "
		"	8.8, "
		"	'01-JAN-2008', "
		"	'08:00:00', "
		"	'testchar8', "
		"	'testvarchar8', "
		"	NULL, "
		"	NULL)"));
	printf("\n");


	// affected rows
	printf("AFFECTED ROWS: \n");
	assertEqInt(sqlrcur_affectedRows(cur),1);
	printf("\n");


	// stored procedure
	printf("STORED PROCEDURE: \n");
	sqlrcur_prepareQuery(cur,"select * from testproc(?,?,?,NULL)");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",1.1,2,1);
	sqlrcur_inputBindString(cur,"3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,(uint32_t)0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"1.1000");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"hello");
	sqlrcur_prepareQuery(cur,"execute procedure testproc ?, ?, ?, NULL");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",1.1,2,1);
	sqlrcur_inputBindString(cur,"3","hello");
	sqlrcur_defineOutputBindInteger(cur,"1");
	sqlrcur_defineOutputBindDouble(cur,"2");
	sqlrcur_defineOutputBindString(cur,"3",20);
	sqlrcur_defineOutputBindBlob(cur,"4");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"1"),1);
	//assertEqDbl(sqlrcur_getOutputBindDouble(cur,"2"),1.1);
	assertEqStr(sqlrcur_getOutputBindString(cur,"3"),"hello               ");
	printf("\n");


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
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTDOUBLE"),"DOUBLE PRECISION");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,6),"DATE");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTDATE"),"DATE");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,7),"TIME");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTTIME"),"TIME");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,8),"CHAR");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTCHAR"),"CHAR");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,9),"VARCHAR");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTVARCHAR"),"VARCHAR");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,10),"TIMESTAMP");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTTIMESTAMP"),"TIMESTAMP");
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
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"1.10");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,3),"1.10");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,4),"1.1000");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,5),"1.1000");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,6),"2001:01:01");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,7),"01:00:00");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,8),"testchar1                                         ");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,9),"testvarchar1");
	printf("\n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,2),"8.80");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,3),"8.80");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,4),"8.8000");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,5),"8.8000");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,6),"2008:01:01");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,7),"08:00:00");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,8),"testchar8                                         ");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,9),"testvarchar8");
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
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTDECIMAL"),"1.10");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTNUMERIC"),"1.10");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTFLOAT"),"1.1000");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTDOUBLE"),"1.1000");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTDATE"),"2001:01:01");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTTIME"),"01:00:00");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTCHAR"),"testchar1                                         ");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTVARCHAR"),"testvarchar1");
	printf("\n");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTINTEGER"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTSMALLINT"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTDECIMAL"),"8.80");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTNUMERIC"),"8.80");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTFLOAT"),"8.8000");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTDOUBLE"),"8.8000");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTDATE"),"2008:01:01");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTTIME"),"08:00:00");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTCHAR"),"testchar8                                         ");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTVARCHAR"),"testvarchar8");
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
	assertEqStr(fields[2],"1.10");
	assertEqStr(fields[3],"1.10");
	assertEqStr(fields[4],"1.1000");
	assertEqStr(fields[5],"1.1000");
	assertEqStr(fields[6],"2001:01:01");
	assertEqStr(fields[7],"01:00:00");
	assertEqStr(fields[8],"testchar1                                         ");
	assertEqStr(fields[9],"testvarchar1");
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


	// individual substitutions
	printf("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,
		"select "
		"	$(var1), "
		"	'$(var2)', "
		"	'$(var3)' "
		"from "
		"	rdb$database ");
	sqlrcur_subLong(cur,"var1",1);
	sqlrcur_subString(cur,"var2","hello");
	sqlrcur_subDouble(cur,"var3",10.5556,6,4);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// fields
	printf("FIELDS: \n");
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
	printf("\n");


	// fields
	printf("FIELDS: \n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"hi");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"bye");
	printf("\n");


	// array substitutions
	printf("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,
		"select "
		"	$(var1), "
		"	$(var2), "
		"	$(var3) "
		"from "
		"	rdb$database ");
	sqlrcur_subLongs(cur,subvars,subvallongs);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// fields
	printf("FIELDS: \n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"2");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"3");
	printf("\n");


	// array substitutions
	printf("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,
		"select "
		"	$(var1), "
		"	$(var2), "
		"	$(var3) "
		"from "
		"	rdb$database ");
	sqlrcur_subDoubles(cur,subvars,subvaldoubles,precs,scales);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// fields
	printf("FIELDS: \n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"10.55");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"10.556");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"10.5556");
	printf("\n");


	// nulls as nulls
	printf("NULLS AS NULLS: \n");
	sqlrcur_getNullsAsNulls(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	1, "
		"	NULL, "
		"	NULL "
		"from "
		"	rdb$database "));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	1, "
		"	NULL, "
		"	NULL "
		"from "
		"	rdb$database "));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"");
	sqlrcur_getNullsAsNulls(cur);
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
	printf("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqInt(sqlrcur_colCount(cur),12);
	printf("\n");


	// column names for cached result set
	printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
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
	printf("\n");


	// cached result set with result set buffer size
	printf("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n");
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
	printf("FROM ONE CACHE FILE TO ANOTHER: \n");
	sqlrcur_cacheToFile(cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile1"));
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile2"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	printf("\n");


	// from one cache file to another with result set buffer size
	printf("FROM ONE CACHE FILE TO ANOTHER "
		"WITH RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile1"));
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile2"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize(cur,0);
	printf("\n");


	// cached result set with suspend and result set buffer size
	printf("CACHED RESULT SET WITH SUSPEND "
		"AND RESULT SET BUFFER SIZE: \n");
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


	// commit
	printf("COMMIT: \n");
	secondcon=sqlrcon_alloc("sqlrelay",9000,
			"/tmp/test.socket","testuser","testpassword",0,1);
	secondcur=sqlrcur_alloc(secondcon);
	assertTrue(sqlrcur_sendQuery(secondcur,
		"select "
		"	count(*) "
		"from "
		"	testtable "));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"0");
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_sendQuery(secondcur,
		"select "
		"	count(*) "
		"from "
		"	testtable "));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"8");
	assertTrue(sqlrcon_autoCommitOn(con));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	10, "
		"	10, "
		"	10.1, "
		"	10.1, "
		"	10.1, "
		"	10.1, "
		"	'01-JAN-2010', "
		"	'10:00:00', "
		"	'testchar10', "
		"	'testvarchar10', "
		"	NULL, "
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery(secondcur,
		"select "
		"	count(*) "
		"from "
		"	testtable "));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"9");
	assertTrue(sqlrcon_autoCommitOff(con));
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
	assertTrue(sqlrcur_resumeResultSet(cur,id));
	assertEqStr(sqlrcur_getFieldByIndex(cur,4,0),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,5,0),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,6,0),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),NULL);
	printf("\n");

	// clean up table
	sqlrcon_commit(con);
	sqlrcur_sendQuery(cur,"delete from testtable");
	sqlrcon_commit(con);
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
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable1 "
		"values ("
		"	1, "
		"	2, "
		"	3, "
		"	4)"));
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable1 "
		"values ("
		"	1, "
		"	2, "
		"	3, "
		"	4)"));
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable1 "
		"values ("
		"	1, "
		"	2, "
		"	3, "
		"	4)"));
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable1 "
		"values ("
		"	1, "
		"	2, "
		"	3, "
		"	4)"));
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	printf("\n");

	reportTestStatus();

	return status;
}
