// Copyright (c) David Muse
// See the file COPYING for more information.

import com.firstworks.sqlrelay.SQLRConnection;
import com.firstworks.sqlrelay.SQLRCursor;


class tls extends sqlrtest {

	public static void	main(String[] args) {

		String[]	isolationlevels={
					"READ COMMITTED","SERIALIZABLE"};
		String[]	bindvars={"1","2","3","4","5"};
		String[]	bindvals={"4","testchar4","testvarchar4",
						"01-JAN-2004","testlong4"};
		String[]	arraybindvars={"var1","var2","var3",
						"var4","var5"};
		String[]	arraybindvals={"7","testchar7","testvarchar7",
						"01-JAN-2007","testlong7"};
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
		String		clobvar;
		long		clobvarlength;
		byte[]		blobvar;
		long		blobvarlength;
		int		LARGE_BUFFER_LENGTH=8192;

		String	cert="../sqlrelay.conf.d/tls/client.pem";
		String	ca="../sqlrelay.conf.d/tls/ca.pem";
		if (System.getProperty("os.name").
			toLowerCase().indexOf("win")>=0) {
			cert="..\\sqlrelay.conf.d\\tls\\client.pfx";
			ca="..\\sqlrelay.conf.d\\tls\\ca.pfx";
		}


		// hostname
		String	hostname="";
		try {
			hostname=java.net.InetAddress
				.getLocalHost().getHostName();
			int idx=hostname.indexOf('.');
			if (idx>=0) {
				hostname=hostname.substring(0,idx);
			}
		} catch (Exception e) {
		}


		// instantiation
		SQLRConnection con=new SQLRConnection("sqlrelay",(short)9000,
						"/tmp/test.socket",
						null,null,0,1);
		SQLRCursor cur=new SQLRCursor(con);
		con.enableTls(null,cert,null,null,"ca",ca,(short)0);


		// identify
		System.out.println("IDENTIFY: ");
		assertEquals(con.identify(),"oracle");
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
		assertEquals(con.bindFormat(),":*");
		System.out.println();


		// nextval format
		System.out.println("NEXTVAL FORMAT: ");
		assertEquals(con.nextvalFormat(),"%s.nextval");
		System.out.println();


		// isolation levels
		System.out.println("ISOLATION LEVELS: ");
		for (String il : isolationlevels) {
			// oracle requires the isolation level to
			// be the first query of the transaction
			assertTrue(con.commit());
			// you can set the isolation level,
			// but to get it, you have to
			// have permissions to read from sys.v_$session and
			// sys.v_$transaction
			assertTrue(con.setIsolationLevel(il));
			System.out.println();
		}
		// reset to the default isolation level
		assertTrue(con.commit());
		assertTrue(con.setIsolationLevel(isolationlevels[0]));
		System.out.println();


		// create testtable
		System.out.println("CREATE TESTTABLE: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	testnumber number, "+
			"	testchar char(40), "+
			"	testvarchar varchar2(40), "+
			"	testdate date, "+
			"	testlong long, "+
			"	testclob clob, "+
			"	testblob blob)"));
		System.out.println();


