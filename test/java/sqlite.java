// Copyright (c) David Muse
// See the file COPYING for more information.

import com.firstworks.sqlrelay.SQLRConnection;
import com.firstworks.sqlrelay.SQLRCursor;


class sqlite extends sqlrtest {

	public static void	main(String[] args) {

		String[]	isolationlevels={"0","1"};
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

		int		LARGE_BUFFER_LENGTH=8192;


		// instantiation
		SQLRConnection con=new SQLRConnection("sqlrelay",(short)9000,
						"/tmp/test.socket","testuser",
						"testpassword",0,1);
		SQLRCursor cur=new SQLRCursor(con);


		// identify
		System.out.println("IDENTIFY: ");
		assertEquals(con.identify(),"sqlite");
		System.out.println();


		// db version
		System.out.println("DB VERSION: ");
		String	dbversion=con.dbVersion();
		boolean	issqlite3=true;
		if (dbversion==null ||
			dbversion.equals("unknown") ||
			dbversion.length()==0 ||
			!Character.isDigit(dbversion.charAt(0)) ||
			(dbversion.charAt(0)-'0')<3) {
			issqlite3=false;
		}
		System.out.println();


		// ping
		System.out.println("PING: ");
		assertTrue(con.ping());
		System.out.println();


		// transaction state
		System.out.println("TRANSACTION STATE: ");
		assertEquals(con.getDefaultTransactionModel(),"explicit");
		assertEquals(con.getTransactionModel(),"explicit");
		assertFalse(con.getInTransaction());
		assertTrue(con.getAutoCommit());
		System.out.println();


		// bind format
		System.out.println("BIND FORMAT: ");
		assertEquals(con.bindFormat(),":*");
		System.out.println();


		// nextval format
		System.out.println("NEXTVAL FORMAT: ");
		assertEquals(con.nextvalFormat(),"");
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
		con.begin();
		cur.sendQuery("drop table if exists testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	testint int, "+
			"	testfloat float, "+
			"	testchar char(40), "+
			"	testvarchar varchar(40), "+
			"	testclob clob, "+
			"	testblob blob)"));
		con.commit();
		System.out.println();


