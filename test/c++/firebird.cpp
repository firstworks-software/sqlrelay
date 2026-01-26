// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclient.h>
#include <rudiments/process.h>
#include <rudiments/stdio.h>

#include "assert.cpp"

sqlrconnection	*con;
sqlrcursor	*cur;
sqlrconnection	*secondcon;
sqlrcursor	*secondcur;

int main(int argc, char **argv) {

	const char	*bindvars[13]={"1","2","3","4","5","6",
				"7","8","9","10","11","12",NULL};
	const char	*bindvals[12]={"4","4","4.4","4.4","4.4","4.4",
				"01-JAN-2004","04:00:00",
				"testchar4","testvarchar4",NULL,"testblob4"};
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
	assertEquals(con->identify(),"firebird");
	stdoutput.printf("\n");

	// ping
	stdoutput.printf("PING: \n");
	assertTrue(con->ping());
	stdoutput.printf("\n");

	// bind format
	stdoutput.printf("BIND FORMAT: \n");
	assertEquals(con->bindFormat(),"?");
	stdoutput.printf("\n");

	// nextval format
	stdoutput.printf("NEXTVAL FORMAT: \n");
	assertEquals(con->nextvalFormat(),"next value for %s");
	stdoutput.printf("\n");

	// isolation levels
	stdoutput.printf("ISOLATION LEVELS: \n");
	// though firebird does support a "set transaction ..." statement to
	// set the isolation level, it looks like, in firebird, you can really
	// only set it through the TPB at the start of a transaction, so
	// attempts to set it shoud fail
	assertFalse(con->setIsolationLevel("read committed"));
	assertEquals(con->getIsolationLevel(),"read committed");
	stdoutput.printf("\n");

	// clean up table
	cur->sendQuery("delete from testtable");
	con->commit();

	stdoutput.printf("INSERT: \n");
	assertTrue(cur->sendQuery("insert into testtable values (1,1,1.1,1.1,1.1,1.1,'01-JAN-2001','01:00:00','testchar1','testvarchar1',NULL,'testblob1')"));
	stdoutput.printf("\n");

	stdoutput.printf("BIND BY POSITION: \n");
	cur->prepareQuery("insert into testtable values (?,?,?,?,?,?,?,?,?,?,?,?)");
	assertEquals(cur->countBindVariables(),12);
	cur->inputBind("1",2);
	cur->inputBind("2",2);
	cur->inputBind("3",2.2,2,1);
	cur->inputBind("4",2.2,2,1);
	cur->inputBind("5",2.2,2,1);
	cur->inputBind("6",2.2,2,1);
	cur->inputBind("7",2002,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("8",-1,-1,-1,2,0,0,0,NULL,false);
	cur->inputBind("9","testchar2");
	cur->inputBind("10","testvarchar2");
	cur->inputBind("11",(char *)NULL);
	cur->inputBindBlob("12","testblob2",9);
	assertTrue(cur->executeQuery());
	cur->clearBinds();
	cur->inputBind("1",3);
	cur->inputBind("2",3);
	cur->inputBind("3",3.3,2,1);
	cur->inputBind("4",3.3,2,1);
	cur->inputBind("5",3.3,2,1);
	cur->inputBind("6",3.3,2,1);
	cur->inputBind("7",2003,1,1,-1,-1,-1,-1,NULL,false);
	cur->inputBind("8",-1,-1,-1,3,0,0,0,NULL,false);
	cur->inputBind("9","testchar3");
	cur->inputBind("10","testvarchar3");
	cur->inputBind("11",(char *)NULL);
	cur->inputBindBlob("13","testblob3",9);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");

	stdoutput.printf("ARRAY OF BINDS BY POSITION: \n");
	cur->clearBinds();
	cur->inputBinds(bindvars,bindvals);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");

	stdoutput.printf("INSERT: \n");
	assertTrue(cur->sendQuery("insert into testtable values (5,5,5.5,5.5,5.5,5.5,'01-JAN-2005','05:00:00','testchar5','testvarchar5',NULL,'testblob5')"));
	assertTrue(cur->sendQuery("insert into testtable values (6,6,6.6,6.6,6.6,6.6,'01-JAN-2006','06:00:00','testchar6','testvarchar6',NULL,'testblob6')"));
	assertTrue(cur->sendQuery("insert into testtable values (7,7,7.7,7.7,7.7,7.7,'01-JAN-2007','07:00:00','testchar7','testvarchar7',NULL,'testblob7')"));
	assertTrue(cur->sendQuery("insert into testtable values (8,8,8.8,8.8,8.8,8.8,'01-JAN-2008','08:00:00','testchar8','testvarchar8',NULL,'testblob8')"));
	stdoutput.printf("\n");

	stdoutput.printf("AFFECTED ROWS: \n");
	assertEquals(cur->affectedRows(),0);
	stdoutput.printf("\n");

	stdoutput.printf("STORED PROCEDURE: \n");
	cur->prepareQuery("select * from testproc(?,?,?,?)");
	cur->inputBind("1",1);
	cur->inputBind("2",1.1,2,1);
	cur->inputBind("3","hello");
	cur->inputBindBlob("4","blob",4);
	assertTrue(cur->executeQuery());
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"1.1000");
	assertEquals(cur->getField(0,2),"hello");
	assertEquals(cur->getField(0,3),"blob");
	cur->prepareQuery("execute procedure testproc ?, ?, ?, ?");
	cur->inputBind("1",1);
	cur->inputBind("2",1.1,2,1);
	cur->inputBind("3","hello");
	cur->inputBindBlob("4","blob",4);
	cur->defineOutputBindInteger("1");
	cur->defineOutputBindDouble("2");
	cur->defineOutputBindString("3",20);
	cur->defineOutputBindBlob("4");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindInteger("1"),1);
	//assertEquals(cur->getOutputBindDouble("2"),1.1);
	assertEquals(cur->getOutputBindString("3"),"hello               ");
	assertEquals(cur->getOutputBindBlob("4"),"blob");
	stdoutput.printf("\n");

	stdoutput.printf("LONG BLOB: \n");
	cur->sendQuery("delete from testtable1");
	cur->prepareQuery("insert into testtable1 values (?)");
	char	blobval[20*1024+1];
	for (int i=0; i<20*1024; i++) {
		blobval[i]='C';
	}
	blobval[20*1024]='\0';
	cur->inputBindClob("1",blobval,20*1024);
	assertTrue(cur->executeQuery());
	cur->sendQuery("select testblob from testtable1");
	assertEquals(cur->getFieldLength(0,"TESTBLOB"),20*1024);
	assertEquals(cur->getField(0,"TESTBLOB"),blobval);
	cur->prepareQuery("execute procedure testproc1 ?");
	cur->inputBindBlob("1",blobval,20*1024);
	cur->defineOutputBindBlob("1");
	assertTrue(cur->executeQuery());
	assertEquals(cur->getOutputBindLength("1"),20*1024);
	assertEquals(cur->getOutputBindBlob("1"),blobval);
	stdoutput.printf("\n");

	stdoutput.printf("SELECT: \n");
	assertTrue(cur->sendQuery("select * from testtable order by testinteger"));
	stdoutput.printf("\n");

	stdoutput.printf("COLUMN COUNT: \n");
	assertEquals(cur->colCount(),12);
	stdoutput.printf("\n");

	stdoutput.printf("COLUMN NAMES: \n");
	assertEquals(cur->getColumnName(0),"TESTINTEGER");
	assertEquals(cur->getColumnName(1),"TESTSMALLINT");
	assertEquals(cur->getColumnName(2),"TESTDECIMAL");
	assertEquals(cur->getColumnName(3),"TESTNUMERIC");
	assertEquals(cur->getColumnName(4),"TESTFLOAT");
	assertEquals(cur->getColumnName(5),"TESTDOUBLE");
	assertEquals(cur->getColumnName(6),"TESTDATE");
	assertEquals(cur->getColumnName(7),"TESTTIME");
	assertEquals(cur->getColumnName(8),"TESTCHAR");
	assertEquals(cur->getColumnName(9),"TESTVARCHAR");
	assertEquals(cur->getColumnName(10),"TESTTIMESTAMP");
	assertEquals(cur->getColumnName(11),"TESTBLOB");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"TESTINTEGER");
	assertEquals(cols[1],"TESTSMALLINT");
	assertEquals(cols[2],"TESTDECIMAL");
	assertEquals(cols[3],"TESTNUMERIC");
	assertEquals(cols[4],"TESTFLOAT");
	assertEquals(cols[5],"TESTDOUBLE");
	assertEquals(cols[6],"TESTDATE");
	assertEquals(cols[7],"TESTTIME");
	assertEquals(cols[8],"TESTCHAR");
	assertEquals(cols[9],"TESTVARCHAR");
	assertEquals(cols[10],"TESTTIMESTAMP");
	assertEquals(cols[11],"TESTBLOB");
	stdoutput.printf("\n");

	stdoutput.printf("COLUMN TYPES: \n");
	assertEquals(cur->getColumnType((uint32_t)0),"INTEGER");
	assertEquals(cur->getColumnType("TESTINTEGER"),"INTEGER");
	assertEquals(cur->getColumnType(1),"SMALLINT");
	assertEquals(cur->getColumnType("TESTSMALLINT"),"SMALLINT");
	assertEquals(cur->getColumnType(2),"DECIMAL");
	assertEquals(cur->getColumnType("TESTDECIMAL"),"DECIMAL");
	assertEquals(cur->getColumnType(3),"NUMERIC");
	assertEquals(cur->getColumnType("TESTNUMERIC"),"NUMERIC");
	assertEquals(cur->getColumnType(4),"FLOAT");
	assertEquals(cur->getColumnType("TESTFLOAT"),"FLOAT");
	assertEquals(cur->getColumnType(5),"DOUBLE PRECISION");
	assertEquals(cur->getColumnType("TESTDOUBLE"),"DOUBLE PRECISION");
	assertEquals(cur->getColumnType(6),"DATE");
	assertEquals(cur->getColumnType("TESTDATE"),"DATE");
	assertEquals(cur->getColumnType(7),"TIME");
	assertEquals(cur->getColumnType("TESTTIME"),"TIME");
	assertEquals(cur->getColumnType(8),"CHAR");
	assertEquals(cur->getColumnType("TESTCHAR"),"CHAR");
	assertEquals(cur->getColumnType(9),"VARCHAR");
	assertEquals(cur->getColumnType("TESTVARCHAR"),"VARCHAR");
	assertEquals(cur->getColumnType(10),"TIMESTAMP");
	assertEquals(cur->getColumnType("TESTTIMESTAMP"),"TIMESTAMP");
	assertEquals(cur->getColumnType(11),"BLOB");
	assertEquals(cur->getColumnType("TESTBLOB"),"BLOB");
	stdoutput.printf("\n");

	stdoutput.printf("COLUMN LENGTH: \n");
	assertEquals(cur->getColumnLength((uint32_t)0),4);
	assertEquals(cur->getColumnLength("TESTINTEGER"),4);
	assertEquals(cur->getColumnLength(1),2);
	assertEquals(cur->getColumnLength("TESTSMALLINT"),2);
	assertEquals(cur->getColumnLength(2),8);
	assertEquals(cur->getColumnLength("TESTDECIMAL"),8);
	assertEquals(cur->getColumnLength(3),8);
	assertEquals(cur->getColumnLength("TESTNUMERIC"),8);
	assertEquals(cur->getColumnLength(4),4);
	assertEquals(cur->getColumnLength("TESTFLOAT"),4);
	assertEquals(cur->getColumnLength(5),8);
	assertEquals(cur->getColumnLength("TESTDOUBLE"),8);
	assertEquals(cur->getColumnLength(6),4);
	assertEquals(cur->getColumnLength("TESTDATE"),4);
	assertEquals(cur->getColumnLength(7),4);
	assertEquals(cur->getColumnLength("TESTTIME"),4);
	assertEquals(cur->getColumnLength(8),50);
	assertEquals(cur->getColumnLength("TESTCHAR"),50);
	assertEquals(cur->getColumnLength(9),50);
	assertEquals(cur->getColumnLength("TESTVARCHAR"),50);
	assertEquals(cur->getColumnLength(10),8);
	assertEquals(cur->getColumnLength("TESTTIMESTAMP"),8);
	assertEquals(cur->getColumnLength(11),8);
	assertEquals(cur->getColumnLength("TESTBLOB"),8);
	stdoutput.printf("\n");

	stdoutput.printf("LONGEST COLUMN: \n");
	assertEquals(cur->getLongest((uint32_t)0),1);
	assertEquals(cur->getLongest("TESTINTEGER"),1);
	assertEquals(cur->getLongest(1),1);
	assertEquals(cur->getLongest("TESTSMALLINT"),1);
	assertEquals(cur->getLongest(2),4);
	assertEquals(cur->getLongest("TESTDECIMAL"),4);
	assertEquals(cur->getLongest(3),4);
	assertEquals(cur->getLongest("TESTNUMERIC"),4);
	assertEquals(cur->getLongest(4),6);
	assertEquals(cur->getLongest("TESTFLOAT"),6);
	assertEquals(cur->getLongest(5),6);
	assertEquals(cur->getLongest("TESTDOUBLE"),6);
	assertEquals(cur->getLongest(6),10);
	assertEquals(cur->getLongest("TESTDATE"),10);
	assertEquals(cur->getLongest(7),8);
	assertEquals(cur->getLongest("TESTTIME"),8);
	assertEquals(cur->getLongest(8),50);
	assertEquals(cur->getLongest("TESTCHAR"),50);
	assertEquals(cur->getLongest(9),12);
	assertEquals(cur->getLongest("TESTVARCHAR"),12);
	assertEquals(cur->getLongest(10),0);
	assertEquals(cur->getLongest("TESTTIMESTAMP"),0);
	assertEquals(cur->getLongest(11),9);
	assertEquals(cur->getLongest("TESTBLOB"),9);
	stdoutput.printf("\n");

	stdoutput.printf("ROW COUNT: \n");
	assertEquals(cur->rowCount(),8);
	stdoutput.printf("\n");

	stdoutput.printf("TOTAL ROWS: \n");
	assertEquals(cur->totalRows(),0);
	stdoutput.printf("\n");

	stdoutput.printf("FIRST ROW INDEX: \n");
	assertEquals(cur->firstRowIndex(),0);
	stdoutput.printf("\n");

	stdoutput.printf("END OF RESULT SET: \n");
	assertTrue(cur->endOfResultSet());
	stdoutput.printf("\n");

	stdoutput.printf("FIELDS BY INDEX: \n");
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"1");
	assertEquals(cur->getField(0,2),"1.10");
	assertEquals(cur->getField(0,3),"1.10");
	assertEquals(cur->getField(0,4),"1.1000");
	assertEquals(cur->getField(0,5),"1.1000");
	assertEquals(cur->getField(0,6),"2001:01:01");
	assertEquals(cur->getField(0,7),"01:00:00");
	assertEquals(cur->getField(0,8),"testchar1                                         ");
	assertEquals(cur->getField(0,9),"testvarchar1");
	assertEquals(cur->getField(0,11),"testblob1");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	assertEquals(cur->getField(7,1),"8");
	assertEquals(cur->getField(7,2),"8.80");
	assertEquals(cur->getField(7,3),"8.80");
	assertEquals(cur->getField(7,4),"8.8000");
	assertEquals(cur->getField(7,5),"8.8000");
	assertEquals(cur->getField(7,6),"2008:01:01");
	assertEquals(cur->getField(7,7),"08:00:00");
	assertEquals(cur->getField(7,8),"testchar8                                         ");
	assertEquals(cur->getField(7,9),"testvarchar8");
	assertEquals(cur->getField(7,11),"testblob8");
	stdoutput.printf("\n");

	stdoutput.printf("FIELD LENGTHS BY INDEX: \n");
	assertEquals(cur->getFieldLength(0,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(0,1),1);
	assertEquals(cur->getFieldLength(0,2),4);
	assertEquals(cur->getFieldLength(0,3),4);
	assertEquals(cur->getFieldLength(0,4),6);
	assertEquals(cur->getFieldLength(0,5),6);
	assertEquals(cur->getFieldLength(0,6),10);
	assertEquals(cur->getFieldLength(0,7),8);
	assertEquals(cur->getFieldLength(0,8),50);
	assertEquals(cur->getFieldLength(0,9),12);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,(uint32_t)0),1);
	assertEquals(cur->getFieldLength(7,1),1);
	assertEquals(cur->getFieldLength(7,2),4);
	assertEquals(cur->getFieldLength(7,3),4);
	assertEquals(cur->getFieldLength(7,4),6);
	assertEquals(cur->getFieldLength(7,5),6);
	assertEquals(cur->getFieldLength(7,6),10);
	assertEquals(cur->getFieldLength(7,7),8);
	assertEquals(cur->getFieldLength(7,8),50);
	assertEquals(cur->getFieldLength(7,9),12);
	stdoutput.printf("\n");

	stdoutput.printf("FIELDS BY NAME: \n");
	assertEquals(cur->getField(0,"TESTINTEGER"),"1");
	assertEquals(cur->getField(0,"TESTSMALLINT"),"1");
	assertEquals(cur->getField(0,"TESTDECIMAL"),"1.10");
	assertEquals(cur->getField(0,"TESTNUMERIC"),"1.10");
	assertEquals(cur->getField(0,"TESTFLOAT"),"1.1000");
	assertEquals(cur->getField(0,"TESTDOUBLE"),"1.1000");
	assertEquals(cur->getField(0,"TESTDATE"),"2001:01:01");
	assertEquals(cur->getField(0,"TESTTIME"),"01:00:00");
	assertEquals(cur->getField(0,"TESTCHAR"),"testchar1                                         ");
	assertEquals(cur->getField(0,"TESTVARCHAR"),"testvarchar1");
	assertEquals(cur->getField(0,"TESTBLOB"),"testblob1");
	stdoutput.printf("\n");
	assertEquals(cur->getField(7,"TESTINTEGER"),"8");
	assertEquals(cur->getField(7,"TESTSMALLINT"),"8");
	assertEquals(cur->getField(7,"TESTDECIMAL"),"8.80");
	assertEquals(cur->getField(7,"TESTNUMERIC"),"8.80");
	assertEquals(cur->getField(7,"TESTFLOAT"),"8.8000");
	assertEquals(cur->getField(7,"TESTDOUBLE"),"8.8000");
	assertEquals(cur->getField(7,"TESTDATE"),"2008:01:01");
	assertEquals(cur->getField(7,"TESTTIME"),"08:00:00");
	assertEquals(cur->getField(7,"TESTCHAR"),"testchar8                                         ");
	assertEquals(cur->getField(7,"TESTVARCHAR"),"testvarchar8");
	assertEquals(cur->getField(7,"TESTBLOB"),"testblob8");
	stdoutput.printf("\n");

	stdoutput.printf("FIELD LENGTHS BY NAME: \n");
	assertEquals(cur->getFieldLength(0,"TESTINTEGER"),1);
	assertEquals(cur->getFieldLength(0,"TESTSMALLINT"),1);
	assertEquals(cur->getFieldLength(0,"TESTDECIMAL"),4);
	assertEquals(cur->getFieldLength(0,"TESTNUMERIC"),4);
	assertEquals(cur->getFieldLength(0,"TESTFLOAT"),6);
	assertEquals(cur->getFieldLength(0,"TESTDOUBLE"),6);
	assertEquals(cur->getFieldLength(0,"TESTDATE"),10);
	assertEquals(cur->getFieldLength(0,"TESTTIME"),8);
	assertEquals(cur->getFieldLength(0,"TESTCHAR"),50);
	assertEquals(cur->getFieldLength(0,"TESTVARCHAR"),12);
	stdoutput.printf("\n");
	assertEquals(cur->getFieldLength(7,"TESTINTEGER"),1);
	assertEquals(cur->getFieldLength(7,"TESTSMALLINT"),1);
	assertEquals(cur->getFieldLength(7,"TESTDECIMAL"),4);
	assertEquals(cur->getFieldLength(7,"TESTNUMERIC"),4);
	assertEquals(cur->getFieldLength(7,"TESTFLOAT"),6);
	assertEquals(cur->getFieldLength(7,"TESTDOUBLE"),6);
	assertEquals(cur->getFieldLength(7,"TESTDATE"),10);
	assertEquals(cur->getFieldLength(7,"TESTTIME"),8);
	assertEquals(cur->getFieldLength(7,"TESTCHAR"),50);
	assertEquals(cur->getFieldLength(7,"TESTVARCHAR"),12);
	stdoutput.printf("\n");

	stdoutput.printf("FIELDS BY ARRAY: \n");
	fields=cur->getRow(0);
	assertEquals(fields[0],"1");
	assertEquals(fields[1],"1");
	assertEquals(fields[2],"1.10");
	assertEquals(fields[3],"1.10");
	assertEquals(fields[4],"1.1000");
	assertEquals(fields[5],"1.1000");
	assertEquals(fields[6],"2001:01:01");
	assertEquals(fields[7],"01:00:00");
	assertEquals(fields[8],"testchar1                                         ");
	assertEquals(fields[9],"testvarchar1");
	assertEquals(fields[11],"testblob1");
	stdoutput.printf("\n");

	stdoutput.printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=cur->getRowLengths(0);
	assertEquals(fieldlens[0],1);
	assertEquals(fieldlens[1],1);
	assertEquals(fieldlens[2],4);
	assertEquals(fieldlens[3],4);
	assertEquals(fieldlens[4],6);
	assertEquals(fieldlens[5],6);
	assertEquals(fieldlens[6],10);
	assertEquals(fieldlens[7],8);
	assertEquals(fieldlens[8],50);
	assertEquals(fieldlens[9],12);
	stdoutput.printf("\n");

	stdoutput.printf("INDIVIDUAL SUBSTITUTIONS: \n");
	cur->prepareQuery("select $(var1),'$(var2)',$(var3) from rdb$database");
	cur->substitution("var1",1);
	cur->substitution("var2","hello");
	cur->substitution("var3",10.5556,6,4);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");

	stdoutput.printf("FIELDS: \n");
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"hello");
	assertEquals(cur->getField(0,2),"10.5556");
	stdoutput.printf("\n");

	stdoutput.printf("ARRAY SUBSTITUTIONS: \n");
	cur->prepareQuery("select '$(var1)','$(var2)','$(var3)' from rdb$database");
	cur->substitutions(subvars,subvalstrings);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");

	stdoutput.printf("FIELDS: \n");
	assertEquals(cur->getField(0,(uint32_t)0),"hi");
	assertEquals(cur->getField(0,1),"hello");
	assertEquals(cur->getField(0,2),"bye");
	stdoutput.printf("\n");

	stdoutput.printf("ARRAY SUBSTITUTIONS: \n");
	cur->prepareQuery("select $(var1),$(var2),$(var3) from rdb$database");
	cur->substitutions(subvars,subvallongs);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");

	stdoutput.printf("FIELDS: \n");
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"2");
	assertEquals(cur->getField(0,2),"3");
	stdoutput.printf("\n");

	stdoutput.printf("ARRAY SUBSTITUTIONS: \n");
	cur->prepareQuery("select $(var1),$(var2),$(var3) from rdb$database");
	cur->substitutions(subvars,subvaldoubles,precs,scales);
	assertTrue(cur->executeQuery());
	stdoutput.printf("\n");

	stdoutput.printf("FIELDS: \n");
	assertEquals(cur->getField(0,(uint32_t)0),"10.55");
	assertEquals(cur->getField(0,1),"10.556");
	assertEquals(cur->getField(0,2),"10.5556");
	stdoutput.printf("\n");

	stdoutput.printf("NULLS as Nulls: \n");
	cur->getNullsAsNulls();
	assertTrue(cur->sendQuery("select 1,NULL,NULL from rdb$database"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),NULL);
	assertEquals(cur->getField(0,2),NULL);
	cur->getNullsAsEmptyStrings();
	assertTrue(cur->sendQuery("select 1,NULL,NULL from rdb$database"));
	assertEquals(cur->getField(0,(uint32_t)0),"1");
	assertEquals(cur->getField(0,1),"");
	assertEquals(cur->getField(0,2),"");
	cur->getNullsAsNulls();
	stdoutput.printf("\n");

	stdoutput.printf("RESULT SET BUFFER SIZE: \n");
	assertEquals(cur->getResultSetBufferSize(),0);
	cur->setResultSetBufferSize(2);
	assertTrue(cur->sendQuery("select * from testtable order by testinteger"));
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
	assertTrue(cur->sendQuery("select * from testtable order by testinteger"));
	assertEquals(cur->getColumnName(0),NULL);
	assertEquals(cur->getColumnLength((uint32_t)0),0);
	assertEquals(cur->getColumnType((uint32_t)0),NULL);
	cur->getColumnInfo();
	assertTrue(cur->sendQuery("select * from testtable order by testinteger"));
	assertEquals(cur->getColumnName(0),"TESTINTEGER");
	assertEquals(cur->getColumnLength((uint32_t)0),4);
	assertEquals(cur->getColumnType((uint32_t)0),"INTEGER");
	stdoutput.printf("\n");

	stdoutput.printf("SUSPENDED SESSION: \n");
	assertTrue(cur->sendQuery("select * from testtable order by testinteger"));
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
	assertTrue(cur->sendQuery("select * from testtable order by testinteger"));
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
	assertTrue(cur->sendQuery("select * from testtable order by testinteger"));
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
	assertTrue(cur->sendQuery("select * from testtable order by testinteger"));
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
	assertTrue(cur->sendQuery("select * from testtable order by testinteger"));
	filename=charstring::duplicate(cur->getCacheFileName());
	assertEquals(filename,"cachefile1");
	cur->cacheOff();
	assertTrue(cur->openCachedResultSet(filename));
	assertEquals(cur->getField(7,(uint32_t)0),"8");
	delete[] filename;
	stdoutput.printf("\n");

	stdoutput.printf("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEquals(cur->colCount(),12);
	stdoutput.printf("\n");

	stdoutput.printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEquals(cur->getColumnName(0),"TESTINTEGER");
	assertEquals(cur->getColumnName(1),"TESTSMALLINT");
	assertEquals(cur->getColumnName(2),"TESTDECIMAL");
	assertEquals(cur->getColumnName(3),"TESTNUMERIC");
	assertEquals(cur->getColumnName(4),"TESTFLOAT");
	assertEquals(cur->getColumnName(5),"TESTDOUBLE");
	assertEquals(cur->getColumnName(6),"TESTDATE");
	assertEquals(cur->getColumnName(7),"TESTTIME");
	assertEquals(cur->getColumnName(8),"TESTCHAR");
	assertEquals(cur->getColumnName(9),"TESTVARCHAR");
	assertEquals(cur->getColumnName(10),"TESTTIMESTAMP");
	assertEquals(cur->getColumnName(11),"TESTBLOB");
	cols=cur->getColumnNames();
	assertEquals(cols[0],"TESTINTEGER");
	assertEquals(cols[1],"TESTSMALLINT");
	assertEquals(cols[2],"TESTDECIMAL");
	assertEquals(cols[3],"TESTNUMERIC");
	assertEquals(cols[4],"TESTFLOAT");
	assertEquals(cols[5],"TESTDOUBLE");
	assertEquals(cols[6],"TESTDATE");
	assertEquals(cols[7],"TESTTIME");
	assertEquals(cols[8],"TESTCHAR");
	assertEquals(cols[9],"TESTVARCHAR");
	assertEquals(cols[10],"TESTTIMESTAMP");
	assertEquals(cols[11],"TESTBLOB");
	stdoutput.printf("\n");

	stdoutput.printf("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n");
	cur->setResultSetBufferSize(2);
	cur->cacheToFile("cachefile1");
	cur->setCacheTtl(200);
	assertTrue(cur->sendQuery("select * from testtable order by testinteger"));
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
	assertTrue(cur->sendQuery("select * from testtable order by testinteger"));
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
	assertTrue(con->autoCommitOn());
	assertTrue(cur->sendQuery("insert into testtable values (10,10,10.1,10.1,10.1,10.1,'01-JAN-2010','10:00:00','testchar10','testvarchar10',NULL,NULL)"));
	assertTrue(secondcur->sendQuery("select count(*) from testtable"));
	assertEquals(secondcur->getField(0,(uint32_t)0),"9");
	assertTrue(con->autoCommitOff());
	stdoutput.printf("\n");

	stdoutput.printf("FINISHED SUSPENDED SESSION: \n");
	assertTrue(cur->sendQuery("select * from testtable order by testinteger"));
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
	con->commit();
	cur->sendQuery("delete from testtable");
	con->commit();
	stdoutput.printf("\n");

	// invalid queries...
	stdoutput.printf("INVALID QUERIES: \n");
	assertFalse(cur->sendQuery("select * from testtable1 order by testinteger"));
	assertFalse(cur->sendQuery("select * from testtable1 order by testinteger"));
	assertFalse(cur->sendQuery("select * from testtable1 order by testinteger"));
	assertFalse(cur->sendQuery("select * from testtable1 order by testinteger"));
	stdoutput.printf("\n");
	assertFalse(cur->sendQuery("insert into testtable1 values (1,2,3,4)"));
	assertFalse(cur->sendQuery("insert into testtable1 values (1,2,3,4)"));
	assertFalse(cur->sendQuery("insert into testtable1 values (1,2,3,4)"));
	assertFalse(cur->sendQuery("insert into testtable1 values (1,2,3,4)"));
	stdoutput.printf("\n");
	assertFalse(cur->sendQuery("create table testtable"));
	assertFalse(cur->sendQuery("create table testtable"));
	assertFalse(cur->sendQuery("create table testtable"));
	assertFalse(cur->sendQuery("create table testtable"));
	stdoutput.printf("\n");

	return 0;
}
