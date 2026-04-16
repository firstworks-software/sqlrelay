// Copyright (c) David Muse
// See the file COPYING for more information.

import com.firstworks.sqlrelay.SQLRConnection;
import com.firstworks.sqlrelay.SQLRCursor;


class firebird extends sqlrtest {

	public static void	main(String[] args) {

		String[]	bindvars={"1","2","3","4","5","6",
					"7","8","9","10","11","12"};
		String[]	bindvals={"7","7","7.7","7.7","7.7","7.7",
					"01-JAN-2007","07:00:00",
					"testchar7","testvarchar7",
					null,"testblob7"};
		String[]	cols;
		String[]	fields;
		long[]		fieldlens;
		String[]	subvars={"var1","var2","var3"};
		long[]		subvallongs={1,2,3};
		String[]	subvalstrings={"hi","hello","bye"};
		double[]	subvaldoubles={10.55,10.556,10.5556};
		int[]		precs={4,5,6};
		int[]		scales={2,3,4};
		short		port;
		String		socket;
		short		id;
		String		filename;
		long		counter=0;

		int		LARGE_BUFFER_LENGTH=20*1024;
		StringBuilder	largebuffer=new StringBuilder();


		// instantiation
		SQLRConnection con=new SQLRConnection("sqlrelay",(short)9000,
					"/tmp/test.socket","testuser",
					"testpassword",0,1);
		SQLRCursor cur=new SQLRCursor(con);


		// identify
		System.out.println("IDENTIFY: ");
		assertEquals(con.identify(),"firebird");
		System.out.println();


		// ping
		System.out.println("PING: ");
		assertTrue(con.ping());
		System.out.println();


		// bind format
		System.out.println("BIND FORMAT: ");
		assertEquals(con.bindFormat(),"?");
		System.out.println();


		// nextval format
		System.out.println("NEXTVAL FORMAT: ");
		assertEquals(con.nextvalFormat(),"next value for %s");
		System.out.println();


		// isolation levels
		System.out.println("ISOLATION LEVELS: ");
		// though firebird does support a
		// "set transaction ..." statement to
		// set the isolation level, it looks
		// like, in firebird, you can really
		// only set it through the TPB at the
		// start of a transaction, so attempts to set it should fail
		assertFalse(con.setIsolationLevel("read committed"));
		assertEquals(con.getIsolationLevel(),"read committed");
		System.out.println();


		// insert
		System.out.println("INSERT: ");
		cur.sendQuery("delete from testtable");
		con.commit();
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	1, "+
			"	1, "+
			"	1.1, "+
			"	1.1, "+
			"	1.1, "+
			"	1.1, "+
			"	'01-JAN-2001', "+
			"	'01:00:00', "+
			"	'testchar1', "+
			"	'testvarchar1', "+
			"	NULL, "+
			"	'testblob1')"));
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
			"	?, "+
			"	?)");
		assertEquals(cur.countBindVariables(),12);
		cur.inputBind("1",2);
		cur.inputBind("2",2);
		cur.inputBind("3",2.2,2,1);
		cur.inputBind("4",2.2,2,1);
		cur.inputBind("5",2.2,2,1);
		cur.inputBind("6",2.2,2,1);
		cur.inputBind("7",(short)2002,(short)1,(short)1,
				(short)-1,(short)-1,(short)-1,-1,null,false);
		cur.inputBind("8",(short)-1,(short)-1,(short)-1,
				(short)2,(short)0,(short)0,0,null,false);
		cur.inputBind("9","testchar2");
		cur.inputBind("10","testvarchar2");
		cur.inputBind("11",(String)null);
		cur.inputBindBlob("12",(new String("testblob2")).getBytes(),9);
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("1",3);
		cur.inputBind("2",3);
		cur.inputBind("3",3.3,2,1);
		cur.inputBind("4",3.3,2,1);
		cur.inputBind("5",3.3,2,1);
		cur.inputBind("6",3.3,2,1);
		cur.inputBind("7",(short)2003,(short)1,(short)1,
				(short)-1,(short)-1,(short)-1,-1,null,false);
		cur.inputBind("8",(short)-1,(short)-1,(short)-1,
				(short)3,(short)0,(short)0,0,null,false);
		cur.inputBind("9","testchar3");
		cur.inputBind("10","testvarchar3");
		cur.inputBind("11",(String)null);
		cur.inputBindBlob("12",(new String("testblob3")).getBytes(),9);
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("1",4);
		cur.inputBind("2",4);
		cur.inputBind("3",4.4,2,1);
		cur.inputBind("4",4.4,2,1);
		cur.inputBind("5",4.4,2,1);
		cur.inputBind("6",4.4,2,1);
		cur.inputBind("7",(short)2004,(short)1,(short)1,
				(short)-1,(short)-1,(short)-1,-1,null,false);
		cur.inputBind("8",(short)-1,(short)-1,(short)-1,
				(short)4,(short)0,(short)0,0,null,false);
		cur.inputBind("9","testchar4");
		cur.inputBind("10","testvarchar4");
		cur.inputBind("11",(String)null);
		cur.inputBindBlob("12",(new String("testblob4")).getBytes(),9);
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("1",5);
		cur.inputBind("2",5);
		cur.inputBind("3",5.5,2,1);
		cur.inputBind("4",5.5,2,1);
		cur.inputBind("5",5.5,2,1);
		cur.inputBind("6",5.5,2,1);
		cur.inputBind("7",(short)2005,(short)1,(short)1,
				(short)-1,(short)-1,(short)-1,-1,null,false);
		cur.inputBind("8",(short)-1,(short)-1,(short)-1,
				(short)5,(short)0,(short)0,0,null,false);
		cur.inputBind("9","testchar5");
		cur.inputBind("10","testvarchar5");
		cur.inputBind("11",(String)null);
		cur.inputBindBlob("12",(new String("testblob5")).getBytes(),9);
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("1",6);
		cur.inputBind("2",6);
		cur.inputBind("3",6.6,2,1);
		cur.inputBind("4",6.6,2,1);
		cur.inputBind("5",6.6,2,1);
		cur.inputBind("6",6.6,2,1);
		cur.inputBind("7",(short)2006,(short)1,(short)1,
				(short)-1,(short)-1,(short)-1,-1,null,false);
		cur.inputBind("8",(short)-1,(short)-1,(short)-1,
				(short)6,(short)0,(short)0,0,null,false);
		cur.inputBind("9","testchar6");
		cur.inputBind("10","testvarchar6");
		cur.inputBind("11",(String)null);
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
		cur.inputBind("3",8.8,2,1);
		cur.inputBind("4",8.8,2,1);
		cur.inputBind("5",8.8,2,1);
		cur.inputBind("6",8.8,2,1);
		cur.inputBind("7",(short)2008,(short)1,(short)1,
				(short)-1,(short)-1,(short)-1,-1,null,false);
		cur.inputBind("8",(short)-1,(short)-1,(short)-1,
				(short)8,(short)0,(short)0,0,null,false);
		cur.inputBind("9","testchar8");
		cur.inputBind("10","testvarchar8");
		cur.inputBind("11",(String)null);
		cur.inputBindBlob("12",(new String("testblob8")).getBytes(),9);
		cur.validateBinds();
		assertTrue(cur.executeQuery());
		System.out.println();


		// input bind by name
		// firebird doesn't support bind by name


		// array of input binds by name
		// firebird doesn't support bind by name


		// input bind by name with validation
		// firebird doesn't support bind by name


		// select
		System.out.println("SELECT: ");
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testinteger "));
		System.out.println();


		// column count
		System.out.println("COLUMN COUNT: ");
		assertEquals(cur.colCount(),12);
		System.out.println();


		// column names
		System.out.println("COLUMN NAMES: ");
		assertEquals(cur.getColumnName(0),"TESTINTEGER");
		assertEquals(cur.getColumnName(1),"TESTSMALLINT");
		assertEquals(cur.getColumnName(2),"TESTDECIMAL");
		assertEquals(cur.getColumnName(3),"TESTNUMERIC");
		assertEquals(cur.getColumnName(4),"TESTFLOAT");
		assertEquals(cur.getColumnName(5),"TESTDOUBLE");
		assertEquals(cur.getColumnName(6),"TESTDATE");
		assertEquals(cur.getColumnName(7),"TESTTIME");
		assertEquals(cur.getColumnName(8),"TESTCHAR");
		assertEquals(cur.getColumnName(9),"TESTVARCHAR");
		assertEquals(cur.getColumnName(10),"TESTTIMESTAMP");
		assertEquals(cur.getColumnName(11),"TESTBLOB");
		cols=cur.getColumnNames();
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
		System.out.println();


		// column types
		System.out.println("COLUMN TYPES: ");
		assertEquals(cur.getColumnType(0),"INTEGER");
		assertEquals(cur.getColumnType("TESTINTEGER"),"INTEGER");
		assertEquals(cur.getColumnType(1),"SMALLINT");
		assertEquals(cur.getColumnType("TESTSMALLINT"),"SMALLINT");
		assertEquals(cur.getColumnType(2),"DECIMAL");
		assertEquals(cur.getColumnType("TESTDECIMAL"),"DECIMAL");
		assertEquals(cur.getColumnType(3),"NUMERIC");
		assertEquals(cur.getColumnType("TESTNUMERIC"),"NUMERIC");
		assertEquals(cur.getColumnType(4),"FLOAT");
		assertEquals(cur.getColumnType("TESTFLOAT"),"FLOAT");
		assertEquals(cur.getColumnType(5),"DOUBLE PRECISION");
		assertEquals(cur.getColumnType("TESTDOUBLE"),
					"DOUBLE PRECISION");
		assertEquals(cur.getColumnType(6),"DATE");
		assertEquals(cur.getColumnType("TESTDATE"),"DATE");
		assertEquals(cur.getColumnType(7),"TIME");
		assertEquals(cur.getColumnType("TESTTIME"),"TIME");
		assertEquals(cur.getColumnType(8),"CHAR");
		assertEquals(cur.getColumnType("TESTCHAR"),"CHAR");
		assertEquals(cur.getColumnType(9),"VARCHAR");
		assertEquals(cur.getColumnType("TESTVARCHAR"),"VARCHAR");
		assertEquals(cur.getColumnType(10),"TIMESTAMP");
		assertEquals(cur.getColumnType("TESTTIMESTAMP"),"TIMESTAMP");
		assertEquals(cur.getColumnType(11),"BLOB");
		assertEquals(cur.getColumnType("TESTBLOB"),"BLOB");
		System.out.println();


		// column length
		System.out.println("COLUMN LENGTH: ");
		assertEquals(cur.getColumnLength(0),4);
		assertEquals(cur.getColumnLength("TESTINTEGER"),4);
		assertEquals(cur.getColumnLength(1),2);
		assertEquals(cur.getColumnLength("TESTSMALLINT"),2);
		assertEquals(cur.getColumnLength(2),8);
		assertEquals(cur.getColumnLength("TESTDECIMAL"),8);
		assertEquals(cur.getColumnLength(3),8);
		assertEquals(cur.getColumnLength("TESTNUMERIC"),8);
		assertEquals(cur.getColumnLength(4),4);
		assertEquals(cur.getColumnLength("TESTFLOAT"),4);
		assertEquals(cur.getColumnLength(5),8);
		assertEquals(cur.getColumnLength("TESTDOUBLE"),8);
		assertEquals(cur.getColumnLength(6),4);
		assertEquals(cur.getColumnLength("TESTDATE"),4);
		assertEquals(cur.getColumnLength(7),4);
		assertEquals(cur.getColumnLength("TESTTIME"),4);
		assertEquals(cur.getColumnLength(8),50);
		assertEquals(cur.getColumnLength("TESTCHAR"),50);
		assertEquals(cur.getColumnLength(9),50);
		assertEquals(cur.getColumnLength("TESTVARCHAR"),50);
		assertEquals(cur.getColumnLength(10),8);
		assertEquals(cur.getColumnLength("TESTTIMESTAMP"),8);
		assertEquals(cur.getColumnLength(11),8);
		assertEquals(cur.getColumnLength("TESTBLOB"),8);
		System.out.println();


		// longest column
		System.out.println("LONGEST COLUMN: ");
		assertEquals(cur.getLongest(0),1);
		assertEquals(cur.getLongest("TESTINTEGER"),1);
		assertEquals(cur.getLongest(1),1);
		assertEquals(cur.getLongest("TESTSMALLINT"),1);
		assertEquals(cur.getLongest(2),4);
		assertEquals(cur.getLongest("TESTDECIMAL"),4);
		assertEquals(cur.getLongest(3),4);
		assertEquals(cur.getLongest("TESTNUMERIC"),4);
		assertEquals(cur.getLongest(4),6);
		assertEquals(cur.getLongest("TESTFLOAT"),6);
		assertEquals(cur.getLongest(5),6);
		assertEquals(cur.getLongest("TESTDOUBLE"),6);
		assertEquals(cur.getLongest(6),10);
		assertEquals(cur.getLongest("TESTDATE"),10);
		assertEquals(cur.getLongest(7),8);
		assertEquals(cur.getLongest("TESTTIME"),8);
		assertEquals(cur.getLongest(8),50);
		assertEquals(cur.getLongest("TESTCHAR"),50);
		assertEquals(cur.getLongest(9),12);
		assertEquals(cur.getLongest("TESTVARCHAR"),12);
		assertEquals(cur.getLongest(10),0);
		assertEquals(cur.getLongest("TESTTIMESTAMP"),0);
		assertEquals(cur.getLongest(11),9);
		assertEquals(cur.getLongest("TESTBLOB"),9);
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
		assertEquals(cur.getField(0,2),"1.10");
		assertEquals(cur.getField(0,3),"1.10");
		assertEquals(cur.getField(0,4),"1.1000");
		assertEquals(cur.getField(0,5),"1.1000");
		assertEquals(cur.getField(0,6),"2001:01:01");
		assertEquals(cur.getField(0,7),"01:00:00");
		assertEquals(cur.getField(0,8),"testchar1"+
				"                                         ");
		assertEquals(cur.getField(0,9),"testvarchar1");
		assertEquals(cur.getField(0,11),"testblob1");
		System.out.println();
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(7,1),"8");
		assertEquals(cur.getField(7,2),"8.80");
		assertEquals(cur.getField(7,3),"8.80");
		assertEquals(cur.getField(7,4),"8.8000");
		assertEquals(cur.getField(7,5),"8.8000");
		assertEquals(cur.getField(7,6),"2008:01:01");
		assertEquals(cur.getField(7,7),"08:00:00");
		assertEquals(cur.getField(7,8),"testchar8"+
				"                                         ");
		assertEquals(cur.getField(7,9),"testvarchar8");
		assertEquals(cur.getField(7,11),"testblob8");
		System.out.println();


		// field lengths by index
		System.out.println("FIELD LENGTHS BY INDEX: ");
		assertEquals(cur.getFieldLength(0,0),1);
		assertEquals(cur.getFieldLength(0,1),1);
		assertEquals(cur.getFieldLength(0,2),4);
		assertEquals(cur.getFieldLength(0,3),4);
		assertEquals(cur.getFieldLength(0,4),6);
		assertEquals(cur.getFieldLength(0,5),6);
		assertEquals(cur.getFieldLength(0,6),10);
		assertEquals(cur.getFieldLength(0,7),8);
		assertEquals(cur.getFieldLength(0,8),50);
		assertEquals(cur.getFieldLength(0,9),12);
		System.out.println();
		assertEquals(cur.getFieldLength(7,0),1);
		assertEquals(cur.getFieldLength(7,1),1);
		assertEquals(cur.getFieldLength(7,2),4);
		assertEquals(cur.getFieldLength(7,3),4);
		assertEquals(cur.getFieldLength(7,4),6);
		assertEquals(cur.getFieldLength(7,5),6);
		assertEquals(cur.getFieldLength(7,6),10);
		assertEquals(cur.getFieldLength(7,7),8);
		assertEquals(cur.getFieldLength(7,8),50);
		assertEquals(cur.getFieldLength(7,9),12);
		System.out.println();


		// fields by name
		System.out.println("FIELDS BY NAME: ");
		assertEquals(cur.getField(0,"TESTINTEGER"),"1");
		assertEquals(cur.getField(0,"TESTSMALLINT"),"1");
		assertEquals(cur.getField(0,"TESTDECIMAL"),"1.10");
		assertEquals(cur.getField(0,"TESTNUMERIC"),"1.10");
		assertEquals(cur.getField(0,"TESTFLOAT"),"1.1000");
		assertEquals(cur.getField(0,"TESTDOUBLE"),"1.1000");
		assertEquals(cur.getField(0,"TESTDATE"),"2001:01:01");
		assertEquals(cur.getField(0,"TESTTIME"),"01:00:00");
		assertEquals(cur.getField(0,"TESTCHAR"),"testchar1"+
				"                                         ");
		assertEquals(cur.getField(0,"TESTVARCHAR"),"testvarchar1");
		assertEquals(cur.getField(0,"TESTBLOB"),"testblob1");
		System.out.println();
		assertEquals(cur.getField(7,"TESTINTEGER"),"8");
		assertEquals(cur.getField(7,"TESTSMALLINT"),"8");
		assertEquals(cur.getField(7,"TESTDECIMAL"),"8.80");
		assertEquals(cur.getField(7,"TESTNUMERIC"),"8.80");
		assertEquals(cur.getField(7,"TESTFLOAT"),"8.8000");
		assertEquals(cur.getField(7,"TESTDOUBLE"),"8.8000");
		assertEquals(cur.getField(7,"TESTDATE"),"2008:01:01");
		assertEquals(cur.getField(7,"TESTTIME"),"08:00:00");
		assertEquals(cur.getField(7,"TESTCHAR"),"testchar8"+
				"                                         ");
		assertEquals(cur.getField(7,"TESTVARCHAR"),"testvarchar8");
		assertEquals(cur.getField(7,"TESTBLOB"),"testblob8");
		System.out.println();


		// field lengths by name
		System.out.println("FIELD LENGTHS BY NAME: ");
		assertEquals(cur.getFieldLength(0,"TESTINTEGER"),1);
		assertEquals(cur.getFieldLength(0,"TESTSMALLINT"),1);
		assertEquals(cur.getFieldLength(0,"TESTDECIMAL"),4);
		assertEquals(cur.getFieldLength(0,"TESTNUMERIC"),4);
		assertEquals(cur.getFieldLength(0,"TESTFLOAT"),6);
		assertEquals(cur.getFieldLength(0,"TESTDOUBLE"),6);
		assertEquals(cur.getFieldLength(0,"TESTDATE"),10);
		assertEquals(cur.getFieldLength(0,"TESTTIME"),8);
		assertEquals(cur.getFieldLength(0,"TESTCHAR"),50);
		assertEquals(cur.getFieldLength(0,"TESTVARCHAR"),12);
		System.out.println();
		assertEquals(cur.getFieldLength(7,"TESTINTEGER"),1);
		assertEquals(cur.getFieldLength(7,"TESTSMALLINT"),1);
		assertEquals(cur.getFieldLength(7,"TESTDECIMAL"),4);
		assertEquals(cur.getFieldLength(7,"TESTNUMERIC"),4);
		assertEquals(cur.getFieldLength(7,"TESTFLOAT"),6);
		assertEquals(cur.getFieldLength(7,"TESTDOUBLE"),6);
		assertEquals(cur.getFieldLength(7,"TESTDATE"),10);
		assertEquals(cur.getFieldLength(7,"TESTTIME"),8);
		assertEquals(cur.getFieldLength(7,"TESTCHAR"),50);
		assertEquals(cur.getFieldLength(7,"TESTVARCHAR"),12);
		System.out.println();


		// fields by array
		System.out.println("FIELDS BY ARRAY: ");
		fields=cur.getRow(0);
		assertEquals(fields[0],"1");
		assertEquals(fields[1],"1");
		assertEquals(fields[2],"1.10");
		assertEquals(fields[3],"1.10");
		assertEquals(fields[4],"1.1000");
		assertEquals(fields[5],"1.1000");
		assertEquals(fields[6],"2001:01:01");
		assertEquals(fields[7],"01:00:00");
		assertEquals(fields[8],"testchar1"+
				"                                         ");
		assertEquals(fields[9],"testvarchar1");
		assertEquals(fields[11],"testblob1");
		System.out.println();


		// field lengths by array
		System.out.println("FIELD LENGTHS BY ARRAY: ");
		fieldlens=cur.getRowLengths(0);
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
			"	testinteger "));
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
			"	testinteger "));
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
			"	testinteger "));
		assertEquals(cur.getColumnName(0),"TESTINTEGER");
		assertEquals(cur.getColumnLength(0),4);
		assertEquals(cur.getColumnType(0),"INTEGER");
		System.out.println();


		// suspended session
		System.out.println("SUSPENDED SESSION: ");
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testinteger "));
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
			"	testinteger "));
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
			"	testinteger "));
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
			"	testinteger "));
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
			"	testinteger "));
		filename=cur.getCacheFileName();
		assertEquals(filename,"cachefile1");
		cur.cacheOff();
		assertTrue(cur.openCachedResultSet(filename));
		assertEquals(cur.getField(7,0),"8");
		System.out.println();


		// column count for cached result set
		System.out.println("COLUMN COUNT FOR CACHED RESULT SET: ");
		assertEquals(cur.colCount(),12);
		System.out.println();


		// column names for cached result set
		System.out.println("COLUMN NAMES FOR CACHED RESULT SET: ");
		assertEquals(cur.getColumnName(0),"TESTINTEGER");
		assertEquals(cur.getColumnName(1),"TESTSMALLINT");
		assertEquals(cur.getColumnName(2),"TESTDECIMAL");
		assertEquals(cur.getColumnName(3),"TESTNUMERIC");
		assertEquals(cur.getColumnName(4),"TESTFLOAT");
		assertEquals(cur.getColumnName(5),"TESTDOUBLE");
		assertEquals(cur.getColumnName(6),"TESTDATE");
		assertEquals(cur.getColumnName(7),"TESTTIME");
		assertEquals(cur.getColumnName(8),"TESTCHAR");
		assertEquals(cur.getColumnName(9),"TESTVARCHAR");
		assertEquals(cur.getColumnName(10),"TESTTIMESTAMP");
		assertEquals(cur.getColumnName(11),"TESTBLOB");
		cols=cur.getColumnNames();
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
		System.out.println();


		// cached result set with result set buffer size
		System.out.println("CACHED RESULT SET WITH "+
			"RESULT SET BUFFER SIZE: ");
		cur.setResultSetBufferSize(2);
		cur.cacheToFile("cachefile1");
		cur.setCacheTtl(200);
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testinteger "));
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
			"	testinteger "));
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
			"	testinteger "));
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

			SQLRCursor secondcur2=new SQLRCursor(con);
			secondcur2.setResultSetBufferSize(1);
			assertTrue(secondcur2.sendQuery(
				"select "+
				"	* "+
				"from "+
				"	testtable"));
			secondcur2.closeResultSet();
		}
		cur.setResultSetBufferSize(0);
		System.out.println();


		// commit and rollback
		System.out.println("COMMIT AND ROLLBACK: ");
		SQLRConnection secondcon=new SQLRConnection("sqlrelay",
				(short)9000,"/tmp/test.socket","testuser",
				"testpassword",0,1);
		SQLRCursor secondcur=new SQLRCursor(secondcon);
		assertTrue(secondcur.sendQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	testtable"));
		assertEquals(secondcur.getField(0,0),"0");
		assertTrue(con.commit());
		assertTrue(secondcur.sendQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	testtable"));
		assertEquals(secondcur.getField(0,0),"8");
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	10, "+
			"	10, "+
			"	10.1, "+
			"	10.1, "+
			"	10.1, "+
			"	10.1, "+
			"	'01-JAN-2010', "+
			"	'10:00:00', "+
			"	'testchar10', "+
			"	'testvarchar10', "+
			"	NULL, "+
			"	NULL)"));
		assertTrue(con.rollback());
		assertTrue(secondcur.sendQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	testtable"));
		assertEquals(secondcur.getField(0,0),"8");
		assertTrue(con.autoCommitOn());
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	10, "+
			"	10, "+
			"	10.1, "+
			"	10.1, "+
			"	10.1, "+
			"	10.1, "+
			"	'01-JAN-2010', "+
			"	'10:00:00', "+
			"	'testchar10', "+
			"	'testvarchar10', "+
			"	NULL, "+
			"	NULL)"));
		assertTrue(secondcur.sendQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	testtable"));
		assertEquals(secondcur.getField(0,0),"9");
		assertTrue(con.autoCommitOff());
		assertTrue(cur.sendQuery("delete from testtable"));
		con.commit();
		System.out.println();


		// individual substitutions
		System.out.println("INDIVIDUAL SUBSTITUTIONS: ");
		cur.prepareQuery(
			"select "+
			"	$(var1),'$(var2)',$(var3) "+
			"from "+
			"	rdb$database");
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
		cur.prepareQuery(
			"select "+
			"	'$(var1)', "+
			"	'$(var2)', "+
			"	'$(var3)' "+
			"from "+
			"	rdb$database ");
		cur.substitutions(subvars,subvalstrings);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"hi");
		assertEquals(cur.getField(0,1),"hello");
		assertEquals(cur.getField(0,2),"bye");
		System.out.println();
		cur.prepareQuery(
			"select "+
			"	$(var1),$(var2),$(var3) "+
			"from "+
			"	rdb$database");
		cur.substitutions(subvars,subvallongs);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"2");
		assertEquals(cur.getField(0,2),"3");
		System.out.println();
		cur.prepareQuery(
			"select "+
			"	$(var1),$(var2),$(var3) "+
			"from "+
			"	rdb$database");
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
			"	1,NULL,NULL "+
			"from "+
			"	rdb$database"));
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),null);
		assertEquals(cur.getField(0,2),null);
		cur.getNullsAsEmptyStrings();
		assertTrue(cur.sendQuery(
			"select "+
			"	1,NULL,NULL "+
			"from "+
			"	rdb$database"));
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"");
		assertEquals(cur.getField(0,2),"");
		System.out.println();


		// null and empty lobs
		System.out.println("NULL AND EMPTY LOBS: ");
		cur.getNullsAsNulls();
		cur.sendQuery("delete from testtable1");
		cur.prepareQuery("insert into testtable1 values (?)");
		cur.inputBindBlob("1",(new String("")).getBytes(),0);
		assertTrue(cur.executeQuery());
		cur.sendQuery("select testblob from testtable1");
		assertEquals(cur.getField(0,"TESTBLOB"),"");
		cur.sendQuery("delete from testtable1");
		cur.prepareQuery("insert into testtable1 values (?)");
		cur.inputBindBlob("1",null,0);
		assertTrue(cur.executeQuery());
		cur.sendQuery("select testblob from testtable1");
		assertEquals(cur.getField(0,"TESTBLOB"),null);
		cur.getNullsAsEmptyStrings();
		assertTrue(cur.sendQuery("delete from testtable1"));
		System.out.println();


		// long lobs
		System.out.println("LONG LOBS: ");
		cur.sendQuery("delete from testtable1");
		cur.prepareQuery("insert into testtable1 values (?)");
		for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
			largebuffer.append('C');
		}
		cur.inputBindClob("1",largebuffer.toString(),
			LARGE_BUFFER_LENGTH);
		assertTrue(cur.executeQuery());
		cur.sendQuery("select testblob from testtable1");
		assertEquals(cur.getFieldLength(0,"TESTBLOB"),
			LARGE_BUFFER_LENGTH);
		assertEquals(cur.getField(0,"TESTBLOB"),largebuffer.toString());
		assertTrue(cur.sendQuery("delete from testtable1"));
		System.out.println();


		// output bind by position
		System.out.println("OUTPUT BIND BY POSITION: ");
		cur.getNullsAsNulls();
		cur.prepareQuery("execute procedure testproc ?, ?, ?, ?");
		cur.inputBind("1",1);
		cur.inputBind("2",1.1,2,1);
		cur.inputBind("3","hello");
		cur.inputBindBlob("4",(new String("blob")).getBytes(),4);
		cur.defineOutputBindInteger("1");
		cur.defineOutputBindDouble("2");
		cur.defineOutputBindString("3",20);
		cur.defineOutputBindBlob("4");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("1"),1);
		//assertEquals(
		//	cur.getOutputBindDouble("2"),
		//				1.1);
		assertEquals(cur.getOutputBindString("3"),
			"hello               ");
		assertEquals(cur.getOutputBindBlob("4"),"blob",4);
		cur.getNullsAsEmptyStrings();
		System.out.println();


		// output bind by name
		// firebird doesn't support bind by name


		// output bind by name with validation
		// firebird doesn't support
		// bind by name


		// lob output bind
		System.out.println("LOB OUTPUT BIND: ");
		cur.prepareQuery("execute procedure testproc1 ?");
		cur.inputBindBlob("1",(new String("hello")).getBytes(),5);
		cur.defineOutputBindBlob("1");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindBlob("1"),"hello",5);
		assertEquals(cur.getOutputBindLength("1"),5);
		System.out.println();


		// long output bind
		System.out.println("LONG OUTPUT BIND: ");
		largebuffer=new StringBuilder();
		for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
			largebuffer.append('C');
		}
		cur.prepareQuery("execute procedure testproc1 ?");
		cur.inputBindBlob("1",(new String(largebuffer.toString()
			)).getBytes(),LARGE_BUFFER_LENGTH);
		cur.defineOutputBindBlob("1");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindLength("1"),LARGE_BUFFER_LENGTH);
		assertEquals(cur.getOutputBindBlob("1"),largebuffer.toString(),
			LARGE_BUFFER_LENGTH);
		System.out.println();


		// negative input bind
		System.out.println("NEGATIVE INPUT BIND: ");
		cur.prepareQuery("select cast(? as integer) from rdb$database");
		cur.inputBind("1",-1);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"-1");
		System.out.println();


		// bind validation
		// firebird doesn't support bind by name


		// rebinding
		System.out.println("REBINDING: ");
		cur.prepareQuery("execute procedure testproc ?, ?, ?, ?");
		cur.inputBind("1",1);
		cur.inputBind("2",1.1,2,1);
		cur.inputBind("3","hello");
		cur.inputBindBlob("4",(new String("blob")).getBytes(),4);
		cur.defineOutputBindInteger("1");
		cur.defineOutputBindDouble("2");
		cur.defineOutputBindString("3",20);
		cur.defineOutputBindBlob("4");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("1"),1);
		cur.inputBind("1",2);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("1"),2);
		cur.inputBind("1",3);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("1"),3);
		System.out.println();


		// reexecute
		System.out.println("REEXECUTE: ");
		cur.prepareQuery("select 1 from rdb$database");
		assertTrue(cur.executeQuery());
		assertEquals(cur.rowCount(),1);
		assertEquals(cur.getField(0,0),"1");
		System.out.println();
		assertTrue(cur.executeQuery());
		assertEquals(cur.rowCount(),1);
		assertEquals(cur.getField(0,0),"1");
		System.out.println();
		cur.prepareQuery("select cast(? as int) from rdb$database");
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
		cur.prepareQuery(
			"execute block (in1 int = ?, "+
			"	in2 double precision = ?, "+
			"	in3 varchar(20) = ?) "+
			"as begin end");
		cur.inputBind("1",1);
		cur.inputBind("2",1.1,2,1);
		cur.inputBind("3","hello");
		assertTrue(cur.executeQuery());
		System.out.println();


		// stored procedure returning single value
		System.out.println("STORED PROCEDURE RETURNING SINGLE VALUE: ");
		cur.prepareQuery(
			"execute block (in1 int = ?, "+
			"	in2 double precision = ?, "+
			"	in3 varchar(20) = ?) "+
			"returns (out1 int) as begin "+
			"	out1 = in1; "+
			"	suspend; "+
			"end");
		cur.inputBind("1",1);
		cur.inputBind("2",1.1,2,1);
		cur.inputBind("3","hello");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"1");
		System.out.println();


		// stored procedure returning multiple values
		System.out.println("STORED PROCEDURE "+
			"RETURNING MULTIPLE VALUES: ");
		cur.prepareQuery(
			"execute block (in1 int = ?, "+
			"	in2 double precision = ?, "+
			"	in3 varchar(20) = ?) "+
			"returns (out1 int, "+
			"	out2 double precision, "+
			"	out3 varchar(20)) "+
			"as begin "+
			"	out1 = in1; "+
			"	out2 = in2; "+
			"	out3 = in3; "+
			"	suspend; "+
			"end");
		cur.inputBind("1",1);
		cur.inputBind("2",1.1,2,1);
		cur.inputBind("3","hello");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"1.1000");
		assertEquals(cur.getField(0,2),"hello");
		System.out.println();


		// stored procedure returning result set
		System.out.println("STORED PROCEDURE RETURNING RESULT SET: ");
		cur.prepareQuery("execute block returns (out1 int) as "+
			"declare i int; begin "+
			"	i = 1; "+
			"	while (i <= 8) do "+
			"	begin "+
			"		out1 = i; "+
			"		suspend; "+
			"		i = i + 1; "+
			"	end "+
			"end");
		assertTrue(cur.executeQuery());
		assertEquals(cur.rowCount(),8);
		System.out.println();


		// temporary tables firebird supports temporary tables,
		// but we're omitting this for now


		// encoded binary data
		// firebird doesn't support
		// encoded binary data


		// quotes
		System.out.println("QUOTES: ");
		cur.sendQuery("delete from table testtable1");
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable1 "+
			"values ("+
			"	'''''')"));
		assertTrue(cur.sendQuery("select testblob from testtable1"));
		assertEquals(cur.getFieldLength(0,0),2);
		assertTrue(cur.getField(0,0).equals("''"));
		assertTrue(cur.sendQuery("delete from testtable1"));
		System.out.println();


		// last insert id
		// firebird doesn't support auto-increment


		// database is schema
		System.out.println("DATABASE IS SCHEMA: ");
		assertFalse(con.getDatabaseIsSchema());
		System.out.println();


		// catalog list
		System.out.println("CATALOG LIST: ");
		assertTrue(cur.getCatalogList(null));
		assertEquals(cur.getColumnName(0),"Database");
		System.out.println();


		// schema list
		System.out.println("SCHEMA LIST: ");
		assertTrue(cur.getSchemaList(null));
		assertEquals(cur.getColumnName(0),"Database");
		boolean found=false;
		for (long i=0; i<cur.rowCount(); i++) {
			String val=cur.getField(i,"Database");
			if (val!=null &&val.equals("TESTUSER")) {
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
		assertTrue(cur.getTableList(null));
		counter=0;
		for (long i=0; i<cur.rowCount(); i++) {
			String name=cur.getField(i,"Tables_in_xxx");
			if (name!=null &&(name.equals("TESTTABLE1") ||
				name.equals("TESTTABLE2") ||name.equals(
					"TESTTABLE3"))) {
				counter++;
			}
		}
		assertEquals(counter,3);
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
		assertEquals(cur.getField(0,"precision"),"32767");
		assertEquals(cur.getField(0,"local_type_name"),"CHAR");
		assertTrue(cur.getTypeInfoList("varchar"));
		assertEquals(cur.getField(0,"type_name"),"VARCHAR");
		assertEquals(cur.getField(0,"data_type"),"12");
		assertEquals(cur.getField(0,"precision"),"32765");
		assertEquals(cur.getField(0,"local_type_name"),"VARCHAR");
		assertTrue(cur.getTypeInfoList("date"));
		assertEquals(cur.getField(0,"type_name"),"DATE");
		assertEquals(cur.getField(0,"data_type"),"91");
		assertEquals(cur.getField(0,"precision"),"10");
		assertEquals(cur.getField(0,"local_type_name"),"DATE");
		System.out.println();


		// column list
		System.out.println("COLUMN LIST: ");
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
		assertTrue(cur.getField(0,"column_name").equals("TESTINTEGER"));
		assertTrue(cur.getField(1,"column_name").equals(
				"TESTSMALLINT"));
		assertTrue(cur.getField(2,"column_name").equals("TESTDECIMAL"));
		assertTrue(cur.getField(3,"column_name").equals("TESTNUMERIC"));
		assertTrue(cur.getField(4,"column_name").equals("TESTFLOAT"));
		assertTrue(cur.getField(5,"column_name").equals("TESTDOUBLE"));
		assertTrue(cur.getField(6,"column_name").equals("TESTDATE"));
		assertTrue(cur.getField(7,"column_name").equals("TESTTIME"));
		assertTrue(cur.getField(8,"column_name").equals("TESTCHAR"));
		assertTrue(cur.getField(9,"column_name").equals("TESTVARCHAR"));
		assertTrue(cur.getField(10,"column_name").equals(
				"TESTTIMESTAMP"));
		assertTrue(cur.getField(11,"column_name").equals("TESTBLOB"));
		assertTrue(cur.getField(0,"data_type").equals("INTEGER"));
		assertTrue(cur.getField(1,"data_type").equals("SMALLINT"));
		assertTrue(cur.getField(2,"data_type").equals("DECIMAL"));
		assertTrue(cur.getField(3,"data_type").equals("NUMERIC"));
		assertTrue(cur.getField(4,"data_type").equals("FLOAT"));
		assertTrue(cur.getField(5,"data_type").equals(
				"DOUBLE PRECISION"));
		assertTrue(cur.getField(6,"data_type").equals("DATE"));
		assertTrue(cur.getField(7,"data_type").equals("TIME"));
		assertTrue(cur.getField(8,"data_type").equals("CHAR"));
		assertTrue(cur.getField(9,"data_type").equals("VARCHAR"));
		assertTrue(cur.getField(10,"data_type").equals("TIMESTAMP"));
		assertTrue(cur.getField(11,"data_type").equals(
			"BLOB SUB_TYPE BINARY"));
		System.out.println();


		// column list - auto_increment, primary key
		System.out.println("COLUMN LIST - auto_increment, "+
			"primary key: ");
		assertTrue(cur.getColumnList("testtable2",null));
		assertTrue(cur.getField(0,"extra").contains("auto_increment"));
		assertTrue(cur.getField(0,"column_key").contains("PRI"));
		assertFalse(cur.getField(1,"extra").contains("auto_increment"));
		assertFalse(cur.getField(1,"column_key").contains("PRI"));
		System.out.println();
		assertTrue(cur.getColumnList("testtable3",null));
		assertFalse(cur.getField(0,"extra").contains("auto_increment"));
		assertTrue(cur.getField(0,"column_key").contains("PRI"));
		System.out.println();


		// primary keys list
		System.out.println("PRIMARY KEYS LIST: ");
		assertTrue(cur.getPrimaryKeysList("testtable2",null));
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
		assertTrue(cur.getField(0,"table").equals("TESTTABLE2"));
		assertEquals(cur.getField(0,"seq_in_index"),"1");
		assertTrue(cur.getField(0,"column_name").equals("COL1"));
		assertTrue(cur.getField(0,"key_name")!=null &&!cur.getField(0,
				"key_name").isEmpty());
		System.out.println();


		// key and index list
		System.out.println("KEY AND INDEX LIST: ");
		assertTrue(cur.getKeyAndIndexList("testtable2",null));
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
		assertTrue(cur.getField(0,"table").equals("TESTTABLE2"));
		assertEquals(cur.getField(0,"non_unique"),"0");
		assertEquals(cur.getField(0,"seq_in_index"),"1");
		assertTrue(cur.getField(0,"column_name").equals("COL1"));
		assertEquals(cur.getField(0,"collation"),"A");
		assertEquals(cur.getField(0,"index_type"),"3");
		assertTrue(cur.getField(0,"key_name")!=null &&!cur.getField(0,
				"key_name").isEmpty());
		System.out.println();


		// procedure list
		System.out.println("PROCEDURE LIST: ");
		assertTrue(cur.getProcedureList(null));
		counter=0;
		for (long i=0; i<cur.rowCount(); i++) {
			String name=cur.getField(i,"routine_name");
			if (name!=null &&(name.equals("TESTPROC") ||name.equals(
					"TESTPROC1"))) {
				counter++;
			}
		}
		assertEquals(counter,2);
		System.out.println();


		// procedure parameter list
		System.out.println("PROCEDURE PARAMETER LIST: ");
		assertTrue(cur.getProcedureParameterList("testproc",null));
		assertEquals(cur.getColumnName(0),"parameter_name");
		assertEquals(cur.getColumnName(1),"parameter_mode");
		assertEquals(cur.getColumnName(2),"data_type");
		assertEquals(cur.getColumnName(3),"character_maximum_length");
		assertEquals(cur.getColumnName(4),"ordinal_position");
		assertEquals(cur.rowCount(),8);
		assertEquals(cur.getField(0,"parameter_name"),"OUT1");
		assertEquals(cur.getField(0,"parameter_mode"),"4");
		assertEquals(cur.getField(0,"data_type"),"INTEGER");
		assertEquals(cur.getField(0,"ordinal_position"),"1");
		assertEquals(cur.getField(1,"parameter_name"),"OUT2");
		assertEquals(cur.getField(1,"parameter_mode"),"4");
		assertEquals(cur.getField(1,"data_type"),"FLOAT");
		assertEquals(cur.getField(1,"ordinal_position"),"2");
		assertEquals(cur.getField(2,"parameter_name"),"OUT3");
		assertEquals(cur.getField(2,"parameter_mode"),"4");
		assertEquals(cur.getField(2,"data_type"),"VARCHAR");
		assertEquals(cur.getField(2,"ordinal_position"),"3");
		assertEquals(cur.getField(3,"parameter_name"),"OUT4");
		assertEquals(cur.getField(3,"parameter_mode"),"4");
		assertEquals(cur.getField(3,"data_type"),
			"BLOB SUB_TYPE BINARY");
		assertEquals(cur.getField(3,"ordinal_position"),"4");
		assertEquals(cur.getField(4,"parameter_name"),"IN1");
		assertEquals(cur.getField(4,"parameter_mode"),"1");
		assertEquals(cur.getField(4,"data_type"),"INTEGER");
		assertEquals(cur.getField(4,"ordinal_position"),"1");
		assertEquals(cur.getField(5,"parameter_name"),"IN2");
		assertEquals(cur.getField(5,"parameter_mode"),"1");
		assertEquals(cur.getField(5,"data_type"),"FLOAT");
		assertEquals(cur.getField(5,"ordinal_position"),"2");
		assertEquals(cur.getField(6,"parameter_name"),"IN3");
		assertEquals(cur.getField(6,"parameter_mode"),"1");
		assertEquals(cur.getField(6,"data_type"),"VARCHAR");
		assertEquals(cur.getField(6,"ordinal_position"),"3");
		assertEquals(cur.getField(7,"parameter_name"),"IN4");
		assertEquals(cur.getField(7,"parameter_mode"),"1");
		assertEquals(cur.getField(7,"data_type"),
			"BLOB SUB_TYPE BINARY");
		assertEquals(cur.getField(7,"ordinal_position"),"4");
		System.out.println();


		// invalid queries
		System.out.println("INVALID QUERIES: ");
		assertFalse(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable1 "+
			"order by "+
			"	testinteger "));
		assertFalse(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable1 "+
			"order by "+
			"	testinteger "));
		assertFalse(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable1 "+
			"order by "+
			"	testinteger "));
		assertFalse(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable1 "+
			"order by "+
			"	testinteger "));
		System.out.println();
		assertFalse(cur.sendQuery(
			"insert into "+
			"	testtable1 "+
			"values ("+
			"	1,2,3,4)"));
		assertFalse(cur.sendQuery(
			"insert into "+
			"	testtable1 "+
			"values ("+
			"	1,2,3,4)"));
		assertFalse(cur.sendQuery(
			"insert into "+
			"	testtable1 "+
			"values ("+
			"	1,2,3,4)"));
		assertFalse(cur.sendQuery(
			"insert into "+
			"	testtable1 "+
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
