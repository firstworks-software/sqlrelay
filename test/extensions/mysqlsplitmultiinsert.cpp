// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclient.h>
#include <rudiments/process.h>
#include <rudiments/bytestring.h>
#include <rudiments/stdio.h>

#include "../c++/asserts.cpp"

sqlrconnection	*con=NULL;
sqlrcursor	*cur=NULL;
sqlrconnection	*secondcon=NULL;
sqlrcursor	*secondcur=NULL;

int main(int argc, char **argv) {

	// instantiation
	con=new sqlrconnection("sqlrelay",9018,"/tmp/mysqlsplitmultiinsert.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);

	// create testtable
	// #10122: `testvar)char` is a column whose own name contains a
	// close-paren
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
		"	testtimestamp timestamp, "
		"	`testvar)char` varchar(40))"));
	stdoutput.printf("\n");


	// insert
	stdoutput.printf("INSERT: \n");
	stringbuffer	multiinsert;
	multiinsert.append(
		"insert into testtable values "
		"(1,1,1,1,1,1.5,1.5,1.5,'2001-01-01','01:00:00','2001-01-01 01:00:00','2001','char1','varchar1','text1','tinytext1','mediumtext1','longtext1','blob1','tinyblob1','mediumblob1','longblob1',NULL,NULL),"
		"(2,2,2,2,2,2.5,2.5,2.5,'2002-01-01','02:00:00','2002-01-01 02:00:00','2002','char2','varchar2','text2','tinytext2','mediumtext2','longtext2','blob2','tinyblob2','mediumblob2','longblob2',NULL,NULL),"
		"(3,3,3,3,3,3.5,3.5,3.5,'2003-01-01','03:00:00','2003-01-01 03:00:00','2003','char3','varchar3','text3','tinytext3','mediumtext3','longtext3','blob3','tinyblob3','mediumblob3','longblob3',NULL,NULL),"
		"(4,4,4,4,4,4.5,4.5,4.5,'2004-01-01','04:00:00','2004-01-01 04:00:00','2004','char4','varchar4','text4','tinytext4','mediumtext4','longtext4','blob4','tinyblob4','mediumblob4','longblob4',NULL,NULL),"
		"(5,5,5,5,5,5.5,5.5,5.5,'2005-01-01','05:00:00','2005-01-01 05:00:00','2005','char5',\"v,a)l\",'text5','tinytext5','mediumtext5','longtext5','blob5','tinyblob5','mediumblob5','longblob5',NULL,NULL)");
	if (charstring::convertToInteger(con->dbVersion())>=5) {
		multiinsert.append(
			" on duplicate key update testtinyint=values(testtinyint)");
	}
	assertTrue(cur->sendQuery(multiinsert.getString()));
	stdoutput.printf("\n");


	// affected rows
	stdoutput.printf("AFFECTED ROWS: \n");
	assertEquals(cur->affectedRows(),5);
	stdoutput.printf("\n");


	// row count
	stdoutput.printf("ROW COUNT: \n");
	assertTrue(cur->sendQuery("select count(*) from testtable"));
	assertEquals(cur->getFieldAsInteger(0,(uint32_t)0),5);
	stdoutput.printf("\n");


	// verify the comma/close-paren embedded in the double-quoted
	// value above didn't get mistaken for a row separator
	stdoutput.printf("QUOTED COMMA/PAREN VALUE: \n");
	assertTrue(cur->sendQuery(
		"select testvarchar from testtable "
		"where testtinyint=5"));
	assertEquals(cur->getField((uint64_t)0,"testvarchar"),"v,a)l");
	stdoutput.printf("\n");


	// insert with columns
	// #10122: the column list names a backtick-quoted column whose own
	// name contains a close-paren, and it comes last, after the
	// timestamp column - a quote-blind column-list scan would stop at
	// the embedded close-paren instead of the real one, truncating the
	// column list and corrupting the rewritten single-inserts
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
		"testtimestamp,`testvar)char`) values "
		"(1,1,1,1,1,1.5,1.5,1.5,"
		"'2001-01-01','01:00:00',"
		"'2001-01-01 01:00:00','2001',"
		"'char1','varchar1',"
		"'text1','tinytext1',"
		"'mediumtext1','longtext1',"
		"'blob1','tinyblob1',"
		"'mediumblob1','longblob1',"
		"NULL,'v)1'),"
		"(2,2,2,2,2,2.5,2.5,2.5,"
		"'2002-01-01','02:00:00',"
		"'2002-01-01 02:00:00','2002',"
		"'char2','varchar2',"
		"'text2','tinytext2',"
		"'mediumtext2','longtext2',"
		"'blob2','tinyblob2',"
		"'mediumblob2','longblob2',"
		"NULL,'v)2'),"
		"(3,3,3,3,3,3.5,3.5,3.5,"
		"'2003-01-01','03:00:00',"
		"'2003-01-01 03:00:00','2003',"
		"'char3','varchar3',"
		"'text3','tinytext3',"
		"'mediumtext3','longtext3',"
		"'blob3','tinyblob3',"
		"'mediumblob3','longblob3',"
		"NULL,'v)3'),"
		"(4,4,4,4,4,4.5,4.5,4.5,"
		"'2004-01-01','04:00:00',"
		"'2004-01-01 04:00:00','2004',"
		"'char4','varchar4',"
		"'text4','tinytext4',"
		"'mediumtext4','longtext4',"
		"'blob4','tinyblob4',"
		"'mediumblob4','longblob4',"
		"NULL,'v)4')");
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
	assertEquals(cur->getFieldAsInteger(0,(uint32_t)0),9);
	stdoutput.printf("\n");


	// #10122: the quoted column's value must land on that column,
	// instead of the column list having been truncated at the embedded
	// close-paren and desynced from the values
	stdoutput.printf("QUOTED COLUMN NAME WITH EMBEDDED PAREN: \n");
	// (testtinyint=3 also matches a row from the plain "insert"
	// above, so narrow to the row that has this column set)
	assertTrue(cur->sendQuery(
		"select `testvar)char` from testtable "
		"where testtinyint=3 and testint=3 and "
		"`testvar)char` is not null"));
	assertEquals(cur->getField((uint64_t)0,"testvar)char"),"v)3");
	stdoutput.printf("\n");

	cur->sendQuery("drop table testtable");

	delete cur;
	delete con;

	reportTestStatus();

	return status;
}
