// Copyright (c) David Muse
// See the file COPYING for more information.

import com.firstworks.sqlrelay.SQLRConnection;
import com.firstworks.sqlrelay.SQLRCursor;


class freetds extends sqlrtest {
	
	public static void	main(String[] args) {

		String	dbtype;
		String[]	isolationlevels={"1","0","2","3"};
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
		assertEquals(con.identify(),"freetds");
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
		assertTrue(cur.sendQuery("create table testtable (testint int, testsmallint smallint, testtinyint tinyint, testreal real, testfloat float, testdecimal decimal(4,1), testnumeric numeric(4,1), testmoney money, testsmallmoney smallmoney, testdatetime datetime, testsmalldatetime smalldatetime, testchar char(40), testvarchar varchar(40), testbit bit)"));
		System.out.println();


		// begin transaction
		System.out.println("BEGIN TRANSACTION: ");
		assertTrue(cur.sendQuery("begin tran"));
		System.out.println();


		// insert
		System.out.println("INSERT: ");
		assertTrue(cur.sendQuery("insert into testtable values (1,1,1,1.1,1.1,1.1,1.1,1.00,1.00,'01-Jan-2001 01:00:00','01-Jan-2001 01:00:00','testchar1','testvarchar1',1)"));
		System.out.println();


		// affected rows
		System.out.println("AFFECTED ROWS: ");
		assertEquals(cur.affectedRows(),1);
		System.out.println();


		// bind by position
		System.out.println("BIND BY POSITION: ");
		cur.prepareQuery("insert into testtable values (?,?,?,?,?,?,?,?,?,?,?,?,?,?)");
		assertEquals(cur.countBindVariables(),14);
		cur.inputBind("1",2);
		cur.inputBind("2",2);
		cur.inputBind("3",2);
		cur.inputBind("4",2.2,2,1);
		cur.inputBind("5",2.2,2,1);
		cur.inputBind("6",2.2,2,1);
		cur.inputBind("7",2.2,2,1);
		cur.inputBind("8",2.00,3,2);
		cur.inputBind("9",2.00,3,2);
		cur.inputBind("10","01-Jan-2002 02:00:00");
		cur.inputBind("11","01-Jan-2002 02:00:00");
		cur.inputBind("12","testchar2");
		cur.inputBind("13","testvarchar2");
		cur.inputBind("14",1);
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("1",3);
		cur.inputBind("2",3);
		cur.inputBind("3",3);
		cur.inputBind("4",3.3,2,1);
		cur.inputBind("5",3.3,2,1);
		cur.inputBind("6",3.3,2,1);
		cur.inputBind("7",3.3,2,1);
		cur.inputBind("8",3.00,3,2);
		cur.inputBind("9",3.00,3,2);
		cur.inputBind("10","01-Jan-2003 03:00:00");
		cur.inputBind("11","01-Jan-2003 03:00:00");
		cur.inputBind("12","testchar3");
		cur.inputBind("13","testvarchar3");
		cur.inputBind("14",1);
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("1",4);
		cur.inputBind("2",4);
		cur.inputBind("3",4);
		cur.inputBind("4",4.4,2,1);
		cur.inputBind("5",4.4,2,1);
		cur.inputBind("6",4.4,2,1);
		cur.inputBind("7",4.4,2,1);
		cur.inputBind("8",4.00,3,2);
		cur.inputBind("9",4.00,3,2);
		cur.inputBind("10","01-Jan-2004 04:00:00");
		cur.inputBind("11","01-Jan-2004 04:00:00");
		cur.inputBind("12","testchar4");
		cur.inputBind("13","testvarchar4");
		cur.inputBind("14",1);
		assertTrue(cur.executeQuery());
		System.out.println();


		// bind by name
		System.out.println("BIND BY NAME: ");
		cur.clearBinds();
		cur.prepareQuery("insert into testtable values (@var1,@var2,@var3,@var4,@var5,@var6,@var7,@var8,@var9,@var10,@var11,@var12,@var13,@var14)");
		cur.inputBind("var1",5);
		cur.inputBind("var2",5);
		cur.inputBind("var3",5);
		cur.inputBind("var4",5.5,2,1);
		cur.inputBind("var5",5.5,2,1);
		cur.inputBind("var6",5.5,2,1);
		cur.inputBind("var7",5.5,2,1);
		cur.inputBind("var8",5.00,3,2);
		cur.inputBind("var9",5.00,3,2);
		cur.inputBind("var10","01-Jan-2005 05:00:00");
		cur.inputBind("var11","01-Jan-2005 05:00:00");
		cur.inputBind("var12","testchar5");
		cur.inputBind("var13","testvarchar5");
		cur.inputBind("var14",1);
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("var1",6);
		cur.inputBind("var2",6);
		cur.inputBind("var3",6);
		cur.inputBind("var4",6.6,2,1);
		cur.inputBind("var5",6.6,2,1);
		cur.inputBind("var6",6.6,2,1);
		cur.inputBind("var7",6.6,2,1);
		cur.inputBind("var8",6.00,3,2);
		cur.inputBind("var9",6.00,3,2);
		cur.inputBind("var10","01-Jan-2006 06:00:00");
		cur.inputBind("var11","01-Jan-2006 06:00:00");
		cur.inputBind("var12","testchar6");
		cur.inputBind("var13","testvarchar6");
		cur.inputBind("var14",1);
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("var1",7);
		cur.inputBind("var2",7);
		cur.inputBind("var3",7);
		cur.inputBind("var4",7.7,2,1);
		cur.inputBind("var5",7.7,2,1);
		cur.inputBind("var6",7.7,2,1);
		cur.inputBind("var7",7.7,2,1);
		cur.inputBind("var8",7.00,3,2);
		cur.inputBind("var9",7.00,3,2);
		cur.inputBind("var10","01-Jan-2007 07:00:00");
		cur.inputBind("var11","01-Jan-2007 07:00:00");
		cur.inputBind("var12","testchar7");
		cur.inputBind("var13","testvarchar7");
		cur.inputBind("var14",1);
		assertTrue(cur.executeQuery());
		System.out.println();


		// bind by name with validation
		System.out.println("BIND BY NAME WITH VALIDATION: ");
		cur.clearBinds();
		cur.inputBind("var1",8);
		cur.inputBind("var2",8);
		cur.inputBind("var3",8);
		cur.inputBind("var4",8.8,2,1);
		cur.inputBind("var5",8.8,2,1);
		cur.inputBind("var6",8.8,2,1);
		cur.inputBind("var7",8.8,2,1);
		cur.inputBind("var8",8.00,3,2);
		cur.inputBind("var9",8.00,3,2);
		cur.inputBind("var10","01-Jan-2008 08:00:00");
		cur.inputBind("var11","01-Jan-2008 08:00:00");
		cur.inputBind("var12","testchar8");
		cur.inputBind("var13","testvarchar8");
		cur.inputBind("var14",1);
		cur.inputBind("var15","junkvalue");
		cur.validateBinds();
		assertTrue(cur.executeQuery());
		System.out.println();


		// select
		System.out.println("SELECT: ");
		assertTrue(cur.sendQuery("select * from testtable order by testint"));
		System.out.println();


		// column count
		System.out.println("COLUMN COUNT: ");
		assertEquals(cur.colCount(),14);
		System.out.println();


		// column names
		System.out.println("COLUMN NAMES: ");
		assertEquals(cur.getColumnName(0),"testint");
		assertEquals(cur.getColumnName(1),"testsmallint");
		assertEquals(cur.getColumnName(2),"testtinyint");
		assertEquals(cur.getColumnName(3),"testreal");
		assertEquals(cur.getColumnName(4),"testfloat");
		assertEquals(cur.getColumnName(5),"testdecimal");
		assertEquals(cur.getColumnName(6),"testnumeric");
		assertEquals(cur.getColumnName(7),"testmoney");
		assertEquals(cur.getColumnName(8),"testsmallmoney");
		assertEquals(cur.getColumnName(9),"testdatetime");
		assertEquals(cur.getColumnName(10),"testsmalldatetime");
		assertEquals(cur.getColumnName(11),"testchar");
		assertEquals(cur.getColumnName(12),"testvarchar");
		assertEquals(cur.getColumnName(13),"testbit");
		cols=cur.getColumnNames();
		assertEquals(cols[0],"testint");
		assertEquals(cols[1],"testsmallint");
		assertEquals(cols[2],"testtinyint");
		assertEquals(cols[3],"testreal");
		assertEquals(cols[4],"testfloat");
		assertEquals(cols[5],"testdecimal");
		assertEquals(cols[6],"testnumeric");
		assertEquals(cols[7],"testmoney");
		assertEquals(cols[8],"testsmallmoney");
		assertEquals(cols[9],"testdatetime");
		assertEquals(cols[10],"testsmalldatetime");
		assertEquals(cols[11],"testchar");
		assertEquals(cols[12],"testvarchar");
		assertEquals(cols[13],"testbit");
		System.out.println();


		// column types
		System.out.println("COLUMN TYPES: ");
		assertEquals(cur.getColumnType(0),"INT");
		assertEquals(cur.getColumnType("testint"),"INT");
		assertEquals(cur.getColumnType(1),"SMALLINT");
		assertEquals(cur.getColumnType("testsmallint"),"SMALLINT");
		assertEquals(cur.getColumnType(2),"TINYINT");
		assertEquals(cur.getColumnType("testtinyint"),"TINYINT");
		assertEquals(cur.getColumnType(3),"REAL");
		assertEquals(cur.getColumnType("testreal"),"REAL");
		assertEquals(cur.getColumnType(4),"FLOAT");
		assertEquals(cur.getColumnType("testfloat"),"FLOAT");
		assertEquals(cur.getColumnType(5),"DECIMAL");
		assertEquals(cur.getColumnType("testdecimal"),"DECIMAL");
		assertEquals(cur.getColumnType(6),"NUMERIC");
		assertEquals(cur.getColumnType("testnumeric"),"NUMERIC");
		assertEquals(cur.getColumnType(7),"MONEY");
		assertEquals(cur.getColumnType("testmoney"),"MONEY");
		assertEquals(cur.getColumnType(8),"SMALLMONEY");
		assertEquals(cur.getColumnType("testsmallmoney"),"SMALLMONEY");
		assertEquals(cur.getColumnType(9),"DATETIME");
		assertEquals(cur.getColumnType("testdatetime"),"DATETIME");
		assertEquals(cur.getColumnType(10),"SMALLDATETIME");
		assertEquals(cur.getColumnType("testsmalldatetime"),"SMALLDATETIME");
		assertEquals(cur.getColumnType(11),"CHAR");
		assertEquals(cur.getColumnType("testchar"),"CHAR");
		assertEquals(cur.getColumnType(12),"CHAR");
		assertEquals(cur.getColumnType("testvarchar"),"CHAR");
		assertEquals(cur.getColumnType(13),"BIT");
		assertEquals(cur.getColumnType("testbit"),"BIT");
		System.out.println();


		// column length
		System.out.println("COLUMN LENGTH: ");
		assertEquals(cur.getColumnLength(0),4);
		assertEquals(cur.getColumnLength("testint"),4);
		assertEquals(cur.getColumnLength(1),2);
		assertEquals(cur.getColumnLength("testsmallint"),2);
		assertEquals(cur.getColumnLength(2),1);
		assertEquals(cur.getColumnLength("testtinyint"),1);
		assertEquals(cur.getColumnLength(3),4);
		assertEquals(cur.getColumnLength("testreal"),4);
		assertEquals(cur.getColumnLength(4),8);
		assertEquals(cur.getColumnLength("testfloat"),8);
		// these seem to fluctuate with every freetds release
		/*assertEquals(cur.getColumnLength(5),3);
		assertEquals(cur.getColumnLength("testdecimal"),3);
		assertEquals(cur.getColumnLength(6),3);
		assertEquals(cur.getColumnLength("testnumeric"),3);*/
		assertEquals(cur.getColumnLength(7),8);
		assertEquals(cur.getColumnLength("testmoney"),8);
		assertEquals(cur.getColumnLength(8),4);
		assertEquals(cur.getColumnLength("testsmallmoney"),4);
		assertEquals(cur.getColumnLength(9),8);
		assertEquals(cur.getColumnLength("testdatetime"),8);
		assertEquals(cur.getColumnLength(10),4);
		assertEquals(cur.getColumnLength("testsmalldatetime"),4);
		// these seem to fluctuate too
		/*assertEquals(cur.getColumnLength(11),40);
		assertEquals(cur.getColumnLength("testchar"),40);
		assertEquals(cur.getColumnLength(12),40);
		assertEquals(cur.getColumnLength("testvarchar"),40);*/
		assertEquals(cur.getColumnLength(13),1);
		assertEquals(cur.getColumnLength("testbit"),1);
		System.out.println();


		// longest column
		System.out.println("LONGEST COLUMN: ");
		assertEquals(cur.getLongest(0),1);
		assertEquals(cur.getLongest("testint"),1);
		assertEquals(cur.getLongest(1),1);
		assertEquals(cur.getLongest("testsmallint"),1);
		assertEquals(cur.getLongest(2),1);
		assertEquals(cur.getLongest("testtinyint"),1);
		//assertEquals(cur.getLongest(3),3);
		//assertEquals(cur.getLongest("testreal"),3);
		//assertEquals(cur.getLongest(4),17);
		//assertEquals(cur.getLongest("testfloat"),17);
		//assertEquals(cur.getLongest(5),3);
		//assertEquals(cur.getLongest("testdecimal"),3);
		//assertEquals(cur.getLongest(6),3);
		//assertEquals(cur.getLongest("testnumeric"),3);
		//assertEquals(cur.getLongest(7),4);
		//assertEquals(cur.getLongest("testmoney"),4);
		//assertEquals(cur.getLongest(8),4);
		//assertEquals(cur.getLongest("testsmallmoney"),4);
		//assertEquals(cur.getLongest(9),26);
		//assertEquals(cur.getLongest("testdatetime"),26);
		//assertEquals(cur.getLongest(10),26);
		//assertEquals(cur.getLongest("testsmalldatetime"),26);
		assertEquals(cur.getLongest(11),40);
		assertEquals(cur.getLongest("testchar"),40);
		assertEquals(cur.getLongest(12),12);
		assertEquals(cur.getLongest("testvarchar"),12);
		assertEquals(cur.getLongest(13),1);
		assertEquals(cur.getLongest("testbit"),1);
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
		//assertEquals(cur.getField(0,3),"1.1");
		//assertEquals(cur.getField(0,4),"1.1");
		assertEquals(cur.getField(0,5),"1.1");
		assertEquals(cur.getField(0,6),"1.1");
		//assertEquals(cur.getField(0,7),"1.00");
		//assertEquals(cur.getField(0,8),"1.00");
		//assertEquals(cur.getField(0,9),"Jan  1 2001 01:00:00:000AM");
		//assertEquals(cur.getField(0,10),"Jan  1 2001 01:00:00:000AM");
		assertEquals(cur.getField(0,11),"testchar1                               ");
		assertEquals(cur.getField(0,12),"testvarchar1");
		assertEquals(cur.getField(0,13),"1");
		System.out.println();
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(7,1),"8");
		assertEquals(cur.getField(7,2),"8");
		//assertEquals(cur.getField(7,3),"8.8");
		//assertEquals(cur.getField(7,4),"8.8");
		//assertEquals(cur.getField(7,5),"8.8");
		//assertEquals(cur.getField(7,6),"8.8");
		//assertEquals(cur.getField(7,7),"8.00");
		//assertEquals(cur.getField(7,8),"8.00");
		//assertEquals(cur.getField(7,9),"Jan  1 2008 08:00:00:000AM");
		//assertEquals(cur.getField(7,10),"Jan  1 2008 08:00:00:000AM");
		assertEquals(cur.getField(7,11),"testchar8                               ");
		assertEquals(cur.getField(7,12),"testvarchar8");
		assertEquals(cur.getField(7,13),"1");
		System.out.println();


