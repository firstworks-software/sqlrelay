// Copyright (c) David Muse
// See the file COPYING for more information.

#include "../../config.h"
#include <sqlrelay/sqlrclient.h>
#include <rudiments/charstring.h>
#include <rudiments/process.h>
#include <rudiments/stdio.h>

#include "assert.cpp"

sqlrconnection	*con;
sqlrcursor	*cur;
sqlrconnection	*secondcon;
sqlrcursor	*secondcur;

int main(int argc, char **argv) {

	const char	*subvars[4]={"var1","var2","var3",NULL};
	const char	*subvalstrings[3]={"hi","hello","bye"};
	int64_t		subvallongs[3]={1,2,3};
	double		subvaldoubles[3]={10.55,10.556,10.5556};
	uint32_t	precs[3]={4,5,6};
	uint32_t	scales[3]={2,3,4};
	const char * const *cols;
	const char * const *fields;
	uint16_t	port;
	char		*socket;
	uint16_t	id;
	char		*filename;
	uint32_t	*fieldlens;

	// instantiation
	con=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	cur=new sqlrcursor(con);

	// get database type
	stdoutput.printf("IDENTIFY: \n");
	assertEquals(con->identify(),"sqlite");
	stdoutput.printf("\n");

	// ping
	stdoutput.printf("PING: \n");
	assertTrue(con->ping());
	stdoutput.printf("\n");

	// bind format
	stdoutput.printf("BIND FORMAT: \n");
	assertEquals(con->bindFormat(),":*");
	stdoutput.printf("\n");

	// nextval format
	stdoutput.printf("NEXTVAL FORMAT: \n");
	assertEquals(con->nextvalFormat(),"");
	stdoutput.printf("\n");

	// drop existing table
	cur->sendQuery("begin transaction");
	cur->sendQuery("drop table testtable");
	con->commit();

	// create a new table
	stdoutput.printf("CREATE TEMPTABLE: \n");
	cur->sendQuery("begin transaction");
	assertTrue(cur->sendQuery("create table testtable (testint int, testfloat float, testchar char(40), testvarchar varchar(40), testclob clob, testblob blob)"));
	con->commit();
	stdoutput.printf("\n");

	stdoutput.printf("BEGIN TRANACTION: \n");
	cur->sendQuery("begin transaction");
	stdoutput.printf("\n");

	stdoutput.printf("INSERT: \n");
	assertTrue(cur->sendQuery("insert into testtable values (1,1.1,'testchar1','testvarchar1','testclob1','testblob1')"));
	assertTrue(cur->sendQuery("insert into testtable values (2,2.2,'testchar2','testvarchar2','testclob2','testblob2')"));
	assertTrue(cur->sendQuery("insert into testtable values (3,3.3,'testchar3','testvarchar3','testclob3','testblob3')"));
	assertTrue(cur->sendQuery("insert into testtable values (4,4.4,'testchar4','testvarchar4','testclob4','testblob4')"));
	stdoutput.printf("\n");

	stdoutput.printf("AFFECTED ROWS: \n");
	assertEquals(cur->affectedRows(),0);
	stdoutput.printf("\n");

	stdoutput.printf("BIND BY NAME: \n");
	cur->prepareQuery("insert into testtable values (:var1,:var2,:var3,:var4,:var5,:var6)");
	assertEquals(cur->countBindVariables(),6);
	cur->inputBind("var1",5);
	cur->inputBind("var2",5.5,4,1);
	cur->inputBind("var3","testchar5");
	cur->inputBind("var4","testvarchar5");
	cur->inputBindClob("var5","testclob5",9);
	cur->inputBindBlob("var6","testblob5",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("var1",6);
	cur->inputBind("var2",6.6,4,1);
	cur->inputBind("var3","testchar6");
	cur->inputBind("var4","testvarchar6");
	cur->inputBindClob("var5","testclob6",9);
	cur->inputBindBlob("var6","testblob6",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("var1",7);
	cur->inputBind("var2",7.7,4,1);
	cur->inputBind("var3","testchar7");
	cur->inputBind("var4","testvarchar7");
	cur->inputBindClob("var5","testclob7",9);
	cur->inputBindBlob("var6","testblob7",9);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");

	stdoutput.printf("BIND BY NAME WITH VALIDATION: \n");
	cur->clearBinds();
	cur->inputBind("var1",8);
	cur->inputBind("var2",8.8,4,1);
	cur->inputBind("var3","testchar8");
	cur->inputBind("var4","testvarchar8");
	cur->inputBindClob("var5","testclob8",9);
	cur->inputBindBlob("var6","testblob8",9);
	cur->validateBinds();
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");

	stdoutput.printf("SELECT: \n");
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	stdoutput.printf("\n");

	stdoutput.printf("COLUMN COUNT: \n");
	assertEquals(cur->colCount(),6);
	stdoutput.printf("\n");

	stdoutput.printf("COLUMN NAMES: \n");
	assertEquals(cur->getColumnName(0),"testint");
	assertEquals(cur->getColumnName(1),"testfloat");
	assertEquals(cur->getColumnName(2),"testchar");
	assertEquals(cur->getColumnName(3),"testvarchar");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"testint");
	assertEquals(cols[1],"testfloat");
	assertEquals(cols[2],"testchar");
	assertEquals(cols[3],"testvarchar");
	stdoutput.printf("\n");

	stdoutput.printf("COLUMN TYPES: \n");
	#ifdef HAVE_SQLITE3_STMT
	assertEquals(cur->getColumnType((uint32_t)0),"INTEGER");
	assertEquals(cur->getColumnType("testint"),"INTEGER");
	assertEquals(cur->getColumnType(1),"FLOAT");
	assertEquals(cur->getColumnType("testfloat"),"FLOAT");
	assertEquals(cur->getColumnType(2),"STRING");
	assertEquals(cur->getColumnType("testchar"),"STRING");
	assertEquals(cur->getColumnType(3),"STRING");
	assertEquals(cur->getColumnType("testvarchar"),"STRING");
	assertEquals(cur->getColumnType(4),"STRING");
	assertEquals(cur->getColumnType("testclob"),"STRING");
	assertEquals(cur->getColumnType(5),"STRING");
	assertEquals(cur->getColumnType("testblob"),"STRING");
	#else
	assertEquals(cur->getColumnType((uint32_t)0),"UNKNOWN");
	assertEquals(cur->getColumnType("testint"),"UNKNOWN");
	assertEquals(cur->getColumnType(1),"UNKNOWN");
	assertEquals(cur->getColumnType("testfloat"),"UNKNOWN");
	assertEquals(cur->getColumnType(2),"UNKNOWN");
	assertEquals(cur->getColumnType("testchar"),"UNKNOWN");
	assertEquals(cur->getColumnType(3),"UNKNOWN");
	assertEquals(cur->getColumnType("testvarchar"),"UNKNOWN");
	assertEquals(cur->getColumnType(4),"UNKNOWN");
	assertEquals(cur->getColumnType("testclob"),"UNKNOWN");
	assertEquals(cur->getColumnType(5),"UNKNOWN");
	assertEquals(cur->getColumnType("testblob"),"UNKNOWN");
	#endif
	stdoutput.printf("\n");

	stdoutput.printf("COLUMN LENGTH: \n");
	assertEquals(cur->getColumnLength((uint32_t)0),0);
	assertEquals(cur->getColumnLength("testint"),0);
	assertEquals(cur->getColumnLength(1),0);
	assertEquals(cur->getColumnLength("testfloat"),0);
	assertEquals(cur->getColumnLength(2),0);
	assertEquals(cur->getColumnLength("testchar"),0);
	assertEquals(cur->getColumnLength(3),0);
	assertEquals(cur->getColumnLength("testvarchar"),0);
	assertEquals(cur->getColumnLength(4),0);
	assertEquals(cur->getColumnLength("testclob"),0);
	assertEquals(cur->getColumnLength(5),0);
	assertEquals(cur->getColumnLength("testblob"),0);
	stdoutput.printf("\n");

	stdoutput.printf("LONGEST COLUMN: \n");
	assertEquals(cur->getLongest((uint32_t)0),1);
	assertEquals(cur->getLongest("testint"),1);
	assertEquals(cur->getLongest(1),3);
	assertEquals(cur->getLongest("testfloat"),3);
	assertEquals(cur->getLongest(2),9);
	assertEquals(cur->getLongest("testchar"),9);
	assertEquals(cur->getLongest(3),12);
	assertEquals(cur->getLongest("testvarchar"),12);
	assertEquals(cur->getLongest(4),9);
	assertEquals(cur->getLongest("testclob"),9);
	assertEquals(cur->getLongest(5),9);
	assertEquals(cur->getLongest("testblob"),9);
	stdoutput.printf("\n");

	stdoutput.printf("ROW COUNT: \n");
	assertEquals(cur->rowCount(),8);
	stdoutput.printf("\n");

	stdoutput.printf("TOTAL ROWS: \n");
	#ifdef HAVE_SQLITE3_STMT
	assertEquals(cur->totalRows(),0);
	#else
	assertEquals(cur->totalRows(),8);
	#endif
	stdoutput.printf("\n");

	stdoutput.printf("FIRST ROW INDEX: \n");
	assertEquals(cur->firstRowIndex(),0);
	stdoutput.printf("\n");

	stdoutput.printf("END OF RESULT SET: \n");
	assertTrue(cur->endOfResultSet());
	stdoutput.printf("\n");

	stdoutput.printf("FIELDS BY INDEX: \n");
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"1.1");
	assertEquals(cur->getField(0,2),"testchar1");
	assertEquals(cur->getField(0,3),"testvarchar1");
	assertEquals(cur->getField(0,4),"testclob1");
	assertEquals(cur->getField(0,5),"testblob1");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(7,1),"8.8");
	assertEquals(cur->getField(7,2),"testchar8");
	assertEquals(cur->getField(7,3),"testvarchar8");
	assertEquals(cur->getField(7,4),"testclob8");
	assertEquals(cur->getField(7,5),"testblob8");
	stdoutput.printf("\n");

	stdoutput.printf("FIELD LENGTHS BY INDEX: \n");
	assertEquals(cur->getFieldLength(0,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(0,1),3);
	assertEquals(cur->getFieldLength(0,2),9);
	assertEquals(cur->getFieldLength(0,3),12);
	assertEquals(cur->getFieldLength(0,4),9);
	assertEquals(cur->getFieldLength(0,5),9);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(7,1),3);
	assertEquals(cur->getFieldLength(7,2),9);
	assertEquals(cur->getFieldLength(7,3),12);
	assertEquals(cur->getFieldLength(7,4),9);
	assertEquals(cur->getFieldLength(7,5),9);
	stdoutput.printf("\n");

	stdoutput.printf("FIELDS BY NAME: \n");
	assertEquals(cur->getField(0,"testint"),"1");
	assertEquals(cur->getField(0,"testfloat"),"1.1");
	assertEquals(cur->getField(0,"testchar"),"testchar1");
	assertEquals(cur->getField(0,"testvarchar"),"testvarchar1");
	assertEquals(cur->getField(0,"testclob"),"testclob1");
	assertEquals(cur->getField(0,"testblob"),"testblob1");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,"testint"),"8");
	assertEquals(cur->getField(7,"testfloat"),"8.8");
	assertEquals(cur->getField(7,"testchar"),"testchar8");
	assertEquals(cur->getField(7,"testvarchar"),"testvarchar8");
	assertEquals(cur->getField(7,"testclob"),"testclob8");
	assertEquals(cur->getField(7,"testblob"),"testblob8");
	stdoutput.printf("\n");

	stdoutput.printf("FIELD LENGTHS BY NAME: \n");
	assertEquals(cur->getFieldLength(0,"testint"),1);
	assertEquals(cur->getFieldLength(0,"testfloat"),3);
	assertEquals(cur->getFieldLength(0,"testchar"),9);
	assertEquals(cur->getFieldLength(0,"testvarchar"),12);
	assertEquals(cur->getFieldLength(0,"testclob"),9);
	assertEquals(cur->getFieldLength(0,"testblob"),9);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,"testint"),1);
	assertEquals(cur->getFieldLength(7,"testfloat"),3);
	assertEquals(cur->getFieldLength(7,"testchar"),9);
	assertEquals(cur->getFieldLength(7,"testvarchar"),12);
	assertEquals(cur->getFieldLength(7,"testclob"),9);
	assertEquals(cur->getFieldLength(7,"testblob"),9);
	stdoutput.printf("\n");

	stdoutput.printf("FIELDS BY ARRAY: \n");
	fields=cur->getRow(0);
	assertEquals(fields[0],"1");
	assertEquals(fields[1],"1.1");
	assertEquals(fields[2],"testchar1");
	assertEquals(fields[3],"testvarchar1");
	assertEquals(fields[4],"testclob1");
	assertEquals(fields[5],"testblob1");
	stdoutput.printf("\n");

	stdoutput.printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=cur->getRowLengths(0);
	assertEquals(fieldlens[0],1);
	assertEquals(fieldlens[1],3);
	assertEquals(fieldlens[2],9);
	assertEquals(fieldlens[3],12);
	assertEquals(fieldlens[4],9);
	assertEquals(fieldlens[5],9);
	stdoutput.printf("\n");

	stdoutput.printf("INDIVIDUAL SUBSTITUTIONS: \n");
	cur->sendQuery("drop table testtable1");
	assertTrue(cur->sendQuery("create table testtable1 (col1 int, col2 char, col3 float)"));
	cur->prepareQuery("insert into testtable1 values ($(var1),'$(var2)',$(var3))");
	cur->substitution("var1",1);
	cur->substitution("var2","hello");
	cur->substitution("var3",10.5556,6,4);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");

	stdoutput.printf("FIELDS: \n");
	assertTrue(cur->sendQuery("select * from testtable1"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"hello");
	assertEquals(cur->getField(0,2),"10.5556");
	assertTrue(cur->sendQuery("delete from testtable1"));
	stdoutput.printf("\n");

	stdoutput.printf("ARRAY SUBSTITUTIONS: \n");
	cur->prepareQuery("insert into testtable1 values ('$(var1)','$(var2)','$(var3)')");
	cur->substitutions(subvars,subvalstrings);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");

	stdoutput.printf("FIELDS: \n");
	assertTrue(cur->sendQuery("select * from testtable1"));
	assertEquals(cur->getField(0,(uint32_t)0),"hi");
	assertEquals(cur->getField(0,1),"hello");
	assertEquals(cur->getField(0,2),"bye");
	assertTrue(cur->sendQuery("delete from testtable1"));
	stdoutput.printf("\n");


	stdoutput.printf("ARRAY SUBSTITUTIONS: \n");
	cur->prepareQuery("insert into testtable1 values ($(var1),'$(var2)',$(var3))");
	cur->substitutions(subvars,subvallongs);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");

	stdoutput.printf("FIELDS: \n");
	assertTrue(cur->sendQuery("select * from testtable1"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"2");
	assertEquals(cur->getField(0,2),"3.0");
	assertTrue(cur->sendQuery("delete from testtable1"));
	stdoutput.printf("\n");


	stdoutput.printf("ARRAY SUBSTITUTIONS: \n");
	cur->prepareQuery("insert into testtable1 values ($(var1),'$(var2)',$(var3))");
	cur->substitutions(subvars,subvaldoubles,precs,scales);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");

	stdoutput.printf("FIELDS: \n");
	assertTrue(cur->sendQuery("select * from testtable1"));
	assertEquals(cur->getField(0,(uint32_t)0),"10.55");
	assertEquals(cur->getField(0,1),"10.556");
	assertEquals(cur->getField(0,2),"10.5556");
	assertTrue(cur->sendQuery("delete from testtable1"));
	stdoutput.printf("\n");


	stdoutput.printf("NULLS as Nulls: \n");
	cur->getNullsAsNulls();
	assertTrue(cur->sendQuery("insert into testtable1 values (1,NULL,NULL)"));
	assertTrue(cur->sendQuery("select * from testtable1"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),NULL);
	assertEquals(cur->getField(0,2),NULL);
	cur->getNullsAsEmptyStrings();
	assertTrue(cur->sendQuery("select * from testtable1"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"");
	assertEquals(cur->getField(0,2),"");
	cur->getNullsAsNulls();
	stdoutput.printf("\n");

	stdoutput.printf("RESULT SET BUFFER SIZE: \n");
	assertEquals(cur->getResultSetBufferSize(),0);
	cur->setResultSetBufferSize(2);
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
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

	stdoutput.printf("DONT GET COLUMN INFO: \n");
	cur->dontGetColumnInfo();
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	assertEquals(cur->getColumnName(0),NULL);
	assertEquals(cur->getColumnLength((uint32_t)0),0);
	assertEquals(cur->getColumnType((uint32_t)0),NULL);
	cur->getColumnInfo();
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	assertEquals(cur->getColumnName(0),"testint");
	assertEquals(cur->getColumnLength((uint32_t)0),0);
	#ifdef HAVE_SQLITE3_STMT
	assertEquals(cur->getColumnType((uint32_t)0),"INTEGER");
	#else
	assertEquals(cur->getColumnType((uint32_t)0),"UNKNOWN");
	#endif
	stdoutput.printf("\n");

	stdoutput.printf("SUSPENDED SESSION: \n");
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
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

	stdoutput.printf("SUSPENDED RESULT SET: \n");
	cur->setResultSetBufferSize(2);
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
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

	stdoutput.printf("CACHED RESULT SET: \n");
	cur->cacheToFile("cachefile1");
	cur->setCacheTtl(200);
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	filename=charstring::duplicate(cur->getCacheFileName());
	assertEquals(filename,"cachefile1");
	cur->cacheOff();
	assertTrue(cur->openCachedResultSet(filename));
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	delete[] filename;
	stdoutput.printf("\n");

	stdoutput.printf("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEquals(cur->colCount(),6);
	stdoutput.printf("\n");

	stdoutput.printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEquals(cur->getColumnName(0),"testint");
	assertEquals(cur->getColumnName(1),"testfloat");
	assertEquals(cur->getColumnName(2),"testchar");
	assertEquals(cur->getColumnName(3),"testvarchar");
	assertEquals(cur->getColumnName(4),"testclob");
	assertEquals(cur->getColumnName(5),"testblob");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"testint");
	assertEquals(cols[1],"testfloat");
	assertEquals(cols[2],"testchar");
	assertEquals(cols[3],"testvarchar");
	assertEquals(cols[4],"testclob");
	assertEquals(cols[5],"testblob");
	stdoutput.printf("\n");

	stdoutput.printf("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n");
	cur->setResultSetBufferSize(2);
	cur->cacheToFile("cachefile1");
	cur->setCacheTtl(200);
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
	filename=charstring::duplicate(cur->getCacheFileName());
	assertEquals(filename,"cachefile1");
	cur->cacheOff();
	assertTrue(cur->openCachedResultSet(filename));
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(8,(uint32_t)0),NULL);
	cur->setResultSetBufferSize(0);
	delete[] filename;
	stdoutput.printf("\n");

	stdoutput.printf("FROM ONE CACHE FILE TO ANOTHER: \n");
	cur->cacheToFile("cachefile2");
	assertTrue(cur->openCachedResultSet("cachefile1"));
	cur->cacheOff();
	assertTrue(cur->openCachedResultSet("cachefile2"));
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(8,(uint32_t)0),NULL);
	stdoutput.printf("\n");

	stdoutput.printf("FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET BUFFER SIZE: \n");
	cur->setResultSetBufferSize(2);
	cur->cacheToFile("cachefile2");
	assertTrue(cur->openCachedResultSet("cachefile1"));
	cur->cacheOff();
	assertTrue(cur->openCachedResultSet("cachefile2"));
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(8,(uint32_t)0),NULL);
	cur->setResultSetBufferSize(0);
	stdoutput.printf("\n");

	stdoutput.printf("CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE: \n");
	cur->setResultSetBufferSize(2);
	cur->cacheToFile("cachefile1");
	cur->setCacheTtl(200);
	assertTrue(cur->sendQuery("select * from testtable order by testint"));
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

	stdoutput.printf("COMMIT AND ROLLBACK: \n");
	secondcon=new sqlrconnection("sqlrelay",9000,"/tmp/test.socket",
						"testuser","testpassword",0,1);
	secondcur=new sqlrcursor(secondcon);
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"0");
	assertTrue(con->commit());
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"8");
	assertTrue(cur->sendQuery("insert into testtable values (10,10.1,'testchar10','testvarchar10','testclob10','testblob10')"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"9");
	stdoutput.printf("\n");

	stdoutput.printf("FINISHED SUSPENDED SESSION: \n");
	assertTrue(cur->sendQuery("select * from testtable"));
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

	// drop existing table
	cur->sendQuery("drop table testtable");

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

	// last insert row id
	stdoutput.printf("LAST INSERT ROW ID: \n");
	assertTrue(cur->sendQuery("select last insert rowid"));
	assertEquals(cur->colCount(),1);
	assertEquals(cur->rowCount(),1);
	assertEquals(cur->getColumnName(0),"LASTINSERTROWID");
	assertFalse(charstring::isNullOrEmpty(cur->getField(0,(uint32_t)0)));
	stdoutput.printf("\n");

	// invalid queries...
	stdoutput.printf("INVALID QUERIES: \n");
	assertFalse(cur->sendQuery("select * from testtable"));
	assertFalse(cur->sendQuery("select * from testtable"));
	assertFalse(cur->sendQuery("select * from testtable"));
	assertFalse(cur->sendQuery("select * from testtable"));
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

	return 0;
}
