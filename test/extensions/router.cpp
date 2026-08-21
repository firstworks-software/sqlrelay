// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclient.h>
#include <rudiments/charstring.h>
#include <rudiments/process.h>
#include <rudiments/snooze.h>
#include <rudiments/stdio.h>

#include "../c++/asserts.cpp"

sqlrconnection	*con=NULL;
sqlrcursor	*cur=NULL;
sqlrconnection	*secondcon=NULL;
sqlrcursor	*secondcur=NULL;

int main(int argc, char **argv) {

	const char * const *cols;
	const char * const *fields;
	uint32_t	*fieldlens;

	// instantiation
	con=new sqlrconnection("sqlrelay",9015,"/tmp/router.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);

	// get database type


	// identify
	stdoutput.printf("IDENTIFY: \n");
	assertEquals(con->identify(),"router");
	stdoutput.printf("\n");

	// get the db version
	const char	*dbversion=con->dbVersion();
	uint32_t	majorversion=dbversion[0]-'0';


	// ping
	stdoutput.printf("PING: \n");
	assertTrue(con->ping());
	stdoutput.printf("\n");


	// filtered-out queries
	stdoutput.printf("FILTERED-OUT QUERIES: \n");
	assertFalse(cur->sendQuery("create table junktable (col1 int)"));
	assertFalse(cur->sendQuery("insert into junktable values (1)"));
	assertFalse(cur->sendQuery("update junktable set col1=2"));
	assertFalse(cur->sendQuery("delete from junktable"));
	assertFalse(cur->sendQuery("drop table junktable (col1 int)"));
	stdoutput.printf("\n");


	// create testtables
	stdoutput.printf("CREATE TESTTABLES: \n");
	cur->sendQuery("drop table if exists testtable1");
	cur->sendQuery("drop table if exists testtable2");
	assertTrue(cur->sendQuery(
		"create table testtable1 ("
		"	testint int, "
		"	testfloat float, "
		"	testreal real, "
		"	testsmallint smallint, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testdate date, "
		"	testtime time, "
		"	testtimestamp timestamp)"));
	assertTrue(cur->sendQuery(
		"create table testtable2 ("
		"	testint int, "
		"	testfloat float, "
		"	testreal real, "
		"	testsmallint smallint, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testdate date, "
		"	testtime time, "
		"	testtimestamp timestamp)"));
	stdoutput.printf("\n");


	// insert
	stdoutput.printf("INSERT: \n");
	assertTrue(cur->sendQuery("begin"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable1 "
		"values ("
		"	1, "
		"	1.5, "
		"	1.5, "
		"	1, "
		"	'testchar1', "
		"	'testvarchar1', "
		"	'2001-01-01', "
		"	'01:00:00', "
		"	NULL)"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable1 "
		"values ("
		"	2, "
		"	2.5, "
		"	2.5, "
		"	2, "
		"	'testchar2', "
		"	'testvarchar2', "
		"	'2002-01-01', "
		"	'02:00:00', "
		"	NULL)"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable1 "
		"values ("
		"	3, "
		"	3.5, "
		"	3.5, "
		"	3, "
		"	'testchar3', "
		"	'testvarchar3', "
		"	'2003-01-01', "
		"	'03:00:00', "
		"	NULL)"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable1 "
		"values ("
		"	4, "
		"	4.5, "
		"	4.5, "
		"	4, "
		"	'testchar4', "
		"	'testvarchar4', "
		"	'2004-01-01', "
		"	'04:00:00', "
		"	NULL)"));
	stdoutput.printf("\n");
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable2 "
		"values ("
		"	1, "
		"	1.5, "
		"	1.5, "
		"	1, "
		"	'testchar1', "
		"	'testvarchar1', "
		"	'2001-01-01', "
		"	'01:00:00', "
		"	NULL)"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable2 "
		"values ("
		"	2, "
		"	2.5, "
		"	2.5, "
		"	2, "
		"	'testchar2', "
		"	'testvarchar2', "
		"	'2002-01-01', "
		"	'02:00:00', "
		"	NULL)"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable2 "
		"values ("
		"	3, "
		"	3.5, "
		"	3.5, "
		"	3, "
		"	'testchar3', "
		"	'testvarchar3', "
		"	'2003-01-01', "
		"	'03:00:00', "
		"	NULL)"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable2 "
		"values ("
		"	4, "
		"	4.5, "
		"	4.5, "
		"	4, "
		"	'testchar4', "
		"	'testvarchar4', "
		"	'2004-01-01', "
		"	'04:00:00', "
		"	NULL)"));
	stdoutput.printf("\n");


	// affected rows
	stdoutput.printf("AFFECTED ROWS: \n");
	assertEquals(cur->affectedRows(),1);
	stdoutput.printf("\n");


	// input bind by name
	stdoutput.printf("INPUT BIND BY NAME: \n");
	cur->prepareQuery(
		"insert into "
		"	testtable1 "
		"values ("
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	NULL)");
	assertEquals(cur->countBindVariables(),8);
	cur->inputBind("1",5);
	cur->inputBind("2",5.5,4,2);
	cur->inputBind("3",5.5,4,2);
	cur->inputBind("4",5);
	cur->inputBind("5","testchar5");
	cur->inputBind("6","testvarchar5");
	cur->inputBind("7","2005-01-01");
	cur->inputBind("8","05:00:00");
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",6);
	cur->inputBind("2",6.5,4,2);
	cur->inputBind("3",6.5,4,2);
	cur->inputBind("4",6);
	cur->inputBind("5","testchar6");
	cur->inputBind("6","testvarchar6");
	cur->inputBind("7","2006-01-01");
	cur->inputBind("8","06:00:00");
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",7);
	cur->inputBind("2",7.5,4,2);
	cur->inputBind("3",7.5,4,2);
	cur->inputBind("4",7);
	cur->inputBind("5","testchar7");
	cur->inputBind("6","testvarchar7");
	cur->inputBind("7","2007-01-01");
	cur->inputBind("8","07:00:00");
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// input bind by name with validation
	stdoutput.printf("INPUT BIND BY NAME WITH VALIDATION: \n");
	cur->clearBinds();
	cur->inputBind("1",8);
	cur->inputBind("2",8.5,4,2);
	cur->inputBind("3",8.5,4,2);
	cur->inputBind("4",8);
	cur->inputBind("5","testchar8");
	cur->inputBind("6","testvarchar8");
	cur->inputBind("7","2008-01-01");
	cur->inputBind("8","08:00:00");
	cur->inputBind("9","junkvalue");
	cur->validateBinds();
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// input bind by name
	stdoutput.printf("INPUT BIND BY NAME: \n");
	cur->prepareQuery(
		"insert into "
		"	testtable2 "
		"values ("
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	NULL)");
	assertEquals(cur->countBindVariables(),8);
	cur->inputBind("1",5);
	cur->inputBind("2",5.5,4,2);
	cur->inputBind("3",5.5,4,2);
	cur->inputBind("4",5);
	cur->inputBind("5","testchar5");
	cur->inputBind("6","testvarchar5");
	cur->inputBind("7","2005-01-01");
	cur->inputBind("8","05:00:00");
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",6);
	cur->inputBind("2",6.5,4,2);
	cur->inputBind("3",6.5,4,2);
	cur->inputBind("4",6);
	cur->inputBind("5","testchar6");
	cur->inputBind("6","testvarchar6");
	cur->inputBind("7","2006-01-01");
	cur->inputBind("8","06:00:00");
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",7);
	cur->inputBind("2",7.5,4,2);
	cur->inputBind("3",7.5,4,2);
	cur->inputBind("4",7);
	cur->inputBind("5","testchar7");
	cur->inputBind("6","testvarchar7");
	cur->inputBind("7","2007-01-01");
	cur->inputBind("8","07:00:00");
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// input bind by name with validation
	stdoutput.printf("INPUT BIND BY NAME WITH VALIDATION: \n");
	cur->clearBinds();
	cur->inputBind("1",8);
	cur->inputBind("2",8.5,4,2);
	cur->inputBind("3",8.5,4,2);
	cur->inputBind("4",8);
	cur->inputBind("5","testchar8");
	cur->inputBind("6","testvarchar8");
	cur->inputBind("7","2008-01-01");
	cur->inputBind("8","08:00:00");
	cur->inputBind("9","junkvalue");
	cur->validateBinds();
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");


	// select
	stdoutput.printf("SELECT: \n");
	// It may take some time for the replication to actually occur.
	// Exactly how long depends on how busy everything is.  So, loop
	// until we get the value that we're looking for, or fail 10 times.
	uint64_t	rowcount=0;
	for (uint16_t i=0; i<10; i++) {
		assertTrue(con->commit());
		stdoutput.printf("loop %d...\n",i);
		bool	success=cur->sendQuery(
				"select * from testtable1 order by testint");
		if (!success) {
			assertTrue(success);
		}
		rowcount=cur->rowCount();
		if (rowcount==8) {
			break;
		}
		snooze::macrosnooze(1,0);
	}
	stdoutput.printf("\n");


	// row count
	stdoutput.printf("ROW COUNT: \n");
	assertEquals(rowcount,8);
	stdoutput.printf("\n");


	// column count
	stdoutput.printf("COLUMN COUNT: \n");
	assertEquals(cur->colCount(),9);
	stdoutput.printf("\n");


	// column names
	stdoutput.printf("COLUMN NAMES: \n");
	assertEquals(cur->getColumnName(0),"testint");
	assertEquals(cur->getColumnName(1),"testfloat");
	assertEquals(cur->getColumnName(2),"testreal");
	assertEquals(cur->getColumnName(3),"testsmallint");
	assertEquals(cur->getColumnName(4),"testchar");
	assertEquals(cur->getColumnName(5),"testvarchar");
	assertEquals(cur->getColumnName(6),"testdate");
	assertEquals(cur->getColumnName(7),"testtime");
	assertEquals(cur->getColumnName(8),"testtimestamp");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"testint");
	assertEquals(cols[1],"testfloat");
	assertEquals(cols[2],"testreal");
	assertEquals(cols[3],"testsmallint");
	assertEquals(cols[4],"testchar");
	assertEquals(cols[5],"testvarchar");
	assertEquals(cols[6],"testdate");
	assertEquals(cols[7],"testtime");
	assertEquals(cols[8],"testtimestamp");
	stdoutput.printf("\n");


	// column types
	stdoutput.printf("COLUMN TYPES: \n");
	assertEquals(cur->getColumnType((uint32_t)0),"INT");
	assertEquals(cur->getColumnType("testint"),"INT");
	assertEquals(cur->getColumnType(1),"FLOAT");
	assertEquals(cur->getColumnType("testfloat"),"FLOAT");
	assertEquals(cur->getColumnType(2),"REAL");
	assertEquals(cur->getColumnType("testreal"),"REAL");
	assertEquals(cur->getColumnType(3),"SMALLINT");
	assertEquals(cur->getColumnType("testsmallint"),"SMALLINT");
	if (majorversion==3) {
		assertEquals(cur->getColumnType(4),"VARSTRING");
		assertEquals(cur->getColumnType("testchar"),"VARSTRING");
	} else {
		assertEquals(cur->getColumnType(4),"STRING");
		assertEquals(cur->getColumnType("testchar"),"STRING");
	}
	assertEquals(cur->getColumnType(5),"VARSTRING");
	assertEquals(cur->getColumnType("testvarchar"),"VARSTRING");
	assertEquals(cur->getColumnType(6),"DATE");
	assertEquals(cur->getColumnType("testdate"),"DATE");
	assertEquals(cur->getColumnType(7),"TIME");
	assertEquals(cur->getColumnType("testtime"),"TIME");
	assertEquals(cur->getColumnType(8),"TIMESTAMP");
	assertEquals(cur->getColumnType("testtimestamp"),"TIMESTAMP");
	stdoutput.printf("\n");


	// column length
	stdoutput.printf("COLUMN LENGTH: \n");
	assertEquals(cur->getColumnLength((uint32_t)0),4);
	assertEquals(cur->getColumnLength("testint"),4);
	assertEquals(cur->getColumnLength(1),4);
	assertEquals(cur->getColumnLength("testfloat"),4);
	assertEquals(cur->getColumnLength(2),8);
	assertEquals(cur->getColumnLength("testreal"),8);
	assertEquals(cur->getColumnLength(3),2);
	assertEquals(cur->getColumnLength("testsmallint"),2);
	// testchar/testvarchar are char(40)/varchar(40); the mysql connection
	// charset is latin1 (1 byte/char) so the lengths are 40/41
	// mysql 3 silently converts char to varchar, adding a length byte,
	// which is why it reports testchar as VARSTRING above
	if (majorversion==3) {
		assertEquals(cur->getColumnLength(4),41);
		assertEquals(cur->getColumnLength("testchar"),41);
	} else {
		assertEquals(cur->getColumnLength(4),40);
		assertEquals(cur->getColumnLength("testchar"),40);
	}
	assertEquals(cur->getColumnLength(5),41);
	assertEquals(cur->getColumnLength("testvarchar"),41);
	assertEquals(cur->getColumnLength(6),3);
	assertEquals(cur->getColumnLength("testdate"),3);
	assertEquals(cur->getColumnLength(7),3);
	assertEquals(cur->getColumnLength("testtime"),3);
	assertEquals(cur->getColumnLength(8),4);
	assertEquals(cur->getColumnLength("testtimestamp"),4);
	stdoutput.printf("\n");

	/*stdoutput.printf("LONGEST COLUMN: \n");
	// FIXME: weird, this returns 0 but the next one works
	assertEquals(cur->getLongest((uint32_t)0),1);
	assertEquals(cur->getLongest("testint"),1);
	assertEquals(cur->getLongest(1),3);
	assertEquals(cur->getLongest("testfloat"),3);
	assertEquals(cur->getLongest(2),3);
	assertEquals(cur->getLongest("testreal"),3);
	assertEquals(cur->getLongest(3),1);
	assertEquals(cur->getLongest("testsmallint"),1);
	assertEquals(cur->getLongest(4),9);
	assertEquals(cur->getLongest("testchar"),9);
	assertEquals(cur->getLongest(5),12);
	assertEquals(cur->getLongest("testvarchar"),12);
	assertEquals(cur->getLongest(6),10);
	assertEquals(cur->getLongest("testdate"),10);
	assertEquals(cur->getLongest(7),8);
	assertEquals(cur->getLongest("testtime"),8);
	stdoutput.printf("\n");*/


	// total rows
	stdoutput.printf("TOTAL ROWS: \n");
	assertEquals(cur->totalRows(),8);
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
	assertEquals(cur->getField(0,1),"1.5");
	assertEquals(cur->getField(0,2),"1.5");
	assertEquals(cur->getField(0,3),"1");
	assertEquals(cur->getField(0,4),"testchar1");
	assertEquals(cur->getField(0,5),"testvarchar1");
	assertEquals(cur->getField(0,6),"2001-01-01");
	assertEquals(cur->getField(0,7),"01:00:00");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(7,1),"8.5");
	assertEquals(cur->getField(7,2),"8.5");
	assertEquals(cur->getField(7,3),"8");
	assertEquals(cur->getField(7,4),"testchar8");
	assertEquals(cur->getField(7,5),"testvarchar8");
	assertEquals(cur->getField(7,6),"2008-01-01");
	assertEquals(cur->getField(7,7),"08:00:00");
	stdoutput.printf("\n");


	// field lengths by index
	stdoutput.printf("FIELD LENGTHS BY INDEX: \n");
	assertEquals(cur->getFieldLength(0,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(0,1),3);
	assertEquals(cur->getFieldLength(0,2),3);
	assertEquals(cur->getFieldLength(0,3),1);
	assertEquals(cur->getFieldLength(0,4),9);
	assertEquals(cur->getFieldLength(0,5),12);
	assertEquals(cur->getFieldLength(0,6),10);
	assertEquals(cur->getFieldLength(0,7),8);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(7,1),3);
	assertEquals(cur->getFieldLength(7,2),3);
	assertEquals(cur->getFieldLength(7,3),1);
	assertEquals(cur->getFieldLength(7,4),9);
	assertEquals(cur->getFieldLength(7,5),12);
	assertEquals(cur->getFieldLength(7,6),10);
	assertEquals(cur->getFieldLength(7,7),8);
	stdoutput.printf("\n");


	// fields by name
	stdoutput.printf("FIELDS BY NAME: \n");
	assertEquals(cur->getField(0,"testint"),"1");
	assertEquals(cur->getField(0,"testfloat"),"1.5");
	assertEquals(cur->getField(0,"testreal"),"1.5");
	assertEquals(cur->getField(0,"testsmallint"),"1");
	assertEquals(cur->getField(0,"testchar"),"testchar1");
	assertEquals(cur->getField(0,"testvarchar"),"testvarchar1");
	assertEquals(cur->getField(0,"testdate"),"2001-01-01");
	assertEquals(cur->getField(0,"testtime"),"01:00:00");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,"testint"),"8");
	assertEquals(cur->getField(7,"testfloat"),"8.5");
	assertEquals(cur->getField(7,"testreal"),"8.5");
	assertEquals(cur->getField(7,"testsmallint"),"8");
	assertEquals(cur->getField(7,"testchar"),"testchar8");
	assertEquals(cur->getField(7,"testvarchar"),"testvarchar8");
	assertEquals(cur->getField(7,"testdate"),"2008-01-01");
	assertEquals(cur->getField(7,"testtime"),"08:00:00");
	stdoutput.printf("\n");


	// field lengths by name
	stdoutput.printf("FIELD LENGTHS BY NAME: \n");
	assertEquals(cur->getFieldLength(0,"testint"),1);
	assertEquals(cur->getFieldLength(0,"testfloat"),3);
	assertEquals(cur->getFieldLength(0,"testreal"),3);
	assertEquals(cur->getFieldLength(0,"testsmallint"),1);
	assertEquals(cur->getFieldLength(0,"testchar"),9);
	assertEquals(cur->getFieldLength(0,"testvarchar"),12);
	assertEquals(cur->getFieldLength(0,"testdate"),10);
	assertEquals(cur->getFieldLength(0,"testtime"),8);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,"testint"),1);
	assertEquals(cur->getFieldLength(7,"testfloat"),3);
	assertEquals(cur->getFieldLength(7,"testreal"),3);
	assertEquals(cur->getFieldLength(7,"testsmallint"),1);
	assertEquals(cur->getFieldLength(7,"testchar"),9);
	assertEquals(cur->getFieldLength(7,"testvarchar"),12);
	assertEquals(cur->getFieldLength(7,"testdate"),10);
	assertEquals(cur->getFieldLength(7,"testtime"),8);
	stdoutput.printf("\n");


	// fields by array
	stdoutput.printf("FIELDS BY ARRAY: \n");
	fields=cur->getRow(0);
	assertEquals(fields[0],"1");
	assertEquals(fields[1],"1.5");
	assertEquals(fields[2],"1.5");
	assertEquals(fields[3],"1");
	assertEquals(fields[4],"testchar1");
	assertEquals(fields[5],"testvarchar1");
	assertEquals(fields[6],"2001-01-01");
	assertEquals(fields[7],"01:00:00");
	stdoutput.printf("\n");


	// field lengths by array
	stdoutput.printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=cur->getRowLengths(0);
	assertEquals(fieldlens[0],1);
	assertEquals(fieldlens[1],3);
	assertEquals(fieldlens[2],3);
	assertEquals(fieldlens[3],1);
	assertEquals(fieldlens[4],9);
	assertEquals(fieldlens[5],12);
	assertEquals(fieldlens[6],10);
	assertEquals(fieldlens[7],8);
	stdoutput.printf("\n");


	// select
	stdoutput.printf("SELECT: \n");
	// It may take some time for the replication to actually occur.
	// Exactly how long depends on how busy everything is.  So, loop
	// until we get the value that we're looking for, or fail 10 times.
	rowcount=0;
	for (uint16_t i=0; i<10; i++) {
		assertTrue(con->commit());
		stdoutput.printf("loop %d...\n",i);
		bool	success=cur->sendQuery(
				"select * from testtable1 order by testint");
		if (!success) {
			assertTrue(success);
		}
		rowcount=cur->rowCount();
		if (rowcount==8) {
			break;
		}
		snooze::macrosnooze(1,0);
	}
	stdoutput.printf("\n");


	// row count
	stdoutput.printf("ROW COUNT: \n");
	assertEquals(rowcount,8);
	stdoutput.printf("\n");


	// column count
	stdoutput.printf("COLUMN COUNT: \n");
	assertEquals(cur->colCount(),9);
	stdoutput.printf("\n");


	// column names
	stdoutput.printf("COLUMN NAMES: \n");
	assertEquals(cur->getColumnName(0),"testint");
	assertEquals(cur->getColumnName(1),"testfloat");
	assertEquals(cur->getColumnName(2),"testreal");
	assertEquals(cur->getColumnName(3),"testsmallint");
	assertEquals(cur->getColumnName(4),"testchar");
	assertEquals(cur->getColumnName(5),"testvarchar");
	assertEquals(cur->getColumnName(6),"testdate");
	assertEquals(cur->getColumnName(7),"testtime");
	assertEquals(cur->getColumnName(8),"testtimestamp");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"testint");
	assertEquals(cols[1],"testfloat");
	assertEquals(cols[2],"testreal");
	assertEquals(cols[3],"testsmallint");
	assertEquals(cols[4],"testchar");
	assertEquals(cols[5],"testvarchar");
	assertEquals(cols[6],"testdate");
	assertEquals(cols[7],"testtime");
	assertEquals(cols[8],"testtimestamp");
	stdoutput.printf("\n");


	// column types
	stdoutput.printf("COLUMN TYPES: \n");
	assertEquals(cur->getColumnType((uint32_t)0),"INT");
	assertEquals(cur->getColumnType("testint"),"INT");
	assertEquals(cur->getColumnType(1),"FLOAT");
	assertEquals(cur->getColumnType("testfloat"),"FLOAT");
	assertEquals(cur->getColumnType(2),"REAL");
	assertEquals(cur->getColumnType("testreal"),"REAL");
	assertEquals(cur->getColumnType(3),"SMALLINT");
	assertEquals(cur->getColumnType("testsmallint"),"SMALLINT");
	if (majorversion==3) {
		assertEquals(cur->getColumnType(4),"VARSTRING");
		assertEquals(cur->getColumnType("testchar"),"VARSTRING");
	} else {
		assertEquals(cur->getColumnType(4),"STRING");
		assertEquals(cur->getColumnType("testchar"),"STRING");
	}
	assertEquals(cur->getColumnType(5),"VARSTRING");
	assertEquals(cur->getColumnType("testvarchar"),"VARSTRING");
	assertEquals(cur->getColumnType(6),"DATE");
	assertEquals(cur->getColumnType("testdate"),"DATE");
	assertEquals(cur->getColumnType(7),"TIME");
	assertEquals(cur->getColumnType("testtime"),"TIME");
	assertEquals(cur->getColumnType(8),"TIMESTAMP");
	assertEquals(cur->getColumnType("testtimestamp"),"TIMESTAMP");
	stdoutput.printf("\n");


	// column length
	stdoutput.printf("COLUMN LENGTH: \n");
	assertEquals(cur->getColumnLength((uint32_t)0),4);
	assertEquals(cur->getColumnLength("testint"),4);
	assertEquals(cur->getColumnLength(1),4);
	assertEquals(cur->getColumnLength("testfloat"),4);
	assertEquals(cur->getColumnLength(2),8);
	assertEquals(cur->getColumnLength("testreal"),8);
	assertEquals(cur->getColumnLength(3),2);
	assertEquals(cur->getColumnLength("testsmallint"),2);
	// testchar/testvarchar are char(40)/varchar(40); the mysql connection
	// charset is latin1 (1 byte/char) so the lengths are 40/41
	// mysql 3 silently converts char to varchar, adding a length byte,
	// which is why it reports testchar as VARSTRING above
	if (majorversion==3) {
		assertEquals(cur->getColumnLength(4),41);
		assertEquals(cur->getColumnLength("testchar"),41);
	} else {
		assertEquals(cur->getColumnLength(4),40);
		assertEquals(cur->getColumnLength("testchar"),40);
	}
	assertEquals(cur->getColumnLength(5),41);
	assertEquals(cur->getColumnLength("testvarchar"),41);
	assertEquals(cur->getColumnLength(6),3);
	assertEquals(cur->getColumnLength("testdate"),3);
	assertEquals(cur->getColumnLength(7),3);
	assertEquals(cur->getColumnLength("testtime"),3);
	assertEquals(cur->getColumnLength(8),4);
	assertEquals(cur->getColumnLength("testtimestamp"),4);
	stdoutput.printf("\n");


	// longest column
	stdoutput.printf("LONGEST COLUMN: \n");
	assertEquals(cur->getLongest((uint32_t)0),1);
	assertEquals(cur->getLongest("testint"),1);
	assertEquals(cur->getLongest(1),3);
	assertEquals(cur->getLongest("testfloat"),3);
	assertEquals(cur->getLongest(2),3);
	assertEquals(cur->getLongest("testreal"),3);
	assertEquals(cur->getLongest(3),1);
	assertEquals(cur->getLongest("testsmallint"),1);
	assertEquals(cur->getLongest(4),9);
	assertEquals(cur->getLongest("testchar"),9);
	assertEquals(cur->getLongest(5),12);
	assertEquals(cur->getLongest("testvarchar"),12);
	assertEquals(cur->getLongest(6),10);
	assertEquals(cur->getLongest("testdate"),10);
	assertEquals(cur->getLongest(7),8);
	assertEquals(cur->getLongest("testtime"),8);
	stdoutput.printf("\n");


	// total rows
	stdoutput.printf("TOTAL ROWS: \n");
	assertEquals(cur->totalRows(),8);
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
	assertEquals(cur->getField(0,1),"1.5");
	assertEquals(cur->getField(0,2),"1.5");
	assertEquals(cur->getField(0,3),"1");
	assertEquals(cur->getField(0,4),"testchar1");
	assertEquals(cur->getField(0,5),"testvarchar1");
	assertEquals(cur->getField(0,6),"2001-01-01");
	assertEquals(cur->getField(0,7),"01:00:00");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(7,1),"8.5");
	assertEquals(cur->getField(7,2),"8.5");
	assertEquals(cur->getField(7,3),"8");
	assertEquals(cur->getField(7,4),"testchar8");
	assertEquals(cur->getField(7,5),"testvarchar8");
	assertEquals(cur->getField(7,6),"2008-01-01");
	assertEquals(cur->getField(7,7),"08:00:00");
	stdoutput.printf("\n");


	// field lengths by index
	stdoutput.printf("FIELD LENGTHS BY INDEX: \n");
	assertEquals(cur->getFieldLength(0,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(0,1),3);
	assertEquals(cur->getFieldLength(0,2),3);
	assertEquals(cur->getFieldLength(0,3),1);
	assertEquals(cur->getFieldLength(0,4),9);
	assertEquals(cur->getFieldLength(0,5),12);
	assertEquals(cur->getFieldLength(0,6),10);
	assertEquals(cur->getFieldLength(0,7),8);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(7,1),3);
	assertEquals(cur->getFieldLength(7,2),3);
	assertEquals(cur->getFieldLength(7,3),1);
	assertEquals(cur->getFieldLength(7,4),9);
	assertEquals(cur->getFieldLength(7,5),12);
	assertEquals(cur->getFieldLength(7,6),10);
	assertEquals(cur->getFieldLength(7,7),8);
	stdoutput.printf("\n");


	// fields by name
	stdoutput.printf("FIELDS BY NAME: \n");
	assertEquals(cur->getField(0,"testint"),"1");
	assertEquals(cur->getField(0,"testfloat"),"1.5");
	assertEquals(cur->getField(0,"testreal"),"1.5");
	assertEquals(cur->getField(0,"testsmallint"),"1");
	assertEquals(cur->getField(0,"testchar"),"testchar1");
	assertEquals(cur->getField(0,"testvarchar"),"testvarchar1");
	assertEquals(cur->getField(0,"testdate"),"2001-01-01");
	assertEquals(cur->getField(0,"testtime"),"01:00:00");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,"testint"),"8");
	assertEquals(cur->getField(7,"testfloat"),"8.5");
	assertEquals(cur->getField(7,"testreal"),"8.5");
	assertEquals(cur->getField(7,"testsmallint"),"8");
	assertEquals(cur->getField(7,"testchar"),"testchar8");
	assertEquals(cur->getField(7,"testvarchar"),"testvarchar8");
	assertEquals(cur->getField(7,"testdate"),"2008-01-01");
	assertEquals(cur->getField(7,"testtime"),"08:00:00");
	stdoutput.printf("\n");


	// field lengths by name
	stdoutput.printf("FIELD LENGTHS BY NAME: \n");
	assertEquals(cur->getFieldLength(0,"testint"),1);
	assertEquals(cur->getFieldLength(0,"testfloat"),3);
	assertEquals(cur->getFieldLength(0,"testreal"),3);
	assertEquals(cur->getFieldLength(0,"testsmallint"),1);
	assertEquals(cur->getFieldLength(0,"testchar"),9);
	assertEquals(cur->getFieldLength(0,"testvarchar"),12);
	assertEquals(cur->getFieldLength(0,"testdate"),10);
	assertEquals(cur->getFieldLength(0,"testtime"),8);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,"testint"),1);
	assertEquals(cur->getFieldLength(7,"testfloat"),3);
	assertEquals(cur->getFieldLength(7,"testreal"),3);
	assertEquals(cur->getFieldLength(7,"testsmallint"),1);
	assertEquals(cur->getFieldLength(7,"testchar"),9);
	assertEquals(cur->getFieldLength(7,"testvarchar"),12);
	assertEquals(cur->getFieldLength(7,"testdate"),10);
	assertEquals(cur->getFieldLength(7,"testtime"),8);
	stdoutput.printf("\n");


	// fields by array
	stdoutput.printf("FIELDS BY ARRAY: \n");
	fields=cur->getRow(0);
	assertEquals(fields[0],"1");
	assertEquals(fields[1],"1.5");
	assertEquals(fields[2],"1.5");
	assertEquals(fields[3],"1");
	assertEquals(fields[4],"testchar1");
	assertEquals(fields[5],"testvarchar1");
	assertEquals(fields[6],"2001-01-01");
	assertEquals(fields[7],"01:00:00");
	stdoutput.printf("\n");


	// field lengths by array
	stdoutput.printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=cur->getRowLengths(0);
	assertEquals(fieldlens[0],1);
	assertEquals(fieldlens[1],3);
	assertEquals(fieldlens[2],3);
	assertEquals(fieldlens[3],1);
	assertEquals(fieldlens[4],9);
	assertEquals(fieldlens[5],12);
	assertEquals(fieldlens[6],10);
	assertEquals(fieldlens[7],8);
	stdoutput.printf("\n");


	// commit
	stdoutput.printf("COMMIT: \n");
	secondcon=new sqlrconnection("sqlrelay",9015,"/tmp/router.socket",
						"testuser","testpassword",0,1);
	secondcur=new sqlrcursor(secondcon);
	// It may take some time for the replication to actually occur.
	// Exactly how long depends on how busy everything is.  So, loop
	// until we get the values that we're looking for, or fail 10 times.
	const char	*val="";
	for (uint16_t i=0; i<10; i++) {
		assertTrue(con->commit());
		assertTrue(secondcon->commit());
		stdoutput.printf("loop %d...\n",i);
		bool	success=secondcur->sendQuery(
					"select count(*) from testtable1");
		if (!success) {
			assertTrue(success);
		}
		val=secondcur->getField(0,(uint32_t)0);
		if (!charstring::compare(val,"8")) {
			break;
		}
		snooze::macrosnooze(1,0);
	}
	for (uint16_t i=0; i<10; i++) {
		assertTrue(con->commit());
		assertTrue(secondcon->commit());
		stdoutput.printf("loop %d...\n",i);
		bool	success=secondcur->sendQuery(
					"select count(*) from testtable2");
		if (!success) {
			assertTrue(success);
		}
		val=secondcur->getField(0,(uint32_t)0);
		if (!charstring::compare(val,"8")) {
			break;
		}
		snooze::macrosnooze(1,0);
	}
	assertTrue(secondcur->sendQuery("select count(*) from testtable1"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"8");
	assertTrue(secondcur->sendQuery("select count(*) from testtable2"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"8");
	assertTrue(con->autoCommitOn());
	assertTrue(secondcon->autoCommitOn());
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable1 "
		"values ("
		"	10, "
		"	10.5, "
		"	10.5, "
		"	10, "
		"	'testchar10', "
		"	'testvarchar10', "
		"	'2010-01-01', "
		"	'10:00:00', "
		"	NULL)"));
	assertTrue(cur->sendQuery(
		"insert into "
		"	testtable2 "
		"values ("
		"	10, "
		"	10.5, "
		"	10.5, "
		"	10, "
		"	'testchar10', "
		"	'testvarchar10', "
		"	'2010-01-01', "
		"	'10:00:00', "
		"	NULL)"));
	// Same routine as above, but we won't commit each time because
	// autocommit is enabled.
	val="";
	for (uint16_t i=0; i<10; i++) {
		stdoutput.printf("loop %d...\n",i);
		bool	success=secondcur->sendQuery(
					"select count(*) from testtable1");
		if (!success) {
			assertTrue(success);
		}
		val=secondcur->getField(0,(uint32_t)0);
		if (!charstring::compare(val,"9")) {
			break;
		}
		snooze::macrosnooze(1,0);
	}
	for (uint16_t i=0; i<10; i++) {
		stdoutput.printf("loop %d...\n",i);
		bool	success=secondcur->sendQuery(
					"select count(*) from testtable2");
		if (!success) {
			assertTrue(success);
		}
		val=secondcur->getField(0,(uint32_t)0);
		if (!charstring::compare(val,"9")) {
			break;
		}
		snooze::macrosnooze(1,0);
	}
	assertTrue(secondcur->sendQuery("select count(*) from testtable1"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"9");
	assertTrue(secondcur->sendQuery("select count(*) from testtable2"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"9");
	assertTrue(con->autoCommitOff());
	assertTrue(secondcon->autoCommitOff());
	assertTrue(cur->sendQuery("begin"));
	stdoutput.printf("\n");

	// drop existing table
	cur->sendQuery("drop table testtable1");
	cur->sendQuery("drop table testtable2");

	stdoutput.printf("\n");

	reportTestStatus();

	return status;
}