		// field lengths by index
		System.out.println("FIELD LENGTHS BY INDEX: ");
		assertEquals(cur.getFieldLength(0,0),1);
		assertEquals(cur.getFieldLength(0,1),1);
		assertEquals(cur.getFieldLength(0,2),1);
		//assertEquals(cur.getFieldLength(0,3),3);
		//assertEquals(cur.getFieldLength(0,4),3);
		//assertEquals(cur.getFieldLength(0,5),3);
		//assertEquals(cur.getFieldLength(0,6),3);
		//assertEquals(cur.getFieldLength(0,7),4);
		//assertEquals(cur.getFieldLength(0,8),4);
		//assertEquals(cur.getFieldLength(0,9),26);
		//assertEquals(cur.getFieldLength(0,10),26);
		assertEquals(cur.getFieldLength(0,11),40);
		assertEquals(cur.getFieldLength(0,12),12);
		assertEquals(cur.getFieldLength(0,13),1);
		System.out.println();
		assertEquals(cur.getFieldLength(7,0),1);
		assertEquals(cur.getFieldLength(7,1),1);
		assertEquals(cur.getFieldLength(7,2),1);
		//assertEquals(cur.getFieldLength(7,3),3);
		//assertEquals(cur.getFieldLength(7,4),17);
		//assertEquals(cur.getFieldLength(7,5),3);
		//assertEquals(cur.getFieldLength(7,6),3);
		//assertEquals(cur.getFieldLength(7,7),4);
		//assertEquals(cur.getFieldLength(7,8),4);
		//assertEquals(cur.getFieldLength(7,9),26);
		//assertEquals(cur.getFieldLength(7,10),26);
		assertEquals(cur.getFieldLength(7,11),40);
		assertEquals(cur.getFieldLength(7,12),12);
		assertEquals(cur.getFieldLength(7,13),1);
		System.out.println();


