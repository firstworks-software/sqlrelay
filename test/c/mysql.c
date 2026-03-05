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

	const char	*dbversion;
	uint32_t	majorversion;
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
	assertEqualsString(sqlrcon_identify(con),"mysql");
	printf("\n");

	// get the db version
	dbversion=sqlrcon_dbVersion(con);
	majorversion=dbversion[0]-'0';


	// ping
	printf("PING: \n");
	assertTrue(sqlrcon_ping(con));
	printf("\n");


	// isolation levels
	printf("ISOLATION LEVELS: \n");
	{
		const char	*isolationlevels[]={
					"REPEATABLE-READ",
					"READ-UNCOMMITTED",
					"READ-COMMITTED",
					"SERIALIZABLE",NULL};
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

	// create a new table


	// create temptable
	printf("CREATE TEMPTABLE: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testtinyint tinyint, "
		"	testsmallint smallint, "
		"	testmediumint mediumint, "
		"	testint int, "
		"	testbigint bigint, "
		"	testfloat float, "
		"	testreal real, "
		"	testdecimal decimal(2,1), "
		"	testdate date, "
		"	testtime time, "
		"	testdatetime datetime, "
		"	testyear year, "
		"	testchar char(40), "
		"	testtext text, "
		"	testvarchar varchar(40), "
		"	testtinytext tinytext, "
		"	testmediumtext mediumtext, "
		"	testlongtext longtext, "
		"	testtimestamp timestamp)"));
	printf("\n");


	// begin transaction
	printf("BEGIN TRANSACTION: \n");
	assertTrue(sqlrcur_sendQuery(cur,"begin"));
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
		"	1, "
		"	1, "
		"	1.1, "
		"	1.1, "
		"	1.1, "
		"	'2001-01-01', "
		"	'01:00:00', "
		"	'2001-01-01 01:00:00', "
		"	'2001', "
		"	'char1', "
		"	'text1', "
		"	'varchar1', "
		"	'tinytext1', "
		"	'mediumtext1', "
		"	'longtext1', "
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	2, "
		"	2, "
		"	2, "
		"	2, "
		"	2, "
		"	2.1, "
		"	2.1, "
		"	2.1, "
		"	'2002-01-01', "
		"	'02:00:00', "
		"	'2002-01-01 02:00:00', "
		"	'2002', "
		"	'char2', "
		"	'text2', "
		"	'varchar2', "
		"	'tinytext2', "
		"	'mediumtext2', "
		"	'longtext2', "
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	3, "
		"	3, "
		"	3, "
		"	3, "
		"	3, "
		"	3.1, "
		"	3.1, "
		"	3.1, "
		"	'2003-01-01', "
		"	'03:00:00', "
		"	'2003-01-01 03:00:00', "
		"	'2003', "
		"	'char3', "
		"	'text3', "
		"	'varchar3', "
		"	'tinytext3', "
		"	'mediumtext3', "
		"	'longtext3', "
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	4, "
		"	4, "
		"	4, "
		"	4, "
		"	4, "
		"	4.1, "
		"	4.1, "
		"	4.1, "
		"	'2004-01-01', "
		"	'04:00:00', "
		"	'2004-01-01 04:00:00', "
		"	'2004', "
		"	'char4', "
		"	'text4', "
		"	'varchar4', "
		"	'tinytext4', "
		"	'mediumtext4', "
		"	'longtext4', "
		"	NULL)"));
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
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	NULL)");
	assertEqualsInt(sqlrcur_countBindVariables(cur),18);
	sqlrcur_inputBindLong(cur,"1",5);
	sqlrcur_inputBindLong(cur,"2",5);
	sqlrcur_inputBindLong(cur,"3",5);
	sqlrcur_inputBindLong(cur,"4",5);
	sqlrcur_inputBindLong(cur,"5",5);
	sqlrcur_inputBindDouble(cur,"6",5.1,2,1);
	sqlrcur_inputBindDouble(cur,"7",5.1,2,1);
	sqlrcur_inputBindDouble(cur,"8",5.1,2,1);
	sqlrcur_inputBindString(cur,"9","2005-01-01");
	sqlrcur_inputBindString(cur,"10","05:00:00");
	sqlrcur_inputBindString(cur,"11","2005-01-01 05:00:00");
	sqlrcur_inputBindString(cur,"12","2005");
	sqlrcur_inputBindString(cur,"13","char5");
	sqlrcur_inputBindString(cur,"14","text5");
	sqlrcur_inputBindString(cur,"15","varchar5");
	sqlrcur_inputBindString(cur,"16","tinytext5");
	sqlrcur_inputBindString(cur,"17","mediumtext5");
	sqlrcur_inputBindString(cur,"18","longtext5");
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",6);
	sqlrcur_inputBindLong(cur,"2",6);
	sqlrcur_inputBindLong(cur,"3",6);
	sqlrcur_inputBindLong(cur,"4",6);
	sqlrcur_inputBindLong(cur,"5",6);
	sqlrcur_inputBindDouble(cur,"6",6.1,2,1);
	sqlrcur_inputBindDouble(cur,"7",6.1,2,1);
	sqlrcur_inputBindDouble(cur,"8",6.1,2,1);
	sqlrcur_inputBindString(cur,"9","2006-01-01");
	sqlrcur_inputBindString(cur,"10","06:00:00");
	sqlrcur_inputBindString(cur,"11","2006-01-01 06:00:00");
	sqlrcur_inputBindString(cur,"12","2006");
	sqlrcur_inputBindString(cur,"13","char6");
	sqlrcur_inputBindString(cur,"14","text6");
	sqlrcur_inputBindString(cur,"15","varchar6");
	sqlrcur_inputBindString(cur,"16","tinytext6");
	sqlrcur_inputBindString(cur,"17","mediumtext6");
	sqlrcur_inputBindString(cur,"18","longtext6");
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",7);
	sqlrcur_inputBindLong(cur,"2",7);
	sqlrcur_inputBindLong(cur,"3",7);
	sqlrcur_inputBindLong(cur,"4",7);
	sqlrcur_inputBindLong(cur,"5",7);
	sqlrcur_inputBindDouble(cur,"6",7.1,2,1);
	sqlrcur_inputBindDouble(cur,"7",7.1,2,1);
	sqlrcur_inputBindDouble(cur,"8",7.1,2,1);
	sqlrcur_inputBindString(cur,"9","2007-01-01");
	sqlrcur_inputBindString(cur,"10","07:00:00");
	sqlrcur_inputBindString(cur,"11","2007-01-01 07:00:00");
	sqlrcur_inputBindString(cur,"12","2007");
	sqlrcur_inputBindString(cur,"13","char7");
	sqlrcur_inputBindString(cur,"14","text7");
	sqlrcur_inputBindString(cur,"15","varchar7");
	sqlrcur_inputBindString(cur,"16","tinytext7");
	sqlrcur_inputBindString(cur,"17","mediumtext7");
	sqlrcur_inputBindString(cur,"18","longtext7");
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// bind by position with validation
	printf("BIND BY POSITION WITH VALIDATION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",8);
	sqlrcur_inputBindLong(cur,"2",8);
	sqlrcur_inputBindLong(cur,"3",8);
	sqlrcur_inputBindLong(cur,"4",8);
	sqlrcur_inputBindLong(cur,"5",8);
	sqlrcur_inputBindDouble(cur,"6",8.1,2,1);
	sqlrcur_inputBindDouble(cur,"7",8.1,2,1);
	sqlrcur_inputBindDouble(cur,"8",8.1,2,1);
	sqlrcur_inputBindString(cur,"9","2008-01-01");
	sqlrcur_inputBindString(cur,"10","08:00:00");
	sqlrcur_inputBindString(cur,"11","2008-01-01 08:00:00");
	sqlrcur_inputBindString(cur,"12","2008");
	sqlrcur_inputBindString(cur,"13","char8");
	sqlrcur_inputBindString(cur,"14","text8");
	sqlrcur_inputBindString(cur,"15","varchar8");
	sqlrcur_inputBindString(cur,"16","tinytext8");
	sqlrcur_inputBindString(cur,"17","mediumtext8");
	sqlrcur_inputBindString(cur,"18","longtext8");
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
		"	testtinyint "));
	printf("\n");


	// column count
	printf("COLUMN COUNT: \n");
	assertEqualsInt(sqlrcur_colCount(cur),19);
	printf("\n");


	// column names
	printf("COLUMN NAMES: \n");
	assertEqualsString(sqlrcur_getColumnName(cur,0),"testtinyint");
	assertEqualsString(sqlrcur_getColumnName(cur,1),"testsmallint");
	assertEqualsString(sqlrcur_getColumnName(cur,2),"testmediumint");
	assertEqualsString(sqlrcur_getColumnName(cur,3),"testint");
	assertEqualsString(sqlrcur_getColumnName(cur,4),"testbigint");
	assertEqualsString(sqlrcur_getColumnName(cur,5),"testfloat");
	assertEqualsString(sqlrcur_getColumnName(cur,6),"testreal");
	assertEqualsString(sqlrcur_getColumnName(cur,7),"testdecimal");
	assertEqualsString(sqlrcur_getColumnName(cur,8),"testdate");
	assertEqualsString(sqlrcur_getColumnName(cur,9),"testtime");
	assertEqualsString(sqlrcur_getColumnName(cur,10),"testdatetime");
	assertEqualsString(sqlrcur_getColumnName(cur,11),"testyear");
	assertEqualsString(sqlrcur_getColumnName(cur,12),"testchar");
	assertEqualsString(sqlrcur_getColumnName(cur,13),"testtext");
	assertEqualsString(sqlrcur_getColumnName(cur,14),"testvarchar");
	assertEqualsString(sqlrcur_getColumnName(cur,15),"testtinytext");
	assertEqualsString(sqlrcur_getColumnName(cur,16),"testmediumtext");
	assertEqualsString(sqlrcur_getColumnName(cur,17),"testlongtext");
	assertEqualsString(sqlrcur_getColumnName(cur,18),"testtimestamp");
	cols=sqlrcur_getColumnNames(cur);
	assertEqualsString(cols[0],"testtinyint");
	assertEqualsString(cols[1],"testsmallint");
	assertEqualsString(cols[2],"testmediumint");
	assertEqualsString(cols[3],"testint");
	assertEqualsString(cols[4],"testbigint");
	assertEqualsString(cols[5],"testfloat");
	assertEqualsString(cols[6],"testreal");
	assertEqualsString(cols[7],"testdecimal");
	assertEqualsString(cols[8],"testdate");
	assertEqualsString(cols[9],"testtime");
	assertEqualsString(cols[10],"testdatetime");
	assertEqualsString(cols[11],"testyear");
	assertEqualsString(cols[12],"testchar");
	assertEqualsString(cols[13],"testtext");
	assertEqualsString(cols[14],"testvarchar");
	assertEqualsString(cols[15],"testtinytext");
	assertEqualsString(cols[16],"testmediumtext");
	assertEqualsString(cols[17],"testlongtext");
	assertEqualsString(cols[18],"testtimestamp");
	printf("\n");


	// column types
	printf("COLUMN TYPES: \n");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),"TINYINT");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,1),"SMALLINT");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,2),"MEDIUMINT");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,3),"INT");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,4),"BIGINT");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,5),"FLOAT");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,6),"REAL");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,7),"DECIMAL");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,8),"DATE");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,9),"TIME");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,10),"DATETIME");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,11),"YEAR");
	if (majorversion==3) {
		assertEqualsString(
			sqlrcur_getColumnTypeByIndex(cur,12),"VARSTRING");
	} else {
		assertEqualsString(
			sqlrcur_getColumnTypeByIndex(cur,12),"STRING");
	}
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,13),"BLOB");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,14),"VARSTRING");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,15),"TINYBLOB");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,16),"MEDIUMBLOB");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,17),"LONGBLOB");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,18),"TIMESTAMP");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testtinyint"),"TINYINT");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testsmallint"),"SMALLINT");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testmediumint"),"MEDIUMINT");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testint"),"INT");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testbigint"),"BIGINT");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testfloat"),"FLOAT");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testreal"),"REAL");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testdecimal"),"DECIMAL");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testdate"),"DATE");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testtime"),"TIME");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testdatetime"),"DATETIME");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testyear"),"YEAR");
	if (majorversion==3) {
		assertEqualsString(
		sqlrcur_getColumnTypeByName(cur,"testchar"),"VARSTRING");
	} else {
		assertEqualsString(
		sqlrcur_getColumnTypeByName(cur,"testchar"),"STRING");
	}
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testtext"),"BLOB");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testvarchar"),"VARSTRING");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testtinytext"),"TINYBLOB");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testmediumtext"),"MEDIUMBLOB");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testlongtext"),"LONGBLOB");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"testtimestamp"),"TIMESTAMP");
	printf("\n");


	// column length
	printf("COLUMN LENGTH: \n");
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,0),1);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,1),2);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,2),3);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,3),4);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,4),8);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,5),4);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,6),8);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,7),6);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,8),3);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,9),3);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,10),8);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,11),1);
	//assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,12),40);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,13),65535);
	//assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,14),41);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,15),255);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,16),16777215);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,17),2147483647);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,18),4);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testtinyint"),1);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testsmallint"),2);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testmediumint"),3);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testint"),4);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testbigint"),8);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testfloat"),4);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testreal"),8);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testdecimal"),6);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testdate"),3);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testtime"),3);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testdatetime"),8);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testyear"),1);
	//assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testchar"),40);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testtext"),65535);
	//assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testvarchar"),41);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testtinytext"),255);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testmediumtext"),16777215);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testlongtext"),2147483647);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"testtimestamp"),4);
	printf("\n");


	// longest column
	printf("LONGEST COLUMN: \n");
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,0),1);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,1),1);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,2),1);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,3),1);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,4),1);
	//assertEqualsInt(sqlrcur_getLongestByIndex(cur,5),3);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,6),3);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,7),3);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,8),10);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,9),8);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,10),19);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,11),4);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,12),5);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,13),5);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,14),8);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,15),9);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,16),11);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,17),9);
	if (majorversion==3) {
		assertEqualsInt(sqlrcur_getLongestByIndex(cur,18),14);
	} else {
		assertEqualsInt(sqlrcur_getLongestByIndex(cur,18),19);
	}
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testtinyint"),1);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testsmallint"),1);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testmediumint"),1);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testint"),1);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testbigint"),1);
	//assertEqualsInt(sqlrcur_getLongestByName(cur,"testfloat"),3);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testreal"),3);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testdecimal"),3);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testdate"),10);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testtime"),8);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testdatetime"),19);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testyear"),4);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testchar"),5);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testtext"),5);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testvarchar"),8);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testtinytext"),9);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testmediumtext"),11);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"testlongtext"),9);
	if (majorversion==3) {
		assertEqualsInt(
			sqlrcur_getLongestByName(cur,"testtimestamp"),14);
	} else {
		assertEqualsInt(
			sqlrcur_getLongestByName(cur,"testtimestamp"),19);
	}
	printf("\n");


	// row count
	printf("ROW COUNT: \n");
	assertEqualsInt(sqlrcur_rowCount(cur),8);
	printf("\n");


	// total rows
	printf("TOTAL ROWS: \n");
	// older versions of mysql know this
	//assertEqualsInt(sqlrcur_totalRows(cur),0);
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
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,3),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,4),"1");
	//assertEqualsString(sqlrcur_getFieldByIndex(cur,0,5),"1.1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,6),"1.1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,7),"1.1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,8),"2001-01-01");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,9),"01:00:00");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,10),"2001-01-01 01:00:00");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,11),"2001");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,12),"char1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,13),"text1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,14),"varchar1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,15),"tinytext1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,16),"mediumtext1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,17),"longtext1");
	printf("\n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,1),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,2),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,3),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,4),"8");
	//assertEqualsString(sqlrcur_getFieldByIndex(cur,7,5),"8.1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,6),"8.1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,7),"8.1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,8),"2008-01-01");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,9),"08:00:00");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,10),"2008-01-01 08:00:00");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,11),"2008");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,12),"char8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,13),"text8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,14),"varchar8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,15),"tinytext8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,16),"mediumtext8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,17),"longtext8");
	printf("\n");


	// field lengths by index
	printf("FIELD LENGTHS BY INDEX: \n");
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,0),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,1),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,2),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,3),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,4),1);
	//assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,5),3);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,6),3);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,7),3);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,8),10);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,9),8);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,10),19);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,11),4);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,12),5);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,13),5);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,14),8);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,15),9);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,16),11);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,17),9);
	printf("\n");
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,0),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,1),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,2),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,3),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,4),1);
	//assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,5),3);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,6),3);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,7),3);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,8),10);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,9),8);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,10),19);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,11),4);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,12),5);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,13),5);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,14),8);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,15),9);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,16),11);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,17),9);
	printf("\n");


	// fields by name
	printf("FIELDS BY NAME: \n");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testtinyint"),"1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testsmallint"),"1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testmediumint"),"1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testint"),"1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testbigint"),"1");
	//assertEqualsString(sqlrcur_getFieldByName(cur,0,"testfloat"),"1.1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testreal"),"1.1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testdecimal"),"1.1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testdate"),"2001-01-01");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testtime"),"01:00:00");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testdatetime"),"2001-01-01 01:00:00");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testyear"),"2001");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testchar"),"char1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testtext"),"text1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testvarchar"),"varchar1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testtinytext"),"tinytext1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testmediumtext"),"mediumtext1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"testlongtext"),"longtext1");
	printf("\n");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testtinyint"),"8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testsmallint"),"8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testmediumint"),"8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testint"),"8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testbigint"),"8");
	//assertEqualsString(sqlrcur_getFieldByName(cur,7,"testfloat"),"8.1");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testreal"),"8.1");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testdecimal"),"8.1");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testdate"),"2008-01-01");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testtime"),"08:00:00");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testdatetime"),"2008-01-01 08:00:00");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testyear"),"2008");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testchar"),"char8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testtext"),"text8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testvarchar"),"varchar8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testtinytext"),"tinytext8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testmediumtext"),"mediumtext8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"testlongtext"),"longtext8");
	printf("\n");


	// field lengths by name
	printf("FIELD LENGTHS BY NAME: \n");
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testtinyint"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testsmallint"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testmediumint"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testint"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testbigint"),1);
	//assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testfloat"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testreal"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testdecimal"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testdate"),10);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testtime"),8);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testdatetime"),19);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testyear"),4);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testchar"),5);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testtext"),5);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testvarchar"),8);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testtinytext"),9);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testmediumtext"),11);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"testlongtext"),9);
	printf("\n");
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testtinyint"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testsmallint"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testmediumint"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testint"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testbigint"),1);
	//assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testfloat"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testreal"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testdecimal"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testdate"),10);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testtime"),8);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testdatetime"),19);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testyear"),4);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testchar"),5);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testtext"),5);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testvarchar"),8);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testtinytext"),9);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testmediumtext"),11);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"testlongtext"),9);
	printf("\n");


	// fields by array
	printf("FIELDS BY ARRAY: \n");
	fields=sqlrcur_getRow(cur,0);
	assertEqualsString(fields[0],"1");
	assertEqualsString(fields[1],"1");
	assertEqualsString(fields[2],"1");
	assertEqualsString(fields[3],"1");
	assertEqualsString(fields[4],"1");
	//assertEqualsString(fields[5],"1.1");
	assertEqualsString(fields[6],"1.1");
	assertEqualsString(fields[7],"1.1");
	assertEqualsString(fields[8],"2001-01-01");
	assertEqualsString(fields[9],"01:00:00");
	assertEqualsString(fields[10],"2001-01-01 01:00:00");
	assertEqualsString(fields[11],"2001");
	assertEqualsString(fields[12],"char1");
	assertEqualsString(fields[13],"text1");
	assertEqualsString(fields[14],"varchar1");
	assertEqualsString(fields[15],"tinytext1");
	assertEqualsString(fields[16],"mediumtext1");
	assertEqualsString(fields[17],"longtext1");
	printf("\n");


	// field lengths by array
	printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=sqlrcur_getRowLengths(cur,0);
	assertEqualsInt(fieldlens[0],1);
	assertEqualsInt(fieldlens[1],1);
	assertEqualsInt(fieldlens[2],1);
	assertEqualsInt(fieldlens[3],1);
	assertEqualsInt(fieldlens[4],1);
	//assertEqualsInt(fieldlens[5],3);
	assertEqualsInt(fieldlens[6],3);
	assertEqualsInt(fieldlens[7],3);
	assertEqualsInt(fieldlens[8],10);
	assertEqualsInt(fieldlens[9],8);
	assertEqualsInt(fieldlens[10],19);
	assertEqualsInt(fieldlens[11],4);
	assertEqualsInt(fieldlens[12],5);
	assertEqualsInt(fieldlens[13],5);
	assertEqualsInt(fieldlens[14],8);
	assertEqualsInt(fieldlens[15],9);
	assertEqualsInt(fieldlens[16],11);
	assertEqualsInt(fieldlens[17],9);
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
		"	testtinyint "));
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
		"	testtinyint "));
	assertEqualsString(sqlrcur_getColumnName(cur,0),NULL);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,0),0);
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),NULL);
	printf("\n");
	sqlrcur_getColumnInfo(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	assertEqualsString(sqlrcur_getColumnName(cur,0),"testtinyint");
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,0),1);
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),"TINYINT");
	printf("\n");


	// suspended session
	printf("SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
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
		"	testtinyint "));
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
		"	testtinyint "));
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
		"	testtinyint "));
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
		"	testtinyint "));
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqualsString(filename,"cachefile1");
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	free(filename);
	printf("\n");


	// column count for cached result set
	printf("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqualsInt(sqlrcur_colCount(cur),19);
	printf("\n");


	// column names for cached result set
	printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqualsString(sqlrcur_getColumnName(cur,0),"testtinyint");
	assertEqualsString(sqlrcur_getColumnName(cur,1),"testsmallint");
	assertEqualsString(sqlrcur_getColumnName(cur,2),"testmediumint");
	assertEqualsString(sqlrcur_getColumnName(cur,3),"testint");
	assertEqualsString(sqlrcur_getColumnName(cur,4),"testbigint");
	assertEqualsString(sqlrcur_getColumnName(cur,5),"testfloat");
	assertEqualsString(sqlrcur_getColumnName(cur,6),"testreal");
	assertEqualsString(sqlrcur_getColumnName(cur,7),"testdecimal");
	assertEqualsString(sqlrcur_getColumnName(cur,8),"testdate");
	assertEqualsString(sqlrcur_getColumnName(cur,9),"testtime");
	assertEqualsString(sqlrcur_getColumnName(cur,10),"testdatetime");
	assertEqualsString(sqlrcur_getColumnName(cur,11),"testyear");
	assertEqualsString(sqlrcur_getColumnName(cur,12),"testchar");
	assertEqualsString(sqlrcur_getColumnName(cur,13),"testtext");
	assertEqualsString(sqlrcur_getColumnName(cur,14),"testvarchar");
	assertEqualsString(sqlrcur_getColumnName(cur,15),"testtinytext");
	assertEqualsString(sqlrcur_getColumnName(cur,16),"testmediumtext");
	assertEqualsString(sqlrcur_getColumnName(cur,17),"testlongtext");
	cols=sqlrcur_getColumnNames(cur);
	assertEqualsString(cols[0],"testtinyint");
	assertEqualsString(cols[1],"testsmallint");
	assertEqualsString(cols[2],"testmediumint");
	assertEqualsString(cols[3],"testint");
	assertEqualsString(cols[4],"testbigint");
	assertEqualsString(cols[5],"testfloat");
	assertEqualsString(cols[6],"testreal");
	assertEqualsString(cols[7],"testdecimal");
	assertEqualsString(cols[8],"testdate");
	assertEqualsString(cols[9],"testtime");
	assertEqualsString(cols[10],"testdatetime");
	assertEqualsString(cols[11],"testyear");
	assertEqualsString(cols[12],"testchar");
	assertEqualsString(cols[13],"testtext");
	assertEqualsString(cols[14],"testvarchar");
	assertEqualsString(cols[15],"testtinytext");
	assertEqualsString(cols[16],"testmediumtext");
	assertEqualsString(cols[17],"testlongtext");
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
		"	testtinyint "));
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
		"	testtinyint "));
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
	// Note: Mysql's default isolation level is repeatable-read,
	// not read-committed like most other db's.  Both sessions must
	// commit to see the changes that each other has made.
	secondcon=sqlrcon_alloc("sqlrelay",9000,
			"/tmp/test.socket","testuser","testpassword",0,1);
	secondcur=sqlrcur_alloc(secondcon);
	assertTrue(sqlrcur_sendQuery(secondcur,
		"select "
		"	count(*) "
		"from "
		"	testtable "));
	if (majorversion>3) {
		assertEqualsString(sqlrcur_getFieldByIndex(secondcur,0,0),"0");
	} else {
		assertEqualsString(sqlrcur_getFieldByIndex(secondcur,0,0),"8");
	}
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcon_commit(secondcon));
	assertTrue(sqlrcur_sendQuery(secondcur,
		"select "
		"	count(*) "
		"from "
		"	testtable "));
	assertEqualsString(sqlrcur_getFieldByIndex(secondcur,0,0),"8");
	assertTrue(sqlrcon_autoCommitOn(con));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	10, "
		"	10, "
		"	10, "
		"	10, "
		"	10, "
		"	10.1, "
		"	10.1, "
		"	1.1, "
		"	'2010-01-01', "
		"	'10:00:00', "
		"	'2010-01-01 10:00:00', "
		"	'2010', "
		"	'char10', "
		"	'text10', "
		"	'varchar10', "
		"	'tinytext10', "
		"	'mediumtext10', "
		"	'longtext10', "
		"	NULL)"));
	assertTrue(sqlrcon_commit(secondcon));
	assertTrue(sqlrcur_sendQuery(secondcur,
		"select "
		"	count(*) "
		"from "
		"	testtable "));
	assertEqualsString(sqlrcur_getFieldByIndex(secondcur,0,0),"9");
	assertTrue(sqlrcon_autoCommitOff(con));
	sqlrcon_commit(secondcon);
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
		"	testtinyint "));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
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
