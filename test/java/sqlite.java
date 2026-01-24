// Copyright (c) David Muse
// See the file COPYING for more information.

import com.firstworks.sqlrelay.SQLRConnection;
import com.firstworks.sqlrelay.SQLRCursor;


class sqlite extends sqlrtest {
	
	public static void	main(String[] args) {
	
		String	dbtype;
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
		System.out.println("IDENTIFY: ");
		assertEquals(con.identify(),"sqlite");
		System.out.println();
	
		// ping
		System.out.println("PING: ");
		assertTrue(con.ping());
		System.out.println();
	
		// drop existing table
		cur.sendQuery("begin transaction");
		cur.sendQuery("drop table testtable");
		con.commit();
	
		// create a new table
		System.out.println("CREATE TEMPTABLE: ");
		cur.sendQuery("begin transaction");
		assertTrue(cur.sendQuery("create table testtable (testint int, testfloat float, testchar char(40), testvarchar varchar(40))"));
		con.commit();
		System.out.println();
	
		System.out.println("INSERT: ");
		cur.sendQuery("begin transaction");
		assertTrue(cur.sendQuery("insert into testtable values (1,1.1,'testchar1','testvarchar1')"));
		assertTrue(cur.sendQuery("insert into testtable values (2,2.2,'testchar2','testvarchar2')"));
		assertTrue(cur.sendQuery("insert into testtable values (3,3.3,'testchar3','testvarchar3')"));
		assertTrue(cur.sendQuery("insert into testtable values (4,4.4,'testchar4','testvarchar4')"));
		System.out.println();
	
		System.out.println("AFFECTED ROWS: ");
		assertEquals(cur.affectedRows(),0);
		System.out.println();
	
		System.out.println("BIND BY NAME: ");
		cur.prepareQuery("insert into testtable values (:var1,:var2,:var3,:var4)");
		assertEquals(cur.countBindVariables(),4);
		cur.inputBind("var1",5);
		cur.inputBind("var2",5.5,4,1);
		cur.inputBind("var3","testchar5");
		cur.inputBind("var4","testvarchar5");
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("var1",6);
		cur.inputBind("var2",6.6,4,1);
		cur.inputBind("var3","testchar6");
		cur.inputBind("var4","testvarchar6");
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("var1",7);
		cur.inputBind("var2",7.7,4,1);
		cur.inputBind("var3","testchar7");
		cur.inputBind("var4","testvarchar7");
		assertTrue(cur.executeQuery());
		System.out.println();
	
		System.out.println("BIND BY NAME WITH VALIDATION: ");
		cur.clearBinds();
		cur.inputBind("var1",8);
		cur.inputBind("var2",8.8,4,1);
		cur.inputBind("var3","testchar8");
		cur.inputBind("var4","testvarchar8");
		cur.validateBinds();
		assertTrue(cur.executeQuery());
		System.out.println();
	
		System.out.println("SELECT: ");
		assertTrue(cur.sendQuery("select * from testtable order by testint"));
		System.out.println();
	
		System.out.println("COLUMN COUNT: ");
		assertEquals(cur.colCount(),4);
		System.out.println();
	
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
	
		System.out.println("COLUMN TYPES: ");
		assertEquals(cur.getColumnType(0),"INTEGER");
		assertEquals(cur.getColumnType("testint"),"INTEGER");
		assertEquals(cur.getColumnType(1),"FLOAT");
		assertEquals(cur.getColumnType("testfloat"),"FLOAT");
		assertEquals(cur.getColumnType(2),"STRING");
		assertEquals(cur.getColumnType("testchar"),"STRING");
		assertEquals(cur.getColumnType(3),"STRING");
		assertEquals(cur.getColumnType("testvarchar"),"STRING");
		System.out.println();
	
		System.out.println("COLUMN LENGTH: ");
		assertEquals(cur.getColumnLength(0),0);
		assertEquals(cur.getColumnLength("testint"),0);
		assertEquals(cur.getColumnLength(1),0);
		assertEquals(cur.getColumnLength("testfloat"),0);
		assertEquals(cur.getColumnLength(2),0);
		assertEquals(cur.getColumnLength("testchar"),0);
		assertEquals(cur.getColumnLength(3),0);
		assertEquals(cur.getColumnLength("testvarchar"),0);
		System.out.println();
	
		System.out.println("LONGEST COLUMN: ");
		assertEquals(cur.getLongest(0),1);
		assertEquals(cur.getLongest("testint"),1);
		assertEquals(cur.getLongest(1),3);
		assertEquals(cur.getLongest("testfloat"),3);
		assertEquals(cur.getLongest(2),9);
		assertEquals(cur.getLongest("testchar"),9);
		assertEquals(cur.getLongest(3),12);
		assertEquals(cur.getLongest("testvarchar"),12);
		System.out.println();
	
		System.out.println("ROW COUNT: ");
		assertEquals(cur.rowCount(),8);
		System.out.println();
	
		System.out.println("TOTAL ROWS: ");
		assertEquals(cur.totalRows(),0);
		System.out.println();
	
		System.out.println("FIRST ROW INDEX: ");
		assertEquals(cur.firstRowIndex(),0);
		System.out.println();
	
		System.out.println("END OF RESULT SET: ");
		assertTrue(cur.endOfResultSet());
		System.out.println();
	
		System.out.println("FIELDS BY INDEX: ");
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"1.1");
		assertEquals(cur.getField(0,2),"testchar1");
		assertEquals(cur.getField(0,3),"testvarchar1");
		System.out.println();
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(7,1),"8.8");
		assertEquals(cur.getField(7,2),"testchar8");
		assertEquals(cur.getField(7,3),"testvarchar8");
		System.out.println();
	
		System.out.println("FIELD LENGTHS BY INDEX: ");
		assertEquals(cur.getFieldLength(0,0),1);
		assertEquals(cur.getFieldLength(0,1),3);
		assertEquals(cur.getFieldLength(0,2),9);
		assertEquals(cur.getFieldLength(0,3),12);
		System.out.println();
		assertEquals(cur.getFieldLength(7,0),1);
		assertEquals(cur.getFieldLength(7,1),3);
		assertEquals(cur.getFieldLength(7,2),9);
		assertEquals(cur.getFieldLength(7,3),12);
		System.out.println();
	
		System.out.println("FIELDS BY NAME: ");
		assertEquals(cur.getField(0,"testint"),"1");
		assertEquals(cur.getField(0,"testfloat"),"1.1");
		assertEquals(cur.getField(0,"testchar"),"testchar1");
		assertEquals(cur.getField(0,"testvarchar"),"testvarchar1");
		System.out.println();
		assertEquals(cur.getField(7,"testint"),"8");
		assertEquals(cur.getField(7,"testfloat"),"8.8");
		assertEquals(cur.getField(7,"testchar"),"testchar8");
		assertEquals(cur.getField(7,"testvarchar"),"testvarchar8");
		System.out.println();
	
		System.out.println("FIELD LENGTHS BY NAME: ");
		assertEquals(cur.getFieldLength(0,"testint"),1);
		assertEquals(cur.getFieldLength(0,"testfloat"),3);
		assertEquals(cur.getFieldLength(0,"testchar"),9);
		assertEquals(cur.getFieldLength(0,"testvarchar"),12);
		System.out.println();
		assertEquals(cur.getFieldLength(7,"testint"),1);
		assertEquals(cur.getFieldLength(7,"testfloat"),3);
		assertEquals(cur.getFieldLength(7,"testchar"),9);
		assertEquals(cur.getFieldLength(7,"testvarchar"),12);
		System.out.println();
	
		System.out.println("FIELDS BY ARRAY: ");
		fields=cur.getRow(0);
		assertEquals(fields[0],"1");
		assertEquals(fields[1],"1.1");
		assertEquals(fields[2],"testchar1");
		assertEquals(fields[3],"testvarchar1");
		System.out.println();
	
		System.out.println("FIELD LENGTHS BY ARRAY: ");
		fieldlens=cur.getRowLengths(0);
		assertEquals(fieldlens[0],1);
		assertEquals(fieldlens[1],3);
		assertEquals(fieldlens[2],9);
		assertEquals(fieldlens[3],12);
		System.out.println();
	
		System.out.println("INDIVIDUAL SUBSTITUTIONS: ");
		cur.sendQuery("drop table testtable1");
		assertTrue(cur.sendQuery("create table testtable1 (col1 int, col2 char, col3 float)"));
		cur.prepareQuery("insert into testtable1 values ($(var1),'$(var2)',$(var3))");
		cur.substitution("var1",1);
		cur.substitution("var2","hello");
		cur.substitution("var3",10.5556,6,4);
		assertTrue(cur.executeQuery());
		System.out.println();
	
		System.out.println("FIELDS: ");
		assertTrue(cur.sendQuery("select * from testtable1"));
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"hello");
		assertEquals(cur.getField(0,2),"10.5556");
		assertTrue(cur.sendQuery("delete from testtable1"));
		System.out.println();
	
		System.out.println("ARRAY SUBSTITUTIONS: ");
		cur.prepareQuery("insert into testtable1 values ('$(var1)','$(var2)','$(var3)')");
		cur.substitutions(subvars,subvalstrings);
		assertTrue(cur.executeQuery());
		System.out.println();
	
		System.out.println("FIELDS: ");
		assertTrue(cur.sendQuery("select * from testtable1"));
		assertEquals(cur.getField(0,0),"hi");
		assertEquals(cur.getField(0,1),"hello");
		assertEquals(cur.getField(0,2),"bye");
		assertTrue(cur.sendQuery("delete from testtable1"));
		System.out.println();
	
	
		System.out.println("ARRAY SUBSTITUTIONS: ");
		cur.prepareQuery("insert into testtable1 values ($(var1),'$(var2)',$(var3))");
		cur.substitutions(subvars,subvallongs);
		assertTrue(cur.executeQuery());
		System.out.println();
	
		System.out.println("FIELDS: ");
		assertTrue(cur.sendQuery("select * from testtable1"));
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"2");
		assertEquals(cur.getField(0,2),"3.0");
		assertTrue(cur.sendQuery("delete from testtable1"));
		System.out.println();
	
	
		System.out.println("ARRAY SUBSTITUTIONS: ");
		cur.prepareQuery("insert into testtable1 values ($(var1),'$(var2)',$(var3))");
		cur.substitutions(subvars,subvaldoubles,precs,scales);
		assertTrue(cur.executeQuery());
		System.out.println();
	
		System.out.println("FIELDS: ");
		assertTrue(cur.sendQuery("select * from testtable1"));
		assertEquals(cur.getField(0,0),"10.55");
		assertEquals(cur.getField(0,1),"10.556");
		assertEquals(cur.getField(0,2),"10.5556");
		assertTrue(cur.sendQuery("delete from testtable1"));
		System.out.println();
	
	
		System.out.println("nullS as Nulls: ");
		cur.getNullsAsNulls();
		assertTrue(cur.sendQuery("insert into testtable1 values (1,null,null)"));
		assertTrue(cur.sendQuery("select * from testtable1"));
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),null);
		assertEquals(cur.getField(0,2),null);
		cur.getNullsAsEmptyStrings();
		assertTrue(cur.sendQuery("select * from testtable1"));
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"");
		assertEquals(cur.getField(0,2),"");
		cur.getNullsAsNulls();
		System.out.println();
	
		System.out.println("RESULT SET BUFFER SIZE: ");
		assertEquals(cur.getResultSetBufferSize(),0);
		cur.setResultSetBufferSize(2);
		assertTrue(cur.sendQuery("select * from testtable order by testint"));
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
	
		System.out.println("DONT GET COLUMN INFO: ");
		cur.dontGetColumnInfo();
		assertTrue(cur.sendQuery("select * from testtable order by testint"));
		assertEquals(cur.getColumnName(0),null);
		assertEquals(cur.getColumnLength(0),0);
		assertEquals(cur.getColumnType(0),null);
		cur.getColumnInfo();
		assertTrue(cur.sendQuery("select * from testtable order by testint"));
		assertEquals(cur.getColumnName(0),"testint");
		assertEquals(cur.getColumnLength(0),0);
		assertEquals(cur.getColumnType(0),"INTEGER");
		System.out.println();
	
		System.out.println("SUSPENDED SESSION: ");
		assertTrue(cur.sendQuery("select * from testtable order by testint"));
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
		assertTrue(cur.sendQuery("select * from testtable order by testint"));
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
		assertTrue(cur.sendQuery("select * from testtable order by testint"));
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
	
		System.out.println("SUSPENDED RESULT SET: ");
		cur.setResultSetBufferSize(2);
		assertTrue(cur.sendQuery("select * from testtable order by testint"));
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
	
		System.out.println("CACHED RESULT SET: ");
		cur.cacheToFile("cachefile1");
		cur.setCacheTtl(200);
		assertTrue(cur.sendQuery("select * from testtable order by testint"));
		filename=cur.getCacheFileName();
		assertEquals(filename,"cachefile1");
		cur.cacheOff();
		assertTrue(cur.openCachedResultSet(filename));
		assertEquals(cur.getField(7,0),"8");
		System.out.println();
	
		System.out.println("COLUMN COUNT FOR CACHED RESULT SET: ");
		assertEquals(cur.colCount(),4);
		System.out.println();
	
		System.out.println("COLUMN NAMES FOR CACHED RESULT SET: ");
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
	
		System.out.println("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
		cur.setResultSetBufferSize(2);
		cur.cacheToFile("cachefile1");
		cur.setCacheTtl(200);
		assertTrue(cur.sendQuery("select * from testtable order by testint"));
		filename=cur.getCacheFileName();
		assertEquals(filename,"cachefile1");
		cur.cacheOff();
		assertTrue(cur.openCachedResultSet(filename));
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(8,0),null);
		cur.setResultSetBufferSize(0);
		System.out.println();
	
		System.out.println("FROM ONE CACHE FILE TO ANOTHER: ");
		cur.cacheToFile("cachefile2");
		assertTrue(cur.openCachedResultSet("cachefile1"));
		cur.cacheOff();
		assertTrue(cur.openCachedResultSet("cachefile2"));
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(8,0),null);
		System.out.println();
	
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
	
		System.out.println("CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE: ");
		cur.setResultSetBufferSize(2);
		cur.cacheToFile("cachefile1");
		cur.setCacheTtl(200);
		assertTrue(cur.sendQuery("select * from testtable order by testint"));
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

	    	System.out.println("COMMIT AND ROLLBACK: \n");
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
	    	assertTrue(cur.sendQuery("insert into testtable values (10,10.1,'testchar10','testvarchar10')"));
	    	assertEquals(secondcur.sendQuery("select count(*) from testtable"),1);
	    	assertEquals(secondcur.getField(0,0),"9");
		System.out.println();


		System.out.println("FINISHED SUSPENDED SESSION: ");
		assertTrue(cur.sendQuery("select * from testtable order by testint"));
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
		System.out.println("INVALID QUERIES: ");
		assertFalse(cur.sendQuery("select * from testtable"));
		assertFalse(cur.sendQuery("select * from testtable"));
		assertFalse(cur.sendQuery("select * from testtable"));
		assertFalse(cur.sendQuery("select * from testtable"));
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

		System.exit(0);
	}
}