		// fields by name
		System.out.println("FIELDS BY NAME: ");
		assertEquals(cur.getField(0,"testint"),"1");
		assertEquals(cur.getField(0,"testsmallint"),"1");
		assertEquals(cur.getField(0,"testtinyint"),"1");
		//assertEquals(cur.getField(0,"testreal"),"1.1");
		//assertEquals(cur.getField(0,"testfloat"),"1.1");
		assertEquals(cur.getField(0,"testdecimal"),"1.1");
		assertEquals(cur.getField(0,"testnumeric"),"1.1");
		//assertEquals(cur.getField(0,"testmoney"),"1.00");
		//assertEquals(cur.getField(0,"testsmallmoney"),"1.00");
		//assertEquals(cur.getField(0,"testdatetime"),"Jan  1 2001 01:00:00:000AM");
		//assertEquals(cur.getField(0,"testsmalldatetime"),"Jan  1 2001 01:00:00:000AM");
		assertEquals(cur.getField(0,"testchar"),"testchar1                               ");
		assertEquals(cur.getField(0,"testvarchar"),"testvarchar1");
		assertEquals(cur.getField(0,"testbit"),"1");
		System.out.println();
		assertEquals(cur.getField(7,"testint"),"8");
		assertEquals(cur.getField(7,"testsmallint"),"8");
		assertEquals(cur.getField(7,"testtinyint"),"8");
		//assertEquals(cur.getField(7,"testreal"),"8.8");
		//assertEquals(cur.getField(7,"testfloat"),"8.8");
		//assertEquals(cur.getField(7,"testdecimal"),"8.8");
		//assertEquals(cur.getField(7,"testnumeric"),"8.8");
		//assertEquals(cur.getField(7,"testmoney"),"8.00");
		//assertEquals(cur.getField(7,"testsmallmoney"),"8.00");
		//assertEquals(cur.getField(7,"testdatetime"),"Jan  1 2008 08:00:00:000AM");
		//assertEquals(cur.getField(7,"testsmalldatetime"),"Jan  1 2008 08:00:00:000AM");
		assertEquals(cur.getField(7,"testchar"),"testchar8                               ");
		assertEquals(cur.getField(7,"testvarchar"),"testvarchar8");
		assertEquals(cur.getField(7,"testbit"),"1");
		System.out.println();


