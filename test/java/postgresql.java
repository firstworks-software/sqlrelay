// Copyright (c) David Muse
// See the file COPYING for more information.

import com.firstworks.sqlrelay.SQLRConnection;
import com.firstworks.sqlrelay.SQLRCursor;


class postgresql extends sqlrtest {

	public static void	main(String[] args) {

		String[]	isolationlevels={
					"read committed",
					"read uncommitted",
					"repeatable read",
					"serializable"};
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
						"testuser","testpassword",0,1);
		SQLRCursor cur=new SQLRCursor(con);


		// identify
		System.out.println("IDENTIFY: ");
		assertEquals(con.identify(),"postgresql");
		System.out.println();


		// ping
		System.out.println("PING: ");
		assertTrue(con.ping());
		System.out.println();

		// isolation levels
		/*System.out.println("ISOLATION LEVELS: ");
		for (String il : isolationlevels) {
			// postgresql requires the isolation level to
			// be the first query of the transaction
			con.begin();
			assertTrue(con.setIsolationLevel(il));
			assertEquals(con.getIsolationLevel(),il);
			con.commit();
			System.out.println();
		}
		// reset to the default isolation level
		con.begin();
		assertTrue(con.setIsolationLevel(isolationlevels[0]));
		con.commit();
		System.out.println();*/

		// drop existing table
		cur.sendQuery("drop table testtable");


