// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclient.h>
#include <rudiments/process.h>
#include <rudiments/bytestring.h>
#include <rudiments/randomnumber.h>
#include <rudiments/stdio.h>

#include "../../config.h"

//#define PROFILING 1

#ifdef PROFILING
	class noio {
		public:
			void printf(const char * str, ...) {};
			void flush() {};
	};
	static noio nooutput;
	#define stdoutput nooutput
#endif

#include "asserts.cpp"

sqlrconnection	*con;
sqlrcursor	*cur;
sqlrconnection	*secondcon;
sqlrcursor	*secondcur;

int main(int argc, char **argv) {

#ifdef PROFILING
for (uint16_t a=0; a<50; a++) {
#endif

	const char	*isolationlevels[]={
				"REPEATABLE-READ","READ-UNCOMMITTED",
				"READ-COMMITTED","SERIALIZABLE",NULL};
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
	uint64_t	counter=0;

	// instantiation
	con=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);


	// identify
	stdoutput.printf("IDENTIFY: \n");
	assertEquals(con->identify(),"mysql");
	stdoutput.printf("\n");

        // get the db version
        const char      *dbversion=con->dbVersion();
        uint32_t        majorversion=dbversion[0]-'0';


	// ping
	stdoutput.printf("PING: \n");
	assertTrue(con->ping());
	stdoutput.printf("\n");


	// bind format
	stdoutput.printf("BIND FORMAT: \n");
#ifdef HAVE_MYSQL_STMT_PREPARE
	assertEquals(con->bindFormat(),"?");
#else
	assertEquals(con->bindFormat(),":*");
#endif
	stdoutput.printf("\n");


	// nextval format
	stdoutput.printf("NEXTVAL FORMAT: \n");
	assertEquals(con->nextvalFormat(),"");
	stdoutput.printf("\n");


	// isolation levels
	stdoutput.printf("ISOLATION LEVELS: \n");
	for (const char **il=isolationlevels; *il; il++) {
		assertTrue(con->setIsolationLevel(*il));
		assertEquals(con->getIsolationLevel(),*il);
		stdoutput.printf("\n");
	}
	// reset to the default isolation level
	assertTrue(con->setIsolationLevel(isolationlevels[0]));
	stdoutput.printf("\n");

	// drop existing table
	cur->sendQuery("drop table testtable");


	// create temptable
	stdoutput.printf("CREATE TEMPTABLE: \n");
	assertTrue(cur->sendQuery(
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
		"	testvarchar varchar(40), "
		"	testtext text, "
		"	testtinytext tinytext, "
		"	testmediumtext mediumtext, "
		"	testlongtext longtext, "
		"	testblob blob, "
		"	testtinyblob tinyblob, "
		"	testmediumblob mediumblob, "
		"	testlongblob longblob, "
		"	testtimestamp timestamp)"));
	stdoutput.printf("\n");


	// begin transaction
	stdoutput.printf("BEGIN TRANSACTION: \n");
	assertTrue(cur->sendQuery("begin"));
	stdoutput.printf("\n");


	// insert
	stdoutput.printf("INSERT: \n");
	assertTrue(cur->sendQuery(
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
	assertTrue(cur->sendQuery(
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
	assertTrue(cur->sendQuery(
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
	assertTrue(cur->sendQuery(
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
	stdoutput.printf("\n");


	// affected rows
	stdoutput.printf("AFFECTED ROWS: \n");
	assertEquals(cur->affectedRows(),1);
	stdoutput.printf("\n");


	// bind by position
	stdoutput.printf("BIND BY POSITION: \n");
	cur->prepareQuery(
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
	assertEquals(cur->countBindVariables(),22);
	cur->inputBind("1",5);
	cur->inputBind("2",5);
	cur->inputBind("3",5);
	cur->inputBind("4",5);
	cur->inputBind("5",5);
	cur->inputBind("6",5.1,2,1);
	cur->inputBind("7",5.1,2,1);
	cur->inputBind("8",5.1,2,1);
	cur->inputBind("9","2005-01-01");
	cur->inputBind("10","05:00:00");
	cur->inputBind("11",2005,1,1,5,0,0,0,NULL,false);
	cur->inputBind("12","2005");
	cur->inputBind("13","char5");
	cur->inputBind("14","varchar5");
	cur->inputBindClob("15","text5",5);
	cur->inputBindClob("16","tinytext5",9);
	cur->inputBindClob("17","mediumtext5",11);
	cur->inputBindClob("18","longtext5",9);
	cur->inputBindBlob("19","blob5",5);
	cur->inputBindBlob("20","tinyblob5",9);
	cur->inputBindBlob("21","mediumblob5",11);
	cur->inputBindBlob("22","longblob5",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",6);
	cur->inputBind("2",6);
	cur->inputBind("3",6);
	cur->inputBind("4",6);
	cur->inputBind("5",6);
	cur->inputBind("6",6.1,2,1);
	cur->inputBind("7",6.1,2,1);
	cur->inputBind("8",6.1,2,1);
	cur->inputBind("9","2006-01-01");
	cur->inputBind("10","06:00:00");
	cur->inputBind("11",2006,1,1,6,0,0,0,NULL,false);
	cur->inputBind("12","2006");
	cur->inputBind("13","char6");
	cur->inputBind("14","varchar6");
	cur->inputBindClob("15","text6",5);
	cur->inputBindClob("16","tinytext6",9);
	cur->inputBindClob("17","mediumtext6",11);
	cur->inputBindClob("18","longtext6",9);
	cur->inputBindBlob("19","blob6",5);
	cur->inputBindBlob("20","tinyblob6",9);
	cur->inputBindBlob("21","mediumblob6",11);
	cur->inputBindBlob("22","longblob6",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",7);
	cur->inputBind("2",7);
	cur->inputBind("3",7);
	cur->inputBind("4",7);
	cur->inputBind("5",7);
	cur->inputBind("6",7.1,2,1);
	cur->inputBind("7",7.1,2,1);
	cur->inputBind("8",7.1,2,1);
	cur->inputBind("9","2007-01-01");
	cur->inputBind("10","07:00:00");
	cur->inputBind("11",2007,1,1,7,0,0,0,NULL,false);
	cur->inputBind("12","2007");
	cur->inputBind("13","char7");
	cur->inputBind("14","varchar7");
	cur->inputBindClob("15","text7",5);
	cur->inputBindClob("16","tinytext7",9);
	cur->inputBindClob("17","mediumtext7",11);
	cur->inputBindClob("18","longtext7",9);
	cur->inputBindBlob("19","blob7",5);
	cur->inputBindBlob("20","tinyblob7",9);
	cur->inputBindBlob("21","mediumblob7",11);
	cur->inputBindBlob("22","longblob7",9);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// bind by position with validation
	stdoutput.printf("BIND BY POSITION WITH VALIDATION: \n");
	cur->clearBinds();
	cur->inputBind("1",8);
	cur->inputBind("2",8);
	cur->inputBind("3",8);
	cur->inputBind("4",8);
	cur->inputBind("5",8);
	cur->inputBind("6",8.1,2,1);
	cur->inputBind("7",8.1,2,1);
	cur->inputBind("8",8.1,2,1);
	cur->inputBind("9","2008-01-01");
	cur->inputBind("10","08:00:00");
	cur->inputBind("11",2008,1,1,8,0,0,0,NULL,false);
	cur->inputBind("12","2008");
	cur->inputBind("13","char8");
	cur->inputBind("14","varchar8");
	cur->inputBindClob("15","text8",5);
	cur->inputBindClob("16","tinytext8",9);
	cur->inputBindClob("17","mediumtext8",11);
	cur->inputBindClob("18","longtext8",9);
	cur->inputBindBlob("19","blob8",5);
	cur->inputBindBlob("20","tinyblob8",9);
	cur->inputBindBlob("21","mediumblob8",11);
	cur->inputBindBlob("22","longblob8",9);
	cur->validateBinds();
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// select
	stdoutput.printf("SELECT: \n");
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	stdoutput.printf("\n");


	// column count
	stdoutput.printf("COLUMN COUNT: \n");
	assertEquals(cur->colCount(),23);
	stdoutput.printf("\n");


	// column names
	stdoutput.printf("COLUMN NAMES: \n");
	assertEquals(cur->getColumnName(0),"testtinyint");
	assertEquals(cur->getColumnName(1),"testsmallint");
	assertEquals(cur->getColumnName(2),"testmediumint");
	assertEquals(cur->getColumnName(3),"testint");
	assertEquals(cur->getColumnName(4),"testbigint");
	assertEquals(cur->getColumnName(5),"testfloat");
	assertEquals(cur->getColumnName(6),"testreal");
	assertEquals(cur->getColumnName(7),"testdecimal");
	assertEquals(cur->getColumnName(8),"testdate");
	assertEquals(cur->getColumnName(9),"testtime");
	assertEquals(cur->getColumnName(10),"testdatetime");
	assertEquals(cur->getColumnName(11),"testyear");
	assertEquals(cur->getColumnName(12),"testchar");
	assertEquals(cur->getColumnName(13),"testvarchar");
	assertEquals(cur->getColumnName(14),"testtext");
	assertEquals(cur->getColumnName(15),"testtinytext");
	assertEquals(cur->getColumnName(16),"testmediumtext");
	assertEquals(cur->getColumnName(17),"testlongtext");
	assertEquals(cur->getColumnName(18),"testblob");
	assertEquals(cur->getColumnName(19),"testtinyblob");
	assertEquals(cur->getColumnName(20),"testmediumblob");
	assertEquals(cur->getColumnName(21),"testlongblob");
	assertEquals(cur->getColumnName(22),"testtimestamp");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"testtinyint");
	assertEquals(cols[1],"testsmallint");
	assertEquals(cols[2],"testmediumint");
	assertEquals(cols[3],"testint");
	assertEquals(cols[4],"testbigint");
	assertEquals(cols[5],"testfloat");
	assertEquals(cols[6],"testreal");
	assertEquals(cols[7],"testdecimal");
	assertEquals(cols[8],"testdate");
	assertEquals(cols[9],"testtime");
	assertEquals(cols[10],"testdatetime");
	assertEquals(cols[11],"testyear");
	assertEquals(cols[12],"testchar");
	assertEquals(cols[13],"testvarchar");
	assertEquals(cols[14],"testtext");
	assertEquals(cols[15],"testtinytext");
	assertEquals(cols[16],"testmediumtext");
	assertEquals(cols[17],"testlongtext");
	assertEquals(cols[18],"testblob");
	assertEquals(cols[19],"testtinyblob");
	assertEquals(cols[20],"testmediumblob");
	assertEquals(cols[21],"testlongblob");
	assertEquals(cols[22],"testtimestamp");
	stdoutput.printf("\n");


	// column types
	stdoutput.printf("COLUMN TYPES: \n");
	assertEquals(cur->getColumnType((uint32_t)0),"TINYINT");
	assertEquals(cur->getColumnType(1),"SMALLINT");
	assertEquals(cur->getColumnType(2),"MEDIUMINT");
	assertEquals(cur->getColumnType(3),"INT");
	assertEquals(cur->getColumnType(4),"BIGINT");
	assertEquals(cur->getColumnType(5),"FLOAT");
	assertEquals(cur->getColumnType(6),"REAL");
	assertEquals(cur->getColumnType(7),"DECIMAL");
	assertEquals(cur->getColumnType(8),"DATE");
	assertEquals(cur->getColumnType(9),"TIME");
	assertEquals(cur->getColumnType(10),"DATETIME");
	assertEquals(cur->getColumnType(11),"YEAR");
	if (majorversion==3) {
		assertEquals(cur->getColumnType(12),"VARSTRING");
	} else {
		assertEquals(cur->getColumnType(12),"STRING");
	}
	assertEquals(cur->getColumnType(13),"VARSTRING");
	assertEquals(cur->getColumnType(14),"BLOB");
	assertEquals(cur->getColumnType(15),"TINYBLOB");
	assertEquals(cur->getColumnType(16),"MEDIUMBLOB");
	assertEquals(cur->getColumnType(17),"LONGBLOB");
	assertEquals(cur->getColumnType(18),"BLOB");
	assertEquals(cur->getColumnType(19),"TINYBLOB");
	assertEquals(cur->getColumnType(20),"MEDIUMBLOB");
	assertEquals(cur->getColumnType(21),"LONGBLOB");
	assertEquals(cur->getColumnType(22),"TIMESTAMP");
	assertEquals(cur->getColumnType("testtinyint"),"TINYINT");
	assertEquals(cur->getColumnType("testsmallint"),"SMALLINT");
	assertEquals(cur->getColumnType("testmediumint"),"MEDIUMINT");
	assertEquals(cur->getColumnType("testint"),"INT");
	assertEquals(cur->getColumnType("testbigint"),"BIGINT");
	assertEquals(cur->getColumnType("testfloat"),"FLOAT");
	assertEquals(cur->getColumnType("testreal"),"REAL");
	assertEquals(cur->getColumnType("testdecimal"),"DECIMAL");
	assertEquals(cur->getColumnType("testdate"),"DATE");
	assertEquals(cur->getColumnType("testtime"),"TIME");
	assertEquals(cur->getColumnType("testdatetime"),"DATETIME");
	assertEquals(cur->getColumnType("testyear"),"YEAR");
	if (majorversion==3) {
		assertEquals(cur->getColumnType("testchar"),"VARSTRING");
	} else {
		assertEquals(cur->getColumnType("testchar"),"STRING");
	}
	assertEquals(cur->getColumnType("testvarchar"),"VARSTRING");
	assertEquals(cur->getColumnType("testtext"),"BLOB");
	assertEquals(cur->getColumnType("testtinytext"),"TINYBLOB");
	assertEquals(cur->getColumnType("testmediumtext"),"MEDIUMBLOB");
	assertEquals(cur->getColumnType("testlongtext"),"LONGBLOB");
	assertEquals(cur->getColumnType("testblob"),"BLOB");
	assertEquals(cur->getColumnType("testtinyblob"),"TINYBLOB");
	assertEquals(cur->getColumnType("testmediumblob"),"MEDIUMBLOB");
	assertEquals(cur->getColumnType("testlongblob"),"LONGBLOB");
	assertEquals(cur->getColumnType("testtimestamp"),"TIMESTAMP");
	stdoutput.printf("\n");


	// column length
	stdoutput.printf("COLUMN LENGTH: \n");
	assertEquals(cur->getColumnLength((uint32_t)0),1);
	assertEquals(cur->getColumnLength(1),2);
	assertEquals(cur->getColumnLength(2),3);
	assertEquals(cur->getColumnLength(3),4);
	assertEquals(cur->getColumnLength(4),8);
	assertEquals(cur->getColumnLength(5),4);
	assertEquals(cur->getColumnLength(6),8);
	assertEquals(cur->getColumnLength(7),6);
	assertEquals(cur->getColumnLength(8),3);
	assertEquals(cur->getColumnLength(9),3);
	assertEquals(cur->getColumnLength(10),8);
	assertEquals(cur->getColumnLength(11),1);
	// these can be 120/121 if the db charset is utf8
	//assertEquals(cur->getColumnLength(12),40);
	//assertEquals(cur->getColumnLength(13),41);
	assertEquals(cur->getColumnLength(14),65535);
	assertEquals(cur->getColumnLength(15),255);
	assertEquals(cur->getColumnLength(16),16777215);
	assertEquals(cur->getColumnLength(17),2147483647);
	assertEquals(cur->getColumnLength(18),65535);
	assertEquals(cur->getColumnLength(19),255);
	assertEquals(cur->getColumnLength(20),16777215);
	assertEquals(cur->getColumnLength(21),2147483647);
	assertEquals(cur->getColumnLength(22),4);
	assertEquals(cur->getColumnLength("testtinyint"),1);
	assertEquals(cur->getColumnLength("testsmallint"),2);
	assertEquals(cur->getColumnLength("testmediumint"),3);
	assertEquals(cur->getColumnLength("testint"),4);
	assertEquals(cur->getColumnLength("testbigint"),8);
	assertEquals(cur->getColumnLength("testfloat"),4);
	assertEquals(cur->getColumnLength("testreal"),8);
	assertEquals(cur->getColumnLength("testdecimal"),6);
	assertEquals(cur->getColumnLength("testdate"),3);
	assertEquals(cur->getColumnLength("testtime"),3);
	assertEquals(cur->getColumnLength("testdatetime"),8);
	assertEquals(cur->getColumnLength("testyear"),1);
	// these can be 120/121 if the db charset is utf8
	//assertEquals(cur->getColumnLength("testchar"),40);
	//assertEquals(cur->getColumnLength("testvarchar"),41);
	assertEquals(cur->getColumnLength("testtext"),65535);
	assertEquals(cur->getColumnLength("testtinytext"),255);
	assertEquals(cur->getColumnLength("testmediumtext"),16777215);
	assertEquals(cur->getColumnLength("testlongtext"),2147483647);
	assertEquals(cur->getColumnLength("testblob"),65535);
	assertEquals(cur->getColumnLength("testtinyblob"),255);
	assertEquals(cur->getColumnLength("testmediumblob"),16777215);
	assertEquals(cur->getColumnLength("testlongblob"),2147483647);
	assertEquals(cur->getColumnLength("testtimestamp"),4);
	stdoutput.printf("\n");


	// longest column
	stdoutput.printf("LONGEST COLUMN: \n");
	assertEquals(cur->getLongest((uint32_t)0),1);
	assertEquals(cur->getLongest(1),1);
	assertEquals(cur->getLongest(2),1);
	assertEquals(cur->getLongest(3),1);
	assertEquals(cur->getLongest(4),1);
	//assertEquals(cur->getLongest(5),3);
	assertEquals(cur->getLongest(6),3);
	assertEquals(cur->getLongest(7),3);
	assertEquals(cur->getLongest(8),10);
	assertEquals(cur->getLongest(9),8);
	assertEquals(cur->getLongest(10),19);
	assertEquals(cur->getLongest(11),4);
	assertEquals(cur->getLongest(12),5);
	assertEquals(cur->getLongest(13),8);
	assertEquals(cur->getLongest(14),5);
	assertEquals(cur->getLongest(15),9);
	assertEquals(cur->getLongest(16),11);
	assertEquals(cur->getLongest(17),9);
	assertEquals(cur->getLongest(18),5);
	assertEquals(cur->getLongest(19),9);
	assertEquals(cur->getLongest(20),11);
	assertEquals(cur->getLongest(21),9);
	if (majorversion==3) {
		assertEquals(cur->getLongest(22),14);
	} else {
		assertEquals(cur->getLongest(22),19);
	}
	assertEquals(cur->getLongest("testtinyint"),1);
	assertEquals(cur->getLongest("testsmallint"),1);
	assertEquals(cur->getLongest("testmediumint"),1);
	assertEquals(cur->getLongest("testint"),1);
	assertEquals(cur->getLongest("testbigint"),1);
	//assertEquals(cur->getLongest("testfloat"),3);
	assertEquals(cur->getLongest("testreal"),3);
	assertEquals(cur->getLongest("testdecimal"),3);
	assertEquals(cur->getLongest("testdate"),10);
	assertEquals(cur->getLongest("testtime"),8);
	assertEquals(cur->getLongest("testdatetime"),19);
	assertEquals(cur->getLongest("testyear"),4);
	assertEquals(cur->getLongest("testchar"),5);
	assertEquals(cur->getLongest("testvarchar"),8);
	assertEquals(cur->getLongest("testtext"),5);
	assertEquals(cur->getLongest("testtinytext"),9);
	assertEquals(cur->getLongest("testmediumtext"),11);
	assertEquals(cur->getLongest("testlongtext"),9);
	assertEquals(cur->getLongest("testblob"),5);
	assertEquals(cur->getLongest("testtinyblob"),9);
	assertEquals(cur->getLongest("testmediumblob"),11);
	assertEquals(cur->getLongest("testlongblob"),9);
	if (majorversion==3) {
		assertEquals(cur->getLongest("testtimestamp"),14);
	} else {
		assertEquals(cur->getLongest("testtimestamp"),19);
	}
	stdoutput.printf("\n");


	// row count
	stdoutput.printf("ROW COUNT: \n");
	assertEquals(cur->rowCount(),8);
	stdoutput.printf("\n");


	// total rows
	stdoutput.printf("TOTAL ROWS: \n");
	// older versions of mysql know this
	//assertEquals(cur->totalRows(),0);
	stdoutput.printf("\n");


	// first row index
	stdoutput.printf("FIRST ROW INDEX: \n");
	assertEquals(cur->firstRowIndex(),0);
	stdoutput.printf("\n");


	// end of result set
	stdoutput.printf("END OF RESULT SET: \n");
	assertTrue(cur->endOfResultSet());
	stdoutput.printf("\n");


	// fields by index
	stdoutput.printf("FIELDS BY INDEX: \n");
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"1");
	assertEquals(cur->getField(0,2),"1");
	assertEquals(cur->getField(0,3),"1");
	assertEquals(cur->getField(0,4),"1");
	//assertEquals(cur->getField(0,5),"1.1");
	assertEquals(cur->getField(0,6),"1.1");
	assertEquals(cur->getField(0,7),"1.1");
	assertEquals(cur->getField(0,8),"2001-01-01");
	assertEquals(cur->getField(0,9),"01:00:00");
	assertEquals(cur->getField(0,10),"2001-01-01 01:00:00");
	assertEquals(cur->getField(0,11),"2001");
	assertEquals(cur->getField(0,12),"char1");
	assertEquals(cur->getField(0,13),"varchar1");
	assertEquals(cur->getField(0,14),"text1");
	assertEquals(cur->getField(0,15),"tinytext1");
	assertEquals(cur->getField(0,16),"mediumtext1");
	assertEquals(cur->getField(0,17),"longtext1");
	assertEquals(cur->getField(0,18),"blob1");
	assertEquals(cur->getField(0,19),"tinyblob1");
	assertEquals(cur->getField(0,20),"mediumblob1");
	assertEquals(cur->getField(0,21),"longblob1");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(7,1),"8");
	assertEquals(cur->getField(7,2),"8");
	assertEquals(cur->getField(7,3),"8");
	assertEquals(cur->getField(7,4),"8");
	//assertEquals(cur->getField(7,5),"8.1");
	assertEquals(cur->getField(7,6),"8.1");
	assertEquals(cur->getField(7,7),"8.1");
	assertEquals(cur->getField(7,8),"2008-01-01");
	assertEquals(cur->getField(7,9),"08:00:00");
	assertEquals(cur->getField(7,10),"2008-01-01 08:00:00");
	assertEquals(cur->getField(7,11),"2008");
	assertEquals(cur->getField(7,12),"char8");
	assertEquals(cur->getField(7,13),"varchar8");
	assertEquals(cur->getField(7,14),"text8");
	assertEquals(cur->getField(7,15),"tinytext8");
	assertEquals(cur->getField(7,16),"mediumtext8");
	assertEquals(cur->getField(7,17),"longtext8");
	assertEquals(cur->getField(7,18),"blob8");
	assertEquals(cur->getField(7,19),"tinyblob8");
	assertEquals(cur->getField(7,20),"mediumblob8");
	assertEquals(cur->getField(7,21),"longblob8");
	stdoutput.printf("\n");


	// field lengths by index
	stdoutput.printf("FIELD LENGTHS BY INDEX: \n");
	assertEquals(cur->getFieldLength(0,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(0,1),1);
	assertEquals(cur->getFieldLength(0,2),1);
	assertEquals(cur->getFieldLength(0,3),1);
	assertEquals(cur->getFieldLength(0,4),1);
	//assertEquals(cur->getFieldLength(0,5),3);
	assertEquals(cur->getFieldLength(0,6),3);
	assertEquals(cur->getFieldLength(0,7),3);
	assertEquals(cur->getFieldLength(0,8),10);
	assertEquals(cur->getFieldLength(0,9),8);
	assertEquals(cur->getFieldLength(0,10),19);
	assertEquals(cur->getFieldLength(0,11),4);
	assertEquals(cur->getFieldLength(0,12),5);
	assertEquals(cur->getFieldLength(0,13),8);
	assertEquals(cur->getFieldLength(0,14),5);
	assertEquals(cur->getFieldLength(0,15),9);
	assertEquals(cur->getFieldLength(0,16),11);
	assertEquals(cur->getFieldLength(0,17),9);
	assertEquals(cur->getFieldLength(0,18),5);
	assertEquals(cur->getFieldLength(0,19),9);
	assertEquals(cur->getFieldLength(0,20),11);
	assertEquals(cur->getFieldLength(0,21),9);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(7,1),1);
	assertEquals(cur->getFieldLength(7,2),1);
	assertEquals(cur->getFieldLength(7,3),1);
	assertEquals(cur->getFieldLength(7,4),1);
	//assertEquals(cur->getFieldLength(7,5),3);
	assertEquals(cur->getFieldLength(7,6),3);
	assertEquals(cur->getFieldLength(7,7),3);
	assertEquals(cur->getFieldLength(7,8),10);
	assertEquals(cur->getFieldLength(7,9),8);
	assertEquals(cur->getFieldLength(7,10),19);
	assertEquals(cur->getFieldLength(7,11),4);
	assertEquals(cur->getFieldLength(7,12),5);
	assertEquals(cur->getFieldLength(7,13),8);
	assertEquals(cur->getFieldLength(7,14),5);
	assertEquals(cur->getFieldLength(7,15),9);
	assertEquals(cur->getFieldLength(7,16),11);
	assertEquals(cur->getFieldLength(7,17),9);
	assertEquals(cur->getFieldLength(7,18),5);
	assertEquals(cur->getFieldLength(7,19),9);
	assertEquals(cur->getFieldLength(7,20),11);
	assertEquals(cur->getFieldLength(7,21),9);
	stdoutput.printf("\n");


	// fields by name
	stdoutput.printf("FIELDS BY NAME: \n");
	assertEquals(cur->getField(0,"testtinyint"),"1");
	assertEquals(cur->getField(0,"testsmallint"),"1");
	assertEquals(cur->getField(0,"testmediumint"),"1");
	assertEquals(cur->getField(0,"testint"),"1");
	assertEquals(cur->getField(0,"testbigint"),"1");
	//assertEquals(cur->getField(0,"testfloat"),"1.1");
	assertEquals(cur->getField(0,"testreal"),"1.1");
	assertEquals(cur->getField(0,"testdecimal"),"1.1");
	assertEquals(cur->getField(0,"testdate"),"2001-01-01");
	assertEquals(cur->getField(0,"testtime"),"01:00:00");
	assertEquals(cur->getField(0,"testdatetime"),"2001-01-01 01:00:00");
	assertEquals(cur->getField(0,"testyear"),"2001");
	assertEquals(cur->getField(0,"testchar"),"char1");
	assertEquals(cur->getField(0,"testvarchar"),"varchar1");
	assertEquals(cur->getField(0,"testtext"),"text1");
	assertEquals(cur->getField(0,"testtinytext"),"tinytext1");
	assertEquals(cur->getField(0,"testmediumtext"),"mediumtext1");
	assertEquals(cur->getField(0,"testlongtext"),"longtext1");
	assertEquals(cur->getField(0,"testblob"),"blob1");
	assertEquals(cur->getField(0,"testlongblob"),"longblob1");
	assertEquals(cur->getField(0,"testtinyblob"),"tinyblob1");
	assertEquals(cur->getField(0,"testmediumblob"),"mediumblob1");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,"testtinyint"),"8");
	assertEquals(cur->getField(7,"testsmallint"),"8");
	assertEquals(cur->getField(7,"testmediumint"),"8");
	assertEquals(cur->getField(7,"testint"),"8");
	assertEquals(cur->getField(7,"testbigint"),"8");
	//assertEquals(cur->getField(7,"testfloat"),"8.1");
	assertEquals(cur->getField(7,"testreal"),"8.1");
	assertEquals(cur->getField(7,"testdecimal"),"8.1");
	assertEquals(cur->getField(7,"testdate"),"2008-01-01");
	assertEquals(cur->getField(7,"testtime"),"08:00:00");
	assertEquals(cur->getField(7,"testdatetime"),"2008-01-01 08:00:00");
	assertEquals(cur->getField(7,"testyear"),"2008");
	assertEquals(cur->getField(7,"testchar"),"char8");
	assertEquals(cur->getField(7,"testvarchar"),"varchar8");
	assertEquals(cur->getField(7,"testtext"),"text8");
	assertEquals(cur->getField(7,"testtinytext"),"tinytext8");
	assertEquals(cur->getField(7,"testmediumtext"),"mediumtext8");
	assertEquals(cur->getField(7,"testlongtext"),"longtext8");
	assertEquals(cur->getField(7,"testblob"),"blob8");
	assertEquals(cur->getField(7,"testlongblob"),"longblob8");
	assertEquals(cur->getField(7,"testtinyblob"),"tinyblob8");
	assertEquals(cur->getField(7,"testmediumblob"),"mediumblob8");
	stdoutput.printf("\n");


	// field lengths by name
	stdoutput.printf("FIELD LENGTHS BY NAME: \n");
	assertEquals(cur->getFieldLength(0,"testtinyint"),1);
	assertEquals(cur->getFieldLength(0,"testsmallint"),1);
	assertEquals(cur->getFieldLength(0,"testmediumint"),1);
	assertEquals(cur->getFieldLength(0,"testint"),1);
	assertEquals(cur->getFieldLength(0,"testbigint"),1);
	//assertEquals(cur->getFieldLength(0,"testfloat"),3);
	assertEquals(cur->getFieldLength(0,"testreal"),3);
	assertEquals(cur->getFieldLength(0,"testdecimal"),3);
	assertEquals(cur->getFieldLength(0,"testdate"),10);
	assertEquals(cur->getFieldLength(0,"testtime"),8);
	assertEquals(cur->getFieldLength(0,"testdatetime"),19);
	assertEquals(cur->getFieldLength(0,"testyear"),4);
	assertEquals(cur->getFieldLength(0,"testchar"),5);
	assertEquals(cur->getFieldLength(0,"testvarchar"),8);
	assertEquals(cur->getFieldLength(0,"testtext"),5);
	assertEquals(cur->getFieldLength(0,"testtinytext"),9);
	assertEquals(cur->getFieldLength(0,"testmediumtext"),11);
	assertEquals(cur->getFieldLength(0,"testlongtext"),9);
	assertEquals(cur->getFieldLength(0,"testblob"),5);
	assertEquals(cur->getFieldLength(0,"testtinyblob"),9);
	assertEquals(cur->getFieldLength(0,"testmediumblob"),11);
	assertEquals(cur->getFieldLength(0,"testlongblob"),9);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,"testtinyint"),1);
	assertEquals(cur->getFieldLength(7,"testsmallint"),1);
	assertEquals(cur->getFieldLength(7,"testmediumint"),1);
	assertEquals(cur->getFieldLength(7,"testint"),1);
	assertEquals(cur->getFieldLength(7,"testbigint"),1);
	//assertEquals(cur->getFieldLength(7,"testfloat"),3);
	assertEquals(cur->getFieldLength(7,"testreal"),3);
	assertEquals(cur->getFieldLength(7,"testdecimal"),3);
	assertEquals(cur->getFieldLength(7,"testdate"),10);
	assertEquals(cur->getFieldLength(7,"testtime"),8);
	assertEquals(cur->getFieldLength(7,"testdatetime"),19);
	assertEquals(cur->getFieldLength(7,"testyear"),4);
	assertEquals(cur->getFieldLength(7,"testchar"),5);
	assertEquals(cur->getFieldLength(7,"testvarchar"),8);
	assertEquals(cur->getFieldLength(7,"testtext"),5);
	assertEquals(cur->getFieldLength(7,"testtinytext"),9);
	assertEquals(cur->getFieldLength(7,"testmediumtext"),11);
	assertEquals(cur->getFieldLength(7,"testlongtext"),9);
	assertEquals(cur->getFieldLength(7,"testblob"),5);
	assertEquals(cur->getFieldLength(7,"testtinyblob"),9);
	assertEquals(cur->getFieldLength(7,"testmediumblob"),11);
	assertEquals(cur->getFieldLength(7,"testlongblob"),9);
	stdoutput.printf("\n");


	// fields by array
	stdoutput.printf("FIELDS BY ARRAY: \n");
	fields=cur->getRow(0);
	assertEquals(fields[0],"1");
	assertEquals(fields[1],"1");
	assertEquals(fields[2],"1");
	assertEquals(fields[3],"1");
	assertEquals(fields[4],"1");
	//assertEquals(fields[5],"1.1");
	assertEquals(fields[6],"1.1");
	assertEquals(fields[7],"1.1");
	assertEquals(fields[8],"2001-01-01");
	assertEquals(fields[9],"01:00:00");
	assertEquals(fields[10],"2001-01-01 01:00:00");
	assertEquals(fields[11],"2001");
	assertEquals(fields[12],"char1");
	assertEquals(fields[13],"varchar1");
	assertEquals(fields[14],"text1");
	assertEquals(fields[15],"tinytext1");
	assertEquals(fields[16],"mediumtext1");
	assertEquals(fields[17],"longtext1");
	assertEquals(fields[18],"blob1");
	assertEquals(fields[19],"tinyblob1");
	assertEquals(fields[20],"mediumblob1");
	assertEquals(fields[21],"longblob1");
	stdoutput.printf("\n");


	// field lengths by array
	stdoutput.printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=cur->getRowLengths(0);
	assertEquals(fieldlens[0],1);
	assertEquals(fieldlens[1],1);
	assertEquals(fieldlens[2],1);
	assertEquals(fieldlens[3],1);
	assertEquals(fieldlens[4],1);
	//assertEquals(fieldlens[5],3);
	assertEquals(fieldlens[6],3);
	assertEquals(fieldlens[7],3);
	assertEquals(fieldlens[8],10);
	assertEquals(fieldlens[9],8);
	assertEquals(fieldlens[10],19);
	assertEquals(fieldlens[11],4);
	assertEquals(fieldlens[12],5);
	assertEquals(fieldlens[13],8);
	assertEquals(fieldlens[14],5);
	assertEquals(fieldlens[15],9);
	assertEquals(fieldlens[16],11);
	assertEquals(fieldlens[17],9);
	assertEquals(fieldlens[18],5);
	assertEquals(fieldlens[19],9);
	assertEquals(fieldlens[20],11);
	assertEquals(fieldlens[21],9);
	stdoutput.printf("\n");


	// individual substitutions
	stdoutput.printf("INDIVIDUAL SUBSTITUTIONS: \n");
	cur->prepareQuery("select $(var1),'$(var2)',$(var3)");
	cur->substitution("var1",1);
	cur->substitution("var2","hello");
	cur->substitution("var3",10.5556,6,4);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// fields
	stdoutput.printf("FIELDS: \n");
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"hello");
	assertEquals(cur->getField(0,2),"10.5556");
	stdoutput.printf("\n");


	// array substitutions
	stdoutput.printf("ARRAY SUBSTITUTIONS: \n");
	cur->prepareQuery("select $(var1),$(var2),$(var3)");
	cur->substitutions(subvars,subvallongs);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// fields
	stdoutput.printf("FIELDS: \n");
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"2");
	assertEquals(cur->getField(0,2),"3");
	stdoutput.printf("\n");


	// array substitutions
	stdoutput.printf("ARRAY SUBSTITUTIONS: \n");
	cur->prepareQuery("select '$(var1)','$(var2)','$(var3)'");
	cur->substitutions(subvars,subvalstrings);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// fields
	stdoutput.printf("FIELDS: \n");
	assertEquals(cur->getField(0,(uint32_t)0),"hi");
	assertEquals(cur->getField(0,1),"hello");
	assertEquals(cur->getField(0,2),"bye");
	stdoutput.printf("\n");


	// array substitutions
	stdoutput.printf("ARRAY SUBSTITUTIONS: \n");
	cur->prepareQuery("select $(var1),$(var2),$(var3)");
	cur->substitutions(subvars,subvaldoubles,precs,scales);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// fields
	stdoutput.printf("FIELDS: \n");
	assertEquals(cur->getField(0,(uint32_t)0),"10.55");
	assertEquals(cur->getField(0,1),"10.556");
	assertEquals(cur->getField(0,2),"10.5556");
	stdoutput.printf("\n");


	// nulls as nulls
	stdoutput.printf("NULLS as Nulls: \n");
	cur->getNullsAsNulls();
	assertTrue(cur->sendQuery("select NULL,1,NULL"));
	assertEquals(cur->getField(0,(uint32_t)0),NULL);
	assertEquals(cur->getField(0,1),"1");
	assertEquals(cur->getField(0,2),NULL);
	cur->getNullsAsEmptyStrings();
	assertTrue(cur->sendQuery("select NULL,1,NULL"));
	assertEquals(cur->getField(0,(uint32_t)0),"");
	assertEquals(cur->getField(0,1),"1");
	assertEquals(cur->getField(0,2),"");
	cur->getNullsAsNulls();
	stdoutput.printf("\n");


	// result set buffer size
	stdoutput.printf("RESULT SET BUFFER SIZE: \n");
	assertEquals(cur->getResultSetBufferSize(),0);
	cur->setResultSetBufferSize(2);
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	assertEquals(cur->getResultSetBufferSize(),2);
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),0);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),2);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(1,(uint32_t)0),"2");
	assertEquals(cur->getField(2,(uint32_t)0),"3");
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),2);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),4);
	assertEquals(cur->getField(6,(uint32_t)0),"7");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),6);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),8);
	assertEquals(cur->getField(8,(uint32_t)0),NULL);
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),8);
	assertTrue(cur->endOfResultSet());
	assertEquals(cur->rowCount(),8);
	stdoutput.printf("\n");


	// dont get column info
	stdoutput.printf("DONT GET COLUMN INFO: \n");
	cur->dontGetColumnInfo();
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	assertEquals(cur->getColumnName(0),NULL);
	assertEquals(cur->getColumnLength((uint32_t)0),0);
	assertEquals(cur->getColumnType((uint32_t)0),NULL);
	stdoutput.printf("\n");
	cur->getColumnInfo();
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	assertEquals(cur->getColumnName(0),"testtinyint");
	assertEquals(cur->getColumnLength((uint32_t)0),1);
	assertEquals(cur->getColumnType((uint32_t)0),"TINYINT");
	stdoutput.printf("\n");


	// suspended session
	stdoutput.printf("SUSPENDED SESSION: \n");
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	cur->suspendResultSet();
	assertTrue(con->suspendSession());
	port=con->getConnectionPort();
	socket=charstring::duplicate(con->getConnectionSocket());
	assertTrue(con->resumeSession(port,socket));
	stdoutput.printf("\n");
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(1,(uint32_t)0),"2");
	assertEquals(cur->getField(2,(uint32_t)0),"3");
	assertEquals(cur->getField(3,(uint32_t)0),"4");
	assertEquals(cur->getField(4,(uint32_t)0),"5");
	assertEquals(cur->getField(5,(uint32_t)0),"6");
	assertEquals(cur->getField(6,(uint32_t)0),"7");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	stdoutput.printf("\n");
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	cur->suspendResultSet();
	assertTrue(con->suspendSession());
	port=con->getConnectionPort();
	socket=charstring::duplicate(con->getConnectionSocket());
	assertTrue(con->resumeSession(port,socket));
	stdoutput.printf("\n");
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(1,(uint32_t)0),"2");
	assertEquals(cur->getField(2,(uint32_t)0),"3");
	assertEquals(cur->getField(3,(uint32_t)0),"4");
	assertEquals(cur->getField(4,(uint32_t)0),"5");
	assertEquals(cur->getField(5,(uint32_t)0),"6");
	assertEquals(cur->getField(6,(uint32_t)0),"7");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	stdoutput.printf("\n");
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	cur->suspendResultSet();
	assertTrue(con->suspendSession());
	port=con->getConnectionPort();
	socket=charstring::duplicate(con->getConnectionSocket());
	assertTrue(con->resumeSession(port,socket));
	stdoutput.printf("\n");
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(1,(uint32_t)0),"2");
	assertEquals(cur->getField(2,(uint32_t)0),"3");
	assertEquals(cur->getField(3,(uint32_t)0),"4");
	assertEquals(cur->getField(4,(uint32_t)0),"5");
	assertEquals(cur->getField(5,(uint32_t)0),"6");
	assertEquals(cur->getField(6,(uint32_t)0),"7");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	stdoutput.printf("\n");


	// suspended result set
	stdoutput.printf("SUSPENDED RESULT SET: \n");
	cur->setResultSetBufferSize(2);
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	assertEquals(cur->getField(2,(uint32_t)0),"3");
	id=cur->getResultSetId();
	cur->suspendResultSet();
	assertTrue(con->suspendSession());
	port=con->getConnectionPort();
	socket=charstring::duplicate(con->getConnectionSocket());
	assertTrue(con->resumeSession(port,socket));
	assertTrue(cur->resumeResultSet(id));
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),4);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),6);
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),6);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),8);
	assertEquals(cur->getField(8,(uint32_t)0),NULL);
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),8);
	assertTrue(cur->endOfResultSet());
	assertEquals(cur->rowCount(),8);
	cur->setResultSetBufferSize(0);
	stdoutput.printf("\n");


	// cached result set
	stdoutput.printf("CACHED RESULT SET: \n");
	cur->cacheToFile("cachefile1");
	cur->setCacheTtl(200);
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	filename=charstring::duplicate(cur->getCacheFileName());
	assertEquals(filename,"cachefile1");
	cur->cacheOff();
	assertTrue(cur->openCachedResultSet(filename));
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	delete[] filename;
	stdoutput.printf("\n");


	// column count for cached result set
	stdoutput.printf("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEquals(cur->colCount(),23);
	stdoutput.printf("\n");


	// column names for cached result set
	stdoutput.printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEquals(cur->getColumnName(0),"testtinyint");
	assertEquals(cur->getColumnName(1),"testsmallint");
	assertEquals(cur->getColumnName(2),"testmediumint");
	assertEquals(cur->getColumnName(3),"testint");
	assertEquals(cur->getColumnName(4),"testbigint");
	assertEquals(cur->getColumnName(5),"testfloat");
	assertEquals(cur->getColumnName(6),"testreal");
	assertEquals(cur->getColumnName(7),"testdecimal");
	assertEquals(cur->getColumnName(8),"testdate");
	assertEquals(cur->getColumnName(9),"testtime");
	assertEquals(cur->getColumnName(10),"testdatetime");
	assertEquals(cur->getColumnName(11),"testyear");
	assertEquals(cur->getColumnName(12),"testchar");
	assertEquals(cur->getColumnName(13),"testvarchar");
	assertEquals(cur->getColumnName(14),"testtext");
	assertEquals(cur->getColumnName(15),"testtinytext");
	assertEquals(cur->getColumnName(16),"testmediumtext");
	assertEquals(cur->getColumnName(17),"testlongtext");
	assertEquals(cur->getColumnName(18),"testblob");
	assertEquals(cur->getColumnName(19),"testtinyblob");
	assertEquals(cur->getColumnName(20),"testmediumblob");
	assertEquals(cur->getColumnName(21),"testlongblob");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"testtinyint");
	assertEquals(cols[1],"testsmallint");
	assertEquals(cols[2],"testmediumint");
	assertEquals(cols[3],"testint");
	assertEquals(cols[4],"testbigint");
	assertEquals(cols[5],"testfloat");
	assertEquals(cols[6],"testreal");
	assertEquals(cols[7],"testdecimal");
	assertEquals(cols[8],"testdate");
	assertEquals(cols[9],"testtime");
	assertEquals(cols[10],"testdatetime");
	assertEquals(cols[11],"testyear");
	assertEquals(cols[12],"testchar");
	assertEquals(cols[13],"testvarchar");
	assertEquals(cols[14],"testtext");
	assertEquals(cols[15],"testtinytext");
	assertEquals(cols[16],"testmediumtext");
	assertEquals(cols[17],"testlongtext");
	assertEquals(cols[18],"testblob");
	assertEquals(cols[19],"testtinyblob");
	assertEquals(cols[20],"testmediumblob");
	assertEquals(cols[21],"testlongblob");
	stdoutput.printf("\n");


	// cached result set with result set buffer size
	stdoutput.printf("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n");
	cur->setResultSetBufferSize(2);
	cur->cacheToFile("cachefile1");
	cur->setCacheTtl(200);
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	filename=charstring::duplicate(cur->getCacheFileName());
	assertEquals(filename,"cachefile1");
	cur->cacheOff();
	assertTrue(cur->openCachedResultSet(filename));
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(8,(uint32_t)0),NULL);
	cur->setResultSetBufferSize(0);
	delete[] filename;
	stdoutput.printf("\n");


	// from one cache file to another
	stdoutput.printf("FROM ONE CACHE FILE TO ANOTHER: \n");
	cur->cacheToFile("cachefile2");
	assertTrue(cur->openCachedResultSet("cachefile1"));
	cur->cacheOff();
	assertTrue(cur->openCachedResultSet("cachefile2"));
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(8,(uint32_t)0),NULL);
	stdoutput.printf("\n");


	// from one cache file to another with result set buffer size
	stdoutput.printf("FROM ONE CACHE FILE TO ANOTHER "
				"WITH RESULT SET BUFFER SIZE: \n");
	cur->setResultSetBufferSize(2);
	cur->cacheToFile("cachefile2");
	assertTrue(cur->openCachedResultSet("cachefile1"));
	cur->cacheOff();
	assertTrue(cur->openCachedResultSet("cachefile2"));
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(8,(uint32_t)0),NULL);
	cur->setResultSetBufferSize(0);
	stdoutput.printf("\n");


	// cached result set with suspend and result set buffer size
	stdoutput.printf("CACHED RESULT SET WITH SUSPEND "
				"AND RESULT SET BUFFER SIZE: \n");
	cur->setResultSetBufferSize(2);
	cur->cacheToFile("cachefile1");
	cur->setCacheTtl(200);
	assertTrue(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	assertEquals(cur->getField(2,(uint32_t)0),"3");
	filename=charstring::duplicate(cur->getCacheFileName());
	assertEquals(filename,"cachefile1");
	id=cur->getResultSetId();
	cur->suspendResultSet();
	assertTrue(con->suspendSession());
	port=con->getConnectionPort();
	socket=charstring::duplicate(con->getConnectionSocket());
	stdoutput.printf("\n");
	assertTrue(con->resumeSession(port,socket));
	assertTrue(cur->resumeCachedResultSet(id,filename));
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),4);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),6);
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),6);
	assertFalse(cur->endOfResultSet());
	assertEquals(cur->rowCount(),8);
	assertEquals(cur->getField(8,(uint32_t)0),NULL);
	stdoutput.printf("\n");
	assertEquals(cur->firstRowIndex(),8);
	assertTrue(cur->endOfResultSet());
	assertEquals(cur->rowCount(),8);
	cur->cacheOff();
	stdoutput.printf("\n");
	assertTrue(cur->openCachedResultSet(filename));
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(8,(uint32_t)0),NULL);
	cur->setResultSetBufferSize(0);
	delete[] filename;
	stdoutput.printf("\n");


	// commit and rollback
	stdoutput.printf("COMMIT AND ROLLBACK: \n");
	// Note: Mysql's default isolation level is repeatable-read,
	// not read-committed like most other db's.  Both sessions must
	// commit to see the changes that each other has made.
	secondcon=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	secondcur=new sqlrcursor(secondcon);
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	if (majorversion>3) {
		assertEquals(secondcur->getField(0,(uint32_t)0),"0");
	} else {
		assertEquals(secondcur->getField(0,(uint32_t)0),"8");
	}
	assertTrue(con->commit());
	assertTrue(secondcon->commit());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"8");
	assertTrue(con->autoCommitOn());
	assertTrue(cur->sendQuery(
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
		"	'varchar10', "
		"	'text10', "
		"	'tinytext10', "
		"	'mediumtext10', "
		"	'longtext10', "
		"	'blob10', "
		"	'tinyblob10', "
		"	'mediumblob10', "
		"	'longblob10', "
		"	NULL)"));
	assertTrue(secondcon->commit());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"9");
	assertTrue(con->autoCommitOff());
	secondcon->commit();
	stdoutput.printf("\n");


	// finished suspended session
	stdoutput.printf("FINISHED SUSPENDED SESSION: \n");
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	assertEquals(cur->getField(4,(uint32_t)0),"5");
	assertEquals(cur->getField(5,(uint32_t)0),"6");
	assertEquals(cur->getField(6,(uint32_t)0),"7");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	id=cur->getResultSetId();
	cur->suspendResultSet();
	assertTrue(con->suspendSession());
	port=con->getConnectionPort();
	socket=charstring::duplicate(con->getConnectionSocket());
	assertTrue(con->resumeSession(port,socket));
	assertTrue(cur->resumeResultSet(id));
	assertEquals(cur->getField(4,(uint32_t)0),NULL);
	assertEquals(cur->getField(5,(uint32_t)0),NULL);
	assertEquals(cur->getField(6,(uint32_t)0),NULL);
	assertEquals(cur->getField(7,(uint32_t)0),NULL);
	stdoutput.printf("\n");


	// temporary tables
	stdoutput.printf("TEMPORARY TABLES: \n");
	cur->sendQuery("drop table temptable\n");
	cur->sendQuery("create temporary table temptable (col1 int)");
	assertTrue(cur->sendQuery("insert into temptable values (1)"));
	assertTrue(cur->sendQuery("select count(*) from temptable"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	con->endSession();
	stdoutput.printf("\n");
	assertFalse(cur->sendQuery("select count(*) from temptable"));
	cur->sendQuery("drop table temptable\n");
	stdoutput.printf("\n");

	if (majorversion>3) {


		// functions
		stdoutput.printf("FUNCTIONS: \n");
		cur->sendQuery("drop function if exists testfunc");
		assertTrue(cur->sendQuery(
			"create function testfunc(in1 int, in2 "
			"	int) returns int return in1+in2;"));
		cur->prepareQuery("select testfunc(?,?)");
		cur->inputBind("1",10);
		cur->inputBind("2",20);
		assertTrue(cur->executeQuery());
		assertEquals(cur->getField(0,(uint32_t)0),"30");
		cur->sendQuery("drop function if exists testfunc");
		stdoutput.printf("\n");


		// stored procedures
		stdoutput.printf("STORED PROCEDURES: \n");
		// return no values
		cur->sendQuery("drop procedure if exists testproc");
		assertTrue(cur->sendQuery(
			"create procedure testproc("
			"	in in1 int, "
			"	in in2 float, "
			"	in in3 char(20)) "
			"begin "
			"	select in1, in2, in3; "
			"end;"));
		cur->prepareQuery("call testproc(?,?,?)");
		cur->inputBind("1",1);
		cur->inputBind("2",1.1,4,2);
		cur->inputBind("3","hello");
		assertTrue(cur->executeQuery());
		assertEquals(cur->getField(0,(uint32_t)0),"1");
		assertEquals(cur->getField(0,(uint32_t)1),"1.1");
		assertEquals(cur->getField(0,(uint32_t)2),"hello");
		cur->sendQuery("drop procedure testproc");
		stdoutput.printf("\n");
		// return values
		assertTrue(cur->sendQuery(
			"create procedure testproc("
			"	out out1 int, "
			"	out out2 float, "
			"	out out3 char(20)) "
			"begin "
			"	select 1, 1.1, 'hello' "
			"		into out1, out2, out3; "
			"end;"));
		assertTrue(cur->sendQuery("set @out1=0, @out2=0.0, @out3=''"));
		assertTrue(cur->sendQuery("call testproc(@out1,@out2,@out3)"));
		assertTrue(cur->sendQuery("select @out1, @out2, @out3"));
		assertEquals(cur->getField(0,(uint32_t)0),"1");
		//assertEquals(cur->getFieldAsDouble(0,(uint32_t)1),1.1);
		assertEquals(cur->getField(0,(uint32_t)2),"hello");
		cur->sendQuery("drop procedure testproc");
		stdoutput.printf("\n");
	}

	// drop existing table
	cur->sendQuery("drop table testtable");


	// long lobs
	stdoutput.printf("LONG LOBS: \n");
	cur->sendQuery("drop table testtable1");
	cur->sendQuery(
		"create table testtable1 ("
		"	testtext longtext, "
		"	testblob longblob)");
	cur->prepareQuery("insert into testtable1 values (?,?)");
	char	clobval[8*1024+1];
	char	blobval[8*1024+1];
	for (uint32_t i=0; i<8*1024; i++) {
		clobval[i]='C';
		blobval[i]='C';
	}
	clobval[8*1024]='\0';
	blobval[8*1024]='\0';
	cur->inputBindClob("1",clobval,8*1024);
	cur->inputBindBlob("2",blobval,8*1024);
	assertTrue(cur->executeQuery());
	cur->sendQuery("select * from testtable1");
	assertEquals(cur->getField(0,"testtext"),clobval);
	assertEquals(cur->getField(0,"testblob"),clobval);
	cur->sendQuery("drop table testtable1");
	stdoutput.printf("\n");


	// binary data
	if (majorversion>3) {
		// binary data - all chars - \-escaped
		stdoutput.printf("BINARY DATA - all chars - \\-escaped: \n");

		assertTrue(cur->sendQuery(
			"create table testtable (col1 longblob)"));

		// binary 0-255 (slash-escaped)
		byte_t	buffer[256];
		for (uint16_t i=0; i<256; i++) {
			buffer[i]=i;
		}
		stringbuffer	query;
		query.append("insert into testtable values (_binary'");
		for (uint64_t i=0; i<sizeof(buffer); i++) {
			if (buffer[i]=='\'') {
				query.append('\\');
			}
			if (buffer[i]=='\\') {
				query.append('\\');
			}
			query.append(buffer[i]);
		}
		query.append("')");
		assertTrue(cur->sendQuery(
				query.getString(),query.getSize()));
		assertTrue(cur->sendQuery("select col1 from testtable"));
		assertEquals(cur->getFieldLength(0,(uint32_t)0),sizeof(buffer));
		assertEquals(bytestring::compare(
					cur->getField(0,(uint32_t)0),
					buffer,sizeof(buffer)),0);
		assertTrue(cur->sendQuery("delete from testtable"));
		stdoutput.printf("\n");


		// binary data - '' - ''-escaped
		stdoutput.printf("BINARY DATA - '' - ''-escaped: \n");
		assertTrue(cur->sendQuery(
			"insert into testtable values (_binary'''''')"));
		assertTrue(cur->sendQuery("select col1 from testtable"));
		assertEquals(cur->getFieldLength(0,(uint32_t)0),2);
		assertEquals(charstring::compare(
					cur->getField(0,(uint32_t)0),
					"''"),0);
		assertTrue(cur->sendQuery("delete from testtable"));
		stdoutput.printf("\n");


		// binary data - '' - '',\-escaped
		stdoutput.printf("BINARY DATA - '' - '',\\-escaped: \n");
		assertTrue(cur->sendQuery(
			"insert into testtable values (_binary'''\\'')"));
		assertTrue(cur->sendQuery("select col1 from testtable"));
		assertEquals(cur->getFieldLength(0,(uint32_t)0),2);
		assertEquals(charstring::compare(
					cur->getField(0,(uint32_t)0),
					"''"),0);
		assertTrue(cur->sendQuery("delete from testtable"));
		stdoutput.printf("\n");


		// binary data - '' - \,''-escaped
		stdoutput.printf("BINARY DATA - '' - \\,''-escaped: \n");
		assertTrue(cur->sendQuery(
			"insert into testtable values (_binary'\\'''')"));
		assertTrue(cur->sendQuery("select col1 from testtable"));
		assertEquals(cur->getFieldLength(0,(uint32_t)0),2);
		assertEquals(charstring::compare(
					cur->getField(0,(uint32_t)0),
					"''"),0);
		assertTrue(cur->sendQuery("delete from testtable"));
		stdoutput.printf("\n");


		// binary data - "" - unescaped
		stdoutput.printf("BINARY DATA - \"\" - unescaped: \n");
		assertTrue(cur->sendQuery(
			"insert into testtable values (_binary'\"\"')"));
		assertTrue(cur->sendQuery("select col1 from testtable"));
		assertEquals(cur->getFieldLength(0,(uint32_t)0),2);
		assertEquals(charstring::compare(
					cur->getField(0,(uint32_t)0),
					"\"\""),0);
		assertTrue(cur->sendQuery("delete from testtable"));
		stdoutput.printf("\n");


		// binary data - (null)"" - unescaped
		stdoutput.printf("BINARY DATA - (null)\"\" - unescaped: \n");
		assertTrue(cur->sendQuery(
			"insert into testtable values "
			"(_binary'\0\"\"')",43));
		assertTrue(cur->sendQuery("select col1 from testtable"));
		assertEquals(cur->getFieldLength(0,(uint32_t)0),3);
		assertEquals(bytestring::compare(
					cur->getField(0,(uint32_t)0),
					"\0\"\"",3),0);
		assertTrue(cur->sendQuery("delete from testtable"));
		stdoutput.printf("\n");


		// binary data - \(null)\"\" - \-escaped
		stdoutput.printf(
			"BINARY DATA - \\(null)\\\"\\\" - \\-escaped: \n");
		assertTrue(cur->sendQuery(
			"insert into testtable values (_binary'\\\0\\\"\\\"')",
			46));
		assertTrue(cur->sendQuery("select col1 from testtable"));
		assertEquals(cur->getFieldLength(0,(uint32_t)0),3);
		assertEquals(bytestring::compare(
					cur->getField(0,(uint32_t)0),
					"\0\"\"",3),0);
		assertTrue(cur->sendQuery("delete from testtable"));
		stdoutput.printf("\n");


		// binary data - \\' - \-escaped
		stdoutput.printf("BINARY DATA - \\\\' - \\-escaped: \n");
		assertTrue(cur->sendQuery(
			"insert into testtable values (_binary'\\\\\\'')",
			44));
		assertTrue(cur->sendQuery("select col1 from testtable"));
		assertEquals(cur->getFieldLength(0,(uint32_t)0),2);
		assertEquals(bytestring::compare(
					cur->getField(0,(uint32_t)0),
					"\\\'",2),0);
		assertTrue(cur->sendQuery("delete from testtable"));
		stdoutput.printf("\n");


		// binary data - random - '',\-escaped
		stdoutput.printf("BINARY DATA - random - '',\\-escaped: \n");
		randomnumber	r1;
		randomnumber	r2;
		r1.setSeed(r1.getSeed());
		r2.setSeed(r2.getSeed());
		char	ch[]={'\'','"','\\','\0'};
		for (uint16_t i=0; i<sizeof(buffer); i++) {
			uint32_t	result1;
			r1.generate(&result1);
			r1.setSeed(result1);
			buffer[i]=ch[r1.scale(result1,0,3)];
		}
		query.clear();
		query.append("insert into testtable values (_binary'");
		for (uint64_t i=0; i<sizeof(buffer); i++) {
			uint32_t	result2;
			r2.generate(&result2);
			r2.setSeed(result2);
			if (buffer[i]=='\'') {
				// randomly escape with \ or ''
				if (r2.scale(result2,0,1)) {
					query.append('\'');
				} else {
					query.append('\\');
				}
			}
			if (buffer[i]=='"') {
				// randomly escape with \ or don't escape
				if (r2.scale(result2,0,1)) {
					query.append('\\');
				}
			}
			if (buffer[i]=='\\') {
				// escape with backslash
				query.append('\\');
			}
			query.append(buffer[i]);
		}
		query.append("')");
		assertTrue(cur->sendQuery(
				query.getString(),query.getSize()));
		assertTrue(cur->sendQuery("select col1 from testtable"));
		assertEquals(cur->getFieldLength(0,(uint32_t)0),sizeof(buffer));
		assertEquals(bytestring::compare(
					cur->getField(0,(uint32_t)0),
					buffer,sizeof(buffer)),0);
		assertTrue(cur->sendQuery("delete from testtable"));

		cur->sendQuery("drop table testtable");
		stdoutput.printf("\n");
	}


	// reexecute
	stdoutput.printf("REEXECUTE: \n");
	cur->prepareQuery("select 1");
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	cur->prepareQuery("select ?");
	cur->inputBind("1",1);
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	stdoutput.printf("\n");
	cur->inputBind("1",2);
	assertTrue(cur->executeQuery());
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getField(0,(uint32_t)0),"2");
	stdoutput.printf("\n");


	// database is schema
	stdoutput.printf("DATABASE IS SCHEMA: \n");
	assertTrue(con->getDatabaseIsSchema());
	stdoutput.printf("\n");


	// catalog list
	stdoutput.printf("CATALOG LIST: \n");
con->debugOn();
	assertTrue(cur->getCatalogList(NULL));
con->debugOff();
	assertEquals(cur->getColumnName(0),"Database");
	assertTrue(cur->rowCount()>0);
	stdoutput.printf("\n");


	// schema list
	stdoutput.printf("SCHEMA LIST: \n");
	assertTrue(cur->getSchemaList(NULL));
	assertEquals(cur->getColumnName(0),"Database");
	assertTrue(cur->rowCount()>0);
	stdoutput.printf("\n");


	// table type list
	stdoutput.printf("TABLE TYPE LIST: \n");
	assertTrue(cur->getTableTypeList());
	assertEquals(cur->getColumnName(0),"table_type");
	bool	found=false;
	for (uint64_t i=0; i<cur->rowCount(); i++) {
		if (!charstring::compareIgnoringCase(
				cur->getField(i,"table_type"),
				"TABLE")) {
			found=true;
			break;
		}
	}
	assertTrue(found);
	stdoutput.printf("\n");


	// table list
	stdoutput.printf("TABLE LIST: \n");
	cur->sendQuery("drop table testtable1");
	cur->sendQuery("drop table testtable2");
	cur->sendQuery("drop table testtable3");
	cur->sendQuery("drop table testtable4");
	assertTrue(cur->sendQuery(
		"create table testtable1 ("
		"	col1 int, "
		"	col2 int)"));
	assertTrue(cur->sendQuery(
		"create table testtable2 ("
		"	col1 int, "
		"	col2 int)"));
	assertTrue(cur->sendQuery(
		"create table testtable3 ("
		"	col1 int, "
		"	col2 int)"));
	assertTrue(cur->sendQuery(
		"create table testtable4 ("
		"	col1 int, "
		"	col2 int)"));
	assertTrue(cur->getTableList(NULL));
	counter=0;
	for (uint64_t i=0; i<cur->rowCount(); i++) {
		const char	*name=cur->getField(i,"Tables_in_xxx");
		if (!charstring::compareIgnoringCase(name,"TESTTABLE1") ||
			!charstring::compareIgnoringCase(name,"TESTTABLE2") ||
			!charstring::compareIgnoringCase(name,"TESTTABLE3") ||
			!charstring::compareIgnoringCase(name,"TESTTABLE4")) {
			counter++;
		}
	}
	assertEquals(counter,4);
	cur->sendQuery("drop table testtable1");
	cur->sendQuery("drop table testtable2");
	cur->sendQuery("drop table testtable3");
	cur->sendQuery("drop table testtable4");
	stdoutput.printf("\n");


	// type info list
	stdoutput.printf("TYPE INFO LIST: \n");
	assertTrue(cur->getTypeInfoList("int"));
	assertEquals(cur->getColumnName(0),"type_name");
	assertEquals(cur->getColumnName(1),"data_type");
	assertEquals(cur->getColumnName(2),"precision");
	assertEquals(cur->getColumnName(3),"literal_prefix");
	assertEquals(cur->getColumnName(4),"literal_suffix");
	assertEquals(cur->getColumnName(5),"create_params");
	assertEquals(cur->getColumnName(6),"nullable");
	assertEquals(cur->getColumnName(7),"case_sensitive");
	assertEquals(cur->getColumnName(8),"searchable");
	assertEquals(cur->getColumnName(9),"unsigned_attribute");
	assertEquals(cur->getColumnName(10),"fixed_prec_scale");
	assertEquals(cur->getColumnName(11),"auto_increment");
	assertEquals(cur->getColumnName(12),"local_type_name");
	assertEquals(cur->getColumnName(13),"minumum_scale");
	assertEquals(cur->getColumnName(14),"maxiumm_scale");
	assertEquals(cur->getColumnName(15),"sql_data_type");
	assertEquals(cur->getColumnName(16),"sql_datetime_sub");
	assertEquals(cur->getColumnName(17),"num_prec_radix");
	assertEquals(cur->getColumnName(18),"interval_precision");
	assertEquals(cur->getField(0,"type_name"),"INT");
	assertEquals(cur->getField(0,"data_type"),"4");
	assertEquals(cur->getField(0,"precision"),"10");
	assertEquals(cur->getField(0,"local_type_name"),"INT");
	assertTrue(cur->getTypeInfoList("char"));
	assertEquals(cur->getField(0,"type_name"),"CHAR");
	assertEquals(cur->getField(0,"data_type"),"1");
	assertEquals(cur->getField(0,"precision"),"255");
	assertEquals(cur->getField(0,"local_type_name"),"CHAR");
	assertTrue(cur->getTypeInfoList("varchar"));
	assertEquals(cur->getField(0,"type_name"),"VARCHAR");
	assertEquals(cur->getField(0,"data_type"),"12");
	assertEquals(cur->getField(0,"precision"),"65535");
	assertEquals(cur->getField(0,"local_type_name"),"VARCHAR");
	assertTrue(cur->getTypeInfoList("date"));
	assertEquals(cur->getField(0,"type_name"),"DATE");
	assertEquals(cur->getField(0,"data_type"),"91");
	assertEquals(cur->getField(0,"precision"),"10");
	assertEquals(cur->getField(0,"local_type_name"),"DATE");
	stdoutput.printf("\n");


	// column list - auto_increment, primary key
	stdoutput.printf("COLUMN LIST - auto_increment, primary key: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 int auto_increment primary key, "
		"	col2 int)"));
	assertTrue(cur->getColumnList("testtable",NULL));
	assertTrue(charstring::containsIgnoringCase(
			cur->getField(0,"extra"),"auto_increment"));
	assertTrue(charstring::containsIgnoringCase(
			cur->getField(0,"column_key"),"PRI"));
	assertFalse(charstring::containsIgnoringCase(
			cur->getField(1,"extra"),"auto_increment"));
	assertFalse(charstring::containsIgnoringCase(
			cur->getField(1,"column_key"),"PRI"));
	stdoutput.printf("\n");
	assertTrue(cur->sendQuery("drop table testtable"));
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)"));
	assertTrue(cur->getColumnList("testtable",NULL));
	assertFalse(charstring::containsIgnoringCase(
			cur->getField(0,"extra"),"auto_increment"));
	assertTrue(charstring::containsIgnoringCase(
			cur->getField(0,"column_key"),"PRI"));
	assertTrue(cur->sendQuery("drop table testtable"));
	stdoutput.printf("\n");


	// primary keys list
	stdoutput.printf("PRIMARY KEYS LIST: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)"));
	assertTrue(cur->getPrimaryKeysList("testtable",NULL));
	assertEquals(cur->getColumnName(0),"table");
	assertEquals(cur->getColumnName(1),"non_unique");
	assertEquals(cur->getColumnName(2),"key_name");
	assertEquals(cur->getColumnName(3),"seq_in_index");
	assertEquals(cur->getColumnName(4),"column_name");
	assertEquals(cur->getColumnName(5),"collation");
	assertEquals(cur->getColumnName(6),"cardinality");
	assertEquals(cur->getColumnName(7),"sub_part");
	assertEquals(cur->getColumnName(8),"packed");
	assertEquals(cur->getColumnName(9),"null");
	assertEquals(cur->getColumnName(10),"index_type");
	assertEquals(cur->getColumnName(11),"comment");
	assertEquals(cur->getColumnName(12),"index_comment");
	assertEquals(cur->rowCount(),1);
	assertTrue(!charstring::compareIgnoringCase(
			cur->getField(0,"table"),"testtable"));
	assertEquals(cur->getField(0,"seq_in_index"),"1");
	assertTrue(!charstring::compareIgnoringCase(
			cur->getField(0,"column_name"),"col1"));
	assertEquals(cur->getField(0,"key_name"),"PRIMARY");
	cur->sendQuery("drop table testtable");
	stdoutput.printf("\n");


	// key and index list
	stdoutput.printf("KEY AND INDEX LIST: \n");
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery(
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)"));
	assertTrue(cur->getKeyAndIndexList("testtable",NULL));
	assertEquals(cur->getColumnName(0),"table");
	assertEquals(cur->getColumnName(1),"non_unique");
	assertEquals(cur->getColumnName(2),"key_name");
	assertEquals(cur->getColumnName(3),"seq_in_index");
	assertEquals(cur->getColumnName(4),"column_name");
	assertEquals(cur->getColumnName(5),"collation");
	assertEquals(cur->getColumnName(6),"cardinality");
	assertEquals(cur->getColumnName(7),"sub_part");
	assertEquals(cur->getColumnName(8),"packed");
	assertEquals(cur->getColumnName(9),"null");
	assertEquals(cur->getColumnName(10),"index_type");
	assertEquals(cur->getColumnName(11),"comment");
	assertEquals(cur->getColumnName(12),"index_comment");
	assertEquals(cur->rowCount(),1);
	assertTrue(!charstring::compareIgnoringCase(
			cur->getField(0,"table"),"testtable"));
	assertEquals(cur->getField(0,"non_unique"),"0");
	assertEquals(cur->getField(0,"seq_in_index"),"1");
	assertTrue(!charstring::compareIgnoringCase(
			cur->getField(0,"column_name"),"col1"));
	assertEquals(cur->getField(0,"collation"),"A");
	assertEquals(cur->getField(0,"index_type"),"3");
	assertEquals(cur->getField(0,"key_name"),"PRIMARY");
	cur->sendQuery("drop table testtable");
	stdoutput.printf("\n");


	// procedure list
	stdoutput.printf("PROCEDURE LIST: \n");
	cur->sendQuery("drop procedure testproc1");
	cur->sendQuery("drop procedure testproc2");
	cur->sendQuery("drop procedure testproc3");
	cur->sendQuery("drop procedure testproc4");
	assertTrue(cur->sendQuery(
		"create procedure testproc1("
		"	in in1 int, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) "
		"begin end"));
	assertTrue(cur->sendQuery(
		"create procedure testproc2("
		"	in in1 int, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) "
		"begin end"));
	assertTrue(cur->sendQuery(
		"create procedure testproc3("
		"	in in1 int, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) "
		"begin end"));
	assertTrue(cur->sendQuery(
		"create procedure testproc4("
		"	in in1 int, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) "
		"begin end"));
	assertTrue(cur->getProcedureList(NULL));
	counter=0;
	for (uint64_t i=0; i<cur->rowCount(); i++) {
		const char	*name=cur->getField(i,"routine_name");
		if (!charstring::compareIgnoringCase(name,"TESTPROC1") ||
			!charstring::compareIgnoringCase(name,"TESTPROC2") ||
			!charstring::compareIgnoringCase(name,"TESTPROC3") ||
			!charstring::compareIgnoringCase(name,"TESTPROC4")) {
			counter++;
		}
	}
	assertEquals(counter,4);
	stdoutput.printf("\n");


	// procedure parameter list
	stdoutput.printf("PROCEDURE PARAMETER LIST: \n");
	assertTrue(cur->getProcedureParameterList("testproc1",NULL));
	assertEquals(cur->getColumnName(0),"parameter_name");
	assertEquals(cur->getColumnName(1),"parameter_mode");
	assertEquals(cur->getColumnName(2),"data_type");
	assertEquals(cur->getColumnName(3),"character_maximum_length");
	assertEquals(cur->getColumnName(4),"ordinal_position");
	assertEquals(cur->rowCount(),4);
	assertEquals(cur->getField(0,"parameter_name"),"in1");
	assertEquals(cur->getField(0,"parameter_mode"),"1");
	assertEquals(cur->getField(0,"data_type"),"int");
	assertEquals(cur->getField(0,"ordinal_position"),"1");
	assertEquals(cur->getField(1,"parameter_name"),"in2");
	assertEquals(cur->getField(1,"parameter_mode"),"1");
	assertEquals(cur->getField(1,"data_type"),"char");
	assertEquals(cur->getField(1,"ordinal_position"),"2");
	assertEquals(cur->getField(2,"parameter_name"),"in3");
	assertEquals(cur->getField(2,"parameter_mode"),"1");
	assertEquals(cur->getField(2,"data_type"),"varchar");
	assertEquals(cur->getField(2,"ordinal_position"),"3");
	assertEquals(cur->getField(3,"parameter_name"),"in4");
	assertEquals(cur->getField(3,"parameter_mode"),"1");
	assertEquals(cur->getField(3,"data_type"),"date");
	assertEquals(cur->getField(3,"ordinal_position"),"4");
	cur->sendQuery("drop procedure testproc1");
	cur->sendQuery("drop procedure testproc2");
	cur->sendQuery("drop procedure testproc3");
	cur->sendQuery("drop procedure testproc4");
	stdoutput.printf("\n");


	// invalid queries
	stdoutput.printf("INVALID QUERIES: \n");
	assertFalse(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	assertFalse(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	assertFalse(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	assertFalse(cur->sendQuery(
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testtinyint "));
	stdoutput.printf("\n");
	assertFalse(cur->sendQuery("insert into testtable values (1,2,3,4)"));
	assertFalse(cur->sendQuery("insert into testtable values (1,2,3,4)"));
	assertFalse(cur->sendQuery("insert into testtable values (1,2,3,4)"));
	assertFalse(cur->sendQuery("insert into testtable values (1,2,3,4)"));
	stdoutput.printf("\n");
	assertFalse(cur->sendQuery("create table testtable"));
	assertFalse(cur->sendQuery("create table testtable"));
	assertFalse(cur->sendQuery("create table testtable"));
	assertFalse(cur->sendQuery("create table testtable"));
	stdoutput.printf("\n");

	delete secondcur;
	delete secondcon;
	delete cur;
	delete con;

#ifdef PROFILING
}
#endif

	reportTestStatus();

	return status;
}