		// field lengths by name
		System.out.println("FIELD LENGTHS BY NAME: ");
		assertEquals(cur.getFieldLength(0,"testint"),1);
		assertEquals(cur.getFieldLength(0,"testsmallint"),1);
		assertEquals(cur.getFieldLength(0,"testtinyint"),1);
		//assertEquals(cur.getFieldLength(0,"testreal"),3);
		//assertEquals(cur.getFieldLength(0,"testfloat"),3);
		//assertEquals(cur.getFieldLength(0,"testdecimal"),3);
		//assertEquals(cur.getFieldLength(0,"testnumeric"),3);
		//assertEquals(cur.getFieldLength(0,"testmoney"),4);
		//assertEquals(cur.getFieldLength(0,"testsmallmoney"),4);
		//assertEquals(cur.getFieldLength(0,"testdatetime"),26);
		//assertEquals(cur.getFieldLength(0,"testsmalldatetime"),26);
		assertEquals(cur.getFieldLength(0,"testchar"),40);
		assertEquals(cur.getFieldLength(0,"testvarchar"),12);
		assertEquals(cur.getFieldLength(0,"testbit"),1);
		System.out.println();
		assertEquals(cur.getFieldLength(7,"testint"),1);
		assertEquals(cur.getFieldLength(7,"testsmallint"),1);
		assertEquals(cur.getFieldLength(7,"testtinyint"),1);
		//assertEquals(cur.getFieldLength(7,"testreal"),3);
		//assertEquals(cur.getFieldLength(7,"testfloat"),17);
		//assertEquals(cur.getFieldLength(7,"testdecimal"),3);
		//assertEquals(cur.getFieldLength(7,"testnumeric"),3);
		//assertEquals(cur.getFieldLength(7,"testmoney"),4);
		//assertEquals(cur.getFieldLength(7,"testsmallmoney"),4);
		//assertEquals(cur.getFieldLength(7,"testdatetime"),26);
		//assertEquals(cur.getFieldLength(7,"testsmalldatetime"),26);
		assertEquals(cur.getFieldLength(7,"testchar"),40);
		assertEquals(cur.getFieldLength(7,"testvarchar"),12);
		assertEquals(cur.getFieldLength(7,"testbit"),1);
		System.out.println();


