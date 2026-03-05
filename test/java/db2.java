// Copyright (c) David Muse
// See the file COPYING for more information.

import com.firstworks.sqlrelay.SQLRConnection;
import com.firstworks.sqlrelay.SQLRCursor;


class db2 extends sqlrtest {

	public static void	main(String[] args) {

		String[]	isolationlevels={"CS","UR","RS","RR"};
		String[]	bindvars={"1","2","3","4","5","6",
					"7","8","9","10"};
		String[]	bindvals={"4","4","4","4.4","4.4","4.4",
					"testchar4","testvarchar4",
					"01/01/2004","04:00:00"};
		String[]	cols;
		String[]	fields;
		long[]	fieldlens;
		String[]	subvars={"var1","var2","var3"};
		long[]	subvallongs={1,2,3};
		String[]	subvalstrings={"hi","hello","bye"};
		double[]	subvaldoubles={10.55,10.556,10.5556};
		int[]	precs={4,5,6};
		int[]	scales={2,3,4};
		short	port;
		String	socket;
		short	id;
		String	filename;
		String	numvar;
		String	stringvar;
		String	floatvar;
		String	dbtype;

		// instantiation
		SQLRConnection con=new SQLRConnection("sqlrelay",
						(short)9000,
						"/tmp/test.socket",
						"db2inst1","testpassword",0,1);
		SQLRCursor cur=new SQLRCursor(con);

		// get database type


		// identify
		System.out.println("IDENTIFY: ");
		assertEquals(con.identify(),"db2");
		System.out.println();


		// ping
		System.out.println("PING: ");
		assertTrue(con.ping());
		System.out.println();


		// isolation levels
		System.out.println("ISOLATION LEVELS: ");
		for (String il : isolationlevels) {
			assertTrue(con.setIsolationLevel(il));
			assertEquals(con.getIsolationLevel(),il);
			System.out.println();
		}
		// reset to the default isolation level
		assertTrue(con.setIsolationLevel(isolationlevels[0]));
		System.out.println();

		// drop existing table
		cur.sendQuery("drop table testtable");


		// create temptable
		System.out.println("CREATE TEMPTABLE: ");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	testsmallint smallint, "+
			"	testint integer, "+
			"	testbigint bigint, "+
			"	testdecimal decimal(10,2), "+
			"	testreal real, "+
			"	testdouble double, "+
			"	testchar char(40), "+
			"	testvarchar varchar(40), "+
			"	testdate date, "+
			"	testtime time, "+
			"	testtimestamp timestamp)"));
		System.out.println();


		// insert
		System.out.println("INSERT: ");
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	1, "+
			"	1, "+
			"	1, "+
			"	1.1, "+
			"	1.1, "+
			"	1.1, "+
			"	'testchar1', "+
			"	'testvarchar1', "+
			"	'01/01/2001', "+
			"	'01:00:00', "+
			"	null)"));
		System.out.println();


		// bind by position
		System.out.println("BIND BY POSITION: ");
		cur.prepareQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	?, "+
			"	?, "+
			"	?, "+
			"	?, "+
			"	?, "+
			"	?, "+
			"	?, "+
			"	?, "+
			"	?, "+
			"	?, "+
			"	null)");
		assertEquals(cur.countBindVariables(),10);
		cur.inputBind("1",2);
		cur.inputBind("2",2);
		cur.inputBind("3",2);
		cur.inputBind("4",2.2,4,2);
		cur.inputBind("5",2.2,4,2);
		cur.inputBind("6",2.2,4,2);
		cur.inputBind("7","testchar2");
		cur.inputBind("8","testvarchar2");
		cur.inputBind("9","01/01/2002");
		cur.inputBind("10","02:00:00");
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("1",3);
		cur.inputBind("2",3);
		cur.inputBind("3",3);
		cur.inputBind("4",3.3,4,2);
		cur.inputBind("5",3.3,4,2);
		cur.inputBind("6",3.3,4,2);
		cur.inputBind("7","testchar3");
		cur.inputBind("8","testvarchar3");
		cur.inputBind("9","01/01/2003");
		cur.inputBind("10","03:00:00");
		assertTrue(cur.executeQuery());
		System.out.println();


		// array of binds by position
		System.out.println("ARRAY OF BINDS BY POSITION: ");
		cur.clearBinds();
		cur.inputBinds(bindvars,bindvals);
		assertTrue(cur.executeQuery());
		System.out.println();


		// insert
		System.out.println("INSERT: ");
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	5, "+
			"	5, "+
			"	5, "+
			"	5.5, "+
			"	5.5, "+
			"	5.5, "+
			"	'testchar5', "+
			"	'testvarchar5', "+
			"	'01/01/2005', "+
			"	'05:00:00', "+
			"	null)"));
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	6, "+
			"	6, "+
			"	6, "+
			"	6.6, "+
			"	6.6, "+
			"	6.6, "+
			"	'testchar6', "+
			"	'testvarchar6', "+
			"	'01/01/2006', "+
			"	'06:00:00', "+
			"	null)"));
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	7, "+
			"	7, "+
			"	7, "+
			"	7.7, "+
			"	7.7, "+
			"	7.7, "+
			"	'testchar7', "+
			"	'testvarchar7', "+
			"	'01/01/2007', "+
			"	'07:00:00', "+
			"	null)"));
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	8, "+
			"	8, "+
			"	8, "+
			"	8.8, "+
			"	8.8, "+
			"	8.8, "+
			"	'testchar8', "+
			"	'testvarchar8', "+
			"	'01/01/2008', "+
			"	'08:00:00', "+
			"	null)"));
		System.out.println();


		// affected rows
		System.out.println("AFFECTED ROWS: ");
		assertEquals(cur.affectedRows(),1);
		System.out.println();


		// stored procedure
		System.out.println("STORED PROCEDURE: ");
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery(
			"create procedure testproc("+
			"	in invar int, "+
			"	out outvar int) "+
			"language sql "+
			"begin "+
			"	set outvar = invar; "+
			"end"));
		cur.prepareQuery("call testproc(?,?)");
		cur.inputBind("1",5);
		cur.defineOutputBindInteger("2");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("2"),5);
		assertTrue(cur.sendQuery("drop procedure testproc"));
		System.out.println();


		// select
		System.out.println("SELECT: ");
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testsmallint "));
		System.out.println();


		// column count
		System.out.println("COLUMN COUNT: ");
		assertEquals(cur.colCount(),11);
		System.out.println();


		// column names
		System.out.println("COLUMN NAMES: ");
		assertEquals(cur.getColumnName(0),"TESTSMALLINT");
		assertEquals(cur.getColumnName(1),"TESTINT");
		assertEquals(cur.getColumnName(2),"TESTBIGINT");
		assertEquals(cur.getColumnName(3),"TESTDECIMAL");
		assertEquals(cur.getColumnName(4),"TESTREAL");
		assertEquals(cur.getColumnName(5),"TESTDOUBLE");
		assertEquals(cur.getColumnName(6),"TESTCHAR");
		assertEquals(cur.getColumnName(7),"TESTVARCHAR");
		assertEquals(cur.getColumnName(8),"TESTDATE");
		assertEquals(cur.getColumnName(9),"TESTTIME");
		assertEquals(cur.getColumnName(10),"TESTTIMESTAMP");
		cols=cur.getColumnNames();
		assertEquals(cols[0],"TESTSMALLINT");
		assertEquals(cols[1],"TESTINT");
		assertEquals(cols[2],"TESTBIGINT");
		assertEquals(cols[3],"TESTDECIMAL");
		assertEquals(cols[4],"TESTREAL");
		assertEquals(cols[5],"TESTDOUBLE");
		assertEquals(cols[6],"TESTCHAR");
		assertEquals(cols[7],"TESTVARCHAR");
		assertEquals(cols[8],"TESTDATE");
		assertEquals(cols[9],"TESTTIME");
		assertEquals(cols[10],"TESTTIMESTAMP");
		System.out.println();


		// column types
		System.out.println("COLUMN TYPES: ");
		assertEquals(cur.getColumnType(0),"SMALLINT");
		assertEquals(cur.getColumnType("TESTSMALLINT"),"SMALLINT");
		assertEquals(cur.getColumnType(1),"INTEGER");
		assertEquals(cur.getColumnType("TESTINT"),"INTEGER");
		assertEquals(cur.getColumnType(2),"BIGINT");
		assertEquals(cur.getColumnType("TESTBIGINT"),"BIGINT");
		assertEquals(cur.getColumnType(3),"DECIMAL");
		assertEquals(cur.getColumnType("TESTDECIMAL"),"DECIMAL");
		assertEquals(cur.getColumnType(4),"REAL");
		assertEquals(cur.getColumnType("TESTREAL"),"REAL");
		assertEquals(cur.getColumnType(5),"DOUBLE");
		assertEquals(cur.getColumnType("TESTDOUBLE"),"DOUBLE");
		assertEquals(cur.getColumnType(6),"CHAR");
		assertEquals(cur.getColumnType("TESTCHAR"),"CHAR");
		assertEquals(cur.getColumnType(7),"VARCHAR");
		assertEquals(cur.getColumnType("TESTVARCHAR"),"VARCHAR");
		assertEquals(cur.getColumnType(8),"DATE");
		assertEquals(cur.getColumnType("TESTDATE"),"DATE");
		assertEquals(cur.getColumnType(9),"TIME");
		assertEquals(cur.getColumnType("TESTTIME"),"TIME");
		assertEquals(cur.getColumnType(10),"TIMESTAMP");
		assertEquals(cur.getColumnType("TESTTIMESTAMP"),"TIMESTAMP");
		System.out.println();


		// column length
		System.out.println("COLUMN LENGTH: ");
		assertEquals(cur.getColumnLength(0),2);
		assertEquals(cur.getColumnLength("TESTSMALLINT"),2);
		assertEquals(cur.getColumnLength(1),4);
		assertEquals(cur.getColumnLength("TESTINT"),4);
		assertEquals(cur.getColumnLength(2),8);
		assertEquals(cur.getColumnLength("TESTBIGINT"),8);
		assertEquals(cur.getColumnLength(3),12);
		assertEquals(cur.getColumnLength("TESTDECIMAL"),12);
		assertEquals(cur.getColumnLength(4),4);
		assertEquals(cur.getColumnLength("TESTREAL"),4);
		assertEquals(cur.getColumnLength(5),8);
		assertEquals(cur.getColumnLength("TESTDOUBLE"),8);
		assertEquals(cur.getColumnLength(6),40);
		assertEquals(cur.getColumnLength("TESTCHAR"),40);
		assertEquals(cur.getColumnLength(7),40);
		assertEquals(cur.getColumnLength("TESTVARCHAR"),40);
		assertEquals(cur.getColumnLength(8),6);
		assertEquals(cur.getColumnLength("TESTDATE"),6);
		assertEquals(cur.getColumnLength(9),6);
		assertEquals(cur.getColumnLength("TESTTIME"),6);
		assertEquals(cur.getColumnLength(10),16);
		assertEquals(cur.getColumnLength("TESTTIMESTAMP"),16);
		System.out.println();


		// longest column
		System.out.println("LONGEST COLUMN: ");
		assertEquals(cur.getLongest(0),1);
		assertEquals(cur.getLongest("TESTSMALLINT"),1);
		assertEquals(cur.getLongest(1),1);
		assertEquals(cur.getLongest("TESTINT"),1);
		assertEquals(cur.getLongest(2),1);
		assertEquals(cur.getLongest("TESTBIGINT"),1);
		assertEquals(cur.getLongest(3),4);
		assertEquals(cur.getLongest("TESTDECIMAL"),4);
		//assertEquals(cur.getLongest(4),3);
		//assertEquals(cur.getLongest("TESTREAL"),3);
		//assertEquals(cur.getLongest(5),3);
		//assertEquals(cur.getLongest("TESTDOUBLE"),3);
		assertEquals(cur.getLongest(6),40);
		assertEquals(cur.getLongest("TESTCHAR"),40);
		assertEquals(cur.getLongest(7),12);
		assertEquals(cur.getLongest("TESTVARCHAR"),12);
		assertEquals(cur.getLongest(8),10);
		assertEquals(cur.getLongest("TESTDATE"),10);
		assertEquals(cur.getLongest(9),8);
		assertEquals(cur.getLongest("TESTTIME"),8);
		System.out.println();


		// row count
		System.out.println("ROW COUNT: ");
		assertEquals(cur.rowCount(),8);
		System.out.println();


		// total rows
		System.out.println("TOTAL ROWS: ");
		assertEquals(cur.totalRows(),0);
		System.out.println();


		// first row index
		System.out.println("FIRST ROW INDEX: ");
		assertEquals(cur.firstRowIndex(),0);
		System.out.println();


		// end of result set
		System.out.println("END OF RESULT SET: ");
		assertTrue(cur.endOfResultSet());
		System.out.println();


		// fields by index
		System.out.println("FIELDS BY INDEX: ");
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"1");
		assertEquals(cur.getField(0,2),"1");
		assertEquals(cur.getField(0,3),"1.10");
		//assertEquals(cur.getField(0,4),"1.1");
		//assertEquals(cur.getField(0,5),"1.1");
		assertEquals(cur.getField(0,6),"testchar1                               ");
		assertEquals(cur.getField(0,7),"testvarchar1");
		assertEquals(cur.getField(0,8),"2001-01-01");
		assertEquals(cur.getField(0,9),"01:00:00");
		System.out.println();
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(7,1),"8");
		assertEquals(cur.getField(7,2),"8");
		assertEquals(cur.getField(7,3),"8.80");
		//assertEquals(cur.getField(7,4),"8.8");
		//assertEquals(cur.getField(7,5),"8.8");
		assertEquals(cur.getField(7,6),"testchar8                               ");
		assertEquals(cur.getField(7,7),"testvarchar8");
		assertEquals(cur.getField(7,8),"2008-01-01");
		assertEquals(cur.getField(7,9),"08:00:00");
		System.out.println();


		// field lengths by index
		System.out.println("FIELD LENGTHS BY INDEX: ");
		assertEquals(cur.getFieldLength(0,0),1);
		assertEquals(cur.getFieldLength(0,1),1);
		assertEquals(cur.getFieldLength(0,2),1);
		assertEquals(cur.getFieldLength(0,3),4);
		//assertEquals(cur.getFieldLength(0,4),3);
		//assertEquals(cur.getFieldLength(0,5),3);
		assertEquals(cur.getFieldLength(0,6),40);
		assertEquals(cur.getFieldLength(0,7),12);
		assertEquals(cur.getFieldLength(0,8),10);
		assertEquals(cur.getFieldLength(0,9),8);
		System.out.println();
		assertEquals(cur.getFieldLength(7,0),1);
		assertEquals(cur.getFieldLength(7,1),1);
		assertEquals(cur.getFieldLength(7,2),1);
		assertEquals(cur.getFieldLength(7,3),4);
		//assertEquals(cur.getFieldLength(7,4),3);
		//assertEquals(cur.getFieldLength(7,5),3);
		assertEquals(cur.getFieldLength(7,6),40);
		assertEquals(cur.getFieldLength(7,7),12);
		assertEquals(cur.getFieldLength(7,8),10);
		assertEquals(cur.getFieldLength(7,9),8);
		System.out.println();


		// fields by name
		System.out.println("FIELDS BY NAME: ");
		assertEquals(cur.getField(0,"TESTSMALLINT"),"1");
		assertEquals(cur.getField(0,"TESTINT"),"1");
		assertEquals(cur.getField(0,"TESTBIGINT"),"1");
		assertEquals(cur.getField(0,"TESTDECIMAL"),"1.10");
		//assertEquals(cur.getField(0,"TESTREAL"),"1.1");
		//assertEquals(cur.getField(0,"TESTDOUBLE"),"1.1");
		assertEquals(cur.getField(0,"TESTCHAR"),"testchar1                               ");
		assertEquals(cur.getField(0,"TESTVARCHAR"),"testvarchar1");
		assertEquals(cur.getField(0,"TESTDATE"),"2001-01-01");
		assertEquals(cur.getField(0,"TESTTIME"),"01:00:00");
		System.out.println();
		assertEquals(cur.getField(7,"TESTSMALLINT"),"8");
		assertEquals(cur.getField(7,"TESTINT"),"8");
		assertEquals(cur.getField(7,"TESTBIGINT"),"8");
		assertEquals(cur.getField(7,"TESTDECIMAL"),"8.80");
		//assertEquals(cur.getField(7,"TESTREAL"),"8.8");
		//assertEquals(cur.getField(7,"TESTDOUBLE"),"8.8");
		assertEquals(cur.getField(7,"TESTCHAR"),"testchar8                               ");
		assertEquals(cur.getField(7,"TESTVARCHAR"),"testvarchar8");
		assertEquals(cur.getField(7,"TESTDATE"),"2008-01-01");
		assertEquals(cur.getField(7,"TESTTIME"),"08:00:00");
		System.out.println();


		// field lengths by name
		System.out.println("FIELD LENGTHS BY NAME: ");
		assertEquals(cur.getFieldLength(0,"TESTSMALLINT"),1);
		assertEquals(cur.getFieldLength(0,"TESTINT"),1);
		assertEquals(cur.getFieldLength(0,"TESTBIGINT"),1);
		assertEquals(cur.getFieldLength(0,"TESTDECIMAL"),4);
		//assertEquals(cur.getFieldLength(0,"TESTREAL"),3);
		//assertEquals(cur.getFieldLength(0,"TESTDOUBLE"),3);
		assertEquals(cur.getFieldLength(0,"TESTCHAR"),40);
		assertEquals(cur.getFieldLength(0,"TESTVARCHAR"),12);
		assertEquals(cur.getFieldLength(0,"TESTDATE"),10);
		assertEquals(cur.getFieldLength(0,"TESTTIME"),8);
		System.out.println();
		assertEquals(cur.getFieldLength(7,"TESTSMALLINT"),1);
		assertEquals(cur.getFieldLength(7,"TESTINT"),1);
		assertEquals(cur.getFieldLength(7,"TESTBIGINT"),1);
		assertEquals(cur.getFieldLength(7,"TESTDECIMAL"),4);
		//assertEquals(cur.getFieldLength(7,"TESTREAL"),3);
		//assertEquals(cur.getFieldLength(7,"TESTDOUBLE"),3);
		assertEquals(cur.getFieldLength(7,"TESTCHAR"),40);
		assertEquals(cur.getFieldLength(7,"TESTVARCHAR"),12);
		assertEquals(cur.getFieldLength(7,"TESTDATE"),10);
		assertEquals(cur.getFieldLength(7,"TESTTIME"),8);
		System.out.println();


		// fields by array
		System.out.println("FIELDS BY ARRAY: ");
		fields=cur.getRow(0);
		assertEquals(fields[0],"1");
		assertEquals(fields[1],"1");
		assertEquals(fields[2],"1");
		assertEquals(fields[3],"1.10");
		//assertEquals(fields[4],"1.1");
		//assertEquals(fields[5],"1.1");
		assertEquals(fields[6],"testchar1                               ");
		assertEquals(fields[7],"testvarchar1");
		assertEquals(fields[8],"2001-01-01");
		assertEquals(fields[9],"01:00:00");
		System.out.println();


		// field lengths by array
		System.out.println("FIELD LENGTHS BY ARRAY: ");
		fieldlens=cur.getRowLengths(0);
		assertEquals(fieldlens[0],1);
		assertEquals(fieldlens[1],1);
		assertEquals(fieldlens[2],1);
		assertEquals(fieldlens[3],4);
		//assertEquals(fieldlens[4],3);
		//assertEquals(fieldlens[5],3);
		assertEquals(fieldlens[6],40);
		assertEquals(fieldlens[7],12);
		assertEquals(fieldlens[8],10);
		assertEquals(fieldlens[9],8);
		System.out.println();


		// individual substitutions
		System.out.println("INDIVIDUAL SUBSTITUTIONS: ");
		cur.prepareQuery("values ($(var1),'$(var2)','$(var3)')");
		cur.substitution("var1",1);
		cur.substitution("var2","hello");
		cur.substitution("var3",10.5556,6,4);
		assertTrue(cur.executeQuery());
		System.out.println();


		// fields
		System.out.println("FIELDS: ");
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"hello");
		assertEquals(cur.getField(0,2),"10.5556");
		System.out.println();


		// array substitutions
		System.out.println("ARRAY SUBSTITUTIONS: ");
		cur.prepareQuery("values ('$(var1)','$(var2)','$(var3)')");
		cur.substitutions(subvars,subvalstrings);
		assertTrue(cur.executeQuery());
		System.out.println();


		// fields
		System.out.println("FIELDS: ");
		assertEquals(cur.getField(0,0),"hi");
		assertEquals(cur.getField(0,1),"hello");
		assertEquals(cur.getField(0,2),"bye");
		System.out.println();


		// array substitutions
		System.out.println("ARRAY SUBSTITUTIONS: ");
		cur.prepareQuery("values ($(var1),$(var2),$(var3))");
		cur.substitutions(subvars,subvallongs);
		assertTrue(cur.executeQuery());
		System.out.println();


		// fields
		System.out.println("FIELDS: ");
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"2");
		assertEquals(cur.getField(0,2),"3");
		System.out.println();


		// array substitutions
		System.out.println("ARRAY SUBSTITUTIONS: ");
		cur.prepareQuery("values ($(var1),$(var2),$(var3))");
		cur.substitutions(subvars,subvaldoubles,precs,scales);
		assertTrue(cur.executeQuery());
		System.out.println();


		// fields
		System.out.println("FIELDS: ");
		assertEquals(cur.getField(0,0),"10.55");
		assertEquals(cur.getField(0,1),"10.556");
		assertEquals(cur.getField(0,2),"10.5556");
		System.out.println();

		System.out.println("nullS as Nulls: ");
		cur.sendQuery("drop table testtable1");
		cur.sendQuery(
			"create table testtable1 ("+
			"	col1 char(1), "+
			"	col2 char(1), "+
			"	col3 char(1))");
		cur.getNullsAsNulls();
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable1 "+
			"values ("+
			"	'1', "+
			"	null, "+
			"	null)"));
		assertTrue(cur.sendQuery("select * from testtable1"));
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),null);
		assertEquals(cur.getField(0,2),null);
		cur.getNullsAsEmptyStrings();
		assertTrue(cur.sendQuery("select * from testtable1"));
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"");
		assertEquals(cur.getField(0,2),"");
		cur.sendQuery("drop table testtable1");
		cur.getNullsAsNulls();
		System.out.println();


		// result set buffer size
		System.out.println("RESULT SET BUFFER SIZE: ");
		assertEquals(cur.getResultSetBufferSize(),0);
		cur.setResultSetBufferSize(2);
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testsmallint "));
		assertEquals(cur.getResultSetBufferSize(),2);
		System.out.println();
		assertEquals(cur.firstRowIndex(),0);
		assertFalse(cur.endOfResultSet());
		assertEquals(cur.rowCount(),2);
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(1,0),"2");
		assertEquals(cur.getField(2,0),"3");
		System.out.println();
		assertEquals(cur.firstRowIndex(),2);
		assertFalse(cur.endOfResultSet());
		assertEquals(cur.rowCount(),4);
		assertEquals(cur.getField(6,0),"7");
		assertEquals(cur.getField(7,0),"8");
		System.out.println();
		assertEquals(cur.firstRowIndex(),6);
		assertFalse(cur.endOfResultSet());
		assertEquals(cur.rowCount(),8);
		assertEquals(cur.getField(8,0),null);
		System.out.println();
		assertEquals(cur.firstRowIndex(),8);
		assertTrue(cur.endOfResultSet());
		assertEquals(cur.rowCount(),8);
		cur.setResultSetBufferSize(0);
		System.out.println();


		// dont get column info
		System.out.println("DONT GET COLUMN INFO: ");
		cur.dontGetColumnInfo();
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testsmallint "));
		assertEquals(cur.getColumnName(0),null);
		assertEquals(cur.getColumnLength(0),0);
		assertEquals(cur.getColumnType(0),null);
		cur.getColumnInfo();
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testsmallint "));
		assertEquals(cur.getColumnName(0),"TESTSMALLINT");
		assertEquals(cur.getColumnLength(0),2);
		assertEquals(cur.getColumnType(0),"SMALLINT");
		System.out.println();


		// suspended session
		System.out.println("SUSPENDED SESSION: ");
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testsmallint "));
		cur.suspendResultSet();
		assertTrue(con.suspendSession());
		port=con.getConnectionPort();
		socket=con.getConnectionSocket();
		assertTrue(con.resumeSession(port,socket));
		System.out.println();
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(1,0),"2");
		assertEquals(cur.getField(2,0),"3");
		assertEquals(cur.getField(3,0),"4");
		assertEquals(cur.getField(4,0),"5");
		assertEquals(cur.getField(5,0),"6");
		assertEquals(cur.getField(6,0),"7");
		assertEquals(cur.getField(7,0),"8");
		System.out.println();
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testsmallint "));
		cur.suspendResultSet();
		assertTrue(con.suspendSession());
		port=con.getConnectionPort();
		socket=con.getConnectionSocket();
		assertTrue(con.resumeSession(port,socket));
		System.out.println();
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(1,0),"2");
		assertEquals(cur.getField(2,0),"3");
		assertEquals(cur.getField(3,0),"4");
		assertEquals(cur.getField(4,0),"5");
		assertEquals(cur.getField(5,0),"6");
		assertEquals(cur.getField(6,0),"7");
		assertEquals(cur.getField(7,0),"8");
		System.out.println();
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testsmallint "));
		cur.suspendResultSet();
		assertTrue(con.suspendSession());
		port=con.getConnectionPort();
		socket=con.getConnectionSocket();
		assertTrue(con.resumeSession(port,socket));
		System.out.println();
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(1,0),"2");
		assertEquals(cur.getField(2,0),"3");
		assertEquals(cur.getField(3,0),"4");
		assertEquals(cur.getField(4,0),"5");
		assertEquals(cur.getField(5,0),"6");
		assertEquals(cur.getField(6,0),"7");
		assertEquals(cur.getField(7,0),"8");
		System.out.println();


		// suspended result set
		System.out.println("SUSPENDED RESULT SET: ");
		cur.setResultSetBufferSize(2);
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testsmallint "));
		assertEquals(cur.getField(2,0),"3");
		id=cur.getResultSetId();
		cur.suspendResultSet();
		assertTrue(con.suspendSession());
		port=con.getConnectionPort();
		socket=con.getConnectionSocket();
		assertTrue(con.resumeSession(port,socket));
		assertTrue(cur.resumeResultSet(id));
		System.out.println();
		assertEquals(cur.firstRowIndex(),4);
		assertFalse(cur.endOfResultSet());
		assertEquals(cur.rowCount(),6);
		assertEquals(cur.getField(7,0),"8");
		System.out.println();
		assertEquals(cur.firstRowIndex(),6);
		assertFalse(cur.endOfResultSet());
		assertEquals(cur.rowCount(),8);
		assertEquals(cur.getField(8,0),null);
		System.out.println();
		assertEquals(cur.firstRowIndex(),8);
		assertTrue(cur.endOfResultSet());
		assertEquals(cur.rowCount(),8);
		cur.setResultSetBufferSize(0);
		System.out.println();


		// cached result set
		System.out.println("CACHED RESULT SET: ");
		cur.cacheToFile("cachefile1");
		cur.setCacheTtl(200);
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testsmallint "));
		filename=cur.getCacheFileName();
		assertEquals(filename,"cachefile1");
		cur.cacheOff();
		assertTrue(cur.openCachedResultSet(filename));
		assertEquals(cur.getField(7,0),"8");
		System.out.println();


		// column count for cached result set
		System.out.println("COLUMN COUNT FOR CACHED RESULT SET: ");
		assertEquals(cur.colCount(),11);
		System.out.println();


		// column names for cached result set
		System.out.println("COLUMN NAMES FOR CACHED RESULT SET: ");
		assertEquals(cur.getColumnName(0),"TESTSMALLINT");
		assertEquals(cur.getColumnName(1),"TESTINT");
		assertEquals(cur.getColumnName(2),"TESTBIGINT");
		assertEquals(cur.getColumnName(3),"TESTDECIMAL");
		assertEquals(cur.getColumnName(4),"TESTREAL");
		assertEquals(cur.getColumnName(5),"TESTDOUBLE");
		assertEquals(cur.getColumnName(6),"TESTCHAR");
		assertEquals(cur.getColumnName(7),"TESTVARCHAR");
		assertEquals(cur.getColumnName(8),"TESTDATE");
		assertEquals(cur.getColumnName(9),"TESTTIME");
		assertEquals(cur.getColumnName(10),"TESTTIMESTAMP");
		cols=cur.getColumnNames();
		assertEquals(cols[0],"TESTSMALLINT");
		assertEquals(cols[1],"TESTINT");
		assertEquals(cols[2],"TESTBIGINT");
		assertEquals(cols[3],"TESTDECIMAL");
		assertEquals(cols[4],"TESTREAL");
		assertEquals(cols[5],"TESTDOUBLE");
		assertEquals(cols[6],"TESTCHAR");
		assertEquals(cols[7],"TESTVARCHAR");
		assertEquals(cols[8],"TESTDATE");
		assertEquals(cols[9],"TESTTIME");
		assertEquals(cols[10],"TESTTIMESTAMP");
		System.out.println();


		// cached result set with result set buffer size
		System.out.println("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
		cur.setResultSetBufferSize(2);
		cur.cacheToFile("cachefile1");
		cur.setCacheTtl(200);
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testsmallint "));
		filename=cur.getCacheFileName();
		assertEquals(filename,"cachefile1");
		cur.cacheOff();
		assertTrue(cur.openCachedResultSet(filename));
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(8,0),null);
		cur.setResultSetBufferSize(0);
		System.out.println();


		// from one cache file to another
		System.out.println("FROM ONE CACHE FILE TO ANOTHER: ");
		cur.cacheToFile("cachefile2");
		assertTrue(cur.openCachedResultSet("cachefile1"));
		cur.cacheOff();
		assertTrue(cur.openCachedResultSet("cachefile2"));
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(8,0),null);
		System.out.println();


		// from one cache file to another with result set buffer size
		System.out.println("FROM ONE CACHE FILE TO ANOTHER "+
					"WITH RESULT SET BUFFER SIZE: ");
		cur.setResultSetBufferSize(2);
		cur.cacheToFile("cachefile2");
		assertTrue(cur.openCachedResultSet("cachefile1"));
		cur.cacheOff();
		assertTrue(cur.openCachedResultSet("cachefile2"));
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(8,0),null);
		cur.setResultSetBufferSize(0);
		System.out.println();


		// cached result set with suspend and result set buffer size
		System.out.println("CACHED RESULT SET WITH SUSPEND "+
					"AND RESULT SET BUFFER SIZE: ");
		cur.setResultSetBufferSize(2);
		cur.cacheToFile("cachefile1");
		cur.setCacheTtl(200);
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testsmallint "));
		assertEquals(cur.getField(2,0),"3");
		filename=cur.getCacheFileName();
		assertEquals(filename,"cachefile1");
		id=cur.getResultSetId();
		cur.suspendResultSet();
		assertTrue(con.suspendSession());
		port=con.getConnectionPort();
		socket=con.getConnectionSocket();
		System.out.println();
		assertTrue(con.resumeSession(port,socket));
		assertTrue(cur.resumeCachedResultSet(id,filename));
		System.out.println();
		assertEquals(cur.firstRowIndex(),4);
		assertFalse(cur.endOfResultSet());
		assertEquals(cur.rowCount(),6);
		assertEquals(cur.getField(7,0),"8");
		System.out.println();
		assertEquals(cur.firstRowIndex(),6);
		assertFalse(cur.endOfResultSet());
		assertEquals(cur.rowCount(),8);
		assertEquals(cur.getField(8,0),null);
		System.out.println();
		assertEquals(cur.firstRowIndex(),8);
		assertTrue(cur.endOfResultSet());
		assertEquals(cur.rowCount(),8);
		cur.cacheOff();
		System.out.println();
		assertTrue(cur.openCachedResultSet(filename));
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(8,0),null);
		cur.setResultSetBufferSize(0);
		System.out.println();


		// finished suspended session
		System.out.println("FINISHED SUSPENDED SESSION: ");
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testint "));
		assertEquals(cur.getField(4,0),"5");
		assertEquals(cur.getField(5,0),"6");
		assertEquals(cur.getField(6,0),"7");
		assertEquals(cur.getField(7,0),"8");
		id=cur.getResultSetId();
		cur.suspendResultSet();
		assertTrue(con.suspendSession());
		port=con.getConnectionPort();
		socket=con.getConnectionSocket();
		assertTrue(con.resumeSession(port,socket));
		assertTrue(cur.resumeResultSet(id));
		assertEquals(cur.getField(4,0),null);
		assertEquals(cur.getField(5,0),null);
		assertEquals(cur.getField(6,0),null);
		assertEquals(cur.getField(7,0),null);
		System.out.println();

		// drop existing table
		con.commit();
		cur.sendQuery("drop table testtable");
		System.out.println();


		// invalid queries
		System.out.println("INVALID QUERIES: ");
		assertFalse(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testsmallint "));
		assertFalse(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testsmallint "));
		assertFalse(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testsmallint "));
		assertFalse(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testsmallint "));
		System.out.println();
		assertFalse(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	1, "+
			"	2, "+
			"	3, "+
			"	4)"));
		assertFalse(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	1, "+
			"	2, "+
			"	3, "+
			"	4)"));
		assertFalse(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	1, "+
			"	2, "+
			"	3, "+
			"	4)"));
		assertFalse(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	1, "+
			"	2, "+
			"	3, "+
			"	4)"));
		System.out.println();
		assertFalse(cur.sendQuery("create table testtable"));
		assertFalse(cur.sendQuery("create table testtable"));
		assertFalse(cur.sendQuery("create table testtable"));
		assertFalse(cur.sendQuery("create table testtable"));
		System.out.println();

		reportTestStatus();

		System.exit(status);
	}
}
