// Copyright (c) David Muse
// See the file COPYING for more information.

import com.firstworks.sqlrelay.SQLRConnection;
import com.firstworks.sqlrelay.SQLRCursor;


class db2 extends sqlrtest {

	public static void	main(String[] args) {

		String[]	isolationlevels={"CS","UR","RS","RR"};
		String[]	bindvars={"1","2","3","4","5","6",
					"7","8","9","10","11","12"};
		String[]	bindvals={"7","7","7","7.7","7.7","7.7",
					"testchar7","testvarchar7","01/01/2007",
					"07:00:00","testclob7",null};
		String[]	cols;
		String[]	fields;
		long[]		fieldlens;
		String[]	subvars={"var1","var2","var3"};
		long[]		subvallongs={1,2,3};
		String[]	subvalstrings={"hi","hello","bye"};
		double[]	subvaldoubles={10.55,10.556,10.5556};
		int[]		precs={4,5,6};
		int[]		scales={2,3,4};
		long		numvar;
		String		stringvar;
		double		floatvar;
		short		year;
		short		month;
		short		day;
		short		hour;
		short		minute;
		short		second;
		int		microsecond;
		String		tz;
		boolean		isnegative;
		String		nullvar;
		short		port;
		String		socket;
		short		id;
		String		filename;
		String		clobvar;
		long		clobvarlength;
		byte[]		blobvar;
		long		blobvarlength;
		long		counter=0;

		int		LARGE_BUFFER_LENGTH=20*1024;


		// instantiation
		SQLRConnection con=new SQLRConnection("sqlrelay",(short)9000,
						"/tmp/test.socket","db2inst1",
						"testpassword",0,1);
		SQLRCursor cur=new SQLRCursor(con);


		// identify
		System.out.println("IDENTIFY: ");
		assertEquals(con.identify(),"db2");
		System.out.println();


		// ping
		System.out.println("PING: ");
		assertTrue(con.ping());
		System.out.println();


		// transaction state
		System.out.println("TRANSACTION STATE: ");
		assertEquals(con.getDefaultTransactionModel(),"implicit");
		assertEquals(con.getTransactionModel(),"implicit");
		assertTrue(con.getInTransaction());
		assertFalse(con.getAutoCommit());
		System.out.println();


		// bind format
		System.out.println("BIND FORMAT: ");
		assertEquals(con.bindFormat(),"?");
		System.out.println();


		// nextval format
		System.out.println("NEXTVAL FORMAT: ");
		assertEquals(con.nextvalFormat(),"(nextval for %s)");
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


		// create testtable
		System.out.println("CREATE TESTTABLE: ");
		cur.sendQuery("drop table testtable");
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
			"	testtimestamp timestamp, "+
			"	testclob clob, "+
			"	testblob blob)"));
		assertTrue(con.commit());
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
			"	NULL, "+
			"	'testclob1', "+
			"	blob('testblob1'))"));
		System.out.println();


		// affected rows
		System.out.println("AFFECTED ROWS: ");
		assertEquals(cur.affectedRows(),1);
		System.out.println();


		// input bind by position
		System.out.println("INPUT BIND BY POSITION: ");
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
			"	NULL, "+
			"	?, "+
			"	?)");
		assertEquals(cur.countBindVariables(),12);
		cur.inputBind("1",2);
		cur.inputBind("2",2);
		cur.inputBind("3",2);
		cur.inputBind("4",2.2,4,2);
		cur.inputBind("5",2.2,4,2);
		cur.inputBind("6",2.2,4,2);
		cur.inputBind("7","testchar2");
		cur.inputBind("8","testvarchar2");
		cur.inputBind("9",
			(short)2002,(short)1,(short)1,(short)-1,(short)-1,
			(short)-1,(short)-1,null,false);
		cur.inputBind("10",
			(short)-1,(short)-1,(short)-1,(short)2,(short)0,
			(short)0,(short)0,null,false);
		cur.inputBindClob("11","testclob2",9);
		cur.inputBindBlob("12",(new String("testblob2")).getBytes(),9);
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
		cur.inputBind("9",
			(short)2003,(short)1,(short)1,(short)-1,(short)-1,
			(short)-1,(short)-1,null,false);
		cur.inputBind("10",
			(short)-1,(short)-1,(short)-1,(short)3,(short)0,
			(short)0,(short)0,null,false);
		cur.inputBindClob("11","testclob3",9);
		cur.inputBindBlob("12",(new String("testblob3")).getBytes(),9);
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("1",4);
		cur.inputBind("2",4);
		cur.inputBind("3",4);
		cur.inputBind("4",4.4,4,2);
		cur.inputBind("5",4.4,4,2);
		cur.inputBind("6",4.4,4,2);
		cur.inputBind("7","testchar4");
		cur.inputBind("8","testvarchar4");
		cur.inputBind("9",
			(short)2004,(short)1,(short)1,(short)-1,(short)-1,
			(short)-1,(short)-1,null,false);
		cur.inputBind("10",
			(short)-1,(short)-1,(short)-1,(short)4,(short)0,
			(short)0,(short)0,null,false);
		cur.inputBindClob("11","testclob4",9);
		cur.inputBindBlob("12",(new String("testblob4")).getBytes(),9);
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("1",5);
		cur.inputBind("2",5);
		cur.inputBind("3",5);
		cur.inputBind("4",5.5,4,2);
		cur.inputBind("5",5.5,4,2);
		cur.inputBind("6",5.5,4,2);
		cur.inputBind("7","testchar5");
		cur.inputBind("8","testvarchar5");
		cur.inputBind("9",
			(short)2005,(short)1,(short)1,(short)-1,(short)-1,
			(short)-1,(short)-1,null,false);
		cur.inputBind("10",
			(short)-1,(short)-1,(short)-1,(short)5,(short)0,
			(short)0,(short)0,null,false);
		cur.inputBindClob("11","testclob5",9);
		cur.inputBindBlob("12",(new String("testblob5")).getBytes(),9);
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("1",6);
		cur.inputBind("2",6);
		cur.inputBind("3",6);
		cur.inputBind("4",6.6,4,2);
		cur.inputBind("5",6.6,4,2);
		cur.inputBind("6",6.6,4,2);
		cur.inputBind("7","testchar6");
		cur.inputBind("8","testvarchar6");
		cur.inputBind("9",
			(short)2006,(short)1,(short)1,(short)-1,(short)-1,
			(short)-1,(short)-1,null,false);
		cur.inputBind("10",
			(short)-1,(short)-1,(short)-1,(short)6,(short)0,
			(short)0,(short)0,null,false);
		cur.inputBindClob("11","testclob6",9);
		cur.inputBindBlob("12",(new String("testblob6")).getBytes(),9);
		assertTrue(cur.executeQuery());
		System.out.println();


		// array of input binds by position
		System.out.println("ARRAY OF INPUT BINDS BY POSITION: ");
		cur.clearBinds();
		cur.inputBinds(bindvars,bindvals);
		assertTrue(cur.executeQuery());
		System.out.println();


		// input bind by position with validation
		System.out.println("INPUT BIND BY POSITION WITH VALIDATION: ");
		cur.clearBinds();
		cur.inputBind("1",8);
		cur.inputBind("2",8);
		cur.inputBind("3",8);
		cur.inputBind("4",8.8,4,2);
		cur.inputBind("5",8.8,4,2);
		cur.inputBind("6",8.8,4,2);
		cur.inputBind("7","testchar8");
		cur.inputBind("8","testvarchar8");
		cur.inputBind("9",
			(short)2008,(short)1,(short)1,(short)-1,(short)-1,
			(short)-1,(short)-1,null,false);
		cur.inputBind("10",
			(short)-1,(short)-1,(short)-1,(short)8,(short)0,
			(short)0,(short)0,null,false);
		cur.inputBindClob("11","testclob8",9);
		cur.inputBindBlob("12",(new String("testblob8")).getBytes(),9);
		cur.validateBinds();
		assertTrue(cur.executeQuery());
		System.out.println();


		// input bind by name
		// db2 doesn't support bind by name


		// array of input binds by name
		// db2 doesn't support bind by name


		// input bind by name with validation
		// db2 doesn't support bind by name


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
		assertEquals(cur.colCount(),13);
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
		//assertEquals(
		//	cur.getLongest("TESTREAL"),3);
		//assertEquals(cur.getLongest(5),3);
		//assertEquals(
		//	cur.getLongest("TESTDOUBLE"),3);
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
		//assertEquals(
		//	cur.getField(0,4),"1.1");
		//assertEquals(
		//	cur.getField(0,5),"1.1");
		assertEquals(cur.getField(0,6),"testchar1"+
					"                               ");
		assertEquals(cur.getField(0,7),"testvarchar1");
		assertEquals(cur.getField(0,8),"2001-01-01");
		assertEquals(cur.getField(0,9),"01:00:00");
		System.out.println();
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(7,1),"8");
		assertEquals(cur.getField(7,2),"8");
		assertEquals(cur.getField(7,3),"8.80");
		//assertEquals(
		//	cur.getField(7,4),"8.8");
		//assertEquals(
		//	cur.getField(7,5),"8.8");
		assertEquals(cur.getField(7,6),"testchar8"+
					"                               ");
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
		//assertEquals(
		//	cur.getFieldLength(0,4),3);
		//assertEquals(
		//	cur.getFieldLength(0,5),3);
		assertEquals(cur.getFieldLength(0,6),40);
		assertEquals(cur.getFieldLength(0,7),12);
		assertEquals(cur.getFieldLength(0,8),10);
		assertEquals(cur.getFieldLength(0,9),8);
		System.out.println();
		assertEquals(cur.getFieldLength(7,0),1);
		assertEquals(cur.getFieldLength(7,1),1);
		assertEquals(cur.getFieldLength(7,2),1);
		assertEquals(cur.getFieldLength(7,3),4);
		//assertEquals(
		//	cur.getFieldLength(7,4),3);
		//assertEquals(
		//	cur.getFieldLength(7,5),3);
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
		//assertEquals(
		//	cur.getField(0,
		//		"TESTREAL"),"1.1");
		//assertEquals(
		//	cur.getField(0,
		//		"TESTDOUBLE"),"1.1");
		assertEquals(cur.getField(0,"TESTCHAR"),"testchar1"+
					"                               ");
		assertEquals(cur.getField(0,"TESTVARCHAR"),"testvarchar1");
		assertEquals(cur.getField(0,"TESTDATE"),"2001-01-01");
		assertEquals(cur.getField(0,"TESTTIME"),"01:00:00");
		System.out.println();
		assertEquals(cur.getField(7,"TESTSMALLINT"),"8");
		assertEquals(cur.getField(7,"TESTINT"),"8");
		assertEquals(cur.getField(7,"TESTBIGINT"),"8");
		assertEquals(cur.getField(7,"TESTDECIMAL"),"8.80");
		//assertEquals(
		//	cur.getField(7,
		//		"TESTREAL"),"8.8");
		//assertEquals(
		//	cur.getField(7,
		//		"TESTDOUBLE"),"8.8");
		assertEquals(cur.getField(7,"TESTCHAR"),"testchar8"+
					"                               ");
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
		//assertEquals(
		//	cur.getFieldLength(0,
		//		"TESTREAL"),3);
		//assertEquals(
		//	cur.getFieldLength(0,
		//		"TESTDOUBLE"),3);
		assertEquals(cur.getFieldLength(0,"TESTCHAR"),40);
		assertEquals(cur.getFieldLength(0,"TESTVARCHAR"),12);
		assertEquals(cur.getFieldLength(0,"TESTDATE"),10);
		assertEquals(cur.getFieldLength(0,"TESTTIME"),8);
		System.out.println();
		assertEquals(cur.getFieldLength(7,"TESTSMALLINT"),1);
		assertEquals(cur.getFieldLength(7,"TESTINT"),1);
		assertEquals(cur.getFieldLength(7,"TESTBIGINT"),1);
		assertEquals(cur.getFieldLength(7,"TESTDECIMAL"),4);
		//assertEquals(
		//	cur.getFieldLength(7,
		//		"TESTREAL"),3);
		//assertEquals(
		//	cur.getFieldLength(7,
		//		"TESTDOUBLE"),3);
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
		assertEquals(fields[6],"testchar1"+
					"                               ");
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
		assertEquals(cur.colCount(),13);
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
		System.out.println("CACHED RESULT SET WITH RESULT SET BUFFER "+
			"SIZE: ");
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
			"	testint"));
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


		// nested selects
		System.out.println("NESTED SELECTS: ");
		cur.setResultSetBufferSize(1);
		assertTrue(cur.sendQuery("select * from testtable"));
		for (int i=0; cur.getRow(i)!=null; i++) {
			SQLRCursor secondcur=new SQLRCursor(con);
			secondcur.setResultSetBufferSize(1);
			assertTrue(secondcur.sendQuery(
				"select "+
				"	* "+
				"from "+
				"	testtable"));
			secondcur.closeResultSet();
		}
		cur.setResultSetBufferSize(0);
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// reset transaction state
		System.out.println("RESET TRANSACTION STATE: ");
		assertTrue(con.commit());
		assertEquals(con.getTransactionModel(),"implicit");
		assertFalse(con.getAutoCommit());
		System.out.println();


		// transaction behavior - implicit
		System.out.println("TRANSACTION BEHAVIOR - implicit: ");
		assertTrue(con.setTransactionModel("implicit"));
		assertEquals(con.getTransactionModel(),"implicit");
		assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
		// db2 DDL is transactional; commit so the table is visible to the
		// second connection (the commit implicitly starts a new tx)
		assertTrue(con.commit());
		SQLRConnection secondcon=new SQLRConnection("sqlrelay",
				(short)9000,"/tmp/test.socket","db2inst1",
				"testpassword",0,1);
		SQLRCursor secondcur=new SQLRCursor(secondcon);
		// session is in a transaction; insert is not visible until commit
		assertTrue(con.getInTransaction());
		assertFalse(con.getAutoCommit());
		assertTrue(cur.sendQuery("insert into testtable values (1)"));
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"0");
		// commit makes it visible, and implicitly starts a new transaction
		assertTrue(con.commit());
		assertTrue(con.getInTransaction());
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"1");
		// rollback discards, and implicitly starts a new transaction
		assertTrue(cur.sendQuery("insert into testtable values (2)"));
		assertTrue(con.rollback());
		assertTrue(con.getInTransaction());
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"1");
		// autoCommitOn takes effect immediately
		assertTrue(con.autoCommitOn());
		assertTrue(con.getAutoCommit());
		assertFalse(con.getInTransaction());
		assertTrue(cur.sendQuery("insert into testtable values (3)"));
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"2");
		// autoCommitOff takes effect immediately
		assertTrue(con.autoCommitOff());
		assertFalse(con.getAutoCommit());
		assertTrue(con.getInTransaction());
		secondcur.closeResultSet();
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// transaction behavior - explicit
		System.out.println("TRANSACTION BEHAVIOR - explicit: ");
		assertTrue(con.setTransactionModel("explicit"));
		assertEquals(con.getTransactionModel(),"explicit");
		assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
		// begin starts a new transaction; insert is not visible until commit
		assertTrue(con.begin());
		assertTrue(con.getInTransaction());
		assertTrue(cur.sendQuery("insert into testtable values (1)"));
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"0");
		// commit makes it visible; no new transaction is started
		assertTrue(con.commit());
		assertFalse(con.getInTransaction());
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"1");
		// begin, insert, rollback discards; no new transaction is started
		assertTrue(con.begin());
		assertTrue(cur.sendQuery("insert into testtable values (2)"));
		assertTrue(con.rollback());
		assertFalse(con.getInTransaction());
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"1");
		// autoCommitOn takes effect immediately
		assertTrue(con.autoCommitOn());
		assertTrue(con.getAutoCommit());
		assertTrue(cur.sendQuery("insert into testtable values (3)"));
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"2");
		// autoCommitOff takes effect immediately
		assertTrue(con.autoCommitOff());
		assertFalse(con.getAutoCommit());
		secondcur.closeResultSet();
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// transaction behavior - explicit-deferred
		System.out.println("TRANSACTION BEHAVIOR - explicit-deferred: ");
		assertTrue(con.setTransactionModel("explicit-deferred"));
		assertEquals(con.getTransactionModel(),"explicit-deferred");
		// switch to autocommit-on so the begin/commit cycles below
		// bracket explicit transactions (autocommit-off semantics are
		// exercised at the end of this block)
		assertTrue(con.autoCommitOn());
		assertTrue(con.getAutoCommit());
		assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
		// begin starts a transaction; commit makes it visible
		assertTrue(con.begin());
		assertTrue(con.getInTransaction());
		assertTrue(cur.sendQuery("insert into testtable values (1)"));
		assertTrue(con.commit());
		assertFalse(con.getInTransaction());
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"1");
		// begin, insert, rollback discards
		assertTrue(con.begin());
		assertTrue(cur.sendQuery("insert into testtable values (2)"));
		assertTrue(con.rollback());
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"1");
		// during a transaction started by begin(), autoCommitOn is a
		// no-op: the autocommit setting takes effect after the user
		// explicitly commits/rollbacks the tx (mysql-native semantic)
		assertTrue(con.begin());
		assertTrue(cur.sendQuery("insert into testtable values (3)"));
		assertTrue(con.autoCommitOn());
		assertFalse(con.getAutoCommit());
		assertTrue(con.getInTransaction());
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"1");
		// explicit commit ends the tx; autocommit-on now takes effect
		assertTrue(con.commit());
		assertTrue(con.getAutoCommit());
		assertFalse(con.getInTransaction());
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"2");
		// autocommit is on; subsequent inserts are visible immediately
		assertTrue(cur.sendQuery("insert into testtable values (4)"));
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"3");
		// autoCommitOff takes effect immediately when not in a transaction
		assertTrue(con.autoCommitOff());
		assertFalse(con.getAutoCommit());
		// autocommit-off persists across commit/rollback; each commit or
		// rollback ends the current implicit tx and a new one starts for
		// the next statement
		assertTrue(cur.sendQuery("insert into testtable values (5)"));
		assertTrue(con.commit());
		assertFalse(con.getAutoCommit());
		assertTrue(con.getInTransaction());
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"4");
		assertTrue(cur.sendQuery("insert into testtable values (6)"));
		assertTrue(con.rollback());
		assertFalse(con.getAutoCommit());
		assertTrue(con.getInTransaction());
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"4");
		// autoCommitOff during a transaction changes the variable
		// immediately but the in-flight tx continues; only after the
		// next explicit commit/rollback does the new autocommit-off
		// setting drop us into a new implicit tx (mysql-asymmetric
		// semantic)
		assertTrue(con.autoCommitOn());
		assertTrue(con.getAutoCommit());
		assertTrue(con.begin());
		assertTrue(cur.sendQuery("insert into testtable values (7)"));
		assertTrue(con.autoCommitOff());
		assertFalse(con.getAutoCommit());
		assertTrue(con.getInTransaction());
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"4");
		assertTrue(con.commit());
		assertFalse(con.getAutoCommit());
		assertTrue(con.getInTransaction());
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"5");
		secondcur.closeResultSet();
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// transaction behavior - explicit-error
		System.out.println("TRANSACTION BEHAVIOR - explicit-error: ");
		assertTrue(con.setTransactionModel("explicit-error"));
		assertEquals(con.getTransactionModel(),"explicit-error");
		assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
		// begin, insert, commit
		assertTrue(con.begin());
		assertTrue(con.getInTransaction());
		assertTrue(cur.sendQuery("insert into testtable values (1)"));
		assertTrue(con.commit());
		assertFalse(con.getInTransaction());
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"1");
		// begin, insert, rollback
		assertTrue(con.begin());
		assertTrue(cur.sendQuery("insert into testtable values (2)"));
		assertTrue(con.rollback());
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"1");
		// while in a transaction, autoCommitOn/Off throw an error
		assertTrue(con.begin());
		assertFalse(con.autoCommitOn());
		assertFalse(con.autoCommitOff());
		assertTrue(con.commit());
		// outside of a transaction, autoCommitOn takes effect immediately
		assertTrue(con.autoCommitOn());
		assertTrue(con.getAutoCommit());
		assertTrue(cur.sendQuery("insert into testtable values (3)"));
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"2");
		// autoCommitOff takes effect immediately
		assertTrue(con.autoCommitOff());
		assertFalse(con.getAutoCommit());
		secondcur.closeResultSet();
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// transaction behavior - none
		System.out.println("TRANSACTION BEHAVIOR - none: ");
		assertTrue(con.setTransactionModel("none"));
		assertEquals(con.getTransactionModel(),"none");
		assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
		// no transactions; everything is visible immediately
		assertTrue(con.getAutoCommit());
		assertFalse(con.getInTransaction());
		assertTrue(cur.sendQuery("insert into testtable values (1)"));
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"1");
		// commit and rollback are no-ops
		assertTrue(con.commit());
		assertTrue(cur.sendQuery("insert into testtable values (2)"));
		assertTrue(con.rollback());
		assertTrue(secondcur.sendQuery("select count(*) from testtable"));
		assertEquals(secondcur.getField(0,0),"2");
		// autocommit is always on; autoCommitOff is an error
		assertFalse(con.autoCommitOff());
		assertTrue(con.getAutoCommit());
		assertTrue(con.autoCommitOn());
		assertTrue(con.getAutoCommit());
		secondcur.closeResultSet();
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// reset transaction behavior
		System.out.println("RESET TRANSACTION BEHAVIOR: ");
		assertTrue(con.setTransactionModel(con.getDefaultTransactionModel()));
		assertEquals(con.getTransactionModel(),"implicit");
		assertFalse(con.getAutoCommit());
		System.out.println();


		// individual substitutions
		System.out.println("INDIVIDUAL SUBSTITUTIONS: ");
		cur.prepareQuery("values ($(var1),'$(var2)','$(var3)')");
		cur.substitution("var1",1);
		cur.substitution("var2","hello");
		cur.substitution("var3",10.5556,6,4);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"hello");
		assertEquals(cur.getField(0,2),"10.5556");
		System.out.println();


		// array substitutions
		System.out.println("ARRAY SUBSTITUTIONS: ");
		cur.prepareQuery("values ('$(var1)','$(var2)','$(var3)')");
		cur.substitutions(subvars,subvalstrings);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"hi");
		assertEquals(cur.getField(0,1),"hello");
		assertEquals(cur.getField(0,2),"bye");
		System.out.println();
		cur.prepareQuery("values ($(var1),$(var2),$(var3))");
		cur.substitutions(subvars,subvallongs);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"2");
		assertEquals(cur.getField(0,2),"3");
		System.out.println();
		cur.prepareQuery("values ($(var1),$(var2),$(var3))");
		cur.substitutions(subvars,subvaldoubles,precs,scales);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"10.55");
		assertEquals(cur.getField(0,1),"10.556");
		assertEquals(cur.getField(0,2),"10.5556");
		System.out.println();


		// nulls as nulls
		System.out.println("NULLS AS NULLS: ");
		cur.getNullsAsNulls();
		assertTrue(cur.sendQuery(
			"select "+
			"	NULL,1,NULL "+
			"from "+
			"	sysibm.sysdummy1"));
		assertEquals(cur.getField(0,0),null);
		assertEquals(cur.getField(0,1),"1");
		assertEquals(cur.getField(0,2),null);
		cur.getNullsAsEmptyStrings();
		assertTrue(cur.sendQuery(
			"select "+
			"	NULL,1,NULL "+
			"from "+
			"	sysibm.sysdummy1"));
		assertEquals(cur.getField(0,0),"");
		assertEquals(cur.getField(0,1),"1");
		assertEquals(cur.getField(0,2),"");
		System.out.println();


		// null and empty lobs
		System.out.println("NULL AND EMPTY LOBS: ");
		cur.getNullsAsNulls();
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	testclob1 clob, "+
			"	testclob2 clob, "+
			"	testblob1 blob, "+
			"	testblob2 blob)"));
		assertTrue(con.commit());
		cur.prepareQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	?, "+
			"	?, "+
			"	?, "+
			"	?)");
		cur.inputBindClob("1","",0);
		cur.inputBindClob("2",null,0);
		cur.inputBindBlob("3",(new String("")).getBytes(),0);
		cur.inputBindBlob("4",null,0);
		assertTrue(cur.executeQuery());
		cur.sendQuery("select * from testtable");
		assertEquals(cur.getField(0,0),"");
		assertEquals(cur.getField(0,1),null);
		assertEquals(cur.getField(0,2),"");
		assertEquals(cur.getField(0,3),null);
		cur.getNullsAsEmptyStrings();
		assertTrue(cur.sendQuery("drop table testtable"));
		assertTrue(con.commit());
		System.out.println();


		// long lobs
		System.out.println("LONG LOBS: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	testclob clob, "+
			"	testblob blob)"));
		assertTrue(con.commit());
		cur.prepareQuery("insert into testtable values (?,?)");
		StringBuilder	largebuffer=new StringBuilder();
		for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
			largebuffer.append('C');
		}
		cur.inputBindClob("1",largebuffer.toString(),
			LARGE_BUFFER_LENGTH);
		cur.inputBindBlob("2",(largebuffer.toString()).getBytes(),
			LARGE_BUFFER_LENGTH);
		assertTrue(cur.executeQuery());
		cur.sendQuery("select * from testtable");
		assertEquals(cur.getFieldLength(0,"TESTCLOB"),
			LARGE_BUFFER_LENGTH);
		assertEquals(cur.getField(0,"TESTCLOB"),largebuffer.toString());
		assertEquals(cur.getFieldLength(0,"TESTBLOB"),
			LARGE_BUFFER_LENGTH);
		assertEquals(cur.getField(0,"TESTBLOB"),largebuffer.toString(),
			LARGE_BUFFER_LENGTH);
		assertTrue(cur.sendQuery("drop table testtable"));
		assertTrue(con.commit());
		System.out.println();


		// output bind by position
		System.out.println("OUTPUT BIND BY POSITION: ");
		cur.sendQuery("drop procedure testproc");
		cur.getNullsAsNulls();
		assertTrue(cur.sendQuery(
			"create procedure testproc("+
			"	out out1 int, "+
			"	out out2 varchar(20), "+
			"	out out3 double, "+
			"	out out4 date, "+
			"	out out5 varchar(20)) "+
			"language sql begin "+
			"	set out1 = 1; "+
			"	set out2 = 'hello'; "+
			"	set out3 = 2.5; "+
			"	set out4 = '2001-02-03'; "+
			"	set out5 = null; "+
			"end"));
		assertTrue(con.commit());
		cur.prepareQuery("call testproc(?,?,?,?,?)");
		assertEquals(cur.countBindVariables(),5);
		cur.defineOutputBindInteger("1");
		cur.defineOutputBindString("2",20);
		cur.defineOutputBindDouble("3");
		cur.defineOutputBindDate("4");
		cur.defineOutputBindString("5",20);
		assertTrue(cur.executeQuery());
		numvar=cur.getOutputBindInteger("1");
		stringvar=cur.getOutputBindString("2");
		floatvar=cur.getOutputBindDouble("3");
		year=cur.getOutputBindDateYear("4");
		month=cur.getOutputBindDateMonth("4");
		day=cur.getOutputBindDateDay("4");
		hour=cur.getOutputBindDateHour("4");
		minute=cur.getOutputBindDateMinute("4");
		second=cur.getOutputBindDateSecond("4");
		microsecond=cur.getOutputBindDateMicrosecond("4");
		tz=cur.getOutputBindDateTz("4");
		isnegative=cur.getOutputBindDateIsNegative("4");
		nullvar=cur.getOutputBindString("5");
		assertEquals(numvar,1);
		assertEquals(stringvar,"hello");
		assertEquals(floatvar,2.5);
		assertEquals(year,2001);
		assertEquals(month,2);
		assertEquals(day,3);
		assertEquals(hour,0);
		assertEquals(minute,0);
		assertEquals(second,0);
		assertEquals(microsecond,0);
		assertEquals(tz,"");
		assertFalse(isnegative);
		assertEquals(nullvar,null);
		cur.getNullsAsEmptyStrings();
		assertTrue(cur.sendQuery("drop procedure testproc"));
		assertTrue(con.commit());
		System.out.println();


		// output bind by name
		// db2 doesn't support bind by name


		// output bind by name with validation
		// db2 doesn't support bind by name


		// lob output bind
		System.out.println("LOB OUTPUT BIND: ");
		cur.sendQuery("drop table testtable");
		cur.sendQuery(
			"create table testtable ("+
			"	testclob clob, "+
			"	testblob blob)");
		assertTrue(con.commit());
		cur.prepareQuery("insert into testtable values ('hello',?)");
		cur.inputBindBlob("1",(new String("hello")).getBytes(),5);
		assertTrue(cur.executeQuery());
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery(
			"create procedure testproc("+
			"	out out1 clob, "+
			"	out out2 blob) "+
			"language sql begin "+
			"	select testclob "+
			"		into out1 "+
			"		from testtable; "+
			"	select testblob "+
			"		into out2 "+
			"		from testtable; "+
			"end"));
		assertTrue(con.commit());
		cur.prepareQuery("call testproc(?,?)");
		cur.defineOutputBindClob("1");
		cur.defineOutputBindBlob("2");
		assertTrue(cur.executeQuery());
		clobvar=cur.getOutputBindClob("1");
		clobvarlength=cur.getOutputBindLength("1");
		blobvar=cur.getOutputBindBlob("2");
		blobvarlength=cur.getOutputBindLength("2");
		assertEquals(clobvar,"hello",5);
		assertEquals(clobvarlength,5);
		assertEquals(blobvar,"hello",5);
		assertEquals(blobvarlength,5);
		assertTrue(cur.sendQuery("drop procedure testproc"));
		assertTrue(cur.sendQuery("drop table testtable"));
		assertTrue(con.commit());
		System.out.println();


		// long output bind
		System.out.println("LONG OUTPUT BIND: ");
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery(
			"create procedure testproc("+
			"	in in1 clob, "+
			"	out out1 clob) "+
			"language sql begin "+
			"	set out1 = in1; "+
			"end"));
		assertTrue(con.commit());
		StringBuilder	largebuffer2=new StringBuilder();
		for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
			largebuffer2.append('C');
		}
		cur.prepareQuery("call testproc(?,?)");
		cur.inputBindClob("1",largebuffer2.toString(),
			LARGE_BUFFER_LENGTH);
		cur.defineOutputBindClob("2");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindLength("2"),LARGE_BUFFER_LENGTH);
		assertEquals(cur.getOutputBindClob("2"),
			largebuffer2.toString());
		assertTrue(cur.sendQuery("drop procedure testproc"));
		assertTrue(con.commit());
		System.out.println();


		// negative input bind
		System.out.println("NEGATIVE INPUT BIND: ");
		cur.sendQuery("drop table testtable");
		cur.sendQuery("create table testtable (testval integer)");
		assertTrue(con.commit());
		cur.prepareQuery("insert into testtable values (?)");
		cur.inputBind("1",-1);
		assertTrue(cur.executeQuery());
		cur.sendQuery("select testval from testtable");
		assertEquals(cur.getField(0,"TESTVAL"),"-1");
		assertTrue(cur.sendQuery("drop table testtable"));
		assertTrue(con.commit());
		System.out.println();


		// bind validation
		// db2 doesn't support bind by name


		// rebinding
		System.out.println("REBINDING: ");
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery(
			"create procedure testproc("+
			"	in in1 int, "+
			"	out out1 int) "+
			"language sql begin "+
			"	set out1 = in1; "+
			"end"));
		assertTrue(con.commit());
		cur.prepareQuery("call testproc(?,?)");
		cur.inputBind("1",1);
		cur.defineOutputBindInteger("2");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("2"),1);
		cur.inputBind("1",2);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("2"),2);
		cur.inputBind("1",3);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("2"),3);
		assertTrue(cur.sendQuery("drop procedure testproc"));
		assertTrue(con.commit());
		System.out.println();


		// reexecute
		System.out.println("REEXECUTE: ");
		cur.prepareQuery("select 1 from sysibm.sysdummy1");
		assertTrue(cur.executeQuery());
		assertEquals(cur.rowCount(),1);
		assertEquals(cur.getField(0,0),"1");
		System.out.println();
		assertTrue(cur.executeQuery());
		assertEquals(cur.rowCount(),1);
		assertEquals(cur.getField(0,0),"1");
		System.out.println();
		cur.prepareQuery(
			"select "+
			"	cast(? as integer) "+
			"from "+
			"	sysibm.sysdummy1");
		cur.inputBind("1",1);
		assertTrue(cur.executeQuery());
		assertEquals(cur.rowCount(),1);
		assertEquals(cur.getField(0,0),"1");
		System.out.println();
		assertTrue(cur.executeQuery());
		assertEquals(cur.rowCount(),1);
		assertEquals(cur.getField(0,0),"1");
		System.out.println();
		cur.inputBind("1",2);
		assertTrue(cur.executeQuery());
		assertEquals(cur.rowCount(),1);
		assertEquals(cur.getField(0,0),"2");
		System.out.println();


		// stored procedure returning no value
		System.out.println("STORED PROCEDURE RETURNING NO VALUE: ");
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery(
			"create procedure testproc("+
			"	in in1 int, "+
			"	in in2 double, "+
			"	in in3 varchar(20)) "+
			"language sql begin "+
			"	return; "+
			"end"));
		assertTrue(con.commit());
		cur.prepareQuery("call testproc(?,?,?)");
		cur.inputBind("1",1);
		cur.inputBind("2",1.1,2,1);
		cur.inputBind("3","hello");
		assertTrue(cur.executeQuery());
		assertTrue(cur.sendQuery("drop procedure testproc"));
		assertTrue(con.commit());
		System.out.println();


		// stored procedure returning single value
		System.out.println("STORED PROCEDURE RETURNING SINGLE VALUE: ");
		cur.sendQuery("drop function testfunc");
		assertTrue(cur.sendQuery(
			"create function testfunc("+
			"	in1 int, "+
			"	in2 double, "+
			"	in3 varchar(20)) "+
			"returns int language sql begin "+
			"	return in1; "+
			"end"));
		assertTrue(con.commit());
		cur.prepareQuery(
			"select "+
			"	testfunc(?,?,?) "+
			"from "+
			"	sysibm.sysdummy1");
		cur.inputBind("1",1);
		cur.inputBind("2",1.1,2,1);
		cur.inputBind("3","hello");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"1");
		assertTrue(cur.sendQuery("drop function testfunc"));
		assertTrue(con.commit());
		System.out.println();


		// stored procedure returning multiple values
		System.out.println("STORED PROCEDURE RETURNING MULTIPLE "+
			"VALUES: ");
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery(
			"create procedure testproc("+
			"	in in1 int, "+
			"	in in2 double, "+
			"	in in3 varchar(20), "+
			"	in in4 clob, "+
			"	in in5 blob, "+
			"	out out1 int, "+
			"	out out2 double, "+
			"	out out3 varchar(20), "+
			"	out out4 clob, "+
			"	out out5 blob) "+
			"language sql begin "+
			"	set out1 = in1; "+
			"	set out2 = in2; "+
			"	set out3 = in3; "+
			"	set out4 = in4; "+
			"	set out5 = in5; "+
			"end"));
		assertTrue(con.commit());
		cur.prepareQuery("call testproc(?,?,?,?,?,?,?,?,?,?)");
		cur.inputBind("1",1);
		cur.inputBind("2",1.1,2,1);
		cur.inputBind("3","hello");
		cur.inputBindClob("4","clob",4);
		cur.inputBindBlob("5",(new String("blob")).getBytes(),4);
		cur.defineOutputBindInteger("6");
		cur.defineOutputBindDouble("7");
		cur.defineOutputBindString("8",20);
		cur.defineOutputBindClob("9");
		cur.defineOutputBindBlob("10");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("6"),1);
		assertEquals(cur.getOutputBindDouble("7"),1.1);
		assertEquals(cur.getOutputBindString("8"),"hello");
		assertEquals(cur.getOutputBindClob("9"),"clob");
		assertEquals(cur.getOutputBindBlob("10"),"blob",4);
		assertTrue(cur.sendQuery("drop procedure testproc"));
		assertTrue(con.commit());
		System.out.println();


		// stored procedure returning result set
		System.out.println("STORED PROCEDURE RETURNING RESULT SET: ");
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery("create procedure testproc() "+
			"result set 1 language sql begin "+
			"	declare c1 cursor "+
			"		with return for "+
			"		select 1 from "+
			"		sysibm.sysdummy1 "+
			"		union "+
			"		select 2 from "+
			"		sysibm.sysdummy1 "+
			"		union "+
			"		select 3 from "+
			"		sysibm.sysdummy1 "+
			"		union "+
			"		select 4 from "+
			"		sysibm.sysdummy1 "+
			"		union "+
			"		select 5 from "+
			"		sysibm.sysdummy1 "+
			"		union "+
			"		select 6 from "+
			"		sysibm.sysdummy1 "+
			"		union "+
			"		select 7 from "+
			"		sysibm.sysdummy1 "+
			"		union "+
			"		select 8 from "+
			"		sysibm.sysdummy1; "+
			"	open c1; "+
			"end"));
		assertTrue(con.commit());
		assertTrue(cur.sendQuery("call testproc()"));
		assertEquals(cur.rowCount(),8);
		assertTrue(cur.sendQuery("drop procedure testproc"));
		assertTrue(con.commit());
		System.out.println();


		// temporary tables
		System.out.println("TEMPORARY TABLES: ");
		cur.sendQuery("drop table session.temptable");
		assertTrue(cur.sendQuery(
			"declare global temporary table session.temptable ("+
			"	col1 int "+
			") not logged"));
		assertTrue(cur.sendQuery(
			"insert into "+
			"	session.temptable "+
			"values ("+
			"	1)"));
		assertTrue(cur.sendQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	session.temptable"));
		assertEquals(cur.getField(0,0),"1");
		con.endSession();
		System.out.println();
		assertFalse(cur.sendQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	session.temptable"));
		System.out.println();


		// encoded binary data
		System.out.println("ENCODED BINARY DATA: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery("create table testtable (col1 blob)"));
		byte[]	buffer=new byte[256];
		for (int i=0; i<256; i++) {
			buffer[i]=(byte)i;
		}
		StringBuilder	query=new StringBuilder();
		query.append("insert into testtable values (blob(X'");
		for (int i=0; i<buffer.length; i++) {
			query.append(String.format("%02x",buffer[i]&0xff));
		}
		query.append("'))");
		assertTrue(cur.sendQuery(query.toString()));
		assertTrue(cur.sendQuery("select col1 from testtable"));
		assertEquals(cur.getFieldLength(0,0),buffer.length);
		byte[]	result=cur.getFieldAsByteArray(0,0);
		boolean	match=true;
		for (int i=0; i<buffer.length; i++) {
			if (result[i]!=buffer[i]) {
				match=false;
				break;
			}
		}
		assertTrue(match);
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// quotes
		System.out.println("QUOTES: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 varchar(4))"));
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	'''''')"));
		assertTrue(cur.sendQuery("select col1 from testtable"));
		assertEquals(cur.getFieldLength(0,0),2);
		assertEquals(cur.getField(0,0),"''");
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// last insert id
		System.out.println("LAST INSERT ID: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 int not null "+
			"	generated always "+
			"	as identity, "+
			"	col2 int, "+
			"	primary key(col1))"));
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable ("+
			"	col2) "+
			"values ("+
			"	1)"));
		assertEquals(con.getLastInsertId(),1);
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// database is schema
		System.out.println("DATABASE IS SCHEMA: ");
		assertTrue(con.getDatabaseIsSchema());
		System.out.println();


		// catalog list
		System.out.println("CATALOG LIST: ");
		assertTrue(cur.getCatalogList(null));
		assertEquals(cur.getColumnName(0),"Database");
		assertEquals(cur.rowCount(),0);
		System.out.println();


		// schema list
		System.out.println("SCHEMA LIST: ");
		assertTrue(cur.getSchemaList(null));
		assertEquals(cur.getColumnName(0),"Database");
		boolean	found=false;
		for (long i=0; i<cur.rowCount(); i++) {
			String val=cur.getField(i,"Database");
			if (val!=null &&val.equals("DB2INST1")) {
				found=true;
				break;
			}
		}
		assertTrue(found);
		System.out.println();


		// table type list
		System.out.println("TABLE TYPE LIST: ");
		assertTrue(cur.getTableTypeList());
		assertEquals(cur.getColumnName(0),"table_type");
		found=false;
		for (long i=0; i<cur.rowCount(); i++) {
			String val=cur.getField(i,"table_type");
			if (val!=null &&val.equals("TABLE")) {
				found=true;
				break;
			}
		}
		assertTrue(found);
		System.out.println();


		// table list
		System.out.println("TABLE LIST: ");
		cur.sendQuery("drop table testtable1");
		cur.sendQuery("drop table testtable2");
		cur.sendQuery("drop table testtable3");
		cur.sendQuery("drop table testtable4");
		assertTrue(cur.sendQuery(
			"create table testtable1 ("+
			"	col1 integer, "+
			"	col2 integer)"));
		assertTrue(cur.sendQuery(
			"create table testtable2 ("+
			"	col1 integer, "+
			"	col2 integer)"));
		assertTrue(cur.sendQuery(
			"create table testtable3 ("+
			"	col1 integer, "+
			"	col2 integer)"));
		assertTrue(cur.sendQuery(
			"create table testtable4 ("+
			"	col1 integer, "+
			"	col2 integer)"));
		assertTrue(con.commit());
		assertTrue(cur.getTableList(null));
		counter=0;
		for (long i=0; i<cur.rowCount(); i++) {
			String name=cur.getField(i,"Tables_in_xxx");
			if (name!=null &&(name.equals("TESTTABLE1") ||
				name.equals("TESTTABLE2") ||name.equals(
					"TESTTABLE3") ||name.equals(
					"TESTTABLE4"))) {
				counter++;
			}
		}
		assertEquals(counter,4);
		assertTrue(cur.sendQuery("drop table testtable1"));
		assertTrue(cur.sendQuery("drop table testtable2"));
		assertTrue(cur.sendQuery("drop table testtable3"));
		assertTrue(cur.sendQuery("drop table testtable4"));
		assertTrue(con.commit());
		System.out.println();


		// type info list
		System.out.println("TYPE INFO LIST: ");
		assertTrue(cur.getTypeInfoList("integer"));
		assertEquals(cur.getColumnName(0),"type_name");
		assertEquals(cur.getColumnName(1),"data_type");
		assertEquals(cur.getColumnName(2),"precision");
		assertEquals(cur.getColumnName(3),"literal_prefix");
		assertEquals(cur.getColumnName(4),"literal_suffix");
		assertEquals(cur.getColumnName(5),"create_params");
		assertEquals(cur.getColumnName(6),"nullable");
		assertEquals(cur.getColumnName(7),"case_sensitive");
		assertEquals(cur.getColumnName(8),"searchable");
		assertEquals(cur.getColumnName(9),"unsigned_attribute");
		assertEquals(cur.getColumnName(10),"fixed_prec_scale");
		assertEquals(cur.getColumnName(11),"auto_increment");
		assertEquals(cur.getColumnName(12),"local_type_name");
		assertEquals(cur.getColumnName(13),"minumum_scale");
		assertEquals(cur.getColumnName(14),"maxiumm_scale");
		assertEquals(cur.getColumnName(15),"sql_data_type");
		assertEquals(cur.getColumnName(16),"sql_datetime_sub");
		assertEquals(cur.getColumnName(17),"num_prec_radix");
		assertEquals(cur.getColumnName(18),"interval_precision");
		assertEquals(cur.getField(0,"type_name"),"INTEGER");
		assertEquals(cur.getField(0,"data_type"),"4");
		assertEquals(cur.getField(0,"precision"),"10");
		assertEquals(cur.getField(0,"local_type_name"),"INTEGER");
		assertTrue(cur.getTypeInfoList("char"));
		assertEquals(cur.getField(0,"type_name"),"CHAR");
		assertEquals(cur.getField(0,"data_type"),"1");
		assertEquals(cur.getField(0,"precision"),"254");
		assertEquals(cur.getField(0,"local_type_name"),"CHAR");
		assertTrue(cur.getTypeInfoList("varchar"));
		assertEquals(cur.getField(0,"type_name"),"VARCHAR");
		assertEquals(cur.getField(0,"data_type"),"12");
		assertEquals(cur.getField(0,"precision"),"32672");
		assertEquals(cur.getField(0,"local_type_name"),"VARCHAR");
		assertTrue(cur.getTypeInfoList("date"));
		assertEquals(cur.getField(0,"type_name"),"DATE");
		assertEquals(cur.getField(0,"data_type"),"91");
		assertEquals(cur.getField(0,"precision"),"10");
		assertEquals(cur.getField(0,"local_type_name"),"DATE");
		System.out.println();


		// column list
		System.out.println("COLUMN LIST: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	testsmallint smallint, "+
			"	testint integer, "+
			"	testbigint bigint, "+
			"	testdecimal "+
			"		decimal(10,2), "+
			"	testreal real, "+
			"	testdouble double, "+
			"	testchar char(40), "+
			"	testvarchar "+
			"		varchar(40), "+
			"	testdate date, "+
			"	testtime time, "+
			"	testtimestamp "+
			"		timestamp, "+
			"	testclob clob, "+
			"	testblob blob)"));
		assertTrue(con.commit());
		assertTrue(cur.getColumnList("testtable",null));
		assertEquals(cur.getColumnName(0),"column_name");
		assertEquals(cur.getColumnName(1),"data_type");
		assertEquals(cur.getColumnName(2),"character_maximum_length");
		assertEquals(cur.getColumnName(3),"numeric_precision");
		assertEquals(cur.getColumnName(4),"numeric_scale");
		assertEquals(cur.getColumnName(5),"is_nullable");
		assertEquals(cur.getColumnName(6),"column_key");
		assertEquals(cur.getColumnName(7),"column_default");
		assertEquals(cur.getColumnName(8),"extra");
		assertEquals(cur.getField(0,"column_name"),"TESTSMALLINT");
		assertEquals(cur.getField(1,"column_name"),"TESTINT");
		assertEquals(cur.getField(2,"column_name"),"TESTBIGINT");
		assertEquals(cur.getField(3,"column_name"),"TESTDECIMAL");
		assertEquals(cur.getField(4,"column_name"),"TESTREAL");
		assertEquals(cur.getField(5,"column_name"),"TESTDOUBLE");
		assertEquals(cur.getField(6,"column_name"),"TESTCHAR");
		assertEquals(cur.getField(7,"column_name"),"TESTVARCHAR");
		assertEquals(cur.getField(8,"column_name"),"TESTDATE");
		assertEquals(cur.getField(9,"column_name"),"TESTTIME");
		assertEquals(cur.getField(10,"column_name"),"TESTTIMESTAMP");
		assertEquals(cur.getField(11,"column_name"),"TESTCLOB");
		assertEquals(cur.getField(12,"column_name"),"TESTBLOB");
		assertEquals(cur.getField(0,"data_type"),"SMALLINT");
		assertEquals(cur.getField(1,"data_type"),"INTEGER");
		assertEquals(cur.getField(2,"data_type"),"BIGINT");
		assertEquals(cur.getField(3,"data_type"),"DECIMAL");
		assertEquals(cur.getField(4,"data_type"),"REAL");
		assertEquals(cur.getField(5,"data_type"),"DOUBLE");
		assertEquals(cur.getField(6,"data_type"),"CHARACTER");
		assertEquals(cur.getField(7,"data_type"),"VARCHAR");
		assertEquals(cur.getField(8,"data_type"),"DATE");
		assertEquals(cur.getField(9,"data_type"),"TIME");
		assertEquals(cur.getField(10,"data_type"),"TIMESTAMP");
		assertEquals(cur.getField(11,"data_type"),"CLOB");
		assertEquals(cur.getField(12,"data_type"),"BLOB");
		assertTrue(cur.sendQuery("drop table testtable"));
		assertTrue(con.commit());
		System.out.println();


		// column list - auto_increment, primary key
		System.out.println("COLUMN LIST - auto_increment, "+
			"primary key: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 int generated "+
			"	always as identity "+
			"	primary key, "+
			"	col2 int)"));
		assertTrue(con.commit());
		assertTrue(cur.getColumnList("testtable",null));
		assertTrue(cur.getField(0,"extra").contains("auto_increment"));
		assertTrue(cur.getField(0,"column_key").contains("PRI"));
		assertFalse(cur.getField(1,"extra").contains("auto_increment"));
		assertFalse(cur.getField(1,"column_key").contains("PRI"));
		System.out.println();
		assertTrue(cur.sendQuery("drop table testtable"));
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 int not null "+
			"	primary key, "+
			"	col2 int)"));
		assertTrue(con.commit());
		assertTrue(cur.getColumnList("testtable",null));
		assertFalse(cur.getField(0,"extra").contains("auto_increment"));
		assertTrue(cur.getField(0,"column_key").contains("PRI"));
		assertTrue(cur.sendQuery("drop table testtable"));
		assertTrue(con.commit());
		System.out.println();


		// primary keys list
		System.out.println("PRIMARY KEYS LIST: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 int not null "+
			"	primary key, "+
			"	col2 int)"));
		assertTrue(con.commit());
		assertTrue(cur.getPrimaryKeysList("testtable",null));
		assertEquals(cur.getColumnName(0),"table");
		assertEquals(cur.getColumnName(1),"non_unique");
		assertEquals(cur.getColumnName(2),"key_name");
		assertEquals(cur.getColumnName(3),"seq_in_index");
		assertEquals(cur.getColumnName(4),"column_name");
		assertEquals(cur.getColumnName(5),"collation");
		assertEquals(cur.getColumnName(6),"cardinality");
		assertEquals(cur.getColumnName(7),"sub_part");
		assertEquals(cur.getColumnName(8),"packed");
		assertEquals(cur.getColumnName(9),"null");
		assertEquals(cur.getColumnName(10),"index_type");
		assertEquals(cur.getColumnName(11),"comment");
		assertEquals(cur.getColumnName(12),"index_comment");
		assertEquals(cur.rowCount(),1);
		assertEquals(cur.getField(0,"table"),"TESTTABLE");
		assertEquals(cur.getField(0,"seq_in_index"),"1");
		assertEquals(cur.getField(0,"column_name"),"COL1");
		assertTrue(cur.getField(0,"key_name")!=null &&
			!cur.getField(0,"key_name").isEmpty());
		assertTrue(cur.sendQuery("drop table testtable"));
		assertTrue(con.commit());
		System.out.println();


		// key and index list
		System.out.println("KEY AND INDEX LIST: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 int not null "+
			"	primary key, "+
			"	col2 int)"));
		assertTrue(con.commit());
		assertTrue(cur.getKeyAndIndexList("testtable",null));
		assertEquals(cur.getColumnName(0),"table");
		assertEquals(cur.getColumnName(1),"non_unique");
		assertEquals(cur.getColumnName(2),"key_name");
		assertEquals(cur.getColumnName(3),"seq_in_index");
		assertEquals(cur.getColumnName(4),"column_name");
		assertEquals(cur.getColumnName(5),"collation");
		assertEquals(cur.getColumnName(6),"cardinality");
		assertEquals(cur.getColumnName(7),"sub_part");
		assertEquals(cur.getColumnName(8),"packed");
		assertEquals(cur.getColumnName(9),"null");
		assertEquals(cur.getColumnName(10),"index_type");
		assertEquals(cur.getColumnName(11),"comment");
		assertEquals(cur.getColumnName(12),"index_comment");
		assertEquals(cur.rowCount(),1);
		assertEquals(cur.getField(0,"table"),"TESTTABLE");
		assertEquals(cur.getField(0,"non_unique"),"0");
		assertEquals(cur.getField(0,"seq_in_index"),"1");
		assertEquals(cur.getField(0,"column_name"),"COL1");
		assertEquals(cur.getField(0,"collation"),"A");
		assertEquals(cur.getField(0,"index_type"),"3");
		assertTrue(cur.getField(0,"key_name")!=null &&
			!cur.getField(0,"key_name").isEmpty());
		assertTrue(cur.sendQuery("drop table testtable"));
		assertTrue(con.commit());
		System.out.println();


		// procedure list
		System.out.println("PROCEDURE LIST: ");
		cur.sendQuery("drop procedure testproc1");
		cur.sendQuery("drop procedure testproc2");
		cur.sendQuery("drop procedure testproc3");
		cur.sendQuery("drop procedure testproc4");
		assertTrue(cur.sendQuery(
			"create procedure testproc1("+
			"	in in1 integer, "+
			"	in in2 char(20), "+
			"	in in3 varchar(20), "+
			"	in in4 date) "+
			"language sql begin end"));
		assertTrue(cur.sendQuery(
			"create procedure testproc2("+
			"	in in1 integer, "+
			"	in in2 char(20), "+
			"	in in3 varchar(20), "+
			"	in in4 date) "+
			"language sql begin end"));
		assertTrue(cur.sendQuery(
			"create procedure testproc3("+
			"	in in1 integer, "+
			"	in in2 char(20), "+
			"	in in3 varchar(20), "+
			"	in in4 date) "+
			"language sql begin end"));
		assertTrue(cur.sendQuery(
			"create procedure testproc4("+
			"	in in1 integer, "+
			"	in in2 char(20), "+
			"	in in3 varchar(20), "+
			"	in in4 date) "+
			"language sql begin end"));
		assertTrue(con.commit());
		assertTrue(cur.getProcedureList(null));
		counter=0;
		for (long i=0; i<cur.rowCount(); i++) {
			String name=cur.getField(i,"routine_name");
			if (name!=null &&(name.equals("TESTPROC1") ||
				name.equals("TESTPROC2") ||name.equals(
					"TESTPROC3") ||name.equals(
					"TESTPROC4"))) {
				counter++;
			}
		}
		assertEquals(counter,4);
		System.out.println();


		// procedure parameter list
		System.out.println("PROCEDURE PARAMETER LIST: ");
		assertTrue(cur.getProcedureParameterList("testproc1",null));
		assertEquals(cur.getColumnName(0),"parameter_name");
		assertEquals(cur.getColumnName(1),"parameter_mode");
		assertEquals(cur.getColumnName(2),"data_type");
		assertEquals(cur.getColumnName(3),"character_maximum_length");
		assertEquals(cur.getColumnName(4),"ordinal_position");
		assertEquals(cur.rowCount(),4);
		assertEquals(cur.getField(0,"parameter_name"),"IN1");
		assertEquals(cur.getField(0,"parameter_mode"),"1");
		assertEquals(cur.getField(0,"data_type"),"INTEGER");
		assertEquals(cur.getField(0,"ordinal_position"),"1");
		assertEquals(cur.getField(1,"parameter_name"),"IN2");
		assertEquals(cur.getField(1,"parameter_mode"),"1");
		assertEquals(cur.getField(1,"data_type"),"CHARACTER");
		assertEquals(cur.getField(1,"ordinal_position"),"2");
		assertEquals(cur.getField(2,"parameter_name"),"IN3");
		assertEquals(cur.getField(2,"parameter_mode"),"1");
		assertEquals(cur.getField(2,"data_type"),"VARCHAR");
		assertEquals(cur.getField(2,"ordinal_position"),"3");
		assertEquals(cur.getField(3,"parameter_name"),"IN4");
		assertEquals(cur.getField(3,"parameter_mode"),"1");
		assertEquals(cur.getField(3,"data_type"),"DATE");
		assertEquals(cur.getField(3,"ordinal_position"),"4");
		assertTrue(cur.sendQuery("drop procedure testproc1"));
		assertTrue(cur.sendQuery("drop procedure testproc2"));
		assertTrue(cur.sendQuery("drop procedure testproc3"));
		assertTrue(cur.sendQuery("drop procedure testproc4"));
		assertTrue(con.commit());
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
			"	1,2,3,4)"));
		assertFalse(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	1,2,3,4)"));
		assertFalse(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	1,2,3,4)"));
		assertFalse(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	1,2,3,4)"));
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
