// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclientwrapper.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
	#include <windows.h>
	#define sleep(x) Sleep((x)*1000)
	#define snprintf _snprintf
	static int win32gethostname(char *name, int len) {
		DWORD sz=(DWORD)len;
		DWORD i;
		if (!GetComputerNameA(name,&sz)) {
			return -1;
		}
		// GetComputerNameA returns the netbios name uppercased;
		// lowercase it to match gethostname on unix and the test db
		// name, which is the lowercase host name
		for (i=0; i<sz; i++) {
			if (name[i]>='A' && name[i]<='Z') {
				name[i]=name[i]+('a'-'A');
			}
		}
		return 0;
	}
	#define gethostname(name,len) win32gethostname((name),(len))
#else
	#include <strings.h>
	#include <unistd.h>
#endif
#include <stdint.h>

#include "asserts.c"

sqlrcon	con;
sqlrcur	cur;
sqlrcon	secondcon;
sqlrcur	secondcur;

int main(int argc, char **argv) {

	// keep stdout unbuffered so a crash is logged where it actually
	// happens, not at the last flushed line
	setvbuf(stdout,NULL,_IONBF,0);

	const char	*isolationlevels[]={
				"committed read","dirty read",
				"cursor stability","repeatable read",NULL};
	const char	*bindvars[]={"1","2","3","4",
				"5","6","7","8","9","10",
				"11","12","13","14","15","16",NULL};
	const char	*bindvals[]={"t","7","7","7","7",
				"7.5","7.5","7.5","7.5",
				"testchar7","testnchar7",
				"testvarchar7","testnvarchar7",
				"testlvarchar7","01/01/2007",
				"2007-01-01 07:00:00",NULL};
	const char * const *cols;
	const char * const *fields;
	uint32_t	*fieldlens;
	const char	*subvars[]={"var1","var2","var3",NULL};
	int64_t		subvallongs[]={1,2,3};
	const char	*subvalstrings[]={"hi","hello","bye"};
	double		subvaldoubles[]={10.55,10.556,10.5556};
	uint32_t	precs[]={4,5,6};
	uint32_t	scales[]={2,3,4};
	int64_t		numvar;
	const char	*stringvar;
	const char	*nullvar;
	const char	*clobvar;
	uint32_t	clobvarlength;
	const char	*blobvar;
	uint32_t	blobvarlength;
	double		floatvar;
	uint16_t	port;
	char		*socket;
	uint16_t	id;
	char		*filename;
	const char	**il;
	uint64_t	i;

	#define	LARGE_BUFFER_LENGTH	(20*1024)
	char		largebuffer[LARGE_BUFFER_LENGTH+1];


	// hostname
	char	hostname[256];
	char	*dot;
	gethostname(hostname,sizeof(hostname));
	dot=strchr(hostname,'.');
	if (dot) {
		*dot='\0';
	}


	// instantiation
	con=sqlrcon_alloc("sqlrelay",9010,"/tmp/informixtest.socket",
						"testuser","testpassword",0,1);
	cur=sqlrcur_alloc(con);


	// identify
	printf("IDENTIFY: \n");
	assertEqStr(sqlrcon_identify(con),"informix");
	printf("\n");


	// ping
	printf("PING: \n");
	assertTrue(sqlrcon_ping(con));
	printf("\n");


	// transaction state
	printf("TRANSACTION STATE: \n");
	assertEqStr(sqlrcon_getDefaultTransactionModel(con),"implicit");
	assertEqStr(sqlrcon_getTransactionModel(con),"implicit");
	assertTrue(sqlrcon_getInTransaction(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	printf("\n");


	// bind format
	printf("BIND FORMAT: \n");
	assertEqStr(sqlrcon_bindFormat(con),"?");
	printf("\n");


	// nextval format
	printf("NEXTVAL FORMAT: \n");
	assertEqStr(sqlrcon_nextvalFormat(con),"%s.nextval");
	printf("\n");


	// isolation levels
	printf("ISOLATION LEVELS: \n");
	for (il=isolationlevels; *il; il++) {
		// you can set the isolation level, but to get it, you have to
		// have permissions to read from sysmaster:syssqlcurses
		assertTrue(sqlrcon_setIsolationLevel(con,*il));
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
		"	testboolean boolean, "
		"	testsmallint smallint, "
		"	testint integer, "
		"	testbigint bigint, "
		"	testint8 int8, "
		"	testdecimal decimal(10,2), "
		"	testmoney money, "
		"	testsmallfloat smallfloat, "
		"	testfloat float, "
		"	testchar char(40), "
		"	testnchar nchar(40), "
		"	testvarchar varchar(40), "
		"	testnvarchar nvarchar(40), "
		"	testlvarchar lvarchar(40), "
		"	testdate date, "
		"	testdatetime datetime year to second, "
		"	testtext text, "
		"	testbyte byte)"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// insert
	printf("INSERT: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	't', "
		"	1, "
		"	1, "
		"	1, "
		"	1, "
		"	1.5, "
		"	1.5, "
		"	1.5, "
		"	1.5, "
		"	'testchar1', "
		"	'testnchar1', "
		"	'testvarchar1', "
		"	'testnvarchar1', "
		"	'testlvarchar1', "
		"	'01/01/2001', "
		"	'2001-01-01 01:00:00', "
		"	'testtext1', "
		"	null)"));
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
		"	?)");
	assertEqInt(sqlrcur_countBindVariables(cur),18);
	sqlrcur_inputBindString(cur,"1","t");
	sqlrcur_inputBindLong(cur,"2",2);
	sqlrcur_inputBindLong(cur,"3",2);
	sqlrcur_inputBindLong(cur,"4",2);
	sqlrcur_inputBindLong(cur,"5",2);
	sqlrcur_inputBindDouble(cur,"6",2.5,4,2);
	sqlrcur_inputBindDouble(cur,"7",2.5,4,2);
	sqlrcur_inputBindDouble(cur,"8",2.5,4,2);
	sqlrcur_inputBindDouble(cur,"9",2.5,4,2);
	sqlrcur_inputBindString(cur,"10","testchar2");
	sqlrcur_inputBindString(cur,"11","testnchar2");
	sqlrcur_inputBindString(cur,"12","testvarchar2");
	sqlrcur_inputBindString(cur,"13","testnvarchar2");
	sqlrcur_inputBindString(cur,"14","testlvarchar2");
	sqlrcur_inputBindDate(cur,"15",2002,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate(cur,"16",2002,1,1,2,0,0,0,NULL,0);
	sqlrcur_inputBindClob(cur,"17","testtext2",9);
	sqlrcur_inputBindBlob(cur,"18","testbyte2",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindString(cur,"1","t");
	sqlrcur_inputBindLong(cur,"2",3);
	sqlrcur_inputBindLong(cur,"3",3);
	sqlrcur_inputBindLong(cur,"4",3);
	sqlrcur_inputBindLong(cur,"5",3);
	sqlrcur_inputBindDouble(cur,"6",3.5,4,2);
	sqlrcur_inputBindDouble(cur,"7",3.5,4,2);
	sqlrcur_inputBindDouble(cur,"8",3.5,4,2);
	sqlrcur_inputBindDouble(cur,"9",3.5,4,2);
	sqlrcur_inputBindString(cur,"10","testchar3");
	sqlrcur_inputBindString(cur,"11","testnchar3");
	sqlrcur_inputBindString(cur,"12","testvarchar3");
	sqlrcur_inputBindString(cur,"13","testnvarchar3");
	sqlrcur_inputBindString(cur,"14","testlvarchar3");
	sqlrcur_inputBindDate(cur,"15",2003,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate(cur,"16",2003,1,1,3,0,0,0,NULL,0);
	sqlrcur_inputBindClob(cur,"17","testtext3",9);
	sqlrcur_inputBindBlob(cur,"18","testbyte3",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindString(cur,"1","t");
	sqlrcur_inputBindLong(cur,"2",4);
	sqlrcur_inputBindLong(cur,"3",4);
	sqlrcur_inputBindLong(cur,"4",4);
	sqlrcur_inputBindLong(cur,"5",4);
	sqlrcur_inputBindDouble(cur,"6",4.5,4,2);
	sqlrcur_inputBindDouble(cur,"7",4.5,4,2);
	sqlrcur_inputBindDouble(cur,"8",4.5,4,2);
	sqlrcur_inputBindDouble(cur,"9",4.5,4,2);
	sqlrcur_inputBindString(cur,"10","testchar4");
	sqlrcur_inputBindString(cur,"11","testnchar4");
	sqlrcur_inputBindString(cur,"12","testvarchar4");
	sqlrcur_inputBindString(cur,"13","testnvarchar4");
	sqlrcur_inputBindString(cur,"14","testlvarchar4");
	sqlrcur_inputBindDate(cur,"15",2004,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate(cur,"16",2004,1,1,4,0,0,0,NULL,0);
	sqlrcur_inputBindClob(cur,"17","testtext4",9);
	sqlrcur_inputBindBlob(cur,"18","testbyte4",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindString(cur,"1","t");
	sqlrcur_inputBindLong(cur,"2",5);
	sqlrcur_inputBindLong(cur,"3",5);
	sqlrcur_inputBindLong(cur,"4",5);
	sqlrcur_inputBindLong(cur,"5",5);
	sqlrcur_inputBindDouble(cur,"6",5.5,4,2);
	sqlrcur_inputBindDouble(cur,"7",5.5,4,2);
	sqlrcur_inputBindDouble(cur,"8",5.5,4,2);
	sqlrcur_inputBindDouble(cur,"9",5.5,4,2);
	sqlrcur_inputBindString(cur,"10","testchar5");
	sqlrcur_inputBindString(cur,"11","testnchar5");
	sqlrcur_inputBindString(cur,"12","testvarchar5");
	sqlrcur_inputBindString(cur,"13","testnvarchar5");
	sqlrcur_inputBindString(cur,"14","testlvarchar5");
	sqlrcur_inputBindDate(cur,"15",2005,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate(cur,"16",2005,1,1,5,0,0,0,NULL,0);
	sqlrcur_inputBindClob(cur,"17","testtext5",9);
	sqlrcur_inputBindBlob(cur,"18","testbyte5",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindString(cur,"1","t");
	sqlrcur_inputBindLong(cur,"2",6);
	sqlrcur_inputBindLong(cur,"3",6);
	sqlrcur_inputBindLong(cur,"4",6);
	sqlrcur_inputBindLong(cur,"5",6);
	sqlrcur_inputBindDouble(cur,"6",6.5,4,2);
	sqlrcur_inputBindDouble(cur,"7",6.5,4,2);
	sqlrcur_inputBindDouble(cur,"8",6.5,4,2);
	sqlrcur_inputBindDouble(cur,"9",6.5,4,2);
	sqlrcur_inputBindString(cur,"10","testchar6");
	sqlrcur_inputBindString(cur,"11","testnchar6");
	sqlrcur_inputBindString(cur,"12","testvarchar6");
	sqlrcur_inputBindString(cur,"13","testnvarchar6");
	sqlrcur_inputBindString(cur,"14","testlvarchar6");
	sqlrcur_inputBindDate(cur,"15",2006,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate(cur,"16",2006,1,1,6,0,0,0,NULL,0);
	sqlrcur_inputBindClob(cur,"17","testtext6",9);
	sqlrcur_inputBindBlob(cur,"18","testbyte6",9);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// array of input binds by position
	printf("ARRAY OF INPUT BINDS BY POSITION: \n");
	sqlrcur_clearBinds(cur);
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
		"	null, "
		"	null)");
	sqlrcur_inputBindStrings(cur,bindvars,bindvals);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// input bind by position with validation
	printf("INPUT BIND BY POSITION WITH VALIDATION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindString(cur,"1","t");
	sqlrcur_inputBindLong(cur,"2",8);
	sqlrcur_inputBindLong(cur,"3",8);
	sqlrcur_inputBindLong(cur,"4",8);
	sqlrcur_inputBindLong(cur,"5",8);
	sqlrcur_inputBindDouble(cur,"6",8.5,4,2);
	sqlrcur_inputBindDouble(cur,"7",8.5,4,2);
	sqlrcur_inputBindDouble(cur,"8",8.5,4,2);
	sqlrcur_inputBindDouble(cur,"9",8.5,4,2);
	sqlrcur_inputBindString(cur,"10","testchar8");
	sqlrcur_inputBindString(cur,"11","testnchar8");
	sqlrcur_inputBindString(cur,"12","testvarchar8");
	sqlrcur_inputBindString(cur,"13","testnvarchar8");
	sqlrcur_inputBindString(cur,"14","testlvarchar8");
	sqlrcur_inputBindDate(cur,"15",2008,1,1,-1,-1,-1,-1,NULL,0);
	sqlrcur_inputBindDate(cur,"16",2008,1,1,8,0,0,0,NULL,0);
	sqlrcur_inputBindClob(cur,"17","testtext8",9);
	sqlrcur_inputBindBlob(cur,"18","testbyte8",9);
	sqlrcur_validateBinds(cur);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// input bind by name
	// informix doesn't support bind by name


	// array of input binds by name
	// informix doesn't support bind by name


	// input bind by name with validation
	// informix doesn't support bind by name


	// select
	printf("SELECT: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	printf("\n");


	// column count
	printf("COLUMN COUNT: \n");
	assertEqInt(sqlrcur_colCount(cur),18);
	printf("\n");


	// column names
	printf("COLUMN NAMES: \n");
	assertEqStr(sqlrcur_getColumnName(cur,0),"testboolean");
	assertEqStr(sqlrcur_getColumnName(cur,1),"testsmallint");
	assertEqStr(sqlrcur_getColumnName(cur,2),"testint");
	assertEqStr(sqlrcur_getColumnName(cur,3),"testbigint");
	assertEqStr(sqlrcur_getColumnName(cur,4),"testint8");
	assertEqStr(sqlrcur_getColumnName(cur,5),"testdecimal");
	assertEqStr(sqlrcur_getColumnName(cur,6),"testmoney");
	assertEqStr(sqlrcur_getColumnName(cur,7),"testsmallfloat");
	assertEqStr(sqlrcur_getColumnName(cur,8),"testfloat");
	assertEqStr(sqlrcur_getColumnName(cur,9),"testchar");
	assertEqStr(sqlrcur_getColumnName(cur,10),"testnchar");
	assertEqStr(sqlrcur_getColumnName(cur,11),"testvarchar");
	assertEqStr(sqlrcur_getColumnName(cur,12),"testnvarchar");
	assertEqStr(sqlrcur_getColumnName(cur,13),"testlvarchar");
	assertEqStr(sqlrcur_getColumnName(cur,14),"testdate");
	assertEqStr(sqlrcur_getColumnName(cur,15),"testdatetime");
	assertEqStr(sqlrcur_getColumnName(cur,16),"testtext");
	assertEqStr(sqlrcur_getColumnName(cur,17),"testbyte");
	cols=sqlrcur_getColumnNames(cur);
	assertEqStr(cols[0],"testboolean");
	assertEqStr(cols[1],"testsmallint");
	assertEqStr(cols[2],"testint");
	assertEqStr(cols[3],"testbigint");
	assertEqStr(cols[4],"testint8");
	assertEqStr(cols[5],"testdecimal");
	assertEqStr(cols[6],"testmoney");
	assertEqStr(cols[7],"testsmallfloat");
	assertEqStr(cols[8],"testfloat");
	assertEqStr(cols[9],"testchar");
	assertEqStr(cols[10],"testnchar");
	assertEqStr(cols[11],"testvarchar");
	assertEqStr(cols[12],"testnvarchar");
	assertEqStr(cols[13],"testlvarchar");
	assertEqStr(cols[14],"testdate");
	assertEqStr(cols[15],"testdatetime");
	assertEqStr(cols[16],"testtext");
	assertEqStr(cols[17],"testbyte");
	printf("\n");


	// column types
	printf("COLUMN TYPES: \n");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),"BOOLEAN");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testboolean"),"BOOLEAN");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,1),"SMALLINT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testsmallint"),"SMALLINT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,2),"INTEGER");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testint"),"INTEGER");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,3),"BIGINT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testbigint"),"BIGINT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,4),"INT8");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testint8"),"INT8");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,5),"DECIMAL");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testdecimal"),"DECIMAL");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,6),"MONEY");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testmoney"),"MONEY");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,7),"SMALLFLOAT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testsmallfloat"),
							"SMALLFLOAT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,8),"FLOAT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testfloat"),"FLOAT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,9),"CHAR");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testchar"),"CHAR");
	// informix reports nchar as char, with no way to tell them apart
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,10),"CHAR");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testnchar"),"CHAR");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,11),"VARCHAR");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testvarchar"),"VARCHAR");
	// informix reports nvarchar as varchar, with no way to tell them apart
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,12),"VARCHAR");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testnvarchar"),"VARCHAR");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,13),"LVARCHAR");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testlvarchar"),"LVARCHAR");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,14),"DATE");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testdate"),"DATE");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,15),"DATETIME");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testdatetime"),
							"DATETIME");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,16),"TEXT");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testtext"),"TEXT");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,17),"BYTE");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"testbyte"),"BYTE");
	printf("\n");


	// column length
	printf("COLUMN LENGTH: \n");
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),1);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testboolean"),1);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,1),5);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testsmallint"),5);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,2),10);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testint"),10);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,3),20);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testbigint"),20);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,4),20);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testint8"),20);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,5),10);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testdecimal"),10);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,6),16);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testmoney"),16);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,7),7);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testsmallfloat"),7);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,8),15);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testfloat"),15);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,9),40);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testchar"),40);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,10),40);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testnchar"),40);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,11),40);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testvarchar"),40);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,12),40);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testnvarchar"),40);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,13),40);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testlvarchar"),40);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,14),10);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testdate"),10);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,15),19);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testdatetime"),19);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,16),2147483647);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testtext"),
							2147483647);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,17),2147483647);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"testbyte"),
							2147483647);
	printf("\n");


	// longest column
	printf("LONGEST COLUMN: \n");
	assertEqInt(sqlrcur_getLongestByIndex(cur,0),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"testboolean"),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,1),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"testsmallint"),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,2),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"testint"),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,3),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"testbigint"),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,4),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"testint8"),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,5),4);
	assertEqInt(sqlrcur_getLongestByName(cur,"testdecimal"),4);
	assertEqInt(sqlrcur_getLongestByIndex(cur,6),4);
	assertEqInt(sqlrcur_getLongestByName(cur,"testmoney"),4);
	assertEqInt(sqlrcur_getLongestByIndex(cur,7),3);
	assertEqInt(sqlrcur_getLongestByName(cur,"testsmallfloat"),3);
	assertEqInt(sqlrcur_getLongestByIndex(cur,8),3);
	assertEqInt(sqlrcur_getLongestByName(cur,"testfloat"),3);
	assertEqInt(sqlrcur_getLongestByIndex(cur,9),40);
	assertEqInt(sqlrcur_getLongestByName(cur,"testchar"),40);
	assertEqInt(sqlrcur_getLongestByIndex(cur,10),40);
	assertEqInt(sqlrcur_getLongestByName(cur,"testnchar"),40);
	assertEqInt(sqlrcur_getLongestByIndex(cur,11),12);
	assertEqInt(sqlrcur_getLongestByName(cur,"testvarchar"),12);
	assertEqInt(sqlrcur_getLongestByIndex(cur,12),13);
	assertEqInt(sqlrcur_getLongestByName(cur,"testnvarchar"),13);
	assertEqInt(sqlrcur_getLongestByIndex(cur,13),13);
	assertEqInt(sqlrcur_getLongestByName(cur,"testlvarchar"),13);
	assertEqInt(sqlrcur_getLongestByIndex(cur,14),10);
	assertEqInt(sqlrcur_getLongestByName(cur,"testdate"),10);
	assertEqInt(sqlrcur_getLongestByIndex(cur,15),19);
	assertEqInt(sqlrcur_getLongestByName(cur,"testdatetime"),19);
	assertEqInt(sqlrcur_getLongestByIndex(cur,16),9);
	assertEqInt(sqlrcur_getLongestByName(cur,"testtext"),9);
	assertEqInt(sqlrcur_getLongestByIndex(cur,17),9);
	assertEqInt(sqlrcur_getLongestByName(cur,"testbyte"),9);
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
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,5),"1.50");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,6),"1.50");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,7),"1.5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,8),"1.5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,9),
			"testchar1                               ");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,10),
			"testnchar1                              ");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,11),"testvarchar1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,12),"testnvarchar1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,13),"testlvarchar1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,14),"2001-01-01");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,15),
			"2001-01-01 01:00:00");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,16),"testtext1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,17),"");
	printf("\n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,2),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,3),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,4),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,5),"8.50");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,6),"8.50");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,7),"8.5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,8),"8.5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,9),
			"testchar8                               ");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,10),
			"testnchar8                              ");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,11),"testvarchar8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,12),"testnvarchar8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,13),"testlvarchar8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,14),"2008-01-01");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,15),
			"2008-01-01 08:00:00");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,16),"");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,17),"");
	printf("\n");


	// field lengths by index
	printf("FIELD LENGTHS BY INDEX: \n");
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,1),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,2),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,3),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,4),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,5),4);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,6),4);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,7),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,8),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,9),40);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,10),40);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,11),12);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,12),13);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,14),10);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,15),19);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,16),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,17),0);
	printf("\n");
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,0),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,1),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,2),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,3),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,4),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,5),4);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,6),4);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,7),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,8),3);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,9),40);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,10),40);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,11),12);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,12),13);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,14),10);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,15),19);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,16),0);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,17),0);
	printf("\n");


	// fields by name
	printf("FIELDS BY NAME: \n");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testboolean"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testsmallint"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testint"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testbigint"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testint8"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testdecimal"),"1.50");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testmoney"),"1.50");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testsmallfloat"),"1.5");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testfloat"),"1.5");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testchar"),
			"testchar1                               ");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testnchar"),
			"testnchar1                              ");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testvarchar"),
			"testvarchar1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testnvarchar"),
			"testnvarchar1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testlvarchar"),
			"testlvarchar1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testdate"),"2001-01-01");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testdatetime"),
			"2001-01-01 01:00:00");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testtext"),"testtext1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testbyte"),"");
	printf("\n");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testboolean"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testsmallint"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testint"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testbigint"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testint8"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testdecimal"),"8.50");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testmoney"),"8.50");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testsmallfloat"),"8.5");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testfloat"),"8.5");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testchar"),
			"testchar8                               ");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testnchar"),
			"testnchar8                              ");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testvarchar"),
			"testvarchar8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testnvarchar"),
			"testnvarchar8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testlvarchar"),
			"testlvarchar8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testdate"),"2008-01-01");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testdatetime"),
			"2008-01-01 08:00:00");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testtext"),"");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"testbyte"),"");
	printf("\n");


	// field lengths by name
	printf("FIELD LENGTHS BY NAME: \n");
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testboolean"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testsmallint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testbigint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testint8"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testdecimal"),4);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testmoney"),4);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testsmallfloat"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testchar"),40);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testnchar"),40);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testvarchar"),12);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testnvarchar"),13);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testlvarchar"),13);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testdate"),10);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testdatetime"),19);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testtext"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testbyte"),0);
	printf("\n");
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testboolean"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testsmallint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testbigint"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testint8"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testdecimal"),4);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testmoney"),4);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testsmallfloat"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testfloat"),3);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testchar"),40);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testnchar"),40);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testvarchar"),12);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testnvarchar"),13);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testlvarchar"),13);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testdate"),10);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testdatetime"),19);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testtext"),0);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"testbyte"),0);
	printf("\n");


	// fields by array
	printf("FIELDS BY ARRAY: \n");
	fields=sqlrcur_getRow(cur,0);
	assertEqStr(fields[0],"1");
	assertEqStr(fields[1],"1");
	assertEqStr(fields[2],"1");
	assertEqStr(fields[3],"1");
	assertEqStr(fields[4],"1");
	assertEqStr(fields[5],"1.50");
	assertEqStr(fields[6],"1.50");
	assertEqStr(fields[7],"1.5");
	assertEqStr(fields[8],"1.5");
	assertEqStr(fields[9],"testchar1                               ");
	assertEqStr(fields[10],"testnchar1                              ");
	assertEqStr(fields[11],"testvarchar1");
	assertEqStr(fields[12],"testnvarchar1");
	assertEqStr(fields[13],"testlvarchar1");
	assertEqStr(fields[14],"2001-01-01");
	assertEqStr(fields[15],"2001-01-01 01:00:00");
	assertEqStr(fields[16],"testtext1");
	assertEqStr(fields[17],"");
	printf("\n");


	// field lengths by array
	printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=sqlrcur_getRowLengths(cur,0);
	assertEqInt(fieldlens[0],1);
	assertEqInt(fieldlens[1],1);
	assertEqInt(fieldlens[2],1);
	assertEqInt(fieldlens[3],1);
	assertEqInt(fieldlens[4],1);
	assertEqInt(fieldlens[5],4);
	assertEqInt(fieldlens[6],4);
	assertEqInt(fieldlens[7],3);
	assertEqInt(fieldlens[8],3);
	assertEqInt(fieldlens[9],40);
	assertEqInt(fieldlens[10],40);
	assertEqInt(fieldlens[11],12);
	assertEqInt(fieldlens[12],13);
	assertEqInt(fieldlens[14],10);
	assertEqInt(fieldlens[15],19);
	assertEqInt(fieldlens[16],9);
	assertEqInt(fieldlens[17],0);
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
		"	testsmallint "));
	assertEqInt(sqlrcur_getResultSetBufferSize(cur),2);
	printf("\n");
	assertEqInt(sqlrcur_firstRowIndex(cur),0);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),2);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,1,1),"2");
	assertEqStr(sqlrcur_getFieldByIndex(cur,2,1),"3");
	printf("\n");
	assertEqInt(sqlrcur_firstRowIndex(cur),2);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),4);
	assertEqStr(sqlrcur_getFieldByIndex(cur,6,1),"7");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8");
	printf("\n");
	assertEqInt(sqlrcur_firstRowIndex(cur),6);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),8);
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,1),NULL);
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
		"	testsmallint "));
	assertEqStr(sqlrcur_getColumnName(cur,1),NULL);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,1),0);
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,1),NULL);
	sqlrcur_getColumnInfo(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	assertEqStr(sqlrcur_getColumnName(cur,1),"testsmallint");
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,1),5);
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,1),"SMALLINT");
	printf("\n");


	// suspended session
	printf("SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	free(socket);
	printf("\n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,1,1),"2");
	assertEqStr(sqlrcur_getFieldByIndex(cur,2,1),"3");
	assertEqStr(sqlrcur_getFieldByIndex(cur,3,1),"4");
	assertEqStr(sqlrcur_getFieldByIndex(cur,4,1),"5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,5,1),"6");
	assertEqStr(sqlrcur_getFieldByIndex(cur,6,1),"7");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8");
	printf("\n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	free(socket);
	printf("\n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,1,1),"2");
	assertEqStr(sqlrcur_getFieldByIndex(cur,2,1),"3");
	assertEqStr(sqlrcur_getFieldByIndex(cur,3,1),"4");
	assertEqStr(sqlrcur_getFieldByIndex(cur,4,1),"5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,5,1),"6");
	assertEqStr(sqlrcur_getFieldByIndex(cur,6,1),"7");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8");
	printf("\n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	free(socket);
	printf("\n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,1,1),"2");
	assertEqStr(sqlrcur_getFieldByIndex(cur,2,1),"3");
	assertEqStr(sqlrcur_getFieldByIndex(cur,3,1),"4");
	assertEqStr(sqlrcur_getFieldByIndex(cur,4,1),"5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,5,1),"6");
	assertEqStr(sqlrcur_getFieldByIndex(cur,6,1),"7");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8");
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
		"	testsmallint "));
	assertEqStr(sqlrcur_getFieldByIndex(cur,2,1),"3");
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
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8");
	printf("\n");
	assertEqInt(sqlrcur_firstRowIndex(cur),6);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),8);
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,1),NULL);
	printf("\n");
	assertEqInt(sqlrcur_firstRowIndex(cur),8);
	assertTrue(sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),8);
	sqlrcur_setResultSetBufferSize(cur,0);
	printf("\n");


	// cached result set
	printf("CACHED RESULT SET: \n");
	sqlrcur_cacheToFile(cur,"cachefile1-informix");
	sqlrcur_setCacheTtl(cur,200);
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqStr(filename,"cachefile1-informix");
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8");
	free(filename);
	printf("\n");


	// column count for cached result set
	printf("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqInt(sqlrcur_colCount(cur),18);
	printf("\n");


	// column names for cached result set
	printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqStr(sqlrcur_getColumnName(cur,0),"testboolean");
	assertEqStr(sqlrcur_getColumnName(cur,1),"testsmallint");
	assertEqStr(sqlrcur_getColumnName(cur,2),"testint");
	assertEqStr(sqlrcur_getColumnName(cur,3),"testbigint");
	assertEqStr(sqlrcur_getColumnName(cur,4),"testint8");
	assertEqStr(sqlrcur_getColumnName(cur,5),"testdecimal");
	assertEqStr(sqlrcur_getColumnName(cur,6),"testmoney");
	assertEqStr(sqlrcur_getColumnName(cur,7),"testsmallfloat");
	assertEqStr(sqlrcur_getColumnName(cur,8),"testfloat");
	assertEqStr(sqlrcur_getColumnName(cur,9),"testchar");
	assertEqStr(sqlrcur_getColumnName(cur,10),"testnchar");
	assertEqStr(sqlrcur_getColumnName(cur,11),"testvarchar");
	assertEqStr(sqlrcur_getColumnName(cur,12),"testnvarchar");
	assertEqStr(sqlrcur_getColumnName(cur,13),"testlvarchar");
	assertEqStr(sqlrcur_getColumnName(cur,14),"testdate");
	assertEqStr(sqlrcur_getColumnName(cur,15),"testdatetime");
	assertEqStr(sqlrcur_getColumnName(cur,16),"testtext");
	assertEqStr(sqlrcur_getColumnName(cur,17),"testbyte");
	cols=sqlrcur_getColumnNames(cur);
	assertEqStr(cols[0],"testboolean");
	assertEqStr(cols[1],"testsmallint");
	assertEqStr(cols[2],"testint");
	assertEqStr(cols[3],"testbigint");
	assertEqStr(cols[4],"testint8");
	assertEqStr(cols[5],"testdecimal");
	assertEqStr(cols[6],"testmoney");
	assertEqStr(cols[7],"testsmallfloat");
	assertEqStr(cols[8],"testfloat");
	assertEqStr(cols[9],"testchar");
	assertEqStr(cols[10],"testnchar");
	assertEqStr(cols[11],"testvarchar");
	assertEqStr(cols[12],"testnvarchar");
	assertEqStr(cols[13],"testlvarchar");
	assertEqStr(cols[14],"testdate");
	assertEqStr(cols[15],"testdatetime");
	assertEqStr(cols[16],"testtext");
	assertEqStr(cols[17],"testbyte");
	printf("\n");


	// cached result set with result set buffer size
	printf("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile1-informix");
	sqlrcur_setCacheTtl(cur,200);
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqStr(filename,"cachefile1-informix");
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,1),NULL);
	sqlrcur_setResultSetBufferSize(cur,0);
	free(filename);
	printf("\n");


	// from one cache file to another
	printf("FROM ONE CACHE FILE TO ANOTHER: \n");
	sqlrcur_cacheToFile(cur,"cachefile2-informix");
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile1-informix"));
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile2-informix"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,1),NULL);
	printf("\n");


	// from one cache file to another with result set buffer size
	printf("FROM ONE CACHE FILE TO ANOTHER "
				"WITH RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile2-informix");
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile1-informix"));
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile2-informix"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,1),NULL);
	sqlrcur_setResultSetBufferSize(cur,0);
	printf("\n");


	// cached result set with suspend and result set buffer size
	printf("CACHED RESULT SET WITH SUSPEND "
				"AND RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile1-informix");
	sqlrcur_setCacheTtl(cur,200);
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	assertEqStr(sqlrcur_getFieldByIndex(cur,2,1),"3");
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqStr(filename,"cachefile1-informix");
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
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8");
	printf("\n");
	assertEqInt(sqlrcur_firstRowIndex(cur),6);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),8);
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,1),NULL);
	printf("\n");
	assertEqInt(sqlrcur_firstRowIndex(cur),8);
	assertTrue(sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),8);
	sqlrcur_cacheOff(cur);
	printf("\n");
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,1),NULL);
	sqlrcur_setResultSetBufferSize(cur,0);
	free(filename);
	printf("\n");


	// finished suspended session
	printf("FINISHED SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select * from testtable order by testint"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,4,1),"5");
	assertEqStr(sqlrcur_getFieldByIndex(cur,5,1),"6");
	assertEqStr(sqlrcur_getFieldByIndex(cur,6,1),"7");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),"8");
	id=sqlrcur_getResultSetId(cur);
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	free(socket);
	assertTrue(sqlrcur_resumeResultSet(cur,id));
	assertEqStr(sqlrcur_getFieldByIndex(cur,4,1),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,5,1),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,6,1),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),NULL);
	printf("\n");


	// nested selects
	printf("NESTED SELECTS: \n");
	sqlrcur_setResultSetBufferSize(cur,1);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable"));
	secondcur=sqlrcur_alloc(con);
	sqlrcur_setResultSetBufferSize(secondcur,1);
	for (i=0; sqlrcur_getRow(cur,i); i++) {
		assertTrue(sqlrcur_sendQuery(
				secondcur,"select * from testtable"));
	}
	sqlrcur_free(secondcur);
	sqlrcur_setResultSetBufferSize(cur,0);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// reset transaction state
	printf("RESET TRANSACTION STATE: \n");
	assertTrue(sqlrcon_commit(con));
	assertEqStr(sqlrcon_getTransactionModel(con),"implicit");
	assertFalse(sqlrcon_getAutoCommit(con));
	printf("\n");


	// transaction behavior - implicit
	// Informix has no MVCC option -- the isolation level is either dirty
	// reads (where the second connection sees uncommitted rows) or
	// committed read (where it blocks or errors on locked rows) -- so
	// the visibility assertions below may need to be revisited
	printf("TRANSACTION BEHAVIOR - implicit: \n");
	assertTrue(sqlrcon_setTransactionModel(con,"implicit"));
	assertEqStr(sqlrcon_getTransactionModel(con),"implicit");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable (col1 integer)"));
	// informix DDL is transactional in logged mode; commit so the table
	// is visible to the second connection (commit implicitly starts a
	// new tx)
	assertTrue(sqlrcon_commit(con));
	secondcon=sqlrcon_alloc("sqlrelay",9010,"/tmp/informixtest.socket",
						"testuser","testpassword",0,1);
	secondcur=sqlrcur_alloc(secondcon);
	// Informix has no MVCC; under default committed-read isolation,
	// secondcur's catalog/data read errors with "Cannot get system
	// information for table" while cur holds row locks from the
	// in-flight tx.  Use dirty-read on secondcur so it sees the
	// uncommitted writes — the test then verifies dirty-read
	// semantics instead of MVCC visibility.
	assertTrue(sqlrcur_sendQuery(secondcur,"set isolation to dirty read"));
	// session is in a transaction; insert is visible via dirty read
	assertTrue(sqlrcon_getInTransaction(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (1)"));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
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
	// see note above re: informix dirty-read workaround
	assertTrue(sqlrcur_sendQuery(secondcur,"set isolation to dirty read"));
	// begin starts a new transaction; insert is visible via dirty read
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (1)"));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"1");
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
	// see note in - implicit section re: informix dirty-read workaround
	assertTrue(sqlrcur_sendQuery(secondcur,"set isolation to dirty read"));
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
	// explicitly commits/rollbacks the tx (mysql-native semantic).
	// dirty-read on secondcur sees the in-flight insert (count=2)
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (3)"));
	assertTrue(sqlrcon_autoCommitOn(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"2");
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
	// dirty-read on secondcur sees the in-flight insert (count=5)
	assertTrue(sqlrcon_autoCommitOn(con));
	assertTrue(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcon_begin(con));
	assertTrue(sqlrcur_sendQuery(cur,"insert into testtable values (7)"));
	assertTrue(sqlrcon_autoCommitOff(con));
	assertFalse(sqlrcon_getAutoCommit(con));
	assertTrue(sqlrcon_getInTransaction(con));
	assertTrue(sqlrcur_sendQuery(secondcur,"select count(*) from testtable"));
	assertEqStr(sqlrcur_getFieldByIndex(secondcur,0,0),"5");
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
	assertEqStr(sqlrcon_getTransactionModel(con),"implicit");
	assertFalse(sqlrcon_getAutoCommit(con));
	printf("\n");


	// individual substitutions
	printf("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,
		"select "
		"	$(var1), "
		"	'$(var2)', "
		"	'$(var3)' "
		"from "
		"	sysmaster:sysdual ");
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
	sqlrcur_prepareQuery(cur,
		"select "
		"	'$(var1)', "
		"	'$(var2)', "
		"	'$(var3)' "
		"from "
		"	sysmaster:sysdual ");
	sqlrcur_subStrings(cur,subvars,subvalstrings);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"hi");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"bye");
	printf("\n");
	sqlrcur_prepareQuery(cur,
		"select "
		"	$(var1), "
		"	$(var2), "
		"	$(var3) "
		"from "
		"	sysmaster:sysdual ");
	sqlrcur_subLongs(cur,subvars,subvallongs);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"2");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"3");
	printf("\n");
	sqlrcur_prepareQuery(cur,
		"select "
		"	$(var1), "
		"	$(var2), "
		"	$(var3) "
		"from "
		"	sysmaster:sysdual ");
	sqlrcur_subDoubles(cur,subvars,subvaldoubles,precs,scales);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"10.55");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"10.556");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"10.5556");
	printf("\n");


	// nulls as nulls
	printf("NULLS AS NULLS: \n");
	sqlrcur_getNullsAsNulls(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"select NULL::int,1,NULL::int from sysmaster:sysdual"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"select NULL::int,1,NULL::int from sysmaster:sysdual"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"");
	printf("\n");


	// output bind by position
	printf("OUTPUT BIND BY POSITION: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	sqlrcur_getNullsAsNulls(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc("
		"	out out1 int, "
		"	out out2 varchar(20), "
		"	out out3 float, "
		"	out out4 varchar(20)) "
		"let out1 = 1; "
		"	let out2 = 'hello'; "
		"	let out3 = 2.5; "
		"	let out4 = null; "
		"end procedure;"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,"{call testproc(?,?,?,?)}");
	assertEqInt(sqlrcur_countBindVariables(cur),4);
	sqlrcur_defineOutputBindInteger(cur,"1");
	sqlrcur_defineOutputBindString(cur,"2",20);
	sqlrcur_defineOutputBindDouble(cur,"3");
	sqlrcur_defineOutputBindString(cur,"4",20);
	assertTrue(sqlrcur_executeQuery(cur));
	numvar=sqlrcur_getOutputBindInteger(cur,"1");
	stringvar=sqlrcur_getOutputBindString(cur,"2");
	floatvar=sqlrcur_getOutputBindDouble(cur,"3");
	nullvar=sqlrcur_getOutputBindString(cur,"4");
	assertEqInt(numvar,1);
	assertEqStr(stringvar,"hello");
	assertEqDbl(floatvar,2.5);
	assertEqStr(nullvar,NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// output bind by name
	// informix doesn't support bind by name


	// output bind by name with validation
	// informix doesn't support bind by name


	// lob output bind
	printf("LOB OUTPUT BIND: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testclob clob, "
		"	testblob blob)"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,"insert into testtable values (?,?)");
	sqlrcur_inputBindClob(cur,"1","hello",5);
	sqlrcur_inputBindBlob(cur,"2","hello",5);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc("
		"	out out1 clob, "
		"	out out2 blob) "
		"select testclob, testblob "
		"	into out1, out2 "
		"	from testtable; "
		"end procedure;"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,"{call testproc(?,?)}");
	sqlrcur_defineOutputBindClob(cur,"1");
	sqlrcur_defineOutputBindBlob(cur,"2");
	assertTrue(sqlrcur_executeQuery(cur));
	clobvar=sqlrcur_getOutputBindClob(cur,"1");
	clobvarlength=sqlrcur_getOutputBindLength(cur,"1");
	blobvar=sqlrcur_getOutputBindBlob(cur,"2");
	blobvarlength=sqlrcur_getOutputBindLength(cur,"2");
	assertEqStrLen(clobvar,"hello",5);
	assertEqInt(clobvarlength,5);
	assertEqStrLen(blobvar,"hello",5);
	assertEqInt(blobvarlength,5);
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// long output bind
	printf("LONG OUTPUT BIND: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc("
		"	in1 clob, "
		"	out out1 clob) "
		"let out1 = in1; "
		"	end procedure;"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,"{call testproc(?,?)}");
	for (i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	sqlrcur_inputBindClob(cur,"1",largebuffer,LARGE_BUFFER_LENGTH);
	sqlrcur_defineOutputBindClob(cur,"2");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindLength(cur,"2"),
			LARGE_BUFFER_LENGTH);
	assertEqStr(sqlrcur_getOutputBindClob(cur,"2"),largebuffer);
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// negative input bind
	printf("NEGATIVE INPUT BIND: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable (testval int)"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,"insert into testtable values (?)");
	sqlrcur_inputBindLong(cur,"1",-1);
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"select testval from testtable"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"testval"),"-1");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// bind validation
	// informix doesn't support bind by name

	// rebinding
	printf("REBINDING: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc("
		"	in1 int, "
		"	out out1 int) "
		"let out1 = in1; "
		"end procedure;"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,"{call testproc(?,?)}");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_defineOutputBindInteger(cur,"2");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"2"),1);
	sqlrcur_inputBindLong(cur,"1",2);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"2"),2);
	sqlrcur_inputBindLong(cur,"1",3);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"2"),3);
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// reexecute
	printf("REEXECUTE: \n");
	sqlrcur_prepareQuery(cur,
		"select 1 from sysmaster:sysdual");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	printf("\n");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	printf("\n");
	sqlrcur_prepareQuery(cur,
		"select ?::int from sysmaster:sysdual");
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
	printf("STORED PROCEDURE RETURNING NO VALUE: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc("
		"	in1 int, "
		"	in2 float, "
		"	in3 varchar(20)) "
		"end procedure;"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,"{call testproc(?,?,?)}");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",2.5,2,1);
	sqlrcur_inputBindString(cur,"3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// stored procedure returning single value
	printf("STORED PROCEDURE RETURNING SINGLE VALUE: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc("
		"	in1 int, "
		"	in2 float, "
		"	in3 varchar(20), "
		"	out out1 int) "
		"let out1 = in1; "
		"end procedure;"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,"{call testproc(?,?,?,?)}");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",2.5,2,1);
	sqlrcur_inputBindString(cur,"3","hello");
	sqlrcur_defineOutputBindInteger(cur,"4");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"4"),1);
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// stored procedure returning multiple values
	printf("STORED PROCEDURE RETURNING MULTIPLE VALUES: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc("
		"	in1 int, "
		"	in2 float, "
		"	in3 varchar(20), "
		"	out out1 int, "
		"	out out2 float, "
		"	out out3 varchar(20)) "
		"let out1 = in1; "
		"	let out2 = in2; "
		"	let out3 = in3; "
		"end procedure;"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,"{call testproc(?,?,?,?,?,?)}");
	sqlrcur_inputBindLong(cur,"1",1);
	sqlrcur_inputBindDouble(cur,"2",2.5,2,1);
	sqlrcur_inputBindString(cur,"3","hello");
	sqlrcur_defineOutputBindInteger(cur,"4");
	sqlrcur_defineOutputBindDouble(cur,"5");
	sqlrcur_defineOutputBindString(cur,"6",20);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"4"),1);
	assertEqDbl(sqlrcur_getOutputBindDouble(cur,"5"),2.5);
	assertEqStr(sqlrcur_getOutputBindString(cur,"6"),"hello");
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// stored procedure returning result set
	printf("STORED PROCEDURE RETURNING RESULT SET: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc() "
		"returning boolean, smallint, varchar(40); "
		"	define out1 boolean; "
		"	define out2 smallint; "
		"	define out3 varchar(40); "
		"	foreach "
		"		select "
		"			testboolean, "
		"			testsmallint, "
		"			testvarchar "
		"		into out1,out2,out3 "
		"		from ( "
		"			select "
		"				't' as testboolean, "
		"				1 as testsmallint, "
		"				'1' as testvarchar "
		"			from "
		"				sysmaster:sysdual "
		"			union "
		"			select "
		"				't' as testboolean, "
		"				2 as testsmallint, "
		"				'2' as testvarchar "
		"			from "
		"				sysmaster:sysdual "
		"			union "
		"			select "
		"				't' as testboolean, "
		"				3 as testsmallint, "
		"				'3' as testvarchar "
		"			from "
		"				sysmaster:sysdual "
		"			union "
		"			select "
		"				't' as testboolean, "
		"				4 as testsmallint, "
		"				'4' as testvarchar "
		"			from "
		"				sysmaster:sysdual "
		"			union "
		"			select "
		"				't' as testboolean, "
		"				5 as testsmallint, "
		"				'5' as testvarchar "
		"			from "
		"				sysmaster:sysdual "
		"			union "
		"			select "
		"				't' as testboolean, "
		"				6 as testsmallint, "
		"				'6' as testvarchar "
		"			from "
		"				sysmaster:sysdual "
		"			union "
		"			select "
		"				't' as testboolean, "
		"				7 as testsmallint, "
		"				'7' as testvarchar "
		"			from "
		"				sysmaster:sysdual "
		"			union "
		"			select "
		"				't' as testboolean, "
		"				8 as testsmallint, "
		"				'8' as testvarchar "
		"			from "
		"				sysmaster:sysdual "
		"		) "
		"	return out1,out2,out3 "
		"	with resume; "
		"	end foreach; "
		"	end procedure;"));
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_sendQuery(cur,"{call testproc()}"));
	assertEqInt(sqlrcur_rowCount(cur),8);
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// null and empty lobs
	printf("NULL AND EMPTY LOBS: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	sqlrcur_getNullsAsNulls(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testclob1 clob, "
		"	testclob2 clob, "
		"	testblob1 blob, "
		"	testblob2 blob)"));
	assertTrue(sqlrcon_commit(con));
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
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// long lobs
	printf("LONG LOBS: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testtext text, "
		"	testbyte byte)"));
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,
		"insert into testtable values (?,?)");
	for (i=0; i<LARGE_BUFFER_LENGTH; i++) {
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
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"testbyte"),
			LARGE_BUFFER_LENGTH);
	assertEqStrLen(sqlrcur_getFieldByName(cur,0,"testbyte"),
			largebuffer,LARGE_BUFFER_LENGTH);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// temporary tables
	printf("TEMPORARY TABLES: \n");
	sqlrcur_sendQuery(cur,"drop table temptable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create temp table temptable (col1 int)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into temptable values (1)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"select count(*) from temptable"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	sqlrcon_endSession(con);
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,
		"select count(*) from temptable"));
	printf("\n");


	// encoded binary data
	// informix doesn't support encoded binary data


	// quotes
	printf("QUOTES: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable (col1 varchar(4))"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into testtable values ('''''')"));
	assertTrue(sqlrcur_sendQuery(cur,
		"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),2);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"''");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// last insert id
	printf("LAST INSERT ID: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
			"create table testtable "
			"	(col1 serial primary key, "
			"	col2 int)"));
	assertTrue(sqlrcur_sendQuery(cur,
			"insert into testtable (col2) values (1)"));
	assertEqInt(sqlrcon_getLastInsertId(con),1);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// database is schema
	printf("DATABASE IS SCHEMA: \n");
	assertFalse(sqlrcon_getDatabaseIsSchema(con));
	printf("\n");


	// catalog list
	printf("CATALOG LIST: \n");
	assertTrue(sqlrcur_getCatalogList(cur,NULL));
	assertEqStr(sqlrcur_getColumnName(cur,0),"Database");
	assertInResultSet(cur,"Database",hostname);
	printf("\n");


	// schema list
	printf("SCHEMA LIST: \n");
	// informix requires that a table exist that is
	// owned by a user for the user to be reported
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 integer, "
		"	col2 integer)"));
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_getSchemaList(cur,NULL));
	assertEqStr(sqlrcur_getColumnName(cur,0),"Database");
	assertInResultSet(cur,"Database","testuser");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	assertTrue(sqlrcon_commit(con));
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
		"	col1 integer, "
		"	col2 integer)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable2 ("
		"	col1 integer, "
		"	col2 integer)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable3 ("
		"	col1 integer, "
		"	col2 integer)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable4 ("
		"	col1 integer, "
		"	col2 integer)"));
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_getTableList(cur,NULL));
	assertInResultSet(cur,"Tables_in_xxx","testtable1");
	assertInResultSet(cur,"Tables_in_xxx","testtable2");
	assertInResultSet(cur,"Tables_in_xxx","testtable3");
	assertInResultSet(cur,"Tables_in_xxx","testtable4");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable1"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable2"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable3"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable4"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// type info list
	printf("TYPE INFO LIST: \n");
	assertTrue(sqlrcur_getTypeInfoList(cur,"integer"));
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
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"INTEGER");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"4");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"10");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),
							"INTEGER");
	assertTrue(sqlrcur_getTypeInfoList(cur,"char"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"CHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"32767");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"CHAR");
	assertTrue(sqlrcur_getTypeInfoList(cur,"varchar"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"VARCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"12");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"255");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),
							"VARCHAR");
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
		"	testboolean boolean, "
		"	testsmallint smallint, "
		"	testint integer, "
		"	testbigint bigint, "
		"	testint8 int8, "
		"	testdecimal decimal(10,2), "
		"	testmoney money, "
		"	testsmallfloat smallfloat, "
		"	testfloat float, "
		"	testchar char(40), "
		"	testnchar nchar(40), "
		"	testvarchar varchar(40), "
		"	testnvarchar nvarchar(40), "
		"	testlvarchar lvarchar(40), "
		"	testdate date, "
		"	testdatetime datetime year to second, "
		"	testtext text, "
		"	testbyte byte)"));
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_getColumnList(cur,"testtable",NULL));
	assertEqStr(sqlrcur_getColumnName(cur,0),"column_name");
	assertEqStr(sqlrcur_getColumnName(cur,1),"data_type");
	assertEqStr(sqlrcur_getColumnName(cur,2),
			"character_maximum_length");
	assertEqStr(sqlrcur_getColumnName(cur,3),"numeric_precision");
	assertEqStr(sqlrcur_getColumnName(cur,4),"numeric_scale");
	assertEqStr(sqlrcur_getColumnName(cur,5),"is_nullable");
	assertEqStr(sqlrcur_getColumnName(cur,6),"column_key");
	assertEqStr(sqlrcur_getColumnName(cur,7),"column_default");
	assertEqStr(sqlrcur_getColumnName(cur,8),"extra");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"column_name"),
							"testboolean");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"column_name"),
							"testsmallint");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"column_name"),
							"testint");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"column_name"),
							"testbigint");
	assertEqStr(sqlrcur_getFieldByName(cur,4,"column_name"),
							"testint8");
	assertEqStr(sqlrcur_getFieldByName(cur,5,"column_name"),
							"testdecimal");
	assertEqStr(sqlrcur_getFieldByName(cur,6,"column_name"),
							"testmoney");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"column_name"),
							"testsmallfloat");
	assertEqStr(sqlrcur_getFieldByName(cur,8,"column_name"),
							"testfloat");
	assertEqStr(sqlrcur_getFieldByName(cur,9,"column_name"),
							"testchar");
	assertEqStr(sqlrcur_getFieldByName(cur,10,"column_name"),
							"testnchar");
	assertEqStr(sqlrcur_getFieldByName(cur,11,"column_name"),
							"testvarchar");
	assertEqStr(sqlrcur_getFieldByName(cur,12,"column_name"),
							"testnvarchar");
	assertEqStr(sqlrcur_getFieldByName(cur,13,"column_name"),
							"testlvarchar");
	assertEqStr(sqlrcur_getFieldByName(cur,14,"column_name"),
							"testdate");
	assertEqStr(sqlrcur_getFieldByName(cur,15,"column_name"),
							"testdatetime");
	assertEqStr(sqlrcur_getFieldByName(cur,16,"column_name"),
							"testtext");
	assertEqStr(sqlrcur_getFieldByName(cur,17,"column_name"),
							"testbyte");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"BOOLEAN");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"data_type"),"SMALLINT");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"data_type"),"INTEGER");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"data_type"),"BIGINT");
	assertEqStr(sqlrcur_getFieldByName(cur,4,"data_type"),"INT8");
	assertEqStr(sqlrcur_getFieldByName(cur,5,"data_type"),"DECIMAL");
	assertEqStr(sqlrcur_getFieldByName(cur,6,"data_type"),"MONEY");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"data_type"),
							"SMALLFLOAT");
	assertEqStr(sqlrcur_getFieldByName(cur,8,"data_type"),"FLOAT");
	assertEqStr(sqlrcur_getFieldByName(cur,9,"data_type"),"CHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,10,"data_type"),"NCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,11,"data_type"),"VARCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,12,"data_type"),"NVARCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,13,"data_type"),"LVARCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,14,"data_type"),"DATE");
	assertEqStr(sqlrcur_getFieldByName(cur,15,"data_type"),"DATETIME");
	assertEqStr(sqlrcur_getFieldByName(cur,16,"data_type"),"TEXT");
	assertEqStr(sqlrcur_getFieldByName(cur,17,"data_type"),"BYTE");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// column list - auto_increment, primary key
	printf("COLUMN LIST - auto_increment, primary key: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 serial primary key, "
		"	col2 int)"));
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_getColumnList(cur,"testtable",NULL));
	assertContains(sqlrcur_getFieldByName(cur,0,"extra"),
				"auto_increment");
	assertContains(sqlrcur_getFieldByName(cur,0,"column_key"),
				"PRI");
	assertNotContains(sqlrcur_getFieldByName(cur,1,"extra"),
				"auto_increment");
	assertNotContains(sqlrcur_getFieldByName(cur,1,"column_key"),
				"PRI");
	printf("\n");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)"));
	assertTrue(sqlrcur_getColumnList(cur,"testtable",NULL));
	assertNotContains(sqlrcur_getFieldByName(cur,0,"extra"),
				"auto_increment");
	assertContains(sqlrcur_getFieldByName(cur,0,"column_key"),
				"PRI");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// primary keys list
	printf("PRIMARY KEYS LIST: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 integer primary key, "
		"	col2 integer)"));
	assertTrue(sqlrcon_commit(con));
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
	assertEqStr(sqlrcur_getFieldByName(cur,0,"table"),
							"testtable");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"seq_in_index"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"column_name"),
							"col1");
	{
		const char *keyname=
			sqlrcur_getFieldByName(cur,0,"key_name");
		assertTrue(keyname && keyname[0]);
	}
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// key and index list
	printf("KEY AND INDEX LIST: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 integer primary key, "
		"	col2 integer)"));
	assertTrue(sqlrcon_commit(con));
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
	assertEqStr(sqlrcur_getFieldByName(cur,0,"table"),
							"testtable");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"non_unique"),"0");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"seq_in_index"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"column_name"),
							"col1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"collation"),"A");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"index_type"),"3");
	{
		const char *keyname=
			sqlrcur_getFieldByName(cur,0,"key_name");
		assertTrue(keyname && keyname[0]);
	}
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// procedure list
	printf("PROCEDURE LIST: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc1");
	sqlrcur_sendQuery(cur,"drop procedure testproc2");
	sqlrcur_sendQuery(cur,"drop procedure testproc3");
	sqlrcur_sendQuery(cur,"drop procedure testproc4");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc1("
		"	in1 integer, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"define x integer; "
		"let x = 1; "
		"end procedure;"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc2("
		"	in1 integer, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"define x integer; "
		"let x = 1; "
		"end procedure;"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc3("
		"	in1 integer, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"define x integer; "
		"let x = 1; "
		"end procedure;"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc4("
		"	in1 integer, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"define x integer; "
		"let x = 1; "
		"end procedure;"));
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_getProcedureList(cur,NULL));
	assertInResultSet(cur,"routine_name","testproc1");
	assertInResultSet(cur,"routine_name","testproc2");
	assertInResultSet(cur,"routine_name","testproc3");
	assertInResultSet(cur,"routine_name","testproc4");
	printf("\n");


	// procedure parameter list
	printf("PROCEDURE PARAMETER LIST: \n");
	assertTrue(sqlrcur_getProcedureParameterList(
					cur,"testproc1",NULL));
	assertEqStr(sqlrcur_getColumnName(cur,0),"parameter_name");
	assertEqStr(sqlrcur_getColumnName(cur,1),"parameter_mode");
	assertEqStr(sqlrcur_getColumnName(cur,2),"data_type");
	assertEqStr(sqlrcur_getColumnName(cur,3),
			"character_maximum_length");
	assertEqStr(sqlrcur_getColumnName(cur,4),"ordinal_position");
	assertEqInt(sqlrcur_rowCount(cur),4);
	assertEqStr(sqlrcur_getFieldByName(cur,0,"parameter_name"),
							"in1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"integer");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"ordinal_position"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"parameter_name"),
							"in2");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"data_type"),"char");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"ordinal_position"),"2");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"parameter_name"),
							"in3");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"data_type"),"varchar");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"ordinal_position"),"3");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"parameter_name"),
							"in4");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"data_type"),"date");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"ordinal_position"),"4");
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc1"));
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc2"));
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc3"));
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc4"));
	assertTrue(sqlrcon_commit(con));
	printf("\n");


	// invalid queries
	printf("INVALID QUERIES: \n");
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint "));
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into testtable values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into testtable values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into testtable values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into testtable values (1,2,3,4)"));
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