		// fields by array
		System.out.println("FIELDS BY ARRAY: ");
		fields=cur.getRow(0);
		assertEquals(fields[0],"1");
		assertEquals(fields[1],"1");
		assertEquals(fields[2],"1");
		//assertEquals(fields[3],"1.1");
		//assertEquals(fields[4],"1.1");
		assertEquals(fields[5],"1.1");
		assertEquals(fields[6],"1.1");
		//assertEquals(fields[7],"1.00");
		//assertEquals(fields[8],"1.00");
		//assertEquals(fields[9],"Jan  1 2001 01:00:00:000AM");
		//assertEquals(fields[10],"Jan  1 2001 01:00:00:000AM");
		assertEquals(fields[11],"testchar1                               ");
		assertEquals(fields[12],"testvarchar1");
		assertEquals(fields[13],"1");
		System.out.println();


		// field lengths by array
		System.out.println("FIELD LENGTHS BY ARRAY: ");
		fieldlens=cur.getRowLengths(0);
		assertEquals(fieldlens[0],1);
		assertEquals(fieldlens[1],1);
		assertEquals(fieldlens[2],1);
		//assertEquals(fieldlens[3],3);
		//assertEquals(fieldlens[4],3);
		assertEquals(fieldlens[5],3);
		assertEquals(fieldlens[6],3);
		//assertEquals(fieldlens[7],4);
		//assertEquals(fieldlens[8],4);
		//assertEquals(fieldlens[9],26);
		//assertEquals(fieldlens[10],26);
		assertEquals(fieldlens[11],40);
		assertEquals(fieldlens[12],12);
		assertEquals(fieldlens[13],1);
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


