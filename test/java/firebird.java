// Copyright (c) David Muse
// See the file COPYING for more information.

import com.firstworks.sqlrelay.SQLRConnection;
import com.firstworks.sqlrelay.SQLRCursor;


class firebird extends sqlrtest {
	
	public static void	main(String[] args) {
	
		String	dbtype;
		String[]	bindvars={"1","2","3","4","5","6",
					"7","8","9","10","11"};
		String[]	bindvals={"4","4","4.4","4.4","4.4","4.4",
					"01-JAN-2004","04:00:00",
					"testchar4","testvarchar4",null};
		String[]	subvars={"var1","var2","var3"};
		String[]	subvalstrings={"hi","hello","bye"};
		long[]	subvallongs={1,2,3};
		double[]	subvaldoubles={10.55,10.556,10.5556};
		int[]	precs={4,5,6};
		int[]	scales={2,3,4};
		String	numvar;
		String	stringvar;
		String	floatvar;
		String[]	cols;
		String[]	fields;
		short	port;
		String	socket;
		short	id;
		String	filename;
		long[]	fieldlens;
	
		// instantiation
		SQLRConnection con=new SQLRConnection("sqlrelay",
						(short)9000,
						"/tmp/test.socket",
						"testuser","testpassword",0,1);
		SQLRCursor cur=new SQLRCursor(con);
	
		// get database type


		// identify
		System.out.println("IDENTIFY: ");
		assertEquals(con.identify(),"firebird");
		System.out.println();


		// ping
		System.out.println("PING: ");
		assertTrue(con.ping());
		System.out.println();


		// isolation levels
		System.out.println("ISOLATION LEVELS: ");
		// though firebird does support a "set transaction ..." statement to
		// set the isolation level, it looks like, in firebird, you can really
		// only set it through the TPB at the start of a transaction, so
		// attempts to set it should fail
		assertFalse(con.setIsolationLevel("read committed"));
		assertEquals(con.getIsolationLevel(),"read committed");
		System.out.println();

		// clear table
		cur.sendQuery("delete from testtable");
		con.commit();


		// insert
		System.out.println("INSERT: ");
		assertTrue(cur.sendQuery("insert into testtable values (1,1,1.1,1.1,1.1,1.1,'01-JAN-2001','01:00:00','testchar1','testvarchar1',null,null)"));
		System.out.println();


		// bind by position
		System.out.println("BIND BY POSITION: ");

		cur.prepareQuery("insert into testtable values (?,?,?,?,?,?,?,?,?,?,?,null)");
		assertEquals(cur.countBindVariables(),11);
		cur.inputBind("1",2);
		cur.inputBind("2",2);
		cur.inputBind("3",2.2,2,1);
		cur.inputBind("4",2.2,2,1);
		cur.inputBind("5",2.2,2,1);
		cur.inputBind("6",2.2,2,1);
		cur.inputBind("7","01-JAN-2002");
		cur.inputBind("8","02:00:00");
		cur.inputBind("9","testchar2");
		cur.inputBind("10","testvarchar2");
		cur.inputBind("11",null);
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("1",3);
		cur.inputBind("2",3);
		cur.inputBind("3",3.3,2,1);
		cur.inputBind("4",3.3,2,1);
		cur.inputBind("5",3.3,2,1);
		cur.inputBind("6",3.3,2,1);
		cur.inputBind("7","01-JAN-2003");
		cur.inputBind("8","03:00:00");
		cur.inputBind("9","testchar3");
		cur.inputBind("10","testvarchar3");
		cur.inputBind("11",null);
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
		assertTrue(cur.sendQuery("insert into testtable values (5,5,5.5,5.5,5.5,5.5,'01-JAN-2005','05:00:00','testchar5','testvarchar5',null,null)"));
		assertTrue(cur.sendQuery("insert into testtable values (6,6,6.6,6.6,6.6,6.6,'01-JAN-2006','06:00:00','testchar6','testvarchar6',null,null)"));
		assertTrue(cur.sendQuery("insert into testtable values (7,7,7.7,7.7,7.7,7.7,'01-JAN-2007','07:00:00','testchar7','testvarchar7',null,null)"));
		assertTrue(cur.sendQuery("insert into testtable values (8,8,8.8,8.8,8.8,8.8,'01-JAN-2008','08:00:00','testchar8','testvarchar8',null,null)"));
		System.out.println();


		// affected rows
		System.out.println("AFFECTED ROWS: ");
		assertEquals(cur.affectedRows(),0);
		System.out.println();


	    	// stored procedure
	    	System.out.println("STORED PROCEDURE: ");
	    	cur.prepareQuery("select * from testproc(?,?,?,null)");
	    	cur.inputBind("1",1);
	    	cur.inputBind("2",1.1,2,1);
	    	cur.inputBind("3","hello");
	    	assertTrue(cur.executeQuery());
	    	assertEquals(cur.getField(0,0),"1");
	    	assertEquals(cur.getField(0,1),"1.1000");
	    	assertEquals(cur.getField(0,2),"hello");
	    	cur.prepareQuery("execute procedure testproc ?, ?, ?, null");
	    	cur.inputBind("1",1);
	    	cur.inputBind("2",1.1,2,1);
	    	cur.inputBind("3","hello");
		cur.defineOutputBindInteger("1");
		cur.defineOutputBindDouble("2");
		cur.defineOutputBindString("3",20);
		cur.defineOutputBindBlob("4");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("1"),1);
		//assertEquals(cur.getOutputBindDouble("2"),1.1);
		assertEquals(cur.getOutputBindString("3"),"hello               ");
	    	System.out.println();


