// Copyright (c) David Muse
// See the file COPYING for more information.

import com.firstworks.sqlrelay.SQLRConnection;
import com.firstworks.sqlrelay.SQLRCursor;


class freetds extends sqlrtest {

	public static void	main(String[] args) {

		String[]	isolationlevels={"1","0","2","3"};
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
		char[]		largebuffer=new char[LARGE_BUFFER_LENGTH];

		// hostname
		String	hostname="";
		try {
			hostname=java.net.InetAddress
					.getLocalHost().getHostName().toLowerCase();
			int idx=hostname.indexOf('.');
			if (idx>0) {
				hostname=hostname.substring(0,idx);
			}
		} catch (Exception e) { }
		String	dumptran="dump tran "+hostname+" with truncate_only";


		// instantiation
		SQLRConnection con=new SQLRConnection("sqlrelay",(short)9005,
					"/tmp/freetds.socket","testuser",
					"testpassword",0,1);
		SQLRCursor cur=new SQLRCursor(con);


		// identify
		System.out.println("IDENTIFY: ");
		assertEquals(con.identify(),"freetds");
		System.out.println();


		// ping
		System.out.println("PING: ");
		assertTrue(con.ping());
		System.out.println();


		// transaction state
		System.out.println("TRANSACTION STATE: ");
		assertEquals(con.getDefaultTransactionModel(),"explicit-error");
		assertEquals(con.getTransactionModel(),"explicit-error");
		assertFalse(con.getInTransaction());
		assertTrue(con.getAutoCommit());
		System.out.println();


		// bind format
		System.out.println("BIND FORMAT: ");
		assertEquals(con.bindFormat(),"@*");
		System.out.println();


		// nextval format
		System.out.println("NEXTVAL FORMAT: ");
		assertEquals(con.nextvalFormat(),"%s.nextval");
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
		cur.sendQuery(dumptran);
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	testint int, "+
			"	testsmallint smallint, "+
			"	testtinyint tinyint, "+
			"	testreal real, "+
			"	testfloat float, "+
			"	testdecimal decimal(4,1), "+
			"	testnumeric numeric(4,1), "+
			"	testmoney money, "+
			"	testsmallmoney smallmoney, "+
			"	testdatetime datetime, "+
			"	testsmalldatetime "+
			"		smalldatetime, "+
			"	testchar char(40), "+
			"	testvarchar varchar(40), "+
			"	testbit bit, "+
			"	testdate date, "+
			"	testtime time, "+
			"	testbigdatetime bigdatetime, "+
			"	testbigtime bigtime) "+
			"lock datarows"));
		System.out.println();


		// insert
		System.out.println("INSERT: ");
		assertTrue(con.begin());
		assertTrue(cur.sendQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	1, "+
			"	1, "+
			"	1, "+
			"	1.5, "+
			"	1.5, "+
			"	1.5, "+
			"	1.5, "+
			"	1.00, "+
			"	1.00, "+
			"	'01-Jan-2001 01:00:00', "+
			"	'01-Jan-2001 01:00:00', "+
			"	'testchar1', "+
			"	'testvarchar1', "+
			"	1, "+
			"	'01-Jan-2001', "+
			"	'13:01:01', "+
			"	'01-Jan-2001 13:01:01', "+
			"	'01:01:01.001000')"));
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
			"	?, "+
			"	?, "+
			"	?, "+
			"	?, "+
			"	?, "+
			"	?, "+
			"	?)");
		assertEquals(cur.countBindVariables(),18);
		cur.inputBind("1",2);
		cur.inputBind("2",2);
		cur.inputBind("3",2);
		cur.inputBind("4",2.5,2,1);
		cur.inputBind("5",2.5,2,1);
		cur.inputBind("6",2.5,2,1);
		cur.inputBind("7",2.5,2,1);
		cur.inputBind("8",2.00,3,2);
		cur.inputBind("9",2.00,3,2);
		cur.inputBind("10","01-Jan-2002 02:00:00");
		cur.inputBind("11","01-Jan-2002 02:00:00");
		cur.inputBind("12","testchar2");
		cur.inputBind("13","testvarchar2");
		cur.inputBind("14",1);
		cur.inputBind("15","01-Jan-2001");
		cur.inputBind("16","13:01:01");
		cur.inputBind("17","01-Jan-2001 13:01:01");
		cur.inputBind("18","01:01:01.001000");
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("1",3);
		cur.inputBind("2",3);
		cur.inputBind("3",3);
		cur.inputBind("4",3.5,2,1);
		cur.inputBind("5",3.5,2,1);
		cur.inputBind("6",3.5,2,1);
		cur.inputBind("7",3.5,2,1);
		cur.inputBind("8",3.00,3,2);
		cur.inputBind("9",3.00,3,2);
		cur.inputBind("10","01-Jan-2003 03:00:00");
		cur.inputBind("11","01-Jan-2003 03:00:00");
		cur.inputBind("12","testchar3");
		cur.inputBind("13","testvarchar3");
		cur.inputBind("14",1);
		cur.inputBind("15","01-Jan-2001");
		cur.inputBind("16","13:01:01");
		cur.inputBind("17","01-Jan-2001 13:01:01");
		cur.inputBind("18","01:01:01.001000");
		assertTrue(cur.executeQuery());
		System.out.println();


		// array of input binds by position
		// freetds doesn't support implicit
		// conversion of string binds to other
		// data types, so arrays of binds don't generally work.


		// input bind by position with validation
		System.out.println("INPUT BIND BY POSITION WITH VALIDATION: ");
		cur.clearBinds();
		cur.inputBind("1",4);
		cur.inputBind("2",4);
		cur.inputBind("3",4);
		cur.inputBind("4",4.5,2,1);
		cur.inputBind("5",4.5,2,1);
		cur.inputBind("6",4.5,2,1);
		cur.inputBind("7",4.5,2,1);
		cur.inputBind("8",4.00,3,2);
		cur.inputBind("9",4.00,3,2);
		cur.inputBind("10","01-Jan-2004 04:00:00");
		cur.inputBind("11","01-Jan-2004 04:00:00");
		cur.inputBind("12","testchar4");
		cur.inputBind("13","testvarchar4");
		cur.inputBind("14",1);
		cur.inputBind("15","01-Jan-2001");
		cur.inputBind("16","13:01:01");
		cur.inputBind("17","01-Jan-2001 13:01:01");
		cur.inputBind("18","01:01:01.001000");
		cur.validateBinds();
		assertTrue(cur.executeQuery());
		System.out.println();


		// input bind by name
		System.out.println("INPUT BIND BY NAME: ");
		cur.clearBinds();
		cur.prepareQuery(
			"insert into "+
			"	testtable "+
			"values ("+
			"	@var1, "+
			"	@var2, "+
			"	@var3, "+
			"	@var4, "+
			"	@var5, "+
			"	@var6, "+
			"	@var7, "+
			"	@var8, "+
			"	@var9, "+
			"	@var10, "+
			"	@var11, "+
			"	@var12, "+
			"	@var13, "+
			"	@var14, "+
			"	@var15, "+
			"	@var16, "+
			"	@var17, "+
			"	@var18)");
		assertEquals(cur.countBindVariables(),18);
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
		cur.inputBind("var15","01-Jan-2001");
		cur.inputBind("var16","13:01:01");
		cur.inputBind("var17","01-Jan-2001 13:01:01");
		cur.inputBind("var18","01:01:01.001000");
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("var1",6);
		cur.inputBind("var2",6);
		cur.inputBind("var3",6);
		cur.inputBind("var4",6.5,2,1);
		cur.inputBind("var5",6.5,2,1);
		cur.inputBind("var6",6.5,2,1);
		cur.inputBind("var7",6.5,2,1);
		cur.inputBind("var8",6.00,3,2);
		cur.inputBind("var9",6.00,3,2);
		cur.inputBind("var10","01-Jan-2006 06:00:00");
		cur.inputBind("var11","01-Jan-2006 06:00:00");
		cur.inputBind("var12","testchar6");
		cur.inputBind("var13","testvarchar6");
		cur.inputBind("var14",1);
		cur.inputBind("var15","01-Jan-2001");
		cur.inputBind("var16","13:01:01");
		cur.inputBind("var17","01-Jan-2001 13:01:01");
		cur.inputBind("var18","01:01:01.001000");
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("var1",7);
		cur.inputBind("var2",7);
		cur.inputBind("var3",7);
		cur.inputBind("var4",7.5,2,1);
		cur.inputBind("var5",7.5,2,1);
		cur.inputBind("var6",7.5,2,1);
		cur.inputBind("var7",7.5,2,1);
		cur.inputBind("var8",7.00,3,2);
		cur.inputBind("var9",7.00,3,2);
		cur.inputBind("var10","01-Jan-2007 07:00:00");
		cur.inputBind("var11","01-Jan-2007 07:00:00");
		cur.inputBind("var12","testchar7");
		cur.inputBind("var13","testvarchar7");
		cur.inputBind("var14",1);
		cur.inputBind("var15","01-Jan-2001");
		cur.inputBind("var16","13:01:01");
		cur.inputBind("var17","01-Jan-2001 13:01:01");
		cur.inputBind("var18","01:01:01.001000");
		assertTrue(cur.executeQuery());
		System.out.println();


		// array of input binds by name
		// freetds doesn't support implicit
		// conversion of string binds to other
		// data types, so arrays of binds don't generally work.


		// input bind by name with validation
		System.out.println("INPUT BIND BY NAME WITH VALIDATION: ");
		cur.clearBinds();
		cur.inputBind("var1",8);
		cur.inputBind("var2",8);
		cur.inputBind("var3",8);
		cur.inputBind("var4",8.5,2,1);
		cur.inputBind("var5",8.5,2,1);
		cur.inputBind("var6",8.5,2,1);
		cur.inputBind("var7",8.5,2,1);
		cur.inputBind("var8",8.00,3,2);
		cur.inputBind("var9",8.00,3,2);
		cur.inputBind("var10","01-Jan-2008 08:00:00");
		cur.inputBind("var11","01-Jan-2008 08:00:00");
		cur.inputBind("var12","testchar8");
		cur.inputBind("var13","testvarchar8");
		cur.inputBind("var14",1);
		cur.inputBind("var15","01-Jan-2001");
		cur.inputBind("var16","13:01:01");
		cur.inputBind("var17","01-Jan-2001 13:01:01");
		cur.inputBind("var18","01:01:01.001000");
		cur.inputBind("var19","junkvalue");
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
		assertEquals(cur.colCount(),18);
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
		assertEquals(cur.getColumnName(14),"testdate");
		assertEquals(cur.getColumnName(15),"testtime");
		assertEquals(cur.getColumnName(16),"testbigdatetime");
		assertEquals(cur.getColumnName(17),"testbigtime");
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
		assertEquals(cols[14],"testdate");
		assertEquals(cols[15],"testtime");
		assertEquals(cols[16],"testbigdatetime");
		assertEquals(cols[17],"testbigtime");
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
		assertEquals(cur.getColumnType("testsmalldatetime"),
							"SMALLDATETIME");
		assertEquals(cur.getColumnType(11),"CHAR");
		assertEquals(cur.getColumnType("testchar"),"CHAR");
		assertEquals(cur.getColumnType(12),"VARCHAR");
		assertEquals(cur.getColumnType("testvarchar"),"VARCHAR");
		assertEquals(cur.getColumnType(13),"BIT");
		assertEquals(cur.getColumnType("testbit"),"BIT");
		assertEquals(cur.getColumnType(14),"DATE");
		assertEquals(cur.getColumnType("testdate"),"DATE");
		assertEquals(cur.getColumnType(15),"TIME");
		assertEquals(cur.getColumnType("testtime"),"TIME");
		assertEquals(cur.getColumnType(16),"TIMESTAMP");
		assertEquals(cur.getColumnType("testbigdatetime"),"TIMESTAMP");
		assertEquals(cur.getColumnType(17),"TIME");
		assertEquals(cur.getColumnType("testbigtime"),"TIME");
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
		// freetds reports the decimal/numeric display length as 35
		assertEquals(
			cur.getColumnLength(5),35);
		assertEquals(
			cur.getColumnLength(
				"testdecimal"),35);
		assertEquals(
			cur.getColumnLength(6),35);
		assertEquals(
			cur.getColumnLength(
				"testnumeric"),35);
		assertEquals(cur.getColumnLength(7),8);
		assertEquals(cur.getColumnLength("testmoney"),8);
		assertEquals(cur.getColumnLength(8),4);
		assertEquals(cur.getColumnLength("testsmallmoney"),4);
		assertEquals(cur.getColumnLength(9),8);
		assertEquals(cur.getColumnLength("testdatetime"),8);
		assertEquals(cur.getColumnLength(10),4);
		assertEquals(cur.getColumnLength("testsmalldatetime"),4);
		// char(40)/varchar(40) report the declared length 40 (not multiplied)
		assertEquals(
			cur.getColumnLength(11),40);
		assertEquals(
			cur.getColumnLength(
				"testchar"),40);
		assertEquals(
			cur.getColumnLength(12),40);
		assertEquals(
			cur.getColumnLength(
				"testvarchar"),40);
		assertEquals(cur.getColumnLength(13),1);
		assertEquals(cur.getColumnLength("testbit"),1);
		assertEquals(cur.getColumnLength(14),4);
		assertEquals(cur.getColumnLength("testdate"),4);
		assertEquals(cur.getColumnLength(15),4);
		assertEquals(cur.getColumnLength("testtime"),4);
		assertEquals(cur.getColumnLength(16),8);
		assertEquals(cur.getColumnLength("testbigdatetime"),8);
		assertEquals(cur.getColumnLength(17),8);
		assertEquals(cur.getColumnLength("testbigtime"),8);
		System.out.println();


		// longest column
		System.out.println("LONGEST COLUMN: ");
		assertEquals(cur.getLongest(0),1);
		assertEquals(cur.getLongest("testint"),1);
		assertEquals(cur.getLongest(1),1);
		assertEquals(cur.getLongest("testsmallint"),1);
		assertEquals(cur.getLongest(2),1);
		assertEquals(cur.getLongest("testtinyint"),1);
		assertEquals(cur.getLongest(3),3);
		assertEquals(cur.getLongest("testreal"),3);
		assertEquals(cur.getLongest(4),3);
		assertEquals(cur.getLongest("testfloat"),3);
		assertEquals(cur.getLongest(5),3);
		assertEquals(cur.getLongest("testdecimal"),3);
		assertEquals(cur.getLongest(6),3);
		assertEquals(cur.getLongest("testnumeric"),3);
		assertMoneyLengthEquals(cur.getLongest(7),6);
		assertMoneyLengthEquals(cur.getLongest("testmoney"),6);
		assertMoneyLengthEquals(cur.getLongest(8),6);
		assertMoneyLengthEquals(cur.getLongest("testsmallmoney"),6);
		// freetds datetime rendering for the fixture tds version
		assertEquals(cur.getLongest(9),26);
		assertEquals(
			cur.getLongest("testdatetime"),
			26);
		assertEquals(cur.getLongest(10),26);
		assertEquals(
			cur.getLongest(
				"testsmalldatetime"),26);
		assertEquals(cur.getLongest(11),40);
		assertEquals(cur.getLongest("testchar"),40);
		assertEquals(cur.getLongest(12),12);
		assertEquals(cur.getLongest("testvarchar"),12);
		assertEquals(cur.getLongest(13),1);
		assertEquals(cur.getLongest("testbit"),1);
		// freetds datetime rendering for the fixture tds version
		assertEquals(cur.getLongest(14),26);
		assertEquals(cur.getLongest("testdate"),26);
		assertEquals(cur.getLongest(15),26);
		assertEquals(cur.getLongest("testtime"),26);
		assertEquals(cur.getLongest(16),26);
		assertEquals(cur.getLongest("testbigdatetime"),26);
		assertEquals(cur.getLongest(17),26);
		assertEquals(cur.getLongest("testbigtime"),26);
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
		assertEquals(cur.getField(0,3),"1.5");
		assertEquals(cur.getField(0,4),"1.5");
		assertEquals(cur.getField(0,5),"1.5");
		assertEquals(cur.getField(0,6),"1.5");
		assertMoneyEquals(cur.getField(0,7),"1.0000");
		assertMoneyEquals(cur.getField(0,8),"1.0000");
		// freetds datetime rendering for the fixture tds version
		assertEquals(cur.getField(0,9),
			"Jan  1 2001 01:00:00:000AM");
		assertEquals(cur.getField(0,10),
			"Jan  1 2001 01:00:00:000AM");
		assertEquals(cur.getField(0,11),"testchar1"+
					"                               ");
		assertEquals(cur.getField(0,12),"testvarchar1");
		assertEquals(cur.getField(0,13),"1");
		// freetds datetime rendering for the fixture tds version
		assertEquals(cur.getField(0,14),"Jan  1 2001 00:00:00:000AM");
		assertEquals(cur.getField(0,15),"Jan  1 1900 01:01:01:000PM");
		assertEquals(cur.getField(0,16),"Jan  1 2001 01:01:01:000PM");
		assertEquals(cur.getField(0,17),"Jan  1 1900 01:01:01:001AM");
		System.out.println();
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(7,1),"8");
		assertEquals(cur.getField(7,2),"8");
		assertEquals(cur.getField(7,3),"8.5");
		assertEquals(cur.getField(7,4),"8.5");
		assertEquals(cur.getField(7,5),"8.5");
		assertEquals(cur.getField(7,6),"8.5");
		assertMoneyEquals(cur.getField(7,7),"8.0000");
		assertMoneyEquals(cur.getField(7,8),"8.0000");
		// freetds datetime rendering for the fixture tds version
		assertEquals(cur.getField(7,9),
			"Jan  1 2008 08:00:00:000AM");
		assertEquals(cur.getField(7,10),
			"Jan  1 2008 08:00:00:000AM");
		assertEquals(cur.getField(7,11),"testchar8"+
					"                               ");
		assertEquals(cur.getField(7,12),"testvarchar8");
		assertEquals(cur.getField(7,13),"1");
		// freetds datetime rendering for the fixture tds version
		assertEquals(cur.getField(7,14),"Jan  1 2001 00:00:00:000AM");
		assertEquals(cur.getField(7,15),"Jan  1 1900 01:01:01:000PM");
		assertEquals(cur.getField(7,16),"Jan  1 2001 01:01:01:000PM");
		assertEquals(cur.getField(7,17),"Jan  1 1900 01:01:01:001AM");
		System.out.println();


		// field lengths by index
		System.out.println("FIELD LENGTHS BY INDEX: ");
		assertEquals(cur.getFieldLength(0,0),1);
		assertEquals(cur.getFieldLength(0,1),1);
		assertEquals(cur.getFieldLength(0,2),1);
		assertEquals(cur.getFieldLength(0,3),3);
		assertEquals(cur.getFieldLength(0,4),3);
		assertEquals(cur.getFieldLength(0,5),3);
		assertEquals(cur.getFieldLength(0,6),3);
		assertMoneyLengthEquals(cur.getFieldLength(0,7),6);
		assertMoneyLengthEquals(cur.getFieldLength(0,8),6);
		// freetds datetime rendering for the fixture tds version
		assertEquals(
			cur.getFieldLength(0,9),26);
		assertEquals(
			cur.getFieldLength(0,10),26);
		assertEquals(cur.getFieldLength(0,11),40);
		assertEquals(cur.getFieldLength(0,12),12);
		assertEquals(cur.getFieldLength(0,13),1);
		// freetds datetime rendering for the fixture tds version
		assertEquals(cur.getFieldLength(0,14),26);
		assertEquals(cur.getFieldLength(0,15),26);
		assertEquals(cur.getFieldLength(0,16),26);
		assertEquals(cur.getFieldLength(0,17),26);
		System.out.println();
		assertEquals(cur.getFieldLength(7,0),1);
		assertEquals(cur.getFieldLength(7,1),1);
		assertEquals(cur.getFieldLength(7,2),1);
		assertEquals(cur.getFieldLength(7,3),3);
		assertEquals(cur.getFieldLength(7,4),3);
		assertEquals(cur.getFieldLength(7,5),3);
		assertEquals(cur.getFieldLength(7,6),3);
		assertMoneyLengthEquals(cur.getFieldLength(7,7),6);
		assertMoneyLengthEquals(cur.getFieldLength(7,8),6);
		// freetds datetime rendering for the fixture tds version
		assertEquals(
			cur.getFieldLength(7,9),26);
		assertEquals(
			cur.getFieldLength(7,10),26);
		assertEquals(cur.getFieldLength(7,11),40);
		assertEquals(cur.getFieldLength(7,12),12);
		assertEquals(cur.getFieldLength(7,13),1);
		// freetds datetime rendering for the fixture tds version
		assertEquals(cur.getFieldLength(7,14),26);
		assertEquals(cur.getFieldLength(7,15),26);
		assertEquals(cur.getFieldLength(7,16),26);
		assertEquals(cur.getFieldLength(7,17),26);
		System.out.println();


		// fields by name
		System.out.println("FIELDS BY NAME: ");
		assertEquals(cur.getField(0,"testint"),"1");
		assertEquals(cur.getField(0,"testsmallint"),"1");
		assertEquals(cur.getField(0,"testtinyint"),"1");
		assertEquals(cur.getField(0,"testreal"),"1.5");
		assertEquals(cur.getField(0,"testfloat"),"1.5");
		assertEquals(cur.getField(0,"testdecimal"),"1.5");
		assertEquals(cur.getField(0,"testnumeric"),"1.5");
		assertMoneyEquals(cur.getField(0,"testmoney"),"1.0000");
		assertMoneyEquals(cur.getField(0,"testsmallmoney"),"1.0000");
		// freetds datetime rendering for the fixture tds version
		assertEquals(
			cur.getField(0,"testdatetime"),
			"Jan  1 2001 01:00:00:000AM");
		assertEquals(
			cur.getField(0,
				"testsmalldatetime"),
			"Jan  1 2001 01:00:00:000AM");
		assertEquals(cur.getField(0,"testchar"),"testchar1"+
					"                               ");
		assertEquals(cur.getField(0,"testvarchar"),"testvarchar1");
		assertEquals(cur.getField(0,"testbit"),"1");
		// freetds datetime rendering for the fixture tds version
		assertEquals(cur.getField(0,"testdate"),"Jan  1 2001 00:00:00:000AM");
		assertEquals(cur.getField(0,"testtime"),"Jan  1 1900 01:01:01:000PM");
		assertEquals(cur.getField(0,"testbigdatetime"),
			"Jan  1 2001 01:01:01:000PM");
		assertEquals(cur.getField(0,"testbigtime"),
			"Jan  1 1900 01:01:01:001AM");
		System.out.println();
		assertEquals(cur.getField(7,"testint"),"8");
		assertEquals(cur.getField(7,"testsmallint"),"8");
		assertEquals(cur.getField(7,"testtinyint"),"8");
		assertEquals(cur.getField(7,"testreal"),"8.5");
		assertEquals(cur.getField(7,"testfloat"),"8.5");
		assertEquals(cur.getField(7,"testdecimal"),"8.5");
		assertEquals(cur.getField(7,"testnumeric"),"8.5");
		assertMoneyEquals(cur.getField(7,"testmoney"),"8.0000");
		assertMoneyEquals(cur.getField(7,"testsmallmoney"),"8.0000");
		// freetds datetime rendering for the fixture tds version
		assertEquals(
			cur.getField(7,"testdatetime"),
			"Jan  1 2008 08:00:00:000AM");
		assertEquals(
			cur.getField(7,
				"testsmalldatetime"),
			"Jan  1 2008 08:00:00:000AM");
		assertEquals(cur.getField(7,"testchar"),"testchar8"+
					"                               ");
		assertEquals(cur.getField(7,"testvarchar"),"testvarchar8");
		assertEquals(cur.getField(7,"testbit"),"1");
		// freetds datetime rendering for the fixture tds version
		assertEquals(cur.getField(7,"testdate"),"Jan  1 2001 00:00:00:000AM");
		assertEquals(cur.getField(7,"testtime"),"Jan  1 1900 01:01:01:000PM");
		assertEquals(cur.getField(7,"testbigdatetime"),
			"Jan  1 2001 01:01:01:000PM");
		assertEquals(cur.getField(7,"testbigtime"),
			"Jan  1 1900 01:01:01:001AM");
		System.out.println();


		// field lengths by name
		System.out.println("FIELD LENGTHS BY NAME: ");
		assertEquals(cur.getFieldLength(0,"testint"),1);
		assertEquals(cur.getFieldLength(0,"testsmallint"),1);
		assertEquals(cur.getFieldLength(0,"testtinyint"),1);
		assertEquals(cur.getFieldLength(0,"testreal"),3);
		assertEquals(cur.getFieldLength(0,"testfloat"),3);
		assertEquals(cur.getFieldLength(0,"testdecimal"),3);
		assertEquals(cur.getFieldLength(0,"testnumeric"),3);
		assertMoneyLengthEquals(cur.getFieldLength(0,"testmoney"),6);
		assertMoneyLengthEquals(cur.getFieldLength(0,"testsmallmoney"),6);
		// freetds datetime rendering for the fixture tds version
		assertEquals(
			cur.getFieldLength(0,
				"testdatetime"),26);
		assertEquals(
			cur.getFieldLength(0,
				"testsmalldatetime"),
			26);
		assertEquals(cur.getFieldLength(0,"testchar"),40);
		assertEquals(cur.getFieldLength(0,"testvarchar"),12);
		assertEquals(cur.getFieldLength(0,"testbit"),1);
		// freetds datetime rendering for the fixture tds version
		assertEquals(cur.getFieldLength(0,"testdate"),26);
		assertEquals(cur.getFieldLength(0,"testtime"),26);
		assertEquals(cur.getFieldLength(0,"testbigdatetime"),26);
		assertEquals(cur.getFieldLength(0,"testbigtime"),26);
		System.out.println();
		assertEquals(cur.getFieldLength(7,"testint"),1);
		assertEquals(cur.getFieldLength(7,"testsmallint"),1);
		assertEquals(cur.getFieldLength(7,"testtinyint"),1);
		assertEquals(cur.getFieldLength(7,"testreal"),3);
		assertEquals(cur.getFieldLength(7,"testfloat"),3);
		assertEquals(cur.getFieldLength(7,"testdecimal"),3);
		assertEquals(cur.getFieldLength(7,"testnumeric"),3);
		assertMoneyLengthEquals(cur.getFieldLength(7,"testmoney"),6);
		assertMoneyLengthEquals(cur.getFieldLength(7,"testsmallmoney"),6);
		// freetds datetime rendering for the fixture tds version
		assertEquals(
			cur.getFieldLength(7,
				"testdatetime"),26);
		assertEquals(
			cur.getFieldLength(7,
				"testsmalldatetime"),
			26);
		assertEquals(cur.getFieldLength(7,"testchar"),40);
		assertEquals(cur.getFieldLength(7,"testvarchar"),12);
		assertEquals(cur.getFieldLength(7,"testbit"),1);
		// freetds datetime rendering for the fixture tds version
		assertEquals(cur.getFieldLength(7,"testdate"),26);
		assertEquals(cur.getFieldLength(7,"testtime"),26);
		assertEquals(cur.getFieldLength(7,"testbigdatetime"),26);
		assertEquals(cur.getFieldLength(7,"testbigtime"),26);
		System.out.println();


		// fields by array
		System.out.println("FIELDS BY ARRAY: ");
		fields=cur.getRow(0);
		assertEquals(fields[0],"1");
		assertEquals(fields[1],"1");
		assertEquals(fields[2],"1");
		assertEquals(fields[3],"1.5");
		assertEquals(fields[4],"1.5");
		assertEquals(fields[5],"1.5");
		assertEquals(fields[6],"1.5");
		assertMoneyEquals(fields[7],"1.0000");
		assertMoneyEquals(fields[8],"1.0000");
		// freetds datetime rendering for the fixture tds version
		assertEquals(fields[9],
			"Jan  1 2001 01:00:00:000AM");
		assertEquals(fields[10],
			"Jan  1 2001 01:00:00:000AM");
		assertEquals(fields[11],"testchar1"+
					"                               ");
		assertEquals(fields[12],"testvarchar1");
		assertEquals(fields[13],"1");
		// freetds datetime rendering for the fixture tds version
		assertEquals(fields[14],"Jan  1 2001 00:00:00:000AM");
		assertEquals(fields[15],"Jan  1 1900 01:01:01:000PM");
		assertEquals(fields[16],"Jan  1 2001 01:01:01:000PM");
		assertEquals(fields[17],"Jan  1 1900 01:01:01:001AM");
		System.out.println();


		// field lengths by array
		System.out.println("FIELD LENGTHS BY ARRAY: ");
		fieldlens=cur.getRowLengths(0);
		assertEquals(fieldlens[0],1);
		assertEquals(fieldlens[1],1);
		assertEquals(fieldlens[2],1);
		assertEquals(fieldlens[3],3);
		assertEquals(fieldlens[4],3);
		assertEquals(fieldlens[5],3);
		assertEquals(fieldlens[6],3);
		assertMoneyLengthEquals(fieldlens[7],6);
		assertMoneyLengthEquals(fieldlens[8],6);
		// freetds datetime rendering for the fixture tds version
		assertEquals(fieldlens[9],26);
		assertEquals(fieldlens[10],26);
		assertEquals(fieldlens[11],40);
		assertEquals(fieldlens[12],12);
		assertEquals(fieldlens[13],1);
		// freetds datetime rendering for the fixture tds version
		assertEquals(fieldlens[14],26);
		assertEquals(fieldlens[15],26);
		assertEquals(fieldlens[16],26);
		assertEquals(fieldlens[17],26);
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
		assertEquals(cur.getColumnType(0),"INT");
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
		cur.cacheToFile("cachefile1-freetds");
		cur.setCacheTtl(200);
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testint "));
		filename=cur.getCacheFileName();
		assertEquals(filename,"cachefile1-freetds");
		cur.cacheOff();
		assertTrue(cur.openCachedResultSet(filename));
		assertEquals(cur.getField(7,0),"8");
		System.out.println();


		// column count for cached result set
		System.out.println("COLUMN COUNT FOR CACHED RESULT SET: ");
		assertEquals(cur.colCount(),18);
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
		assertEquals(cur.getColumnName(14),"testdate");
		assertEquals(cur.getColumnName(15),"testtime");
		assertEquals(cur.getColumnName(16),"testbigdatetime");
		assertEquals(cur.getColumnName(17),"testbigtime");
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
		assertEquals(cols[14],"testdate");
		assertEquals(cols[15],"testtime");
		assertEquals(cols[16],"testbigdatetime");
		assertEquals(cols[17],"testbigtime");
		System.out.println();


		// cached result set with result set buffer size
		System.out.println("CACHED RESULT SET WITH "+
			"RESULT SET BUFFER SIZE: ");
		cur.setResultSetBufferSize(2);
		cur.cacheToFile("cachefile1-freetds");
		cur.setCacheTtl(200);
		assertTrue(cur.sendQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testint "));
		filename=cur.getCacheFileName();
		assertEquals(filename,"cachefile1-freetds");
		cur.cacheOff();
		assertTrue(cur.openCachedResultSet(filename));
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(8,0),null);
		cur.setResultSetBufferSize(0);
		System.out.println();


		// from one cache file to another
		System.out.println("FROM ONE CACHE FILE TO ANOTHER: ");
		cur.cacheToFile("cachefile2-freetds");
		assertTrue(cur.openCachedResultSet("cachefile1-freetds"));
		cur.cacheOff();
		assertTrue(cur.openCachedResultSet("cachefile2-freetds"));
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(8,0),null);
		System.out.println();


		// from one cache file to another with result set buffer size
		System.out.println("FROM ONE CACHE FILE TO ANOTHER "+
			"WITH RESULT SET BUFFER SIZE: ");
		cur.setResultSetBufferSize(2);
		cur.cacheToFile("cachefile2-freetds");
		assertTrue(cur.openCachedResultSet("cachefile1-freetds"));
		cur.cacheOff();
		assertTrue(cur.openCachedResultSet("cachefile2-freetds"));
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(8,0),null);
		cur.setResultSetBufferSize(0);
		System.out.println();


		// cached result set with suspend and result set buffer size
		System.out.println("CACHED RESULT SET WITH SUSPEND "+
			"AND RESULT SET BUFFER SIZE: ");
		cur.setResultSetBufferSize(2);
		cur.cacheToFile("cachefile1-freetds");
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
		assertEquals(filename,"cachefile1-freetds");
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


		// nested selects
		System.out.println("NESTED SELECTS: ");
		// can't do this with freetds
		//cur.setResultSetBufferSize(1);
		assertTrue(cur.sendQuery("select * from testtable"));
		SQLRCursor secondcur2=new SQLRCursor(con);
		secondcur2.setResultSetBufferSize(1);
		for (long i=0; cur.getRow(i)!=null; i++) {
			assertTrue(secondcur2.sendQuery(
				"select "+
				"	* "+
				"from "+
				"	testtable"));
		}
		secondcur2.closeResultSet();
		//cur.setResultSetBufferSize(0);
		assertTrue(con.commit());
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// reset transaction state
		System.out.println("RESET TRANSACTION STATE: ");
		assertTrue(con.commit());
		assertEquals(con.getTransactionModel(),"explicit-error");
		assertTrue(con.getAutoCommit());
		System.out.println();


		// transaction behavior - implicit
		System.out.println("TRANSACTION BEHAVIOR - implicit: ");
		// sap ase rejects DDL inside a chained-mode (multi-statement) tx
		// unless `sp_dboption ... 'ddl in tran', true` is set on the db;
		// create the table while still in unchained mode, then switch.
		// `lock datarows` is needed so secondcur's count(*) scan doesn't
		// block on the writer's page lock from the in-flight insert
		assertTrue(cur.sendQuery(
			"create table testtable (col1 integer) lock datarows"));
		assertTrue(con.setTransactionModel("implicit"));
		assertEquals(con.getTransactionModel(),"implicit");
		SQLRConnection secondcon=new SQLRConnection("sqlrelay",
				(short)9005,"/tmp/freetds.socket","testuser",
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
		// switch back to unchained mode so the drop isn't rejected
		assertTrue(con.setTransactionModel("explicit-error"));
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// transaction behavior - explicit
		System.out.println("TRANSACTION BEHAVIOR - explicit: ");
		assertTrue(con.setTransactionModel("explicit"));
		assertEquals(con.getTransactionModel(),"explicit");
		assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"));
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
		// switch back to unchained mode so the drop isn't rejected
		assertTrue(con.setTransactionModel("explicit-error"));
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
		assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"));
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
		// switch back to unchained mode so the drop isn't rejected
		assertTrue(con.setTransactionModel("explicit-error"));
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// transaction behavior - explicit-error
		System.out.println("TRANSACTION BEHAVIOR - explicit-error: ");
		assertTrue(con.setTransactionModel("explicit-error"));
		assertEquals(con.getTransactionModel(),"explicit-error");
		assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"));
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
		// commit the open tx so the drop isn't rejected as DDL inside a
		// chained-mode transaction (in explicit-error model, autoCommitOn
		// from inside a tx errors out by design, so commit is the route
		// back to autocommit-on / unchained mode)
		assertTrue(con.commit());
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// transaction behavior - none
		System.out.println("TRANSACTION BEHAVIOR - none: ");
		assertTrue(con.setTransactionModel("none"));
		assertEquals(con.getTransactionModel(),"none");
		assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"));
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
		assertEquals(con.getTransactionModel(),"explicit-error");
		assertTrue(con.getAutoCommit());
		System.out.println();


		// individual substitutions
		System.out.println("INDIVIDUAL SUBSTITUTIONS: ");
		cur.prepareQuery("select $(var1),'$(var2)',$(var3)");
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
		cur.prepareQuery("select $(var1),$(var2),$(var3)");
		cur.substitutions(subvars,subvallongs);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"2");
		assertEquals(cur.getField(0,2),"3");
		System.out.println();
		cur.prepareQuery("select '$(var1)','$(var2)','$(var3)'");
		cur.substitutions(subvars,subvalstrings);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"hi");
		assertEquals(cur.getField(0,1),"hello");
		assertEquals(cur.getField(0,2),"bye");
		System.out.println();
		cur.prepareQuery("select $(var1),$(var2),$(var3)");
		cur.substitutions(subvars,subvaldoubles,precs,scales);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getField(0,0),"10.55");
		assertEquals(cur.getField(0,1),"10.556");
		assertEquals(cur.getField(0,2),"10.5556");
		System.out.println();


		// nulls as nulls
		System.out.println("NULLS AS NULLS: ");
		cur.getNullsAsNulls();
		assertTrue(cur.sendQuery("select NULL,1,NULL"));
		assertEquals(cur.getField(0,0),null);
		assertEquals(cur.getField(0,1),"1");
		assertEquals(cur.getField(0,2),null);
		cur.getNullsAsEmptyStrings();
		assertTrue(cur.sendQuery("select NULL,1,NULL"));
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
			"	testclob1 text NULL, "+
			"	testclob2 text NULL, "+
			"	testblob1 image NULL, "+
			"	testblob2 image NULL)"));
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
		// sap converts empty strings to a
		// single space.  It's possible that
		// if we had true input bind support on
		// the backend, then this would work
		// correctly, but for now we're faking
		// binds, and inserting an empty string,
		// so we have to check for a single space here.
		assertEquals(cur.getField(0,0)," ");
		assertEquals(cur.getField(0,1),null);
		// sap doesn't really support inserting
		// an empty string into a binary column.
		// The minimum that can be inserted is a
		// single \0.  That ends up being
		// interpreted as an empty string here,
		// but it's actually a single \0 character, not zero characters.
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
			"	testclob text, "+
			"	testblob image) "+
			"lock datarows");
		cur.prepareQuery("insert into testtable values (?,?)");
		for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
			largebuffer[i]='C';
		}
		String	largestr=new String(largebuffer);
		cur.inputBindClob("1",largestr,LARGE_BUFFER_LENGTH);
		cur.inputBindBlob("2",largestr.getBytes(),LARGE_BUFFER_LENGTH);
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


		// output bind by position FreeTDS needs to support cursors
		// for this to work
		/*System.out.println("OUTPUT BIND BY POSITION: ");
		cur.sendQuery("drop procedure testproc");
		cur.getNullsAsNulls();
		assertTrue(cur.sendQuery(
			"create procedure testproc "+
			"	@out1 int output, "+
			"	@out2 varchar(20) "+
			"		output, "+
			"	@out3 float output, "+
			"	@out4 datetime output, "+
			"	@out5 varchar(20) "+
			"		output as "+
			"select @out1=1, "+
			"	@out2='hello', "+
			"	@out3=2.5, "+
			"	@out4='2001-02-03', "+
			"	@out5=null"));
		cur.prepareQuery("exec testproc");
		assertEquals(cur.countBindVariables(),0);
		cur.defineOutputBindInteger("1");
		cur.defineOutputBindString("2",20);
		cur.defineOutputBindDouble("3");
		cur.defineOutputBindDate("4");
		cur.defineOutputBindString("5",20);
		assertTrue(cur.executeQuery());
		numvar=cur.getOutputBindInteger("1");
		stringvar=cur.getOutputBindString("2");
		floatvar=cur.getOutputBindDouble("3");
		cur.getOutputBindDate("4",year,month,day,hour,minute,second,
			microsecond,tz,isnegative);
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
		nullvar=cur.getOutputBindString("5");
		assertEquals(nullvar,null);
		cur.getNullsAsEmptyStrings();
		assertTrue(cur.sendQuery("drop procedure testproc"));
		System.out.println();
		*/


		// output bind by name FreeTDS needs to support cursors
		// for this to work
		/*System.out.println("OUTPUT BIND BY NAME: ");
		cur.sendQuery("drop procedure testproc");
		cur.getNullsAsNulls();
		assertTrue(cur.sendQuery(
			"create procedure testproc "+
			"	@out1 int output, "+
			"	@out2 varchar(20) "+
			"		output, "+
			"	@out3 float output, "+
			"	@out4 datetime output, "+
			"	@out5 varchar(20) "+
			"		output as "+
			"select @out1=1, "+
			"	@out2='hello', "+
			"	@out3=2.5, "+
			"	@out4='2001-02-03', "+
			"	@out5=null"));
		cur.prepareQuery("exec testproc");
		assertEquals(cur.countBindVariables(),0);
		cur.defineOutputBindInteger("out1");
		cur.defineOutputBindString("out2",20);
		cur.defineOutputBindDouble("out3");
		cur.defineOutputBindDate("out4");
		cur.defineOutputBindString("out5",20);
		assertTrue(cur.executeQuery());
		numvar=cur.getOutputBindInteger("out1");
		stringvar=cur.getOutputBindString("out2");
		floatvar=cur.getOutputBindDouble("out3");
		cur.getOutputBindDate("out4",year,month,day,hour,minute,second,
			microsecond,tz,isnegative);
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
		nullvar=cur.getOutputBindString("out5");
		assertEquals(nullvar,null);
		cur.getNullsAsEmptyStrings();
		assertTrue(cur.sendQuery("drop procedure testproc"));
		System.out.println();
		*/


		// output bind by name with validation
		// Even if FreeTDS supported cursors...
		// validateBinds() can't be used for
		// output binds, with sap.  In sap,
		// when executing a procedure, you don't
		// declare any bind variable delimiters
		// in the query.  eg, you just do: "exec testproc", not
		// "exec testproc(@out1,@out2)".
		// If you call validateBinds(), it won't
		// find any binds in the query, and will
		// filter out any binds that you declare.


		// lob output bind
		// sap doesn't support lobs as output
		// parameters to stored procedures,
		// and there's no way to directly select
		// into a lob bind variable


		// long output bind FreeTDS needs to support cursors
		// for this to work
		/*System.out.println("LONG OUTPUT BIND: ");
		cur.sendQuery("drop procedure testproc");
		for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
			largebuffer[i]='C';
		}
		String	query="create procedure testproc @bindval varchar("+
			LARGE_BUFFER_LENGTH+") output as set @bindval='"+
			new String(largebuffer)+"'";
		assertTrue(cur.sendQuery(query));
		cur.prepareQuery("exec testproc");
		cur.defineOutputBindString("bindval",LARGE_BUFFER_LENGTH);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindLength("bindval"),
			LARGE_BUFFER_LENGTH);
		assertEquals(cur.getOutputBindString("bindval"),
			new String(largebuffer));
		assertTrue(cur.sendQuery("drop procedure testproc"));
		System.out.println();
		*/


		// negative input bind
		System.out.println("NEGATIVE INPUT BIND: ");
		cur.sendQuery("drop table testtable");
		cur.sendQuery("create table testtable (testval int)");
		cur.prepareQuery("insert into testtable values (@testval)");
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
		cur.substitution("var1","@var1");
		assertTrue(cur.validBind("var1"));
		assertFalse(cur.validBind("var2"));
		assertFalse(cur.validBind("var3"));
		assertFalse(cur.validBind("var4"));
		System.out.println();
		cur.substitution("var2","@var2");
		assertTrue(cur.validBind("var1"));
		assertTrue(cur.validBind("var2"));
		assertFalse(cur.validBind("var3"));
		assertFalse(cur.validBind("var4"));
		System.out.println();
		cur.substitution("var3","@var3");
		assertTrue(cur.validBind("var1"));
		assertTrue(cur.validBind("var2"));
		assertTrue(cur.validBind("var3"));
		assertFalse(cur.validBind("var4"));
		assertTrue(cur.executeQuery());
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// rebinding FreeTDS needs to support cursors for this to work
		/*System.out.println("REBINDING: ");
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery(
			"create procedure testproc "+
			"	@in1 int, "+
			"	@out1 int output as "+
			"select @out1=@in1"));
		cur.prepareQuery("exec testproc");
		cur.inputBind("in1",1);
		cur.defineOutputBindInteger("out1");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("out1"),1);
		cur.inputBind("in1",2);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("out1"),2);
		cur.inputBind("in1",3);
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("out1"),3);
		assertTrue(cur.sendQuery("drop procedure testproc"));
		System.out.println();
		*/


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
		cur.prepareQuery("select cast(? as int)");
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
		// FreeTDS needs to support cursors for this to work
		/*System.out.println("STORED PROCEDURE RETURNING NO VALUE: ");
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery(
			"create procedure testproc "+
			"	@in1 int, "+
			"	@in2 float, "+
			"	@in3 varchar(20) as "+
			"return"));
		cur.prepareQuery("exec testproc");
		cur.inputBind("in1",1);
		cur.inputBind("in2",2.5,2,1);
		cur.inputBind("in3","hello");
		assertTrue(cur.executeQuery());
		assertTrue(cur.sendQuery("drop procedure testproc"));
		System.out.println();
		*/


		// stored procedure returning single value
		// FreeTDS needs to support cursors for this to work
		/*System.out.println("STORED PROCEDURE "+
			"RETURNING SINGLE VALUE: ");
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery(
			"create procedure testproc "+
			"	@in1 int, "+
			"	@in2 float, "+
			"	@in3 varchar(20), "+
			"	@out1 int output as "+
			"select @out1=@in1"));
		cur.prepareQuery("exec testproc");
		cur.inputBind("in1",1);
		cur.inputBind("in2",2.5,2,1);
		cur.inputBind("in3","hello");
		cur.defineOutputBindInteger("out1");
		assertTrue(cur.executeQuery());
		assertEquals(cur.getOutputBindInteger("out1"),1);
		assertTrue(cur.sendQuery("drop procedure testproc"));
		System.out.println();
		*/


		// stored procedure returning multiple values
		// FreeTDS needs to support cursors for this to work
		/*System.out.println("STORED PROCEDURE "+
			"RETURNING MULTIPLE VALUES: ");
		cur.sendQuery("drop procedure testproc");
		assertTrue(cur.sendQuery("create procedure testproc @in1 int, "+
			"       @in2 float,        @in3 varchar(20), "+
			"       @out1 int output,        @out2 float output, "+
			"       @out3 varchar(20) "+
			"		output as "+
			"select @out1=@in1,        @out2=@in2, "+
			"       @out3=@in3"));
		cur.prepareQuery("exec testproc");
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
		*/


		// stored procedure returning result set
		System.out.println("STORED PROCEDURE RETURNING RESULT SET: ");
		cur.sendQuery("drop procedure testselectproc");
		assertTrue(cur.sendQuery(
			"create procedure testselectproc as "+
			"	select 1 "+
			"	union "+
			"	select 2 "+
			"	union "+
			"	select 3 "+
			"	union "+
			"	select 4 "+
			"	union "+
			"	select 5 "+
			"	union "+
			"	select 6 "+
			"	union "+
			"	select 7 "+
			"	union "+
			"	select 8"));
		assertTrue(cur.sendQuery("exec testselectproc"));
		assertEquals(cur.rowCount(),8);
		assertTrue(cur.sendQuery("drop procedure testselectproc"));
		System.out.println();


		// temporary tables
		System.out.println("TEMPORARY TABLES: ");
		cur.sendQuery("drop table #temptable");
		cur.sendQuery("create table #temptable (col1 int)");
		assertTrue(cur.sendQuery("insert into #temptable values (1)"));
		assertTrue(cur.sendQuery("select count(*) from #temptable"));
		assertEquals(cur.getField(0,0),"1");
		con.endSession();
		System.out.println();
		assertFalse(cur.sendQuery("select count(*) from #temptable"));
		System.out.println();


		// encoded binary data
		System.out.println("ENCODED BINARY DATA: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 image)"));
		byte[]	buffer=new byte[256];
		for (int i=0; i<256; i++) {
			buffer[i]=(byte)i;
		}
		StringBuilder	query=new StringBuilder();
		query.append("insert into testtable values (0x");
		for (int i=0; i<buffer.length; i++) {
			query.append(String.format("%02x",buffer[i]&0xff));
		}
		query.append(")");
		assertTrue(cur.sendQuery(query.toString()));
		assertTrue(cur.sendQuery("select col1 from testtable"));
		assertEquals(cur.getFieldLength(0,0),buffer.length);
		assertEquals(cur.getFieldAsByteArray(0,0),buffer,buffer.length);
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
			"	col1 int identity "+
			"primary key, "+
			"	col2 int)"));
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
		assertFalse(con.getDatabaseIsSchema());
		System.out.println();


		// catalog list
		System.out.println("CATALOG LIST: ");
		assertTrue(cur.getCatalogList(null));
		assertEquals(cur.getColumnName(0),"Database");
		assertInResultSet(cur,"Database",hostname);
		System.out.println();


		// schema list
		System.out.println("SCHEMA LIST: ");
		cur.sendQuery("drop table testtable");
		// the get schema list query that is
		// used with sap will only return the
		// names of schemas that have at least
		// one database object in them, so to be
		// sure that there is one, we'll create a table
		assertTrue(cur.sendQuery("create table testtable (col1 int)"));
		assertTrue(cur.getSchemaList(null));
		assertEquals(cur.getColumnName(0),"Database");
		assertInResultSet(cur,"Database","dbo");
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// table type list
		System.out.println("TABLE TYPE LIST: ");
		assertTrue(cur.getTableTypeList());
		assertEquals(cur.getColumnName(0),"table_type");
		assertInResultSet(cur,"table_type","TABLE");
		System.out.println();


		// table list
		System.out.println("TABLE LIST: ");
		cur.sendQuery("drop table testtable1");
		cur.sendQuery("drop table testtable2");
		cur.sendQuery("drop table testtable3");
		cur.sendQuery("drop table testtable4");
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
		assertTrue(cur.sendQuery("drop table testtable1"));
		assertTrue(cur.sendQuery("drop table testtable2"));
		assertTrue(cur.sendQuery("drop table testtable3"));
		assertTrue(cur.sendQuery("drop table testtable4"));
		System.out.println();


		// type info list
		System.out.println("TYPE INFO LIST: ");
		assertTrue(cur.getTypeInfoList("int"));
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
		assertEquals(cur.getField(0,"type_name"),"INT");
		assertEquals(cur.getField(0,"data_type"),"4");
		assertEquals(cur.getField(0,"precision"),"10");
		assertEquals(cur.getField(0,"local_type_name"),"INT");
		assertTrue(cur.getTypeInfoList("char"));
		assertEquals(cur.getField(0,"type_name"),"CHAR");
		assertEquals(cur.getField(0,"data_type"),"1");
		assertEquals(cur.getField(0,"precision"),"8000");
		assertEquals(cur.getField(0,"local_type_name"),"CHAR");
		assertTrue(cur.getTypeInfoList("varchar"));
		assertEquals(cur.getField(0,"type_name"),"VARCHAR");
		assertEquals(cur.getField(0,"data_type"),"12");
		assertEquals(cur.getField(0,"precision"),"8000");
		assertEquals(cur.getField(0,"local_type_name"),"VARCHAR");
		assertTrue(cur.getTypeInfoList("datetime"));
		assertEquals(cur.getField(0,"type_name"),"DATETIME");
		assertEquals(cur.getField(0,"data_type"),"93");
		assertEquals(cur.getField(0,"precision"),"23");
		assertEquals(cur.getField(0,"local_type_name"),"DATETIME");
		System.out.println();


		// column list
		System.out.println("COLUMN LIST: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	testint int, "+
			"	testsmallint smallint, "+
			"	testtinyint tinyint, "+
			"	testreal real, "+
			"	testfloat float, "+
			"	testdecimal decimal(4,1), "+
			"	testnumeric numeric(4,1), "+
			"	testmoney money, "+
			"	testsmallmoney "+
			"		smallmoney, "+
			"	testdatetime datetime, "+
			"	testsmalldatetime "+
			"		smalldatetime, "+
			"	testchar char(40), "+
			"	testvarchar varchar(40), "+
			"	testbit bit, "+
			"	testdate date, "+
			"	testtime time, "+
			"	testbigdatetime bigdatetime, "+
			"	testbigtime bigtime)"));
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
		assertEquals(cur.getField(1,"column_name"),"testsmallint");
		assertEquals(cur.getField(2,"column_name"),"testtinyint");
		assertEquals(cur.getField(3,"column_name"),"testreal");
		assertEquals(cur.getField(4,"column_name"),"testfloat");
		assertEquals(cur.getField(5,"column_name"),"testdecimal");
		assertEquals(cur.getField(6,"column_name"),"testnumeric");
		assertEquals(cur.getField(7,"column_name"),"testmoney");
		assertEquals(cur.getField(8,"column_name"),"testsmallmoney");
		assertEquals(cur.getField(9,"column_name"),"testdatetime");
		assertEquals(cur.getField(10,"column_name"),
			"testsmalldatetime");
		assertEquals(cur.getField(11,"column_name"),"testchar");
		assertEquals(cur.getField(12,"column_name"),"testvarchar");
		assertEquals(cur.getField(13,"column_name"),"testbit");
		assertEquals(cur.getField(14,"column_name"),"testdate");
		assertEquals(cur.getField(15,"column_name"),"testtime");
		assertEquals(cur.getField(16,"column_name"),"testbigdatetime");
		assertEquals(cur.getField(17,"column_name"),"testbigtime");
		assertEquals(cur.getField(0,"data_type"),"int");
		assertEquals(cur.getField(1,"data_type"),"smallint");
		assertEquals(cur.getField(2,"data_type"),"tinyint");
		assertEquals(cur.getField(3,"data_type"),"real");
		assertEquals(cur.getField(4,"data_type"),"float");
		assertEquals(cur.getField(5,"data_type"),"decimal");
		assertEquals(cur.getField(6,"data_type"),"numeric");
		assertEquals(cur.getField(7,"data_type"),"money");
		assertEquals(cur.getField(8,"data_type"),"smallmoney");
		assertEquals(cur.getField(9,"data_type"),"datetime");
		assertEquals(cur.getField(10,"data_type"),"smalldatetime");
		assertEquals(cur.getField(11,"data_type"),"char");
		assertEquals(cur.getField(12,"data_type"),"varchar");
		assertEquals(cur.getField(13,"data_type"),"bit");
		assertEquals(cur.getField(14,"data_type"),"date");
		assertEquals(cur.getField(15,"data_type"),"time");
		assertEquals(cur.getField(16,"data_type"),"bigdatetime");
		assertEquals(cur.getField(17,"data_type"),"bigtime");
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// column list - auto_increment, primary key
		System.out.println("COLUMN LIST - auto_increment, "+
			"primary key: ");
		cur.sendQuery("drop table testtable");
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 int identity "+
			"primary key, "+
			"	col2 int)"));
		assertTrue(cur.getColumnList("testtable",null));
		assertEquals(cur.getField(0,"extra"),"auto_increment");
		assertEquals(cur.getField(0,"column_key"),"PRI");
		assertEquals(cur.getField(1,"extra"),"");
		assertEquals(cur.getField(1,"column_key"),"");
		System.out.println();
		assertTrue(cur.sendQuery("drop table testtable"));
		assertTrue(cur.sendQuery(
			"create table testtable ("+
			"	col1 int primary key, "+
			"	col2 int)"));
		assertTrue(cur.getColumnList("testtable",null));
		assertEquals(cur.getField(0,"extra"),"");
		assertEquals(cur.getField(0,"column_key"),"PRI");
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// primary keys list
		System.out.println("PRIMARY KEYS LIST: ");
		cur.sendQuery("drop table testtable");
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
		assertStartsWith(cur.getField(0,"key_name"),"testtable_col1_");
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// key and index list
		System.out.println("KEY AND INDEX LIST: ");
		cur.sendQuery("drop table testtable");
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
		assertEquals(cur.getField(0,"index_type"),"1");
		assertStartsWith(cur.getField(0,"key_name"),"testtable_col1_");
		assertTrue(cur.sendQuery("drop table testtable"));
		System.out.println();


		// procedure list
		System.out.println("PROCEDURE LIST: ");
		cur.sendQuery("drop procedure testproc1");
		cur.sendQuery("drop procedure testproc2");
		cur.sendQuery("drop procedure testproc3");
		cur.sendQuery("drop procedure testproc4");
		assertTrue(cur.sendQuery(
			"create procedure testproc1 "+
			"	@in1 int, "+
			"	@in2 char(20), "+
			"	@in3 varchar(20), "+
			"	@in4 datetime "+
			"as select 1"));
		assertTrue(cur.sendQuery(
			"create procedure testproc2 "+
			"	@in1 int, "+
			"	@in2 char(20), "+
			"	@in3 varchar(20), "+
			"	@in4 datetime "+
			"as select 1"));
		assertTrue(cur.sendQuery(
			"create procedure testproc3 "+
			"	@in1 int, "+
			"	@in2 char(20), "+
			"	@in3 varchar(20), "+
			"	@in4 datetime "+
			"as select 1"));
		assertTrue(cur.sendQuery(
			"create procedure testproc4 "+
			"	@in1 int, "+
			"	@in2 char(20), "+
			"	@in3 varchar(20), "+
			"	@in4 datetime "+
			"as select 1"));
		assertTrue(cur.getProcedureList(null));
		assertInResultSet(cur,"routine_name","testproc1");
		assertInResultSet(cur,"routine_name","testproc2");
		assertInResultSet(cur,"routine_name","testproc3");
		assertInResultSet(cur,"routine_name","testproc4");
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
		assertEquals(cur.getField(0,"parameter_name"),"@in1");
		assertEquals(cur.getField(0,"parameter_mode"),"1");
		assertEquals(cur.getField(0,"data_type"),"int");
		assertEquals(cur.getField(0,"ordinal_position"),"1");
		assertEquals(cur.getField(1,"parameter_name"),"@in2");
		assertEquals(cur.getField(1,"parameter_mode"),"1");
		assertEquals(cur.getField(1,"data_type"),"char");
		assertEquals(cur.getField(1,"ordinal_position"),"2");
		assertEquals(cur.getField(2,"parameter_name"),"@in3");
		assertEquals(cur.getField(2,"parameter_mode"),"1");
		assertEquals(cur.getField(2,"data_type"),"varchar");
		assertEquals(cur.getField(2,"ordinal_position"),"3");
		assertEquals(cur.getField(3,"parameter_name"),"@in4");
		assertEquals(cur.getField(3,"parameter_mode"),"1");
		assertEquals(cur.getField(3,"data_type"),"datetime");
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
