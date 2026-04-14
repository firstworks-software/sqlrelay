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

	const char	*isolationlevels[]=
				{"1","0","2","3",NULL};
	const char	**il;
	const char * const *cols;
	const char * const *fields;
	uint32_t	*fieldlens;
	const char	*subvars[]=
				{"var1","var2","var3",NULL};
	int64_t		subvallongs[]={1,2,3};
	const char	*subvalstrings[]=
				{"hi","hello","bye"};
	double		subvaldoubles[]=
				{10.55,10.556,10.5556};
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
	uint16_t	port;
	char		*socket;
	uint16_t	id;
	char		*filename;
	uint64_t	counter=0;
	uint64_t	i;
	int		found;
	const char	*name;
	unsigned char	buffer[256];
	uint16_t	bi;
	char		querystr[512+256];
	char		hex[3];

	#define	LARGE_BUFFER_LENGTH	255
	char		largebuffer[LARGE_BUFFER_LENGTH+1];
	char		query[LARGE_BUFFER_LENGTH+256];


	// instantiation
	con=sqlrcon_alloc("sqlrelay",9000,
			"/tmp/test.socket",
			"testuser","testpassword",0,1);
	cur=sqlrcur_alloc(con);


	// identify
	printf("IDENTIFY: \n");
	assertEqStr(sqlrcon_identify(con),"sap");
	printf("\n");


	// ping
	printf("PING: \n");
	assertTrue(sqlrcon_ping(con));
	printf("\n");


	// bind format
	printf("BIND FORMAT: \n");
	assertEqStr(sqlrcon_bindFormat(con),"@*");
	printf("\n");


	// nextval format
	printf("NEXTVAL FORMAT: \n");
	assertEqStr(sqlrcon_nextvalFormat(con),
					"%s.nextval");
	printf("\n");


	// isolation levels
	printf("ISOLATION LEVELS: \n");
	for (il=isolationlevels; *il; il++) {
		assertTrue(
			sqlrcon_setIsolationLevel(
				con,*il));
		assertEqStr(
			sqlrcon_getIsolationLevel(con),
			*il);
		printf("\n");
	}
	// reset to the default isolation level
	assertTrue(sqlrcon_setIsolationLevel(
				con,
				isolationlevels[0]));
	printf("\n");


	// create testtable
	printf("CREATE TESTTABLE: \n");
	sqlrcur_sendQuery(cur,
		"drop table testtable");
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
		"	testsmalldatetime "
		"smalldatetime, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testbit bit, "
		"	testtext text) "
		"lock datarows"));
	printf("\n");


	// insert
	printf("INSERT: \n");
	assertTrue(sqlrcon_begin(con));
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
		"	1, "
		"	'testtext1')"));
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
		"	@var14, "
		"	@var15)");
	assertEqInt(
		sqlrcur_countBindVariables(cur),15);
	sqlrcur_inputBindLong(cur,"1",2);
	sqlrcur_inputBindLong(cur,"2",2);
	sqlrcur_inputBindLong(cur,"3",2);
	sqlrcur_inputBindDouble(cur,"4",2.2,2,1);
	sqlrcur_inputBindDouble(cur,"5",2.2,2,1);
	sqlrcur_inputBindDouble(cur,"6",2.2,2,1);
	sqlrcur_inputBindDouble(cur,"7",2.2,2,1);
	sqlrcur_inputBindDouble(cur,"8",2.00,3,2);
	sqlrcur_inputBindDouble(cur,"9",2.00,3,2);
	sqlrcur_inputBindString(cur,"10",
			"01-Jan-2002 02:00:00");
	sqlrcur_inputBindString(cur,"11",
			"01-Jan-2002 02:00:00");
	sqlrcur_inputBindString(cur,"12",
			"testchar2");
	sqlrcur_inputBindString(cur,"13",
			"testvarchar2");
	sqlrcur_inputBindLong(cur,"14",1);
	sqlrcur_inputBindClob(cur,"15",
			"testtext2",9);
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
	sqlrcur_inputBindString(cur,"10",
			"01-Jan-2003 03:00:00");
	sqlrcur_inputBindString(cur,"11",
			"01-Jan-2003 03:00:00");
	sqlrcur_inputBindString(cur,"12",
			"testchar3");
	sqlrcur_inputBindString(cur,"13",
			"testvarchar3");
	sqlrcur_inputBindLong(cur,"14",1);
	sqlrcur_inputBindClob(cur,"15",
			"testtext3",9);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// array of input binds by position
	// sap doesn't support implicit conversion
	// of string binds to other data types, so
	// arrays of binds don't generally work.
	// Omitting the test.


	// input bind by position with validation
	printf("INPUT BIND BY POSITION "
		"WITH VALIDATION: \n");
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
	sqlrcur_inputBindString(cur,"10",
			"01-Jan-2004 04:00:00");
	sqlrcur_inputBindString(cur,"11",
			"01-Jan-2004 04:00:00");
	sqlrcur_inputBindString(cur,"12",
			"testchar4");
	sqlrcur_inputBindString(cur,"13",
			"testvarchar4");
	sqlrcur_inputBindLong(cur,"14",1);
	sqlrcur_inputBindClob(cur,"15",
			"testtext4",9);
	sqlrcur_validateBinds(cur);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// input bind by name
	printf("INPUT BIND BY NAME: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",5);
	sqlrcur_inputBindLong(cur,"var2",5);
	sqlrcur_inputBindLong(cur,"var3",5);
	sqlrcur_inputBindDouble(cur,
			"var4",5.5,2,1);
	sqlrcur_inputBindDouble(cur,
			"var5",5.5,2,1);
	sqlrcur_inputBindDouble(cur,
			"var6",5.5,2,1);
	sqlrcur_inputBindDouble(cur,
			"var7",5.5,2,1);
	sqlrcur_inputBindDouble(cur,
			"var8",5.00,3,2);
	sqlrcur_inputBindDouble(cur,
			"var9",5.00,3,2);
	sqlrcur_inputBindString(cur,"var10",
			"01-Jan-2005 05:00:00");
	sqlrcur_inputBindString(cur,"var11",
			"01-Jan-2005 05:00:00");
	sqlrcur_inputBindString(cur,"var12",
			"testchar5");
	sqlrcur_inputBindString(cur,"var13",
			"testvarchar5");
	sqlrcur_inputBindLong(cur,"var14",1);
	sqlrcur_inputBindClob(cur,"var15",
			"testtext5",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",6);
	sqlrcur_inputBindLong(cur,"var2",6);
	sqlrcur_inputBindLong(cur,"var3",6);
	sqlrcur_inputBindDouble(cur,
			"var4",6.6,2,1);
	sqlrcur_inputBindDouble(cur,
			"var5",6.6,2,1);
	sqlrcur_inputBindDouble(cur,
			"var6",6.6,2,1);
	sqlrcur_inputBindDouble(cur,
			"var7",6.6,2,1);
	sqlrcur_inputBindDouble(cur,
			"var8",6.00,3,2);
	sqlrcur_inputBindDouble(cur,
			"var9",6.00,3,2);
	sqlrcur_inputBindString(cur,"var10",
			"01-Jan-2006 06:00:00");
	sqlrcur_inputBindString(cur,"var11",
			"01-Jan-2006 06:00:00");
	sqlrcur_inputBindString(cur,"var12",
			"testchar6");
	sqlrcur_inputBindString(cur,"var13",
			"testvarchar6");
	sqlrcur_inputBindLong(cur,"var14",1);
	sqlrcur_inputBindClob(cur,"var15",
			"testtext6",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",7);
	sqlrcur_inputBindLong(cur,"var2",7);
	sqlrcur_inputBindLong(cur,"var3",7);
	sqlrcur_inputBindDouble(cur,
			"var4",7.7,2,1);
	sqlrcur_inputBindDouble(cur,
			"var5",7.7,2,1);
	sqlrcur_inputBindDouble(cur,
			"var6",7.7,2,1);
	sqlrcur_inputBindDouble(cur,
			"var7",7.7,2,1);
	sqlrcur_inputBindDouble(cur,
			"var8",7.00,3,2);
	sqlrcur_inputBindDouble(cur,
			"var9",7.00,3,2);
	sqlrcur_inputBindString(cur,"var10",
			"01-Jan-2007 07:00:00");
	sqlrcur_inputBindString(cur,"var11",
			"01-Jan-2007 07:00:00");
	sqlrcur_inputBindString(cur,"var12",
			"testchar7");
	sqlrcur_inputBindString(cur,"var13",
			"testvarchar7");
	sqlrcur_inputBindLong(cur,"var14",1);
	sqlrcur_inputBindClob(cur,"var15",
			"testtext7",9);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// array of input binds by name
	// sap doesn't support implicit conversion
	// of string binds to other data types, so
	// arrays of binds don't generally work.
	// Omitting the test.


	// input bind by name with validation
	printf("INPUT BIND BY NAME "
		"WITH VALIDATION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",8);
	sqlrcur_inputBindLong(cur,"var2",8);
	sqlrcur_inputBindLong(cur,"var3",8);
	sqlrcur_inputBindDouble(cur,
			"var4",8.8,2,1);
	sqlrcur_inputBindDouble(cur,
			"var5",8.8,2,1);
	sqlrcur_inputBindDouble(cur,
			"var6",8.8,2,1);
	sqlrcur_inputBindDouble(cur,
			"var7",8.8,2,1);
	sqlrcur_inputBindDouble(cur,
			"var8",8.00,3,2);
	sqlrcur_inputBindDouble(cur,
			"var9",8.00,3,2);
	sqlrcur_inputBindString(cur,"var10",
			"01-Jan-2008 08:00:00");
	sqlrcur_inputBindString(cur,"var11",
			"01-Jan-2008 08:00:00");
	sqlrcur_inputBindString(cur,"var12",
			"testchar8");
	sqlrcur_inputBindString(cur,"var13",
			"testvarchar8");
	sqlrcur_inputBindLong(cur,"var14",1);
	sqlrcur_inputBindClob(cur,"var15",
			"testtext8",9);
	sqlrcur_inputBindString(cur,"var16",
			"junkvalue");
	sqlrcur_validateBinds(cur);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// select
	printf("SELECT: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select * from testtable "
		"order by testint"));
	printf("\n");


	// column count
	printf("COLUMN COUNT: \n");
	assertEqInt(sqlrcur_colCount(cur),15);
	printf("\n");


	// column names
	printf("COLUMN NAMES: \n");
	assertEqStr(
		sqlrcur_getColumnName(cur,0),
		"testint");
	assertEqStr(
		sqlrcur_getColumnName(cur,1),
		"testsmallint");
	assertEqStr(
		sqlrcur_getColumnName(cur,2),
		"testtinyint");
	assertEqStr(
		sqlrcur_getColumnName(cur,3),
		"testreal");
	assertEqStr(
		sqlrcur_getColumnName(cur,4),
		"testfloat");
	assertEqStr(
		sqlrcur_getColumnName(cur,5),
		"testdecimal");
	assertEqStr(
		sqlrcur_getColumnName(cur,6),
		"testnumeric");
	assertEqStr(
		sqlrcur_getColumnName(cur,7),
		"testmoney");
	assertEqStr(
		sqlrcur_getColumnName(cur,8),
		"testsmallmoney");
	assertEqStr(
		sqlrcur_getColumnName(cur,9),
		"testdatetime");
	assertEqStr(
		sqlrcur_getColumnName(cur,10),
		"testsmalldatetime");
	assertEqStr(
		sqlrcur_getColumnName(cur,11),
		"testchar");
	assertEqStr(
		sqlrcur_getColumnName(cur,12),
		"testvarchar");
	assertEqStr(
		sqlrcur_getColumnName(cur,13),
		"testbit");
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
	assertEqStr(cols[10],
		"testsmalldatetime");
	assertEqStr(cols[11],"testchar");
	assertEqStr(cols[12],"testvarchar");
	assertEqStr(cols[13],"testbit");
	printf("\n");


	// column types
	printf("COLUMN TYPES: \n");
	assertEqStr(
		sqlrcur_getColumnTypeByIndex(cur,0),
		"INT");
	assertEqStr(
		sqlrcur_getColumnTypeByName(
			cur,"testint"),
		"INT");
	assertEqStr(
		sqlrcur_getColumnTypeByIndex(cur,1),
		"SMALLINT");
	assertEqStr(
		sqlrcur_getColumnTypeByName(
			cur,"testsmallint"),
		"SMALLINT");
	assertEqStr(
		sqlrcur_getColumnTypeByIndex(cur,2),
		"TINYINT");
	assertEqStr(
		sqlrcur_getColumnTypeByName(
			cur,"testtinyint"),
		"TINYINT");
	assertEqStr(
		sqlrcur_getColumnTypeByIndex(cur,3),
		"REAL");
	assertEqStr(
		sqlrcur_getColumnTypeByName(
			cur,"testreal"),
		"REAL");
	assertEqStr(
		sqlrcur_getColumnTypeByIndex(cur,4),
		"FLOAT");
	assertEqStr(
		sqlrcur_getColumnTypeByName(
			cur,"testfloat"),
		"FLOAT");
	assertEqStr(
		sqlrcur_getColumnTypeByIndex(cur,5),
		"DECIMAL");
	assertEqStr(
		sqlrcur_getColumnTypeByName(
			cur,"testdecimal"),
		"DECIMAL");
	assertEqStr(
		sqlrcur_getColumnTypeByIndex(cur,6),
		"NUMERIC");
	assertEqStr(
		sqlrcur_getColumnTypeByName(
			cur,"testnumeric"),
		"NUMERIC");
	assertEqStr(
		sqlrcur_getColumnTypeByIndex(cur,7),
		"MONEY");
	assertEqStr(
		sqlrcur_getColumnTypeByName(
			cur,"testmoney"),
		"MONEY");
	assertEqStr(
		sqlrcur_getColumnTypeByIndex(cur,8),
		"SMALLMONEY");
	assertEqStr(
		sqlrcur_getColumnTypeByName(
			cur,"testsmallmoney"),
		"SMALLMONEY");
	assertEqStr(
		sqlrcur_getColumnTypeByIndex(cur,9),
		"DATETIME");
	assertEqStr(
		sqlrcur_getColumnTypeByName(
			cur,"testdatetime"),
		"DATETIME");
	assertEqStr(
		sqlrcur_getColumnTypeByIndex(
			cur,10),
		"SMALLDATETIME");
	assertEqStr(
		sqlrcur_getColumnTypeByName(
			cur,"testsmalldatetime"),
		"SMALLDATETIME");
	assertEqStr(
		sqlrcur_getColumnTypeByIndex(
			cur,11),
		"CHAR");
	assertEqStr(
		sqlrcur_getColumnTypeByName(
			cur,"testchar"),
		"CHAR");
	assertEqStr(
		sqlrcur_getColumnTypeByIndex(
			cur,12),
		"CHAR");
	assertEqStr(
		sqlrcur_getColumnTypeByName(
			cur,"testvarchar"),
		"CHAR");
	assertEqStr(
		sqlrcur_getColumnTypeByIndex(
			cur,13),
		"BIT");
	assertEqStr(
		sqlrcur_getColumnTypeByName(
			cur,"testbit"),
		"BIT");
	printf("\n");


	// column length
	printf("COLUMN LENGTH: \n");
	assertEqInt(
		sqlrcur_getColumnLengthByIndex(
			cur,0),4);
	assertEqInt(
		sqlrcur_getColumnLengthByName(
			cur,"testint"),4);
	assertEqInt(
		sqlrcur_getColumnLengthByIndex(
			cur,1),2);
	assertEqInt(
		sqlrcur_getColumnLengthByName(
			cur,"testsmallint"),2);
	assertEqInt(
		sqlrcur_getColumnLengthByIndex(
			cur,2),1);
	assertEqInt(
		sqlrcur_getColumnLengthByName(
			cur,"testtinyint"),1);
	assertEqInt(
		sqlrcur_getColumnLengthByIndex(
			cur,3),4);
	assertEqInt(
		sqlrcur_getColumnLengthByName(
			cur,"testreal"),4);
	assertEqInt(
		sqlrcur_getColumnLengthByIndex(
			cur,4),8);
	assertEqInt(
		sqlrcur_getColumnLengthByName(
			cur,"testfloat"),8);
	assertEqInt(
		sqlrcur_getColumnLengthByIndex(
			cur,5),35);
	assertEqInt(
		sqlrcur_getColumnLengthByName(
			cur,"testdecimal"),35);
	assertEqInt(
		sqlrcur_getColumnLengthByIndex(
			cur,6),35);
	assertEqInt(
		sqlrcur_getColumnLengthByName(
			cur,"testnumeric"),35);
	assertEqInt(
		sqlrcur_getColumnLengthByIndex(
			cur,7),8);
	assertEqInt(
		sqlrcur_getColumnLengthByName(
			cur,"testmoney"),8);
	assertEqInt(
		sqlrcur_getColumnLengthByIndex(
			cur,8),4);
	assertEqInt(
		sqlrcur_getColumnLengthByName(
			cur,"testsmallmoney"),4);
	assertEqInt(
		sqlrcur_getColumnLengthByIndex(
			cur,9),8);
	assertEqInt(
		sqlrcur_getColumnLengthByName(
			cur,"testdatetime"),8);
	assertEqInt(
		sqlrcur_getColumnLengthByIndex(
			cur,10),4);
	assertEqInt(
		sqlrcur_getColumnLengthByName(
			cur,"testsmalldatetime"),
		4);
	assertEqInt(
		sqlrcur_getColumnLengthByIndex(
			cur,11),40);
	assertEqInt(
		sqlrcur_getColumnLengthByName(
			cur,"testchar"),40);
	assertEqInt(
		sqlrcur_getColumnLengthByIndex(
			cur,12),40);
	assertEqInt(
		sqlrcur_getColumnLengthByName(
			cur,"testvarchar"),40);
	assertEqInt(
		sqlrcur_getColumnLengthByIndex(
			cur,13),1);
	assertEqInt(
		sqlrcur_getColumnLengthByName(
			cur,"testbit"),1);
	printf("\n");


	// longest column
	printf("LONGEST COLUMN: \n");
	assertEqInt(
		sqlrcur_getLongestByIndex(cur,0),
		1);
	assertEqInt(
		sqlrcur_getLongestByName(
			cur,"testint"),1);
	assertEqInt(
		sqlrcur_getLongestByIndex(cur,1),
		1);
	assertEqInt(
		sqlrcur_getLongestByName(
			cur,"testsmallint"),1);
	assertEqInt(
		sqlrcur_getLongestByIndex(cur,2),
		1);
	assertEqInt(
		sqlrcur_getLongestByName(
			cur,"testtinyint"),1);
	assertEqInt(
		sqlrcur_getLongestByIndex(cur,3),
		18);
	assertEqInt(
		sqlrcur_getLongestByName(
			cur,"testreal"),18);
	assertEqInt(
		sqlrcur_getLongestByIndex(cur,4),
		18);
	assertEqInt(
		sqlrcur_getLongestByName(
			cur,"testfloat"),18);
	assertEqInt(
		sqlrcur_getLongestByIndex(cur,5),
		3);
	assertEqInt(
		sqlrcur_getLongestByName(
			cur,"testdecimal"),3);
	assertEqInt(
		sqlrcur_getLongestByIndex(cur,6),
		3);
	assertEqInt(
		sqlrcur_getLongestByName(
			cur,"testnumeric"),3);
	assertEqInt(
		sqlrcur_getLongestByIndex(cur,7),
		4);
	assertEqInt(
		sqlrcur_getLongestByName(
			cur,"testmoney"),4);
	assertEqInt(
		sqlrcur_getLongestByIndex(cur,8),
		4);
	assertEqInt(
		sqlrcur_getLongestByName(
			cur,"testsmallmoney"),4);
	assertEqInt(
		sqlrcur_getLongestByIndex(cur,9),
		19);
	assertEqInt(
		sqlrcur_getLongestByName(
			cur,"testdatetime"),19);
	assertEqInt(
		sqlrcur_getLongestByIndex(cur,10),
		19);
	assertEqInt(
		sqlrcur_getLongestByName(
			cur,"testsmalldatetime"),
		19);
	assertEqInt(
		sqlrcur_getLongestByIndex(cur,11),
		40);
	assertEqInt(
		sqlrcur_getLongestByName(
			cur,"testchar"),40);
	assertEqInt(
		sqlrcur_getLongestByIndex(cur,12),
		12);
	assertEqInt(
		sqlrcur_getLongestByName(
			cur,"testvarchar"),12);
	assertEqInt(
		sqlrcur_getLongestByIndex(cur,13),
		1);
	assertEqInt(
		sqlrcur_getLongestByName(
			cur,"testbit"),1);
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
	assertEqInt(
		sqlrcur_firstRowIndex(cur),0);
	printf("\n");


	// end of result set
	printf("END OF RESULT SET: \n");
	assertTrue(
		sqlrcur_endOfResultSet(cur));
	printf("\n");


	// fields by index
	printf("FIELDS BY INDEX: \n");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,0),
		"1");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,1),
		"1");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,2),
		"1");
	//assertEqStr(
	//	sqlrcur_getFieldByIndex(cur,0,3),
	//	"1.1");
	//assertEqStr(
	//	sqlrcur_getFieldByIndex(cur,0,4),
	//	"1.1");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,5),
		"1.1");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,6),
		"1.1");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,7),
		"1.00");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,8),
		"1.00");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,9),
		"Jan  1 2001  1:00AM");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,10),
		"Jan  1 2001  1:00AM");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,11),
		"testchar1"
		"                               ");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,12),
		"testvarchar1");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,13),
		"1");
	printf("\n");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,0),
		"8");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,1),
		"8");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,2),
		"8");
	//assertEqStr(
	//	sqlrcur_getFieldByIndex(cur,7,3),
	//	"8.8");
	//assertEqStr(
	//	sqlrcur_getFieldByIndex(cur,7,4),
	//	"8.8");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,5),
		"8.8");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,6),
		"8.8");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,7),
		"8.00");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,8),
		"8.00");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,9),
		"Jan  1 2008  8:00AM");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,10),
		"Jan  1 2008  8:00AM");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,11),
		"testchar8"
		"                               ");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,12),
		"testvarchar8");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,13),
		"1");
	printf("\n");


	// field lengths by index
	printf("FIELD LENGTHS BY INDEX: \n");
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,0,0),1);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,0,1),1);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,0,2),1);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,0,3),18);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,0,4),18);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,0,5),3);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,0,6),3);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,0,7),4);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,0,8),4);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,0,9),19);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,0,10),19);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,0,11),40);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,0,12),12);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,0,13),1);
	printf("\n");
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,7,0),1);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,7,1),1);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,7,2),1);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,7,3),18);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,7,4),18);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,7,5),3);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,7,6),3);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,7,7),4);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,7,8),4);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,7,9),19);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,7,10),19);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,7,11),40);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,7,12),12);
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,7,13),1);
	printf("\n");


	// fields by name
	printf("FIELDS BY NAME: \n");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"testint"),
		"1");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"testsmallint"),
		"1");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"testtinyint"),
		"1");
	//assertEqStr(
	//	sqlrcur_getFieldByName(
	//		cur,0,"testreal"),
	//	"1.1");
	//assertEqStr(
	//	sqlrcur_getFieldByName(
	//		cur,0,"testfloat"),
	//	"1.1");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"testdecimal"),
		"1.1");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"testnumeric"),
		"1.1");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"testmoney"),
		"1.00");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"testsmallmoney"),
		"1.00");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"testdatetime"),
		"Jan  1 2001  1:00AM");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"testsmalldatetime"),
		"Jan  1 2001  1:00AM");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"testchar"),
		"testchar1"
		"                               ");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"testvarchar"),
		"testvarchar1");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"testbit"),
		"1");
	printf("\n");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,7,"testint"),
		"8");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,7,"testsmallint"),
		"8");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,7,"testtinyint"),
		"8");
	//assertEqStr(
	//	sqlrcur_getFieldByName(
	//		cur,7,"testreal"),
	//	"8.8");
	//assertEqStr(
	//	sqlrcur_getFieldByName(
	//		cur,7,"testfloat"),
	//	"8.8");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,7,"testdecimal"),
		"8.8");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,7,"testnumeric"),
		"8.8");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,7,"testmoney"),
		"8.00");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,7,"testsmallmoney"),
		"8.00");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,7,"testdatetime"),
		"Jan  1 2008  8:00AM");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,7,"testsmalldatetime"),
		"Jan  1 2008  8:00AM");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,7,"testchar"),
		"testchar8"
		"                               ");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,7,"testvarchar"),
		"testvarchar8");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,7,"testbit"),
		"1");
	printf("\n");


	// field lengths by name
	printf("FIELD LENGTHS BY NAME: \n");
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,0,"testint"),1);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,0,"testsmallint"),1);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,0,"testtinyint"),1);
	//assertEqInt(
	//	sqlrcur_getFieldLengthByName(
	//		cur,0,"testreal"),3);
	//assertEqInt(
	//	sqlrcur_getFieldLengthByName(
	//		cur,0,"testfloat"),3);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,0,"testdecimal"),3);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,0,"testnumeric"),3);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,0,"testmoney"),4);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,0,"testsmallmoney"),
		4);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,0,"testdatetime"),
		19);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,0,"testsmalldatetime"),
		19);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,0,"testchar"),40);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,0,"testvarchar"),
		12);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,0,"testbit"),1);
	printf("\n");
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,7,"testint"),1);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,7,"testsmallint"),1);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,7,"testtinyint"),1);
	//assertEqInt(
	//	sqlrcur_getFieldLengthByName(
	//		cur,7,"testreal"),3);
	//assertEqInt(
	//	sqlrcur_getFieldLengthByName(
	//		cur,7,"testfloat"),3);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,7,"testdecimal"),3);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,7,"testnumeric"),3);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,7,"testmoney"),4);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,7,"testsmallmoney"),
		4);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,7,"testdatetime"),
		19);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,7,"testsmalldatetime"),
		19);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,7,"testchar"),40);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,7,"testvarchar"),
		12);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,7,"testbit"),1);
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
	assertEqStr(fields[9],
		"Jan  1 2001  1:00AM");
	assertEqStr(fields[10],
		"Jan  1 2001  1:00AM");
	assertEqStr(fields[11],
		"testchar1"
		"                               ");
	assertEqStr(fields[12],
		"testvarchar1");
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


	// result set buffer size
	printf("RESULT SET BUFFER SIZE: \n");
	assertEqInt(
		sqlrcur_getResultSetBufferSize(cur),
		0);
	sqlrcur_setResultSetBufferSize(cur,2);
	assertTrue(sqlrcur_sendQuery(cur,
		"select * from testtable "
		"order by testint"));
	assertEqInt(
		sqlrcur_getResultSetBufferSize(cur),
		2);
	printf("\n");
	assertEqInt(
		sqlrcur_firstRowIndex(cur),0);
	assertFalse(
		sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),2);
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,0),
		"1");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,1,0),
		"2");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,2,0),
		"3");
	printf("\n");
	assertEqInt(
		sqlrcur_firstRowIndex(cur),2);
	assertFalse(
		sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),4);
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,6,0),
		"7");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,0),
		"8");
	printf("\n");
	assertEqInt(
		sqlrcur_firstRowIndex(cur),6);
	assertFalse(
		sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),8);
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,8,0),
		NULL);
	printf("\n");
	assertEqInt(
		sqlrcur_firstRowIndex(cur),8);
	assertTrue(
		sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),8);
	sqlrcur_setResultSetBufferSize(cur,0);
	printf("\n");


	// dont get column info
	printf("DONT GET COLUMN INFO: \n");
	sqlrcur_dontGetColumnInfo(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"select * from testtable "
		"order by testint"));
	assertEqStr(
		sqlrcur_getColumnName(cur,0),
		NULL);
	assertEqInt(
		sqlrcur_getColumnLengthByIndex(
			cur,0),0);
	assertEqStr(
		sqlrcur_getColumnTypeByIndex(
			cur,0),NULL);
	sqlrcur_getColumnInfo(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"select * from testtable "
		"order by testint"));
	assertEqStr(
		sqlrcur_getColumnName(cur,0),
		"testint");
	assertEqInt(
		sqlrcur_getColumnLengthByIndex(
			cur,0),4);
	assertEqStr(
		sqlrcur_getColumnTypeByIndex(
			cur,0),"INT");
	printf("\n");


	// suspended session
	printf("SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select * from testtable "
		"order by testint"));
	sqlrcur_suspendResultSet(cur);
	assertTrue(
		sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(
		sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(
		con,port,socket));
	free(socket);
	printf("\n");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,0),
		"1");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,1,0),
		"2");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,2,0),
		"3");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,3,0),
		"4");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,4,0),
		"5");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,5,0),
		"6");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,6,0),
		"7");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,0),
		"8");
	printf("\n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select * from testtable "
		"order by testint"));
	sqlrcur_suspendResultSet(cur);
	assertTrue(
		sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(
		sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(
		con,port,socket));
	free(socket);
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,0),
		"1");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,1,0),
		"2");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,2,0),
		"3");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,3,0),
		"4");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,4,0),
		"5");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,5,0),
		"6");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,6,0),
		"7");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,0),
		"8");
	printf("\n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select * from testtable "
		"order by testint"));
	sqlrcur_suspendResultSet(cur);
	assertTrue(
		sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(
		sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(
		con,port,socket));
	free(socket);
	printf("\n");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,0),
		"1");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,1,0),
		"2");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,2,0),
		"3");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,3,0),
		"4");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,4,0),
		"5");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,5,0),
		"6");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,6,0),
		"7");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,0),
		"8");
	printf("\n");


	// suspended result set
	printf("SUSPENDED RESULT SET: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	assertTrue(sqlrcur_sendQuery(cur,
		"select * from testtable "
		"order by testint"));
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,2,0),
		"3");
	id=sqlrcur_getResultSetId(cur);
	sqlrcur_suspendResultSet(cur);
	assertTrue(
		sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(
		sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(
		con,port,socket));
	free(socket);
	assertTrue(
		sqlrcur_resumeResultSet(cur,id));
	printf("\n");
	assertEqInt(
		sqlrcur_firstRowIndex(cur),4);
	assertFalse(
		sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),6);
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,0),
		"8");
	printf("\n");
	assertEqInt(
		sqlrcur_firstRowIndex(cur),6);
	assertFalse(
		sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),8);
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,8,0),
		NULL);
	printf("\n");
	assertEqInt(
		sqlrcur_firstRowIndex(cur),8);
	assertTrue(
		sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),8);
	sqlrcur_setResultSetBufferSize(cur,0);
	printf("\n");


	// cached result set
	printf("CACHED RESULT SET: \n");
	sqlrcur_cacheToFile(cur,"cachefile1");
	sqlrcur_setCacheTtl(cur,200);
	assertTrue(sqlrcur_sendQuery(cur,
		"select * from testtable "
		"order by testint"));
	filename=strdup(
		sqlrcur_getCacheFileName(cur));
	assertEqStr(filename,"cachefile1");
	sqlrcur_cacheOff(cur);
	assertTrue(
		sqlrcur_openCachedResultSet(
			cur,filename));
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,0),
		"8");
	free(filename);
	printf("\n");


	// column count for cached result set
	printf("COLUMN COUNT FOR "
		"CACHED RESULT SET: \n");
	assertEqInt(sqlrcur_colCount(cur),15);
	printf("\n");


	// column names for cached result set
	printf("COLUMN NAMES FOR "
		"CACHED RESULT SET: \n");
	assertEqStr(
		sqlrcur_getColumnName(cur,0),
		"testint");
	assertEqStr(
		sqlrcur_getColumnName(cur,1),
		"testsmallint");
	assertEqStr(
		sqlrcur_getColumnName(cur,2),
		"testtinyint");
	assertEqStr(
		sqlrcur_getColumnName(cur,3),
		"testreal");
	assertEqStr(
		sqlrcur_getColumnName(cur,4),
		"testfloat");
	assertEqStr(
		sqlrcur_getColumnName(cur,5),
		"testdecimal");
	assertEqStr(
		sqlrcur_getColumnName(cur,6),
		"testnumeric");
	assertEqStr(
		sqlrcur_getColumnName(cur,7),
		"testmoney");
	assertEqStr(
		sqlrcur_getColumnName(cur,8),
		"testsmallmoney");
	assertEqStr(
		sqlrcur_getColumnName(cur,9),
		"testdatetime");
	assertEqStr(
		sqlrcur_getColumnName(cur,10),
		"testsmalldatetime");
	assertEqStr(
		sqlrcur_getColumnName(cur,11),
		"testchar");
	assertEqStr(
		sqlrcur_getColumnName(cur,12),
		"testvarchar");
	assertEqStr(
		sqlrcur_getColumnName(cur,13),
		"testbit");
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
	assertEqStr(cols[10],
		"testsmalldatetime");
	assertEqStr(cols[11],"testchar");
	assertEqStr(cols[12],"testvarchar");
	assertEqStr(cols[13],"testbit");
	printf("\n");


	// cached result set with result set
	// buffer size
	printf("CACHED RESULT SET WITH "
		"RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile1");
	sqlrcur_setCacheTtl(cur,200);
	assertTrue(sqlrcur_sendQuery(cur,
		"select * from testtable "
		"order by testint"));
	filename=strdup(
		sqlrcur_getCacheFileName(cur));
	assertEqStr(filename,"cachefile1");
	sqlrcur_cacheOff(cur);
	assertTrue(
		sqlrcur_openCachedResultSet(
			cur,filename));
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,0),
		"8");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,8,0),
		NULL);
	sqlrcur_setResultSetBufferSize(cur,0);
	free(filename);
	printf("\n");


	// from one cache file to another
	printf("FROM ONE CACHE FILE "
		"TO ANOTHER: \n");
	sqlrcur_cacheToFile(cur,"cachefile2");
	assertTrue(
		sqlrcur_openCachedResultSet(
			cur,"cachefile1"));
	sqlrcur_cacheOff(cur);
	assertTrue(
		sqlrcur_openCachedResultSet(
			cur,"cachefile2"));
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,0),
		"8");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,8,0),
		NULL);
	printf("\n");


	// from one cache file to another with
	// result set buffer size
	printf("FROM ONE CACHE FILE TO ANOTHER "
		"WITH RESULT SET "
		"BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile2");
	assertTrue(
		sqlrcur_openCachedResultSet(
			cur,"cachefile1"));
	sqlrcur_cacheOff(cur);
	assertTrue(
		sqlrcur_openCachedResultSet(
			cur,"cachefile2"));
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,0),
		"8");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,8,0),
		NULL);
	sqlrcur_setResultSetBufferSize(cur,0);
	printf("\n");


	// cached result set with suspend and
	// result set buffer size
	printf("CACHED RESULT SET "
		"WITH SUSPEND "
		"AND RESULT SET "
		"BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile1");
	sqlrcur_setCacheTtl(cur,200);
	assertTrue(sqlrcur_sendQuery(cur,
		"select * from testtable "
		"order by testint"));
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,2,0),
		"3");
	filename=strdup(
		sqlrcur_getCacheFileName(cur));
	assertEqStr(filename,"cachefile1");
	id=sqlrcur_getResultSetId(cur);
	sqlrcur_suspendResultSet(cur);
	assertTrue(
		sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(
		sqlrcon_getConnectionSocket(con));
	printf("\n");
	assertTrue(sqlrcon_resumeSession(
		con,port,socket));
	free(socket);
	assertTrue(
		sqlrcur_resumeCachedResultSet(
			cur,id,filename));
	printf("\n");
	assertEqInt(
		sqlrcur_firstRowIndex(cur),4);
	assertFalse(
		sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),6);
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,0),
		"8");
	printf("\n");
	assertEqInt(
		sqlrcur_firstRowIndex(cur),6);
	assertFalse(
		sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),8);
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,8,0),
		NULL);
	printf("\n");
	assertEqInt(
		sqlrcur_firstRowIndex(cur),8);
	assertTrue(
		sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),8);
	sqlrcur_cacheOff(cur);
	printf("\n");
	assertTrue(
		sqlrcur_openCachedResultSet(
			cur,filename));
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,0),
		"8");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,8,0),
		NULL);
	sqlrcur_setResultSetBufferSize(cur,0);
	free(filename);
	printf("\n");


	// finished suspended session
	printf("FINISHED "
		"SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select * from testtable "
		"order by testint"));
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,4,0),
		"5");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,5,0),
		"6");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,6,0),
		"7");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,0),
		"8");
	id=sqlrcur_getResultSetId(cur);
	sqlrcur_suspendResultSet(cur);
	assertTrue(
		sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(
		sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(
		con,port,socket));
	free(socket);
	assertTrue(
		sqlrcur_resumeResultSet(cur,id));
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,4,0),
		NULL);
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,5,0),
		NULL);
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,6,0),
		NULL);
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,7,0),
		NULL);
	printf("\n");


	// nested selects
	printf("NESTED SELECTS: \n");
	sqlrcur_setResultSetBufferSize(cur,1);
	assertTrue(sqlrcur_sendQuery(cur,
		"select * from testtable"));
	for (i=0;
		sqlrcur_getRow(cur,i);
		i++) {
		secondcur=sqlrcur_alloc(con);
		sqlrcur_setResultSetBufferSize(
			secondcur,1);
		assertTrue(
			sqlrcur_sendQuery(
				secondcur,
				"select * "
				"from "
				"testtable"));
		sqlrcur_free(secondcur);
	}
	sqlrcur_setResultSetBufferSize(cur,0);
	printf("\n");


	// commit and rollback
	printf("COMMIT AND ROLLBACK: \n");
	secondcon=sqlrcon_alloc("sqlrelay",9000,
		"/tmp/test.socket",
		"testuser","testpassword",0,1);
	secondcur=sqlrcur_alloc(secondcon);
	assertTrue(sqlrcur_sendQuery(secondcur,
		"select count(*) "
		"from testtable"));
	assertEqStr(
		sqlrcur_getFieldByIndex(
			secondcur,0,0),"0");
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_sendQuery(secondcur,
		"select count(*) "
		"from testtable"));
	assertEqStr(
		sqlrcur_getFieldByIndex(
			secondcur,0,0),"8");
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	10, "
		"	10, "
		"	10, "
		"	10.1, "
		"	10.1, "
		"	10.1, "
		"	10.1, "
		"	10.00, "
		"	10.00, "
		"	'01-Jan-2010 10:00:00', "
		"	'01-Jan-2010 10:00:00', "
		"	'testchar10', "
		"	'testvarchar10', "
		"	10, "
		"	'testtext10')"));
	assertTrue(sqlrcon_rollback(con));
	assertTrue(sqlrcur_sendQuery(secondcur,
		"select count(*) "
		"from testtable"));
	assertEqStr(
		sqlrcur_getFieldByIndex(
			secondcur,0,0),"8");
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	10, "
		"	10, "
		"	10, "
		"	10.1, "
		"	10.1, "
		"	10.1, "
		"	10.1, "
		"	10.00, "
		"	10.00, "
		"	'01-Jan-2010 10:00:00', "
		"	'01-Jan-2010 10:00:00', "
		"	'testchar10', "
		"	'testvarchar10', "
		"	10, "
		"	'testtext10')"));
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_sendQuery(secondcur,
		"select count(*) "
		"from testtable"));
	assertEqStr(
		sqlrcur_getFieldByIndex(
			secondcur,0,0),"9");
	sqlrcur_free(secondcur);
	sqlrcon_free(secondcon);
	assertTrue(sqlrcur_sendQuery(cur,
		"drop table testtable"));
	printf("\n");


	// individual substitutions
	printf("INDIVIDUAL "
		"SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,
		"select $(var1),"
		"'$(var2)',$(var3)");
	sqlrcur_subLong(cur,"var1",1);
	sqlrcur_subString(cur,"var2","hello");
	sqlrcur_subDouble(cur,
		"var3",10.5556,6,4);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,0),
		"1");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,1),
		"hello");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,2),
		"10.5556");
	printf("\n");


	// array substitutions
	printf("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,
		"select $(var1),"
		"$(var2),$(var3)");
	sqlrcur_subLongs(cur,
		subvars,subvallongs);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,0),
		"1");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,1),
		"2");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,2),
		"3");
	printf("\n");
	sqlrcur_prepareQuery(cur,
		"select '$(var1)',"
		"'$(var2)','$(var3)'");
	sqlrcur_subStrings(cur,
		subvars,subvalstrings);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,0),
		"hi");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,1),
		"hello");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,2),
		"bye");
	printf("\n");
	sqlrcur_prepareQuery(cur,
		"select $(var1),"
		"$(var2),$(var3)");
	sqlrcur_subDoubles(cur,subvars,
		subvaldoubles,precs,scales);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,0),
		"10.55");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,1),
		"10.556");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,2),
		"10.5556");
	printf("\n");


	// nulls as nulls
	printf("NULLS AS NULLS: \n");
	sqlrcur_getNullsAsNulls(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"select NULL,1,NULL"));
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,0),
		NULL);
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,1),
		"1");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,2),
		NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"select NULL,1,NULL"));
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,0),
		"");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,1),
		"1");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,2),
		"");
	printf("\n");



	// null and empty lobs
	printf("NULL AND EMPTY LOBS: \n");
	sqlrcur_getNullsAsNulls(cur);
	sqlrcur_sendQuery(cur,
		"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testclob1 text NULL, "
		"	testclob2 text NULL, "
		"	testblob1 image NULL, "
		"	testblob2 image NULL)"));
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	@var1, "
		"	@var2, "
		"	@var3, "
		"	@var4)");
	sqlrcur_inputBindClob(cur,
		"var1","",0);
	sqlrcur_inputBindClob(cur,
		"var2",NULL,0);
	sqlrcur_inputBindBlob(cur,
		"var3","",0);
	sqlrcur_inputBindBlob(cur,
		"var4",NULL,0);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_sendQuery(cur,
		"select * from testtable");
	// sap converts empty strings to a single
	// space.  It's possible that if we had
	// true input bind support on the backend,
	// then this would work correctly, but for
	// now we're faking binds, and inserting
	// an empty string, so we have to check
	// for a single space here.
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,0),
		" ");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,1),
		NULL);
	// see note above for why we're checking
	// for a single space
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,2),
		" ");
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,3),
		NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"drop table testtable"));
	printf("\n");


	// long lobs
	printf("LONG LOBS: \n");
	sqlrcur_sendQuery(cur,
		"drop table testtable");
	sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testclob text NULL, "
		"	testblob image NULL) "
		"lock datarows");
	sqlrcur_prepareQuery(cur,
		"insert into testtable "
		"values (@var1,@var2)");
	for (i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	sqlrcur_inputBindClob(cur,"var1",
		largebuffer,
		LARGE_BUFFER_LENGTH);
	sqlrcur_inputBindBlob(cur,"var2",
		largebuffer,
		LARGE_BUFFER_LENGTH);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_sendQuery(cur,
		"select * from testtable");
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,0,"testclob"),
		LARGE_BUFFER_LENGTH);
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"testclob"),
		largebuffer);
	assertEqInt(
		sqlrcur_getFieldLengthByName(
			cur,0,"testblob"),
		LARGE_BUFFER_LENGTH);
	assertEqStrLen(
		sqlrcur_getFieldByName(
			cur,0,"testblob"),
		largebuffer,
		LARGE_BUFFER_LENGTH);
	assertTrue(sqlrcur_sendQuery(cur,
		"drop table testtable"));
	printf("\n");


	// output bind by position
	printf("OUTPUT BIND "
		"BY POSITION: \n");
	sqlrcur_sendQuery(cur,
		"drop procedure testproc");
	sqlrcur_getNullsAsNulls(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc "
		"	@out1 int output, "
		"	@out2 varchar(20) output, "
		"	@out3 float output, "
		"	@out4 datetime output, "
		"	@out5 varchar(20) "
		"output as "
		"select @out1=1, "
		"	@out2='hello', "
		"	@out3=2.5, "
		"	@out4='2001-02-03', "
		"	@out5=null"));
	sqlrcur_prepareQuery(cur,
		"exec testproc");
	assertEqInt(
		sqlrcur_countBindVariables(cur),
		0);
	sqlrcur_defineOutputBindInteger(
		cur,"1");
	sqlrcur_defineOutputBindString(
		cur,"2",20);
	sqlrcur_defineOutputBindDouble(
		cur,"3");
	sqlrcur_defineOutputBindDate(
		cur,"4");
	sqlrcur_defineOutputBindString(
		cur,"5",20);
	assertTrue(sqlrcur_executeQuery(cur));
	numvar=
		sqlrcur_getOutputBindInteger(
			cur,"1");
	stringvar=
		sqlrcur_getOutputBindString(
			cur,"2");
	floatvar=
		sqlrcur_getOutputBindDouble(
			cur,"3");
	sqlrcur_getOutputBindDate(cur,"4",
		&year,&month,&day,
		&hour,&minute,&second,
		&microsecond,&tz,
		&isnegative);
	nullvar=
		sqlrcur_getOutputBindString(
			cur,"5");
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
	assertEqStr(nullvar,NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"drop procedure testproc"));
	printf("\n");


	// output bind by name
	printf("OUTPUT BIND BY NAME: \n");
	sqlrcur_sendQuery(cur,
		"drop procedure testproc");
	sqlrcur_getNullsAsNulls(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc "
		"	@out1 int output, "
		"	@out2 varchar(20) output, "
		"	@out3 float output, "
		"	@out4 datetime output, "
		"	@out5 varchar(20) "
		"output as "
		"select @out1=1, "
		"	@out2='hello', "
		"	@out3=2.5, "
		"	@out4='2001-02-03', "
		"	@out5=null"));
	sqlrcur_prepareQuery(cur,
		"exec testproc");
	assertEqInt(
		sqlrcur_countBindVariables(cur),
		0);
	sqlrcur_defineOutputBindInteger(
		cur,"out1");
	sqlrcur_defineOutputBindString(
		cur,"out2",20);
	sqlrcur_defineOutputBindDouble(
		cur,"out3");
	sqlrcur_defineOutputBindDate(
		cur,"out4");
	sqlrcur_defineOutputBindString(
		cur,"out5",20);
	assertTrue(sqlrcur_executeQuery(cur));
	numvar=
		sqlrcur_getOutputBindInteger(
			cur,"out1");
	stringvar=
		sqlrcur_getOutputBindString(
			cur,"out2");
	floatvar=
		sqlrcur_getOutputBindDouble(
			cur,"out3");
	sqlrcur_getOutputBindDate(cur,
		"out4",
		&year,&month,&day,
		&hour,&minute,&second,
		&microsecond,&tz,
		&isnegative);
	nullvar=
		sqlrcur_getOutputBindString(
			cur,"out5");
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
	assertEqStr(nullvar,NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"drop procedure testproc"));
	printf("\n");


	// output bind by name with validation
	// validateBinds() can't be used for
	// output binds, with sap.  In sap, when
	// executing a procedure, you don't declare
	// any bind variable delimiters in the
	// query.  eg, you just do:
	// "exec testproc", not
	// "exec testproc(@out1,@out2)".
	// If you call validateBinds(), it won't
	// find any binds in the query, and will
	// filter out any binds that you declare.


	// lob output bind
	// sap doesn't support lobs as output
	// parameters to stored procedures, and
	// there's no way to directly select into
	// a lob bind variable


	// long output bind
	printf("LONG OUTPUT BIND\n");
	sqlrcur_sendQuery(cur,
		"drop procedure testproc");
	for (i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	snprintf(query,sizeof(query),
		"create procedure testproc "
		"@bindval varchar(%d) "
		"output as "
		"set @bindval='%s'",
		LARGE_BUFFER_LENGTH,
		largebuffer);
	assertTrue(
		sqlrcur_sendQuery(cur,query));
	sqlrcur_prepareQuery(cur,
		"exec testproc");
	sqlrcur_defineOutputBindString(cur,
		"bindval",
		LARGE_BUFFER_LENGTH);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(
		sqlrcur_getOutputBindLength(
			cur,"bindval"),
		LARGE_BUFFER_LENGTH);
	assertEqStr(
		sqlrcur_getOutputBindString(
			cur,"bindval"),
		largebuffer);
	assertTrue(sqlrcur_sendQuery(cur,
		"drop procedure testproc"));
	printf("\n");


	// negative input bind
	printf("NEGATIVE INPUT BIND\n");
	sqlrcur_sendQuery(cur,
		"drop table testtable");
	sqlrcur_sendQuery(cur,
		"create table testtable "
		"(testval int)");
	sqlrcur_prepareQuery(cur,
		"insert into testtable "
		"values (@testval)");
	sqlrcur_inputBindLong(cur,
		"testval",-1);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_sendQuery(cur,
		"select testval "
		"from testtable");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"testval"),
		"-1");
	assertTrue(sqlrcur_sendQuery(cur,
		"drop table testtable"));
	printf("\n");


	// bind validation
	printf("BIND VALIDATION: \n");
	sqlrcur_sendQuery(cur,
		"drop table testtable");
	sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 varchar(20), "
		"	col2 varchar(20), "
		"	col3 varchar(20))");
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	$(var1), "
		"	$(var2), "
		"	$(var3))");
	sqlrcur_inputBindString(cur,
		"var1","1");
	sqlrcur_inputBindString(cur,
		"var2","2");
	sqlrcur_inputBindString(cur,
		"var3","3");
	sqlrcur_subString(cur,
		"var1","@var1");
	assertTrue(
		sqlrcur_validBind(cur,"var1"));
	assertFalse(
		sqlrcur_validBind(cur,"var2"));
	assertFalse(
		sqlrcur_validBind(cur,"var3"));
	assertFalse(
		sqlrcur_validBind(cur,"var4"));
	printf("\n");
	sqlrcur_subString(cur,
		"var2","@var2");
	assertTrue(
		sqlrcur_validBind(cur,"var1"));
	assertTrue(
		sqlrcur_validBind(cur,"var2"));
	assertFalse(
		sqlrcur_validBind(cur,"var3"));
	assertFalse(
		sqlrcur_validBind(cur,"var4"));
	printf("\n");
	sqlrcur_subString(cur,
		"var3","@var3");
	assertTrue(
		sqlrcur_validBind(cur,"var1"));
	assertTrue(
		sqlrcur_validBind(cur,"var2"));
	assertTrue(
		sqlrcur_validBind(cur,"var3"));
	assertFalse(
		sqlrcur_validBind(cur,"var4"));
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,
		"drop table testtable"));
	printf("\n");


	// rebinding
	printf("REBINDING: \n");
	sqlrcur_sendQuery(cur,
		"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc "
		"	@in1 int, "
		"	@out1 int output as "
		"select @out1=@in1"));
	sqlrcur_prepareQuery(cur,
		"exec testproc");
	sqlrcur_inputBindLong(cur,"in1",1);
	sqlrcur_defineOutputBindInteger(
		cur,"out1");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(
		sqlrcur_getOutputBindInteger(
			cur,"out1"),1);
	sqlrcur_inputBindLong(cur,"in1",2);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(
		sqlrcur_getOutputBindInteger(
			cur,"out1"),2);
	sqlrcur_inputBindLong(cur,"in1",3);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(
		sqlrcur_getOutputBindInteger(
			cur,"out1"),3);
	assertTrue(sqlrcur_sendQuery(cur,
		"drop procedure testproc"));
	printf("\n");


	// reexecute
	printf("REEXECUTE: \n");
	sqlrcur_prepareQuery(cur,"select 1");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,0),
		"1");
	printf("\n");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,0),
		"1");
	printf("\n");
	sqlrcur_prepareQuery(cur,
		"begin "
		"	select "
		"cast(@var1 as int) "
		"end");
	sqlrcur_inputBindLong(cur,"var1",1);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,0),
		"1");
	printf("\n");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,0),
		"1");
	printf("\n");
	sqlrcur_inputBindLong(cur,"var1",2);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,0),
		"2");
	printf("\n");


	// stored procedure returning no value
	printf("STORED PROCEDURE "
		"RETURNING NO VALUE: \n");
	sqlrcur_sendQuery(cur,
		"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc "
		"	@in1 int, "
		"	@in2 float, "
		"	@in3 varchar(20) as "
		"return"));
	sqlrcur_prepareQuery(cur,
		"exec testproc");
	sqlrcur_inputBindLong(cur,"in1",1);
	sqlrcur_inputBindDouble(cur,
		"in2",1.1,2,1);
	sqlrcur_inputBindString(cur,
		"in3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,
		"drop procedure testproc"));
	printf("\n");


	// stored procedure returning
	// single value
	printf("STORED PROCEDURE "
		"RETURNING SINGLE VALUE: \n");
	sqlrcur_sendQuery(cur,
		"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc "
		"	@in1 int, "
		"	@in2 float, "
		"	@in3 varchar(20), "
		"	@out1 int output as "
		"select @out1=@in1"));
	sqlrcur_prepareQuery(cur,
		"exec testproc");
	sqlrcur_inputBindLong(cur,"in1",1);
	sqlrcur_inputBindDouble(cur,
		"in2",1.1,2,1);
	sqlrcur_inputBindString(cur,
		"in3","hello");
	sqlrcur_defineOutputBindInteger(
		cur,"out1");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(
		sqlrcur_getOutputBindInteger(
			cur,"out1"),1);
	assertTrue(sqlrcur_sendQuery(cur,
		"drop procedure testproc"));
	printf("\n");


	// stored procedure returning
	// multiple values
	printf("STORED PROCEDURE RETURNING "
		"MULTIPLE VALUES: \n");
	sqlrcur_sendQuery(cur,
		"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc "
		"@in1 int, "
		"	@in2 float, "
		"	@in3 varchar(20), "
		"	@out1 int output, "
		"	@out2 float output, "
		"	@out3 varchar(20) "
		"output as "
		"select @out1=@in1, "
		"	@out2=@in2, "
		"	@out3=@in3"));
	sqlrcur_prepareQuery(cur,
		"exec testproc");
	sqlrcur_inputBindLong(cur,"in1",1);
	sqlrcur_inputBindDouble(cur,
		"in2",1.1,2,1);
	sqlrcur_inputBindString(cur,
		"in3","hello");
	sqlrcur_defineOutputBindInteger(
		cur,"out1");
	sqlrcur_defineOutputBindDouble(
		cur,"out2");
	sqlrcur_defineOutputBindString(
		cur,"out3",20);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(
		sqlrcur_getOutputBindInteger(
			cur,"out1"),1);
	assertEqDbl(
		sqlrcur_getOutputBindDouble(
			cur,"out2"),1.1);
	assertEqStr(
		sqlrcur_getOutputBindString(
			cur,"out3"),
		"hello");
	assertTrue(sqlrcur_sendQuery(cur,
		"drop procedure testproc"));
	printf("\n");


	// stored procedure returning result set
	printf("STORED PROCEDURE "
		"RETURNING RESULT SET: \n");
	sqlrcur_sendQuery(cur,
		"drop procedure "
		"testselectproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure "
		"testselectproc as "
		"	select 1 "
		"	union "
		"	select 2 "
		"	union "
		"	select 3 "
		"	union "
		"	select 4 "
		"	union "
		"	select 5 "
		"	union "
		"	select 6 "
		"	union "
		"	select 7 "
		"	union "
		"	select 8"));
	assertTrue(sqlrcur_sendQuery(cur,
		"exec testselectproc"));
	assertEqInt(sqlrcur_rowCount(cur),8);
	assertTrue(sqlrcur_sendQuery(cur,
		"drop procedure "
		"testselectproc"));
	printf("\n");


	// temporary tables
	printf("TEMPORARY TABLES: \n");
	sqlrcur_sendQuery(cur,
		"drop table #temptable\n");
	sqlrcur_sendQuery(cur,
		"create table #temptable "
		"(col1 int)");
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into #temptable "
		"values (1)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"select count(*) "
		"from #temptable"));
	assertEqStr(
		sqlrcur_getFieldByIndex(cur,0,0),
		"1");
	sqlrcon_endSession(con);
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,
		"select count(*) "
		"from #temptable"));
	printf("\n");


	// encoded binary data
	printf("ENCODED BINARY DATA: \n");
	sqlrcur_sendQuery(cur,
		"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable "
		"(col1 image)"));
	for (bi=0; bi<256; bi++) {
		buffer[bi]=(unsigned char)bi;
	}
	strcpy(querystr,
		"insert into testtable "
		"values (0x");
	for (i=0; i<sizeof(buffer); i++) {
		snprintf(hex,sizeof(hex),
			"%02x",buffer[i]);
		strcat(querystr,hex);
	}
	strcat(querystr,")");
	assertTrue(
		sqlrcur_sendQuery(cur,querystr));
	assertTrue(sqlrcur_sendQuery(cur,
		"select col1 from testtable"));
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,0,0),
		sizeof(buffer));
	assertTrue(memcmp(
		sqlrcur_getFieldByIndex(
			cur,0,0),
		buffer,sizeof(buffer))==0);
	assertTrue(sqlrcur_sendQuery(cur,
		"drop table testtable"));
	printf("\n");


	// quotes
	printf("QUOTES: \n");
	sqlrcur_sendQuery(cur,
		"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable "
		"(col1 varchar(4))"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into testtable "
		"values ('''''')"));
	assertTrue(sqlrcur_sendQuery(cur,
		"select col1 "
		"from testtable"));
	assertEqInt(
		sqlrcur_getFieldLengthByIndex(
			cur,0,0),2);
	assertTrue(strcmp(
		sqlrcur_getFieldByIndex(
			cur,0,0),
		"''")==0);
	assertTrue(sqlrcur_sendQuery(cur,
		"drop table testtable"));
	printf("\n");


	// last insert id
	printf("LAST INSERT ID: \n");
	sqlrcur_sendQuery(cur,
		"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable "
		"	(col1 int identity "
		"primary key, "
		"	col2 int)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into testtable "
		"(col2) values (1)"));
	assertEqInt(
		sqlrcon_getLastInsertId(con),1);
	assertTrue(sqlrcur_sendQuery(cur,
		"drop table testtable"));
	printf("\n");


	// database is schema
	printf("DATABASE IS SCHEMA: \n");
	assertFalse(
		sqlrcon_getDatabaseIsSchema(con));
	printf("\n");


	// catalog list
	printf("CATALOG LIST: \n");
	assertTrue(
		sqlrcur_getCatalogList(
			cur,NULL));
	assertEqStr(
		sqlrcur_getColumnName(cur,0),
		"Database");
	assertTrue(sqlrcur_rowCount(cur)>0);
	printf("\n");


	// schema list
	printf("SCHEMA LIST: \n");
	sqlrcur_sendQuery(cur,
		"drop table testtable");
	// the get schema list query that is
	// used with sap will only return the
	// names of schemas that have at least
	// one database object in them, so to
	// be sure that there is one, we'll
	// create a table
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable "
		"(col1 int)"));
	assertTrue(
		sqlrcur_getSchemaList(
			cur,NULL));
	assertEqStr(
		sqlrcur_getColumnName(cur,0),
		"Database");
	assertTrue(sqlrcur_rowCount(cur)>0);
	assertTrue(sqlrcur_sendQuery(cur,
		"drop table testtable"));
	printf("\n");


	// table type list
	printf("TABLE TYPE LIST: \n");
	assertTrue(
		sqlrcur_getTableTypeList(cur));
	assertEqStr(
		sqlrcur_getColumnName(cur,0),
		"table_type");
	found=0;
	for (i=0;
		i<sqlrcur_rowCount(cur);
		i++) {
		if (!strcmp(
			sqlrcur_getFieldByName(
				cur,i,
				"table_type"),
			"TABLE")) {
			found=1;
			break;
		}
	}
	assertTrue(found);
	printf("\n");


	// table list
	printf("TABLE LIST: \n");
	sqlrcur_sendQuery(cur,
		"drop table testtable1");
	sqlrcur_sendQuery(cur,
		"drop table testtable2");
	sqlrcur_sendQuery(cur,
		"drop table testtable3");
	sqlrcur_sendQuery(cur,
		"drop table testtable4");
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
	assertTrue(
		sqlrcur_getTableList(cur,NULL));
	counter=0;
	for (i=0;
		i<sqlrcur_rowCount(cur);
		i++) {
		name=
			sqlrcur_getFieldByName(
				cur,i,
				"Tables_in_xxx");
		if (!strcmp(name,
				"testtable1") ||
			!strcmp(name,
				"testtable2") ||
			!strcmp(name,
				"testtable3") ||
			!strcmp(name,
				"testtable4")) {
			counter++;
		}
	}
	assertEqInt(counter,4);
	assertTrue(sqlrcur_sendQuery(cur,
		"drop table testtable1"));
	assertTrue(sqlrcur_sendQuery(cur,
		"drop table testtable2"));
	assertTrue(sqlrcur_sendQuery(cur,
		"drop table testtable3"));
	assertTrue(sqlrcur_sendQuery(cur,
		"drop table testtable4"));
	printf("\n");


	// type info list
	printf("TYPE INFO LIST: \n");
	assertTrue(
		sqlrcur_getTypeInfoList(
			cur,"int"));
	assertEqStr(
		sqlrcur_getColumnName(cur,0),
		"type_name");
	assertEqStr(
		sqlrcur_getColumnName(cur,1),
		"data_type");
	assertEqStr(
		sqlrcur_getColumnName(cur,2),
		"precision");
	assertEqStr(
		sqlrcur_getColumnName(cur,3),
		"literal_prefix");
	assertEqStr(
		sqlrcur_getColumnName(cur,4),
		"literal_suffix");
	assertEqStr(
		sqlrcur_getColumnName(cur,5),
		"create_params");
	assertEqStr(
		sqlrcur_getColumnName(cur,6),
		"nullable");
	assertEqStr(
		sqlrcur_getColumnName(cur,7),
		"case_sensitive");
	assertEqStr(
		sqlrcur_getColumnName(cur,8),
		"searchable");
	assertEqStr(
		sqlrcur_getColumnName(cur,9),
		"unsigned_attribute");
	assertEqStr(
		sqlrcur_getColumnName(cur,10),
		"fixed_prec_scale");
	assertEqStr(
		sqlrcur_getColumnName(cur,11),
		"auto_increment");
	assertEqStr(
		sqlrcur_getColumnName(cur,12),
		"local_type_name");
	assertEqStr(
		sqlrcur_getColumnName(cur,13),
		"minumum_scale");
	assertEqStr(
		sqlrcur_getColumnName(cur,14),
		"maxiumm_scale");
	assertEqStr(
		sqlrcur_getColumnName(cur,15),
		"sql_data_type");
	assertEqStr(
		sqlrcur_getColumnName(cur,16),
		"sql_datetime_sub");
	assertEqStr(
		sqlrcur_getColumnName(cur,17),
		"num_prec_radix");
	assertEqStr(
		sqlrcur_getColumnName(cur,18),
		"interval_precision");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"type_name"),
		"INT");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"data_type"),
		"4");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"precision"),
		"10");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"local_type_name"),
		"INT");
	assertTrue(
		sqlrcur_getTypeInfoList(
			cur,"char"));
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"type_name"),
		"CHAR");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"data_type"),
		"1");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"precision"),
		"255");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"local_type_name"),
		"CHAR");
	assertTrue(
		sqlrcur_getTypeInfoList(
			cur,"varchar"));
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"type_name"),
		"VARCHAR");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"data_type"),
		"12");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"precision"),
		"255");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"local_type_name"),
		"VARCHAR");
	assertTrue(
		sqlrcur_getTypeInfoList(
			cur,"datetime"));
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"type_name"),
		"DATETIME");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"data_type"),
		"93");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"precision"),
		"23");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"local_type_name"),
		"DATETIME");
	printf("\n");


	// column list
	printf("COLUMN LIST: \n");
	sqlrcur_sendQuery(cur,
		"drop table testtable");
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
		"	testsmalldatetime "
		"smalldatetime, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testbit bit, "
		"	testtext text)"));
	assertTrue(sqlrcur_getColumnList(
		cur,"testtable",NULL));
	assertEqStr(
		sqlrcur_getColumnName(cur,0),
		"column_name");
	assertEqStr(
		sqlrcur_getColumnName(cur,1),
		"data_type");
	assertEqStr(
		sqlrcur_getColumnName(cur,2),
		"character_maximum_length");
	assertEqStr(
		sqlrcur_getColumnName(cur,3),
		"numeric_precision");
	assertEqStr(
		sqlrcur_getColumnName(cur,4),
		"numeric_scale");
	assertEqStr(
		sqlrcur_getColumnName(cur,5),
		"is_nullable");
	assertEqStr(
		sqlrcur_getColumnName(cur,6),
		"column_key");
	assertEqStr(
		sqlrcur_getColumnName(cur,7),
		"column_default");
	assertEqStr(
		sqlrcur_getColumnName(cur,8),
		"extra");
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,0,"column_name"),
		"testint"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,1,"column_name"),
		"testsmallint"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,2,"column_name"),
		"testtinyint"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,3,"column_name"),
		"testreal"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,4,"column_name"),
		"testfloat"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,5,"column_name"),
		"testdecimal"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,6,"column_name"),
		"testnumeric"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,7,"column_name"),
		"testmoney"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,8,"column_name"),
		"testsmallmoney"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,9,"column_name"),
		"testdatetime"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,10,"column_name"),
		"testsmalldatetime"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,11,"column_name"),
		"testchar"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,12,"column_name"),
		"testvarchar"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,13,"column_name"),
		"testbit"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,14,"column_name"),
		"testtext"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,0,"data_type"),
		"int"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,1,"data_type"),
		"smallint"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,2,"data_type"),
		"tinyint"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,3,"data_type"),
		"real"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,4,"data_type"),
		"float"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,5,"data_type"),
		"decimal"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,6,"data_type"),
		"numeric"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,7,"data_type"),
		"money"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,8,"data_type"),
		"smallmoney"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,9,"data_type"),
		"datetime"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,10,"data_type"),
		"smalldatetime"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,11,"data_type"),
		"char"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,12,"data_type"),
		"varchar"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,13,"data_type"),
		"bit"));
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,14,"data_type"),
		"text"));
	assertTrue(sqlrcur_sendQuery(cur,
		"drop table testtable"));
	printf("\n");


	// column list - auto_increment,
	// primary key
	printf("COLUMN LIST - "
		"auto_increment, "
		"primary key: \n");
	sqlrcur_sendQuery(cur,
		"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 int identity "
		"primary key, "
		"	col2 int)"));
	assertTrue(sqlrcur_getColumnList(
		cur,"testtable",NULL));
	assertTrue(strstr(
		sqlrcur_getFieldByName(
			cur,0,"extra"),
		"auto_increment")!=NULL);
	assertTrue(strstr(
		sqlrcur_getFieldByName(
			cur,0,"column_key"),
		"PRI")!=NULL);
	assertFalse(strstr(
		sqlrcur_getFieldByName(
			cur,1,"extra"),
		"auto_increment")!=NULL);
	assertFalse(strstr(
		sqlrcur_getFieldByName(
			cur,1,"column_key"),
		"PRI")!=NULL);
	printf("\n");
	assertTrue(sqlrcur_sendQuery(cur,
		"drop table testtable"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 int "
		"primary key, "
		"	col2 int)"));
	assertTrue(sqlrcur_getColumnList(
		cur,"testtable",NULL));
	assertFalse(strstr(
		sqlrcur_getFieldByName(
			cur,0,"extra"),
		"auto_increment")!=NULL);
	assertTrue(strstr(
		sqlrcur_getFieldByName(
			cur,0,"column_key"),
		"PRI")!=NULL);
	assertTrue(sqlrcur_sendQuery(cur,
		"drop table testtable"));
	printf("\n");


	// primary keys list
	printf("PRIMARY KEYS LIST: \n");
	sqlrcur_sendQuery(cur,
		"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 int "
		"primary key, "
		"	col2 int)"));
	assertTrue(
		sqlrcur_getPrimaryKeysList(
			cur,"testtable",NULL));
	assertEqStr(
		sqlrcur_getColumnName(cur,0),
		"table");
	assertEqStr(
		sqlrcur_getColumnName(cur,1),
		"non_unique");
	assertEqStr(
		sqlrcur_getColumnName(cur,2),
		"key_name");
	assertEqStr(
		sqlrcur_getColumnName(cur,3),
		"seq_in_index");
	assertEqStr(
		sqlrcur_getColumnName(cur,4),
		"column_name");
	assertEqStr(
		sqlrcur_getColumnName(cur,5),
		"collation");
	assertEqStr(
		sqlrcur_getColumnName(cur,6),
		"cardinality");
	assertEqStr(
		sqlrcur_getColumnName(cur,7),
		"sub_part");
	assertEqStr(
		sqlrcur_getColumnName(cur,8),
		"packed");
	assertEqStr(
		sqlrcur_getColumnName(cur,9),
		"null");
	assertEqStr(
		sqlrcur_getColumnName(cur,10),
		"index_type");
	assertEqStr(
		sqlrcur_getColumnName(cur,11),
		"comment");
	assertEqStr(
		sqlrcur_getColumnName(cur,12),
		"index_comment");
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,0,"table"),
		"testtable"));
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"seq_in_index"),
		"1");
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,0,"column_name"),
		"col1"));
	assertTrue(
		sqlrcur_getFieldByName(
			cur,0,"key_name")
		!=NULL &&
		strlen(
			sqlrcur_getFieldByName(
				cur,0,
				"key_name"))
		>0);
	assertTrue(sqlrcur_sendQuery(cur,
		"drop table testtable"));
	printf("\n");


	// key and index list
	printf("KEY AND INDEX LIST: \n");
	sqlrcur_sendQuery(cur,
		"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 int "
		"primary key, "
		"	col2 int)"));
	assertTrue(
		sqlrcur_getKeyAndIndexList(
			cur,"testtable",NULL));
	assertEqStr(
		sqlrcur_getColumnName(cur,0),
		"table");
	assertEqStr(
		sqlrcur_getColumnName(cur,1),
		"non_unique");
	assertEqStr(
		sqlrcur_getColumnName(cur,2),
		"key_name");
	assertEqStr(
		sqlrcur_getColumnName(cur,3),
		"seq_in_index");
	assertEqStr(
		sqlrcur_getColumnName(cur,4),
		"column_name");
	assertEqStr(
		sqlrcur_getColumnName(cur,5),
		"collation");
	assertEqStr(
		sqlrcur_getColumnName(cur,6),
		"cardinality");
	assertEqStr(
		sqlrcur_getColumnName(cur,7),
		"sub_part");
	assertEqStr(
		sqlrcur_getColumnName(cur,8),
		"packed");
	assertEqStr(
		sqlrcur_getColumnName(cur,9),
		"null");
	assertEqStr(
		sqlrcur_getColumnName(cur,10),
		"index_type");
	assertEqStr(
		sqlrcur_getColumnName(cur,11),
		"comment");
	assertEqStr(
		sqlrcur_getColumnName(cur,12),
		"index_comment");
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,0,"table"),
		"testtable"));
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"non_unique"),
		"FALSE");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"seq_in_index"),
		"1");
	assertTrue(!strcmp(
		sqlrcur_getFieldByName(
			cur,0,"column_name"),
		"col1"));
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"collation"),
		"A");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"index_type"),
		"1");
	assertTrue(
		sqlrcur_getFieldByName(
			cur,0,"key_name")
		!=NULL &&
		strlen(
			sqlrcur_getFieldByName(
				cur,0,
				"key_name"))
		>0);
	assertTrue(sqlrcur_sendQuery(cur,
		"drop table testtable"));
	printf("\n");


	// procedure list
	printf("PROCEDURE LIST: \n");
	sqlrcur_sendQuery(cur,
		"drop procedure testproc1");
	sqlrcur_sendQuery(cur,
		"drop procedure testproc2");
	sqlrcur_sendQuery(cur,
		"drop procedure testproc3");
	sqlrcur_sendQuery(cur,
		"drop procedure testproc4");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc1 "
		"	@in1 int, "
		"	@in2 char(20), "
		"	@in3 varchar(20), "
		"	@in4 datetime "
		"as select 1"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc2 "
		"	@in1 int, "
		"	@in2 char(20), "
		"	@in3 varchar(20), "
		"	@in4 datetime "
		"as select 1"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc3 "
		"	@in1 int, "
		"	@in2 char(20), "
		"	@in3 varchar(20), "
		"	@in4 datetime "
		"as select 1"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc4 "
		"	@in1 int, "
		"	@in2 char(20), "
		"	@in3 varchar(20), "
		"	@in4 datetime "
		"as select 1"));
	assertTrue(
		sqlrcur_getProcedureList(
			cur,NULL));
	counter=0;
	for (i=0;
		i<sqlrcur_rowCount(cur);
		i++) {
		name=
			sqlrcur_getFieldByName(
				cur,i,
				"routine_name");
		if (!strcmp(name,
				"testproc1") ||
			!strcmp(name,
				"testproc2") ||
			!strcmp(name,
				"testproc3") ||
			!strcmp(name,
				"testproc4")) {
			counter++;
		}
	}
	assertEqInt(counter,4);
	printf("\n");


	// procedure parameter list
	printf("PROCEDURE "
		"PARAMETER LIST: \n");
	assertTrue(
		sqlrcur_getProcedureParameterList(
			cur,
			"testproc1",NULL));
	assertEqStr(
		sqlrcur_getColumnName(cur,0),
		"parameter_name");
	assertEqStr(
		sqlrcur_getColumnName(cur,1),
		"parameter_mode");
	assertEqStr(
		sqlrcur_getColumnName(cur,2),
		"data_type");
	assertEqStr(
		sqlrcur_getColumnName(cur,3),
		"character_maximum_length");
	assertEqStr(
		sqlrcur_getColumnName(cur,4),
		"ordinal_position");
	assertEqInt(sqlrcur_rowCount(cur),4);
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"parameter_name"),
		"@in1");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"parameter_mode"),
		"1");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,"data_type"),
		"int");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,0,
			"ordinal_position"),
		"1");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,1,"parameter_name"),
		"@in2");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,1,"parameter_mode"),
		"1");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,1,"data_type"),
		"char");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,1,
			"ordinal_position"),
		"2");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,2,"parameter_name"),
		"@in3");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,2,"parameter_mode"),
		"1");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,2,"data_type"),
		"varchar");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,2,
			"ordinal_position"),
		"3");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,3,"parameter_name"),
		"@in4");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,3,"parameter_mode"),
		"1");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,3,"data_type"),
		"datetime");
	assertEqStr(
		sqlrcur_getFieldByName(
			cur,3,
			"ordinal_position"),
		"4");
	assertTrue(sqlrcur_sendQuery(cur,
		"drop procedure testproc1"));
	assertTrue(sqlrcur_sendQuery(cur,
		"drop procedure testproc2"));
	assertTrue(sqlrcur_sendQuery(cur,
		"drop procedure testproc3"));
	assertTrue(sqlrcur_sendQuery(cur,
		"drop procedure testproc4"));
	printf("\n");


	// invalid queries
	printf("INVALID QUERIES: \n");
	assertFalse(sqlrcur_sendQuery(cur,
		"select * from testtable "
		"order by testint"));
	assertFalse(sqlrcur_sendQuery(cur,
		"select * from testtable "
		"order by testint"));
	assertFalse(sqlrcur_sendQuery(cur,
		"select * from testtable "
		"order by testint"));
	assertFalse(sqlrcur_sendQuery(cur,
		"select * from testtable "
		"order by testint"));
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into testtable "
		"values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into testtable "
		"values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into testtable "
		"values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into testtable "
		"values (1,2,3,4)"));
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,
		"create table testtable"));
	assertFalse(sqlrcur_sendQuery(cur,
		"create table testtable"));
	assertFalse(sqlrcur_sendQuery(cur,
		"create table testtable"));
	assertFalse(sqlrcur_sendQuery(cur,
		"create table testtable"));
	printf("\n");

	reportTestStatus();

	return status;
}
