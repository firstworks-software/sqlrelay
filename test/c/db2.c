// Copyright (c) David Muse
// See the file COPYING for more information.

#include <sqlrelay/sqlrclientwrapper.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "asserts.c"

sqlrcon	con;
sqlrcur	cur;
sqlrcon	secondcon;
sqlrcur	secondcur;

int main(int argc, char **argv) {

	const char	*bindvars[]={"1","2","3","4","5","6",
					"7","8","9","10",NULL};
	const char	*bindvals[]={"4","4","4","4.4","4.4","4.4",
			"testchar4","testvarchar4","01/01/2004","04:00:00"};
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

	// instantiation
	con=sqlrcon_alloc("sqlrelay",9000,
			"/tmp/test.socket","db2inst1","testpassword",0,1);
	cur=sqlrcur_alloc(con);

	// get database type


	// identify
	printf("IDENTIFY: \n");
	assertEqualsString(sqlrcon_identify(con),"db2");
	printf("\n");


	// ping
	printf("PING: \n");
	assertTrue(sqlrcon_ping(con));
	printf("\n");


	// isolation levels
	printf("ISOLATION LEVELS: \n");
	{
		const char	*isolationlevels[]={"CS","UR","RS","RR",NULL};
		const char	**il;
		for (il=isolationlevels; *il; il++) {
			assertTrue(sqlrcon_setIsolationLevel(con,*il));
			assertEqualsString(sqlrcon_getIsolationLevel(con),*il);
			printf("\n");
		}
		// reset to the default isolation level
		assertTrue(sqlrcon_setIsolationLevel(con,isolationlevels[0]));
		printf("\n");
	}

	// drop existing table
	sqlrcur_sendQuery(cur,"drop table testtable");


	// create temptable
	printf("CREATE TEMPTABLE: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable ("
		"	testsmallint smallint, "
		"	testint integer, "
		"	testbigint bigint, "
		"	testdecimal decimal(10,2), "
		"	testreal real, "
		"	testdouble double, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testdate date, "
		"	testtime time, "
		"	testtimestamp timestamp)"));
	printf("\n");


	// insert
	printf("INSERT: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	1, "
		"	1, "
		"	1.1, "
		"	1.1, "
		"	1.1, "
		"	'testchar1', "
		"	'testvarchar1', "
		"	'01/01/2001', "
		"	'01:00:00', "
		"	NULL)"));
	printf("\n");


	// bind by position
	printf("BIND BY POSITION: \n");
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
		"	NULL)");
	assertEqualsInt(sqlrcur_countBindVariables(cur),10);
	sqlrcur_inputBindLong(cur,"1",2);
	sqlrcur_inputBindLong(cur,"2",2);
	sqlrcur_inputBindLong(cur,"3",2);
	sqlrcur_inputBindDouble(cur,"4",2.2,4,2);
	sqlrcur_inputBindDouble(cur,"5",2.2,4,2);
	sqlrcur_inputBindDouble(cur,"6",2.2,4,2);
	sqlrcur_inputBindString(cur,"7","testchar2");
	sqlrcur_inputBindString(cur,"8","testvarchar2");
	sqlrcur_inputBindString(cur,"9","01/01/2002");
	sqlrcur_inputBindString(cur,"10","02:00:00");
	assertTrue(sqlrcur_executeQuery(cur));
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",3);
	sqlrcur_inputBindLong(cur,"2",3);
	sqlrcur_inputBindLong(cur,"3",3);
	sqlrcur_inputBindDouble(cur,"4",3.3,4,2);
	sqlrcur_inputBindDouble(cur,"5",3.3,4,2);
	sqlrcur_inputBindDouble(cur,"6",3.3,4,2);
	sqlrcur_inputBindString(cur,"7","testchar3");
	sqlrcur_inputBindString(cur,"8","testvarchar3");
	sqlrcur_inputBindString(cur,"9","01/01/2003");
	sqlrcur_inputBindString(cur,"10","03:00:00");
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// array of binds by position
	printf("ARRAY OF BINDS BY POSITION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindStrings(cur,bindvars,bindvals);
	assertTrue(sqlrcur_executeQuery(cur));
	printf("\n");


	// insert
	printf("INSERT: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	5, "
		"	5, "
		"	5, "
		"	5.5, "
		"	5.5, "
		"	5.5, "
		"	'testchar5', "
		"	'testvarchar5', "
		"	'01/01/2005', "
		"	'05:00:00', "
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	6, "
		"	6, "
		"	6, "
		"	6.6, "
		"	6.6, "
		"	6.6, "
		"	'testchar6', "
		"	'testvarchar6', "
		"	'01/01/2006', "
		"	'06:00:00', "
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	7, "
		"	7, "
		"	7, "
		"	7.7, "
		"	7.7, "
		"	7.7, "
		"	'testchar7', "
		"	'testvarchar7', "
		"	'01/01/2007', "
		"	'07:00:00', "
		"	NULL)"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable "
		"values ("
		"	8, "
		"	8, "
		"	8, "
		"	8.8, "
		"	8.8, "
		"	8.8, "
		"	'testchar8', "
		"	'testvarchar8', "
		"	'01/01/2008', "
		"	'08:00:00', "
		"	NULL)"));
	printf("\n");


	// affected rows
	printf("AFFECTED ROWS: \n");
	assertEqualsInt(sqlrcur_affectedRows(cur),1);
	printf("\n");


	// stored procedure
	printf("STORED PROCEDURE: \n");
	sqlrcur_sendQuery(cur,"drop procedure testproc");
	assertTrue(sqlrcur_sendQuery(cur,
		"create procedure testproc("
		"	in invar int, "
		"	out outvar int) "
		"language sql "
		"begin "
		"	set outvar = invar; "
		"end"));
	sqlrcur_prepareQuery(cur,"call testproc(?,?)");
	sqlrcur_inputBindLong(cur,"1",5);
	sqlrcur_defineOutputBindInteger(cur,"2");
	assertTrue(sqlrcur_executeQuery(cur));
	assertEqualsInt(sqlrcur_getOutputBindInteger(cur,"2"),5);
	assertTrue(sqlrcur_sendQuery(cur,"drop procedure testproc"));
	printf("\n");


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
	assertEqualsInt(sqlrcur_colCount(cur),11);
	printf("\n");


	// column names
	printf("COLUMN NAMES: \n");
	assertEqualsString(sqlrcur_getColumnName(cur,0),"TESTSMALLINT");
	assertEqualsString(sqlrcur_getColumnName(cur,1),"TESTINT");
	assertEqualsString(sqlrcur_getColumnName(cur,2),"TESTBIGINT");
	assertEqualsString(sqlrcur_getColumnName(cur,3),"TESTDECIMAL");
	assertEqualsString(sqlrcur_getColumnName(cur,4),"TESTREAL");
	assertEqualsString(sqlrcur_getColumnName(cur,5),"TESTDOUBLE");
	assertEqualsString(sqlrcur_getColumnName(cur,6),"TESTCHAR");
	assertEqualsString(sqlrcur_getColumnName(cur,7),"TESTVARCHAR");
	assertEqualsString(sqlrcur_getColumnName(cur,8),"TESTDATE");
	assertEqualsString(sqlrcur_getColumnName(cur,9),"TESTTIME");
	assertEqualsString(sqlrcur_getColumnName(cur,10),"TESTTIMESTAMP");
	cols=sqlrcur_getColumnNames(cur);
	assertEqualsString(cols[0],"TESTSMALLINT");
	assertEqualsString(cols[1],"TESTINT");
	assertEqualsString(cols[2],"TESTBIGINT");
	assertEqualsString(cols[3],"TESTDECIMAL");
	assertEqualsString(cols[4],"TESTREAL");
	assertEqualsString(cols[5],"TESTDOUBLE");
	assertEqualsString(cols[6],"TESTCHAR");
	assertEqualsString(cols[7],"TESTVARCHAR");
	assertEqualsString(cols[8],"TESTDATE");
	assertEqualsString(cols[9],"TESTTIME");
	assertEqualsString(cols[10],"TESTTIMESTAMP");
	printf("\n");


	// column types
	printf("COLUMN TYPES: \n");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),"SMALLINT");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"TESTSMALLINT"),"SMALLINT");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,1),"INTEGER");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"TESTINT"),"INTEGER");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,2),"BIGINT");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"TESTBIGINT"),"BIGINT");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,3),"DECIMAL");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"TESTDECIMAL"),"DECIMAL");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,4),"REAL");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"TESTREAL"),"REAL");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,5),"DOUBLE");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"TESTDOUBLE"),"DOUBLE");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,6),"CHAR");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"TESTCHAR"),"CHAR");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,7),"VARCHAR");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"TESTVARCHAR"),"VARCHAR");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,8),"DATE");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"TESTDATE"),"DATE");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,9),"TIME");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"TESTTIME"),"TIME");
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,10),"TIMESTAMP");
	assertEqualsString(sqlrcur_getColumnTypeByName(cur,"TESTTIMESTAMP"),"TIMESTAMP");
	printf("\n");


	// column length
	printf("COLUMN LENGTH: \n");
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,0),2);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"TESTSMALLINT"),2);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,1),4);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"TESTINT"),4);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,2),8);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"TESTBIGINT"),8);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,3),12);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"TESTDECIMAL"),12);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,4),4);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"TESTREAL"),4);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,5),8);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"TESTDOUBLE"),8);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,6),40);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"TESTCHAR"),40);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,7),40);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"TESTVARCHAR"),40);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,8),6);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"TESTDATE"),6);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,9),6);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"TESTTIME"),6);
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,10),16);
	assertEqualsInt(sqlrcur_getColumnLengthByName(cur,"TESTTIMESTAMP"),16);
	printf("\n");


	// longest column
	printf("LONGEST COLUMN: \n");
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,0),1);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"TESTSMALLINT"),1);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,1),1);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"TESTINT"),1);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,2),1);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"TESTBIGINT"),1);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,3),4);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"TESTDECIMAL"),4);
	//assertEqualsInt(sqlrcur_getLongestByIndex(cur,4),3);
	//assertEqualsInt(sqlrcur_getLongestByName(cur,"TESTREAL"),3);
	//assertEqualsInt(sqlrcur_getLongestByIndex(cur,5),3);
	//assertEqualsInt(sqlrcur_getLongestByName(cur,"TESTDOUBLE"),3);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,6),40);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"TESTCHAR"),40);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,7),12);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"TESTVARCHAR"),12);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,8),10);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"TESTDATE"),10);
	assertEqualsInt(sqlrcur_getLongestByIndex(cur,9),8);
	assertEqualsInt(sqlrcur_getLongestByName(cur,"TESTTIME"),8);
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
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,3),"1.10");
	//assertEqualsString(sqlrcur_getFieldByIndex(cur,0,4),"1.1");
	//assertEqualsString(sqlrcur_getFieldByIndex(cur,0,5),"1.1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,6),"testchar1                               ");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,7),"testvarchar1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,8),"2001-01-01");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,9),"01:00:00");
	printf("\n");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,1),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,2),"8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,3),"8.80");
	//assertEqualsString(sqlrcur_getFieldByIndex(cur,7,4),"8.8");
	//assertEqualsString(sqlrcur_getFieldByIndex(cur,7,5),"8.8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,6),"testchar8                               ");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,7),"testvarchar8");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,8),"2008-01-01");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,9),"08:00:00");
	printf("\n");


	// field lengths by index
	printf("FIELD LENGTHS BY INDEX: \n");
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,0),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,1),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,2),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,3),4);
	//assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,4),3);
	//assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,5),3);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,6),40);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,7),12);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,8),10);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,0,9),8);
	printf("\n");
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,0),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,1),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,2),1);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,3),4);
	//assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,4),3);
	//assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,5),3);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,6),40);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,7),12);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,8),10);
	assertEqualsInt(sqlrcur_getFieldLengthByIndex(cur,7,9),8);
	printf("\n");


	// fields by name
	printf("FIELDS BY NAME: \n");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"TESTSMALLINT"),"1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"TESTINT"),"1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"TESTBIGINT"),"1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"TESTDECIMAL"),"1.10");
	//assertEqualsString(sqlrcur_getFieldByName(cur,0,"TESTREAL"),"1.1");
	//assertEqualsString(sqlrcur_getFieldByName(cur,0,"TESTDOUBLE"),"1.1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"TESTCHAR"),"testchar1                               ");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"TESTVARCHAR"),"testvarchar1");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"TESTDATE"),"2001-01-01");
	assertEqualsString(sqlrcur_getFieldByName(cur,0,"TESTTIME"),"01:00:00");
	printf("\n");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"TESTSMALLINT"),"8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"TESTINT"),"8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"TESTBIGINT"),"8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"TESTDECIMAL"),"8.80");
	//assertEqualsString(sqlrcur_getFieldByName(cur,7,"TESTREAL"),"8.8");
	//assertEqualsString(sqlrcur_getFieldByName(cur,7,"TESTDOUBLE"),"8.8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"TESTCHAR"),"testchar8                               ");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"TESTVARCHAR"),"testvarchar8");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"TESTDATE"),"2008-01-01");
	assertEqualsString(sqlrcur_getFieldByName(cur,7,"TESTTIME"),"08:00:00");
	printf("\n");


	// field lengths by name
	printf("FIELD LENGTHS BY NAME: \n");
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"TESTSMALLINT"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"TESTINT"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"TESTBIGINT"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"TESTDECIMAL"),4);
	//assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"TESTREAL"),3);
	//assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"TESTDOUBLE"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"TESTCHAR"),40);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"TESTVARCHAR"),12);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"TESTDATE"),10);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,0,"TESTTIME"),8);
	printf("\n");
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"TESTSMALLINT"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"TESTINT"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"TESTBIGINT"),1);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"TESTDECIMAL"),4);
	//assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"TESTREAL"),3);
	//assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"TESTDOUBLE"),3);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"TESTCHAR"),40);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"TESTVARCHAR"),12);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"TESTDATE"),10);
	assertEqualsInt(sqlrcur_getFieldLengthByName(cur,7,"TESTTIME"),8);
	printf("\n");


	// fields by array
	printf("FIELDS BY ARRAY: \n");
	fields=sqlrcur_getRow(cur,0);
	assertEqualsString(fields[0],"1");
	assertEqualsString(fields[1],"1");
	assertEqualsString(fields[2],"1");
	assertEqualsString(fields[3],"1.10");
	//assertEqualsString(fields[4],"1.1");
	//assertEqualsString(fields[5],"1.1");
	assertEqualsString(fields[6],"testchar1                               ");
	assertEqualsString(fields[7],"testvarchar1");
	assertEqualsString(fields[8],"2001-01-01");
	assertEqualsString(fields[9],"01:00:00");
	printf("\n");


	// field lengths by array
	printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=sqlrcur_getRowLengths(cur,0);
	assertEqualsInt(fieldlens[0],1);
	assertEqualsInt(fieldlens[1],1);
	assertEqualsInt(fieldlens[2],1);
	assertEqualsInt(fieldlens[3],4);
	//assertEqualsInt(fieldlens[4],3);
	//assertEqualsInt(fieldlens[5],3);
	assertEqualsInt(fieldlens[6],40);
	assertEqualsInt(fieldlens[7],12);
	assertEqualsInt(fieldlens[8],10);
	assertEqualsInt(fieldlens[9],8);
	printf("\n");


	// individual substitutions
	printf("INDIVIDUAL SUBSTITUTIONS: \n");
	sqlrcur_prepareQuery(cur,"values ($(var1),'$(var2)',$(var3))");
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
	sqlrcur_prepareQuery(cur,"values ('$(var1)','$(var2)','$(var3)')");
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
	sqlrcur_prepareQuery(cur,"values ($(var1),$(var2),$(var3))");
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
	sqlrcur_prepareQuery(cur,"values ($(var1),$(var2),$(var3))");
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
	printf("NULLS as Nulls: \n");
	sqlrcur_sendQuery(cur,"drop table testtable1");
	assertTrue(sqlrcur_sendQuery(cur,
		"create table testtable1 ("
		"	col1 char(1), "
		"	col2 char(1), "
		"	col3 char (1))"));
	assertTrue(sqlrcur_sendQuery(cur,
		"insert into "
		"	testtable1 "
		"values ("
		"	'1', "
		"	NULL, "
		"	NULL)"));
	sqlrcur_getNullsAsNulls(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable1"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),NULL);
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertTrue(sqlrcur_sendQuery(cur,"select * from testtable1"));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,1),"");
	assertEqualsString(sqlrcur_getFieldByIndex(cur,0,2),"");
	assertTrue(sqlrcur_sendQuery(cur,"drop table testtable1"));
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
		"	testsmallint "));
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
		"	testsmallint "));
	assertEqualsString(sqlrcur_getColumnName(cur,0),"TESTSMALLINT");
	assertEqualsInt(sqlrcur_getColumnLengthByIndex(cur,0),2);
	assertEqualsString(sqlrcur_getColumnTypeByIndex(cur,0),"SMALLINT");
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
		"	testsmallint "));
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
		"	testsmallint "));
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
		"	testsmallint "));
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
		"	testsmallint "));
	filename=strdup(sqlrcur_getCacheFileName(cur));
	assertEqualsString(filename,"cachefile1");
	sqlrcur_cacheOff(cur);
	assertTrue(sqlrcur_openCachedResultSet(cur,filename));
	assertEqualsString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	free(filename);
	printf("\n");


	// column count for cached result set
	printf("COLUMN COUNT FOR CACHED RESULT SET: \n");
	assertEqualsInt(sqlrcur_colCount(cur),11);
	printf("\n");


	// column names for cached result set
	printf("COLUMN NAMES FOR CACHED RESULT SET: \n");
	assertEqualsString(sqlrcur_getColumnName(cur,0),"TESTSMALLINT");
	assertEqualsString(sqlrcur_getColumnName(cur,1),"TESTINT");
	assertEqualsString(sqlrcur_getColumnName(cur,2),"TESTBIGINT");
	assertEqualsString(sqlrcur_getColumnName(cur,3),"TESTDECIMAL");
	assertEqualsString(sqlrcur_getColumnName(cur,4),"TESTREAL");
	assertEqualsString(sqlrcur_getColumnName(cur,5),"TESTDOUBLE");
	assertEqualsString(sqlrcur_getColumnName(cur,6),"TESTCHAR");
	assertEqualsString(sqlrcur_getColumnName(cur,7),"TESTVARCHAR");
	assertEqualsString(sqlrcur_getColumnName(cur,8),"TESTDATE");
	assertEqualsString(sqlrcur_getColumnName(cur,9),"TESTTIME");
	assertEqualsString(sqlrcur_getColumnName(cur,10),"TESTTIMESTAMP");
	cols=sqlrcur_getColumnNames(cur);
	assertEqualsString(cols[0],"TESTSMALLINT");
	assertEqualsString(cols[1],"TESTINT");
	assertEqualsString(cols[2],"TESTBIGINT");
	assertEqualsString(cols[3],"TESTDECIMAL");
	assertEqualsString(cols[4],"TESTREAL");
	assertEqualsString(cols[5],"TESTDOUBLE");
	assertEqualsString(cols[6],"TESTCHAR");
	assertEqualsString(cols[7],"TESTVARCHAR");
	assertEqualsString(cols[8],"TESTDATE");
	assertEqualsString(cols[9],"TESTTIME");
	assertEqualsString(cols[10],"TESTTIMESTAMP");
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
		"	testsmallint "));
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
		"	testsmallint "));
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


	// finished suspended session
	printf("FINISHED SUSPENDED SESSION: \n");
	assertTrue(sqlrcur_sendQuery(cur,
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testint "));
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
	sqlrcon_commit(con);
	sqlrcur_sendQuery(cur,"drop table testtable");
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