		// insert
		System.out.println("INSERT: ");
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	1, "+
			"	'testchar1', "+
			"	'testvarchar1', "+
			"	'01-JAN-2001', "+
			"	'testlong1', "+
			"	'testclob1', "+
			"	empty_blob())"));
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
			"	:var1, "+
			"	:var2, "+
			"	:var3, "+
			"	:var4, "+
			"	:var5, "+
			"	:var6, "+
			"	:var7)");
		assertEquals(cur.countBindVariables(),7);
		cur.inputBind("1",2);
		cur.inputBind("2","testchar2");
		cur.inputBind("3","testvarchar2");
		cur.inputBind("4",(short)2002,(short)1,(short)1,
				(short)0,(short)0,(short)0,0,null,false);
		cur.inputBind("5","testlong2");
		cur.inputBindClob("6","testclob2",9);
		cur.inputBindBlob("7",(new String("testblob2")).getBytes(),9);
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("1",3);
		cur.inputBind("2","testchar3");
		cur.inputBind("3","testvarchar3");
		cur.inputBind("4",(short)2003,(short)1,(short)1,
				(short)0,(short)0,(short)0,0,null,false);
		cur.inputBind("5","testlong3");
		cur.inputBindClob("6","testclob3",9);
		cur.inputBindBlob("7",(new String("testblob3")).getBytes(),9);
		assertTrue(cur.executeQuery());
		System.out.println();


		// array of input binds by position
		System.out.println("ARRAY OF INPUT BINDS BY POSITION: ");
		cur.clearBinds();
		cur.inputBinds(bindvars,bindvals);
		cur.inputBindClob("6","testclob4",9);
		cur.inputBindBlob("7",(new String("testblob4")).getBytes(),9);
		assertTrue(cur.executeQuery());
		System.out.println();


		// input bind by position with validation
		System.out.println("INPUT BIND BY POSITION WITH VALIDATION: ");
		cur.clearBinds();
		cur.inputBind("1",5);
		cur.inputBind("2","testchar5");
		cur.inputBind("3","testvarchar5");
		cur.inputBind("4",(short)2005,(short)1,(short)1,
				(short)0,(short)0,(short)0,0,null,false);
		cur.inputBind("5","testlong5");
		cur.inputBindClob("6","testclob5",9);
		cur.inputBindBlob("7",(new String("testblob5")).getBytes(),9);
		cur.validateBinds();
		assertTrue(cur.executeQuery());
		cur.clearBinds();


		// input bind by name
		System.out.println("INPUT BIND BY NAME: ");
		cur.clearBinds();
		cur.inputBind("var1",6);
		cur.inputBind("var2","testchar6");
		cur.inputBind("var3","testvarchar6");
		cur.inputBind("var4",(short)2006,(short)1,(short)1,
				(short)0,(short)0,(short)0,0,null,false);
		cur.inputBind("var5","testlong6");
		cur.inputBindClob("var6","testclob6",9);
		cur.inputBindBlob("var7",
			(new String("testblob6")).getBytes(),9);
		assertTrue(cur.executeQuery());
		System.out.println();


		// array of input binds by name
		System.out.println("ARRAY OF INPUT BINDS BY NAME: ");
		cur.clearBinds();
		cur.inputBinds(arraybindvars,arraybindvals);
		cur.inputBindClob("var6","testclob7",9);
		cur.inputBindBlob("var7",
			(new String("testblob7")).getBytes(),9);
		assertTrue(cur.executeQuery());
		System.out.println();


		// input bind by name with validation
		System.out.println("INPUT BIND BY NAME WITH VALIDATION: ");
		cur.clearBinds();
		cur.inputBind("var1",8);
		cur.inputBind("var2","testchar8");
		cur.inputBind("var3","testvarchar8");
		cur.inputBind("var4",(short)2008,(short)1,(short)1,
				(short)0,(short)0,(short)0,0,null,false);
		cur.inputBind("var5","testlong8");
		cur.inputBindClob("var6","testclob8",9);
		cur.inputBindBlob("var7",
			(new String("testblob8")).getBytes(),9);
		cur.inputBind("var9","junkvalue");
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
			"	testnumber"));
		System.out.println();


		// column count
		System.out.println("COLUMN COUNT: ");
		assertEquals(cur.colCount(),7);
		System.out.println();


		// column names
		System.out.println("COLUMN NAMES: ");
		assertEquals(cur.getColumnName(0),"TESTNUMBER");
		assertEquals(cur.getColumnName(1),"TESTCHAR");
		assertEquals(cur.getColumnName(2),"TESTVARCHAR");
		assertEquals(cur.getColumnName(3),"TESTDATE");
		assertEquals(cur.getColumnName(4),"TESTLONG");
		assertEquals(cur.getColumnName(5),"TESTCLOB");
		assertEquals(cur.getColumnName(6),"TESTBLOB");
		cols=cur.getColumnNames();
		assertEquals(cols[0],"TESTNUMBER");
		assertEquals(cols[1],"TESTCHAR");
		assertEquals(cols[2],"TESTVARCHAR");
		assertEquals(cols[3],"TESTDATE");
		assertEquals(cols[4],"TESTLONG");
		assertEquals(cols[5],"TESTCLOB");
		assertEquals(cols[6],"TESTBLOB");
		System.out.println();


		// column types
		System.out.println("COLUMN TYPES: ");
		assertEquals(cur.getColumnType(0),"NUMBER");
		assertEquals(cur.getColumnType("TESTNUMBER"),"NUMBER");
		assertEquals(cur.getColumnType(1),"CHAR");
		assertEquals(cur.getColumnType("TESTCHAR"),"CHAR");
		assertEquals(cur.getColumnType(2),"VARCHAR2");
		assertEquals(cur.getColumnType("TESTVARCHAR"),"VARCHAR2");
		assertEquals(cur.getColumnType(3),"DATE");
		assertEquals(cur.getColumnType("TESTDATE"),"DATE");
		assertEquals(cur.getColumnType(4),"LONG");
		assertEquals(cur.getColumnType("TESTLONG"),"LONG");
		assertEquals(cur.getColumnType(5),"CLOB");
		assertEquals(cur.getColumnType("TESTCLOB"),"CLOB");
		assertEquals(cur.getColumnType(6),"BLOB");
		assertEquals(cur.getColumnType("TESTBLOB"),"BLOB");
		System.out.println();


		// column length
		System.out.println("COLUMN LENGTH: ");
		assertEquals(cur.getColumnLength(0),22);
		assertEquals(cur.getColumnLength("TESTNUMBER"),22);
		assertEquals(cur.getColumnLength(1),40);
		assertEquals(cur.getColumnLength("TESTCHAR"),40);
		assertEquals(cur.getColumnLength(2),40);
		assertEquals(cur.getColumnLength("TESTVARCHAR"),40);
		assertEquals(cur.getColumnLength(3),7);
		assertEquals(cur.getColumnLength("TESTDATE"),7);
		assertEquals(cur.getColumnLength(4),0);
		assertEquals(cur.getColumnLength("TESTLONG"),0);
		assertEquals(cur.getColumnLength(5),0);
		assertEquals(cur.getColumnLength("TESTCLOB"),0);
		assertEquals(cur.getColumnLength(6),0);
		assertEquals(cur.getColumnLength("TESTBLOB"),0);
		System.out.println();


		// longest column
		System.out.println("LONGEST COLUMN: ");
		assertEquals(cur.getLongest(0),1);
		assertEquals(cur.getLongest("TESTNUMBER"),1);
		assertEquals(cur.getLongest(1),40);
		assertEquals(cur.getLongest("TESTCHAR"),40);
		assertEquals(cur.getLongest(2),12);
		assertEquals(cur.getLongest("TESTVARCHAR"),12);
		assertEquals(cur.getLongest(3),9);
		assertEquals(cur.getLongest("TESTDATE"),9);
		assertEquals(cur.getLongest(4),9);
		assertEquals(cur.getLongest("TESTLONG"),9);
		assertEquals(cur.getLongest(5),9);
		assertEquals(cur.getLongest("TESTCLOB"),9);
		assertEquals(cur.getLongest(6),9);
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
		assertEquals(cur.getField(0,1),"testchar1"+
					"                               ");
		assertEquals(cur.getField(0,2),"testvarchar1");
		assertEquals(cur.getField(0,3),"01-JAN-01");
		assertEquals(cur.getField(0,4),"testlong1");
		assertEquals(cur.getField(0,5),"testclob1");
		assertEquals(cur.getField(0,6),"");
		System.out.println();
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(7,1),"testchar8"+
					"                               ");
		assertEquals(cur.getField(7,2),"testvarchar8");
		assertEquals(cur.getField(7,3),"01-JAN-08");
		assertEquals(cur.getField(7,4),"testlong8");
		assertEquals(cur.getField(7,5),"testclob8");
		assertEquals(cur.getField(7,6),"testblob8");
		System.out.println();


		// field lengths by index
		System.out.println("FIELD LENGTHS BY INDEX: ");
		assertEquals(cur.getFieldLength(0,0),1);
		assertEquals(cur.getFieldLength(0,1),40);
		assertEquals(cur.getFieldLength(0,2),12);
		assertEquals(cur.getFieldLength(0,3),9);
		assertEquals(cur.getFieldLength(0,4),9);
		assertEquals(cur.getFieldLength(0,5),9);
		assertEquals(cur.getFieldLength(0,6),0);
		System.out.println();
		assertEquals(cur.getFieldLength(7,0),1);
		assertEquals(cur.getFieldLength(7,1),40);
		assertEquals(cur.getFieldLength(7,2),12);
		assertEquals(cur.getFieldLength(7,3),9);
		assertEquals(cur.getFieldLength(7,4),9);
		assertEquals(cur.getFieldLength(7,5),9);
		assertEquals(cur.getFieldLength(7,6),9);
		System.out.println();


		// fields by name
		System.out.println("FIELDS BY NAME: ");
		assertEquals(cur.getField(0,"TESTNUMBER"),"1");
		assertEquals(cur.getField(0,"TESTCHAR"),"testchar1"+
					"                               ");
		assertEquals(cur.getField(0,"TESTVARCHAR"),"testvarchar1");
		assertEquals(cur.getField(0,"TESTDATE"),"01-JAN-01");
		assertEquals(cur.getField(0,"TESTLONG"),"testlong1");
		assertEquals(cur.getField(0,"TESTCLOB"),"testclob1");
		assertEquals(cur.getField(0,"TESTBLOB"),"");
		System.out.println();
		assertEquals(cur.getField(7,"TESTNUMBER"),"8");
		assertEquals(cur.getField(7,"TESTCHAR"),"testchar8"+
					"                               ");
		assertEquals(cur.getField(7,"TESTVARCHAR"),"testvarchar8");
		assertEquals(cur.getField(7,"TESTDATE"),"01-JAN-08");
		assertEquals(cur.getField(7,"TESTLONG"),"testlong8");
		assertEquals(cur.getField(7,"TESTCLOB"),"testclob8");
		assertEquals(cur.getField(7,"TESTBLOB"),"testblob8");
		System.out.println();


		// field lengths by name
		System.out.println("FIELD LENGTHS BY NAME: ");
		assertEquals(cur.getFieldLength(0,"TESTNUMBER"),1);
		assertEquals(cur.getFieldLength(0,"TESTCHAR"),40);
		assertEquals(cur.getFieldLength(0,"TESTVARCHAR"),12);
		assertEquals(cur.getFieldLength(0,"TESTDATE"),9);
		assertEquals(cur.getFieldLength(0,"TESTLONG"),9);
		assertEquals(cur.getFieldLength(0,"TESTCLOB"),9);
		assertEquals(cur.getFieldLength(0,"TESTBLOB"),0);
		System.out.println();
		assertEquals(cur.getFieldLength(7,"TESTNUMBER"),1);
		assertEquals(cur.getFieldLength(7,"TESTCHAR"),40);
		assertEquals(cur.getFieldLength(7,"TESTVARCHAR"),12);
		assertEquals(cur.getFieldLength(7,"TESTDATE"),9);
		assertEquals(cur.getFieldLength(7,"TESTLONG"),9);
		assertEquals(cur.getFieldLength(7,"TESTCLOB"),9);
		assertEquals(cur.getFieldLength(7,"TESTBLOB"),9);
		System.out.println();


		// fields by array
		System.out.println("FIELDS BY ARRAY: ");
		fields=cur.getRow(0);
		assertEquals(fields[0],"1");
		assertEquals(fields[1],"testchar1"+
					"                               ");
		assertEquals(fields[2],"testvarchar1");
		assertEquals(fields[3],"01-JAN-01");
		assertEquals(fields[4],"testlong1");
		assertEquals(fields[5],"testclob1");
		assertEquals(fields[6],"");
		System.out.println();


		// field lengths by array
		System.out.println("FIELD LENGTHS BY ARRAY: ");
		fieldlens=cur.getRowLengths(0);
		assertEquals(fieldlens[0],1);
		assertEquals(fieldlens[1],40);
		assertEquals(fieldlens[2],12);
		assertEquals(fieldlens[3],9);
		assertEquals(fieldlens[4],9);
		assertEquals(fieldlens[5],9);
		assertEquals(fieldlens[6],0);
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
			"	testnumber"));
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
			"	testnumber"));
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
			"	testnumber"));
		assertEquals(cur.getColumnName(0),"TESTNUMBER");
		assertEquals(cur.getColumnLength(0),22);
		assertEquals(cur.getColumnType(0),"NUMBER");
		System.out.println();


		// suspended session
		System.out.println("SUSPENDED SESSION: ");
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testnumber"));
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
			"	testnumber"));
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
			"	testnumber"));
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
			"	testnumber"));
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
			"	testnumber"));
		filename=cur.getCacheFileName();
		assertEquals(filename,"cachefile1");
		cur.cacheOff();
		assertTrue(cur.openCachedResultSet(filename));
		assertEquals(cur.getField(7,0),"8");
		System.out.println();


		// column count for cached result set
		System.out.println("COLUMN COUNT FOR CACHED RESULT SET: ");
		assertEquals(cur.colCount(),7);
		System.out.println();


		// column names for cached result set
		System.out.println("COLUMN NAMES FOR CACHED RESULT SET: ");
		assertEquals(cur.getColumnName(0),"TESTNUMBER");
		assertEquals(cur.getColumnName(1),"TESTCHAR");
		assertEquals(cur.getColumnName(2),"TESTVARCHAR");
		assertEquals(cur.getColumnName(3),"TESTDATE");
		assertEquals(cur.getColumnName(4),"TESTLONG");
		assertEquals(cur.getColumnName(5),"TESTCLOB");
		assertEquals(cur.getColumnName(6),"TESTBLOB");
		cols=cur.getColumnNames();
		assertEquals(cols[0],"TESTNUMBER");
		assertEquals(cols[1],"TESTCHAR");
		assertEquals(cols[2],"TESTVARCHAR");
		assertEquals(cols[3],"TESTDATE");
		assertEquals(cols[4],"TESTLONG");
		assertEquals(cols[5],"TESTCLOB");
		assertEquals(cols[6],"TESTBLOB");
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
			"	testnumber"));
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
			"	testnumber"));
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
			"	testnumber"));
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
				"select * from testtable"));
		}
		secondcur2.closeResultSet();
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
		SQLRConnection secondcon=new SQLRConnection("sqlrelay",
				(short)9000,"/tmp/test.socket",null,
				null,0,1);
		SQLRCursor secondcur=new SQLRCursor(secondcon);
		secondcon.enableTls(null,cert,null,null,"ca",ca,(short)0);
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
		cur.prepareQuery("select $(var1),'$(var2)',$(var3) from dual");
		cur.substitution("var1","$(var11)");
		cur.substitution("var2","$(var21)");
		cur.substitution("var3","$(var31)");
		cur.substitution("var11","$(var111)");
		cur.substitution("var21","$(var211)");
		cur.substitution("var31","$(var311)");
		cur.substitution("var111",1);
		cur.substitution("var211","hello");
		cur.substitution("var311",10.5556,6,4);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"hello");
		assertEquals(cur.getField(0,2),"10.5556");
		System.out.println();


		// array substitutions
		System.out.println("ARRAY SUBSTITUTIONS: ");
		cur.prepareQuery("select $(var1),$(var2),$(var3) from dual");
		cur.substitutions(subvars,subvallongs);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"2");
		assertEquals(cur.getField(0,2),"3");
		System.out.println();
		cur.prepareQuery(
			"select "+
			"	'$(var1)', "+
			"	'$(var2)', "+
			"	'$(var3)' "+
			"from "+
			"	dual");
		cur.substitutions(subvars,subvalstrings);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"hi");
		assertEquals(cur.getField(0,1),"hello");
		assertEquals(cur.getField(0,2),"bye");
		System.out.println();
		cur.prepareQuery("select $(var1),$(var2),$(var3) from dual");
		cur.substitutions(subvars,subvaldoubles,precs,scales);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"10.55");
		assertEquals(cur.getField(0,1),"10.556");
		assertEquals(cur.getField(0,2),"10.5556");
		System.out.println();


		// nulls as nulls
		System.out.println("NULLS AS NULLS: ");
		cur.getNullsAsNulls();
		assertTrue(cur.sendQuery("select NULL,1,NULL from dual"));
		assertEquals(cur.getField(0,0),null);
		assertEquals(cur.getField(0,1),"1");
		assertEquals(cur.getField(0,2),null);
		cur.getNullsAsEmptyStrings();
		assertTrue(cur.sendQuery("select NULL,1,NULL from dual"));
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
			"	:clobval, "+
			"	:blobval)");
		StringBuilder largebuffer=new StringBuilder();
		for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
			largebuffer.append('C');
		}
		cur.inputBindClob("clobval",largebuffer.toString(),
			LARGE_BUFFER_LENGTH);
		cur.inputBindBlob("blobval",(largebuffer.toString()).getBytes(),
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
		System.out.println();


		// output bind by position
		System.out.println("OUTPUT BIND BY POSITION: ");
		cur.getNullsAsNulls();
		cur.prepareQuery(
			"begin "+
			"	:numvar:=1; "+
			"	:stringvar:='hello'; "+
			"	:floatvar:=2.5; "+
			"	:datevar:='03-FEB-2001'; "+
			"	:nullvar:=null; "+
			"end;");
		assertEquals(cur.countBindVariables(),5);
		cur.defineOutputBindInteger("1");
		cur.defineOutputBindString("2",10);
		cur.defineOutputBindDouble("3");
		cur.defineOutputBindDate("4");
		cur.defineOutputBindString("5",10);
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
		System.out.println();


		// output bind by name
		System.out.println("OUTPUT BIND BY NAME: ");
		cur.getNullsAsNulls();
		cur.clearBinds();
		cur.defineOutputBindInteger("numvar");
		cur.defineOutputBindString("stringvar",10);
		cur.defineOutputBindDouble("floatvar");
		cur.defineOutputBindDate("datevar");
		cur.defineOutputBindString("nullvar",10);
		assertTrue(cur.executeQuery());
		numvar=cur.getOutputBindInteger("numvar");
		stringvar=cur.getOutputBindString("stringvar");
		floatvar=cur.getOutputBindDouble("floatvar");
		year=cur.getOutputBindDateYear("datevar");
		month=cur.getOutputBindDateMonth("datevar");
		day=cur.getOutputBindDateDay("datevar");
		hour=cur.getOutputBindDateHour("datevar");
		minute=cur.getOutputBindDateMinute("datevar");
		second=cur.getOutputBindDateSecond("datevar");
		microsecond=cur.getOutputBindDateMicrosecond("datevar");
		tz=cur.getOutputBindDateTz("datevar");
		isnegative=cur.getOutputBindDateIsNegative("datevar");
		nullvar=cur.getOutputBindString("nullvar");
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
		System.out.println();


		// output bind by name with validation
		System.out.println("OUTPUT BIND BY NAME WITH VALIDATION: ");
		cur.getNullsAsNulls();
		cur.clearBinds();
		cur.defineOutputBindInteger("numvar");
		cur.defineOutputBindString("stringvar",10);
		cur.defineOutputBindDouble("floatvar");
		cur.defineOutputBindDate("datevar");
		cur.defineOutputBindString("nullvar",10);
		cur.defineOutputBindString("dummyvar",10);
		cur.validateBinds();
		assertTrue(cur.executeQuery());
		numvar=cur.getOutputBindInteger("numvar");
		stringvar=cur.getOutputBindString("stringvar");
		floatvar=cur.getOutputBindDouble("floatvar");
		year=cur.getOutputBindDateYear("datevar");
		month=cur.getOutputBindDateMonth("datevar");
		day=cur.getOutputBindDateDay("datevar");
		hour=cur.getOutputBindDateHour("datevar");
		minute=cur.getOutputBindDateMinute("datevar");
		second=cur.getOutputBindDateSecond("datevar");
		microsecond=cur.getOutputBindDateMicrosecond("datevar");
		tz=cur.getOutputBindDateTz("datevar");
		isnegative=cur.getOutputBindDateIsNegative("datevar");
		nullvar=cur.getOutputBindString("nullvar");
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
		System.out.println();


		// lob output bind
		System.out.println("LOB OUTPUT BIND: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	testclob clob, "+
			"	testblob blob)"));
		cur.prepareQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	'hello', "+
			"	:var1)");
		cur.inputBindBlob("var1",(new String("hello")).getBytes(),5);
		assertTrue(cur.executeQuery());
		cur.prepareQuery(
			"begin "+
			"	select testclob "+
			"		into :clobvar "+
			"		from testtable; "+
			"	select testblob "+
			"		into :blobvar "+
			"		from testtable; "+
			"end;");
		cur.defineOutputBindClob("clobvar");
		cur.defineOutputBindBlob("blobvar");
		assertTrue(cur.executeQuery());
		clobvar=cur.getOutputBindClob("clobvar");
		clobvarlength=cur.getOutputBindLength("clobvar");
		blobvar=cur.getOutputBindBlob("blobvar");
		blobvarlength=cur.getOutputBindLength("blobvar");
		assertEquals(clobvar,"hello",5);
		assertEquals(clobvarlength,5);
		assertEquals(blobvar,"hello",5);
		assertEquals(blobvarlength,5);
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// long output bind
		System.out.println("LONG OUTPUT BIND: ");
		StringBuilder lobuf=new StringBuilder();
		for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
			lobuf.append('C');
		}
		StringBuilder loquery=new StringBuilder();
		loquery.append("begin :bindval:='");
		loquery.append(lobuf.toString());
		loquery.append("'; end;");
		cur.prepareQuery(loquery.toString());
		cur.defineOutputBindString("bindval",LARGE_BUFFER_LENGTH);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindLength("bindval"),
						LARGE_BUFFER_LENGTH);
		assertEquals(cur.getOutputBindString("bindval"),
						lobuf.toString());
		System.out.println();


		// negative input bind
		System.out.println("NEGATIVE INPUT BIND: ");
		cur.sendQuery("drop table testtable");
		cur.sendQuery("create table testtable (testval number)");
		cur.prepareQuery("insert into testtable values (:testval)");
		cur.inputBind("testval",-1);
		assertTrue(cur.executeQuery());
		cur.sendQuery("select testval from testtable");
		assertEquals(cur.getField(0,"TESTVAL"),"-1");
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// bind validation
		System.out.println("BIND VALIDATION: ");
		cur.sendQuery("drop table testtable");
		cur.sendQuery(
			"create table testtable ("+
			"	col1 varchar2(20), "+
			"	col2 varchar2(20), "+
			"	col3 varchar2(20))");
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
		cur.prepareQuery(
			"begin "+
			"	:out:= :in; "+
			"end;");
		cur.inputBind("in",1);
		cur.defineOutputBindInteger("out");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("out"),1);
		cur.inputBind("in",2);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("out"),2);
		cur.inputBind("in",3);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("out"),3);
		System.out.println();


		// reexecute
		System.out.println("REEXECUTE: ");
		cur.prepareQuery("select 1 from dual");
		assertTrue(cur.executeQuery());
		assertEquals(cur.rowCount(),1);
		assertEquals(cur.getField(0,0),"1");
		System.out.println();
		assertTrue(cur.executeQuery());
		assertEquals(cur.rowCount(),1);
		assertEquals(cur.getField(0,0),"1");
		System.out.println();
		cur.prepareQuery("select :var from dual");
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
		System.out.println("STORED PROCEDURE RETURNING NO VALUE: ");
		cur.sendQuery("drop function testproc");
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery("create or replace "+
			"procedure testproc("+
			"	in1 in number, "+
			"	in2 in number, "+
			"	in3 in varchar2) "+
			"is begin "+
			"	return; "+
			"end;"));
		cur.prepareQuery("begin testproc(:in1,:in2,:in3); end;");
		cur.inputBind("in1",1);
		cur.inputBind("in2",2.5,2,1);
		cur.inputBind("in3","hello");
		assertTrue(cur.executeQuery());
		assertTrue(cur.sendQuery("drop procedure testproc"));
		System.out.println();


		// stored procedure returning single value
		System.out.println("STORED PROCEDURE RETURNING SINGLE VALUE: ");
		cur.sendQuery("drop function testproc");
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery(
			"create or replace function testproc("+
			"	in1 in number, "+
			"	in2 in number, "+
			"	in3 in varchar2) "+
			"	return number "+
			"is begin "+
			"	return in1; "+
			"end;"));
		cur.prepareQuery("select testproc(:in1,:in2,:in3) from dual");
		cur.inputBind("in1",1);
		cur.inputBind("in2",2.5,2,1);
		cur.inputBind("in3","hello");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"1");
		cur.prepareQuery(
			"begin "+
			"	:out1:=testproc(:in1,:in2,:in3); "+
			"end;");
		cur.inputBind("in1",1);
		cur.inputBind("in2",2.5,2,1);
		cur.inputBind("in3","hello");
		cur.defineOutputBindInteger("out1");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("out1"),1);
		assertTrue(cur.sendQuery("drop function testproc"));
		System.out.println();


		// stored procedure returning multiple values
		System.out.println("STORED PROCEDURE RETURNING MULTIPLE "+
					"VALUES: ");
		cur.sendQuery("drop function testproc");
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery("create or replace "+
			"procedure testproc("+
			"	in1 in number, "+
			"	in2 in number, "+
			"	in3 in varchar2, "+
			"	out1 out number, "+
			"	out2 out number, "+
			"	out3 out varchar2) "+
			"is begin "+
			"	out1:=in1; "+
			"	out2:=in2; "+
			"	out3:=in3; "+
			"end;"));
		cur.prepareQuery(
			"begin "+
			"	testproc(:in1,:in2,:in3,"+
			":out1,:out2,:out3); end;");
		cur.inputBind("in1",1);
		cur.inputBind("in2",2.5,2,1);
		cur.inputBind("in3","hello");
		cur.defineOutputBindInteger("out1");
		cur.defineOutputBindDouble("out2");
		cur.defineOutputBindString("out3",20);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("out1"),1);
		assertEquals(cur.getOutputBindDouble("out2"),2.5);
		assertEquals(cur.getOutputBindString("out3"),"hello");
		assertTrue(cur.sendQuery("drop procedure testproc"));
		System.out.println();


		// stored procedure returning result set
		System.out.println("STORED PROCEDURE RETURNING RESULT SET: ");
		cur.sendQuery("drop package types");
		cur.sendQuery("drop function testproc");
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery(
			"create or replace package types is "+
			"	type cursorType "+
			"is "+
			"	ref cursor; "+
			"end;"));
		assertTrue(cur.sendQuery(
			"create or replace function testproc(value in number) "+
			"	return types.cursortype "+
			"is "+
			"	l_cursor	types.cursorType; "+
			"begin "+
			"	open l_cursor for "+
			"		select "+
			"			* "+
			"		from "+
			"			( "+
			"			select 1 as "+
			"				testnumber from dual "+
			"			union "+
			"			select 2 as "+
			"				testnumber from dual "+
			"			union "+
			"			select 3 as "+
			"				testnumber from dual "+
			"			union "+
			"			select 4 as "+
			"				testnumber from dual "+
			"			union "+
			"			select 5 as "+
			"				testnumber from dual "+
			"			union "+
			"			select 6 as "+
			"				testnumber from dual "+
			"			union "+
			"			select 7 as "+
			"				testnumber from dual "+
			"			union "+
			"			select 8 as "+
			"				testnumber from dual "+
			"			) "+
			"		where "+
			"			testnumber>value; "+
			"	return l_cursor; "+
			"end;"));
		cur.prepareQuery(
			"begin "+
			"	:curs1:=testproc(5); "+
			"	:curs2:=testproc(0); "+
			"end;");
		cur.defineOutputBindCursor("curs1");
		cur.defineOutputBindCursor("curs2");
		assertTrue(cur.executeQuery());
		SQLRCursor bindcur1=cur.getOutputBindCursor("curs1");
		assertTrue(bindcur1.fetchFromBindCursor());
		assertEquals(bindcur1.getField(0,0),"6");
		assertEquals(bindcur1.getField(1,0),"7");
		assertEquals(bindcur1.getField(2,0),"8");
		SQLRCursor bindcur2=cur.getOutputBindCursor("curs2");
		assertTrue(bindcur2.fetchFromBindCursor());
		assertEquals(bindcur2.getField(0,0),"1");
		assertEquals(bindcur2.getField(1,0),"2");
		assertEquals(bindcur2.getField(2,0),"3");
		assertTrue(cur.sendQuery("drop function testproc"));
		assertTrue(cur.sendQuery("drop package types"));
		System.out.println();


		// temporary tables
		System.out.println("TEMPORARY TABLES: ");
		cur.prepareQuery("drop table $(HOSTNAME)_temptabledelete");
		cur.substitution("HOSTNAME",hostname);
		cur.executeQuery();
		cur.prepareQuery(
			"create global temporary table "+
			"$(HOSTNAME)_temptabledelete ( "+
			"	col1 number "+
			") on commit delete rows");
		cur.substitution("HOSTNAME",hostname);
		cur.executeQuery();
		cur.prepareQuery(
			"insert into "+
			"	$(HOSTNAME)_temptabledelete "+
			"values ("+
			"	1)");
		cur.substitution("HOSTNAME",hostname);
		assertTrue(cur.executeQuery());
		cur.prepareQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	$(HOSTNAME)_temptabledelete");
		cur.substitution("HOSTNAME",hostname);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"1");
		assertTrue(con.commit());
		cur.prepareQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	$(HOSTNAME)_temptabledelete");
		cur.substitution("HOSTNAME",hostname);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"0");
		cur.prepareQuery("drop table $(HOSTNAME)_temptabledelete");
		cur.substitution("HOSTNAME",hostname);
		cur.executeQuery();
		System.out.println();
		cur.prepareQuery(
			"truncate table $(HOSTNAME)_temptablepreserve");
		cur.substitution("HOSTNAME",hostname);
		cur.executeQuery();
		cur.prepareQuery("drop table $(HOSTNAME)_temptablepreserve");
		cur.substitution("HOSTNAME",hostname);
		cur.executeQuery();
		cur.prepareQuery(
			"create global temporary table "+
			"$(HOSTNAME)_temptablepreserve ("+
			"	col1 number "+
			") on commit preserve rows");
		cur.substitution("HOSTNAME",hostname);
		cur.executeQuery();
		cur.prepareQuery(
			"insert into "+
			"	$(HOSTNAME)_temptablepreserve "+
			"values ("+
			"	1)");
		cur.substitution("HOSTNAME",hostname);
		assertTrue(cur.executeQuery());
		cur.prepareQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	$(HOSTNAME)_temptablepreserve");
		cur.substitution("HOSTNAME",hostname);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"1");
		assertTrue(con.commit());
		cur.prepareQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	$(HOSTNAME)_temptablepreserve");
		cur.substitution("HOSTNAME",hostname);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"1");
		con.endSession();
		System.out.println();
		cur.prepareQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	$(HOSTNAME)_temptablepreserve");
		cur.substitution("HOSTNAME",hostname);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"0");
		cur.prepareQuery(
			"truncate table $(HOSTNAME)_temptablepreserve");
		cur.substitution("HOSTNAME",hostname);
		assertTrue(cur.executeQuery());
		try {
			Thread.sleep(2000);
		} catch (Exception e) {
		}
		cur.prepareQuery("drop table $(HOSTNAME)_temptablepreserve");
		cur.substitution("HOSTNAME",hostname);
		assertTrue(cur.executeQuery());
		cur.prepareQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	$(HOSTNAME)_temptablepreserve");
		cur.substitution("HOSTNAME",hostname);
		assertFalse(cur.executeQuery());
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
		querystr.append("insert into testtable values ('");
		for (int i=0; i<buffer.length; i++) {
			querystr.append(String.format("%02x",buffer[i] & 0xff));
		}
		querystr.append("')");
		assertTrue(cur.sendQuery(querystr.toString()));
		assertTrue(cur.sendQuery("select col1 from testtable"));
		assertEquals(cur.getFieldLength(0,0),buffer.length);
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// quotes
		System.out.println("QUOTES: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 varchar2(4))"));
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
		// oracle doesn't support auto-increment


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
		assertInResultSet(cur,"Database",hostname.toUpperCase());
		System.out.println();


		// table type list
		System.out.println("TABLE TYPE LIST: ");
		assertTrue(cur.getTableTypeList());
		assertEquals(cur.getColumnName(0),"table_type");
		assertEquals(cur.getField(0,"table_type"),"SYNONYM");
		assertEquals(cur.getField(1,"table_type"),"TABLE");
		assertEquals(cur.getField(2,"table_type"),"VIEW");
		System.out.println();


		// table list
		System.out.println("TABLE LIST: ");
		cur.sendQuery("drop table testtable1");
		cur.sendQuery("drop table testtable2");
		cur.sendQuery("drop table testtable3");
		cur.sendQuery("drop table testtable4");
		assertTrue(cur.sendQuery(
			"create table testtable1 ("+
			"	testnumber number, "+
			"	testchar char(40), "+
			"	testvarchar varchar2(40), "+
			"	testdate date, "+
			"	testlong long, "+
			"	testclob clob, "+
			"	testblob blob)"));
		assertTrue(cur.sendQuery(
			"create table testtable2 ("+
			"	testnumber number, "+
			"	testchar char(40), "+
			"	testvarchar varchar2(40), "+
			"	testdate date, "+
			"	testlong long, "+
			"	testclob clob, "+
			"	testblob blob)"));
		assertTrue(cur.sendQuery(
			"create table testtable3 ("+
			"	testnumber number, "+
			"	testchar char(40), "+
			"	testvarchar varchar2(40), "+
			"	testdate date, "+
			"	testlong long, "+
			"	testclob clob, "+
			"	testblob blob)"));
		assertTrue(cur.sendQuery(
			"create table testtable4 ("+
			"	testnumber number, "+
			"	testchar char(40), "+
			"	testvarchar varchar2(40), "+
			"	testdate date, "+
			"	testlong long, "+
			"	testclob clob, "+
			"	testblob blob)"));
		assertTrue(cur.getTableList(null));
		assertInResultSet(cur,"Tables_in_xxx","TESTTABLE1");
		assertInResultSet(cur,"Tables_in_xxx","TESTTABLE2");
		assertInResultSet(cur,"Tables_in_xxx","TESTTABLE3");
		assertInResultSet(cur,"Tables_in_xxx","TESTTABLE4");
		assertTrue(cur.sendQuery("drop table testtable1"));
		assertTrue(cur.sendQuery("drop table testtable2"));
		assertTrue(cur.sendQuery("drop table testtable3"));
		assertTrue(cur.sendQuery("drop table testtable4"));
		System.out.println();


		// type info list
		System.out.println("TYPE INFO LIST: ");
		assertTrue(cur.getTypeInfoList("number"));
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
		assertEquals(cur.getField(0,"type_name"),"NUMBER");
		assertEquals(cur.getField(0,"data_type"),"-7");
		assertEquals(cur.getField(0,"precision"),"1");
		assertEquals(cur.getField(0,"local_type_name"),"NUMBER");
		assertTrue(cur.getTypeInfoList("char"));
		assertEquals(cur.getField(0,"type_name"),"CHAR");
		assertEquals(cur.getField(0,"data_type"),"1");
		assertEquals(cur.getField(0,"precision"),"2000");
		assertEquals(cur.getField(0,"local_type_name"),"CHAR");
		assertTrue(cur.getTypeInfoList("varchar2"));
		assertEquals(cur.getField(0,"type_name"),"VARCHAR2");
		assertEquals(cur.getField(0,"data_type"),"12");
		assertEquals(cur.getField(0,"precision"),"32767");
		assertEquals(cur.getField(0,"local_type_name"),"VARCHAR2");
		assertTrue(cur.getTypeInfoList("date"));
		assertEquals(cur.getField(0,"type_name"),"DATE");
		assertEquals(cur.getField(0,"data_type"),"92");
		assertEquals(cur.getField(0,"precision"),"7");
		assertEquals(cur.getField(0,"local_type_name"),"DATE");
		System.out.println();


		// column list
		System.out.println("COLUMN LIST: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	testnumber number, "+
			"	testchar char(40), "+
			"	testvarchar varchar2(40), "+
			"	testdate date, "+
			"	testlong long, "+
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
		assertEquals(cur.getField(0,"column_name"),"TESTNUMBER");
		assertEquals(cur.getField(1,"column_name"),"TESTCHAR");
		assertEquals(cur.getField(2,"column_name"),"TESTVARCHAR");
		assertEquals(cur.getField(3,"column_name"),"TESTDATE");
		assertEquals(cur.getField(4,"column_name"),"TESTLONG");
		assertEquals(cur.getField(5,"column_name"),"TESTCLOB");
		assertEquals(cur.getField(6,"column_name"),"TESTBLOB");
		assertEquals(cur.getField(0,"data_type"),"NUMBER");
		assertEquals(cur.getField(1,"data_type"),"CHAR");
		assertEquals(cur.getField(2,"data_type"),"VARCHAR2");
		assertEquals(cur.getField(3,"data_type"),"DATE");
		assertEquals(cur.getField(4,"data_type"),"LONG");
		assertEquals(cur.getField(5,"data_type"),"CLOB");
		assertEquals(cur.getField(6,"data_type"),"BLOB");
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// column list - auto_increment, primary key
		// oracle doesn't support auto_increment
		System.out.println("COLUMN LIST - auto_increment, primary "+
					"key: ");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 number primary key, "+
			"	col2 number)"));
		assertTrue(cur.getColumnList("testtable",null));
		assertTrue(cur.getField(0,"column_key").contains("PRI"));
		assertFalse(cur.getField(1,"column_key").contains("PRI"));
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// primary keys list
		System.out.println("PRIMARY KEYS LIST: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 number primary key, "+
			"	col2 number)"));
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
		System.out.println();


		// key and index list
		System.out.println("KEY AND INDEX LIST: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 number primary key, "+
			"	col2 number)"));
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
		System.out.println();


		// procedure list
		System.out.println("PROCEDURE LIST: ");
		cur.sendQuery("drop procedure testproc1");
		cur.sendQuery("drop procedure testproc2");
		cur.sendQuery("drop procedure testproc3");
		cur.sendQuery("drop procedure testproc4");
		assertTrue(cur.sendQuery(
			"create procedure testproc1("+
			"	in1 in number, "+
			"	in2 in char, "+
			"	in3 in varchar2, "+
			"	in4 in date) as "+
			"begin "+
			"	null; "+
			"end;"));
		assertTrue(cur.sendQuery(
			"create procedure testproc2("+
			"	in1 in number, "+
			"	in2 in char, "+
			"	in3 in varchar2, "+
			"	in4 in date) as "+
			"begin "+
			"	null; "+
			"end;"));
		assertTrue(cur.sendQuery(
			"create procedure testproc3("+
			"	in1 in number, "+
			"	in2 in char, "+
			"	in3 in varchar2, "+
			"	in4 in date) as "+
			"begin "+
			"	null; "+
			"end;"));
		assertTrue(cur.sendQuery(
			"create procedure testproc4("+
			"	in1 in number, "+
			"	in2 in char, "+
			"	in3 in varchar2, "+
			"	in4 in date) as "+
			"begin "+
			"	null; "+
			"end;"));
		assertTrue(cur.getProcedureList(null));
		assertInResultSet(cur,"routine_name","TESTPROC1");
		assertInResultSet(cur,"routine_name","TESTPROC2");
		assertInResultSet(cur,"routine_name","TESTPROC3");
		assertInResultSet(cur,"routine_name","TESTPROC4");
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
		assertEquals(cur.getField(0,"data_type"),"NUMBER");
		assertEquals(cur.getField(0,"ordinal_position"),"1");
		assertEquals(cur.getField(1,"parameter_name"),"IN2");
		assertEquals(cur.getField(1,"parameter_mode"),"1");
		assertEquals(cur.getField(1,"data_type"),"CHAR");
		assertEquals(cur.getField(1,"ordinal_position"),"2");
		assertEquals(cur.getField(2,"parameter_name"),"IN3");
		assertEquals(cur.getField(2,"parameter_mode"),"1");
		assertEquals(cur.getField(2,"data_type"),"VARCHAR2");
		assertEquals(cur.getField(2,"ordinal_position"),"3");
		assertEquals(cur.getField(3,"parameter_name"),"IN4");
		assertEquals(cur.getField(3,"parameter_mode"),"1");
		assertEquals(cur.getField(3,"data_type"),"DATE");
		assertEquals(cur.getField(3,"ordinal_position"),"4");
		assertTrue(cur.sendQuery("drop procedure testproc1"));
		assertTrue(cur.sendQuery("drop procedure testproc2"));
		assertTrue(cur.sendQuery("drop procedure testproc3"));
		assertTrue(cur.sendQuery("drop procedure testproc4"));
		System.out.println();


		// invalid queries
		System.out.println("INVALID QUERIES: ");
		assertFalse(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testnumber"));
		assertFalse(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testnumber"));
		assertFalse(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testnumber"));
		assertFalse(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testnumber"));
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
