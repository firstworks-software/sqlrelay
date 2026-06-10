// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclientwrapper.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include "asserts.c"

sqlrcon	con;
sqlrcur	cur;
sqlrcon	secondcon;
sqlrcur	secondcur;

int main(int argc, char **argv) {

	const char	*isolationlevels[]={
				"REPEATABLE-READ","READ-UNCOMMITTED",
				"READ-COMMITTED","SERIALIZABLE",NULL};
	const char	**il;
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
	const char	*dbversion;
	uint32_t	majorversion;
	unsigned int	seed1;
	unsigned int	seed2;
	uint32_t	result1;
	uint32_t	result2;
	unsigned char	buffer[256];
	char		ch[]={'\'','"','\\','\0'};
	char		query[2048];
	int		qlen;

	#define	LARGE_BUFFER_LENGTH	8192
	char	largebuffer[LARGE_BUFFER_LENGTH+1];


	// hostname
	char	hostname[256];
	char	*dot;
	gethostname(hostname,sizeof(hostname));
	dot=strchr(hostname,'.');
	if (dot) {
		*dot='\0';
	}


	// instantiation
	con=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
			"testuser","testpassword",0,1);
	cur=sqlrcur_alloc(con);


	// identify
	printf("IDENTIFY: \n");
	assertEqStr(sqlrcon_identify(con),"mysql");
	printf("\n");


	// db version
	printf("DB VERSION: \n");
	dbversion=sqlrcon_dbVersion(con);
	majorversion=dbversion[0]-'0';
	printf("\n");


	// ping
	printf("PING: \n");
	assertTrue(sqlrcon_ping(con));
	printf("\n");


	// transaction state
	printf("TRANSACTION STATE: \n");
	assertEqStr(sqlrcon_getDefaultTransactionModel(con),"explicit-deferred");
	assertEqStr(sqlrcon_getTransactionModel(con),"explicit-deferred");
	assertFalse(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcon_getAutoCommit(con));
	printf("\n");


	// bind format
	printf("BIND FORMAT: \n");
	if (majorversion>3) {
		assertEqStr(sqlrcon_bindFormat(con),"?");
	} else {
		assertEqStr(sqlrcon_bindFormat(con),":*");
	}
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
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testtinyint tinyint, "
		"	testsmallint smallint, "
		"	testmediumint mediumint,"
		"	testint int, "
		"	testbigint bigint, "
		"	testfloat float, "
		"	testreal real, "
		"	testdecimal decimal(2,1),"
		"	testdate date, "
		"	testtime time, "
		"	testdatetime datetime, "
		"	testyear year, "
		"	testchar char(40), "
		"	testvarchar varchar(40),"
		"	testtext text, "
		"	testtinytext tinytext, "
		"	testmediumtext "
		"	mediumtext, "
		"	testlongtext longtext, "
		"	testblob blob, "
		"	testtinyblob tinyblob, "
		"	testmediumblob "
		"	mediumblob, "
		"	testlongblob longblob, "
		"	testtimestamp timestamp)"));
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
		"	1, "
		"	1, "
		"	1.5, "
		"	1.5, "
		"	1.5, "
		"	'2001-01-01', "
		"	'01:00:00', "
		"	'2001-01-01 01:00:00', "
		"	'2001', "
		"	'char1', "
		"	'varchar1', "
		"	'text1', "
		"	'tinytext1', "
		"	'mediumtext1', "
		"	'longtext1', "
		"	'blob1', "
		"	'tinyblob1', "
		"	'mediumblob1', "
		"	'longblob1', "
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
		"	2.5, "
		"	2.5, "
		"	2.5, "
		"	'2002-01-01', "
		"	'02:00:00', "
		"	'2002-01-01 02:00:00', "
		"	'2002', "
		"	'char2', "
		"	'varchar2', "
		"	'text2', "
		"	'tinytext2', "
		"	'mediumtext2', "
		"	'longtext2', "
		"	'blob2', "
		"	'tinyblob2', "
		"	'mediumblob2', "
		"	'longblob2', "
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
		"	3.5, "
		"	3.5, "
		"	3.5, "
		"	'2003-01-01', "
		"	'03:00:00', "
		"	'2003-01-01 03:00:00', "
		"	'2003', "
		"	'char3', "
		"	'varchar3', "
		"	'text3', "
		"	'tinytext3', "
		"	'mediumtext3', "
		"	'longtext3', "
		"	'blob3', "
		"	'tinyblob3', "
		"	'mediumblob3', "
		"	'longblob3', "
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
		"	4.5, "
		"	4.5, "
		"	4.5, "
		"	'2004-01-01', "
		"	'04:00:00', "
		"	'2004-01-01 04:00:00', "
		"	'2004', "
		"	'char4', "
		"	'varchar4', "
		"	'text4', "
		"	'tinytext4', "
		"	'mediumtext4', "
		"	'longtext4', "
		"	'blob4', "
		"	'tinyblob4', "
		"	'mediumblob4', "
		"	'longblob4', "
		"	NULL)"));
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
	assertEqInt(sqlrcur_countBindVariables(cur),22);
	sqlrcur_inputBindLong(cur,"1",5);
	sqlrcur_inputBindLong(cur,"2",5);
	sqlrcur_inputBindLong(cur,"3",5);
	sqlrcur_inputBindLong(cur,"4",5);
	sqlrcur_inputBindLong(cur,"5",5);
	sqlrcur_inputBindDouble(cur,"6",5.5,2,1);
	sqlrcur_inputBindDouble(cur,"7",5.5,2,1);
	sqlrcur_inputBindDouble(cur,"8",5.5,2,1);
	sqlrcur_inputBindString(cur,"9","2005-01-01");
	sqlrcur_inputBindString(cur,"10","05:00:00");
	sqlrcur_inputBindDate(cur,"11",2005,1,1,5,0,0,0,NULL,0);
	sqlrcur_inputBindString(cur,"12","2005");
	sqlrcur_inputBindString(cur,"13","char5");
	sqlrcur_inputBindString(cur,"14","varchar5");
	sqlrcur_inputBindClob(cur,"15","text5",5);
	sqlrcur_inputBindClob(cur,"16","tinytext5",9);
	sqlrcur_inputBindClob(cur,"17","mediumtext5",11);
	sqlrcur_inputBindClob(cur,"18","longtext5",9);
	sqlrcur_inputBindBlob(cur,"19","blob5",5);
	sqlrcur_inputBindBlob(cur,"20","tinyblob5",9);
	sqlrcur_inputBindBlob(cur,"21","mediumblob5",11);
	sqlrcur_inputBindBlob(cur,"22","longblob5",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",6);
	sqlrcur_inputBindLong(cur,"2",6);
	sqlrcur_inputBindLong(cur,"3",6);
	sqlrcur_inputBindLong(cur,"4",6);
	sqlrcur_inputBindLong(cur,"5",6);
	sqlrcur_inputBindDouble(cur,"6",6.5,2,1);
	sqlrcur_inputBindDouble(cur,"7",6.5,2,1);
	sqlrcur_inputBindDouble(cur,"8",6.5,2,1);
	sqlrcur_inputBindString(cur,"9","2006-01-01");
	sqlrcur_inputBindString(cur,"10","06:00:00");
	sqlrcur_inputBindDate(cur,"11",2006,1,1,6,0,0,0,NULL,0);
	sqlrcur_inputBindString(cur,"12","2006");
	sqlrcur_inputBindString(cur,"13","char6");
	sqlrcur_inputBindString(cur,"14","varchar6");
	sqlrcur_inputBindClob(cur,"15","text6",5);
	sqlrcur_inputBindClob(cur,"16","tinytext6",9);
	sqlrcur_inputBindClob(cur,"17","mediumtext6",11);
	sqlrcur_inputBindClob(cur,"18","longtext6",9);
	sqlrcur_inputBindBlob(cur,"19","blob6",5);
	sqlrcur_inputBindBlob(cur,"20","tinyblob6",9);
	sqlrcur_inputBindBlob(cur,"21","mediumblob6",11);
	sqlrcur_inputBindBlob(cur,"22","longblob6",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",7);
	sqlrcur_inputBindLong(cur,"2",7);
	sqlrcur_inputBindLong(cur,"3",7);
	sqlrcur_inputBindLong(cur,"4",7);
	sqlrcur_inputBindLong(cur,"5",7);
	sqlrcur_inputBindDouble(cur,"6",7.5,2,1);
	sqlrcur_inputBindDouble(cur,"7",7.5,2,1);
	sqlrcur_inputBindDouble(cur,"8",7.5,2,1);
	sqlrcur_inputBindString(cur,"9","2007-01-01");
	sqlrcur_inputBindString(cur,"10","07:00:00");
	sqlrcur_inputBindDate(cur,"11",2007,1,1,7,0,0,0,NULL,0);
	sqlrcur_inputBindString(cur,"12","2007");
	sqlrcur_inputBindString(cur,"13","char7");
	sqlrcur_inputBindString(cur,"14","varchar7");
	sqlrcur_inputBindClob(cur,"15","text7",5);
	sqlrcur_inputBindClob(cur,"16","tinytext7",9);
	sqlrcur_inputBindClob(cur,"17","mediumtext7",11);
	sqlrcur_inputBindClob(cur,"18","longtext7",9);
	sqlrcur_inputBindBlob(cur,"19","blob7",5);
	sqlrcur_inputBindBlob(cur,"20","tinyblob7",9);
	sqlrcur_inputBindBlob(cur,"21","mediumblob7",11);
	sqlrcur_inputBindBlob(cur,"22","longblob7",9);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// array of input binds by position
	// mysql doesn't support implicit
	// conversion of string binds to other
	// data types, so arrays of binds don't
	// generally work.


	// input bind by position with
	// validation
	printf("BIND BY POSITION WITH ""VALIDATION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",8);
	sqlrcur_inputBindLong(cur,"2",8);
	sqlrcur_inputBindLong(cur,"3",8);
	sqlrcur_inputBindLong(cur,"4",8);
	sqlrcur_inputBindLong(cur,"5",8);
	sqlrcur_inputBindDouble(cur,"6",8.5,2,1);
	sqlrcur_inputBindDouble(cur,"7",8.5,2,1);
	sqlrcur_inputBindDouble(cur,"8",8.5,2,1);
	sqlrcur_inputBindString(cur,"9","2008-01-01");
	sqlrcur_inputBindString(cur,"10","08:00:00");
	sqlrcur_inputBindDate(cur,"11",2008,1,1,8,0,0,0,NULL,0);
	sqlrcur_inputBindString(cur,"12","2008");
	sqlrcur_inputBindString(cur,"13","char8");
	sqlrcur_inputBindString(cur,"14","varchar8");
	sqlrcur_inputBindClob(cur,"15","text8",5);
	sqlrcur_inputBindClob(cur,"16","tinytext8",9);
	sqlrcur_inputBindClob(cur,"17","mediumtext8",11);
	sqlrcur_inputBindClob(cur,"18","longtext8",9);
	sqlrcur_inputBindBlob(cur,"19","blob8",5);
	sqlrcur_inputBindBlob(cur,"20","tinyblob8",9);
	sqlrcur_inputBindBlob(cur,"21","mediumblob8",11);
	sqlrcur_inputBindBlob(cur,"22","longblob8",9);
	sqlrcur_validateBinds(cur);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// input bind by name
	// mysql doesn't support bind by name


	// array of input binds by name
	// mysql doesn't support bind by name


	// input bind by name with validation
	// mysql doesn't support bind by name


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
	assertEqInt(sqlrcur_colCount(cur),23);
	printf("\n");


	// column names
	printf("COLUMN NAMES: \n");
	assertEqStr(sqlrcur_getColumnName(cur,0),"testtinyint");
	assertEqStr(sqlrcur_getColumnName(cur,1),"testsmallint");
	assertEqStr(sqlrcur_getColumnName(cur,2),"testmediumint");
	assertEqStr(sqlrcur_getColumnName(cur,3),"testint");
	assertEqStr(sqlrcur_getColumnName(cur,4),"testbigint");
	assertEqStr(sqlrcur_getColumnName(cur,5),"testfloat");
	assertEqStr(sqlrcur_getColumnName(cur,6),"testreal");
	assertEqStr(sqlrcur_getColumnName(cur,7),"testdecimal");
	assertEqStr(sqlrcur_getColumnName(cur,8),"testdate");
	assertEqStr(sqlrcur_getColumnName(cur,9),"testtime");
	assertEqStr(sqlrcur_getColumnName(cur,10),"testdatetime");
	assertEqStr(sqlrcur_getColumnName(cur,11),"testyear");
	assertEqStr(sqlrcur_getColumnName(cur,12),"testchar");
	assertEqStr(sqlrcur_getColumnName(cur,13),"testvarchar");
	assertEqStr(sqlrcur_getColumnName(cur,14),"testtext");
	assertEqStr(sqlrcur_getColumnName(cur,15),"testtinytext");
	assertEqStr(sqlrcur_getColumnName(cur,16),"testmediumtext");
	assertEqStr(sqlrcur_getColumnName(cur,17),"testlongtext");
	assertEqStr(sqlrcur_getColumnName(cur,18),"testblob");
	assertEqStr(sqlrcur_getColumnName(cur,19),"testtinyblob");
	assertEqStr(sqlrcur_getColumnName(cur,20),"testmediumblob");
	assertEqStr(sqlrcur_getColumnName(cur,21),"testlongblob");
	assertEqStr(sqlrcur_getColumnName(cur,22),"testtimestamp");
	cols=sqlrcur_getColumnNames(cur);
	assertEqStr(cols[0],"testtinyint");
	assertEqStr(cols[1],"testsmallint");
	assertEqStr(cols[2],"testmediumint");
	assertEqStr(cols[3],"testint");
	assertEqStr(cols[4],"testbigint");
	assertEqStr(cols[5],"testfloat");
	assertEqStr(cols[6],"testreal");
	assertEqStr(cols[7],"testdecimal");
	assertEqStr(cols[8],"testdate");
	assertEqStr(cols[9],"testtime");
	assertEqStr(cols[10],"testdatetime");
	assertEqStr(cols[11],"testyear");
	assertEqStr(cols[12],"testchar");
	assertEqStr(cols[13],"testvarchar");
	assertEqStr(cols[14],"testtext");
	assertEqStr(cols[15],"testtinytext");
	assertEqStr(cols[16],"testmediumtext");
	assertEqStr(cols[17],"testlongtext");
	assertEqStr(cols[18],"testblob");
	assertEqStr(cols[19],"testtinyblob");
	assertEqStr(cols[20],"testmediumblob");
	assertEqStr(cols[21],"testlongblob");
	assertEqStr(cols[22],"testtimestamp");
	printf("\n");


	// column types
	printf("COLUMN TYPES: \n");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),"TINYINT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,1),"SMALLINT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,2),"MEDIUMINT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,3),"INT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,4),"BIGINT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,5),"FLOAT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,6),"REAL");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,7),"DECIMAL");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,8),"DATE");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,9),"TIME");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,10),"DATETIME");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,11),"YEAR");
	if (majorversion==3) {
		assertEqStr(sqlrcur_getColumnTypeByIndex(cur,12),"VARSTRING");
	} else {
		assertEqStr(sqlrcur_getColumnTypeByIndex(cur,12),"STRING");
	}
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,13),"VARSTRING");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,14),"TEXT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,15),"TINYTEXT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,16),"MEDIUMTEXT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,17),"LONGTEXT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,18),"BLOB");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,19),"TINYBLOB");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,20),"MEDIUMBLOB");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,21),"LONGBLOB");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,22),"TIMESTAMP");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testtinyint"),"TINYINT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testsmallint"),"SMALLINT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testmediumint"),
		"MEDIUMINT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testint"),"INT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testbigint"),"BIGINT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testfloat"),"FLOAT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testreal"),"REAL");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testdecimal"),"DECIMAL");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testdate"),"DATE");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testtime"),"TIME");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testdatetime"),"DATETIME");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testyear"),"YEAR");
	if (majorversion==3) {
		assertEqStr(sqlrcur_getColumnTypeByName(cur,"testchar"),
		    "VARSTRING");
	} else {
		assertEqStr(sqlrcur_getColumnTypeByName(cur,"testchar"),
		    "STRING");
	}
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testvarchar"),"VARSTRING");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testtext"),"TEXT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testtinytext"),"TINYTEXT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testmediumtext"),
		"MEDIUMTEXT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testlongtext"),"LONGTEXT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testblob"),"BLOB");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testtinyblob"),"TINYBLOB");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testmediumblob"),
		"MEDIUMBLOB");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testlongblob"),"LONGBLOB");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testtimestamp"),
		"TIMESTAMP");
	printf("\n");


	// column length
	printf("COLUMN LENGTH: \n");
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),1);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,1),2);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,2),3);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,3),4);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,4),8);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,5),4);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,6),8);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,7),6);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,8),3);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,9),3);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,10),8);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,11),1);
	// testchar/testvarchar are char(40)/varchar(40); the connection
	// charset is utf8mb4 (4 bytes/char) so the reported lengths are 160/161
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,12),160);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,13),161);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,14),65535);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,15),255);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,16),16777215);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,17),2147483647);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,18),65535);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,19),255);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,20),16777215);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,21),2147483647);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,22),4);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testtinyint"),1);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testsmallint"),2);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testmediumint"),3);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testint"),4);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testbigint"),8);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testfloat"),4);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testreal"),8);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testdecimal"),6);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testdate"),3);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testtime"),3);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testdatetime"),8);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testyear"),1);
	// testchar/testvarchar are char(40)/varchar(40); the connection
	// charset is utf8mb4 (4 bytes/char) so the reported lengths are 160/161
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testchar"),160);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testvarchar"),161);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testtext"),65535);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testtinytext"),255);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testmediumtext"),
		16777215);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testlongtext"),
		2147483647);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testblob"),65535);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testtinyblob"),255);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testmediumblob"),
		16777215);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testlongblob"),
		2147483647);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testtimestamp"),4);
	printf("\n");


	// longest column
	printf("LONGEST COLUMN: \n");
	assertEqInt(sqlrcur_getLongestByIndex(cur,0),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,1),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,2),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,3),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,4),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,5),3);
	assertEqInt(sqlrcur_getLongestByIndex(cur,6),3);
	assertEqInt(sqlrcur_getLongestByIndex(cur,7),3);
	assertEqInt(sqlrcur_getLongestByIndex(cur,8),10);
	assertEqInt(sqlrcur_getLongestByIndex(cur,9),8);
	assertEqInt(sqlrcur_getLongestByIndex(cur,10),19);
	assertEqInt(sqlrcur_getLongestByIndex(cur,11),4);
	assertEqInt(sqlrcur_getLongestByIndex(cur,12),5);
	assertEqInt(sqlrcur_getLongestByIndex(cur,13),8);
	assertEqInt(sqlrcur_getLongestByIndex(cur,14),5);
	assertEqInt(sqlrcur_getLongestByIndex(cur,15),9);
	assertEqInt(sqlrcur_getLongestByIndex(cur,16),11);
	assertEqInt(sqlrcur_getLongestByIndex(cur,17),9);
	assertEqInt(sqlrcur_getLongestByIndex(cur,18),5);
	assertEqInt(sqlrcur_getLongestByIndex(cur,19),9);
	assertEqInt(sqlrcur_getLongestByIndex(cur,20),11);
	assertEqInt(sqlrcur_getLongestByIndex(cur,21),9);
	if (majorversion==3) {
		assertEqInt(sqlrcur_getLongestByIndex(cur,22),14);
	} else {
		assertEqInt(sqlrcur_getLongestByIndex(cur,22),19);
	}
	assertEqInt(sqlrcur_getLongestByName(cur,"testtinyint"),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"testsmallint"),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"testmediumint"),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"testint"),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"testbigint"),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"testfloat"),3);
	assertEqInt(sqlrcur_getLongestByName(cur,"testreal"),3);
	assertEqInt(sqlrcur_getLongestByName(cur,"testdecimal"),3);
	assertEqInt(sqlrcur_getLongestByName(cur,"testdate"),10);
	assertEqInt(sqlrcur_getLongestByName(cur,"testtime"),8);
	assertEqInt(sqlrcur_getLongestByName(cur,"testdatetime"),19);
	assertEqInt(sqlrcur_getLongestByName(cur,"testyear"),4);
	assertEqInt(sqlrcur_getLongestByName(cur,"testchar"),5);
	assertEqInt(sqlrcur_getLongestByName(cur,"testvarchar"),8);
	assertEqInt(sqlrcur_getLongestByName(cur,"testtext"),5);
	assertEqInt(sqlrcur_getLongestByName(cur,"testtinytext"),9);
	assertEqInt(sqlrcur_getLongestByName(cur,"testmediumtext"),11);
	assertEqInt(sqlrcur_getLongestByName(cur,"testlongtext"),9);
	assertEqInt(sqlrcur_getLongestByName(cur,"testblob"),5);
	assertEqInt(sqlrcur_getLongestByName(cur,"testtinyblob"),9);
	assertEqInt(sqlrcur_getLongestByName(cur,"testmediumblob"),11);
	assertEqInt(sqlrcur_getLongestByName(cur,"testlongblob"),9);
	if (majorversion==3) {
		assertEqInt(sqlrcur_getLongestByName(cur,"testtimestamp"),14);
	} else {
		assertEqInt(sqlrcur_getLongestByName(cur,"testtimestamp"),19);
	}
	printf("\n");


	// row count
	printf("ROW COUNT: \n");
	assertEqInt(sqlrcur_rowCount(cur),8);
	printf("\n");


	// total rows
	printf("TOTAL ROWS: \n");
	// older versions of mysql know this
	//assertEqInt(sqlrcur_totalRows(cur),0);
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
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,3),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,4),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,5),"1.5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,6),"1.5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,7),"1.5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,8),"2001-01-01");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,9),"01:00:00");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,10),"2001-01-01 01:00:00");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,11),"2001");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,12),"char1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,13),"varchar1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,14),"text1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,15),"tinytext1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,16),"mediumtext1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,17),"longtext1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,18),"blob1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,19),"tinyblob1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,20),"mediumblob1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,21),"longblob1");
	printf("\n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,2),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,3),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,4),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,5),"8.5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,6),"8.5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,7),"8.5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,8),"2008-01-01");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,9),"08:00:00");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,10),"2008-01-01 08:00:00");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,11),"2008");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,12),"char8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,13),"varchar8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,14),"text8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,15),"tinytext8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,16),"mediumtext8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,17),"longtext8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,18),"blob8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,19),"tinyblob8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,20),"mediumblob8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,21),"longblob8");
	printf("\n");


	// field lengths by index
	printf("FIELD LENGTHS BY INDEX: \n");
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,1),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,2),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,3),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,4),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,5),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,6),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,7),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,8),10);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,9),8);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,10),19);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,11),4);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,12),5);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,13),8);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,14),5);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,15),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,16),11);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,17),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,18),5);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,19),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,20),11);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,21),9);
	printf("\n");
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,0),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,1),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,2),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,3),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,4),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,5),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,6),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,7),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,8),10);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,9),8);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,10),19);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,11),4);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,12),5);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,13),8);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,14),5);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,15),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,16),11);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,17),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,18),5);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,19),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,20),11);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,21),9);
	printf("\n");


	// fields by name
	printf("FIELDS BY NAME: \n");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testtinyint"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testsmallint"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testmediumint"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testint"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testbigint"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testfloat"),"1.5");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testreal"),"1.5");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testdecimal"),"1.5");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testdate"),"2001-01-01");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testtime"),"01:00:00");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testdatetime"),
		"2001-01-01 01:00:00");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testyear"),"2001");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testchar"),"char1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testvarchar"),"varchar1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testtext"),"text1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testtinytext"),"tinytext1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testmediumtext"),
		"mediumtext1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testlongtext"),"longtext1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testblob"),"blob1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testlongblob"),"longblob1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testtinyblob"),"tinyblob1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testmediumblob"),
		"mediumblob1");
	printf("\n");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testtinyint"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testsmallint"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testmediumint"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testint"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testbigint"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testfloat"),"8.5");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testreal"),"8.5");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testdecimal"),"8.5");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testdate"),"2008-01-01");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testtime"),"08:00:00");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testdatetime"),
		"2008-01-01 08:00:00");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testyear"),"2008");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testchar"),"char8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testvarchar"),"varchar8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testtext"),"text8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testtinytext"),"tinytext8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testmediumtext"),
		"mediumtext8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testlongtext"),"longtext8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testblob"),"blob8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testlongblob"),"longblob8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testtinyblob"),"tinyblob8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testmediumblob"),
		"mediumblob8");
	printf("\n");


	// field lengths by name
	printf("FIELD LENGTHS BY NAME: \n");
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testtinyint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testsmallint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testmediumint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testbigint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testreal"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testdecimal"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testdate"),10);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testtime"),8);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testdatetime"),19);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testyear"),4);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testchar"),5);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testvarchar"),8);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testtext"),5);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testtinytext"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testmediumtext"),11);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testlongtext"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testblob"),5);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testtinyblob"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testmediumblob"),11);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testlongblob"),9);
	printf("\n");
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testtinyint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testsmallint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testmediumint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testbigint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testreal"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testdecimal"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testdate"),10);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testtime"),8);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testdatetime"),19);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testyear"),4);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testchar"),5);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testvarchar"),8);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testtext"),5);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testtinytext"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testmediumtext"),11);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testlongtext"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testblob"),5);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testtinyblob"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testmediumblob"),11);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testlongblob"),9);
	printf("\n");


	// fields by array
	printf("FIELDS BY ARRAY: \n");
	fields=sqlrcur_getRow(cur,0);
	assertEqStr(fields[0],"1");
	assertEqStr(fields[1],"1");
	assertEqStr(fields[2],"1");
	assertEqStr(fields[3],"1");
	assertEqStr(fields[4],"1");
	assertEqStr(fields[5],"1.5");
	assertEqStr(fields[6],"1.5");
	assertEqStr(fields[7],"1.5");
	assertEqStr(fields[8],"2001-01-01");
	assertEqStr(fields[9],"01:00:00");
	assertEqStr(fields[10],"2001-01-01 01:00:00");
	assertEqStr(fields[11],"2001");
	assertEqStr(fields[12],"char1");
	assertEqStr(fields[13],"varchar1");
	assertEqStr(fields[14],"text1");
	assertEqStr(fields[15],"tinytext1");
	assertEqStr(fields[16],"mediumtext1");
	assertEqStr(fields[17],"longtext1");
	assertEqStr(fields[18],"blob1");
	assertEqStr(fields[19],"tinyblob1");
	assertEqStr(fields[20],"mediumblob1");
	assertEqStr(fields[21],"longblob1");
	printf("\n");


	// field lengths by array
	printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=sqlrcur_getRowLengths(cur,0);
	assertEqInt(fieldlens[0],1);
	assertEqInt(fieldlens[1],1);
	assertEqInt(fieldlens[2],1);
	assertEqInt(fieldlens[3],1);
	assertEqInt(fieldlens[4],1);
	assertEqInt(fieldlens[5],3);
	assertEqInt(fieldlens[6],3);
	assertEqInt(fieldlens[7],3);
	assertEqInt(fieldlens[8],10);
	assertEqInt(fieldlens[9],8);
	assertEqInt(fieldlens[10],19);
	assertEqInt(fieldlens[11],4);
	assertEqInt(fieldlens[12],5);
	assertEqInt(fieldlens[13],8);
	assertEqInt(fieldlens[14],5);
	assertEqInt(fieldlens[15],9);
	assertEqInt(fieldlens[16],11);
	assertEqInt(fieldlens[17],9);
	assertEqInt(fieldlens[18],5);
	assertEqInt(fieldlens[19],9);
	assertEqInt(fieldlens[20],11);
	assertEqInt(fieldlens[21],9);
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
		"	testtinyint "));
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
		"	testtinyint "));
	assertEqStr(sqlrcur_getColumnName(cur,0),NULL);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),0);
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),NULL);
	printf("\n");
	sqlrcur_getColumnInfo(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	assertEqStr(sqlrcur_getColumnName(cur,0),"testtinyint");
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),1);
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),"TINYINT");
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
		"	testtinyint "));
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
		"	testtinyint "));
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
		"	testtinyint "));
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
		"	testtinyint "));
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqStr(filename,"cachefile1");
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	free(filename);
	printf("\n");


	// column count for cached result set
	printf("COLUMN COUNT FOR ""CACHED RESULT SET: \n");
	assertEqInt(sqlrcur_colCount(cur),23);
	printf("\n");


	// column names for cached result set
	printf("COLUMN NAMES FOR ""CACHED RESULT SET: \n");
	assertEqStr(sqlrcur_getColumnName(cur,0),"testtinyint");
	assertEqStr(sqlrcur_getColumnName(cur,1),"testsmallint");
	assertEqStr(sqlrcur_getColumnName(cur,2),"testmediumint");
	assertEqStr(sqlrcur_getColumnName(cur,3),"testint");
	assertEqStr(sqlrcur_getColumnName(cur,4),"testbigint");
	assertEqStr(sqlrcur_getColumnName(cur,5),"testfloat");
	assertEqStr(sqlrcur_getColumnName(cur,6),"testreal");
	assertEqStr(sqlrcur_getColumnName(cur,7),"testdecimal");
	assertEqStr(sqlrcur_getColumnName(cur,8),"testdate");
	assertEqStr(sqlrcur_getColumnName(cur,9),"testtime");
	assertEqStr(sqlrcur_getColumnName(cur,10),"testdatetime");
	assertEqStr(sqlrcur_getColumnName(cur,11),"testyear");
	assertEqStr(sqlrcur_getColumnName(cur,12),"testchar");
	assertEqStr(sqlrcur_getColumnName(cur,13),"testvarchar");
	assertEqStr(sqlrcur_getColumnName(cur,14),"testtext");
	assertEqStr(sqlrcur_getColumnName(cur,15),"testtinytext");
	assertEqStr(sqlrcur_getColumnName(cur,16),"testmediumtext");
	assertEqStr(sqlrcur_getColumnName(cur,17),"testlongtext");
	assertEqStr(sqlrcur_getColumnName(cur,18),"testblob");
	assertEqStr(sqlrcur_getColumnName(cur,19),"testtinyblob");
	assertEqStr(sqlrcur_getColumnName(cur,20),"testmediumblob");
	assertEqStr(sqlrcur_getColumnName(cur,21),"testlongblob");
	cols=sqlrcur_getColumnNames(cur);
	assertEqStr(cols[0],"testtinyint");
	assertEqStr(cols[1],"testsmallint");
	assertEqStr(cols[2],"testmediumint");
	assertEqStr(cols[3],"testint");
	assertEqStr(cols[4],"testbigint");
	assertEqStr(cols[5],"testfloat");
	assertEqStr(cols[6],"testreal");
	assertEqStr(cols[7],"testdecimal");
	assertEqStr(cols[8],"testdate");
	assertEqStr(cols[9],"testtime");
	assertEqStr(cols[10],"testdatetime");
	assertEqStr(cols[11],"testyear");
	assertEqStr(cols[12],"testchar");
	assertEqStr(cols[13],"testvarchar");
	assertEqStr(cols[14],"testtext");
	assertEqStr(cols[15],"testtinytext");
	assertEqStr(cols[16],"testmediumtext");
	assertEqStr(cols[17],"testlongtext");
	assertEqStr(cols[18],"testblob");
	assertEqStr(cols[19],"testtinyblob");
	assertEqStr(cols[20],"testmediumblob");
	assertEqStr(cols[21],"testlongblob");
	printf("\n");


	// cached result set with result set
	// buffer size
	printf("CACHED RESULT SET WITH ""RESULT SET ""BUFFER SIZE: \n");
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


	// from one cache file to another with
	// result set buffer size
	printf("FROM ONE CACHE FILE ""TO ANOTHER WITH RESULT "
		"SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile1"));
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile2"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize(cur,0);
	printf("\n");


	// cached result set with suspend and
	// result set buffer size
	printf("CACHED RESULT SET WITH ""SUSPEND AND RESULT SET "
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
		"	testtinyint "));
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
	printf("FINISHED SUSPENDED ""SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable "
		"order by testint"));
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
	// can't do this with mysql
	//sqlrcur_setResultSetBufferSize(cur,1);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable"));
	secondcur=sqlrcur_alloc(con);
	sqlrcur_setResultSetBufferSize(secondcur,1);
	for (i=0;sqlrcur_getRow(cur,i);i++) {
		assertTrue(sqlrcur_sendQuery(secondcur,"select * from "
				"testtable"));
	}
	sqlrcur_free(secondcur);
	//sqlrcur_setResultSetBufferSize(cur,0);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// reset transaction state
	printf("RESET TRANSACTION STATE: \n");
	assertTrue(sqlrcon_commit(con));
	assertEqStr(sqlrcon_getTransactionModel(con),"explicit-deferred");
	assertTrue(sqlrcon_getAutoCommit(con));
	printf("\n");


	// transaction behavior - implicit
	printf("TRANSACTION BEHAVIOR - implicit: \n");
	assertTrue(sqlrcon_setTransactionModel(con,"implicit"));
	assertEqStr(sqlrcon_getTransactionModel(con),"implicit");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable (col1 integer)"));
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
	assertEqStr(sqlrcon_getTransactionModel(con),"explicit-deferred");
	assertTrue(sqlrcon_getAutoCommit(con));
	printf("\n");


	// individual substitutions
	printf("INDIVIDUAL ""SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"select $(var1),""'$(var2)',$(var3)");
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
	sqlrcur_prepareQuery(cur,"select $(var1),""$(var2),$(var3)");
	sqlrcur_subLongs(cur,subvars,subvallongs);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"2");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"3");
	printf("\n");
	sqlrcur_prepareQuery(cur,"select '$(var1)',""'$(var2)','$(var3)'");
	sqlrcur_subStrings(cur,subvars,subvalstrings);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"hi");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"bye");
	printf("\n");
	sqlrcur_prepareQuery(cur,"select $(var1),""$(var2),$(var3)");
	sqlrcur_subDoubles(cur,subvars,subvaldoubles,precs,scales);
	assertTrue(sqlrcur_executeQuery(cur));
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
	printf("\n");


	// null and empty lobs
	printf("NULL AND EMPTY LOBS: \n");
	sqlrcur_getNullsAsNulls(cur);
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testclob1 longtext, "
		"	testclob2 longtext, "
		"	testblob1 longblob, "
		"	testblob2 longblob)"));
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
	printf("\n");


	// long lobs
	printf("LONG LOBS: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testtext longtext, "
		"	testblob longblob)"));
	sqlrcur_prepareQuery(cur,"insert into testtable ""values (?,?)");
	for (i=0;i<LARGE_BUFFER_LENGTH;i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	sqlrcur_inputBindClob(cur,"1",largebuffer,LARGE_BUFFER_LENGTH);
	sqlrcur_inputBindBlob(cur,"2",largebuffer,LARGE_BUFFER_LENGTH);
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable"));
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testtext"),
		LARGE_BUFFER_LENGTH);
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testtext"),largebuffer);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testblob"),
		LARGE_BUFFER_LENGTH);
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testblob"),largebuffer);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// output bind by position
	// mysql doesn't support output binds

	// output bind by name
	// mysql doesn't support bind by name


	// output bind by name with validation
	// mysql doesn't support bind by name


	// lob output bind
	// mysql doesn't support output binds


	// long output bind
	// mysql doesn't support output binds


	// negative input bind
	printf("NEGATIVE INPUT BIND: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable ""(testval int)"));
	sqlrcur_prepareQuery(cur,"insert into testtable ""values (?)");
	sqlrcur_inputBindLong(cur,"1",-1);
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"select testval ""from testtable"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testval"),"-1");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// bind validation
	// mysql doesn't support bind by name


	// rebinding
	printf("REBINDING: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc("
		"	in in1 int) "
		"begin "
		"	select in1; ""end"));
	sqlrcur_prepareQuery(cur,"call testproc(?)");
	sqlrcur_inputBindLong(cur,"1",1);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	sqlrcur_inputBindLong(cur,"1",2);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"2");
	sqlrcur_inputBindLong(cur,"1",3);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"3");
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
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
	sqlrcur_prepareQuery(cur,"select ?");
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
		"	in in3 varchar(20)) ""begin ""end"));
	sqlrcur_prepareQuery(cur,"call testproc(?,?,?)");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",1.5,2,1);
	sqlrcur_inputBindString(cur,"3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	printf("\n");


	// stored procedure returning single
	// value
	printf("STORED PROCEDURE ""RETURNING SINGLE ""VALUE: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc("
		"	in in1 int, "
		"	in in2 double, "
		"	in in3 varchar(20)) "
		"begin "
		"	select in1; ""end"));
	sqlrcur_prepareQuery(cur,"call testproc(?,?,?)");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",1.5,2,1);
	sqlrcur_inputBindString(cur,"3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	printf("\n");


	// stored procedure returning multiple
	// values
	printf("STORED PROCEDURE ""RETURNING MULTIPLE ""VALUES: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc("
		"	in in1 int, "
		"	in in2 double, "
		"	in in3 varchar(20)) "
		"begin "
		"	select in1, in2, "
		"	in3; ""end"));
	sqlrcur_prepareQuery(cur,"call testproc(?,?,?)");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",1.5,2,1);
	sqlrcur_inputBindString(cur,"3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"1.5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"hello");
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	printf("\n");


	// stored procedure returning result
	// set
	printf("STORED PROCEDURE ""RETURNING RESULT SET: \n");
	sqlrcur_sendQuery(cur,"drop procedure ""testselectproc");
	assertTrue(sqlrcur_sendQuery(cur,"create procedure ""testselectproc() "
		"begin "
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
		"	select 8; ""end"));
	assertTrue(sqlrcur_sendQuery(cur,"call testselectproc()"));
	assertEqInt(sqlrcur_rowCount(cur),8);
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure ""testselectproc"));
	printf("\n");


	// temporary tables
	printf("TEMPORARY TABLES: \n");
	sqlrcur_sendQuery(cur,"drop table temptable");
	assertTrue(sqlrcur_sendQuery(cur,"create temporary table ""temptable (col1 int)"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into temptable ""values (1)"));
	assertTrue(sqlrcur_sendQuery(cur,"select count(*) ""from temptable"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	sqlrcon_endSession(con);
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,"select count(*) ""from temptable"));
	printf("\n");

	if (majorversion>3) {

		// stored procedure returning
		// no value
		printf("STORED PROCEDURE ""RETURNING ""NO VALUE: \n");
		sqlrcur_sendQuery(cur,"drop procedure ""if exists testproc");
		assertTrue(sqlrcur_sendQuery(cur,"create procedure "
			"testproc("
			"	in in1 int, "
			"	in in2 float, "
			"	in in3 "
			"	char(20)) "
			"begin "
			"	select in1, "
			"	in2, in3; ""end;"));
		sqlrcur_prepareQuery(cur,"call testproc(""?,?,?)");
		sqlrcur_inputBindLong(cur,"1",1);
		sqlrcur_inputBindDouble(cur,"2",1.5,4,2);
		sqlrcur_inputBindString(cur,"3","hello");
		assertTrue(sqlrcur_executeQuery(cur));
		assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
		assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"1.5");
		assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"hello");
		sqlrcur_sendQuery(cur,"drop procedure ""testproc");
		printf("\n");


		// stored procedure returning
		// one value
		printf("FUNCTIONS: \n");
		sqlrcur_sendQuery(cur,"drop function ""if exists testfunc");
		assertTrue(sqlrcur_sendQuery(cur,"create function ""testfunc("
			"in1 int, in2 "
			"	int) ""returns int ""return ""in1+in2;"));
		sqlrcur_prepareQuery(cur,"select ""testfunc(?,?)");
		sqlrcur_inputBindLong(cur,"1",10);
		sqlrcur_inputBindLong(cur,"2",20);
		assertTrue(sqlrcur_executeQuery(cur));
		assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"30");
		sqlrcur_sendQuery(cur,"drop function ""if exists ""testfunc");
		printf("\n");


		// stored procedure returning
		// multiple values
		printf("STORED PROCEDURE ""RETURNING MULTIPLE ""VALUES: \n");
		sqlrcur_sendQuery(cur,"drop procedure ""if exists testproc");
		assertTrue(sqlrcur_sendQuery(cur,"create procedure "
			"testproc("
			"	out out1 int, "
			"	out out2 float,"
			"	out out3 "
			"	char(20)) "
			"begin "
			"	select 1, 2.5,"
			"	'hello' "
			"	into out1, "
			"	out2, out3; ""end;"));
		assertTrue(sqlrcur_sendQuery(cur,"set @out1=0, ""@out2=0.0, "
			"@out3=''"));
		assertTrue(sqlrcur_sendQuery(cur,"call testproc(""@out1,@out2,"
			"@out3)"));
		assertTrue(sqlrcur_sendQuery(cur,"select @out1, "
			"@out2, @out3"));
		assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
		assertEqDbl(sqlrcur_getFieldAsDoubleByIndex(cur,0,1),2.5);
		assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"hello");
		sqlrcur_sendQuery(cur,"drop procedure ""testproc");
		printf("\n");


		// stored procedure returning
		// result set
		printf("STORED PROCEDURE ""RETURNING RESULT ""SET: \n");
		sqlrcur_sendQuery(cur,"drop procedure ""if exists "
			"testselectproc");
		assertTrue(sqlrcur_sendQuery(cur,"create procedure "
			"testselectproc() "
			"begin "
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
			"	select 8; ""end"));
		assertTrue(sqlrcur_sendQuery(cur,"call ""testselectproc()"));
		assertEqInt(sqlrcur_rowCount(cur),8);
		sqlrcur_sendQuery(cur,"drop procedure ""testselectproc");
		printf("\n");
	}


	if (majorversion>3) {

		// encoded binary data -
		// all chars - \-escaped
		printf("ENCODED BINARY DATA"" - all chars - ""\\-escaped: \n");
		sqlrcur_sendQuery(cur,"drop table ""testtable");
		assertTrue(sqlrcur_sendQuery(cur,"create table ""testtable "
			"(col1 longblob)"));
		for (i=0; i<256; i++) {
			buffer[i]=i;
		}
		strcpy(query,"insert into ""testtable values ""(_binary'");
		qlen=strlen(query);
		for (i=0;i<sizeof(buffer);i++) {
			if (buffer[i]=='\'') {
				query[qlen++]='\\';
			}
			if (buffer[i]=='\\') {
				query[qlen++]='\\';
			}
			query[qlen++]=buffer[i];
		}
		query[qlen++]='\'';
		query[qlen++]=')';
		query[qlen]='\0';
		assertTrue(sqlrcur_sendQueryWithLength(cur,query,qlen));
		assertTrue(sqlrcur_sendQuery(cur,"select col1 "
			"from testtable"));
		assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),
		    sizeof(buffer));
		assertEqInt(memcmp(sqlrcur_getFieldByIndex(cur,0,0),buffer,
		    sizeof(buffer)),0);
		assertTrue(sqlrcur_sendQuery(cur,"drop table ""testtable"));
		printf("\n");


		// encoded binary data -
		// (null)"" - unescaped
		printf("ENCODED BINARY DATA"" - (null)\"\" - ""unescaped: \n");
		sqlrcur_sendQuery(cur,"drop table ""testtable");
		assertTrue(sqlrcur_sendQuery(cur,"create table ""testtable "
			"(col1 longblob)"));
		assertTrue(sqlrcur_sendQueryWithLength(cur,"insert into "
			"testtable ""values ""(_binary'""\0\"\"')",43));
		assertTrue(sqlrcur_sendQuery(cur,"select col1 "
			"from testtable"));
		assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),3);
		assertEqInt(memcmp(sqlrcur_getFieldByIndex(cur,0,0),
		    "\0\"\"",3),0);
		assertTrue(sqlrcur_sendQuery(cur,"drop table ""testtable"));
		printf("\n");


		// encoded binary data -
		// (null)"" - \-escaped
		printf("ENCODED BINARY DATA"" - \\(null)""\\\"\\\" - "
			"\\-escaped: \n");
		sqlrcur_sendQuery(cur,"drop table ""testtable");
		assertTrue(sqlrcur_sendQuery(cur,"create table ""testtable "
			"(col1 longblob)"));
		assertTrue(sqlrcur_sendQueryWithLength(cur,"insert into "
			"testtable ""values ""(_binary'""\\\0\\\"\\\"""')",46));
		assertTrue(sqlrcur_sendQuery(cur,"select col1 "
			"from testtable"));
		assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),3);
		assertEqInt(memcmp(sqlrcur_getFieldByIndex(cur,0,0),
		    "\0\"\"",3),0);
		assertTrue(sqlrcur_sendQuery(cur,"drop table ""testtable"));
		printf("\n");
	}


	// quotes - '' - ''-escaped
	printf("QUOTES - '' - ""''-escaped: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable "
		"(col1 varchar(4))"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable "
		"values ('''''')"));
	assertTrue(sqlrcur_sendQuery(cur,"select col1 ""from testtable"));
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),2);
	assertEqInt(strcmp(sqlrcur_getFieldByIndex(cur,0,0),"''"),0);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// quotes - '' - '',\-escaped
	printf("QUOTES - '' - ""'',\\-escaped: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable "
		"(col1 varchar(4))"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable "
		"values ('''\\'')"));
	assertTrue(sqlrcur_sendQuery(cur,"select col1 ""from testtable"));
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),2);
	assertEqInt(strcmp(sqlrcur_getFieldByIndex(cur,0,0),"''"),0);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// quotes - '' - \,''-escaped
	printf("QUOTES - '' - ""\\,''-escaped: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable "
		"(col1 varchar(4))"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable "
		"values ('\\'''')"));
	assertTrue(sqlrcur_sendQuery(cur,"select col1 ""from testtable"));
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),2);
	assertEqInt(strcmp(sqlrcur_getFieldByIndex(cur,0,0),"''"),0);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// quotes - \\' - \-escaped
	printf("QUOTES - \\\\' - ""\\-escaped: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable "
		"(col1 varchar(4))"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable "
		"values ('\\\\\\'')"));
	assertTrue(sqlrcur_sendQuery(cur,"select col1 ""from testtable"));
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),2);
	assertEqInt(memcmp(sqlrcur_getFieldByIndex(cur,0,0),"\\\'",2),0);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// quotes - "" - unescaped
	printf("QUOTES - \"\" - ""unescaped: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable "
		"(col1 varchar(4))"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable "
		"values ('\"\"')"));
	assertTrue(sqlrcur_sendQuery(cur,"select col1 ""from testtable"));
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),2);
	assertEqInt(strcmp(sqlrcur_getFieldByIndex(cur,0,0),"\"\""),0);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// quotes - random - '',\-escaped
	printf("QUOTES - random - ""'',\\-escaped: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable "
		"(col1 varchar(512))"));
	srand(time(NULL));
	seed1=rand();
	seed2=rand();
	srand(seed1);
	for (i=0; i<sizeof(buffer); i++) {
		result1=rand();
		srand(result1);
		buffer[i]=ch[result1%4];
	}
	strcpy(query,"insert into testtable ""values ('");
	qlen=strlen(query);
	srand(seed2);
	for (i=0; i<sizeof(buffer); i++) {
		result2=rand();
		srand(result2);
		if (buffer[i]=='\'') {
			// randomly escape
			// with \ or ''
			if (result2%2) {
				query[qlen++]='\'';
			} else {
				query[qlen++]='\\';
			}
		}
		if (buffer[i]=='"') {
			// randomly escape
			// with \ or don't
			if (result2%2) {
				query[qlen++]='\\';
			}
		}
		if (buffer[i]=='\\') {
			// escape with
			// backslash
			query[qlen++]='\\';
		}
		query[qlen++]=buffer[i];
	}
	query[qlen++]='\'';
	query[qlen++]=')';
	query[qlen]='\0';
	assertTrue(sqlrcur_sendQueryWithLength(cur,query,qlen));
	assertTrue(sqlrcur_sendQuery(cur,"select col1 ""from testtable"));
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),sizeof(buffer));
	assertEqInt(memcmp(sqlrcur_getFieldByIndex(cur,0,0),
		buffer,sizeof(buffer)),0);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// last insert id
	printf("LAST INSERT ID: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable "
		"	(col1 int primary key"
		"	auto_increment, "
		"	col2 int)"));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable "
		"values (null,1)"));
	assertEqInt(sqlrcon_getLastInsertId(con),1);
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
	assertInResultSet(cur,"Database","def");
	printf("\n");


	// schema list
	printf("SCHEMA LIST: \n");
	assertTrue(sqlrcur_getSchemaList(cur,NULL));
	assertEqStr(sqlrcur_getColumnName(cur,0),"Database");
	assertInResultSet(cur,"Database",hostname);
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
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable1"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable2"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable3"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable4"));
	printf("\n");


	// type info list
	printf("TYPE INFO LIST: \n");
	assertTrue(sqlrcur_getTypeInfoList(cur,"int"));
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
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"INT");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"4");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"10");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"INT");
	assertTrue(sqlrcur_getTypeInfoList(cur,"char"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"CHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"255");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"CHAR");
	assertTrue(sqlrcur_getTypeInfoList(cur,"varchar"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"VARCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"12");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"65535");
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
		"	testtinyint tinyint, "
		"	testsmallint smallint,"
		"	testmediumint "
		"	mediumint, "
		"	testint int, "
		"	testbigint bigint, "
		"	testfloat float, "
		"	testreal real, "
		"	testdecimal "
		"	decimal(2,1), "
		"	testdate date, "
		"	testtime time, "
		"	testdatetime "
		"	datetime, "
		"	testyear year, "
		"	testchar char(40), "
		"	testvarchar "
		"	varchar(40), "
		"	testtext text, "
		"	testtinytext "
		"	tinytext, "
		"	testmediumtext "
		"	mediumtext, "
		"	testlongtext "
		"	longtext, "
		"	testblob blob, "
		"	testtinyblob "
		"	tinyblob, "
		"	testmediumblob "
		"	mediumblob, "
		"	testlongblob "
		"	longblob, "
		"	testtimestamp "
		"	timestamp)"));
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
	assertEqStr(sqlrcur_getFieldByName(cur,0,"column_name"),"testtinyint");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"column_name"),"testsmallint");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"column_name"),
		"testmediumint");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"column_name"),"testint");
	assertEqStr(sqlrcur_getFieldByName(cur,4,"column_name"),"testbigint");
	assertEqStr(sqlrcur_getFieldByName(cur,5,"column_name"),"testfloat");
	assertEqStr(sqlrcur_getFieldByName(cur,6,"column_name"),"testreal");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"column_name"),"testdecimal");
	assertEqStr(sqlrcur_getFieldByName(cur,8,"column_name"),"testdate");
	assertEqStr(sqlrcur_getFieldByName(cur,9,"column_name"),"testtime");
	assertEqStr(sqlrcur_getFieldByName(cur,10,"column_name"),
		"testdatetime");
	assertEqStr(sqlrcur_getFieldByName(cur,11,"column_name"),"testyear");
	assertEqStr(sqlrcur_getFieldByName(cur,12,"column_name"),"testchar");
	assertEqStr(sqlrcur_getFieldByName(cur,13,"column_name"),"testvarchar");
	assertEqStr(sqlrcur_getFieldByName(cur,14,"column_name"),"testtext");
	assertEqStr(sqlrcur_getFieldByName(cur,15,"column_name"),
		"testtinytext");
	assertEqStr(sqlrcur_getFieldByName(cur,16,"column_name"),
		"testmediumtext");
	assertEqStr(sqlrcur_getFieldByName(cur,17,"column_name"),
		"testlongtext");
	assertEqStr(sqlrcur_getFieldByName(cur,18,"column_name"),"testblob");
	assertEqStr(sqlrcur_getFieldByName(cur,19,"column_name"),
		"testtinyblob");
	assertEqStr(sqlrcur_getFieldByName(cur,20,"column_name"),
		"testmediumblob");
	assertEqStr(sqlrcur_getFieldByName(cur,21,"column_name"),
		"testlongblob");
	assertEqStr(sqlrcur_getFieldByName(cur,22,"column_name"),
		"testtimestamp");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"TINYINT");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"data_type"),"SMALLINT");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"data_type"),"MEDIUMINT");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"data_type"),"INT");
	assertEqStr(sqlrcur_getFieldByName(cur,4,"data_type"),"BIGINT");
	assertEqStr(sqlrcur_getFieldByName(cur,5,"data_type"),"FLOAT");
	// not "REAL"
	assertEqStr(sqlrcur_getFieldByName(cur,6,"data_type"),"DOUBLE");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"data_type"),"DECIMAL");
	assertEqStr(sqlrcur_getFieldByName(cur,8,"data_type"),"DATE");
	assertEqStr(sqlrcur_getFieldByName(cur,9,"data_type"),"TIME");
	assertEqStr(sqlrcur_getFieldByName(cur,10,"data_type"),"DATETIME");
	assertEqStr(sqlrcur_getFieldByName(cur,11,"data_type"),"YEAR");
	assertEqStr(sqlrcur_getFieldByName(cur,12,"data_type"),"CHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,13,"data_type"),"VARCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,14,"data_type"),"TEXT");
	assertEqStr(sqlrcur_getFieldByName(cur,15,"data_type"),"TINYTEXT");
	assertEqStr(sqlrcur_getFieldByName(cur,16,"data_type"),"MEDIUMTEXT");
	assertEqStr(sqlrcur_getFieldByName(cur,17,"data_type"),"LONGTEXT");
	assertEqStr(sqlrcur_getFieldByName(cur,18,"data_type"),"BLOB");
	assertEqStr(sqlrcur_getFieldByName(cur,19,"data_type"),"TINYBLOB");
	assertEqStr(sqlrcur_getFieldByName(cur,20,"data_type"),"MEDIUMBLOB");
	assertEqStr(sqlrcur_getFieldByName(cur,21,"data_type"),"LONGBLOB");
	assertEqStr(sqlrcur_getFieldByName(cur,22,"data_type"),"TIMESTAMP");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// column list - auto_increment,
	// primary key
	printf("COLUMN LIST - ""auto_increment, ""primary key: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 int "
		"	auto_increment "
		"	primary key, "
		"	col2 int)"));
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
		"	col1 int "
		"	primary key, "
		"	col2 int)"));
	assertTrue(sqlrcur_getColumnList(cur,"testtable",NULL));
	assertFalse(strstr(sqlrcur_getFieldByName(cur,0,"extra"),
		"auto_increment")!=NULL);
	assertTrue(strstr(sqlrcur_getFieldByName(cur,0,"column_key"),
		"PRI")!=NULL);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// primary keys list
	printf("PRIMARY KEYS LIST: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 int "
		"	primary key, "
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
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,0,"table"),"testtable"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"seq_in_index"),"1");
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,0,"column_name"),"col1"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"key_name"),"PRIMARY");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// key and index list
	printf("KEY AND INDEX LIST: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 int "
		"	primary key, "
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
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,0,"table"),"testtable"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"non_unique"),"false");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"seq_in_index"),"1");
	assertTrue(!strcmp(sqlrcur_getFieldByName(cur,0,"column_name"),"col1"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"collation"),"A");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"index_type"),"3");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"key_name"),"PRIMARY");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// procedure list
	printf("PROCEDURE LIST: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc1");
	sqlrcur_sendQuery(cur,"drop procedure testproc2");
	sqlrcur_sendQuery(cur,"drop procedure testproc3");
	sqlrcur_sendQuery(cur,"drop procedure testproc4");
	assertTrue(sqlrcur_sendQuery(cur,"create procedure "
		"testproc1("
		"	in in1 int, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) ""begin end"));
	assertTrue(sqlrcur_sendQuery(cur,"create procedure "
		"testproc2("
		"	in in1 int, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) ""begin end"));
	assertTrue(sqlrcur_sendQuery(cur,"create procedure "
		"testproc3("
		"	in in1 int, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) ""begin end"));
	assertTrue(sqlrcur_sendQuery(cur,"create procedure "
		"testproc4("
		"	in in1 int, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) ""begin end"));
	assertTrue(sqlrcur_getProcedureList(cur,NULL));
	assertInResultSet(cur,"routine_name","testproc1");
	assertInResultSet(cur,"routine_name","testproc2");
	assertInResultSet(cur,"routine_name","testproc3");
	assertInResultSet(cur,"routine_name","testproc4");
	printf("\n");


	// procedure parameter list
	printf("PROCEDURE PARAMETER ""LIST: \n");
	assertTrue(sqlrcur_getProcedureParameterList(cur,"testproc1",NULL));
	assertEqStr(sqlrcur_getColumnName(cur,0),"parameter_name");
	assertEqStr(sqlrcur_getColumnName(cur,1),"parameter_mode");
	assertEqStr(sqlrcur_getColumnName(cur,2),"data_type");
	assertEqStr(sqlrcur_getColumnName(cur,3),"character_maximum_length");
	assertEqStr(sqlrcur_getColumnName(cur,4),"ordinal_position");
	assertEqInt(sqlrcur_rowCount(cur),4);
	assertEqStr(sqlrcur_getFieldByName(cur,0,"parameter_name"),"in1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"INT");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"ordinal_position"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"parameter_name"),"in2");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"data_type"),"CHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"ordinal_position"),"2");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"parameter_name"),"in3");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"data_type"),"VARCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"ordinal_position"),"3");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"parameter_name"),"in4");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"data_type"),"DATE");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"ordinal_position"),"4");
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc1"));
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc2"));
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc3"));
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc4"));
	printf("\n");


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

	sqlrcur_free(cur);
	sqlrcon_free(con);

	reportTestStatus();

	return status;
}