		// create temptable
		System.out.println("CREATE TEMPTABLE: ");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	testint int, "+
			"	testfloat float, "+
			"	testreal real, "+
			"	testsmallint smallint, "+
			"	testchar char(40), "+
			"	testvarchar varchar(40), "+
			"	testdate date, "+
			"	testtime time, "+
			"	testtimestamp timestamp)"));
		System.out.println();


		// begin transction
		System.out.println("BEGIN TRANSCTION: ");
		assertTrue(cur.sendQuery("begin"));
		System.out.println();


		// insert
		System.out.println("INSERT: ");
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	1, "+
			"	1.1, "+
			"	1.1, "+
			"	1, "+
			"	'testchar1', "+
			"	'testvarchar1', "+
			"	'01/01/2001', "+
			"	'01:00:00', "+
			"	null)"));
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	2, "+
			"	2.2, "+
			"	2.2, "+
			"	2, "+
			"	'testchar2', "+
			"	'testvarchar2', "+
			"	'01/01/2002', "+
			"	'02:00:00', "+
			"	null)"));
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	3, "+
			"	3.3, "+
			"	3.3, "+
			"	3, "+
			"	'testchar3', "+
			"	'testvarchar3', "+
			"	'01/01/2003', "+
			"	'03:00:00', "+
			"	null)"));
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	4, "+
			"	4.4, "+
			"	4.4, "+
			"	4, "+
			"	'testchar4', "+
			"	'testvarchar4', "+
			"	'01/01/2004', "+
			"	'04:00:00', "+
			"	null)"));
		System.out.println();


		// affected rows
		System.out.println("AFFECTED ROWS: ");
		assertEquals(cur.affectedRows(),1);
		System.out.println();


		// bind by position
		System.out.println("BIND BY POSITION: ");
		cur.prepareQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	$1, "+
			"	$2, "+
			"	$3, "+
			"	$4, "+
			"	$5, "+
			"	$6, "+
			"	$7, "+
			"	$8)");
		assertEquals(cur.countBindVariables(),8);
		cur.inputBind("1",5);
		cur.inputBind("2",5.5,4,2);
		cur.inputBind("3",5.5,4,2);
		cur.inputBind("4",5);
		cur.inputBind("5","testchar5");
		cur.inputBind("6","testvarchar5");
		cur.inputBind("7","01/01/2005");
		cur.inputBind("8","05:00:00");
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("1",6);
		cur.inputBind("2",6.6,4,2);
		cur.inputBind("3",6.6,4,2);
		cur.inputBind("4",6);
		cur.inputBind("5","testchar6");
		cur.inputBind("6","testvarchar6");
		cur.inputBind("7","01/01/2006");
		cur.inputBind("8","06:00:00");
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("1",7);
		cur.inputBind("2",7.7,4,2);
		cur.inputBind("3",7.7,4,2);
		cur.inputBind("4",7);
		cur.inputBind("5","testchar7");
		cur.inputBind("6","testvarchar7");
		cur.inputBind("7","01/01/2007");
		cur.inputBind("8","07:00:00");
		assertTrue(cur.executeQuery());
		System.out.println();


		// bind by position with validation
		System.out.println("BIND BY POSITION WITH VALIDATION: ");
		cur.clearBinds();
		cur.inputBind("1",8);
		cur.inputBind("2",8.8,4,2);
		cur.inputBind("3",8.8,4,2);
		cur.inputBind("4",8);
		cur.inputBind("5","testchar8");
		cur.inputBind("6","testvarchar8");
		cur.inputBind("7","01/01/2008");
		cur.inputBind("8","08:00:00");
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
		assertEquals(cur.colCount(),9);
		System.out.println();


		// column names
		System.out.println("COLUMN NAMES: ");
		assertEquals(cur.getColumnName(0),"testint");
		assertEquals(cur.getColumnName(1),"testfloat");
		assertEquals(cur.getColumnName(2),"testreal");
		assertEquals(cur.getColumnName(3),"testsmallint");
		assertEquals(cur.getColumnName(4),"testchar");
		assertEquals(cur.getColumnName(5),"testvarchar");
		assertEquals(cur.getColumnName(6),"testdate");
		assertEquals(cur.getColumnName(7),"testtime");
		assertEquals(cur.getColumnName(8),"testtimestamp");
		cols=cur.getColumnNames();
		assertEquals(cols[0],"testint");
		assertEquals(cols[1],"testfloat");
		assertEquals(cols[2],"testreal");
		assertEquals(cols[3],"testsmallint");
		assertEquals(cols[4],"testchar");
		assertEquals(cols[5],"testvarchar");
		assertEquals(cols[6],"testdate");
		assertEquals(cols[7],"testtime");
		assertEquals(cols[8],"testtimestamp");
		System.out.println();


		// column types
		System.out.println("COLUMN TYPES: ");
		assertEquals(cur.getColumnType(0),"int4");
		assertEquals(cur.getColumnType("testint"),"int4");
		assertEquals(cur.getColumnType(1),"float8");
		assertEquals(cur.getColumnType("testfloat"),"float8");
		assertEquals(cur.getColumnType(2),"float4");
		assertEquals(cur.getColumnType("testreal"),"float4");
		assertEquals(cur.getColumnType(3),"int2");
		assertEquals(cur.getColumnType("testsmallint"),"int2");
		assertEquals(cur.getColumnType(4),"bpchar");
		assertEquals(cur.getColumnType("testchar"),"bpchar");
		assertEquals(cur.getColumnType(5),"varchar");
		assertEquals(cur.getColumnType("testvarchar"),"varchar");
		assertEquals(cur.getColumnType(6),"date");
		assertEquals(cur.getColumnType("testdate"),"date");
		assertEquals(cur.getColumnType(7),"time");
		assertEquals(cur.getColumnType("testtime"),"time");
		assertEquals(cur.getColumnType(8),"timestamp");
		assertEquals(cur.getColumnType("testtimestamp"),"timestamp");
		System.out.println();


		// column length
		System.out.println("COLUMN LENGTH: ");
		assertEquals(cur.getColumnLength(0),4);
		assertEquals(cur.getColumnLength("testint"),4);
		assertEquals(cur.getColumnLength(1),8);
		assertEquals(cur.getColumnLength("testfloat"),8);
		assertEquals(cur.getColumnLength(2),4);
		assertEquals(cur.getColumnLength("testreal"),4);
		assertEquals(cur.getColumnLength(3),2);
		assertEquals(cur.getColumnLength("testsmallint"),2);
		assertEquals(cur.getColumnLength(4),44);
		assertEquals(cur.getColumnLength("testchar"),44);
		assertEquals(cur.getColumnLength(5),44);
		assertEquals(cur.getColumnLength("testvarchar"),44);
		assertEquals(cur.getColumnLength(6),4);
		assertEquals(cur.getColumnLength("testdate"),4);
		assertEquals(cur.getColumnLength(7),8);
		assertEquals(cur.getColumnLength("testtime"),8);
		assertEquals(cur.getColumnLength(8),8);
		assertEquals(cur.getColumnLength("testtimestamp"),8);
		System.out.println();


		// longest column
		System.out.println("LONGEST COLUMN: ");
		assertEquals(cur.getLongest(0),1);
		assertEquals(cur.getLongest("testint"),1);
		assertEquals(cur.getLongest(1),3);
		assertEquals(cur.getLongest("testfloat"),3);
		assertEquals(cur.getLongest(2),3);
		assertEquals(cur.getLongest("testreal"),3);
		assertEquals(cur.getLongest(3),1);
		assertEquals(cur.getLongest("testsmallint"),1);
		assertEquals(cur.getLongest(4),40);
		assertEquals(cur.getLongest("testchar"),40);
		assertEquals(cur.getLongest(5),12);
		assertEquals(cur.getLongest("testvarchar"),12);
		assertEquals(cur.getLongest(6),10);
		assertEquals(cur.getLongest("testdate"),10);
		assertEquals(cur.getLongest(7),8);
		assertEquals(cur.getLongest("testtime"),8);
		System.out.println();


		// row count
		System.out.println("ROW COUNT: ");
		assertEquals(cur.rowCount(),8);
		System.out.println();

		/*System.out.println("TOTAL ROWS: ");
		assertEquals(cur.totalRows(),8);
		System.out.println();*/


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
		assertEquals(cur.getField(0,1),"1.1");
		assertEquals(cur.getField(0,2),"1.1");
		assertEquals(cur.getField(0,3),"1");
		assertEquals(cur.getField(0,4),"testchar1                               ");
		assertEquals(cur.getField(0,5),"testvarchar1");
		assertEquals(cur.getField(0,6),"2001-01-01");
		assertEquals(cur.getField(0,7),"01:00:00");
		System.out.println();
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(7,1),"8.8");
		assertEquals(cur.getField(7,2),"8.8");
		assertEquals(cur.getField(7,3),"8");
		assertEquals(cur.getField(7,4),"testchar8                               ");
		assertEquals(cur.getField(7,5),"testvarchar8");
		assertEquals(cur.getField(7,6),"2008-01-01");
		assertEquals(cur.getField(7,7),"08:00:00");
		System.out.println();


		// field lengths by index
		System.out.println("FIELD LENGTHS BY INDEX: ");
		assertEquals(cur.getFieldLength(0,0),1);
		assertEquals(cur.getFieldLength(0,1),3);
		assertEquals(cur.getFieldLength(0,2),3);
		assertEquals(cur.getFieldLength(0,3),1);
		assertEquals(cur.getFieldLength(0,4),40);
		assertEquals(cur.getFieldLength(0,5),12);
		assertEquals(cur.getFieldLength(0,6),10);
		assertEquals(cur.getFieldLength(0,7),8);
		System.out.println();
		assertEquals(cur.getFieldLength(7,0),1);
		assertEquals(cur.getFieldLength(7,1),3);
		assertEquals(cur.getFieldLength(7,2),3);
		assertEquals(cur.getFieldLength(7,3),1);
		assertEquals(cur.getFieldLength(7,4),40);
		assertEquals(cur.getFieldLength(7,5),12);
		assertEquals(cur.getFieldLength(7,6),10);
		assertEquals(cur.getFieldLength(7,7),8);
		System.out.println();


		// fields by name
		System.out.println("FIELDS BY NAME: ");
		assertEquals(cur.getField(0,"testint"),"1");
		assertEquals(cur.getField(0,"testfloat"),"1.1");
		assertEquals(cur.getField(0,"testreal"),"1.1");
		assertEquals(cur.getField(0,"testsmallint"),"1");
		assertEquals(cur.getField(0,"testchar"),"testchar1                               ");
		assertEquals(cur.getField(0,"testvarchar"),"testvarchar1");
		assertEquals(cur.getField(0,"testdate"),"2001-01-01");
		assertEquals(cur.getField(0,"testtime"),"01:00:00");
		System.out.println();
		assertEquals(cur.getField(7,"testint"),"8");
		assertEquals(cur.getField(7,"testfloat"),"8.8");
		assertEquals(cur.getField(7,"testreal"),"8.8");
		assertEquals(cur.getField(7,"testsmallint"),"8");
		assertEquals(cur.getField(7,"testchar"),"testchar8                               ");
		assertEquals(cur.getField(7,"testvarchar"),"testvarchar8");
		assertEquals(cur.getField(7,"testdate"),"2008-01-01");
		assertEquals(cur.getField(7,"testtime"),"08:00:00");
		System.out.println();


		// field lengths by name
		System.out.println("FIELD LENGTHS BY NAME: ");
		assertEquals(cur.getFieldLength(0,"testint"),1);
		assertEquals(cur.getFieldLength(0,"testfloat"),3);
		assertEquals(cur.getFieldLength(0,"testreal"),3);
		assertEquals(cur.getFieldLength(0,"testsmallint"),1);
		assertEquals(cur.getFieldLength(0,"testchar"),40);
		assertEquals(cur.getFieldLength(0,"testvarchar"),12);
		assertEquals(cur.getFieldLength(0,"testdate"),10);
		assertEquals(cur.getFieldLength(0,"testtime"),8);
		System.out.println();
		assertEquals(cur.getFieldLength(7,"testint"),1);
		assertEquals(cur.getFieldLength(7,"testfloat"),3);
		assertEquals(cur.getFieldLength(7,"testreal"),3);
		assertEquals(cur.getFieldLength(7,"testsmallint"),1);
		assertEquals(cur.getFieldLength(7,"testchar"),40);
		assertEquals(cur.getFieldLength(7,"testvarchar"),12);
		assertEquals(cur.getFieldLength(7,"testdate"),10);
		assertEquals(cur.getFieldLength(7,"testtime"),8);
		System.out.println();


		// fields by array
		System.out.println("FIELDS BY ARRAY: ");
		fields=cur.getRow(0);
		assertEquals(fields[0],"1");
		assertEquals(fields[1],"1.1");
		assertEquals(fields[2],"1.1");
		assertEquals(fields[3],"1");
		assertEquals(fields[4],"testchar1                               ");
		assertEquals(fields[5],"testvarchar1");
		assertEquals(fields[6],"2001-01-01");
		assertEquals(fields[7],"01:00:00");
		System.out.println();


		// field lengths by array
		System.out.println("FIELD LENGTHS BY ARRAY: ");
		fieldlens=cur.getRowLengths(0);
		assertEquals(fieldlens[0],1);
		assertEquals(fieldlens[1],3);
		assertEquals(fieldlens[2],3);
		assertEquals(fieldlens[3],1);
		assertEquals(fieldlens[4],40);
		assertEquals(fieldlens[5],12);
		assertEquals(fieldlens[6],10);
		assertEquals(fieldlens[7],8);
		System.out.println();


		// individual substitutions
		System.out.println("INDIVIDUAL SUBSTITUTIONS: ");
		cur.prepareQuery("select $(var1),'$(var2)',$(var3)");
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
		cur.prepareQuery("select $(var1),$(var2),$(var3)");
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
		cur.prepareQuery("select '$(var1)','$(var2)','$(var3)'");
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
		cur.prepareQuery("select $(var1),$(var2),$(var3)");
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
		assertTrue(cur.sendQuery("select null,1,null"));
		assertEquals(cur.getField(0,0),null);
		assertEquals(cur.getField(0,1),"1");
		assertEquals(cur.getField(0,2),null);
		cur.getNullsAsEmptyStrings();
		assertTrue(cur.sendQuery("select null,1,null"));
		assertEquals(cur.getField(0,0),"");
		assertEquals(cur.getField(0,1),"1");
		assertEquals(cur.getField(0,2),"");
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
		assertEquals(cur.getColumnLength(0),4);
		assertEquals(cur.getColumnType(0),"int4");
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
		assertEquals(cur.colCount(),9);
		System.out.println();


		// column names for cached result set
		System.out.println("COLUMN NAMES FOR CACHED RESULT SET: ");
		assertEquals(cur.getColumnName(0),"testint");
		assertEquals(cur.getColumnName(1),"testfloat");
		assertEquals(cur.getColumnName(2),"testreal");
		assertEquals(cur.getColumnName(3),"testsmallint");
		assertEquals(cur.getColumnName(4),"testchar");
		assertEquals(cur.getColumnName(5),"testvarchar");
		assertEquals(cur.getColumnName(6),"testdate");
		assertEquals(cur.getColumnName(7),"testtime");
		assertEquals(cur.getColumnName(8),"testtimestamp");
		cols=cur.getColumnNames();
		assertEquals(cols[0],"testint");
		assertEquals(cols[1],"testfloat");
		assertEquals(cols[2],"testreal");
		assertEquals(cols[3],"testsmallint");
		assertEquals(cols[4],"testchar");
		assertEquals(cols[5],"testvarchar");
		assertEquals(cols[6],"testdate");
		assertEquals(cols[7],"testtime");
		assertEquals(cols[8],"testtimestamp");
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


		// commit and rollback
		System.out.println("COMMIT AND ROLLBACK: ");
		SQLRConnection secondcon=new SQLRConnection("sqlrelay",
						(short)9000,
						"/tmp/test.socket",
						"testuser","testpassword",0,1);
		SQLRCursor secondcur=new SQLRCursor(secondcon);
		assertEquals(secondcur.sendQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	testtable "),1);
		assertEquals(secondcur.getField(0,0),"0");
		assertTrue(con.commit());
		assertEquals(secondcur.sendQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	testtable "),1);
		assertEquals(secondcur.getField(0,0),"8");
		//assertTrue(con.autoCommitOn());
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	10, "+
			"	10.1, "+
			"	10.1, "+
			"	10, "+
			"	'testchar10', "+
			"	'testvarchar10', "+
			"	'01/01/2010', "+
			"	'10:00:00', "+
			"	null)"));
		assertEquals(secondcur.sendQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	testtable "),1);
		assertEquals(secondcur.getField(0,0),"9");
		//assertTrue(con.autoCommitOff());
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


		// stored procedures
		System.out.println("STORED PROCEDURES: ");
		// return no values
		cur.sendQuery("drop function testfunc(int,float,char(20))");
		assertTrue(cur.sendQuery(
			"create function testfunc("+
			"	int,float,char(20)) "+
			"returns void as ' "+
			"	declare in1 int; "+
			"	in2 float; "+
			"	in3 char(20); "+
			"begin "+
			"	in1:=$1; "+
			"	in2:=$2; "+
			"	in3:=$3; "+
			"	return; "+
			"end;' language plpgsql"));
		cur.prepareQuery("select testfunc($1,$2,$3)");
		cur.inputBind("1",1);
		cur.inputBind("2",1.1,4,2);
		cur.inputBind("3","hello");
		assertTrue(cur.executeQuery());
		cur.sendQuery("drop function testfunc(int,float,char(20))");
		System.out.println();
		// return single value
		cur.sendQuery("drop function testfunc(int,float,char(20))");
		assertTrue(cur.sendQuery(
			"create function testfunc("+
			"	int,float,char(20)) "+
			"returns int as "+
			"	' begin return $1; end;' "+
			"language plpgsql"));
		cur.prepareQuery("select * from testfunc($1,$2,$3)");
		cur.inputBind("1",1);
		cur.inputBind("2",1.1,4,2);
		cur.inputBind("3","hello");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"1");
		cur.sendQuery("drop function testfunc(int,float,char(20))");
		System.out.println();
		// return multiple values
		cur.sendQuery("drop function testfunc(int,char(20))");
		assertTrue(cur.sendQuery(
			"create function testfunc("+
			"	int,float,char(20)) "+
			"returns record as ' "+
			"	declare output record; "+
			"begin "+
			"	select $1,$2,$3 into output; "+
			"	return output; "+
			"end;' language plpgsql"));
		cur.prepareQuery(
			"select "+
			"	* "+
			"from "+
			"	testfunc($1,$2,$3) "+
			"	as (col1 int, "+
			"		col2 float, "+
			"		col3 bpchar) ");
		cur.inputBind("1",1);
		cur.inputBind("2",1.1,4,2);
		cur.inputBind("3","hello");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"1.1");
		assertEquals(cur.getField(0,2),"hello");
		cur.sendQuery("drop function testfunc(int,float,char(20))");
		System.out.println();
		// return result set
		cur.sendQuery("drop function testfunc()");
		assertTrue(cur.sendQuery(
			"create function testfunc() "+
			"returns setof record as ' "+
			"	declare output record; "+
			"begin "+
			"	for output in "+
			"		select * from testtable "+
			"	loop "+
			"		return next output; "+
			"	end loop; "+
			"	return; "+
			"end;' language plpgsql"));
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testfunc() "+
			"	as (testint int, "+
			"		testfloat float, "+
			"		testreal real, "+
			"		testsmallint smallint, "+
			"		testchar char(40), "+
			"		testvarchar varchar(40), "+
			"		testdate date, "+
			"		testtime time, "+
			"		testtimestamp timestamp) "));
		assertEquals(cur.getField(4,0),"5");
		assertEquals(cur.getField(5,0),"6");
		assertEquals(cur.getField(6,0),"7");
		assertEquals(cur.getField(7,0),"8");
		cur.sendQuery("drop function testfunc()");
		System.out.println();

		// drop existing table
		cur.sendQuery("drop table testtable");


		// invalid queries
		System.out.println("INVALID QUERIES: ");
		assertFalse(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testint "));
		assertFalse(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testint "));
		assertFalse(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testint "));
		assertFalse(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testint "));
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
