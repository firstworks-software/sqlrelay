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

	// instantiation
	con=sqlrcon_alloc("sqlrelay",9000,
			"/tmp/test.socket","testuser","testpassword",0,1);
	cur=sqlrcur_alloc(con);

	// get database type


	// identify
	printf("IDENTIFY: \n");
	assertEqStr(sqlrcon_identify(con),"sap");
	printf("\n");


	// ping
	printf("PING: \n");
	assertTrue(sqlrcon_ping(con));
	printf("\n");


	// isolation levels
	printf("ISOLATION LEVELS: \n");
	{
		const char	*isolationlevels[]={"1","0","2","3",NULL};
		const char	**il;
		for (il=isolationlevels; *il; il++) {
			assertTrue(sqlrcon_setIsolationLevel(con,*il));
			assertEqStr(sqlrcon_getIsolationLevel(con),*il);
			printf("\n");
		}
		// reset to the default isolation level
		assertTrue(sqlrcon_setIsolationLevel(con,isolationlevels[0]));
		printf("\n");
	}

	// drop existing table
	sqlrcur_sendQuery(cur,"drop table testtable");


	// create temptable
	printf("CREATE TEMPTABLE: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testint int, "
		"	testsmallint smallint, "
		"	testtinyint tinyint, "
		"	testreal real, "
		"	testfloat float, "
		"	testdecimal decimal(4,1), "
		"	testnumeric numeric(4,1), "
		"	testmoney money, "
		"	testsmallmoney smallmoney, "
		"	testdatetime datetime, "
		"	testsmalldatetime smalldatetime, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testbit bit)"));
	printf("\n");


	// begin transaction
	printf("BEGIN TRANSACTION: \n");
	//assertTrue(sqlrcur_sendQuery(cur,"begin tran"));
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
		"	1.1, "
		"	1.1, "
		"	1.1, "
		"	1.1, "
		"	1.00, "
		"	1.00, "
		"	'01-Jan-2001 01:00:00', "
		"	'01-Jan-2001 01:00:00', "
		"	'testchar1', "
		"	'testvarchar1', "
		"	1)"));
	printf("\n");


	// affected rows
	printf("AFFECTED ROWS: \n");
	assertEqInt(sqlrcur_affectedRows(cur),1);
	printf("\n");


	// bind by position
	printf("BIND BY POSITION: \n");
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	@var1, "
		"	@var2, "
		"	@var3, "
		"	@var4, "
		"	@var5, "
		"	@var6, "
		"	@var7, "
		"	@var8, "
		"	@var9, "
		"	@var10, "
		"	@var11, "
		"	@var12, "
		"	@var13, "
		"	@var14)");
	assertEqInt(sqlrcur_countBindVariables(cur),14);
	sqlrcur_inputBindLong(cur,"1",2);
	sqlrcur_inputBindLong(cur,"2",2);
	sqlrcur_inputBindLong(cur,"3",2);
	sqlrcur_inputBindDouble(cur,"4",2.2,2,1);
	sqlrcur_inputBindDouble(cur,"5",2.2,2,1);
	sqlrcur_inputBindDouble(cur,"6",2.2,2,1);
	sqlrcur_inputBindDouble(cur,"7",2.2,2,1);
	sqlrcur_inputBindDouble(cur,"8",2.00,3,2);
	sqlrcur_inputBindDouble(cur,"9",2.00,3,2);
	sqlrcur_inputBindString(cur,"10","01-Jan-2002 02:00:00");
	sqlrcur_inputBindString(cur,"11","01-Jan-2002 02:00:00");
	sqlrcur_inputBindString(cur,"12","testchar2");
	sqlrcur_inputBindString(cur,"13","testvarchar2");
	sqlrcur_inputBindLong(cur,"14",1);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",3);
	sqlrcur_inputBindLong(cur,"2",3);
	sqlrcur_inputBindLong(cur,"3",3);
	sqlrcur_inputBindDouble(cur,"4",3.3,2,1);
	sqlrcur_inputBindDouble(cur,"5",3.3,2,1);
	sqlrcur_inputBindDouble(cur,"6",3.3,2,1);
	sqlrcur_inputBindDouble(cur,"7",3.3,2,1);
	sqlrcur_inputBindDouble(cur,"8",3.00,3,2);
	sqlrcur_inputBindDouble(cur,"9",3.00,3,2);
	sqlrcur_inputBindString(cur,"10","01-Jan-2003 03:00:00");
	sqlrcur_inputBindString(cur,"11","01-Jan-2003 03:00:00");
	sqlrcur_inputBindString(cur,"12","testchar3");
	sqlrcur_inputBindString(cur,"13","testvarchar3");
	sqlrcur_inputBindLong(cur,"14",1);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",4);
	sqlrcur_inputBindLong(cur,"2",4);
	sqlrcur_inputBindLong(cur,"3",4);
	sqlrcur_inputBindDouble(cur,"4",4.4,2,1);
	sqlrcur_inputBindDouble(cur,"5",4.4,2,1);
	sqlrcur_inputBindDouble(cur,"6",4.4,2,1);
	sqlrcur_inputBindDouble(cur,"7",4.4,2,1);
	sqlrcur_inputBindDouble(cur,"8",4.00,3,2);
	sqlrcur_inputBindDouble(cur,"9",4.00,3,2);
	sqlrcur_inputBindString(cur,"10","01-Jan-2004 04:00:00");
	sqlrcur_inputBindString(cur,"11","01-Jan-2004 04:00:00");
	sqlrcur_inputBindString(cur,"12","testchar4");
	sqlrcur_inputBindString(cur,"13","testvarchar4");
	sqlrcur_inputBindLong(cur,"14",1);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// bind by name
	printf("BIND BY NAME: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",5);
	sqlrcur_inputBindLong(cur,"var2",5);
	sqlrcur_inputBindLong(cur,"var3",5);
	sqlrcur_inputBindDouble(cur,"var4",5.5,2,1);
	sqlrcur_inputBindDouble(cur,"var5",5.5,2,1);
	sqlrcur_inputBindDouble(cur,"var6",5.5,2,1);
	sqlrcur_inputBindDouble(cur,"var7",5.5,2,1);
	sqlrcur_inputBindDouble(cur,"var8",5.00,3,2);
	sqlrcur_inputBindDouble(cur,"var9",5.00,3,2);
	sqlrcur_inputBindString(cur,"var10","01-Jan-2005 05:00:00");
	sqlrcur_inputBindString(cur,"var11","01-Jan-2005 05:00:00");
	sqlrcur_inputBindString(cur,"var12","testchar5");
	sqlrcur_inputBindString(cur,"var13","testvarchar5");
	sqlrcur_inputBindLong(cur,"var14",1);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",6);
	sqlrcur_inputBindLong(cur,"var2",6);
	sqlrcur_inputBindLong(cur,"var3",6);
	sqlrcur_inputBindDouble(cur,"var4",6.6,2,1);
	sqlrcur_inputBindDouble(cur,"var5",6.6,2,1);
	sqlrcur_inputBindDouble(cur,"var6",6.6,2,1);
	sqlrcur_inputBindDouble(cur,"var7",6.6,2,1);
	sqlrcur_inputBindDouble(cur,"var8",6.00,3,2);
	sqlrcur_inputBindDouble(cur,"var9",6.00,3,2);
	sqlrcur_inputBindString(cur,"var10","01-Jan-2006 06:00:00");
	sqlrcur_inputBindString(cur,"var11","01-Jan-2006 06:00:00");
	sqlrcur_inputBindString(cur,"var12","testchar6");
	sqlrcur_inputBindString(cur,"var13","testvarchar6");
	sqlrcur_inputBindLong(cur,"var14",1);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",7);
	sqlrcur_inputBindLong(cur,"var2",7);
	sqlrcur_inputBindLong(cur,"var3",7);
	sqlrcur_inputBindDouble(cur,"var4",7.7,2,1);
	sqlrcur_inputBindDouble(cur,"var5",7.7,2,1);
	sqlrcur_inputBindDouble(cur,"var6",7.7,2,1);
	sqlrcur_inputBindDouble(cur,"var7",7.7,2,1);
	sqlrcur_inputBindDouble(cur,"var8",7.00,3,2);
	sqlrcur_inputBindDouble(cur,"var9",7.00,3,2);
	sqlrcur_inputBindString(cur,"var10","01-Jan-2007 07:00:00");
	sqlrcur_inputBindString(cur,"var11","01-Jan-2007 07:00:00");
	sqlrcur_inputBindString(cur,"var12","testchar7");
	sqlrcur_inputBindString(cur,"var13","testvarchar7");
	sqlrcur_inputBindLong(cur,"var14",1);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// bind by name with validation
	printf("BIND BY NAME WITH VALIDATION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",8);
	sqlrcur_inputBindLong(cur,"var2",8);
	sqlrcur_inputBindLong(cur,"var3",8);
	sqlrcur_inputBindDouble(cur,"var4",8.8,2,1);
	sqlrcur_inputBindDouble(cur,"var5",8.8,2,1);
	sqlrcur_inputBindDouble(cur,"var6",8.8,2,1);
	sqlrcur_inputBindDouble(cur,"var7",8.8,2,1);
	sqlrcur_inputBindDouble(cur,"var8",8.00,3,2);
	sqlrcur_inputBindDouble(cur,"var9",8.00,3,2);
	sqlrcur_inputBindString(cur,"var10","01-Jan-2008 08:00:00");
	sqlrcur_inputBindString(cur,"var11","01-Jan-2008 08:00:00");
	sqlrcur_inputBindString(cur,"var12","testchar8");
	sqlrcur_inputBindString(cur,"var13","testvarchar8");
	sqlrcur_inputBindLong(cur,"var14",1);
	sqlrcur_inputBindString(cur,"var15","junkvalue");
	sqlrcur_validateBinds(cur);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// select
	printf("SELECT: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testint "));
	printf("\n");


	// column count
	printf("COLUMN COUNT: \n");
	assertEqInt(sqlrcur_colCount(cur),14);
	printf("\n");


	// column names
	printf("COLUMN NAMES: \n");
	assertEqStr(sqlrcur_getColumnName(cur,0),"testint");
	assertEqStr(sqlrcur_getColumnName(cur,1),"testsmallint");
	assertEqStr(sqlrcur_getColumnName(cur,2),"testtinyint");
	assertEqStr(sqlrcur_getColumnName(cur,3),"testreal");
	assertEqStr(sqlrcur_getColumnName(cur,4),"testfloat");
	assertEqStr(sqlrcur_getColumnName(cur,5),"testdecimal");
	assertEqStr(sqlrcur_getColumnName(cur,6),"testnumeric");
	assertEqStr(sqlrcur_getColumnName(cur,7),"testmoney");
	assertEqStr(sqlrcur_getColumnName(cur,8),"testsmallmoney");
	assertEqStr(sqlrcur_getColumnName(cur,9),"testdatetime");
	assertEqStr(sqlrcur_getColumnName(cur,10),"testsmalldatetime");
	assertEqStr(sqlrcur_getColumnName(cur,11),"testchar");
	assertEqStr(sqlrcur_getColumnName(cur,12),"testvarchar");
	assertEqStr(sqlrcur_getColumnName(cur,13),"testbit");
	cols=sqlrcur_getColumnNames(cur);
	assertEqStr(cols[0],"testint");
	assertEqStr(cols[1],"testsmallint");
	assertEqStr(cols[2],"testtinyint");
	assertEqStr(cols[3],"testreal");
	assertEqStr(cols[4],"testfloat");
	assertEqStr(cols[5],"testdecimal");
	assertEqStr(cols[6],"testnumeric");
	assertEqStr(cols[7],"testmoney");
	assertEqStr(cols[8],"testsmallmoney");
	assertEqStr(cols[9],"testdatetime");
	assertEqStr(cols[10],"testsmalldatetime");
	assertEqStr(cols[11],"testchar");
	assertEqStr(cols[12],"testvarchar");
	assertEqStr(cols[13],"testbit");
	printf("\n");


	// column types
	printf("COLUMN TYPES: \n");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),"INT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testint"),"INT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,1),"SMALLINT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testsmallint"),"SMALLINT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,2),"TINYINT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testtinyint"),"TINYINT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,3),"REAL");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testreal"),"REAL");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,4),"FLOAT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testfloat"),"FLOAT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,5),"DECIMAL");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testdecimal"),"DECIMAL");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,6),"NUMERIC");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testnumeric"),"NUMERIC");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,7),"MONEY");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testmoney"),"MONEY");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,8),"SMALLMONEY");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testsmallmoney"),"SMALLMONEY");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,9),"DATETIME");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testdatetime"),"DATETIME");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,10),"SMALLDATETIME");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testsmalldatetime"),"SMALLDATETIME");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,11),"CHAR");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testchar"),"CHAR");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,12),"CHAR");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testvarchar"),"CHAR");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,13),"BIT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testbit"),"BIT");
	printf("\n");


	// column length
	printf("COLUMN LENGTH: \n");
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),4);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testint"),4);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,1),2);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testsmallint"),2);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,2),1);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testtinyint"),1);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,3),4);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testreal"),4);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,4),8);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testfloat"),8);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,5),35);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testdecimal"),35);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,6),35);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testnumeric"),35);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,7),8);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testmoney"),8);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,8),4);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testsmallmoney"),4);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,9),8);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testdatetime"),8);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,10),4);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testsmalldatetime"),4);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,11),40);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testchar"),40);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,12),40);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testvarchar"),40);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,13),1);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testbit"),1);
	printf("\n");


	// longest column
	printf("LONGEST COLUMN: \n");
	assertEqInt(sqlrcur_getLongestByIndex(cur,0),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"testint"),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,1),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"testsmallint"),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,2),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"testtinyint"),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,3),18);
	assertEqInt(sqlrcur_getLongestByName(cur,"testreal"),18);
	assertEqInt(sqlrcur_getLongestByIndex(cur,4),18);
	assertEqInt(sqlrcur_getLongestByName(cur,"testfloat"),18);
	assertEqInt(sqlrcur_getLongestByIndex(cur,5),3);
	assertEqInt(sqlrcur_getLongestByName(cur,"testdecimal"),3);
	assertEqInt(sqlrcur_getLongestByIndex(cur,6),3);
	assertEqInt(sqlrcur_getLongestByName(cur,"testnumeric"),3);
	assertEqInt(sqlrcur_getLongestByIndex(cur,7),4);
	assertEqInt(sqlrcur_getLongestByName(cur,"testmoney"),4);
	assertEqInt(sqlrcur_getLongestByIndex(cur,8),4);
	assertEqInt(sqlrcur_getLongestByName(cur,"testsmallmoney"),4);
	assertEqInt(sqlrcur_getLongestByIndex(cur,9),19);
	assertEqInt(sqlrcur_getLongestByName(cur,"testdatetime"),19);
	assertEqInt(sqlrcur_getLongestByIndex(cur,10),19);
	assertEqInt(sqlrcur_getLongestByName(cur,"testsmalldatetime"),19);
	assertEqInt(sqlrcur_getLongestByIndex(cur,11),40);
	assertEqInt(sqlrcur_getLongestByName(cur,"testchar"),40);
	assertEqInt(sqlrcur_getLongestByIndex(cur,12),12);
	assertEqInt(sqlrcur_getLongestByName(cur,"testvarchar"),12);
	assertEqInt(sqlrcur_getLongestByIndex(cur,13),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"testbit"),1);
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
	//assertEqStr(sqlrcur_getFieldByIndex(cur,0,3),"1.1");
	//assertEqStr(sqlrcur_getFieldByIndex(cur,0,4),"1.1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,5),"1.1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,6),"1.1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,7),"1.00");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,8),"1.00");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,9),"Jan  1 2001  1:00AM");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,10),"Jan  1 2001  1:00AM");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,11),"testchar1                               ");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,12),"testvarchar1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,13),"1");
	printf("\n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,2),"8");
	//assertEqStr(sqlrcur_getFieldByIndex(cur,7,3),"8.8");
	//assertEqStr(sqlrcur_getFieldByIndex(cur,7,4),"8.8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,5),"8.8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,6),"8.8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,7),"8.00");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,8),"8.00");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,9),"Jan  1 2008  8:00AM");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,10),"Jan  1 2008  8:00AM");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,11),"testchar8                               ");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,12),"testvarchar8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,13),"1");
	printf("\n");


	// field lengths by index
	printf("FIELD LENGTHS BY INDEX: \n");
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,1),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,2),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,3),18);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,4),18);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,5),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,6),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,7),4);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,8),4);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,9),19);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,10),19);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,11),40);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,12),12);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,13),1);
	printf("\n");
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,0),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,1),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,2),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,3),18);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,4),18);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,5),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,6),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,7),4);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,8),4);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,9),19);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,10),19);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,11),40);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,12),12);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,13),1);
	printf("\n");


	// fields by name
	printf("FIELDS BY NAME: \n");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testint"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testsmallint"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testtinyint"),"1");
	//assertEqStr(sqlrcur_getFieldByName(cur,0,"testreal"),"1.1");
	//assertEqStr(sqlrcur_getFieldByName(cur,0,"testfloat"),"1.1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testdecimal"),"1.1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testnumeric"),"1.1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testmoney"),"1.00");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testsmallmoney"),"1.00");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testdatetime"),"Jan  1 2001  1:00AM");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testsmalldatetime"),"Jan  1 2001  1:00AM");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testchar"),"testchar1                               ");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testvarchar"),"testvarchar1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testbit"),"1");
	printf("\n");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testint"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testsmallint"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testtinyint"),"8");
	//assertEqStr(sqlrcur_getFieldByName(cur,7,"testreal"),"8.8");
	//assertEqStr(sqlrcur_getFieldByName(cur,7,"testfloat"),"8.8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testdecimal"),"8.8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testnumeric"),"8.8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testmoney"),"8.00");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testsmallmoney"),"8.00");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testdatetime"),"Jan  1 2008  8:00AM");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testsmalldatetime"),"Jan  1 2008  8:00AM");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testchar"),"testchar8                               ");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testvarchar"),"testvarchar8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testbit"),"1");
	printf("\n");


	// field lengths by name
	printf("FIELD LENGTHS BY NAME: \n");
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testsmallint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testtinyint"),1);
	//assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testreal"),3);
	//assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testdecimal"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testnumeric"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testmoney"),4);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testsmallmoney"),4);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testdatetime"),19);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testsmalldatetime"),19);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testchar"),40);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testvarchar"),12);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testbit"),1);
	printf("\n");
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testsmallint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testtinyint"),1);
	//assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testreal"),3);
	//assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testdecimal"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testnumeric"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testmoney"),4);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testsmallmoney"),4);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testdatetime"),19);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testsmalldatetime"),19);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testchar"),40);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testvarchar"),12);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testbit"),1);
	printf("\n");


	// fields by array
	printf("FIELDS BY ARRAY: \n");
	fields=sqlrcur_getRow(cur,0);
	assertEqStr(fields[0],"1");
	assertEqStr(fields[1],"1");
	assertEqStr(fields[2],"1");
	//assertEqStr(fields[3],"1.1");
	//assertEqStr(fields[4],"1.1");
	assertEqStr(fields[5],"1.1");
	assertEqStr(fields[6],"1.1");
	assertEqStr(fields[7],"1.00");
	assertEqStr(fields[8],"1.00");
	assertEqStr(fields[9],"Jan  1 2001  1:00AM");
	assertEqStr(fields[10],"Jan  1 2001  1:00AM");
	assertEqStr(fields[11],"testchar1                               ");
	assertEqStr(fields[12],"testvarchar1");
	assertEqStr(fields[13],"1");
	printf("\n");


	// field lengths by array
	printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=sqlrcur_getRowLengths(cur,0);
	assertEqInt(fieldlens[0],1);
	assertEqInt(fieldlens[1],1);
	assertEqInt(fieldlens[2],1);
	//assertEqInt(fieldlens[3],3);
	//assertEqInt(fieldlens[4],3);
	assertEqInt(fieldlens[5],3);
	assertEqInt(fieldlens[6],3);
	assertEqInt(fieldlens[7],4);
	assertEqInt(fieldlens[8],4);
	assertEqInt(fieldlens[9],19);
	assertEqInt(fieldlens[10],19);
	assertEqInt(fieldlens[11],40);
	assertEqInt(fieldlens[12],12);
	assertEqInt(fieldlens[13],1);
	printf("\n");


	// individual substitutions
	printf("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"select $(var1),'$(var2)',$(var3)");
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
	sqlrcur_prepareQuery(cur,"select $(var1),$(var2),$(var3)");
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
	sqlrcur_prepareQuery(cur,"select '$(var1)','$(var2)','$(var3)'");
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
	sqlrcur_prepareQuery(cur,"select $(var1),$(var2),$(var3)");
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
	assertTrue(sqlrcur_sendQuery(cur,"select NULL,1,NULL"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select NULL,1,NULL"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"1");
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
		"	testint "));
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
		"	testint "));
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
		"	testint "));
	assertEqStr(sqlrcur_getColumnName(cur,0),"testint");
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),4);
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),"INT");
	printf("\n");


	// suspended session
	printf("SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testint "));
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
		"	testint "));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
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
		"	testint "));
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
		"	testint "));
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
		"	testint "));
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqStr(filename,"cachefile1");
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	free(filename);
	printf("\n");


	// column count for cached result set
	printf("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqInt(sqlrcur_colCount(cur),14);
	printf("\n");


	// column names for cached result set
	printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqStr(sqlrcur_getColumnName(cur,0),"testint");
	assertEqStr(sqlrcur_getColumnName(cur,1),"testsmallint");
	assertEqStr(sqlrcur_getColumnName(cur,2),"testtinyint");
	assertEqStr(sqlrcur_getColumnName(cur,3),"testreal");
	assertEqStr(sqlrcur_getColumnName(cur,4),"testfloat");
	assertEqStr(sqlrcur_getColumnName(cur,5),"testdecimal");
	assertEqStr(sqlrcur_getColumnName(cur,6),"testnumeric");
	assertEqStr(sqlrcur_getColumnName(cur,7),"testmoney");
	assertEqStr(sqlrcur_getColumnName(cur,8),"testsmallmoney");
	assertEqStr(sqlrcur_getColumnName(cur,9),"testdatetime");
	assertEqStr(sqlrcur_getColumnName(cur,10),"testsmalldatetime");
	assertEqStr(sqlrcur_getColumnName(cur,11),"testchar");
	assertEqStr(sqlrcur_getColumnName(cur,12),"testvarchar");
	assertEqStr(sqlrcur_getColumnName(cur,13),"testbit");
	cols=sqlrcur_getColumnNames(cur);
	assertEqStr(cols[0],"testint");
	assertEqStr(cols[1],"testsmallint");
	assertEqStr(cols[2],"testtinyint");
	assertEqStr(cols[3],"testreal");
	assertEqStr(cols[4],"testfloat");
	assertEqStr(cols[5],"testdecimal");
	assertEqStr(cols[6],"testnumeric");
	assertEqStr(cols[7],"testmoney");
	assertEqStr(cols[8],"testsmallmoney");
	assertEqStr(cols[9],"testdatetime");
	assertEqStr(cols[10],"testsmalldatetime");
	assertEqStr(cols[11],"testchar");
	assertEqStr(cols[12],"testvarchar");
	assertEqStr(cols[13],"testbit");
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
		"	testint "));
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
		"	testint "));
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


	// finished suspended session
	printf("FINISHED SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testint "));
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

	// drop existing table
	sqlrcur_sendQuery(cur,"drop table testtable");


	// invalid queries
	printf("INVALID QUERIES: \n");
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testint "));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testint "));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testint "));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testint "));
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	2, "
		"	3, "
		"	4)"));
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	2, "
		"	3, "
		"	4)"));
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	2, "
		"	3, "
		"	4)"));
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
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
