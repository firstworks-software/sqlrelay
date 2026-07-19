// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclientwrapper.h>
#include <string.h>
#include <ctype.h>
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
				"READ COMMITTED","SERIALIZABLE",NULL};
	const char	*bindvars[]={"1","2","3","4","5",NULL};
	const char	*bindvals[]={"4","testchar4",
				"testvarchar4","01-JAN-2004","testlong4"};
	const char	*arraybindvars[]={"var1","var2","var3",
					"var4","var5",NULL};
	const char	*arraybindvals[]={"7","testchar7",
				"testvarchar7","01-JAN-2007","testlong7"};
	int64_t		numvar;
	const char	*stringvar;
	double		floatvar;
	int16_t		year=0;
	int16_t		month=0;
	int16_t		day=0;
	int16_t		hour=0;
	int16_t		minute=0;
	int16_t		second=0;
	int32_t		microsecond=0;
	const char	*tz=NULL;
	const char	*nullvar;
	int		isnegative=0;
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
	const char	*clobvar;
	uint32_t	clobvarlength;
	const char	*blobvar;
	uint32_t	blobvarlength;
	#define	LARGE_BUFFER_LENGTH	8192
	char		largebuffer[LARGE_BUFFER_LENGTH+1];
	const char	**il;
	uint64_t	i;
	char		*dot;
	char		*hostname;
	char		*upperhostname;
	char		query[LARGE_BUFFER_LENGTH+25];
	unsigned char	buffer[256];
	char		querystr[1024];
	char		hex[3];
	sqlrcur		bindcur1;
	sqlrcur		bindcur2;

	const char	*cert="../sqlrelay.conf.d/tls/client.pem";
	const char	*ca="../sqlrelay.conf.d/tls/ca.pem";
	#ifdef _WIN32
		cert="..\\sqlrelay.conf.d\\tls\\client.pfx";
		ca="..\\sqlrelay.conf.d\\tls\\ca.pfx";
	#endif


	// hostname
	hostname=(char *)malloc(256);
	gethostname(hostname,256);
	dot=strchr(hostname,'.');
	if (dot) {
		*dot='\0';
	}
	upperhostname=(char *)malloc(256);
	for (i=0; hostname[i]; i++) {
		upperhostname[i]=toupper(hostname[i]);
	}
	upperhostname[i]='\0';


	// instantiation
	con=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
						NULL,NULL,0,1);
	cur=sqlrcur_alloc(con);
	sqlrcon_enableTls(con,NULL,cert,NULL,NULL,"ca",ca,0);


	// identify
	printf("IDENTIFY: \n");
	assertEqStr(sqlrcon_identify(con),"oracle");
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
	assertEqStr(sqlrcon_bindFormat(con),":*");
	printf("\n");


	// nextval format
	printf("NEXTVAL FORMAT: \n");
	assertEqStr(sqlrcon_nextvalFormat(con),"%s.nextval");
	printf("\n");


	// isolation levels
	printf("ISOLATION LEVELS: \n");
	for (il=isolationlevels; *il; il++) {
		// oracle requires the isolation level to
		// be the first query of the transaction
		assertTrue(sqlrcon_commit(con));
		// you can set the isolation level, but to get it, you have to
		// have permisisons to read from sys.v_$session and
		// sys.v_$transaction
		assertTrue(sqlrcon_setIsolationLevel(con,*il));
		printf("\n");
	}
	// reset to the default isolation level
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcon_setIsolationLevel(con,isolationlevels[0]));
	printf("\n");


	// create testtable
	printf("CREATE TESTTABLE: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
		"	testclob clob, "
		"	testblob blob)"));
	printf("\n");


	// insert
	printf("INSERT: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	'testchar1', "
		"	'testvarchar1', "
		"	'01-JAN-2001', "
		"	'testlong1', "
		"	'testclob1', "
		"	empty_blob())"));
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
		"	:var1, "
		"	:var2, "
		"	:var3, "
		"	:var4, "
		"	:var5, "
		"	:var6, "
		"	:var7)");
	assertEqInt(sqlrcur_countBindVariables(cur),7);
	sqlrcur_inputBindLong(cur,"1",2);
	sqlrcur_inputBindString(cur,"2","testchar2");
	sqlrcur_inputBindString(cur,"3","testvarchar2");
	sqlrcur_inputBindDate(cur,"4",2002,1,1,0,0,0,0,NULL,0);
	sqlrcur_inputBindString(cur,"5","testlong2");
	sqlrcur_inputBindClob(cur,"6","testclob2",9);
	sqlrcur_inputBindBlob(cur,"7","testblob2",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",3);
	sqlrcur_inputBindString(cur,"2","testchar3");
	sqlrcur_inputBindString(cur,"3","testvarchar3");
	sqlrcur_inputBindDate(cur,"4",2003,1,1,0,0,0,0,NULL,0);
	sqlrcur_inputBindString(cur,"5","testlong3");
	sqlrcur_inputBindClob(cur,"6","testclob3",9);
	sqlrcur_inputBindBlob(cur,"7","testblob3",9);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// array of input binds by position
	printf("ARRAY OF INPUT BINDS BY POSITION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindStrings(cur,bindvars,bindvals);
	sqlrcur_inputBindClob(cur,"6","testclob4",9);
	sqlrcur_inputBindBlob(cur,"7","testblob4",9);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// input bind by position with validation
	printf("INPUT BIND BY POSITION WITH VALIDATION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",5);
	sqlrcur_inputBindString(cur,"2","testchar5");
	sqlrcur_inputBindString(cur,"3","testvarchar5");
	sqlrcur_inputBindDate(cur,"4",2005,1,1,0,0,0,0,NULL,0);
	sqlrcur_inputBindString(cur,"5","testlong5");
	sqlrcur_inputBindClob(cur,"6","testclob5",9);
	sqlrcur_inputBindBlob(cur,"7","testblob5",9);
	sqlrcur_validateBinds(cur);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);


	// input bind by name
	printf("INPUT BIND BY NAME: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",6);
	sqlrcur_inputBindString(cur,"var2","testchar6");
	sqlrcur_inputBindString(cur,"var3","testvarchar6");
	sqlrcur_inputBindDate(cur,"var4",2006,1,1,0,0,0,0,NULL,0);
	sqlrcur_inputBindString(cur,"var5","testlong6");
	sqlrcur_inputBindClob(cur,"var6","testclob6",9);
	sqlrcur_inputBindBlob(cur,"var7","testblob6",9);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// array of input binds by name
	printf("ARRAY OF INPUT BINDS BY NAME: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindStrings(cur,arraybindvars,arraybindvals);
	sqlrcur_inputBindClob(cur,"var6","testclob7",9);
	sqlrcur_inputBindBlob(cur,"var7","testblob7",9);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// input bind by name with validation
	printf("INPUT BIND BY NAME WITH VALIDATION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",8);
	sqlrcur_inputBindString(cur,"var2","testchar8");
	sqlrcur_inputBindString(cur,"var3","testvarchar8");
	sqlrcur_inputBindDate(cur,"var4",2008,1,1,0,0,0,0,NULL,0);
	sqlrcur_inputBindString(cur,"var5","testlong8");
	sqlrcur_inputBindClob(cur,"var6","testclob8",9);
	sqlrcur_inputBindBlob(cur,"var7","testblob8",9);
	sqlrcur_inputBindString(cur,"var9","junkvalue");
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
		"	testnumber"));
	printf("\n");


	// column count
	printf("COLUMN COUNT: \n");
	assertEqInt(sqlrcur_colCount(cur),7);
	printf("\n");


	// column names
	printf("COLUMN NAMES: \n");
	assertEqStr(sqlrcur_getColumnName(cur,0),"TESTNUMBER");
	assertEqStr(sqlrcur_getColumnName(cur,1),"TESTCHAR");
	assertEqStr(sqlrcur_getColumnName(cur,2),"TESTVARCHAR");
	assertEqStr(sqlrcur_getColumnName(cur,3),"TESTDATE");
	assertEqStr(sqlrcur_getColumnName(cur,4),"TESTLONG");
	assertEqStr(sqlrcur_getColumnName(cur,5),"TESTCLOB");
	assertEqStr(sqlrcur_getColumnName(cur,6),"TESTBLOB");
	cols=sqlrcur_getColumnNames(cur);
	assertEqStr(cols[0],"TESTNUMBER");
	assertEqStr(cols[1],"TESTCHAR");
	assertEqStr(cols[2],"TESTVARCHAR");
	assertEqStr(cols[3],"TESTDATE");
	assertEqStr(cols[4],"TESTLONG");
	assertEqStr(cols[5],"TESTCLOB");
	assertEqStr(cols[6],"TESTBLOB");
	printf("\n");


	// column types
	printf("COLUMN TYPES: \n");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),"NUMBER");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTNUMBER"),"NUMBER");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,1),"CHAR");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTCHAR"),"CHAR");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,2),"VARCHAR2");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTVARCHAR"),"VARCHAR2");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,3),"DATE");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTDATE"),"DATE");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,4),"LONG");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTLONG"),"LONG");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,5),"CLOB");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTCLOB"),"CLOB");
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,6),"BLOB");
	assertEqStr(sqlrcur_getColumnTypeByName(cur,"TESTBLOB"),"BLOB");
	printf("\n");


	// column length
	printf("COLUMN LENGTH: \n");
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),22);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTNUMBER"),22);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,1),40);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTCHAR"),40);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,2),40);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTVARCHAR"),40);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,3),7);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTDATE"),7);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,4),0);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTLONG"),0);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,5),0);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTCLOB"),0);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,6),0);
	assertEqInt(sqlrcur_getColumnLengthByName(cur,"TESTBLOB"),0);
	printf("\n");


	// longest column
	printf("LONGEST COLUMN: \n");
	assertEqInt(sqlrcur_getLongestByIndex(cur,0),1);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTNUMBER"),1);
	assertEqInt(sqlrcur_getLongestByIndex(cur,1),40);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTCHAR"),40);
	assertEqInt(sqlrcur_getLongestByIndex(cur,2),12);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTVARCHAR"),12);
	assertEqInt(sqlrcur_getLongestByIndex(cur,3),9);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTDATE"),9);
	assertEqInt(sqlrcur_getLongestByIndex(cur,4),9);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTLONG"),9);
	assertEqInt(sqlrcur_getLongestByIndex(cur,5),9);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTCLOB"),9);
	assertEqInt(sqlrcur_getLongestByIndex(cur,6),9);
	assertEqInt(sqlrcur_getLongestByName(cur,"TESTBLOB"),9);
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
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),
			"testchar1                               ");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"testvarchar1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,3),"01-JAN-01");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,4),"testlong1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,5),"testclob1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,6),"");
	printf("\n");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,1),
			"testchar8                               ");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,2),"testvarchar8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,3),"01-JAN-08");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,4),"testlong8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,5),"testclob8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,6),"testblob8");
	printf("\n");


	// field lengths by index
	printf("FIELD LENGTHS BY INDEX: \n");
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,1),40);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,2),12);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,3),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,4),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,5),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,6),0);
	printf("\n");
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,0),1);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,1),40);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,2),12);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,3),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,4),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,5),9);
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,7,6),9);
	printf("\n");


	// fields by name
	printf("FIELDS BY NAME: \n");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTNUMBER"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTCHAR"),
			"testchar1                               ");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTVARCHAR"),"testvarchar1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTDATE"),"01-JAN-01");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTLONG"),"testlong1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTCLOB"),"testclob1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTBLOB"),"");
	printf("\n");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTNUMBER"),"8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTCHAR"),
			"testchar8                               ");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTVARCHAR"),"testvarchar8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTDATE"),"01-JAN-08");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTLONG"),"testlong8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTCLOB"),"testclob8");
	assertEqStr(sqlrcur_getFieldByName(cur,7,"TESTBLOB"),"testblob8");
	printf("\n");


	// field lengths by name
	printf("FIELD LENGTHS BY NAME: \n");
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTNUMBER"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTCHAR"),40);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTVARCHAR"),12);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTDATE"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTLONG"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTCLOB"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTBLOB"),0);
	printf("\n");
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTNUMBER"),1);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTCHAR"),40);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTVARCHAR"),12);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTDATE"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTLONG"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTCLOB"),9);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,7,"TESTBLOB"),9);
	printf("\n");


	// fields by array
	printf("FIELDS BY ARRAY: \n");
	fields=sqlrcur_getRow(cur,0);
	assertEqStr(fields[0],"1");
	assertEqStr(fields[1],"testchar1                               ");
	assertEqStr(fields[2],"testvarchar1");
	assertEqStr(fields[3],"01-JAN-01");
	assertEqStr(fields[4],"testlong1");
	assertEqStr(fields[5],"testclob1");
	assertEqStr(fields[6],"");
	printf("\n");


	// field lengths by array
	printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=sqlrcur_getRowLengths(cur,0);
	assertEqInt(fieldlens[0],1);
	assertEqInt(fieldlens[1],40);
	assertEqInt(fieldlens[2],12);
	assertEqInt(fieldlens[3],9);
	assertEqInt(fieldlens[4],9);
	assertEqInt(fieldlens[5],9);
	assertEqInt(fieldlens[6],0);
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
		"	testnumber"));
	assertEqInt(sqlrcur_getResultSetBufferSize(cur),2);
	printf("\n");
	assertEqInt(sqlrcur_firstRowIndex(cur),0);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqInt(sqlrcur_rowCount(cur),2);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,1,0),"2");
	assertEqStr(sqlrcur_getFieldByIndex(cur,2,0),"3");
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
		"	testnumber"));
	assertEqStr(sqlrcur_getColumnName(cur,0),NULL);
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),0);
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),NULL);
	sqlrcur_getColumnInfo(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
	assertEqStr(sqlrcur_getColumnName(cur,0),"TESTNUMBER");
	assertEqInt(sqlrcur_getColumnLengthByIndex(cur,0),22);
	assertEqStr(sqlrcur_getColumnTypeByIndex(cur,0),"NUMBER");
	printf("\n");


	// suspended session
	printf("SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	free((char *)socket);
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
		"	testnumber"));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	free((char *)socket);
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
		"	testnumber"));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	free((char *)socket);
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
		"	testnumber"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,2,0),"3");
	id=sqlrcur_getResultSetId(cur);
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	free((char *)socket);
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
		"	testnumber"));
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqStr(filename,"cachefile1");
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	free(filename);
	printf("\n");


	// column count for cached result set
	printf("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqInt(sqlrcur_colCount(cur),7);
	printf("\n");


	// column names for cached result set
	printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqStr(sqlrcur_getColumnName(cur,0),"TESTNUMBER");
	assertEqStr(sqlrcur_getColumnName(cur,1),"TESTCHAR");
	assertEqStr(sqlrcur_getColumnName(cur,2),"TESTVARCHAR");
	assertEqStr(sqlrcur_getColumnName(cur,3),"TESTDATE");
	assertEqStr(sqlrcur_getColumnName(cur,4),"TESTLONG");
	assertEqStr(sqlrcur_getColumnName(cur,5),"TESTCLOB");
	assertEqStr(sqlrcur_getColumnName(cur,6),"TESTBLOB");
	cols=sqlrcur_getColumnNames(cur);
	assertEqStr(cols[0],"TESTNUMBER");
	assertEqStr(cols[1],"TESTCHAR");
	assertEqStr(cols[2],"TESTVARCHAR");
	assertEqStr(cols[3],"TESTDATE");
	assertEqStr(cols[4],"TESTLONG");
	assertEqStr(cols[5],"TESTCLOB");
	assertEqStr(cols[6],"TESTBLOB");
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
		"	testnumber"));
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
	printf("FROM ONE CACHE FILE TO ANOTHER: \n");
	sqlrcur_cacheToFile(cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile1"));
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile2"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	printf("\n");


	// from one cache file to another with result set buffer size
	printf("FROM ONE CACHE FILE TO ANOTHER "
				"WITH RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile1"));
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile2"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqStr(sqlrcur_getFieldByIndex(cur,8,0),NULL);
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
		"	testnumber"));
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
	free((char *)socket);
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
	printf("FINISHED SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
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
	free((char *)socket);
	assertTrue(sqlrcur_resumeResultSet(cur,id));
	assertEqStr(sqlrcur_getFieldByIndex(cur,4,0),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,5,0),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,6,0),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,7,0),NULL);
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
	printf("TRANSACTION BEHAVIOR - implicit: \n");
	assertTrue(sqlrcon_setTransactionModel(con,"implicit"));
	assertEqStr(sqlrcon_getTransactionModel(con),"implicit");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable (col1 integer)"));
	secondcon=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",
							NULL,NULL,0,1);
	secondcur=sqlrcur_alloc(secondcon);
	sqlrcon_enableTls(secondcon,NULL,cert,NULL,NULL,"ca",ca,0);
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
	assertEqStr(sqlrcon_getTransactionModel(con),"implicit");
	assertFalse(sqlrcon_getAutoCommit(con));
	printf("\n");


	// individual substitutions
	printf("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"select $(var1),'$(var2)',$(var3) from dual");
	sqlrcur_subString(cur,"var1","$(var11)");
	sqlrcur_subString(cur,"var2","$(var21)");
	sqlrcur_subString(cur,"var3","$(var31)");
	sqlrcur_subString(cur,"var11","$(var111)");
	sqlrcur_subString(cur,"var21","$(var211)");
	sqlrcur_subString(cur,"var31","$(var311)");
	sqlrcur_subLong(cur,"var111",1);
	sqlrcur_subString(cur,"var211","hello");
	sqlrcur_subDouble(cur,"var311",10.5556,6,4);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"10.5556");
	printf("\n");


	// array substitutions
	printf("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"select $(var1),$(var2),$(var3) from dual");
	sqlrcur_subLongs(cur,subvars,subvallongs);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"2");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"3");
	printf("\n");
	sqlrcur_prepareQuery(cur,
			"select '$(var1)','$(var2)','$(var3)' from dual");
	sqlrcur_subStrings(cur,subvars,subvalstrings);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"hi");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"bye");
	printf("\n");
	sqlrcur_prepareQuery(cur,"select $(var1),$(var2),$(var3) from dual");
	sqlrcur_subDoubles(cur,subvars,subvaldoubles,precs,scales);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"10.55");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"10.556");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),"10.5556");
	printf("\n");


	// nulls as nulls
	printf("NULLS AS NULLS: \n");
	sqlrcur_getNullsAsNulls(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select NULL,1,NULL from dual"));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),NULL);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select NULL,1,NULL from dual"));
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
		"	testclob1 clob, "
		"	testclob2 clob, "
		"	testblob1 blob, "
		"	testblob2 blob)"));
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	:var1, "
		"	:var2, "
		"	:var3, "
		"	:var4)");
	sqlrcur_inputBindClob(cur,"var1","",0);
	sqlrcur_inputBindClob(cur,"var2",NULL,0);
	sqlrcur_inputBindBlob(cur,"var3","",0);
	sqlrcur_inputBindBlob(cur,"var4",NULL,0);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_sendQuery(cur,"select * from testtable");
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
	sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testclob clob, "
		"	testblob blob)");
	sqlrcur_prepareQuery(cur,
			"insert into testtable values (:clobval,:blobval)");
	for (i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	sqlrcur_inputBindClob(cur,"clobval",largebuffer,LARGE_BUFFER_LENGTH);
	sqlrcur_inputBindBlob(cur,"blobval",largebuffer,LARGE_BUFFER_LENGTH);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_sendQuery(cur,"select * from testtable");
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTCLOB"),
			LARGE_BUFFER_LENGTH);
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTCLOB"),largebuffer);
	assertEqInt(sqlrcur_getFieldLengthByName(cur,0,"TESTBLOB"),
			LARGE_BUFFER_LENGTH);
	assertEqStrLen(sqlrcur_getFieldByName(cur,0,"TESTBLOB"),
			largebuffer,LARGE_BUFFER_LENGTH);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// output bind by position
	printf("OUTPUT BIND BY POSITION: \n");
	sqlrcur_getNullsAsNulls(cur);
	sqlrcur_prepareQuery(cur,
		"begin "
		"	:numvar:=1; "
		"	:stringvar:='hello'; "
		"	:floatvar:=2.5; "
		"	:datevar:='03-FEB-2001'; "
		"	:nullvar:=null; "
		"end;");
	assertEqInt(sqlrcur_countBindVariables(cur),5);
	sqlrcur_defineOutputBindInteger(cur,"1");
	sqlrcur_defineOutputBindString(cur,"2",10);
	sqlrcur_defineOutputBindDouble(cur,"3");
	sqlrcur_defineOutputBindDate(cur,"4");
	sqlrcur_defineOutputBindString(cur,"5",10);
	assertTrue(sqlrcur_executeQuery(cur));
	numvar=sqlrcur_getOutputBindInteger(cur,"1");
	stringvar=sqlrcur_getOutputBindString(cur,"2");
	floatvar=sqlrcur_getOutputBindDouble(cur,"3");
	sqlrcur_getOutputBindDate(cur,"4",&year,&month,&day,
				&hour,&minute,&second,&microsecond,&tz,
				&isnegative);
	nullvar=sqlrcur_getOutputBindString(cur,"5");
	assertEqInt(numvar,1);
	assertEqStr(stringvar,"hello");
	assertEqDbl(floatvar,2.5);
	assertEqInt(year,2001);
	assertEqInt(month,2);
	assertEqInt(day,3);
	assertEqInt(hour,0);
	assertEqInt(minute,0);
	assertEqInt(second,0);
	assertEqInt(microsecond,0);
	assertEqStr(tz,"");
	assertFalse(isnegative);
	assertEqStr(nullvar,NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	printf("\n");


	// output bind by name
	printf("OUTPUT BIND BY NAME: \n");
	sqlrcur_getNullsAsNulls(cur);
	sqlrcur_clearBinds(cur);
	sqlrcur_defineOutputBindInteger(cur,"numvar");
	sqlrcur_defineOutputBindString(cur,"stringvar",10);
	sqlrcur_defineOutputBindDouble(cur,"floatvar");
	sqlrcur_defineOutputBindDate(cur,"datevar");
	sqlrcur_defineOutputBindString(cur,"nullvar",10);
	assertTrue(sqlrcur_executeQuery(cur));
	numvar=sqlrcur_getOutputBindInteger(cur,"numvar");
	stringvar=sqlrcur_getOutputBindString(cur,"stringvar");
	floatvar=sqlrcur_getOutputBindDouble(cur,"floatvar");
	sqlrcur_getOutputBindDate(cur,"datevar",&year,&month,&day,
				&hour,&minute,&second,&microsecond,&tz,
				&isnegative);
	assertEqInt(numvar,1);
	assertEqStr(stringvar,"hello");
	assertEqDbl(floatvar,2.5);
	assertEqInt(year,2001);
	assertEqInt(month,2);
	assertEqInt(day,3);
	assertEqInt(hour,0);
	assertEqInt(minute,0);
	assertEqInt(second,0);
	assertEqInt(microsecond,0);
	assertEqStr(tz,"");
	assertFalse(isnegative);
	nullvar=sqlrcur_getOutputBindString(cur,"nullvar");
	assertEqStr(nullvar,NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	printf("\n");


	// output bind by name with validation
	printf("OUTPUT BIND BY NAME WITH VALIDATION: \n");
	sqlrcur_getNullsAsNulls(cur);
	sqlrcur_clearBinds(cur);
	sqlrcur_defineOutputBindInteger(cur,"numvar");
	sqlrcur_defineOutputBindString(cur,"stringvar",10);
	sqlrcur_defineOutputBindDouble(cur,"floatvar");
	sqlrcur_defineOutputBindDate(cur,"datevar");
	sqlrcur_defineOutputBindString(cur,"nullvar",10);
	sqlrcur_defineOutputBindString(cur,"dummyvar",10);
	sqlrcur_validateBinds(cur);
	assertTrue(sqlrcur_executeQuery(cur));
	numvar=sqlrcur_getOutputBindInteger(cur,"numvar");
	stringvar=sqlrcur_getOutputBindString(cur,"stringvar");
	floatvar=sqlrcur_getOutputBindDouble(cur,"floatvar");
	sqlrcur_getOutputBindDate(cur,"datevar",&year,&month,&day,
				&hour,&minute,&second,&microsecond,&tz,
				&isnegative);
	assertEqInt(numvar,1);
	assertEqStr(stringvar,"hello");
	assertEqDbl(floatvar,2.5);
	assertEqInt(year,2001);
	assertEqInt(month,2);
	assertEqInt(day,3);
	assertEqInt(hour,0);
	assertEqInt(minute,0);
	assertEqInt(second,0);
	assertEqInt(microsecond,0);
	assertEqStr(tz,"");
	assertFalse(isnegative);
	nullvar=sqlrcur_getOutputBindString(cur,"nullvar");
	assertEqStr(nullvar,NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	printf("\n");


	// lob output bind
	printf("LOB OUTPUT BIND: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testclob clob, "
		"	testblob blob)"));
	sqlrcur_prepareQuery(cur,
			"insert into testtable values ('hello',:var1)");
	sqlrcur_inputBindBlob(cur,"var1","hello",5);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_prepareQuery(cur,
		"begin "
		"	select testclob into :clobvar from testtable; "
		"	select testblob into :blobvar from testtable; "
		"end;");
	sqlrcur_defineOutputBindClob(cur,"clobvar");
	sqlrcur_defineOutputBindBlob(cur,"blobvar");
	assertTrue(sqlrcur_executeQuery(cur));
	clobvar=sqlrcur_getOutputBindClob(cur,"clobvar");
	clobvarlength=sqlrcur_getOutputBindLength(cur,"clobvar");
	blobvar=sqlrcur_getOutputBindBlob(cur,"blobvar");
	blobvarlength=sqlrcur_getOutputBindLength(cur,"blobvar");
	assertEqStrLen(clobvar,"hello",5);
	assertEqInt(clobvarlength,5);
	assertEqStrLen(blobvar,"hello",5);
	assertEqInt(blobvarlength,5);
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// long output bind
	printf("LONG OUTPUT BIND: \n");
	for (i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	snprintf(query,sizeof(query),
				"begin :bindval:='%s'; end;",largebuffer);
	sqlrcur_prepareQuery(cur,query);
	sqlrcur_defineOutputBindString(cur,"bindval",LARGE_BUFFER_LENGTH);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindLength(cur,"bindval"),
			LARGE_BUFFER_LENGTH);
	assertEqStr(sqlrcur_getOutputBindString(cur,"bindval"),largebuffer);
	printf("\n");


	// negative input bind
	printf("NEGATIVE INPUT BIND: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	sqlrcur_sendQuery(cur,"create table testtable (testval number)");
	sqlrcur_prepareQuery(cur,"insert into testtable values (:testval)");
	sqlrcur_inputBindLong(cur,"testval",-1);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_sendQuery(cur,"select testval from testtable");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"TESTVAL"),"-1");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// bind validation
	printf("BIND VALIDATION: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 varchar2(20), "
		"	col2 varchar2(20), "
		"	col3 varchar2(20))");
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	$(var1), "
		"	$(var2), "
		"	$(var3))");
	sqlrcur_inputBindString(cur,"var1","1");
	sqlrcur_inputBindString(cur,"var2","2");
	sqlrcur_inputBindString(cur,"var3","3");
	sqlrcur_subString(cur,"var1",":var1");
	assertTrue(sqlrcur_validBind(cur,"var1"));
	assertFalse(sqlrcur_validBind(cur,"var2"));
	assertFalse(sqlrcur_validBind(cur,"var3"));
	assertFalse(sqlrcur_validBind(cur,"var4"));
	printf("\n");
	sqlrcur_subString(cur,"var2",":var2");
	assertTrue(sqlrcur_validBind(cur,"var1"));
	assertTrue(sqlrcur_validBind(cur,"var2"));
	assertFalse(sqlrcur_validBind(cur,"var3"));
	assertFalse(sqlrcur_validBind(cur,"var4"));
	printf("\n");
	sqlrcur_subString(cur,"var3",":var3");
	assertTrue(sqlrcur_validBind(cur,"var1"));
	assertTrue(sqlrcur_validBind(cur,"var2"));
	assertTrue(sqlrcur_validBind(cur,"var3"));
	assertFalse(sqlrcur_validBind(cur,"var4"));
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// rebinding
	printf("REBINDING: \n");
	sqlrcur_prepareQuery(cur,
		"begin "
		"	:out:= :in; "
		"end;");
	sqlrcur_inputBindLong(cur,"in",1);
	sqlrcur_defineOutputBindInteger(cur,"out");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"out"),1);
	sqlrcur_inputBindLong(cur,"in",2);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"out"),2);
	sqlrcur_inputBindLong(cur,"in",3);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"out"),3);
	printf("\n");


	// reexecute
	printf("REEXECUTE: \n");
	sqlrcur_prepareQuery(cur,"select 1 from dual");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	printf("\n");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	printf("\n");
	sqlrcur_prepareQuery(cur,"select :var from dual");
	sqlrcur_inputBindLong(cur,"var",1);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	printf("\n");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	printf("\n");
	sqlrcur_inputBindLong(cur,"var",2);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_rowCount(cur),1);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"2");
	printf("\n");


	// stored procedure returning no value
	printf("STORED PROCEDURE RETURNING NO VALUE: \n");
	sqlrcur_sendQuery(cur,"drop function testproc");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create or replace "
		"procedure testproc("
		"	in1 in number, "
		"	in2 in number, "
		"	in3 in varchar2) "
		"is "
		"begin "
		"	return; "
		"end;"));
	sqlrcur_prepareQuery(cur,"begin testproc(:in1,:in2,:in3); end;");
	sqlrcur_inputBindLong(cur,"in1",1);
	sqlrcur_inputBindDouble(cur,"in2",2.5,2,1);
	sqlrcur_inputBindString(cur,"in3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	printf("\n");


	// stored procedure returning single value
	printf("STORED PROCEDURE RETURNING SINGLE VALUE: \n");
	sqlrcur_sendQuery(cur,"drop function testproc");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create or replace "
		"function testproc("
		"	in1 in number, "
		"	in2 in number, "
		"	in3 in varchar2) "
		"	return number "
		"is "
		"begin "
		"	return in1; "
		"end;"));
	sqlrcur_prepareQuery(cur,"select testproc(:in1,:in2,:in3) from dual");
	sqlrcur_inputBindLong(cur,"in1",1);
	sqlrcur_inputBindDouble(cur,"in2",2.5,2,1);
	sqlrcur_inputBindString(cur,"in3","hello");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	sqlrcur_prepareQuery(cur,
		"begin "
		"	:out1:=testproc(:in1,:in2,:in3); "
		"end;");
	sqlrcur_inputBindLong(cur,"in1",1);
	sqlrcur_inputBindDouble(cur,"in2",2.5,2,1);
	sqlrcur_inputBindString(cur,"in3","hello");
	sqlrcur_defineOutputBindInteger(cur,"out1");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"out1"),1);
	assertTrue(sqlrcur_sendQuery(cur,"drop function testproc"));
	printf("\n");


	// stored procedure returning multiple values
	printf("STORED PROCEDURE RETURNING MULTIPLE VALUES: \n");
	sqlrcur_sendQuery(cur,"drop function testproc");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create or replace "
		"procedure testproc("
		"	in1 in number, "
		"	in2 in number, "
		"	in3 in varchar2, "
		"	out1 out number, "
		"	out2 out number, "
		"	out3 out varchar2) "
		"is "
		"begin "
		"	out1:=in1; "
		"	out2:=in2; "
		"	out3:=in3; "
		"end;"));
	sqlrcur_prepareQuery(cur,
		"begin "
		"	testproc(:in1,:in2,:in3,:out1,:out2,:out3); "
		"end;");
	sqlrcur_inputBindLong(cur,"in1",1);
	sqlrcur_inputBindDouble(cur,"in2",2.5,2,1);
	sqlrcur_inputBindString(cur,"in3","hello");
	sqlrcur_defineOutputBindInteger(cur,"out1");
	sqlrcur_defineOutputBindDouble(cur,"out2");
	sqlrcur_defineOutputBindString(cur,"out3",20);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqInt(sqlrcur_getOutputBindInteger(cur,"out1"),1);
	assertEqDbl(sqlrcur_getOutputBindDouble(cur,"out2"),2.5);
	assertEqStr(sqlrcur_getOutputBindString(cur,"out3"),"hello");
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	printf("\n");


	// stored procedure returning result set
	printf("STORED PROCEDURE RETURNING RESULT SET: \n");
	sqlrcur_sendQuery(cur,"drop package types");
	sqlrcur_sendQuery(cur,"drop function testproc");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create or replace package types is "
		"	type cursorType is ref cursor; "
		"end;"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create or replace "
		"function testproc(value in number) "
		"	return types.cursortype "
		"is "
		"	l_cursor    types.cursorType; "
		"begin "
		"	open l_cursor for "
		"		select "
		"			* "
		"		from "
		"			( "
		"			select 1 as testnumber from dual "
		"			union "
		"			select 2 as testnumber from dual "
		"			union "
		"			select 3 as testnumber from dual "
		"			union "
		"			select 4 as testnumber from dual "
		"			union "
		"			select 5 as testnumber from dual "
		"			union "
		"			select 6 as testnumber from dual "
		"			union "
		"			select 7 as testnumber from dual "
		"			union "
		"			select 8 as testnumber from dual "
		"			) "
		"		where "
		"			testnumber>value; "
		"	return l_cursor; "
		"end;"));
	sqlrcur_prepareQuery(cur,
		"begin "
		"	:curs1:=testproc(5); "
		"	:curs2:=testproc(0); "
		"end;");
	sqlrcur_defineOutputBindCursor(cur,"curs1");
	sqlrcur_defineOutputBindCursor(cur,"curs2");
	assertTrue(sqlrcur_executeQuery(cur));
	bindcur1=sqlrcur_getOutputBindCursor(cur,"curs1");
	assertTrue(sqlrcur_fetchFromBindCursor(bindcur1));
	assertEqStr(sqlrcur_getFieldByIndex(bindcur1,0,0),"6");
	assertEqStr(sqlrcur_getFieldByIndex(bindcur1,1,0),"7");
	assertEqStr(sqlrcur_getFieldByIndex(bindcur1,2,0),"8");
	sqlrcur_free(bindcur1);
	bindcur2=sqlrcur_getOutputBindCursor(cur,"curs2");
	assertTrue(sqlrcur_fetchFromBindCursor(bindcur2));
	assertEqStr(sqlrcur_getFieldByIndex(bindcur2,0,0),"1");
	assertEqStr(sqlrcur_getFieldByIndex(bindcur2,1,0),"2");
	assertEqStr(sqlrcur_getFieldByIndex(bindcur2,2,0),"3");
	sqlrcur_free(bindcur2);
	assertTrue(sqlrcur_sendQuery(cur,"drop function testproc"));
	assertTrue(sqlrcur_sendQuery(cur,"drop package types"));
	printf("\n");


	// temporary tables
	printf("TEMPORARY TABLES: \n");
	sqlrcur_prepareQuery(cur,"drop table $(HOSTNAME)_temptabledelete");
	sqlrcur_subString(cur,"HOSTNAME",hostname);
	sqlrcur_executeQuery(cur);
	sqlrcur_prepareQuery(cur,
		"create global temporary table $(HOSTNAME)_temptabledelete ( "
		"	col1 number "
		") on commit delete rows");
	sqlrcur_subString(cur,"HOSTNAME",hostname);
	sqlrcur_executeQuery(cur);
	sqlrcur_prepareQuery(cur,
			"insert into $(HOSTNAME)_temptabledelete values (1)");
	sqlrcur_subString(cur,"HOSTNAME",hostname);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_prepareQuery(cur,
			"select count(*) from $(HOSTNAME)_temptabledelete");
	sqlrcur_subString(cur,"HOSTNAME",hostname);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,
			"select count(*) from $(HOSTNAME)_temptabledelete");
	sqlrcur_subString(cur,"HOSTNAME",hostname);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"0");
	sqlrcur_prepareQuery(cur,"drop table $(HOSTNAME)_temptabledelete");
	sqlrcur_subString(cur,"HOSTNAME",hostname);
	sqlrcur_executeQuery(cur);
	printf("\n");
	sqlrcur_prepareQuery(cur,
			"truncate table $(HOSTNAME)_temptablepreserve");
	sqlrcur_subString(cur,"HOSTNAME",hostname);
	sqlrcur_executeQuery(cur);
	sqlrcur_prepareQuery(cur,"drop table $(HOSTNAME)_temptablepreserve");
	sqlrcur_subString(cur,"HOSTNAME",hostname);
	sqlrcur_executeQuery(cur);
	sqlrcur_prepareQuery(cur,
		"create global temporary table $(HOSTNAME)_temptablepreserve ("
		"	col1 number "
		") on commit preserve rows");
	sqlrcur_subString(cur,"HOSTNAME",hostname);
	sqlrcur_executeQuery(cur);
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	$(HOSTNAME)_temptablepreserve "
		"values (1)");
	sqlrcur_subString(cur,"HOSTNAME",hostname);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_prepareQuery(cur,
			"select count(*) from $(HOSTNAME)_temptablepreserve");
	sqlrcur_subString(cur,"HOSTNAME",hostname);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertTrue(sqlrcon_commit(con));
	sqlrcur_prepareQuery(cur,
			"select count(*) from $(HOSTNAME)_temptablepreserve");
	sqlrcur_subString(cur,"HOSTNAME",hostname);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"1");
	sqlrcon_endSession(con);
	printf("\n");
	sqlrcur_prepareQuery(cur,
			"select count(*) from $(HOSTNAME)_temptablepreserve");
	sqlrcur_subString(cur,"HOSTNAME",hostname);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"0");
	sqlrcur_prepareQuery(cur,
			"truncate table $(HOSTNAME)_temptablepreserve");
	sqlrcur_subString(cur,"HOSTNAME",hostname);
	assertTrue(sqlrcur_executeQuery(cur));
	sleep(2);
	sqlrcur_prepareQuery(cur,"drop table $(HOSTNAME)_temptablepreserve");
	sqlrcur_subString(cur,"HOSTNAME",hostname);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_prepareQuery(cur,
			"select count(*) from $(HOSTNAME)_temptablepreserve");
	sqlrcur_subString(cur,"HOSTNAME",hostname);
	assertFalse(sqlrcur_executeQuery(cur));
	printf("\n");


	// encoded binary data
	printf("ENCODED BINARY DATA: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,"create table testtable (col1 blob)"));
	for (i=0; i<256; i++) {
		buffer[i]=(unsigned char)i;
	}
	strcpy(querystr,"insert into testtable values ('");
	for (i=0; i<sizeof(buffer); i++) {
		snprintf(hex,sizeof(hex),"%02x",buffer[i]);
		strcat(querystr,hex);
	}
	strcat(querystr,"')");
	assertTrue(sqlrcur_sendQuery(cur,querystr));
	assertTrue(sqlrcur_sendQuery(cur,"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),sizeof(buffer));
	assertEqBin(sqlrcur_getFieldByIndex(cur,0,0),
						buffer,sizeof(buffer));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// quotes
	printf("QUOTES: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(
			cur,"create table testtable (col1 varchar2(4))"));
	assertTrue(sqlrcur_sendQuery(
			cur,"insert into testtable values ('''''')"));
	assertTrue(sqlrcur_sendQuery(cur,"select col1 from testtable"));
	assertEqInt(sqlrcur_getFieldLengthByIndex(cur,0,0),2);
	assertEqStr(sqlrcur_getFieldByIndex(cur,0,0),"''");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// last insert id
	// oracle doesn't support auto-increment


	// database is schema
	printf("DATABASE IS SCHEMA: \n");
	assertTrue(sqlrcon_getDatabaseIsSchema(con));
	printf("\n");


	// catalog list
	printf("CATALOG LIST: \n");
	assertTrue(sqlrcur_getCatalogList(cur,NULL));
	assertEqStr(sqlrcur_getColumnName(cur,0),"Database");
	assertEqInt(sqlrcur_rowCount(cur),0);
	printf("\n");


	// schema list
	printf("SCHEMA LIST: \n");
	assertTrue(sqlrcur_getSchemaList(cur,NULL));
	assertEqStr(sqlrcur_getColumnName(cur,0),"Database");
	assertInResultSet(cur,"Database",upperhostname);
	printf("\n");


	// table type list
	printf("TABLE TYPE LIST: \n");
	assertTrue(sqlrcur_getTableTypeList(cur));
	assertEqStr(sqlrcur_getColumnName(cur,0),"table_type");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"table_type"),"SYNONYM");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"table_type"),"TABLE");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"table_type"),"VIEW");
	printf("\n");


	// table list
	printf("TABLE LIST: \n");
	sqlrcur_sendQuery(cur,"drop table testtable1");
	sqlrcur_sendQuery(cur,"drop table testtable2");
	sqlrcur_sendQuery(cur,"drop table testtable3");
	sqlrcur_sendQuery(cur,"drop table testtable4");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable1 ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
		"	testclob clob, "
		"	testblob blob)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable2 ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
		"	testclob clob, "
		"	testblob blob)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable3 ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
		"	testclob clob, "
		"	testblob blob)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable4 ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
		"	testclob clob, "
		"	testblob blob)"));
	assertTrue(sqlrcur_getTableList(cur,NULL));
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE1");
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE2");
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE3");
	assertInResultSet(cur,"Tables_in_xxx","TESTTABLE4");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable1"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable2"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable3"));
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable4"));
	printf("\n");


	// type info list
	printf("TYPE INFO LIST: \n");
	assertTrue(sqlrcur_getTypeInfoList(cur,"number"));
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
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"NUMBER");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"-7");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"NUMBER");
	assertTrue(sqlrcur_getTypeInfoList(cur,"char"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"CHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"2000");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"CHAR");
	assertTrue(sqlrcur_getTypeInfoList(cur,"varchar2"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"VARCHAR2");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"12");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"32767");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"VARCHAR2");
	assertTrue(sqlrcur_getTypeInfoList(cur,"date"));
	assertEqStr(sqlrcur_getFieldByName(cur,0,"type_name"),"DATE");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"92");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"precision"),"7");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"local_type_name"),"DATE");
	printf("\n");


	// column list
	printf("COLUMN LIST: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
		"	testclob clob, "
		"	testblob blob)"));
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
	assertEqStr(sqlrcur_getFieldByName(cur,0,"column_name"),"TESTNUMBER");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"column_name"),"TESTCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"column_name"),"TESTVARCHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"column_name"),"TESTDATE");
	assertEqStr(sqlrcur_getFieldByName(cur,4,"column_name"),"TESTLONG");
	assertEqStr(sqlrcur_getFieldByName(cur,5,"column_name"),"TESTCLOB");
	assertEqStr(sqlrcur_getFieldByName(cur,6,"column_name"),"TESTBLOB");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"NUMBER");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"data_type"),"CHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"data_type"),"VARCHAR2");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"data_type"),"DATE");
	assertEqStr(sqlrcur_getFieldByName(cur,4,"data_type"),"LONG");
	assertEqStr(sqlrcur_getFieldByName(cur,5,"data_type"),"CLOB");
	assertEqStr(sqlrcur_getFieldByName(cur,6,"data_type"),"BLOB");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// column list - auto_increment, primary key
	// oracle doesn't support auto_increment
	printf("COLUMN LIST - auto_increment, primary key: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 number primary key, "
		"	col2 number)"));
	assertTrue(sqlrcur_getColumnList(cur,"testtable",NULL));
	assertContains(sqlrcur_getFieldByName(cur,0,"column_key"),
				"PRI");
	assertNotContains(sqlrcur_getFieldByName(cur,1,"column_key"),
				"PRI");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// primary keys list
	printf("PRIMARY KEYS LIST: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 number primary key, "
		"	col2 number)"));
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
	assertEqStr(sqlrcur_getFieldByName(cur,0,"table"),"TESTTABLE");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"seq_in_index"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"column_name"),"COL1");
	{
		const char *keyname=sqlrcur_getFieldByName(cur,0,"key_name");
		assertTrue(keyname && keyname[0]);
	}
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// key and index list
	printf("KEY AND INDEX LIST: \n");
	sqlrcur_sendQuery(cur,"drop table testtable");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	col1 number primary key, "
		"	col2 number)"));
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
	assertEqStr(sqlrcur_getFieldByName(cur,0,"table"),"TESTTABLE");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"non_unique"),"0");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"seq_in_index"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"column_name"),"COL1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"collation"),"A");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"index_type"),"3");
	{
		const char *keyname=sqlrcur_getFieldByName(cur,0,"key_name");
		assertTrue(keyname && keyname[0]);
	}
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable"));
	printf("\n");


	// procedure list
	printf("PROCEDURE LIST: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc1");
	sqlrcur_sendQuery(cur,"drop procedure testproc2");
	sqlrcur_sendQuery(cur,"drop procedure testproc3");
	sqlrcur_sendQuery(cur,"drop procedure testproc4");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc1("
		"	in1 in number, "
		"	in2 in char, "
		"	in3 in varchar2, "
		"	in4 in date) as "
		"begin "
		"	null; "
		"end;"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc2("
		"	in1 in number, "
		"	in2 in char, "
		"	in3 in varchar2, "
		"	in4 in date) as "
		"begin "
		"	null; "
		"end;"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc3("
		"	in1 in number, "
		"	in2 in char, "
		"	in3 in varchar2, "
		"	in4 in date) as "
		"begin "
		"	null; "
		"end;"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc4("
		"	in1 in number, "
		"	in2 in char, "
		"	in3 in varchar2, "
		"	in4 in date) as "
		"begin "
		"	null; "
		"end;"));
	assertTrue(sqlrcur_getProcedureList(cur,NULL));
	assertInResultSet(cur,"routine_name","TESTPROC1");
	assertInResultSet(cur,"routine_name","TESTPROC2");
	assertInResultSet(cur,"routine_name","TESTPROC3");
	assertInResultSet(cur,"routine_name","TESTPROC4");
	printf("\n");


	// procedure parameter list
	printf("PROCEDURE PARAMETER LIST: \n");
	assertTrue(sqlrcur_getProcedureParameterList(cur,"testproc1",NULL));
	assertEqStr(sqlrcur_getColumnName(cur,0),"parameter_name");
	assertEqStr(sqlrcur_getColumnName(cur,1),"parameter_mode");
	assertEqStr(sqlrcur_getColumnName(cur,2),"data_type");
	assertEqStr(sqlrcur_getColumnName(cur,3),"character_maximum_length");
	assertEqStr(sqlrcur_getColumnName(cur,4),"ordinal_position");
	assertEqInt(sqlrcur_rowCount(cur),4);
	assertEqStr(sqlrcur_getFieldByName(cur,0,"parameter_name"),"IN1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"data_type"),"NUMBER");
	assertEqStr(sqlrcur_getFieldByName(cur,0,"ordinal_position"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"parameter_name"),"IN2");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"data_type"),"CHAR");
	assertEqStr(sqlrcur_getFieldByName(cur,1,"ordinal_position"),"2");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"parameter_name"),"IN3");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"parameter_mode"),"1");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"data_type"),"VARCHAR2");
	assertEqStr(sqlrcur_getFieldByName(cur,2,"ordinal_position"),"3");
	assertEqStr(sqlrcur_getFieldByName(cur,3,"parameter_name"),"IN4");
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
		"	testnumber"));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber"));
	printf("\n");
	assertFalse(sqlrcur_sendQuery(
			cur,"insert into testtable values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(
			cur,"insert into testtable values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(
			cur,"insert into testtable values (1,2,3,4)"));
	assertFalse(sqlrcur_sendQuery(
			cur,"insert into testtable values (1,2,3,4)"));
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	printf("\n");


	sqlrcur_free(cur);
	sqlrcon_free(con);
	free(hostname);

	reportTestStatus();

	return status;
}
