// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclientwrapper.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "asserts.c"

sqlrcon	con;
sqlrcur	cur;
sqlrcur	bindcur;
sqlrcon	secondcon;
sqlrcur	secondcur;

int main(int argc, char **argv) {

	const char	*bindvars[]={"1","2","3","4","5",NULL};
	const char	*bindvals[]={"4","testchar4","testvarchar4",
						"01-JAN-2004","testlong4"};
	const char	*arraybindvars[]={"var1","var2","var3",
						"var4","var5",NULL};
	const char	*arraybindvals[]={"7","testchar7","testvarchar7",
						"01-JAN-2007","testlong7"};
	int64_t		numvar;
	const char	*stringvar;
	double		floatvar;
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
	char	*filename;
	const char	*clobvar;
	long	clobvarlength;
	const char	*blobvar;
	long	blobvarlength;
	int	i;
	char	clobval[8*1024+1];
	const char	*clobbindvar;
	char	testval[4001];
	char	query[4000+25];

	const char	*cert="../sqlrelay.conf.d/tls/client.pem";
	const char	*ca="../sqlrelay.conf.d/tls/ca.pem";
	#ifdef _WIN32
		cert="..\\sqlrelay.conf.d\\tls\\client.pfx";
		ca="..\\sqlrelay.conf.d\\tls\\ca.pfx";
	#endif


	// instantiation
	con=sqlrcon_alloc("sqlrelay",9000,"/tmp/test.socket",NULL,NULL,0,1);
	cur=sqlrcur_alloc(con);
	sqlrcon_enableTls(con,NULL,cert,NULL,NULL,"ca",ca,0);

	// get database type


	// identify
	printf("IDENTIFY: \n");
	assertEqualsString(sqlrcon_identify(con),"oracle");
	printf("\n");


	// ping
	printf("PING: \n");
	assertTrue(sqlrcon_ping(con));
	printf("\n");

	// drop existing table
	sqlrcur_sendQuery(cur,"drop table testtable");


	// create temptable
	printf("CREATE TEMPTABLE: \n");
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
	assertEqualsInt(sqlrcur_affectedRows(cur),1);
	printf("\n");


	// bind by position
	printf("BIND BY POSITION: \n");
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
	assertEqualsInt(sqlrcur_countBindVariables(cur),7);
	sqlrcur_inputBindLong(cur,"1",2);
	sqlrcur_inputBindString(cur,"2","testchar2");
	sqlrcur_inputBindString(cur,"3","testvarchar2");
	sqlrcur_inputBindDate(cur,"4",2002,1,1,-1,-1,-1,0,NULL,0);
	sqlrcur_inputBindString(cur,"5","testlong2");
	sqlrcur_inputBindClob(cur,"6","testclob2",9);
	sqlrcur_inputBindBlob(cur,"7","testblob2",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",3);
	sqlrcur_inputBindString(cur,"2","testchar3");
	sqlrcur_inputBindString(cur,"3","testvarchar3");
	sqlrcur_inputBindDate(cur,"4",2003,1,1,-1,-1,-1,0,NULL,0);
	sqlrcur_inputBindString(cur,"5","testlong3");
	sqlrcur_inputBindClob(cur,"6","testclob3",9);
	sqlrcur_inputBindBlob(cur,"7","testblob3",9);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// array of binds by position
	printf("ARRAY OF BINDS BY POSITION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindStrings(cur,bindvars,bindvals);
	sqlrcur_inputBindClob(cur,"6","testclob4",9);
	sqlrcur_inputBindBlob(cur,"7","testblob4",9);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// bind by name
	printf("BIND BY NAME: \n");
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
	sqlrcur_inputBindLong(cur,"var1",5);
	sqlrcur_inputBindString(cur,"var2","testchar5");
	sqlrcur_inputBindString(cur,"var3","testvarchar5");
	sqlrcur_inputBindDate(cur,"var4",2005,1,1,-1,-1,-1,0,NULL,0);
	sqlrcur_inputBindString(cur,"var5","testlong5");
	sqlrcur_inputBindClob(cur,"var6","testclob5",9);
	sqlrcur_inputBindBlob(cur,"var7","testblob5",9);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",6);
	sqlrcur_inputBindString(cur,"var2","testchar6");
	sqlrcur_inputBindString(cur,"var3","testvarchar6");
	sqlrcur_inputBindDate(cur,"var4",2006,1,1,-1,-1,-1,0,NULL,0);
	sqlrcur_inputBindString(cur,"var5","testlong6");
	sqlrcur_inputBindClob(cur,"var6","testclob6",9);
	sqlrcur_inputBindBlob(cur,"var7","testblob6",9);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// array of binds by name
	printf("ARRAY OF BINDS BY NAME: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindStrings(cur,arraybindvars,arraybindvals);
	sqlrcur_inputBindClob(cur,"var6","testclob7",9);
	sqlrcur_inputBindBlob(cur,"var7","testblob7",9);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// bind by name with validation
	printf("BIND BY NAME WITH VALIDATION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",8);
	sqlrcur_inputBindString(cur,"var2","testchar8");
	sqlrcur_inputBindString(cur,"var3","testvarchar8");
	sqlrcur_inputBindDate(cur,"var4",2008,1,1,-1,-1,-1,0,NULL,0);
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
		"	testnumber "));
	printf("\n");


	// column count
	printf("COLUMN COUNT: \n");
	assertEqualsInt(sqlrcur_colCount(cur),7);
	printf("\n");


	// column names
	printf("COLUMN NAMES: \n");
	assertEqualsString(sqlrcur_getColumnName(cur,0),"TESTNUMBER");
	assertEqualsString(sqlrcur_getColumnName(cur,1),"TESTCHAR");
	assertEqualsString(sqlrcur_getColumnName(cur,2),"TESTVARCHAR");
	assertEqualsString(sqlrcur_getColumnName(cur,3),"TESTDATE");
	assertEqualsString(sqlrcur_getColumnName(cur,4),"TESTLONG");
	assertEqualsString(sqlrcur_getColumnName(cur,5),"TESTCLOB");
	assertEqualsString(sqlrcur_getColumnName(cur,6),"TESTBLOB");
	cols=sqlrcur_getColumnNames(cur);
	assertEqualsString(cols[0],"TESTNUMBER");
	assertEqualsString(cols[1],"TESTCHAR");
	assertEqualsString(cols[2],"TESTVARCHAR");
	assertEqualsString(cols[3],"TESTDATE");
	assertEqualsString(cols[4],"TESTLONG");
	assertEqualsString(cols[5],"TESTCLOB");
	assertEqualsString(cols[6],"TESTBLOB");
	printf("\n");


	// column types
	printf("COLUMN TYPES: \n");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),"NUMBER");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"TESTNUMBER"),"NUMBER");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,1),"CHAR");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"TESTCHAR"),"CHAR");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,2),"VARCHAR2");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"TESTVARCHAR"),"VARCHAR2");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,3),"DATE");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"TESTDATE"),"DATE");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,4),"LONG");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"TESTLONG"),"LONG");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,5),"CLOB");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"TESTCLOB"),"CLOB");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,6),"BLOB");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"TESTBLOB"),"BLOB");
	printf("\n");


	// column length
	printf("COLUMN LENGTH: \n");
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,0),22);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"TESTNUMBER"),22);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,1),40);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"TESTCHAR"),40);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,2),40);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"TESTVARCHAR"),40);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,3),7);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"TESTDATE"),7);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,4),0);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"TESTLONG"),0);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,5),0);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"TESTCLOB"),0);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,6),0);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"TESTBLOB"),0);
	printf("\n");


	// longest column
	printf("LONGEST COLUMN: \n");
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,0),1);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"TESTNUMBER"),1);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,1),40);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"TESTCHAR"),40);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,2),12);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"TESTVARCHAR"),12);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,3),9);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"TESTDATE"),9);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,4),9);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"TESTLONG"),9);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,5),9);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"TESTCLOB"),9);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,6),9);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"TESTBLOB"),9);
	printf("\n");


	// row count
	printf("ROW COUNT: \n");
	assertEqualsInt(sqlrcur_rowCount(cur),8);
	printf("\n");


	// total rows
	printf("TOTAL ROWS: \n");
	assertEqualsInt(sqlrcur_totalRows(cur),0);
	printf("\n");


	// first row index
	printf("FIRST ROW INDEX: \n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),0);
	printf("\n");


	// end of result set
	printf("END OF RESULT SET: \n");
	assertTrue(sqlrcur_endOfResultSet(cur));
	printf("\n");


	// fields by index
	printf("FIELDS BY INDEX: \n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"testchar1                               ");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"testvarchar1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,3),"01-JAN-01");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,4),"testlong1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,5),"testclob1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,6),"");
	printf("\n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,1),"testchar8                               ");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,2),"testvarchar8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,3),"01-JAN-08");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,4),"testlong8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,5),"testclob8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,6),"testblob8");
	printf("\n");


	// field lengths by index
	printf("FIELD LENGTHS BY INDEX: \n");
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,0),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,1),40);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,2),12);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,3),9);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,4),9);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,5),9);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,6),0);
	printf("\n");
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,0),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,1),40);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,2),12);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,3),9);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,4),9);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,5),9);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,6),9);
	printf("\n");


	// fields by name
	printf("FIELDS BY NAME: \n");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"TESTNUMBER"),"1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"TESTCHAR"),"testchar1                               ");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"TESTVARCHAR"),"testvarchar1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"TESTDATE"),"01-JAN-01");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"TESTLONG"),"testlong1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"TESTCLOB"),"testclob1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"TESTBLOB"),"");
	printf("\n");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"TESTNUMBER"),"8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"TESTCHAR"),"testchar8                               ");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"TESTVARCHAR"),"testvarchar8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"TESTDATE"),"01-JAN-08");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"TESTLONG"),"testlong8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"TESTCLOB"),"testclob8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"TESTBLOB"),"testblob8");
	printf("\n");


	// field lengths by name
	printf("FIELD LENGTHS BY NAME: \n");
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"TESTNUMBER"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"TESTCHAR"),40);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"TESTVARCHAR"),12);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"TESTDATE"),9);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"TESTLONG"),9);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"TESTCLOB"),9);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"TESTBLOB"),0);
	printf("\n");
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"TESTNUMBER"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"TESTCHAR"),40);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"TESTVARCHAR"),12);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"TESTDATE"),9);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"TESTLONG"),9);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"TESTCLOB"),9);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"TESTBLOB"),9);
	printf("\n");


	// fields by array
	printf("FIELDS BY ARRAY: \n");
	fields=sqlrcur_getRow(cur,0);
	assertEqualsString(fields[0],"1");
	assertEqualsString(fields[1],"testchar1                               ");
	assertEqualsString(fields[2],"testvarchar1");
	assertEqualsString(fields[3],"01-JAN-01");
	assertEqualsString(fields[4],"testlong1");
	assertEqualsString(fields[5],"testclob1");
	assertEqualsString(fields[6],"");
	printf("\n");


	// field lengths by array
	printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=sqlrcur_getRowLengths(cur,0);
	assertEqualsInt(fieldlens[0],1);
	assertEqualsInt(fieldlens[1],40);
	assertEqualsInt(fieldlens[2],12);
	assertEqualsInt(fieldlens[3],9);
	assertEqualsInt(fieldlens[4],9);
	assertEqualsInt(fieldlens[5],9);
	assertEqualsInt(fieldlens[6],0);
	printf("\n");


	// individual substitutions
	printf("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"select $(var1),'$(var2)',$(var3) from dual");
	sqlrcur_subLong(cur,"var1",1);
	sqlrcur_subString(cur,"var2","hello");
	sqlrcur_subDouble(cur,"var3",10.5556,6,4);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// fields
	printf("FIELDS: \n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"10.5556");
	printf("\n");


	// array substitutions
	printf("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"select $(var1),$(var2),$(var3) from dual");
	sqlrcur_subLongs(cur,subvars,subvallongs);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// fields
	printf("FIELDS: \n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"2");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"3");
	printf("\n");


	// array substitutions
	printf("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,
		"select "
		"	'$(var1)', "
		"	'$(var2)', "
		"	'$(var3)' "
		"from "
		"	dual ");
	sqlrcur_subStrings(cur,subvars,subvalstrings);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// fields
	printf("FIELDS: \n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"hi");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"hello");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"bye");
	printf("\n");


	// array substitutions
	printf("ARRAY SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"select $(var1),$(var2),$(var3) from dual");
	sqlrcur_subDoubles(cur,subvars,subvaldoubles,precs,scales);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// fields
	printf("FIELDS: \n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"10.55");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"10.556");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"10.5556");
	printf("\n");


	// nulls as nulls
	printf("NULLS AS NULLS: \n");
	sqlrcur_getNullsAsNulls(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select NULL,1,NULL from dual"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),NULL);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select NULL,1,NULL from dual"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"");
	sqlrcur_getNullsAsNulls(cur);
	printf("\n");


	// result set buffer size
	printf("RESULT SET BUFFER SIZE: \n");
	assertEqualsInt(sqlrcur_getResultSetBufferSize(cur),0);
	sqlrcur_setResultSetBufferSize(cur,2);
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber "));
	assertEqualsInt(sqlrcur_getResultSetBufferSize(cur),2);
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),0);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),2);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,1,0),"2");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,2,0),"3");
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),2);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),4);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),6);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),8);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),8);
	assertTrue(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),8);
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
		"	testnumber "));
	assertEqualsString(sqlrcur_getColumnName(cur,0),NULL);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,0),0);
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),NULL);
	sqlrcur_getColumnInfo(cur);
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber "));
	assertEqualsString(sqlrcur_getColumnName(cur,0),"TESTNUMBER");
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,0),22);
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),"NUMBER");
	printf("\n");


	// suspended session
	printf("SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber "));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	printf("\n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,1,0),"2");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,2,0),"3");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,3,0),"4");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,4,0),"5");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,5,0),"6");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	printf("\n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber "));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	printf("\n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,1,0),"2");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,2,0),"3");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,3,0),"4");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,4,0),"5");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,5,0),"6");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	printf("\n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber "));
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	printf("\n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,1,0),"2");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,2,0),"3");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,3,0),"4");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,4,0),"5");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,5,0),"6");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
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
		"	testnumber "));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,2,0),"3");
	id=sqlrcur_getResultSetId(cur);
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	assertTrue(sqlrcur_resumeResultSet(cur,id));
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),4);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),6);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),6);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),8);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),8);
	assertTrue(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),8);
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
		"	testnumber "));
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqualsString(filename,"cachefile1");
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	free(filename);
	printf("\n");


	// column count for cached result set
	printf("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqualsInt(sqlrcur_colCount(cur),7);
	printf("\n");


	// column names for cached result set
	printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqualsString(sqlrcur_getColumnName(cur,0),"TESTNUMBER");
	assertEqualsString(sqlrcur_getColumnName(cur,1),"TESTCHAR");
	assertEqualsString(sqlrcur_getColumnName(cur,2),"TESTVARCHAR");
	assertEqualsString(sqlrcur_getColumnName(cur,3),"TESTDATE");
	assertEqualsString(sqlrcur_getColumnName(cur,4),"TESTLONG");
	assertEqualsString(sqlrcur_getColumnName(cur,5),"TESTCLOB");
	assertEqualsString(sqlrcur_getColumnName(cur,6),"TESTBLOB");
	cols=sqlrcur_getColumnNames(cur);
	assertEqualsString(cols[0],"TESTNUMBER");
	assertEqualsString(cols[1],"TESTCHAR");
	assertEqualsString(cols[2],"TESTVARCHAR");
	assertEqualsString(cols[3],"TESTDATE");
	assertEqualsString(cols[4],"TESTLONG");
	assertEqualsString(cols[5],"TESTCLOB");
	assertEqualsString(cols[6],"TESTBLOB");
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
		"	testnumber "));
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqualsString(filename,"cachefile1");
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize(cur,0);
	free(filename);
	printf("\n");


	// from one cache file to another
	printf("FROM ONE CACHE FILE TO ANOTHER: \n");
	sqlrcur_cacheToFile(cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile1"));
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile2"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	printf("\n");


	// from one cache file to another with result set buffer size
	printf("FROM ONE CACHE FILE TO ANOTHER "
		"WITH RESULT SET BUFFER SIZE: \n");
	sqlrcur_setResultSetBufferSize(cur,2);
	sqlrcur_cacheToFile(cur,"cachefile2");
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile1"));
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,"cachefile2"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,8,0),NULL);
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
		"	testnumber "));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,2,0),"3");
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqualsString(filename,"cachefile1");
	id=sqlrcur_getResultSetId(cur);
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	printf("\n");
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	assertTrue(sqlrcur_resumeCachedResultSet(cur,id,filename));
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),4);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),6);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),6);
	assertFalse(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),8);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	printf("\n");
	assertEqualsInt(sqlrcur_firstRowIndex(cur),8);
	assertTrue(sqlrcur_endOfResultSet(cur));
	assertEqualsInt(sqlrcur_rowCount(cur),8);
	sqlrcur_cacheOff(cur);
	printf("\n");
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	sqlrcur_setResultSetBufferSize(cur,0);
	free(filename);
	printf("\n");


	// commit
	printf("COMMIT: \n");
	secondcon=sqlrcon_alloc("sqlrelay",9000,
				"/tmp/test.socket",NULL,NULL,0,1);
	secondcur=sqlrcur_alloc(secondcon);
	sqlrcon_enableTls(secondcon,NULL,cert,NULL,NULL,"ca",ca,0);
	assertTrue(sqlrcur_sendQuery(secondcur,
		"select "
		"	count(*) "
		"from "
		"	testtable "));
	assertEqualsString(sqlrcur_getFieldByIndex(secondcur,0,0),"0");
	assertTrue(sqlrcon_commit(con));
	assertTrue(sqlrcur_sendQuery(secondcur,
		"select "
		"	count(*) "
		"from "
		"	testtable "));
	assertEqualsString(sqlrcur_getFieldByIndex(secondcur,0,0),"8");
	assertTrue(sqlrcon_autoCommitOn(con));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	10, "
		"	'testchar10', "
		"	'testvarchar10', "
		"	'01-JAN-2010', "
		"	'testlong10', "
		"	'testclob10', "
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery(secondcur,
		"select "
		"	count(*) "
		"from "
		"	testtable "));
	assertEqualsString(sqlrcur_getFieldByIndex(secondcur,0,0),"9");
	assertTrue(sqlrcon_autoCommitOff(con));
	printf("\n");


	// output bind by position
	printf("OUTPUT BIND BY POSITION: \n");
	sqlrcur_prepareQuery(cur,
		"begin "
		"	:numvar:=1; "
		"	:stringvar:='hello'; "
		"	:floatvar:=2.5; "
		"end;");
	sqlrcur_defineOutputBindInteger(cur,"1");
	sqlrcur_defineOutputBindString(cur,"2",10);
	sqlrcur_defineOutputBindDouble(cur,"3");
	assertTrue(sqlrcur_executeQuery(cur));
	numvar=sqlrcur_getOutputBindInteger(cur,"1");
	stringvar=sqlrcur_getOutputBindString(cur,"2");
	floatvar=sqlrcur_getOutputBindDouble(cur,"3");
	assertEqualsInt(numvar,1);
	assertEqualsString(stringvar,"hello");
	assertEqualsDouble(floatvar,2.5);
	printf("\n");


	// output bind by name
	printf("OUTPUT BIND BY NAME: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_defineOutputBindInteger(cur,"numvar");
	sqlrcur_defineOutputBindString(cur,"stringvar",10);
	sqlrcur_defineOutputBindDouble(cur,"floatvar");
	assertTrue(sqlrcur_executeQuery(cur));
	numvar=sqlrcur_getOutputBindInteger(cur,"numvar");
	stringvar=sqlrcur_getOutputBindString(cur,"stringvar");
	floatvar=sqlrcur_getOutputBindDouble(cur,"floatvar");
	assertEqualsInt(numvar,1);
	assertEqualsString(stringvar,"hello");
	assertEqualsDouble(floatvar,2.5);
	printf("\n");


	// output bind by name with validation
	printf("OUTPUT BIND BY NAME WITH VALIDATION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_defineOutputBindInteger(cur,"numvar");
	sqlrcur_defineOutputBindString(cur,"stringvar",10);
	sqlrcur_defineOutputBindDouble(cur,"floatvar");
	sqlrcur_defineOutputBindString(cur,"dummyvar",10);
	sqlrcur_validateBinds(cur);
	assertTrue(sqlrcur_executeQuery(cur));
	numvar=sqlrcur_getOutputBindInteger(cur,"numvar");
	stringvar=sqlrcur_getOutputBindString(cur,"stringvar");
	floatvar=sqlrcur_getOutputBindDouble(cur,"floatvar");
	assertEqualsInt(numvar,1);
	assertEqualsString(stringvar,"hello");
	assertEqualsDouble(floatvar,2.5);
	printf("\n");


	// clob and blob output bind
	printf("CLOB AND BLOB OUTPUT BIND: \n");
	sqlrcur_sendQuery(cur,"drop table testtable1");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable1 ("
		"	testclob clob, "
		"	testblob blob)"));
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	testtable1 "
		"values ("
		"	'hello', "
		"	:var1)");
	sqlrcur_inputBindBlob(cur,"var1","hello",5);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_prepareQuery(cur,
		"begin "
		"	select testclob into :clobvar from testtable1; "
		"	select testblob into :blobvar from testtable1; "
		"end;");
	sqlrcur_defineOutputBindClob(cur,"clobvar");
	sqlrcur_defineOutputBindBlob(cur,"blobvar");
	assertTrue(sqlrcur_executeQuery(cur));
	clobvar=sqlrcur_getOutputBindClob(cur,"clobvar");
	clobvarlength=sqlrcur_getOutputBindLength(cur,"clobvar");
	blobvar=sqlrcur_getOutputBindBlob(cur,"blobvar");
	blobvarlength=sqlrcur_getOutputBindLength(cur,"blobvar");
	assertEqualsStringWithLength(clobvar,"hello",5);
	assertEqualsInt(clobvarlength,5);
	assertEqualsStringWithLength(blobvar,"hello",5);
	assertEqualsInt(blobvarlength,5);
	sqlrcur_sendQuery(cur,"drop table testtable1");
	printf("\n");


	// null and empty clobs and clobs
	printf("NULL AND EMPTY CLOBS AND CLOBS: \n");
	sqlrcur_getNullsAsNulls(cur);
	sqlrcur_sendQuery(cur,
		"create table testtable1 ("
		"	testclob1 clob, "
		"	testclob2 clob, "
		"	testblob1 blob, "
		"	testblob2 blob)");
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	testtable1 "
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
	sqlrcur_sendQuery(cur,"select * from testtable1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),NULL);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),NULL);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),NULL);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,3),NULL);
	sqlrcur_sendQuery(cur,"drop table testtable1");
	printf("\n");


	// cursor binds
	printf("CURSOR BINDS: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"create or replace package types "
		"is "
		"	type cursorType is ref cursor; "
		"end;"));
	assertTrue(sqlrcur_sendQuery(cur,
		"create or replace "
		"function sp_testtable return types.cursortype "
		"as "
		"	l_cursor    types.cursorType; "
		"begin "
		"	open l_cursor for "
		"		select * from testtable; "
		"	return l_cursor; "
		"end;"));
	sqlrcur_prepareQuery(cur,"begin  :curs:=sp_testtable; end;");
	sqlrcur_defineOutputBindCursor(cur,"curs");
	assertTrue(sqlrcur_executeQuery(cur));
	bindcur=sqlrcur_getOutputBindCursor(cur,"curs");
	assertTrue(sqlrcur_fetchFromBindCursor(bindcur));
	assertEqualsString(sqlrcur_getFieldByIndex(bindcur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(bindcur,1,0),"2");
	assertEqualsString(sqlrcur_getFieldByIndex(bindcur,2,0),"3");
	assertEqualsString(sqlrcur_getFieldByIndex(bindcur,3,0),"4");
	assertEqualsString(sqlrcur_getFieldByIndex(bindcur,4,0),"5");
	assertEqualsString(sqlrcur_getFieldByIndex(bindcur,5,0),"6");
	assertEqualsString(sqlrcur_getFieldByIndex(bindcur,6,0),"7");
	assertEqualsString(sqlrcur_getFieldByIndex(bindcur,7,0),"8");
	sqlrcur_free(bindcur);
	printf("\n");


	// long clob
	printf("LONG CLOB: \n");
	sqlrcur_sendQuery(cur,"drop table testtable2");
	sqlrcur_sendQuery(cur,"create table testtable2 (testclob clob)");
	sqlrcur_prepareQuery(cur,"insert into testtable2 values (:clobval)");
	for (i=0; i<8*1024; i++) {
		clobval[i]='C';
	}
	clobval[8*1024]='\0';
	sqlrcur_inputBindClob(cur,"clobval",clobval,8*1024);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_sendQuery(cur,"select testclob from testtable2");
	assertEqualsString(clobval,sqlrcur_getFieldByName(cur,0,"TESTCLOB"));
	sqlrcur_prepareQuery(cur,
		"begin "
		"	select testclob into :clobbindval from testtable2; "
		"end;");
	sqlrcur_defineOutputBindClob(cur,"clobbindval");
	assertTrue(sqlrcur_executeQuery(cur));
	clobbindvar=sqlrcur_getOutputBindClob(cur,"clobbindval");
	assertEqualsInt(sqlrcur_getOutputBindLength(cur,"clobbindval"),8*1024);
	assertEqualsString(clobval,clobbindvar);
	sqlrcur_sendQuery(cur,"drop table testtable2");
	printf("\n");


	printf("LONG OUTPUT BIND\n");
	sqlrcur_sendQuery(cur,"drop table testtable2");
	sqlrcur_sendQuery(cur,
		"create table "
		"	testtable2 (testval varchar2(4000))");
	testval[4000]='\0';
	sqlrcur_prepareQuery(cur,"insert into testtable2 values (:testval)");
	for (i=0; i<4000; i++) {
		testval[i]='C';
	}
	sqlrcur_inputBindString(cur,"testval",testval);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_sendQuery(cur,"select testval from testtable2");
	assertEqualsString(testval,sqlrcur_getFieldByName(cur,0,"TESTVAL"));
	sprintf(query,"begin :bindval:='%s'; end;",testval);
	sqlrcur_prepareQuery(cur,query);
	sqlrcur_defineOutputBindString(cur,"bindval",4000);
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqualsInt(sqlrcur_getOutputBindLength(cur,"bindval"),4000);
	assertEqualsString(sqlrcur_getOutputBindString(cur,"bindval"),testval);
	sqlrcur_sendQuery(cur,"drop table testtable2");
	printf("\n");

	printf("NEGATIVE INPUT BIND\n");
	sqlrcur_sendQuery(cur,"create table testtable2 (testval number)");
	sqlrcur_prepareQuery(cur,"insert into testtable2 values (:testval)");
	sqlrcur_inputBindLong(cur,"testval",-1);
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_sendQuery(cur,"select testval from testtable2");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"TESTVAL"),"-1");
	sqlrcur_sendQuery(cur,"drop table testtable2");
	printf("\n");


	// finished suspended session
	printf("FINISHED SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber "));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,4,0),"5");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,5,0),"6");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	id=sqlrcur_getResultSetId(cur);
	sqlrcur_suspendResultSet(cur);
	assertTrue(sqlrcon_suspendSession(con));
	port=sqlrcon_getConnectionPort(con);
	socket=strdup(sqlrcon_getConnectionSocket(con));
	assertTrue(sqlrcon_resumeSession(con,port,socket));
	assertTrue(sqlrcur_resumeResultSet(cur,id));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,4,0),NULL);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,5,0),NULL);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,6,0),NULL);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),NULL);
	printf("\n");

	// drop existing table
	sqlrcur_sendQuery(cur,"drop table testtable");


	// bind validation
	printf("BIND VALIDATION: \n");
	sqlrcur_sendQuery(cur,"drop table testtable1");
	sqlrcur_sendQuery(cur,
		"create table testtable1 ("
		"	col1 varchar2(20), "
		"	col2 varchar2(20), "
		"	col3 varchar2(20))");
	sqlrcur_prepareQuery(cur,
		"insert into "
		"	testtable1 "
		"values ("
		"	$(var1), "
		"	$(var2), "
		"	$(var3))");
	sqlrcur_inputBindLong(cur,"var1",1);
	sqlrcur_inputBindLong(cur,"var2",2);
	sqlrcur_inputBindLong(cur,"var3",3);
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
	sqlrcur_sendQuery(cur,"drop table testtable1");
	printf("\n");


	// invalid queries
	printf("INVALID QUERIES: \n");
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber "));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber "));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber "));
	assertFalse(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber "));
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	2, "
		"	3, "
		"	4)"));
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	2, "
		"	3, "
		"	4)"));
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	2, "
		"	3, "
		"	4)"));
	assertFalse(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	2, "
		"	3, "
		"	4)"));
	printf("\n");
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	assertFalse(sqlrcur_sendQuery(cur,"create table testtable"));
	printf("\n");

	reportTestStatus();

	return status;
}