		// dont get column info
		System.out.println("DONT GET COLUMN INFO: ");
		cur.dontGetColumnInfo();
		assertTrue(cur.sendQuery("select * from testtable order by testint"));
		assertEquals(cur.getColumnName(0),null);
		assertEquals(cur.getColumnLength(0),0);
		assertEquals(cur.getColumnType(0),null);
		cur.getColumnInfo();
		assertTrue(cur.sendQuery("select * from testtable order by testint"));
		assertEquals(cur.getColumnName(0),"testint");
		assertEquals(cur.getColumnLength(0),4);
		assertEquals(cur.getColumnType(0),"INT");
		System.out.println();


		// suspended session
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


		// suspended result set
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


		// cached result set
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


		// column count for cached result set
		System.out.println("COLUMN COUNT FOR CACHED RESULT SET: ");
		assertEquals(cur.colCount(),14);
		System.out.println();


		// column names for cached result set
		System.out.println("COLUMN NAMES FOR CACHED RESULT SET: ");
		assertEquals(cur.getColumnName(0),"testint");
		assertEquals(cur.getColumnName(1),"testsmallint");
		assertEquals(cur.getColumnName(2),"testtinyint");
		assertEquals(cur.getColumnName(3),"testreal");
		assertEquals(cur.getColumnName(4),"testfloat");
		assertEquals(cur.getColumnName(5),"testdecimal");
		assertEquals(cur.getColumnName(6),"testnumeric");
		assertEquals(cur.getColumnName(7),"testmoney");
		assertEquals(cur.getColumnName(8),"testsmallmoney");
		assertEquals(cur.getColumnName(9),"testdatetime");
		assertEquals(cur.getColumnName(10),"testsmalldatetime");
		assertEquals(cur.getColumnName(11),"testchar");
		assertEquals(cur.getColumnName(12),"testvarchar");
		assertEquals(cur.getColumnName(13),"testbit");
		cols=cur.getColumnNames();
		assertEquals(cols[0],"testint");
		assertEquals(cols[1],"testsmallint");
		assertEquals(cols[2],"testtinyint");
		assertEquals(cols[3],"testreal");
		assertEquals(cols[4],"testfloat");
		assertEquals(cols[5],"testdecimal");
		assertEquals(cols[6],"testnumeric");
		assertEquals(cols[7],"testmoney");
		assertEquals(cols[8],"testsmallmoney");
		assertEquals(cols[9],"testdatetime");
		assertEquals(cols[10],"testsmalldatetime");
		assertEquals(cols[11],"testchar");
		assertEquals(cols[12],"testvarchar");
		assertEquals(cols[13],"testbit");
		System.out.println();


		// cached result set with result set buffer size
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


		// finished suspended session
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
		cur.sendQuery("commit tran");
		cur.sendQuery("drop table testtable");
	
		// invalid queries...


		// invalid queries
		System.out.println("INVALID QUERIES: ");
		assertFalse(cur.sendQuery("select * from testtable order by testint"));
		assertFalse(cur.sendQuery("select * from testtable order by testint"));
		assertFalse(cur.sendQuery("select * from testtable order by testint"));
		assertFalse(cur.sendQuery("select * from testtable order by testint"));
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