		// insert
		System.out.println("INSERT: ");
		assertTrue(con.begin());
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	1, "+
			"	1.5, "+
			"	'testchar1', "+
			"	'testvarchar1', "+
			"	'testclob1', "+
			"	'testblob1')"));
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	2, "+
			"	2.5, "+
			"	'testchar2', "+
			"	'testvarchar2', "+
			"	'testclob2', "+
			"	'testblob2')"));
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	3, "+
			"	3.5, "+
			"	'testchar3', "+
			"	'testvarchar3', "+
			"	'testclob3', "+
			"	'testblob3')"));
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	4, "+
			"	4.5, "+
			"	'testchar4', "+
			"	'testvarchar4', "+
			"	'testclob4', "+
			"	'testblob4')"));
		System.out.println();


		// affected rows
		System.out.println("AFFECTED ROWS: ");
		assertEquals(cur.affectedRows(),1);
		System.out.println();


		// input bind by position
		// sqlite doesn't support bind by position


		// array of input binds by position
		// sqlite doesn't support bind by position


		// input bind by position with validation
		// sqlite doesn't support bind by position


		// input bind by name
		System.out.println("INPUT BIND BY NAME: ");
		cur.prepareQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	:var1, "+
			"	:var2, "+
			"	:var3, "+
			"	:var4, "+
			"	:var5, "+
			"	:var6)");
		assertEquals(cur.countBindVariables(),6);
		cur.inputBind("var1",5);
		cur.inputBind("var2",5.5,4,1);
		cur.inputBind("var3","testchar5");
		cur.inputBind("var4","testvarchar5");
		cur.inputBindClob("var5","testclob5",9);
		cur.inputBindBlob("var6",
			(new String("testblob5")).getBytes(),9);
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("var1",6);
		cur.inputBind("var2",6.5,4,1);
		cur.inputBind("var3","testchar6");
		cur.inputBind("var4","testvarchar6");
		cur.inputBindClob("var5","testclob6",9);
		cur.inputBindBlob("var6",
			(new String("testblob6")).getBytes(),9);
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("var1",7);
		cur.inputBind("var2",7.5,4,1);
		cur.inputBind("var3","testchar7");
		cur.inputBind("var4","testvarchar7");
		cur.inputBindClob("var5","testclob7",9);
		cur.inputBindBlob("var6",
			(new String("testblob7")).getBytes(),9);
		assertTrue(cur.executeQuery());
		System.out.println();


		// array of input binds by name
		// sqlite doesn't support implicit
		// conversion of string binds to other
		// data types, so arrays of binds don't generally work.


		// input bind by name with validation
		System.out.println("INPUT BIND BY NAME WITH VALIDATION: ");
		cur.clearBinds();
		cur.inputBind("var1",8);
		cur.inputBind("var2",8.5,4,1);
		cur.inputBind("var3","testchar8");
		cur.inputBind("var4","testvarchar8");
		cur.inputBindClob("var5","testclob8",9);
		cur.inputBindBlob("var6",
			(new String("testblob8")).getBytes(),9);
		cur.validateBinds();
		assertTrue(cur.executeQuery());
		System.out.println();


		// select
		System.out.println("SELECT: ");
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testint "));
		System.out.println();


		// column count
		System.out.println("COLUMN COUNT: ");
		assertEquals(cur.colCount(),6);
		System.out.println();


		// column names
		System.out.println("COLUMN NAMES: ");
		assertEquals(cur.getColumnName(0),"testint");
		assertEquals(cur.getColumnName(1),"testfloat");
		assertEquals(cur.getColumnName(2),"testchar");
		assertEquals(cur.getColumnName(3),"testvarchar");
		cols=cur.getColumnNames();
		assertEquals(cols[0],"testint");
		assertEquals(cols[1],"testfloat");
		assertEquals(cols[2],"testchar");
		assertEquals(cols[3],"testvarchar");
		System.out.println();


		// column types
		System.out.println("COLUMN TYPES: ");
		if (issqlite3) {
			assertEquals(cur.getColumnType(0),"INTEGER");
			assertEquals(cur.getColumnType("testint"),"INTEGER");
			assertEquals(cur.getColumnType(1),"FLOAT");
			assertEquals(cur.getColumnType("testfloat"),"FLOAT");
			assertEquals(cur.getColumnType(2),"STRING");
			assertEquals(cur.getColumnType("testchar"),"STRING");
			assertEquals(cur.getColumnType(3),"STRING");
			assertEquals(cur.getColumnType("testvarchar"),"STRING");
			assertEquals(cur.getColumnType(4),"STRING");
			assertEquals(cur.getColumnType("testclob"),"STRING");
			assertEquals(cur.getColumnType(5),"STRING");
			assertEquals(cur.getColumnType("testblob"),"STRING");
		} else {
			assertEquals(cur.getColumnType(0),"UNKNOWN");
			assertEquals(cur.getColumnType("testint"),"UNKNOWN");
			assertEquals(cur.getColumnType(1),"UNKNOWN");
			assertEquals(cur.getColumnType("testfloat"),"UNKNOWN");
			assertEquals(cur.getColumnType(2),"UNKNOWN");
			assertEquals(cur.getColumnType("testchar"),"UNKNOWN");
			assertEquals(cur.getColumnType(3),"UNKNOWN");
			assertEquals(cur.getColumnType("testvarchar"),"UNKNOWN");
			assertEquals(cur.getColumnType(4),"UNKNOWN");
			assertEquals(cur.getColumnType("testclob"),"UNKNOWN");
			assertEquals(cur.getColumnType(5),"UNKNOWN");
			assertEquals(cur.getColumnType("testblob"),"UNKNOWN");
		}
		System.out.println();


		// column length
		System.out.println("COLUMN LENGTH: ");
		assertEquals(cur.getColumnLength(0),0);
		assertEquals(cur.getColumnLength("testint"),0);
		assertEquals(cur.getColumnLength(1),0);
		assertEquals(cur.getColumnLength("testfloat"),0);
		assertEquals(cur.getColumnLength(2),0);
		assertEquals(cur.getColumnLength("testchar"),0);
		assertEquals(cur.getColumnLength(3),0);
		assertEquals(cur.getColumnLength("testvarchar"),0);
		assertEquals(cur.getColumnLength(4),0);
		assertEquals(cur.getColumnLength("testclob"),0);
		assertEquals(cur.getColumnLength(5),0);
		assertEquals(cur.getColumnLength("testblob"),0);
		System.out.println();


		// longest column
		System.out.println("LONGEST COLUMN: ");
		assertEquals(cur.getLongest(0),1);
		assertEquals(cur.getLongest("testint"),1);
		assertEquals(cur.getLongest(1),3);
		assertEquals(cur.getLongest("testfloat"),3);
		assertEquals(cur.getLongest(2),9);
		assertEquals(cur.getLongest("testchar"),9);
		assertEquals(cur.getLongest(3),12);
		assertEquals(cur.getLongest("testvarchar"),12);
		assertEquals(cur.getLongest(4),9);
		assertEquals(cur.getLongest("testclob"),9);
		assertEquals(cur.getLongest(5),9);
		assertEquals(cur.getLongest("testblob"),9);
		System.out.println();


		// row count
		System.out.println("ROW COUNT: ");
		assertEquals(cur.rowCount(),8);
		System.out.println();


		// total rows
		System.out.println("TOTAL ROWS: ");
		assertEquals(cur.totalRows(),(issqlite3)?0:8);
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
		assertEquals(cur.getField(0,1),"1.5");
		assertEquals(cur.getField(0,2),"testchar1");
		assertEquals(cur.getField(0,3),"testvarchar1");
		assertEquals(cur.getField(0,4),"testclob1");
		assertEquals(cur.getField(0,5),"testblob1");
		System.out.println();
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(7,1),"8.5");
		assertEquals(cur.getField(7,2),"testchar8");
		assertEquals(cur.getField(7,3),"testvarchar8");
		assertEquals(cur.getField(7,4),"testclob8");
		assertEquals(cur.getField(7,5),"testblob8");
		System.out.println();


		// field lengths by index
		System.out.println("FIELD LENGTHS BY INDEX: ");
		assertEquals(cur.getFieldLength(0,0),1);
		assertEquals(cur.getFieldLength(0,1),3);
		assertEquals(cur.getFieldLength(0,2),9);
		assertEquals(cur.getFieldLength(0,3),12);
		assertEquals(cur.getFieldLength(0,4),9);
		assertEquals(cur.getFieldLength(0,5),9);
		System.out.println();
		assertEquals(cur.getFieldLength(7,0),1);
		assertEquals(cur.getFieldLength(7,1),3);
		assertEquals(cur.getFieldLength(7,2),9);
		assertEquals(cur.getFieldLength(7,3),12);
		assertEquals(cur.getFieldLength(7,4),9);
		assertEquals(cur.getFieldLength(7,5),9);
		System.out.println();


		// fields by name
		System.out.println("FIELDS BY NAME: ");
		assertEquals(cur.getField(0,"testint"),"1");
		assertEquals(cur.getField(0,"testfloat"),"1.5");
		assertEquals(cur.getField(0,"testchar"),"testchar1");
		assertEquals(cur.getField(0,"testvarchar"),"testvarchar1");
		assertEquals(cur.getField(0,"testclob"),"testclob1");
		assertEquals(cur.getField(0,"testblob"),"testblob1");
		System.out.println();
		assertEquals(cur.getField(7,"testint"),"8");
		assertEquals(cur.getField(7,"testfloat"),"8.5");
		assertEquals(cur.getField(7,"testchar"),"testchar8");
		assertEquals(cur.getField(7,"testvarchar"),"testvarchar8");
		assertEquals(cur.getField(7,"testclob"),"testclob8");
		assertEquals(cur.getField(7,"testblob"),"testblob8");
		System.out.println();


		// field lengths by name
		System.out.println("FIELD LENGTHS BY NAME: ");
		assertEquals(cur.getFieldLength(0,"testint"),1);
		assertEquals(cur.getFieldLength(0,"testfloat"),3);
		assertEquals(cur.getFieldLength(0,"testchar"),9);
		assertEquals(cur.getFieldLength(0,"testvarchar"),12);
		assertEquals(cur.getFieldLength(0,"testclob"),9);
		assertEquals(cur.getFieldLength(0,"testblob"),9);
		System.out.println();
		assertEquals(cur.getFieldLength(7,"testint"),1);
		assertEquals(cur.getFieldLength(7,"testfloat"),3);
		assertEquals(cur.getFieldLength(7,"testchar"),9);
		assertEquals(cur.getFieldLength(7,"testvarchar"),12);
		assertEquals(cur.getFieldLength(7,"testclob"),9);
		assertEquals(cur.getFieldLength(7,"testblob"),9);
		System.out.println();


		// fields by array
		System.out.println("FIELDS BY ARRAY: ");
		fields=cur.getRow(0);
		assertEquals(fields[0],"1");
		assertEquals(fields[1],"1.5");
		assertEquals(fields[2],"testchar1");
		assertEquals(fields[3],"testvarchar1");
		assertEquals(fields[4],"testclob1");
		assertEquals(fields[5],"testblob1");
		System.out.println();


		// field lengths by array
		System.out.println("FIELD LENGTHS BY ARRAY: ");
		fieldlens=cur.getRowLengths(0);
		assertEquals(fieldlens[0],1);
		assertEquals(fieldlens[1],3);
		assertEquals(fieldlens[2],9);
		assertEquals(fieldlens[3],12);
		assertEquals(fieldlens[4],9);
		assertEquals(fieldlens[5],9);
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
			"	testint "));
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
			"	testint "));
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
			"	testint "));
		assertEquals(cur.getColumnName(0),"testint");
		assertEquals(cur.getColumnLength(0),0);
		assertEquals(cur.getColumnType(0),
				(issqlite3)?"INTEGER":"UNKNOWN");
		System.out.println();


		// suspended session
		System.out.println("SUSPENDED SESSION: ");
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testint "));
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
			"	testint "));
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
			"	testint "));
		filename=cur.getCacheFileName();
		assertEquals(filename,"cachefile1");
		cur.cacheOff();
		assertTrue(cur.openCachedResultSet(filename));
		assertEquals(cur.getField(7,0),"8");
		System.out.println();


		// column count for cached result set
		System.out.println("COLUMN COUNT FOR CACHED RESULT SET: ");
		assertEquals(cur.colCount(),6);
		System.out.println();


		// column names for cached result set
		System.out.println("COLUMN NAMES FOR CACHED RESULT SET: ");
		assertEquals(cur.getColumnName(0),"testint");
		assertEquals(cur.getColumnName(1),"testfloat");
		assertEquals(cur.getColumnName(2),"testchar");
		assertEquals(cur.getColumnName(3),"testvarchar");
		assertEquals(cur.getColumnName(4),"testclob");
		assertEquals(cur.getColumnName(5),"testblob");
		cols=cur.getColumnNames();
		assertEquals(cols[0],"testint");
		assertEquals(cols[1],"testfloat");
		assertEquals(cols[2],"testchar");
		assertEquals(cols[3],"testvarchar");
		assertEquals(cols[4],"testclob");
		assertEquals(cols[5],"testblob");
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
			"	testint "));
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
		System.out.println("FROM ONE CACHE FILE TO ANOTHER WITH "+
					"RESULT SET BUFFER SIZE: ");
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
		System.out.println("CACHED RESULT SET WITH SUSPEND AND RESULT "+
					"SET BUFFER SIZE: ");
		cur.setResultSetBufferSize(2);
		cur.cacheToFile("cachefile1");
		cur.setCacheTtl(200);
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testint "));
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
		assertTrue(cur.sendQuery("select * from testtable"));
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
		SQLRCursor secondcur2=new SQLRCursor(con);
		secondcur2.setResultSetBufferSize(1);
		for (int i=0; cur.getRow(i)!=null; i++) {
			assertTrue(secondcur2.sendQuery(
				"select "+
				"	* "+
				"from "+
				"	testtable"));
		}
		secondcur2.closeResultSet();
		cur.setResultSetBufferSize(0);
		assertTrue(cur.sendQuery("drop table if exists testtable"));
		System.out.println();


		// reset transaction state
		System.out.println("RESET TRANSACTION STATE: ");
		assertTrue(con.commit());
		assertEquals(con.getTransactionModel(),"explicit");
		assertTrue(con.getAutoCommit());
		System.out.println();


		// transaction behavior - implicit
		System.out.println("TRANSACTION BEHAVIOR - implicit: ");
		assertTrue(con.setTransactionModel("implicit"));
		assertEquals(con.getTransactionModel(),"implicit");
		assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
		// sqlite DDL is transactional; commit so the table is visible
		// to the second connection (the commit implicitly starts a new tx)
		assertTrue(con.commit());
		SQLRConnection secondcon=new SQLRConnection("sqlrelay",
				(short)9000,"/tmp/test.socket","testuser",
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
		assertEquals(con.getTransactionModel(),"explicit");
		assertTrue(con.getAutoCommit());
		System.out.println();


		// individual substitutions
		System.out.println("INDIVIDUAL SUBSTITUTIONS: ");
		cur.sendQuery("drop table if exists testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 int, "+
			"	col2 char, "+
			"	col3 float)"));
		cur.prepareQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	$(var1), "+
			"	'$(var2)', "+
			"	$(var3))");
		cur.substitution("var1",1);
		cur.substitution("var2","hello");
		cur.substitution("var3",10.5556,6,4);
		assertTrue(cur.executeQuery());
		assertTrue(cur.sendQuery("select * from testtable"));
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"hello");
		assertEquals(cur.getField(0,2),"10.5556");
		assertTrue(cur.sendQuery("delete from testtable"));
		System.out.println();


		// array substitutions
		System.out.println("ARRAY SUBSTITUTIONS: ");
		cur.prepareQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	'$(var1)', "+
			"	'$(var2)', "+
			"	'$(var3)')");
		cur.substitutions(subvars,subvalstrings);
		assertTrue(cur.executeQuery());
		assertTrue(cur.sendQuery("select * from testtable"));
		assertEquals(cur.getField(0,0),"hi");
		assertEquals(cur.getField(0,1),"hello");
		assertEquals(cur.getField(0,2),"bye");
		assertTrue(cur.sendQuery("delete from testtable"));
		System.out.println();
		cur.prepareQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	$(var1), "+
			"	'$(var2)', "+
			"	$(var3))");
		cur.substitutions(subvars,subvallongs);
		assertTrue(cur.executeQuery());
		assertTrue(cur.sendQuery("select * from testtable"));
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"2");
		assertEquals(cur.getField(0,2),"3.0");
		assertTrue(cur.sendQuery("delete from testtable"));
		System.out.println();
		cur.prepareQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	$(var1), "+
			"	'$(var2)', "+
			"	$(var3))");
		cur.substitutions(subvars,subvaldoubles,precs,scales);
		assertTrue(cur.executeQuery());
		assertTrue(cur.sendQuery("select * from testtable"));
		assertEquals(cur.getField(0,0),"10.55");
		assertEquals(cur.getField(0,1),"10.556");
		assertEquals(cur.getField(0,2),"10.5556");
		assertTrue(cur.sendQuery("delete from testtable"));
		System.out.println();


		// nulls as nulls
		System.out.println("NULLS AS NULLS: ");
		cur.getNullsAsNulls();
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	1, "+
			"	NULL, "+
			"	NULL)"));
		assertTrue(cur.sendQuery("select * from testtable"));
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),null);
		assertEquals(cur.getField(0,2),null);
		cur.getNullsAsEmptyStrings();
		assertTrue(cur.sendQuery("select * from testtable"));
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"");
		assertEquals(cur.getField(0,2),"");
		assertTrue(cur.sendQuery("drop table if exists testtable"));
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
		cur.prepareQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	:var1, "+
			"	:var2, "+
			"	:var3, "+
			"	:var4)");
		cur.inputBindClob("var1","",0);
		cur.inputBindClob("var2",null,0);
		cur.inputBindBlob("var3",(new String("")).getBytes(),0);
		cur.inputBindBlob("var4",null,0);
		assertTrue(cur.executeQuery());
		cur.sendQuery("select * from testtable");
		assertEquals(cur.getField(0,0),"");
		assertEquals(cur.getField(0,1),null);
		assertEquals(cur.getField(0,2),"");
		assertEquals(cur.getField(0,3),null);
		cur.getNullsAsEmptyStrings();
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// long lobs
		System.out.println("LONG LOBS: ");
		cur.sendQuery("drop table testtable");
		cur.sendQuery(
			"create table testtable ("+
			"	testclob clob, "+
			"	testblob blob)");
		cur.prepareQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	:clobval,:blobval)");
		StringBuilder largebuffer=new StringBuilder();
		for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
			largebuffer.append('C');
		}
		String largestr=largebuffer.toString();
		cur.inputBindClob("clobval",largestr,LARGE_BUFFER_LENGTH);
		cur.inputBindBlob("blobval",largestr.getBytes(),
			LARGE_BUFFER_LENGTH);
		assertTrue(cur.executeQuery());
		cur.sendQuery("select * from testtable");
		assertEquals(cur.getFieldLength(0,"testclob"),
			LARGE_BUFFER_LENGTH);
		assertEquals(cur.getField(0,"testclob"),largestr);
		assertEquals(cur.getFieldLength(0,"testblob"),
			LARGE_BUFFER_LENGTH);
		assertEquals(cur.getField(0,"testblob"),largestr,
			LARGE_BUFFER_LENGTH);
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// output bind by position
		// sqlite doesn't support output binds


		// output bind by name
		// sqlite doesn't support output binds


		// output bind by name with validation
		// sqlite doesn't support output binds


		// lob output bind
		// sqlite doesn't support output binds


		// long output bind
		// sqlite doesn't support output binds


		// negative input bind
		System.out.println("NEGATIVE INPUT BIND: ");
		cur.sendQuery("drop table testtable");
		cur.sendQuery("create table testtable (testval int)");
		cur.prepareQuery("insert into testtable values (:testval)");
		cur.inputBind("testval",-1);
		assertTrue(cur.executeQuery());
		cur.sendQuery("select testval from testtable");
		assertEquals(cur.getField(0,"testval"),"-1");
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// bind validation
		System.out.println("BIND VALIDATION: ");
		cur.sendQuery("drop table testtable");
		cur.sendQuery(
			"create table testtable ("+
			"	col1 varchar(20), "+
			"	col2 varchar(20), "+
			"	col3 varchar(20))");
		cur.prepareQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	$(var1), "+
			"	$(var2), "+
			"	$(var3))");
		cur.inputBind("var1","1");
		cur.inputBind("var2","2");
		cur.inputBind("var3","3");
		cur.substitution("var1",":var1");
		assertTrue(cur.validBind("var1"));
		assertFalse(cur.validBind("var2"));
		assertFalse(cur.validBind("var3"));
		assertFalse(cur.validBind("var4"));
		System.out.println();
		cur.substitution("var2",":var2");
		assertTrue(cur.validBind("var1"));
		assertTrue(cur.validBind("var2"));
		assertFalse(cur.validBind("var3"));
		assertFalse(cur.validBind("var4"));
		System.out.println();
		cur.substitution("var3",":var3");
		assertTrue(cur.validBind("var1"));
		assertTrue(cur.validBind("var2"));
		assertTrue(cur.validBind("var3"));
		assertFalse(cur.validBind("var4"));
		assertTrue(cur.executeQuery());
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// rebinding
		System.out.println("REBINDING: ");
		cur.prepareQuery("select :val");
		cur.inputBind("val",1);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"1");
		cur.inputBind("val",2);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"2");
		cur.inputBind("val",3);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"3");
		System.out.println();


		// reexecute
		System.out.println("REEXECUTE: ");
		cur.prepareQuery("select 1");
		assertTrue(cur.executeQuery());
		assertEquals(cur.rowCount(),1);
		assertEquals(cur.getField(0,0),"1");
		System.out.println();
		assertTrue(cur.executeQuery());
		assertEquals(cur.rowCount(),1);
		assertEquals(cur.getField(0,0),"1");
		System.out.println();
		cur.prepareQuery("select :var");
		cur.inputBind("var",1);
		assertTrue(cur.executeQuery());
		assertEquals(cur.rowCount(),1);
		assertEquals(cur.getField(0,0),"1");
		System.out.println();
		assertTrue(cur.executeQuery());
		assertEquals(cur.rowCount(),1);
		assertEquals(cur.getField(0,0),"1");
		System.out.println();
		cur.inputBind("var",2);
		assertTrue(cur.executeQuery());
		assertEquals(cur.rowCount(),1);
		assertEquals(cur.getField(0,0),"2");
		System.out.println();


		// stored procedure returning no value
		// sqlite doesn't support stored procedures


		// stored procedure returning single value
		// sqlite doesn't support stored procedures


		// stored procedure returning multiple values
		// sqlite doesn't support stored procedures


		// stored procedure returning result set
		// sqlite doesn't support stored procedures


		// temporary tables
		System.out.println("TEMPORARY TABLES: ");
		cur.sendQuery("drop table if exists temptable\n");
		cur.sendQuery("create temporary table temptable (col1 int)");
		assertTrue(cur.sendQuery("insert into temptable values (1)"));
		assertTrue(cur.sendQuery("select count(*) from temptable"));
		assertEquals(cur.getField(0,0),"1");
		con.endSession();
		System.out.println();
		assertFalse(cur.sendQuery("select count(*) from temptable"));
		assertTrue(cur.sendQuery("drop table if exists temptable\n"));
		System.out.println();


		// encoded binary data
		System.out.println("ENCODED BINARY DATA: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery("create table testtable (col1 blob)"));
		byte[] buffer=new byte[256];
		for (int i=0; i<256; i++) {
			buffer[i]=(byte)i;
		}
		StringBuilder querystr=new StringBuilder();
		querystr.append("insert into testtable values (X'");
		for (int i=0; i<buffer.length; i++) {
			querystr.append(String.format("%02x",buffer[i]&0xff));
		}
		querystr.append("')");
		assertTrue(cur.sendQuery(querystr.toString()));
		assertTrue(cur.sendQuery("select col1 from testtable"));
		assertEquals(cur.getFieldLength(0,0),buffer.length);
		assertEquals(cur.getFieldAsByteArray(0,0),
				buffer,buffer.length);
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
			"	col1 integer "+
			"primary key "+
			"	autoincrement, "+
			"	col2 int)"));
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	null,1)"));
		assertEquals(con.getLastInsertId(),1);
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


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
		System.out.println();


		// table type list
		System.out.println("TABLE TYPE LIST: ");
		assertTrue(cur.getTableTypeList());
		assertEquals(cur.getColumnName(0),"table_type");
		assertInResultSet(cur,"table_type","TABLE");
		System.out.println();


		// table list
		System.out.println("TABLE LIST: ");
		cur.sendQuery("drop table if exists testtable1");
		cur.sendQuery("drop table if exists testtable2");
		cur.sendQuery("drop table if exists testtable3");
		cur.sendQuery("drop table if exists testtable4");
		assertTrue(cur.sendQuery(
			"create table testtable1 ("+
			"	col1 int, "+
			"	col2 int)"));
		assertTrue(cur.sendQuery(
			"create table testtable2 ("+
			"	col1 int, "+
			"	col2 int)"));
		assertTrue(cur.sendQuery(
			"create table testtable3 ("+
			"	col1 int, "+
			"	col2 int)"));
		assertTrue(cur.sendQuery(
			"create table testtable4 ("+
			"	col1 int, "+
			"	col2 int)"));
		assertTrue(cur.getTableList(null));
		assertInResultSet(cur,"Tables_in_xxx","testtable1");
		assertInResultSet(cur,"Tables_in_xxx","testtable2");
		assertInResultSet(cur,"Tables_in_xxx","testtable3");
		assertInResultSet(cur,"Tables_in_xxx","testtable4");
		assertTrue(cur.sendQuery("drop table if exists testtable1"));
		assertTrue(cur.sendQuery("drop table if exists testtable2"));
		assertTrue(cur.sendQuery("drop table if exists testtable3"));
		assertTrue(cur.sendQuery("drop table if exists testtable4"));
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
		assertEquals(cur.getField(0,"precision"),"19");
		assertEquals(cur.getField(0,"local_type_name"),"INTEGER");
		assertTrue(cur.getTypeInfoList("char"));
		assertEquals(cur.getField(0,"type_name"),"CHAR");
		assertEquals(cur.getField(0,"data_type"),"1");
		assertEquals(cur.getField(0,"precision"),"2147483647");
		assertEquals(cur.getField(0,"local_type_name"),"CHAR");
		assertTrue(cur.getTypeInfoList("varchar"));
		assertEquals(cur.getField(0,"type_name"),"VARCHAR");
		assertEquals(cur.getField(0,"data_type"),"12");
		assertEquals(cur.getField(0,"precision"),"2147483647");
		assertEquals(cur.getField(0,"local_type_name"),"VARCHAR");
		assertTrue(cur.getTypeInfoList("date"));
		assertEquals(cur.getField(0,"type_name"),"DATE");
		assertEquals(cur.getField(0,"data_type"),"91");
		assertEquals(cur.getField(0,"precision"),"10");
		assertEquals(cur.getField(0,"local_type_name"),"DATE");
		System.out.println();


		// column list
		System.out.println("COLUMN LIST: ");
		cur.sendQuery("drop table if exists testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	testint int, "+
			"	testfloat float, "+
			"	testchar char(40), "+
			"	testvarchar "+
			"varchar(40), "+
			"	testclob clob, "+
			"	testblob blob)"));
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
		assertEquals(cur.getField(0,"column_name"),"testint");
		assertEquals(cur.getField(1,"column_name"),"testfloat");
		assertEquals(cur.getField(2,"column_name"),"testchar");
		assertEquals(cur.getField(3,"column_name"),"testvarchar");
		assertEquals(cur.getField(4,"column_name"),"testclob");
		assertEquals(cur.getField(5,"column_name"),"testblob");
		assertEquals(cur.getField(0,"data_type"),"INT");
		assertEquals(cur.getField(1,"data_type"),"FLOAT");
		assertEquals(cur.getField(2,"data_type"),"CHAR");
		assertEquals(cur.getField(3,"data_type"),"VARCHAR");
		assertEquals(cur.getField(4,"data_type"),"CLOB");
		assertEquals(cur.getField(5,"data_type"),"BLOB");
		assertTrue(cur.sendQuery("drop table if exists testtable"));
		System.out.println();


		// column list - auto_increment, primary key
		System.out.println("COLUMN LIST - auto_increment, primary "+
					"key: ");
		cur.sendQuery("drop table if exists testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 integer primary key autoincrement, "+
			"	col2 int)"));
		assertTrue(cur.getColumnList("testtable",null));
		assertEquals(cur.getField(0,"extra"),"auto_increment");
		assertEquals(cur.getField(0,"column_key"),"PRI");
		assertEquals(cur.getField(1,"extra"),"");
		assertEquals(cur.getField(1,"column_key"),"");
		System.out.println();
		assertTrue(cur.sendQuery("drop table if exists testtable"));
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 int primary key, "+
			"	col2 int)"));
		assertTrue(cur.getColumnList("testtable",null));
		assertEquals(cur.getField(0,"extra"),"");
		assertEquals(cur.getField(0,"column_key"),"PRI");
		assertTrue(cur.sendQuery("drop table if exists testtable"));
		System.out.println();


		// primary keys list
		System.out.println("PRIMARY KEYS LIST: ");
		cur.sendQuery("drop table if exists testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 int primary key, "+
			"	col2 int)"));
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
		assertEquals(cur.getField(0,"table"),"testtable");
		assertEquals(cur.getField(0,"seq_in_index"),"1");
		assertEquals(cur.getField(0,"column_name"),"col1");
		assertTrue(cur.sendQuery("drop table if exists testtable"));
		System.out.println();


		// key and index list
		System.out.println("KEY AND INDEX LIST: ");
		cur.sendQuery("drop table if exists testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 int primary key, "+
			"	col2 int)"));
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
		assertEquals(cur.getField(0,"table"),"testtable");
		assertEquals(cur.getField(0,"non_unique"),"0");
		assertEquals(cur.getField(0,"seq_in_index"),"1");
		assertEquals(cur.getField(0,"column_name"),"col1");
		assertEquals(cur.getField(0,"collation"),"A");
		assertEquals(cur.getField(0,"index_type"),"3");
		assertEquals(cur.getField(0,"key_name"),"sqlite_autoindex_testtable_1");
		assertTrue(cur.sendQuery("drop table if exists testtable"));
		System.out.println();


		// procedure list
		System.out.println("PROCEDURE LIST: ");
		assertTrue(cur.getProcedureList(null));
		assertEquals(cur.rowCount(),0);
		System.out.println();


		// procedure parameter list
		System.out.println("PROCEDURE PARAMETER LIST: ");
		assertTrue(cur.getProcedureParameterList("testproc1",null));
		assertEquals(cur.getColumnName(0),"parameter_name");
		assertEquals(cur.getColumnName(1),"parameter_mode");
		assertEquals(cur.getColumnName(2),"data_type");
		assertEquals(cur.getColumnName(3),"character_maximum_length");
		assertEquals(cur.getColumnName(4),"ordinal_position");
		assertEquals(cur.rowCount(),0);
		System.out.println();


		// invalid queries
		System.out.println("INVALID QUERIES: ");
		assertFalse(cur.sendQuery("select * from testtable"));
		assertFalse(cur.sendQuery("select * from testtable"));
		assertFalse(cur.sendQuery("select * from testtable"));
		assertFalse(cur.sendQuery("select * from testtable"));
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
