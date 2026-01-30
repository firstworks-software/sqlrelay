// Copyright (c) David Muse
// See the file COPYING for more information.

import com.firstworks.sqlrelay.SQLRConnection;
import com.firstworks.sqlrelay.SQLRCursor;


class tls extends sqlrtest {
	
	public static void	main(String[] args) {
	
		String	dbtype;
		String[]	bindvars={"1","2","3","4","5"};
		String[]	bindvals={"4","testchar4","testvarchar4","01-JAN-2004","testlong4"};
		String[]	subvars={"var1","var2","var3"};
		String[]	subvalstrings={"hi","hello","bye"};
		long[]	subvallongs={1,2,3};
		double[]	subvaldoubles={10.55,10.556,10.5556};
		int[]	precs={4,5,6};
		int[]	scales={2,3,4};
		String	clobvar;
		long	clobvarlength;
		byte[]	blobvar;
		long	blobvarlength;
		long	numvar;
		String	stringvar;
		double	floatvar;
		String[]	cols;
		String[]	fields;
		short	port;
		String	socket;
		short	id;
		String	filename;
		String[]	arraybindvars={"var1","var2","var3","var4","var5"};
		String[]	arraybindvals={"7","testchar7","testvarchar7","01-JAN-2007","testlong7"};
		long[]	fieldlens;
	
		String		cert="../sqlrelay.conf.d/tls/client.pem";
		String		ca="../sqlrelay.conf.d/tls/ca.pem";
		if (System.getProperty("os.name").
			toLowerCase().indexOf("win")>=0) {
			cert="..\\sqlrelay.conf.d\\tls\\client.pfx";
			ca="..\\sqlrelay.conf.d\\tls\\ca.pfx";
		}
	
	
		// instantiation
		SQLRConnection con=new SQLRConnection("sqlrelay",
						(short)9000,
						"/tmp/test.socket",
						null,null,0,1);
		SQLRCursor cur=new SQLRCursor(con);
		con.enableTls(null,cert,null,null,"ca",ca,(short)0);
	
		// get database type


		// identify
		System.out.println("IDENTIFY: ");
		assertEquals(con.identify(),"oracle");
		System.out.println();


		// ping
		System.out.println("PING: ");
		assertTrue(con.ping());
		System.out.println();


		// bind validation
		System.out.println("BIND VALIDATION: ");
		cur.sendQuery("drop table testtable1");
		cur.sendQuery("create table testtable1 (col1 varchar2(20), col2 varchar2(20), col3 varchar2(20))");
		cur.prepareQuery("insert into testtable1 values ($(var1),$(var2),$(var3))");
		cur.inputBind("var1",1);
		cur.inputBind("var2",2);
		cur.inputBind("var3",3);
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
		cur.sendQuery("drop table testtable1");
		System.out.println();
	
		// drop existing table
		cur.sendQuery("drop table testtable");


		// create temptable
		System.out.println("CREATE TEMPTABLE: ");
		assertTrue(cur.sendQuery("create table testtable (testnumber number, testchar char(40), testvarchar varchar2(40), testdate date, testlong long, testclob clob, testblob blob)"));
		System.out.println();


		// insert
		System.out.println("INSERT: ");
		assertTrue(cur.sendQuery("insert into testtable values (1,'testchar1','testvarchar1','01-JAN-2001','testlong1','testclob1',empty_blob())"));
		System.out.println();


		// affected rows
		System.out.println("AFFECTED ROWS: ");
		assertEquals(cur.affectedRows(),1);
		System.out.println();


		// bind by position
		System.out.println("BIND BY POSITION: ");
		cur.prepareQuery("insert into testtable values (:var1,:var2,:var3,:var4,:var5,:var6,:var7)");
		assertEquals(cur.countBindVariables(),7);
		cur.inputBind("1",2);
		cur.inputBind("2","testchar2");
		cur.inputBind("3","testvarchar2");
		cur.inputBind("4","01-JAN-2002");
		cur.inputBind("5","testlong2");
		cur.inputBindClob("6","testclob2",9);
		cur.inputBindBlob("7",(new String("testblob2")).getBytes(),9);
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("1",3);
		cur.inputBind("2","testchar3");
		cur.inputBind("3","testvarchar3");
		cur.inputBind("4","01-JAN-2003");
		cur.inputBind("5","testlong3");
		cur.inputBindClob("6","testclob3",9);
		cur.inputBindBlob("7",(new String("testblob3")).getBytes(),9);
		assertTrue(cur.executeQuery());
		System.out.println();


		// array of binds by position
		System.out.println("ARRAY OF BINDS BY POSITION: ");
		cur.clearBinds();
		cur.inputBinds(bindvars,bindvals);
		cur.inputBindClob("var6","testclob4",9);
		cur.inputBindBlob("var7",
				(new String("testblob4")).getBytes(),9);
		assertTrue(cur.executeQuery());
		System.out.println();


		// bind by name
		System.out.println("BIND BY NAME: ");
		cur.prepareQuery("insert into testtable values (:var1,:var2,:var3,:var4,:var5,:var6,:var7)");
		cur.inputBind("var1",5);
		cur.inputBind("var2","testchar5");
		cur.inputBind("var3","testvarchar5");
		cur.inputBind("var4","01-JAN-2005");
		cur.inputBind("var5","testlong5");
		cur.inputBindClob("var6","testclob5",9);
		cur.inputBindBlob("var7",
				(new String("testblob5")).getBytes(),9);
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("var1",6);
		cur.inputBind("var2","testchar6");
		cur.inputBind("var3","testvarchar6");
		cur.inputBind("var4","01-JAN-2006");
		cur.inputBind("var5","testlong6");
		cur.inputBindClob("var6","testclob6",9);
		cur.inputBindBlob("var7",
				(new String("testblob6")).getBytes(),9);
		assertTrue(cur.executeQuery());
		System.out.println();


		// array of binds by name
		System.out.println("ARRAY OF BINDS BY NAME: ");
		cur.clearBinds();
		cur.inputBinds(arraybindvars,arraybindvals);
		cur.inputBindClob("var6","testclob7",9);
		cur.inputBindBlob("var7",
				(new String("testblob7")).getBytes(),9);
		assertTrue(cur.executeQuery());
		System.out.println();


		// bind by name with validation
		System.out.println("BIND BY NAME WITH VALIDATION: ");
		cur.clearBinds();
		cur.inputBind("var1",8);
		cur.inputBind("var2","testchar8");
		cur.inputBind("var3","testvarchar8");
		cur.inputBind("var4","01-JAN-2008");
		cur.inputBind("var5","testlong8");
		cur.inputBindClob("var6","testclob8",9);
		cur.inputBindBlob("var7",
				(new String("testblob8")).getBytes(),9);
		cur.inputBind("var9","junkvalue");
		cur.validateBinds();
		assertTrue(cur.executeQuery());
		System.out.println();


		// output bind by name
		System.out.println("OUTPUT BIND BY NAME: ");
		cur.prepareQuery("begin  :numvar:=1; :stringvar:='hello'; :floatvar:=2.5; end;");
		cur.defineOutputBindInteger("numvar");
		cur.defineOutputBindString("stringvar",10);
		cur.defineOutputBindDouble("floatvar");
		assertTrue(cur.executeQuery());
		numvar=cur.getOutputBindInteger("numvar");
		stringvar=cur.getOutputBindString("stringvar");
		floatvar=cur.getOutputBindDouble("floatvar");
		assertEquals(numvar,1);
		assertEquals(stringvar,"hello");
		assertEquals(floatvar,2.5);
		System.out.println();


		// output bind by position
		System.out.println("OUTPUT BIND BY POSITION: ");
		cur.clearBinds();
		cur.defineOutputBindInteger("1");
		cur.defineOutputBindString("2",10);
		cur.defineOutputBindDouble("3");
		assertTrue(cur.executeQuery());
		numvar=cur.getOutputBindInteger("1");
		stringvar=cur.getOutputBindString("2");
		floatvar=cur.getOutputBindDouble("3");
		assertEquals(numvar,1);
		assertEquals(stringvar,"hello");
		assertEquals(floatvar,2.5);
		System.out.println();


		// output bind by name with validation
		System.out.println("OUTPUT BIND BY NAME WITH VALIDATION: ");
		cur.clearBinds();
		cur.defineOutputBindInteger("numvar");
		cur.defineOutputBindString("stringvar",10);
		cur.defineOutputBindDouble("floatvar");
		cur.defineOutputBindString("dummyvar",10);
		cur.validateBinds();
		assertTrue(cur.executeQuery());
		numvar=cur.getOutputBindInteger("numvar");
		stringvar=cur.getOutputBindString("stringvar");
		floatvar=cur.getOutputBindDouble("floatvar");
		assertEquals(numvar,1);
		assertEquals(stringvar,"hello");
		assertEquals(floatvar,2.5);
		System.out.println();


		// select
		System.out.println("SELECT: ");
		assertTrue(cur.sendQuery("select * from testtable order by testnumber"));
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
		assertEquals(cur.getField(0,1),"testchar1                               ");
		assertEquals(cur.getField(0,2),"testvarchar1");
		assertEquals(cur.getField(0,3),"01-JAN-01");
		assertEquals(cur.getField(0,4),"testlong1");
		assertEquals(cur.getField(0,5),"testclob1");
		assertEquals(cur.getField(0,6),"");
		System.out.println();
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(7,1),"testchar8                               ");
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
		assertEquals(cur.getField(0,"TESTCHAR"),"testchar1                               ");
		assertEquals(cur.getField(0,"TESTVARCHAR"),"testvarchar1");
		assertEquals(cur.getField(0,"TESTDATE"),"01-JAN-01");
		assertEquals(cur.getField(0,"TESTLONG"),"testlong1");
		assertEquals(cur.getField(0,"TESTCLOB"),"testclob1");
		assertEquals(cur.getField(0,"TESTBLOB"),"");
		System.out.println();
		assertEquals(cur.getField(7,"TESTNUMBER"),"8");
		assertEquals(cur.getField(7,"TESTCHAR"),"testchar8                               ");
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
		assertEquals(fields[1],"testchar1                               ");
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


