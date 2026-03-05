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
	assertEqualsString(sqlrcon_identify(con),"sap");
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
			assertEqualsString(sqlrcon_getIsolationLevel(con),*il);
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
	assertEqualsInt(sqlrcur_affectedRows(cur),1);
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
	assertEqualsInt(sqlrcur_countBindVariables(cur),14);
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
	assertEqualsInt(sqlrcur_colCount(cur),14);
	printf("\n");


	// column names
	printf("COLUMN NAMES: \n");
	assertEqualsString(sqlrcur_getColumnName(cur,0),"testint");
	assertEqualsString(sqlrcur_getColumnName(cur,1),"testsmallint");
	assertEqualsString(sqlrcur_getColumnName(cur,2),"testtinyint");
	assertEqualsString(sqlrcur_getColumnName(cur,3),"testreal");
	assertEqualsString(sqlrcur_getColumnName(cur,4),"testfloat");
	assertEqualsString(sqlrcur_getColumnName(cur,5),"testdecimal");
	assertEqualsString(sqlrcur_getColumnName(cur,6),"testnumeric");
	assertEqualsString(sqlrcur_getColumnName(cur,7),"testmoney");
	assertEqualsString(sqlrcur_getColumnName(cur,8),"testsmallmoney");
	assertEqualsString(sqlrcur_getColumnName(cur,9),"testdatetime");
	assertEqualsString(sqlrcur_getColumnName(cur,10),"testsmalldatetime");
	assertEqualsString(sqlrcur_getColumnName(cur,11),"testchar");
	assertEqualsString(sqlrcur_getColumnName(cur,12),"testvarchar");
	assertEqualsString(sqlrcur_getColumnName(cur,13),"testbit");
	cols=sqlrcur_getColumnNames(cur);
	assertEqualsString(cols[0],"testint");
	assertEqualsString(cols[1],"testsmallint");
	assertEqualsString(cols[2],"testtinyint");
	assertEqualsString(cols[3],"testreal");
	assertEqualsString(cols[4],"testfloat");
	assertEqualsString(cols[5],"testdecimal");
	assertEqualsString(cols[6],"testnumeric");
	assertEqualsString(cols[7],"testmoney");
	assertEqualsString(cols[8],"testsmallmoney");
	assertEqualsString(cols[9],"testdatetime");
	assertEqualsString(cols[10],"testsmalldatetime");
	assertEqualsString(cols[11],"testchar");
	assertEqualsString(cols[12],"testvarchar");
	assertEqualsString(cols[13],"testbit");
	printf("\n");


	// column types
	printf("COLUMN TYPES: \n");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),"INT");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testint"),"INT");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,1),"SMALLINT");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testsmallint"),"SMALLINT");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,2),"TINYINT");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testtinyint"),"TINYINT");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,3),"REAL");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testreal"),"REAL");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,4),"FLOAT");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testfloat"),"FLOAT");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,5),"DECIMAL");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testdecimal"),"DECIMAL");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,6),"NUMERIC");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testnumeric"),"NUMERIC");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,7),"MONEY");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testmoney"),"MONEY");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,8),"SMALLMONEY");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testsmallmoney"),"SMALLMONEY");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,9),"DATETIME");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testdatetime"),"DATETIME");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,10),"SMALLDATETIME");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testsmalldatetime"),"SMALLDATETIME");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,11),"CHAR");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testchar"),"CHAR");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,12),"CHAR");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testvarchar"),"CHAR");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,13),"BIT");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testbit"),"BIT");
	printf("\n");


	// column length
	printf("COLUMN LENGTH: \n");
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,0),4);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testint"),4);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,1),2);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testsmallint"),2);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,2),1);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testtinyint"),1);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,3),4);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testreal"),4);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,4),8);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testfloat"),8);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,5),35);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testdecimal"),35);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,6),35);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testnumeric"),35);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,7),8);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testmoney"),8);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,8),4);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testsmallmoney"),4);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,9),8);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testdatetime"),8);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,10),4);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testsmalldatetime"),4);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,11),40);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testchar"),40);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,12),40);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testvarchar"),40);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,13),1);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testbit"),1);
	printf("\n");


	// longest column
	printf("LONGEST COLUMN: \n");
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,0),1);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testint"),1);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,1),1);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testsmallint"),1);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,2),1);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testtinyint"),1);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,3),18);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testreal"),18);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,4),18);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testfloat"),18);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,5),3);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testdecimal"),3);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,6),3);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testnumeric"),3);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,7),4);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testmoney"),4);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,8),4);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testsmallmoney"),4);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,9),19);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testdatetime"),19);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,10),19);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testsmalldatetime"),19);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,11),40);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testchar"),40);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,12),12);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testvarchar"),12);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,13),1);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testbit"),1);
	printf("\n");


	// row count
	printf("ROW COUNT: \n");
	assertEqualsInt(sqlrcur_rowCount(cur),8);
	printf("\n");


	// total rows
	printf("TOTAL ROWS: \n");
	assertEqualsInt(sqlrcur_totalRows(cur),0);
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
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"1");
	//assertEqualsString(sqlrcur_getFieldByIndex(cur,0,3),"1.1");
	//assertEqualsString(sqlrcur_getFieldByIndex(cur,0,4),"1.1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,5),"1.1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,6),"1.1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,7),"1.00");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,8),"1.00");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,9),"Jan  1 2001  1:00AM");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,10),"Jan  1 2001  1:00AM");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,11),"testchar1                               ");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,12),"testvarchar1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,13),"1");
	printf("\n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,1),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,2),"8");
	//assertEqualsString(sqlrcur_getFieldByIndex(cur,7,3),"8.8");
	//assertEqualsString(sqlrcur_getFieldByIndex(cur,7,4),"8.8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,5),"8.8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,6),"8.8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,7),"8.00");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,8),"8.00");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,9),"Jan  1 2008  8:00AM");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,10),"Jan  1 2008  8:00AM");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,11),"testchar8                               ");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,12),"testvarchar8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,13),"1");
	printf("\n");


	// field lengths by index
	printf("FIELD LENGTHS BY INDEX: \n");
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,0),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,1),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,2),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,3),18);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,4),18);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,5),3);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,6),3);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,7),4);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,8),4);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,9),19);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,10),19);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,11),40);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,12),12);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,13),1);
	printf("\n");
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,0),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,1),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,2),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,3),18);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,4),18);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,5),3);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,6),3);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,7),4);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,8),4);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,9),19);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,10),19);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,11),40);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,12),12);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,13),1);
	printf("\n");


	// fields by name
	printf("FIELDS BY NAME: \n");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testint"),"1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testsmallint"),"1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testtinyint"),"1");
	//assertEqualsString(sqlrcur_getFieldByName(cur,0,"testreal"),"1.1");
	//assertEqualsString(sqlrcur_getFieldByName(cur,0,"testfloat"),"1.1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testdecimal"),"1.1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testnumeric"),"1.1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testmoney"),"1.00");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testsmallmoney"),"1.00");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testdatetime"),"Jan  1 2001  1:00AM");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testsmalldatetime"),"Jan  1 2001  1:00AM");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testchar"),"testchar1                               ");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testvarchar"),"testvarchar1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testbit"),"1");
	printf("\n");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testint"),"8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testsmallint"),"8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testtinyint"),"8");
	//assertEqualsString(sqlrcur_getFieldByName(cur,7,"testreal"),"8.8");
	//assertEqualsString(sqlrcur_getFieldByName(cur,7,"testfloat"),"8.8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testdecimal"),"8.8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testnumeric"),"8.8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testmoney"),"8.00");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testsmallmoney"),"8.00");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testdatetime"),"Jan  1 2008  8:00AM");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testsmalldatetime"),"Jan  1 2008  8:00AM");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testchar"),"testchar8                               ");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testvarchar"),"testvarchar8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testbit"),"1");
	printf("\n");


	// field lengths by name
	printf("FIELD LENGTHS BY NAME: \n");
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testint"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testsmallint"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testtinyint"),1);
	//assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testreal"),3);
	//assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testfloat"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testdecimal"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testnumeric"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testmoney"),4);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testsmallmoney"),4);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testdatetime"),19);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testsmalldatetime"),19);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testchar"),40);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testvarchar"),12);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testbit"),1);
	printf("\n");
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testint"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testsmallint"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testtinyint"),1);
	//assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testreal"),3);
	//assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testfloat"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testdecimal"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testnumeric"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testmoney"),4);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testsmallmoney"),4);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testdatetime"),19);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testsmalldatetime"),19);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testchar"),40);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testvarchar"),12);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testbit"),1);
	printf("\n");


	// fields by array
	printf("FIELDS BY ARRAY: \n");
	fields=sqlrcur_getRow(cur,0);
	assertEqualsString(fields[0],"1");
	assertEqualsString(fields[1],"1");
	assertEqualsString(fields[2],"1");
	//assertEqualsString(fields[3],"1.1");
	//assertEqualsString(fields[4],"1.1");
	assertEqualsString(fields[5],"1.1");
	assertEqualsString(fields[6],"1.1");
	assertEqualsString(fields[7],"1.00");
	assertEqualsString(fields[8],"1.00");
	assertEqualsString(fields[9],"Jan  1 2001  1:00AM");
	assertEqualsString(fields[10],"Jan  1 2001  1:00AM");
	assertEqualsString(fields[11],"testchar1                               ");
	assertEqualsString(fields[12],"testvarchar1");
	assertEqualsString(fields[13],"1");
	printf("\n");


	// field lengths by array
	printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=sqlrcur_getRowLengths(cur,0);
	assertEqualsInt(fieldlens[0],1);
	assertEqualsInt(fieldlens[1],1);
	assertEqualsInt(fieldlens[2],1);
	//assertEqualsInt(fieldlens[3],3);
	//assertEqualsInt(fieldlens[4],3);
	assertEqualsInt(fieldlens[5],3);
	assertEqualsInt(fieldlens[6],3);
	assertEqualsInt(fieldlens[7],4);
	assertEqualsInt(fieldlens[8],4);
	assertEqualsInt(fieldlens[9],19);
	assertEqualsInt(fieldlens[10],19);
	assertEqualsInt(fieldlens[11],40);
	assertEqualsInt(fieldlens[12],12);
	assertEqualsInt(fieldlens[13],1);
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
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"10.5556");
	printf("\n");


	// array substitutions
	printf("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"select $(var1),$(var2),$(var3)");
	sqlrcur_subLongs(cur,subvars,subvallongs);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// fields
	printf("FIELDS: \n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"2");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"3");
	printf("\n");


	// array substitutions
	printf("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"select '$(var1)','$(var2)','$(var3)'");
	sqlrcur_subStrings(cur,subvars,subvalstrings);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// fields
	printf("FIELDS: \n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"hi");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"bye");
	printf("\n");


	// array substitutions
	printf("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"select $(var1),$(var2),$(var3)");
	sqlrcur_subDoubles(cur,subvars,subvaldoubles,precs,scales);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// fields
	printf("FIELDS: \n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"10.55");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"10.556");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"10.5556");
	printf("\n");


	// nulls as nulls
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


	// result set buffer size
	printf("RESULT SET BUFFER SIZE: \n");
	assertEqualsInt(sqlrcur_getResultSetBufferSize(cur),0);
	sqlrcur_setResultSetBufferSize(cur,2);
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testint "));
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
	assertEqualsString(sqlrcur_getColumnName(cur,0),NULL);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,0),0);
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),NULL);
	sqlrcur_getColumnInfo(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testint "));
	assertEqualsString(sqlrcur_getColumnName(cur,0),"testint");
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,0),4);
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),"INT");
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
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,1,0),"2");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,2,0),"3");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,3,0),"4");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,4,0),"5");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,5,0),"6");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
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
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,1,0),"2");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,2,0),"3");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,3,0),"4");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,4,0),"5");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,5,0),"6");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
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
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testint "));
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
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testint "));
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqualsString(filename,"cachefile1");
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	free(filename);
	printf("\n");


	// column count for cached result set
	printf("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqualsInt(sqlrcur_colCount(cur),14);
	printf("\n");


	// column names for cached result set
	printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqualsString(sqlrcur_getColumnName(cur,0),"testint");
	assertEqualsString(sqlrcur_getColumnName(cur,1),"testsmallint");
	assertEqualsString(sqlrcur_getColumnName(cur,2),"testtinyint");
	assertEqualsString(sqlrcur_getColumnName(cur,3),"testreal");
	assertEqualsString(sqlrcur_getColumnName(cur,4),"testfloat");
	assertEqualsString(sqlrcur_getColumnName(cur,5),"testdecimal");
	assertEqualsString(sqlrcur_getColumnName(cur,6),"testnumeric");
	assertEqualsString(sqlrcur_getColumnName(cur,7),"testmoney");
	assertEqualsString(sqlrcur_getColumnName(cur,8),"testsmallmoney");
	assertEqualsString(sqlrcur_getColumnName(cur,9),"testdatetime");
	assertEqualsString(sqlrcur_getColumnName(cur,10),"testsmalldatetime");
	assertEqualsString(sqlrcur_getColumnName(cur,11),"testchar");
	assertEqualsString(sqlrcur_getColumnName(cur,12),"testvarchar");
	assertEqualsString(sqlrcur_getColumnName(cur,13),"testbit");
	cols=sqlrcur_getColumnNames(cur);
	assertEqualsString(cols[0],"testint");
	assertEqualsString(cols[1],"testsmallint");
	assertEqualsString(cols[2],"testtinyint");
	assertEqualsString(cols[3],"testreal");
	assertEqualsString(cols[4],"testfloat");
	assertEqualsString(cols[5],"testdecimal");
	assertEqualsString(cols[6],"testnumeric");
	assertEqualsString(cols[7],"testmoney");
	assertEqualsString(cols[8],"testsmallmoney");
	assertEqualsString(cols[9],"testdatetime");
	assertEqualsString(cols[10],"testsmalldatetime");
	assertEqualsString(cols[11],"testchar");
	assertEqualsString(cols[12],"testvarchar");
	assertEqualsString(cols[13],"testbit");
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
	printf("FROM ONE CACHE FILE TO ANOTHER "
		"WITH RESULT SET BUFFER SIZE: \n");
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


	// finished suspended session
	printf("FINISHED SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testint "));
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
