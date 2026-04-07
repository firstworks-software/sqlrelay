// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclient.h>
#include <rudiments/process.h>
#include <rudiments/bytestring.h>
#include <rudiments/stdio.h>

#include "asserts.cpp"

sqlrconnection	*con=NULL;
sqlrcursor	*cur=NULL;
sqlrconnection	*secondcon=NULL;
sqlrcursor	*secondcur=NULL;

int main(int argc, char **argv) {

	// instantiation
	con=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);

	// create testtable
	stdoutput.printf("CREATE TESTTABLE: \n");
	cur->sendQuery("drop table testtable");
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


	// insert
	stdoutput.printf("INSERT: \n");
	stringbuffer	multiinsert;
	multiinsert.append(
		"insert into testtable values "
		"(1,1,1,1,1,1.1,1.1,1.1,'2001-01-01','01:00:00','2001-01-01 01:00:00','2001','char1','varchar1','text1','tinytext1','mediumtext1','longtext1','blob1','tinyblob1','mediumblob1','longblob1',NULL),"
		"(2,2,2,2,2,2.1,2.1,2.1,'2002-01-01','02:00:00','2002-01-01 02:00:00','2002','char2','varchar2','text2','tinytext2','mediumtext2','longtext2','blob2','tinyblob2','mediumblob2','longblob2',NULL),"
		"(3,3,3,3,3,3.1,3.1,3.1,'2003-01-01','03:00:00','2003-01-01 03:00:00','2003','char3','varchar3','text3','tinytext3','mediumtext3','longtext3','blob3','tinyblob3','mediumblob3','longblob3',NULL),"
		"(4,4,4,4,4,4.1,4.1,4.1,'2004-01-01','04:00:00','2004-01-01 04:00:00','2004','char4','varchar4','text4','tinytext4','mediumtext4','longtext4','blob4','tinyblob4','mediumblob4','longblob4',NULL)");
	if (charstring::convertToInteger(con->dbVersion())>=5) {
		multiinsert.append(
			" on duplicate key update testtinyint=values(testtinyint)");
	}
	assertTrue(cur->sendQuery(multiinsert.getString()));
	stdoutput.printf("\n");


	// affected rows
	stdoutput.printf("AFFECTED ROWS: \n");
	assertEquals(cur->affectedRows(),4);
	stdoutput.printf("\n");


	// row count
	stdoutput.printf("ROW COUNT: \n");
	assertTrue(cur->sendQuery("select count(*) from testtable"));
	assertEquals(cur->getFieldAsInteger(0,(uint32_t)0),4);
	stdoutput.printf("\n");


	// insert with columns
	stdoutput.printf("INSERT WITH COLUMNS: \n");
	multiinsert.clear();
	multiinsert.append(
		"insert into testtable ("
		"testtinyint,testsmallint,"
		"testmediumint,testint,"
		"testbigint,"
		"testfloat,testreal,testdecimal,"
		"testdate,testtime,"
		"testdatetime,testyear,"
		"testchar,testvarchar,"
		"testtext,testtinytext,"
		"testmediumtext,testlongtext,"
		"testblob,testtinyblob,"
		"testmediumblob,testlongblob,"
		"testtimestamp) values "
		"(1,1,1,1,1,1.1,1.1,1.1,"
		"'2001-01-01','01:00:00',"
		"'2001-01-01 01:00:00','2001',"
		"'char1','varchar1',"
		"'text1','tinytext1',"
		"'mediumtext1','longtext1',"
		"'blob1','tinyblob1',"
		"'mediumblob1','longblob1',"
		"NULL),"
		"(2,2,2,2,2,2.1,2.1,2.1,"
		"'2002-01-01','02:00:00',"
		"'2002-01-01 02:00:00','2002',"
		"'char2','varchar2',"
		"'text2','tinytext2',"
		"'mediumtext2','longtext2',"
		"'blob2','tinyblob2',"
		"'mediumblob2','longblob2',"
		"NULL),"
		"(3,3,3,3,3,3.1,3.1,3.1,"
		"'2003-01-01','03:00:00',"
		"'2003-01-01 03:00:00','2003',"
		"'char3','varchar3',"
		"'text3','tinytext3',"
		"'mediumtext3','longtext3',"
		"'blob3','tinyblob3',"
		"'mediumblob3','longblob3',"
		"NULL),"
		"(4,4,4,4,4,4.1,4.1,4.1,"
		"'2004-01-01','04:00:00',"
		"'2004-01-01 04:00:00','2004',"
		"'char4','varchar4',"
		"'text4','tinytext4',"
		"'mediumtext4','longtext4',"
		"'blob4','tinyblob4',"
		"'mediumblob4','longblob4',"
		"NULL)");
	if (charstring::convertToInteger(con->dbVersion())>=5) {
		multiinsert.append(
			" on duplicate key update testtinyint=values(testtinyint)");
	}
	assertTrue(cur->sendQuery(multiinsert.getString()));
	stdoutput.printf("\n");


	// affected rows
	stdoutput.printf("AFFECTED ROWS: \n");
	assertEquals(cur->affectedRows(),4);
	stdoutput.printf("\n");


	// row count
	stdoutput.printf("ROW COUNT: \n");
	assertTrue(cur->sendQuery("select count(*) from testtable"));
	assertEquals(cur->getFieldAsInteger(0,(uint32_t)0),8);
	stdoutput.printf("\n");

	cur->sendQuery("drop table testtable");

	delete cur;
	delete con;

	reportTestStatus();

	return status;
}