		// individual substitutions
		System.out.println("INDIVIDUAL SUBSTITUTIONS: ");
		cur.prepareQuery("select $(var1),'$(var2)',$(var3) from dual");
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


		// output bind
		System.out.println("OUTPUT BIND: ");
		cur.prepareQuery("begin :var1:='hello'; end;");
		cur.defineOutputBindString("var1",10);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindString("var1"),"hello");
		System.out.println();


		// array substitutions
		System.out.println("ARRAY SUBSTITUTIONS: ");
		cur.prepareQuery("select $(var1),$(var2),$(var3) from dual");
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
		cur.prepareQuery("select '$(var1)','$(var2)','$(var3)' from dual");
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
		cur.prepareQuery("select $(var1),$(var2),$(var3) from dual");
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
		assertTrue(cur.sendQuery("select null,1,null from dual"));
		assertEquals(cur.getField(0,0),null);
		assertEquals(cur.getField(0,1),"1");
		assertEquals(cur.getField(0,2),null);
		cur.getNullsAsEmptyStrings();
		assertTrue(cur.sendQuery("select null,1,null from dual"));
		assertEquals(cur.getField(0,0),"");
		assertEquals(cur.getField(0,1),"1");
		assertEquals(cur.getField(0,2),"");
		cur.getNullsAsNulls();
		System.out.println();


		// result set buffer size
		System.out.println("RESULT SET BUFFER SIZE: ");
		assertEquals(cur.getResultSetBufferSize(),0);
		cur.setResultSetBufferSize(2);
		assertTrue(cur.sendQuery("select * from testtable order by testnumber"));
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
		assertTrue(cur.sendQuery("select * from testtable order by testnumber"));
		assertEquals(cur.getColumnName(0),null);
		assertEquals(cur.getColumnLength(0),0);
		assertEquals(cur.getColumnType(0),null);
		cur.getColumnInfo();
		assertTrue(cur.sendQuery("select * from testtable order by testnumber"));
		assertEquals(cur.getColumnName(0),"TESTNUMBER");
		assertEquals(cur.getColumnLength(0),22);
		assertEquals(cur.getColumnType(0),"NUMBER");
		System.out.println();


		// suspended session
		System.out.println("SUSPENDED SESSION: ");
		assertTrue(cur.sendQuery("select * from testtable order by testnumber"));
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
		assertTrue(cur.sendQuery("select * from testtable order by testnumber"));
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
		assertTrue(cur.sendQuery("select * from testtable order by testnumber"));
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
		assertTrue(cur.sendQuery("select * from testtable order by testnumber"));
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
		assertTrue(cur.sendQuery("select * from testtable order by testnumber"));
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
		System.out.println("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
		cur.setResultSetBufferSize(2);
		cur.cacheToFile("cachefile1");
		cur.setCacheTtl(200);
		assertTrue(cur.sendQuery("select * from testtable order by testnumber"));
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
		assertTrue(cur.sendQuery("select * from testtable order by testnumber"));
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


		// commit and rollback
		System.out.println("COMMIT AND ROLLBACK: ");
		SQLRConnection secondcon=new SQLRConnection("sqlrelay",
						(short)9000,
						"/tmp/test.socket",
						null,null,0,1);
		SQLRCursor secondcur=new SQLRCursor(secondcon);
		secondcon.enableTls(null,cert,null,null,"ca",ca,(short)0);
		assertEquals(secondcur.sendQuery("select count(*) from testtable"),1);
		assertEquals(secondcur.getField(0,0),"0");
		assertTrue(con.commit());
		assertEquals(secondcur.sendQuery("select count(*) from testtable"),1);
		assertEquals(secondcur.getField(0,0),"8");
		assertTrue(con.autoCommitOn());
		assertTrue(cur.sendQuery("insert into testtable values (10,'testchar10','testvarchar10','01-JAN-2010','testlong10','testclob10',empty_blob())"));
		assertEquals(secondcur.sendQuery("select count(*) from testtable"),1);
		assertEquals(secondcur.getField(0,0),"9");
		assertTrue(con.autoCommitOff());
		System.out.println();


		// clob and blob output bind
		System.out.println("CLOB AND BLOB OUTPUT BIND:");
		cur.sendQuery("drop table testtable1");
		assertTrue(cur.sendQuery("create table testtable1 (testclob clob, testblob blob)"));
		cur.prepareQuery("insert into testtable1 values ('hello',:var1)");
		cur.inputBindBlob("var1",(new String("hello")).getBytes(),5);
		assertTrue(cur.executeQuery());
		cur.prepareQuery("begin select testclob into :clobvar from testtable1;  select testblob into :blobvar from testtable1; end;");
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
		cur.sendQuery("drop table testtable1");
		System.out.println();


		// null and empty clobs and clobs
		System.out.println("NULL AND EMPTY CLOBS AND CLOBS:");
		cur.getNullsAsNulls();
		cur.sendQuery("create table testtable1 (testclob1 clob, testclob2 clob, testblob1 blob, testblob2 blob)");
		cur.prepareQuery("insert into testtable1 values (:var1,:var2,:var3,:var4)");
		cur.inputBindClob("var1","",0);
		cur.inputBindClob("var2",null,0);
		cur.inputBindBlob("var3",(new String("")).getBytes(),0);
		cur.inputBindBlob("var4",null,0);
		assertTrue(cur.executeQuery());
		cur.sendQuery("select * from testtable1");
		assertEquals(cur.getField(0,0),null);
		assertEquals(cur.getField(0,1),null);
		assertEquals(cur.getField(0,2),null);
		assertEquals(cur.getField(0,3),null);
		cur.sendQuery("drop table testtable1");
		System.out.println();


		// cursor binds
		System.out.println("CURSOR BINDS:");
		assertTrue(cur.sendQuery("create or replace package types as type cursorType is ref cursor; end;"));
		assertTrue(cur.sendQuery("create or replace function sp_testtable return types.cursortype as l_cursor    types.cursorType; begin open l_cursor for select * from testtable; return l_cursor; end;"));
		cur.prepareQuery("begin  :curs:=sp_testtable; end;");
		cur.defineOutputBindCursor("curs");
		assertTrue(cur.executeQuery());
		SQLRCursor	bindcur=cur.getOutputBindCursor("curs");
		assertEquals(bindcur.fetchFromBindCursor(),1);
		assertEquals(bindcur.getField(0,0),"1");
		assertEquals(bindcur.getField(1,0),"2");
		assertEquals(bindcur.getField(2,0),"3");
		assertEquals(bindcur.getField(3,0),"4");
		assertEquals(bindcur.getField(4,0),"5");
		assertEquals(bindcur.getField(5,0),"6");
		assertEquals(bindcur.getField(6,0),"7");
		assertEquals(bindcur.getField(7,0),"8");
		System.out.println();


		// long clob
		System.out.println("LONG CLOB:");
		cur.sendQuery("drop table testtable2");
		cur.sendQuery("create table testtable2 (testclob clob)");
		cur.prepareQuery("insert into testtable2 values (:clobval)");
		StringBuffer	clobval=new StringBuffer();
		for (int i=0; i<8*1024; i++) {
			clobval.append('C');
		}
		cur.inputBindClob("clobval",clobval.toString(),8*1024);
		assertTrue(cur.executeQuery());
		cur.sendQuery("select testclob from testtable2");
		assertEquals(clobval.toString(),cur.getField(0,"TESTCLOB"));
		cur.prepareQuery("begin select testclob into :clobbindval from testtable2; end;");
		cur.defineOutputBindClob("clobbindval");
		assertTrue(cur.executeQuery());
		String	clobbindvar=cur.getOutputBindClob("clobbindval");
		assertEquals(cur.getOutputBindLength("clobbindval"),8*1024);
		assertEquals(clobval.toString(),clobbindvar);
		cur.sendQuery("drop table testtable2");
		System.out.println();


		System.out.println("LONG OUTPUT BIND");
		cur.sendQuery("drop table testtable2");
		cur.sendQuery("create table testtable2 (testval varchar2(4000))");
		cur.prepareQuery("insert into testtable2 values (:testval)");
		StringBuffer	testval=new StringBuffer();
		for (int i=0; i<4000; i++) {
			testval.append('C');
		}
		cur.inputBind("testval",testval.toString());
		assertTrue(cur.executeQuery());
		cur.sendQuery("select testval from testtable2");
		assertEquals(testval.toString(),cur.getField(0,"TESTVAL"));
		StringBuffer	query=new StringBuffer();
		query.append("begin :bindval:='");
		query.append(testval.toString());
		query.append("'; end;");
		cur.prepareQuery(query.toString());
		cur.defineOutputBindString("bindval",4000);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindLength("bindval"),4000);
		assertEquals(cur.getOutputBindString("bindval"),testval.toString());
		cur.sendQuery("drop table testtable2");
		System.out.println();

		System.out.println("NEGATIVE INPUT BIND");
		cur.sendQuery("create table testtable2 (testval number)");
		cur.prepareQuery("insert into testtable2 values (:testval)");
		cur.inputBind("testval",-1);
		assertTrue(cur.executeQuery());
		cur.sendQuery("select testval from testtable2");
		assertEquals(cur.getField(0,"TESTVAL"),"-1");
		cur.sendQuery("drop table testtable2");
		System.out.println();


		// finished suspended session
		System.out.println("FINISHED SUSPENDED SESSION: ");
		assertTrue(cur.sendQuery("select * from testtable order by testnumber"));
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
		cur.sendQuery("drop table testtable");
	
		// invalid queries...


		// invalid queries
		System.out.println("INVALID QUERIES: ");
		assertFalse(cur.sendQuery("select * from testtable order by testnumber"));
		assertFalse(cur.sendQuery("select * from testtable order by testnumber"));
		assertFalse(cur.sendQuery("select * from testtable order by testnumber"));
		assertFalse(cur.sendQuery("select * from testtable order by testnumber"));
		System.out.println();
		assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"));
		assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"));
		assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"));
		assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"));
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
