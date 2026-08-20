// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>
#include <rudiments/process.h>
#include <rudiments/datetime.h>
#include <rudiments/signalclasses.h>
#include <sqlrelay/sqlrclient.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../c++/asserts.cpp"

sqlrconnection	*con=NULL;
sqlrcursor	*cur=NULL;
sqlrconnection	*secondcon=NULL;
sqlrcursor	*secondcur=NULL;

int main(int argc, char **argv) {

	// The extensions instance uses connections="0", so the
	// sqlr-connection process exits after each client session.  When the
	// client sends its final endSession(), and the server can receive it,
	// process it, and exit, before the client's final write() system call
	// returns, causing the client to receive a SIGPIPE.  It all depends on
	// timing though, and doesn't happen every time.  We'll ignore SIGPIPE
	// here to manage this.
	#ifdef SIGPIPE
	signalset	set;
	set.removeAllSignals();
	set.addSignal(SIGPIPE);
	signalmanager::ignoreSignals(&set);
	#endif

	// instantiation
	con=new sqlrconnection("sqlrelay",9014,"/tmp/extensionstest.socket",
							"test","test",0,1);
	cur=new sqlrcursor(con);

	con->setClientInfo("extensionstest");


	// ignore select database
	stdoutput.printf("IGNORE SELECT DATABASE: \n");
	char	*originaldb=charstring::duplicate(con->getCurrentDatabase());
	assertEquals((originaldb!=NULL),true);
	assertEquals(con->selectDatabase("nonexistentdb"),true);
	assertEquals(con->getCurrentDatabase(),originaldb);
	delete[] originaldb;
	stdoutput.printf("\n");


	// translate bind variables
	stdoutput.printf("TRANSLATE BIND VARIABLES: \n");
	cur->prepareQuery(
		"select "
		"	:1 "
		"from "
		"	dual "
		"where "
		"	'hel''lo'='hel''lo' and 1=:2 and 2=:3 ");
	cur->validateBinds();
	cur->inputBind("1","hello");
	cur->inputBind("2",1);
	cur->inputBind("3",2);
	assertEquals(cur->validBind("1"),true);
	assertEquals(cur->validBind("2"),true);
	assertEquals(cur->validBind("3"),true);
	assertEquals(cur->validBind("4"),false);
	assertEquals(cur->countBindVariables(),3);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"hello");
	cur->clearBinds();
	stdoutput.printf("\n");

	cur->prepareQuery(
		"select "
		"	@1 "
		"from "
		"	dual "
		"where "
		"	'hel''lo'='hel''lo' and 1=@2 and 2=@3 ");
	cur->validateBinds();
	cur->inputBind("1","hello");
	cur->inputBind("2",1);
	cur->inputBind("3",2);
	assertEquals(cur->validBind("1"),true);
	assertEquals(cur->validBind("2"),true);
	assertEquals(cur->validBind("3"),true);
	assertEquals(cur->validBind("4"),false);
	assertEquals(cur->countBindVariables(),3);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"hello");
	cur->clearBinds();
	stdoutput.printf("\n");

	cur->prepareQuery(
		"select "
		"	$1 "
		"from "
		"	dual "
		"where "
		"	'hel''lo'='hel''lo' and 1=$2 and 2=$3 ");
	cur->validateBinds();
	cur->inputBind("1","hello");
	cur->inputBind("2",1);
	cur->inputBind("3",2);
	assertEquals(cur->validBind("1"),true);
	assertEquals(cur->validBind("2"),true);
	assertEquals(cur->validBind("3"),true);
	assertEquals(cur->validBind("4"),false);
	assertEquals(cur->countBindVariables(),3);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"hello");
	cur->clearBinds();
	stdoutput.printf("\n");

	cur->prepareQuery(
		"select "
		"	? "
		"from "
		"	dual "
		"where "
		"	'hel''lo'='hel''lo' and 1=? and 2=? ");
	cur->validateBinds();
	cur->inputBind("1","hello");
	cur->inputBind("2",1);
	cur->inputBind("3",2);
	assertEquals(cur->validBind("1"),true);
	assertEquals(cur->validBind("2"),true);
	assertEquals(cur->validBind("3"),true);
	assertEquals(cur->validBind("4"),false);
	assertEquals(cur->countBindVariables(),3);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"hello");
	cur->clearBinds();
	stdoutput.printf("\n");


	// fake input bind variables
	stdoutput.printf("FAKE INPUT BIND VARIABLES: \n");
	cur->prepareQuery(
		"select "
		"	'', "
		"	1, "
		"	'', "
		"	:hello, "
		"	'''', "
		"	'\\'' "
		"from "
		"	dual "
		"where "
		"	1=:one ");
	cur->inputBind("hello","hello");
	cur->inputBind("one","1");
	cur->inputBind("nonexistentvar","nonexistentval");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"");
	assertEquals(cur->getField(0,(uint32_t)1),"1");
	assertEquals(cur->getField(0,(uint32_t)2),"");
	assertEquals(cur->getField(0,(uint32_t)3),"hello");
	assertEquals(cur->getField(0,(uint32_t)4),"'");
	assertEquals(cur->getField(0,(uint32_t)5),"'");
	stdoutput.printf("\n");


	// isolation levels
	stdoutput.printf("ISOLATION LEVELS: \n");

	// set autocommit off
	assertTrue(con->autoCommitOff());

	// create a table
	cur->sendQuery("drop table testtable");
	assertTrue(cur->sendQuery("create table testtable (col1 int)"));

	// open a second connection and set autocommit off there too
	secondcon=new sqlrconnection("sqlrelay",9014,"/tmp/extensionstest.socket",
							"test","test",0,1);
	secondcur=new sqlrcursor(secondcon);
	assertTrue(secondcon->autoCommitOff());

	// change the isolation level
	assertTrue(secondcur->sendQuery(
		"alter session "
		"	set isolation_level=serializable"));
	stdoutput.printf("\n");

	// in the second connection, select from the table, it should be empty
	assertTrue(secondcur->sendQuery("select * from testtable"));
	assertEquals(secondcur->rowCount(),0);

	// in the first connection, insert a row into the table
	assertTrue(cur->sendQuery("insert into testtable values (1)"));

	// in the second connection, select again, it should still be empty
	assertTrue(secondcur->sendQuery("select * from testtable"));
	assertEquals(secondcur->rowCount(),0);

	// in the first connecton, commit
	assertTrue(con->commit());
	stdoutput.printf("\n");

	// in the second connection, select again, it should STILL be empty
	assertTrue(secondcur->sendQuery("select * from testtable"));
	assertEquals(secondcur->rowCount(),0);

	// end the second connections sesssion and select again,
	// finally it should see the row
	secondcon->endSession();
	assertTrue(secondcur->sendQuery("select * from testtable"));
	assertEquals(secondcur->rowCount(),1);

	// clean up
	delete secondcur;
	secondcur=NULL;
	delete secondcon;
	secondcon=NULL;
	delete cur;
	delete con;
	con=new sqlrconnection("sqlrelay",9014,"/tmp/extensionstest.socket",
							"test","test",0,1);
	cur=new sqlrcursor(con);
	assertTrue(cur->sendQuery("drop table testtable"));
	con->setClientInfo("extensionstest");
	stdoutput.printf("\n");


	// sqlrcmd cstat
	stdoutput.printf("SQLRCMD CSTAT: \n");
	assertTrue(cur->sendQuery("sqlrcmd cstat"));
	assertEquals(cur->colCount(),9);
	stdoutput.printf("\n");

	assertEquals(cur->getColumnName((uint32_t)0),"INDEX");
	assertEquals(cur->getColumnName(1),"MINE");
	assertEquals(cur->getColumnName(2),"PROCESSID");
	assertEquals(cur->getColumnName(3),"CONNECT");
	assertEquals(cur->getColumnName(4),"STATE");
	assertEquals(cur->getColumnName(5),"STATE_TIME");
	assertEquals(cur->getColumnName(6),"CLIENT_ADDR");
	assertEquals(cur->getColumnName(7),"CLIENT_INFO");
	assertEquals(cur->getColumnName(8),"SQL_TEXT");
	stdoutput.printf("\n");

	assertEquals(cur->getColumnType((uint32_t)0),"NUMBER");
	assertEquals(cur->getColumnType(1),"VARCHAR2");
	assertEquals(cur->getColumnType(2),"NUMBER");
	assertEquals(cur->getColumnType(3),"NUMBER");
	assertEquals(cur->getColumnType(4),"VARCHAR2");
	assertEquals(cur->getColumnType(5),"NUMBER");
	assertEquals(cur->getColumnType(6),"VARCHAR2");
	assertEquals(cur->getColumnType(7),"VARCHAR2");
	assertEquals(cur->getColumnType(8),"VARCHAR2");
	stdoutput.printf("\n");

	assertEquals(cur->getColumnLength((uint32_t)0),10);
	assertEquals(cur->getColumnLength(1),1);
	assertEquals(cur->getColumnLength(2),10);
	assertEquals(cur->getColumnLength(3),12);
	assertEquals(cur->getColumnLength(4),25);
	assertEquals(cur->getColumnLength(5),12);
	assertEquals(cur->getColumnLength(6),24);
	assertEquals(cur->getColumnLength(7),511);
	assertEquals(cur->getColumnLength(8),511);
	stdoutput.printf("\n");

	assertEquals(cur->getColumnPrecision((uint32_t)0),10);
	assertEquals(cur->getColumnPrecision(1),0);
	assertEquals(cur->getColumnPrecision(2),10);
	assertEquals(cur->getColumnPrecision(3),12);
	assertEquals(cur->getColumnPrecision(4),0);
	assertEquals(cur->getColumnPrecision(5),12);
	assertEquals(cur->getColumnPrecision(6),0);
	assertEquals(cur->getColumnPrecision(7),0);
	assertEquals(cur->getColumnPrecision(8),0);
	stdoutput.printf("\n");

	assertEquals(cur->getColumnScale((uint32_t)0),0);
	assertEquals(cur->getColumnScale(1),0);
	assertEquals(cur->getColumnScale(2),0);
	assertEquals(cur->getColumnScale(3),0);
	assertEquals(cur->getColumnScale(4),0);
	assertEquals(cur->getColumnScale(5),2);
	assertEquals(cur->getColumnScale(6),0);
	assertEquals(cur->getColumnScale(7),0);
	assertEquals(cur->getColumnScale(8),0);
	stdoutput.printf("\n");

	assertFalse(cur->getColumnIsNullable((uint32_t)0));
	assertFalse(cur->getColumnIsNullable(1));
	assertFalse(cur->getColumnIsNullable(2));
	assertFalse(cur->getColumnIsNullable(3));
	assertFalse(cur->getColumnIsNullable(4));
	assertFalse(cur->getColumnIsNullable(5));
	assertFalse(cur->getColumnIsNullable(6));
	assertTrue(cur->getColumnIsNullable(7));
	assertTrue(cur->getColumnIsNullable(8));
	stdoutput.printf("\n");

	uint64_t	row=0;
	bool		found=false;
	for (uint64_t i=0; i<cur->rowCount(); i++) {
		if (!charstring::compare(cur->getField(i,(uint32_t)8),
							"sqlrcmd cstat")) {
			found=true;
			row=i;
			break;
		}
	}
	assertEquals(found,true);
	assertEquals(cur->getField(row,(uint32_t)1),"*");
	assertEquals(cur->getField(row,(uint32_t)4),"RETURN_RESULT_SET");
	// 127.0.0.1 on Windows
	//assertEquals(cur->getField(row,(uint32_t)6),"UNIX");
	assertEquals(cur->getField(row,(uint32_t)7),"extensionstest");
	assertEquals(cur->getField(row,(uint32_t)8),"sqlrcmd cstat");
	stdoutput.printf("\n");


	// sqlrcmd gstat
	stdoutput.printf("SQLRCMD GSTAT: \n");
	assertTrue(cur->sendQuery("sqlrcmd gstat"));

	assertEquals(cur->colCount(),2);

	assertEquals(cur->getColumnName((uint32_t)0),"KEY");
	assertEquals(cur->getColumnName(1),"VALUE");

	assertEquals(cur->getColumnType((uint32_t)0),"VARCHAR2");
	assertEquals(cur->getColumnType(1),"VARCHAR2");

	assertEquals(cur->getColumnLength((uint32_t)0),40);
	assertEquals(cur->getColumnLength(1),40);

	assertEquals(cur->getField(0,(uint32_t)0),"start");
	assertEquals(cur->getField(1,(uint32_t)0),"uptime");
	assertEquals(cur->getField(2,(uint32_t)0),"now");
	assertEquals(cur->getField(3,(uint32_t)0),"access_count");
	assertEquals(cur->getField(4,(uint32_t)0),"query_total");
	assertEquals(cur->getField(5,(uint32_t)0),"qpm");
	assertEquals(cur->getField(6,(uint32_t)0),"qpm_1");
	assertEquals(cur->getField(7,(uint32_t)0),"qpm_5");
	assertEquals(cur->getField(8,(uint32_t)0),"qpm_15");
	assertEquals(cur->getField(9,(uint32_t)0),"select_1");
	assertEquals(cur->getField(10,(uint32_t)0),"select_5");
	assertEquals(cur->getField(11,(uint32_t)0),"select_15");
	assertEquals(cur->getField(12,(uint32_t)0),"insert_1");
	assertEquals(cur->getField(13,(uint32_t)0),"insert_5");
	assertEquals(cur->getField(14,(uint32_t)0),"insert_15");
	assertEquals(cur->getField(15,(uint32_t)0),"update_1");
	assertEquals(cur->getField(16,(uint32_t)0),"update_5");
	assertEquals(cur->getField(17,(uint32_t)0),"update_15");
	assertEquals(cur->getField(18,(uint32_t)0),"delete_1");
	assertEquals(cur->getField(19,(uint32_t)0),"delete_5");
	assertEquals(cur->getField(20,(uint32_t)0),"delete_15");
	assertEquals(cur->getField(21,(uint32_t)0),"etc_1");
	assertEquals(cur->getField(22,(uint32_t)0),"etc_5");
	assertEquals(cur->getField(23,(uint32_t)0),"etc_15");
	assertEquals(cur->getField(24,(uint32_t)0),"sqlrcmd_1");
	assertEquals(cur->getField(25,(uint32_t)0),"sqlrcmd_5");
	assertEquals(cur->getField(26,(uint32_t)0),"sqlrcmd_15");
	assertEquals(cur->getField(27,(uint32_t)0),"max_listener");
	assertEquals(cur->getField(28,(uint32_t)0),"max_listener_error");
	assertEquals(cur->getField(29,(uint32_t)0),"busy_listener");
	assertEquals(cur->getField(30,(uint32_t)0),"peak_listener");
	assertEquals(cur->getField(31,(uint32_t)0),"connection");
	assertEquals(cur->getField(32,(uint32_t)0),"session");
	assertEquals(cur->getField(33,(uint32_t)0),"peak_session");
	assertEquals(cur->getField(34,(uint32_t)0),"peak_session_1min");
	assertEquals(cur->getField(35,(uint32_t)0),"peak_session_1min_time");
	stdoutput.printf("\n");


	// session queries: date format
	stdoutput.printf("SESSION QUERIES: Date Format\n");
	assertTrue(cur->sendQuery("select sysdate from dual"));
	datetime	dt;
	dt.initFromSystemDateTime();
	const char	*field=cur->getField(0,(uint32_t)0);
	char	*day=charstring::getSubString(field,0,1);
	char	*month=charstring::getSubString(field,3,4);
	char	*year=charstring::getSubString(field,6,9);
	char	*hour=charstring::getSubString(field,11,12);
	char	*minute=charstring::getSubString(field,14,15);
	assertEquals((int)charstring::convertToInteger(day),(int)dt.getDayOfMonth());
	assertEquals((int)charstring::convertToInteger(month),(int)dt.getMonth());
	assertEquals((int)charstring::convertToInteger(year),(int)dt.getYear());
	assertEquals((int)charstring::convertToInteger(hour),(int)dt.getHour());
	int	dbmin=(int)charstring::convertToInteger(minute);
	int	min=(int)dt.getMinute();
	bool	success=((dbmin==min) || (dbmin==min-1) || (dbmin-1==min));
	assertTrue(success);
	delete[] year;
	delete[] day;
	delete[] month;
	delete[] hour;
	delete[] minute;
	stdoutput.printf("\n");


	// filters
	stdoutput.printf("FILTERS: \n");
	assertFalse(cur->sendQuery("select * from badstring"));
	assertEquals(cur->errorMessage(),"badstring encountered");
	assertFalse(cur->sendQuery("select * from badregex"));
	assertEquals(cur->errorMessage(),"badregex encountered");
	assertEquals(cur->errorNumber(),100);
	assertFalse(cur->sendQuery("select * from badpattern"));
	stdoutput.printf("\n");

	delete cur;
	delete con;


	// pwdencs
	stdoutput.printf("PWDENCS: \n");
	const char	*usrpwds[]={
		"test",
		"rot16test",
		"rot13test",
		"rot10test",
		"md5test",
		"sha1test",
		"sha256test",
		"crypttest",
		"aes128test",
		NULL
	};
	for (const char **usrpwd=usrpwds; *usrpwd; usrpwd++) {
		con=new sqlrconnection("sqlrelay",9014,"/tmp/extensionstest.socket",
							*usrpwd,*usrpwd,0,1);
		cur=new sqlrcursor(con);
		assertTrue(cur->sendQuery("select 1 from dual"));
		stdoutput.printf("\n");
		delete cur;
		delete con;
	}
	stdoutput.printf("\n");


	// upsert
	stdoutput.printf("UPSERT: \n");
	con=new sqlrconnection("sqlrelay",9014,"/tmp/extensionstest.socket",
							"test","test",0,1);
	cur=new sqlrcursor(con);
	secondcur=new sqlrcursor(con);
	cur->sendQuery("drop table student");
	cur->sendQuery("drop sequence student_id");
	assertTrue(cur->sendQuery("create sequence student_id"));
	assertTrue(cur->sendQuery("create table student ("
					"id int, "
					"firstname varchar(20), "
					"lastname varchar(20), "
					"year varchar(20), "
					"major varchar(20), "
					"gpa varchar(20), "
					"primary key (id), "
					"unique (firstname,lastname) "
					")"));
	stdoutput.printf("\n");
	// initial insert
	assertTrue(cur->sendQuery("insert into student values "
				"(student_id.nextval,"
				"'David','Muse','Freshman','ME','4.0')"));
	assertTrue(secondcur->sendQuery("select count(*) from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertTrue(secondcur->sendQuery("select * from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertEquals(secondcur->getField(0,1),"David");
	assertEquals(secondcur->getField(0,2),"Muse");
	assertEquals(secondcur->getField(0,3),"Freshman");
	assertEquals(secondcur->getField(0,4),"ME");
	assertEquals(secondcur->getField(0,5),"4.0");
	stdoutput.printf("\n");
	// should be converted to an update
	assertTrue(cur->sendQuery("insert into student values "
				"(student_id.nextval,"
				"'David','Muse','Sophomore','ME','3.5')"));
	assertTrue(secondcur->sendQuery("select count(*) from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertTrue(secondcur->sendQuery("select * from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertEquals(secondcur->getField(0,1),"David");
	assertEquals(secondcur->getField(0,2),"Muse");
	assertEquals(secondcur->getField(0,3),"Sophomore");
	assertEquals(secondcur->getField(0,4),"ME");
	assertEquals(secondcur->getField(0,5),"3.5");
	stdoutput.printf("\n");
	// with bind variables, should also be converted to an update
	cur->prepareQuery("insert into student values "
				"(student_id.nextval,"
				":firstname,:lastname,:year,:major,:gpa)");
	cur->inputBind("firstname","David");
	cur->inputBind("lastname","Muse");
	cur->inputBind("year","Junior");
	cur->inputBind("major","CS");
	cur->inputBind("gpa","3.0");
	assertTrue(cur->executeQuery());
	assertTrue(secondcur->sendQuery("select count(*) from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertTrue(secondcur->sendQuery("select * from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertEquals(secondcur->getField(0,1),"David");
	assertEquals(secondcur->getField(0,2),"Muse");
	assertEquals(secondcur->getField(0,3),"Junior");
	assertEquals(secondcur->getField(0,4),"CS");
	assertEquals(secondcur->getField(0,5),"3.0");
	stdoutput.printf("\n");
	// reexecute with bind variables, should also be converted to an update
	cur->inputBind("firstname","David");
	cur->inputBind("lastname","Muse");
	cur->inputBind("year","Senior");
	cur->inputBind("major","CS");
	cur->inputBind("gpa","2.5");
	assertTrue(cur->executeQuery());
	assertTrue(secondcur->sendQuery("select count(*) from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertTrue(secondcur->sendQuery("select * from student"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"1");
	assertEquals(secondcur->getField(0,1),"David");
	assertEquals(secondcur->getField(0,2),"Muse");
	assertEquals(secondcur->getField(0,3),"Senior");
	assertEquals(secondcur->getField(0,4),"CS");
	assertEquals(secondcur->getField(0,5),"2.5");
	stdoutput.printf("\n");
	assertTrue(cur->sendQuery("drop table student"));
	assertTrue(cur->sendQuery("drop sequence student_id"));
	delete secondcur;
	secondcur=NULL;
	stdoutput.printf("\n");


	// error translation
	stdoutput.printf("ERROR TRANSLATION: \n");
	assertFalse(cur->sendQuery("select 1"));
	assertEquals(cur->errorNumber(),10923);
	assertStartsWith(cur->errorMessage(),
			"ORA-10923: fRoM kEyWoRd nOt fOuNd wHeRe eXpEcTeD"
			" (code ORA-10923)");
	stdoutput.printf("\n");

	delete cur;
	delete con;

	reportTestStatus();

	return status;
}