		// select
		System.out.println("SELECT: ");
		assertTrue(cur.sendQuery("select * from testtable order by testinteger"));
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
		assertEquals(cur.getColumnType("TESTDOUBLE"),"DOUBLE PRECISION");
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
		assertEquals(cur.getField(0,8),"testchar1                                         ");
		assertEquals(cur.getField(0,9),"testvarchar1");
		System.out.println();
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(7,1),"8");
		assertEquals(cur.getField(7,2),"8.80");
		assertEquals(cur.getField(7,3),"8.80");
		assertEquals(cur.getField(7,4),"8.8000");
		assertEquals(cur.getField(7,5),"8.8000");
		assertEquals(cur.getField(7,6),"2008:01:01");
		assertEquals(cur.getField(7,7),"08:00:00");
		assertEquals(cur.getField(7,8),"testchar8                                         ");
		assertEquals(cur.getField(7,9),"testvarchar8");
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
		assertEquals(cur.getField(0,"TESTCHAR"),"testchar1                                         ");
		assertEquals(cur.getField(0,"TESTVARCHAR"),"testvarchar1");
		System.out.println();
		assertEquals(cur.getField(7,"TESTINTEGER"),"8");
		assertEquals(cur.getField(7,"TESTSMALLINT"),"8");
		assertEquals(cur.getField(7,"TESTDECIMAL"),"8.80");
		assertEquals(cur.getField(7,"TESTNUMERIC"),"8.80");
		assertEquals(cur.getField(7,"TESTFLOAT"),"8.8000");
		assertEquals(cur.getField(7,"TESTDOUBLE"),"8.8000");
		assertEquals(cur.getField(7,"TESTDATE"),"2008:01:01");
		assertEquals(cur.getField(7,"TESTTIME"),"08:00:00");
		assertEquals(cur.getField(7,"TESTCHAR"),"testchar8                                         ");
		assertEquals(cur.getField(7,"TESTVARCHAR"),"testvarchar8");
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
		assertEquals(fields[8],"testchar1                                         ");
		assertEquals(fields[9],"testvarchar1");
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


		// individual substitutions
		System.out.println("INDIVIDUAL SUBSTITUTIONS: ");
		cur.prepareQuery("select $(var1),'$(var2)','$(var3)' from rdb$database");
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
		cur.prepareQuery("select '$(var1)','$(var2)','$(var3)' from rdb$database");
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
		cur.prepareQuery("select $(var1),$(var2),$(var3) from rdb$database");
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
		cur.prepareQuery("select $(var1),$(var2),$(var3) from rdb$database");
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
		cur.getNullsAsNulls();
		assertTrue(cur.sendQuery("select 1,null,null from rdb$database"));
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),null);
		assertEquals(cur.getField(0,2),null);
		cur.getNullsAsEmptyStrings();
		assertTrue(cur.sendQuery("select 1,null,null from rdb$database"));
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"");
		assertEquals(cur.getField(0,2),"");
		cur.getNullsAsNulls();
		System.out.println();


		// result set buffer size
		System.out.println("RESULT SET BUFFER SIZE: ");
		assertEquals(cur.getResultSetBufferSize(),0);
		cur.setResultSetBufferSize(2);
		assertTrue(cur.sendQuery("select * from testtable order by testinteger"));
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
		System.out.println();


		// dont get column info
		System.out.println("DONT GET COLUMN INFO: ");
		cur.dontGetColumnInfo();
		assertTrue(cur.sendQuery("select * from testtable order by testinteger"));
		assertEquals(cur.getColumnName(0),null);
		assertEquals(cur.getColumnLength(0),0);
		assertEquals(cur.getColumnType(0),null);
		cur.getColumnInfo();
		assertTrue(cur.sendQuery("select * from testtable order by testinteger"));
		assertEquals(cur.getColumnName(0),"TESTINTEGER");
		assertEquals(cur.getColumnLength(0),4);
		assertEquals(cur.getColumnType(0),"INTEGER");
		System.out.println();


		// suspended session
		System.out.println("SUSPENDED SESSION: ");
		assertTrue(cur.sendQuery("select * from testtable order by testinteger"));
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
		assertTrue(cur.sendQuery("select * from testtable order by testinteger"));
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
		assertTrue(cur.sendQuery("select * from testtable order by testinteger"));
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
		assertTrue(cur.sendQuery("select * from testtable order by testinteger"));
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
		assertTrue(cur.sendQuery("select * from testtable order by testinteger"));
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
		System.out.println();


		// cached result set with result set buffer size
		System.out.println("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
		cur.setResultSetBufferSize(2);
		cur.cacheToFile("cachefile1");
		cur.setCacheTtl(200);
		assertTrue(cur.sendQuery("select * from testtable order by testinteger"));
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
		System.out.println("FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET BUFFER SIZE: ");
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
		System.out.println("CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE: ");
		cur.setResultSetBufferSize(2);
		cur.cacheToFile("cachefile1");
		cur.setCacheTtl(200);
		assertTrue(cur.sendQuery("select * from testtable order by testinteger"));
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
	
		//System.out.println("COMMIT AND ROLLBACK: ");
		SQLRConnection secondcon=new SQLRConnection("sqlrelay",
						(short)9000,
						"/tmp/test.socket",
						"testuser","testpassword",0,1);
		SQLRCursor secondcur=new SQLRCursor(secondcon);
		assertEquals(secondcur.sendQuery("select count(*) from testtable"),1);
		assertEquals(secondcur.getField(0,0),"0");
		assertTrue(con.commit());
		assertEquals(secondcur.sendQuery("select count(*) from testtable"),1);
		assertEquals(secondcur.getField(0,0),"8");
		assertTrue(con.autoCommitOn());
		assertTrue(cur.sendQuery("insert into testtable values (10,10,10.1,10.1,10.1,10.1,'01-JAN-2010','10:00:00','testchar10','testvarchar10',null,null)"));
		assertEquals(secondcur.sendQuery("select count(*) from testtable"),1);
		assertEquals(secondcur.getField(0,0),"9");
		assertTrue(con.autoCommitOff());
		System.out.println();


		// finished suspended session
		System.out.println("FINISHED SUSPENDED SESSION: ");
		assertTrue(cur.sendQuery("select * from testtable order by testinteger"));
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
		cur.sendQuery("delete from testtable");
		con.commit();
		System.out.println();
	
		// invalid queries...


		// invalid queries
		System.out.println("INVALID QUERIES: ");
		assertFalse(cur.sendQuery("select * from testtable1 order by testinteger"));
		assertFalse(cur.sendQuery("select * from testtable1 order by testinteger"));
		assertFalse(cur.sendQuery("select * from testtable1 order by testinteger"));
		assertFalse(cur.sendQuery("select * from testtable1 order by testinteger"));
		System.out.println();
		assertFalse(cur.sendQuery("insert into testtable1 values (1,2,3,4)"));
		assertFalse(cur.sendQuery("insert into testtable1 values (1,2,3,4)"));
		assertFalse(cur.sendQuery("insert into testtable1 values (1,2,3,4)"));
		assertFalse(cur.sendQuery("insert into testtable1 values (1,2,3,4)"));
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
