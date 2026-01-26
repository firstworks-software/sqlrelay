// Copyright (c) David Muse
// See the file COPYING for more information.

import com.firstworks.sqlrelay.SQLRConnection;
import com.firstworks.sqlrelay.SQLRCursor;


class mysql extends sqlrtest {
	
	public static void	main(String[] args) {

		String	dbtype;
		String[]	isolationlevels={"REPEATABLE-READ","READ-UNCOMMITTED","READ-COMMITTED","SERIALIZABLE"};
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
		assertEquals(con.identify(),"mysql");
		System.out.println();

		// get the db version
		String	dbversion=con.dbVersion();
		int	majorversion=dbversion.charAt(0)-'0';
	
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
	
		// create a new table
		System.out.println("CREATE TEMPTABLE: ");
		assertTrue(cur.sendQuery("create table testtable (testtinyint tinyint, testsmallint smallint, testmediumint mediumint, testint int, testbigint bigint, testfloat float, testreal real, testdecimal decimal(2,1), testdate date, testtime time, testdatetime datetime, testyear year, testchar char(40), testtext text, testvarchar varchar(40), testtinytext tinytext, testmediumtext mediumtext, testlongtext longtext, testtimestamp timestamp)"));
		System.out.println();

		System.out.println("BEGIN TRANSACTION:");
		assertTrue(cur.sendQuery("begin"));
		System.out.println();
	
		System.out.println("INSERT: ");
		assertTrue(cur.sendQuery("insert into testtable values (1,1,1,1,1,1.1,1.1,1.1,'2001-01-01','01:00:00','2001-01-01 01:00:00','2001','char1','text1','varchar1','tinytext1','mediumtext1','longtext1',null)"));
		assertTrue(cur.sendQuery("insert into testtable values (2,2,2,2,2,2.1,2.1,2.1,'2002-01-01','02:00:00','2002-01-01 02:00:00','2002','char2','text2','varchar2','tinytext2','mediumtext2','longtext2',null)"));
		assertTrue(cur.sendQuery("insert into testtable values (3,3,3,3,3,3.1,3.1,3.1,'2003-01-01','03:00:00','2003-01-01 03:00:00','2003','char3','text3','varchar3','tinytext3','mediumtext3','longtext3',null)"));
		assertTrue(cur.sendQuery("insert into testtable values (4,4,4,4,4,4.1,4.1,4.1,'2004-01-01','04:00:00','2004-01-01 04:00:00','2004','char4','text4','varchar4','tinytext4','mediumtext4','longtext4',null)"));
		System.out.println();
	
		System.out.println("AFFECTED ROWS: ");
		assertEquals(cur.affectedRows(),1);
		System.out.println();
	
		System.out.println("BIND BY POSITION: ");
		cur.prepareQuery("insert into testtable values (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,null)");
		assertEquals(cur.countBindVariables(),18);
		cur.inputBind("1",5);
		cur.inputBind("2",5);
		cur.inputBind("3",5);
		cur.inputBind("4",5);
		cur.inputBind("5",5);
		cur.inputBind("6",5.1,2,1);
		cur.inputBind("7",5.1,2,1);
		cur.inputBind("8",5.1,2,1);
		cur.inputBind("9","2005-01-01");
		cur.inputBind("10","05:00:00");
		cur.inputBind("11","2005-01-01 05:00:00");
		cur.inputBind("12","2005");
		cur.inputBind("13","char5");
		cur.inputBind("14","text5");
		cur.inputBind("15","varchar5");
		cur.inputBind("16","tinytext5");
		cur.inputBind("17","mediumtext5");
		cur.inputBind("18","longtext5");
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("1",6);
		cur.inputBind("2",6);
		cur.inputBind("3",6);
		cur.inputBind("4",6);
		cur.inputBind("5",6);
		cur.inputBind("6",6.1,2,1);
		cur.inputBind("7",6.1,2,1);
		cur.inputBind("8",6.1,2,1);
		cur.inputBind("9","2006-01-01");
		cur.inputBind("10","06:00:00");
		cur.inputBind("11","2006-01-01 06:00:00");
		cur.inputBind("12","2006");
		cur.inputBind("13","char6");
		cur.inputBind("14","text6");
		cur.inputBind("15","varchar6");
		cur.inputBind("16","tinytext6");
		cur.inputBind("17","mediumtext6");
		cur.inputBind("18","longtext6");
		assertTrue(cur.executeQuery());
		cur.clearBinds();
		cur.inputBind("1",7);
		cur.inputBind("2",7);
		cur.inputBind("3",7);
		cur.inputBind("4",7);
		cur.inputBind("5",7);
		cur.inputBind("6",7.1,2,1);
		cur.inputBind("7",7.1,2,1);
		cur.inputBind("8",7.1,2,1);
		cur.inputBind("9","2007-01-01");
		cur.inputBind("10","07:00:00");
		cur.inputBind("11","2007-01-01 07:00:00");
		cur.inputBind("12","2007");
		cur.inputBind("13","char7");
		cur.inputBind("14","text7");
		cur.inputBind("15","varchar7");
		cur.inputBind("16","tinytext7");
		cur.inputBind("17","mediumtext7");
		cur.inputBind("18","longtext7");
		assertTrue(cur.executeQuery());
		System.out.println();
	
		System.out.println("BIND BY POSITION WITH VALIDATION: ");
		cur.clearBinds();
		cur.inputBind("1",8);
		cur.inputBind("2",8);
		cur.inputBind("3",8);
		cur.inputBind("4",8);
		cur.inputBind("5",8);
		cur.inputBind("6",8.1,2,1);
		cur.inputBind("7",8.1,2,1);
		cur.inputBind("8",8.1,2,1);
		cur.inputBind("9","2008-01-01");
		cur.inputBind("10","08:00:00");
		cur.inputBind("11","2008-01-01 08:00:00");
		cur.inputBind("12","2008");
		cur.inputBind("13","char8");
		cur.inputBind("14","text8");
		cur.inputBind("15","varchar8");
		cur.inputBind("16","tinytext8");
		cur.inputBind("17","mediumtext8");
		cur.inputBind("18","longtext8");
		cur.validateBinds();
		assertTrue(cur.executeQuery());
		System.out.println();
	
		System.out.println("SELECT: ");
		assertTrue(cur.sendQuery("select * from testtable order by testtinyint"));
		System.out.println();
	
		System.out.println("COLUMN COUNT: ");
		assertEquals(cur.colCount(),19);
		System.out.println();
	
		System.out.println("COLUMN NAMES: ");
		assertEquals(cur.getColumnName(0),"testtinyint");
		assertEquals(cur.getColumnName(1),"testsmallint");
		assertEquals(cur.getColumnName(2),"testmediumint");
		assertEquals(cur.getColumnName(3),"testint");
		assertEquals(cur.getColumnName(4),"testbigint");
		assertEquals(cur.getColumnName(5),"testfloat");
		assertEquals(cur.getColumnName(6),"testreal");
		assertEquals(cur.getColumnName(7),"testdecimal");
		assertEquals(cur.getColumnName(8),"testdate");
		assertEquals(cur.getColumnName(9),"testtime");
		assertEquals(cur.getColumnName(10),"testdatetime");
		assertEquals(cur.getColumnName(11),"testyear");
		assertEquals(cur.getColumnName(12),"testchar");
		assertEquals(cur.getColumnName(13),"testtext");
		assertEquals(cur.getColumnName(14),"testvarchar");
		assertEquals(cur.getColumnName(15),"testtinytext");
		assertEquals(cur.getColumnName(16),"testmediumtext");
		assertEquals(cur.getColumnName(17),"testlongtext");
		assertEquals(cur.getColumnName(18),"testtimestamp");
		cols=cur.getColumnNames();
		assertEquals(cols[0],"testtinyint");
		assertEquals(cols[1],"testsmallint");
		assertEquals(cols[2],"testmediumint");
		assertEquals(cols[3],"testint");
		assertEquals(cols[4],"testbigint");
		assertEquals(cols[5],"testfloat");
		assertEquals(cols[6],"testreal");
		assertEquals(cols[7],"testdecimal");
		assertEquals(cols[8],"testdate");
		assertEquals(cols[9],"testtime");
		assertEquals(cols[10],"testdatetime");
		assertEquals(cols[11],"testyear");
		assertEquals(cols[12],"testchar");
		assertEquals(cols[13],"testtext");
		assertEquals(cols[14],"testvarchar");
		assertEquals(cols[15],"testtinytext");
		assertEquals(cols[16],"testmediumtext");
		assertEquals(cols[17],"testlongtext");
		assertEquals(cols[18],"testtimestamp");
		System.out.println();
	
		System.out.println("COLUMN TYPES: ");
		assertEquals(cur.getColumnType(0),"TINYINT");
		assertEquals(cur.getColumnType(1),"SMALLINT");
		assertEquals(cur.getColumnType(2),"MEDIUMINT");
		assertEquals(cur.getColumnType(3),"INT");
		assertEquals(cur.getColumnType(4),"BIGINT");
		assertEquals(cur.getColumnType(5),"FLOAT");
		assertEquals(cur.getColumnType(6),"REAL");
		assertEquals(cur.getColumnType(7),"DECIMAL");
		assertEquals(cur.getColumnType(8),"DATE");
		assertEquals(cur.getColumnType(9),"TIME");
		assertEquals(cur.getColumnType(10),"DATETIME");
		assertEquals(cur.getColumnType(11),"YEAR");
		if (majorversion==3) {
			assertEquals(cur.getColumnType(12),"VARSTRING");
		} else {
			assertEquals(cur.getColumnType(12),"STRING");
		}
		assertEquals(cur.getColumnType(13),"BLOB");
		assertEquals(cur.getColumnType(14),"VARSTRING");
		assertEquals(cur.getColumnType(15),"TINYBLOB");
		assertEquals(cur.getColumnType(16),"MEDIUMBLOB");
		assertEquals(cur.getColumnType(17),"LONGBLOB");
		assertEquals(cur.getColumnType(18),"TIMESTAMP");
		assertEquals(cur.getColumnType("testtinyint"),"TINYINT");
		assertEquals(cur.getColumnType("testsmallint"),"SMALLINT");
		assertEquals(cur.getColumnType("testmediumint"),"MEDIUMINT");
		assertEquals(cur.getColumnType("testint"),"INT");
		assertEquals(cur.getColumnType("testbigint"),"BIGINT");
		assertEquals(cur.getColumnType("testfloat"),"FLOAT");
		assertEquals(cur.getColumnType("testreal"),"REAL");
		assertEquals(cur.getColumnType("testdecimal"),"DECIMAL");
		assertEquals(cur.getColumnType("testdate"),"DATE");
		assertEquals(cur.getColumnType("testtime"),"TIME");
		assertEquals(cur.getColumnType("testdatetime"),"DATETIME");
		assertEquals(cur.getColumnType("testyear"),"YEAR");
		if (majorversion==3) {
			assertEquals(cur.getColumnType("testchar"),"VARSTRING");
		} else {
			assertEquals(cur.getColumnType("testchar"),"STRING");
		}
		assertEquals(cur.getColumnType("testtext"),"BLOB");
		assertEquals(cur.getColumnType("testvarchar"),"VARSTRING");
		assertEquals(cur.getColumnType("testtinytext"),"TINYBLOB");
		assertEquals(cur.getColumnType("testmediumtext"),"MEDIUMBLOB");
		assertEquals(cur.getColumnType("testlongtext"),"LONGBLOB");
		assertEquals(cur.getColumnType("testtimestamp"),"TIMESTAMP");
		System.out.println();
	
		System.out.println("COLUMN LENGTH: ");
		assertEquals(cur.getColumnLength(0),1);
		assertEquals(cur.getColumnLength(1),2);
		assertEquals(cur.getColumnLength(2),3);
		assertEquals(cur.getColumnLength(3),4);
		assertEquals(cur.getColumnLength(4),8);
		assertEquals(cur.getColumnLength(5),4);
		assertEquals(cur.getColumnLength(6),8);
		assertEquals(cur.getColumnLength(7),6);
		assertEquals(cur.getColumnLength(8),3);
		assertEquals(cur.getColumnLength(9),3);
		assertEquals(cur.getColumnLength(10),8);
		assertEquals(cur.getColumnLength(11),1);
		//assertEquals(cur.getColumnLength(12),40);
		assertEquals(cur.getColumnLength(13),65535);
		//assertEquals(cur.getColumnLength(14),41);
		assertEquals(cur.getColumnLength(15),255);
		assertEquals(cur.getColumnLength(16),16777215);
		assertEquals(cur.getColumnLength(17),2147483647);
		assertEquals(cur.getColumnLength(18),4);
		assertEquals(cur.getColumnLength("testtinyint"),1);
		assertEquals(cur.getColumnLength("testsmallint"),2);
		assertEquals(cur.getColumnLength("testmediumint"),3);
		assertEquals(cur.getColumnLength("testint"),4);
		assertEquals(cur.getColumnLength("testbigint"),8);
		assertEquals(cur.getColumnLength("testfloat"),4);
		assertEquals(cur.getColumnLength("testreal"),8);
		assertEquals(cur.getColumnLength("testdecimal"),6);
		assertEquals(cur.getColumnLength("testdate"),3);
		assertEquals(cur.getColumnLength("testtime"),3);
		assertEquals(cur.getColumnLength("testdatetime"),8);
		assertEquals(cur.getColumnLength("testyear"),1);
		//assertEquals(cur.getColumnLength("testchar"),40);
		assertEquals(cur.getColumnLength("testtext"),65535);
		//assertEquals(cur.getColumnLength("testvarchar"),41);
		assertEquals(cur.getColumnLength("testtinytext"),255);
		assertEquals(cur.getColumnLength("testmediumtext"),16777215);
		assertEquals(cur.getColumnLength("testlongtext"),2147483647);
		assertEquals(cur.getColumnLength("testtimestamp"),4);
		System.out.println();
	
		System.out.println("LONGEST COLUMN: ");
		assertEquals(cur.getLongest(0),1);
		assertEquals(cur.getLongest(1),1);
		assertEquals(cur.getLongest(2),1);
		assertEquals(cur.getLongest(3),1);
		assertEquals(cur.getLongest(4),1);
		//assertEquals(cur.getLongest(5),3);
		assertEquals(cur.getLongest(6),3);
		assertEquals(cur.getLongest(7),3);
		assertEquals(cur.getLongest(8),10);
		assertEquals(cur.getLongest(9),8);
		assertEquals(cur.getLongest(10),19);
		assertEquals(cur.getLongest(11),4);
		assertEquals(cur.getLongest(12),5);
		assertEquals(cur.getLongest(13),5);
		assertEquals(cur.getLongest(14),8);
		assertEquals(cur.getLongest(15),9);
		assertEquals(cur.getLongest(16),11);
		assertEquals(cur.getLongest(17),9);
		if (majorversion==3) {
			assertEquals(cur.getLongest(18),14);
		} else {
			assertEquals(cur.getLongest(18),19);
		}
		assertEquals(cur.getLongest("testtinyint"),1);
		assertEquals(cur.getLongest("testsmallint"),1);
		assertEquals(cur.getLongest("testmediumint"),1);
		assertEquals(cur.getLongest("testint"),1);
		assertEquals(cur.getLongest("testbigint"),1);
		//assertEquals(cur.getLongest("testfloat"),3);
		assertEquals(cur.getLongest("testreal"),3);
		assertEquals(cur.getLongest("testdecimal"),3);
		assertEquals(cur.getLongest("testdate"),10);
		assertEquals(cur.getLongest("testtime"),8);
		assertEquals(cur.getLongest("testdatetime"),19);
		assertEquals(cur.getLongest("testyear"),4);
		assertEquals(cur.getLongest("testchar"),5);
		assertEquals(cur.getLongest("testtext"),5);
		assertEquals(cur.getLongest("testvarchar"),8);
		assertEquals(cur.getLongest("testtinytext"),9);
		assertEquals(cur.getLongest("testmediumtext"),11);
		assertEquals(cur.getLongest("testlongtext"),9);
		if (majorversion==3) {
			assertEquals(cur.getLongest("testtimestamp"),14);
		} else {
			assertEquals(cur.getLongest("testtimestamp"),19);
		}
		System.out.println();
	
		System.out.println("ROW COUNT: ");
		assertEquals(cur.rowCount(),8);
		System.out.println();
	
		System.out.println("TOTAL ROWS: ");
		// older versions of mysql know this
		//assertEquals(cur.totalRows(),0);
		System.out.println();
	
		System.out.println("FIRST ROW INDEX: ");
		assertEquals(cur.firstRowIndex(),0);
		System.out.println();
	
		System.out.println("END OF RESULT SET: ");
		assertTrue(cur.endOfResultSet());
		System.out.println();
	
		System.out.println("FIELDS BY INDEX: ");
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"1");
		assertEquals(cur.getField(0,2),"1");
		assertEquals(cur.getField(0,3),"1");
		assertEquals(cur.getField(0,4),"1");
		//assertEquals(cur.getField(0,5),"1.1");
		assertEquals(cur.getField(0,6),"1.1");
		assertEquals(cur.getField(0,7),"1.1");
		assertEquals(cur.getField(0,8),"2001-01-01");
		assertEquals(cur.getField(0,9),"01:00:00");
		assertEquals(cur.getField(0,10),"2001-01-01 01:00:00");
		assertEquals(cur.getField(0,11),"2001");
		assertEquals(cur.getField(0,12),"char1");
		assertEquals(cur.getField(0,13),"text1");
		assertEquals(cur.getField(0,14),"varchar1");
		assertEquals(cur.getField(0,15),"tinytext1");
		assertEquals(cur.getField(0,16),"mediumtext1");
		assertEquals(cur.getField(0,17),"longtext1");
		System.out.println();
		assertEquals(cur.getField(7,0),"8");
		assertEquals(cur.getField(7,1),"8");
		assertEquals(cur.getField(7,2),"8");
		assertEquals(cur.getField(7,3),"8");
		assertEquals(cur.getField(7,4),"8");
		//assertEquals(cur.getField(7,5),"8.1");
		assertEquals(cur.getField(7,6),"8.1");
		assertEquals(cur.getField(7,7),"8.1");
		assertEquals(cur.getField(7,8),"2008-01-01");
		assertEquals(cur.getField(7,9),"08:00:00");
		assertEquals(cur.getField(7,10),"2008-01-01 08:00:00");
		assertEquals(cur.getField(7,11),"2008");
		assertEquals(cur.getField(7,12),"char8");
		assertEquals(cur.getField(7,13),"text8");
		assertEquals(cur.getField(7,14),"varchar8");
		assertEquals(cur.getField(7,15),"tinytext8");
		assertEquals(cur.getField(7,16),"mediumtext8");
		assertEquals(cur.getField(7,17),"longtext8");
		System.out.println();
	
		System.out.println("FIELD LENGTHS BY INDEX: ");
		assertEquals(cur.getFieldLength(0,0),1);
		assertEquals(cur.getFieldLength(0,1),1);
		assertEquals(cur.getFieldLength(0,2),1);
		assertEquals(cur.getFieldLength(0,3),1);
		assertEquals(cur.getFieldLength(0,4),1);
		//assertEquals(cur.getFieldLength(0,5),3);
		assertEquals(cur.getFieldLength(0,6),3);
		assertEquals(cur.getFieldLength(0,7),3);
		assertEquals(cur.getFieldLength(0,8),10);
		assertEquals(cur.getFieldLength(0,9),8);
		assertEquals(cur.getFieldLength(0,10),19);
		assertEquals(cur.getFieldLength(0,11),4);
		assertEquals(cur.getFieldLength(0,12),5);
		assertEquals(cur.getFieldLength(0,13),5);
		assertEquals(cur.getFieldLength(0,14),8);
		assertEquals(cur.getFieldLength(0,15),9);
		assertEquals(cur.getFieldLength(0,16),11);
		assertEquals(cur.getFieldLength(0,17),9);
		System.out.println();
		assertEquals(cur.getFieldLength(7,0),1);
		assertEquals(cur.getFieldLength(7,1),1);
		assertEquals(cur.getFieldLength(7,2),1);
		assertEquals(cur.getFieldLength(7,3),1);
		assertEquals(cur.getFieldLength(7,4),1);
		//assertEquals(cur.getFieldLength(7,5),3);
		assertEquals(cur.getFieldLength(7,6),3);
		assertEquals(cur.getFieldLength(7,7),3);
		assertEquals(cur.getFieldLength(7,8),10);
		assertEquals(cur.getFieldLength(7,9),8);
		assertEquals(cur.getFieldLength(7,10),19);
		assertEquals(cur.getFieldLength(7,11),4);
		assertEquals(cur.getFieldLength(7,12),5);
		assertEquals(cur.getFieldLength(7,13),5);
		assertEquals(cur.getFieldLength(7,14),8);
		assertEquals(cur.getFieldLength(7,15),9);
		assertEquals(cur.getFieldLength(7,16),11);
		assertEquals(cur.getFieldLength(7,17),9);
		System.out.println();
	
		System.out.println("FIELDS BY NAME: ");
		assertEquals(cur.getField(0,"testtinyint"),"1");
		assertEquals(cur.getField(0,"testsmallint"),"1");
		assertEquals(cur.getField(0,"testmediumint"),"1");
		assertEquals(cur.getField(0,"testint"),"1");
		assertEquals(cur.getField(0,"testbigint"),"1");
		//assertEquals(cur.getField(0,"testfloat"),"1.1");
		assertEquals(cur.getField(0,"testreal"),"1.1");
		assertEquals(cur.getField(0,"testdecimal"),"1.1");
		assertEquals(cur.getField(0,"testdate"),"2001-01-01");
		assertEquals(cur.getField(0,"testtime"),"01:00:00");
		assertEquals(cur.getField(0,"testdatetime"),"2001-01-01 01:00:00");
		assertEquals(cur.getField(0,"testyear"),"2001");
		assertEquals(cur.getField(0,"testchar"),"char1");
		assertEquals(cur.getField(0,"testtext"),"text1");
		assertEquals(cur.getField(0,"testvarchar"),"varchar1");
		assertEquals(cur.getField(0,"testtinytext"),"tinytext1");
		assertEquals(cur.getField(0,"testmediumtext"),"mediumtext1");
		assertEquals(cur.getField(0,"testlongtext"),"longtext1");
		System.out.println();
		assertEquals(cur.getField(7,"testtinyint"),"8");
		assertEquals(cur.getField(7,"testsmallint"),"8");
		assertEquals(cur.getField(7,"testmediumint"),"8");
		assertEquals(cur.getField(7,"testint"),"8");
		assertEquals(cur.getField(7,"testbigint"),"8");
		//assertEquals(cur.getField(7,"testfloat"),"8.1");
		assertEquals(cur.getField(7,"testreal"),"8.1");
		assertEquals(cur.getField(7,"testdecimal"),"8.1");
		assertEquals(cur.getField(7,"testdate"),"2008-01-01");
		assertEquals(cur.getField(7,"testtime"),"08:00:00");
		assertEquals(cur.getField(7,"testdatetime"),"2008-01-01 08:00:00");
		assertEquals(cur.getField(7,"testyear"),"2008");
		assertEquals(cur.getField(7,"testchar"),"char8");
		assertEquals(cur.getField(7,"testtext"),"text8");
		assertEquals(cur.getField(7,"testvarchar"),"varchar8");
		assertEquals(cur.getField(7,"testtinytext"),"tinytext8");
		assertEquals(cur.getField(7,"testmediumtext"),"mediumtext8");
		assertEquals(cur.getField(7,"testlongtext"),"longtext8");
		System.out.println();
	
		System.out.println("FIELD LENGTHS BY NAME: ");
		assertEquals(cur.getFieldLength(0,"testtinyint"),1);
		assertEquals(cur.getFieldLength(0,"testsmallint"),1);
		assertEquals(cur.getFieldLength(0,"testmediumint"),1);
		assertEquals(cur.getFieldLength(0,"testint"),1);
		assertEquals(cur.getFieldLength(0,"testbigint"),1);
		//assertEquals(cur.getFieldLength(0,"testfloat"),3);
		assertEquals(cur.getFieldLength(0,"testreal"),3);
		assertEquals(cur.getFieldLength(0,"testdecimal"),3);
		assertEquals(cur.getFieldLength(0,"testdate"),10);
		assertEquals(cur.getFieldLength(0,"testtime"),8);
		assertEquals(cur.getFieldLength(0,"testdatetime"),19);
		assertEquals(cur.getFieldLength(0,"testyear"),4);
		assertEquals(cur.getFieldLength(0,"testchar"),5);
		assertEquals(cur.getFieldLength(0,"testtext"),5);
		assertEquals(cur.getFieldLength(0,"testvarchar"),8);
		assertEquals(cur.getFieldLength(0,"testtinytext"),9);
		assertEquals(cur.getFieldLength(0,"testmediumtext"),11);
		assertEquals(cur.getFieldLength(0,"testlongtext"),9);
		System.out.println();
		assertEquals(cur.getFieldLength(7,"testtinyint"),1);
		assertEquals(cur.getFieldLength(7,"testsmallint"),1);
		assertEquals(cur.getFieldLength(7,"testmediumint"),1);
		assertEquals(cur.getFieldLength(7,"testint"),1);
		assertEquals(cur.getFieldLength(7,"testbigint"),1);
		//assertEquals(cur.getFieldLength(7,"testfloat"),3);
		assertEquals(cur.getFieldLength(7,"testreal"),3);
		assertEquals(cur.getFieldLength(7,"testdecimal"),3);
		assertEquals(cur.getFieldLength(7,"testdate"),10);
		assertEquals(cur.getFieldLength(7,"testtime"),8);
		assertEquals(cur.getFieldLength(7,"testdatetime"),19);
		assertEquals(cur.getFieldLength(7,"testyear"),4);
		assertEquals(cur.getFieldLength(7,"testchar"),5);
		assertEquals(cur.getFieldLength(7,"testtext"),5);
		assertEquals(cur.getFieldLength(7,"testvarchar"),8);
		assertEquals(cur.getFieldLength(7,"testtinytext"),9);
		assertEquals(cur.getFieldLength(7,"testmediumtext"),11);
		assertEquals(cur.getFieldLength(7,"testlongtext"),9);
		System.out.println();
	
		System.out.println("FIELDS BY ARRAY: ");
		fields=cur.getRow(0);
		assertEquals(fields[0],"1");
		assertEquals(fields[1],"1");
		assertEquals(fields[2],"1");
		assertEquals(fields[3],"1");
		assertEquals(fields[4],"1");
		//assertEquals(fields[5],"1.1");
		assertEquals(fields[6],"1.1");
		assertEquals(fields[7],"1.1");
		assertEquals(fields[8],"2001-01-01");
		assertEquals(fields[9],"01:00:00");
		assertEquals(fields[10],"2001-01-01 01:00:00");
		assertEquals(fields[11],"2001");
		assertEquals(fields[12],"char1");
		assertEquals(fields[13],"text1");
		assertEquals(fields[14],"varchar1");
		assertEquals(fields[15],"tinytext1");
		assertEquals(fields[16],"mediumtext1");
		assertEquals(fields[17],"longtext1");
		System.out.println();
	
		System.out.println("FIELD LENGTHS BY ARRAY: ");
		fieldlens=cur.getRowLengths(0);
		assertEquals(fieldlens[0],1);
		assertEquals(fieldlens[1],1);
		assertEquals(fieldlens[2],1);
		assertEquals(fieldlens[3],1);
		assertEquals(fieldlens[4],1);
		//assertEquals(fieldlens[5],3);
		assertEquals(fieldlens[6],3);
		assertEquals(fieldlens[7],3);
		assertEquals(fieldlens[8],10);
		assertEquals(fieldlens[9],8);
		assertEquals(fieldlens[10],19);
		assertEquals(fieldlens[11],4);
		assertEquals(fieldlens[12],5);
		assertEquals(fieldlens[13],5);
		assertEquals(fieldlens[14],8);
		assertEquals(fieldlens[15],9);
		assertEquals(fieldlens[16],11);
		assertEquals(fieldlens[17],9);
		System.out.println();
	
		System.out.println("INDIVIDUAL SUBSTITUTIONS: ");
		cur.prepareQuery("select $(var1),'$(var2)',$(var3)");
		cur.substitution("var1",1);
		cur.substitution("var2","hello");
		cur.substitution("var3",10.5556,6,4);
		assertTrue(cur.executeQuery());
		System.out.println();
	
		System.out.println("FIELDS: ");
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"hello");
		assertEquals(cur.getField(0,2),"10.5556");
		System.out.println();
	
		System.out.println("ARRAY SUBSTITUTIONS: ");
		cur.prepareQuery("select $(var1),$(var2),$(var3)");
		cur.substitutions(subvars,subvallongs);
		assertTrue(cur.executeQuery());
		System.out.println();
		
		System.out.println("FIELDS: ");
		assertEquals(cur.getField(0,0),"1");
		assertEquals(cur.getField(0,1),"2");
		assertEquals(cur.getField(0,2),"3");
		System.out.println();
		
		System.out.println("ARRAY SUBSTITUTIONS: ");
		cur.prepareQuery("select '$(var1)','$(var2)','$(var3)'");
		cur.substitutions(subvars,subvalstrings);
		assertTrue(cur.executeQuery());
		System.out.println();
	
		System.out.println("FIELDS: ");
		assertEquals(cur.getField(0,0),"hi");
		assertEquals(cur.getField(0,1),"hello");
		assertEquals(cur.getField(0,2),"bye");
		System.out.println();
	
		System.out.println("ARRAY SUBSTITUTIONS: ");
		cur.prepareQuery("select $(var1),$(var2),$(var3)");
		cur.substitutions(subvars,subvaldoubles,precs,scales);
		assertTrue(cur.executeQuery());
		System.out.println();
	
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
	
		System.out.println("RESULT SET BUFFER SIZE: ");
		assertEquals(cur.getResultSetBufferSize(),0);
		cur.setResultSetBufferSize(2);
		assertTrue(cur.sendQuery("select * from testtable order by testtinyint"));
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
		assertTrue(cur.sendQuery("select * from testtable order by testtinyint"));
		assertEquals(cur.getColumnName(0),null);
		assertEquals(cur.getColumnLength(0),0);
		assertEquals(cur.getColumnType(0),null);
		System.out.println();
		cur.getColumnInfo();
		assertTrue(cur.sendQuery("select * from testtable order by testtinyint"));
		assertEquals(cur.getColumnName(0),"testtinyint");
		assertEquals(cur.getColumnLength(0),1);
		assertEquals(cur.getColumnType(0),"TINYINT");
		System.out.println();
	
		System.out.println("SUSPENDED SESSION: ");
		assertTrue(cur.sendQuery("select * from testtable order by testtinyint"));
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
		assertTrue(cur.sendQuery("select * from testtable order by testtinyint"));
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
		assertTrue(cur.sendQuery("select * from testtable order by testtinyint"));
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
		assertTrue(cur.sendQuery("select * from testtable order by testtinyint"));
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
		assertTrue(cur.sendQuery("select * from testtable order by testtinyint"));
		filename=cur.getCacheFileName();
		assertEquals(filename,"cachefile1");
		cur.cacheOff();
		assertTrue(cur.openCachedResultSet(filename));
		assertEquals(cur.getField(7,0),"8");
		System.out.println();
	
		System.out.println("COLUMN COUNT FOR CACHED RESULT SET: ");
		assertEquals(cur.colCount(),19);
		System.out.println();
	
		System.out.println("COLUMN NAMES FOR CACHED RESULT SET: ");
		assertEquals(cur.getColumnName(0),"testtinyint");
		assertEquals(cur.getColumnName(1),"testsmallint");
		assertEquals(cur.getColumnName(2),"testmediumint");
		assertEquals(cur.getColumnName(3),"testint");
		assertEquals(cur.getColumnName(4),"testbigint");
		assertEquals(cur.getColumnName(5),"testfloat");
		assertEquals(cur.getColumnName(6),"testreal");
		assertEquals(cur.getColumnName(7),"testdecimal");
		assertEquals(cur.getColumnName(8),"testdate");
		assertEquals(cur.getColumnName(9),"testtime");
		assertEquals(cur.getColumnName(10),"testdatetime");
		assertEquals(cur.getColumnName(11),"testyear");
		assertEquals(cur.getColumnName(12),"testchar");
		assertEquals(cur.getColumnName(13),"testtext");
		assertEquals(cur.getColumnName(14),"testvarchar");
		assertEquals(cur.getColumnName(15),"testtinytext");
		assertEquals(cur.getColumnName(16),"testmediumtext");
		assertEquals(cur.getColumnName(17),"testlongtext");
		cols=cur.getColumnNames();
		assertEquals(cols[0],"testtinyint");
		assertEquals(cols[1],"testsmallint");
		assertEquals(cols[2],"testmediumint");
		assertEquals(cols[3],"testint");
		assertEquals(cols[4],"testbigint");
		assertEquals(cols[5],"testfloat");
		assertEquals(cols[6],"testreal");
		assertEquals(cols[7],"testdecimal");
		assertEquals(cols[8],"testdate");
		assertEquals(cols[9],"testtime");
		assertEquals(cols[10],"testdatetime");
		assertEquals(cols[11],"testyear");
		assertEquals(cols[12],"testchar");
		assertEquals(cols[13],"testtext");
		assertEquals(cols[14],"testvarchar");
		assertEquals(cols[15],"testtinytext");
		assertEquals(cols[16],"testmediumtext");
		assertEquals(cols[17],"testlongtext");
		System.out.println();
	
		System.out.println("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
		cur.setResultSetBufferSize(2);
		cur.cacheToFile("cachefile1");
		cur.setCacheTtl(200);
		assertTrue(cur.sendQuery("select * from testtable order by testtinyint"));
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
		assertTrue(cur.sendQuery("select * from testtable order by testtinyint"));
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
	
		System.out.println("COMMIT AND ROLLBACK: ");
		// Note: Mysql's default isolation level is repeatable-read,
		// not read-committed like most other db's.  Both sessions must
		// commit to see the changes that each other has made.
		SQLRConnection secondcon=new SQLRConnection("sqlrelay",
						(short)9000,
						"/tmp/test.socket",
						"testuser","testpassword",0,1);
		SQLRCursor secondcur=new SQLRCursor(secondcon);
		assertEquals(secondcur.sendQuery("select count(*) from testtable"),1);
		if (majorversion>3) {
			assertEquals(secondcur.getField(0,0),"0");
		} else {
			assertEquals(secondcur.getField(0,0),"8");
		}
		assertTrue(con.commit());
		assertEquals(secondcon.commit(),1);
		assertEquals(secondcur.sendQuery("select count(*) from testtable"),1);
		assertEquals(secondcur.getField(0,0),"8");
		assertTrue(con.autoCommitOn());
		assertTrue(cur.sendQuery("insert into testtable values (10,10,10,10,10,10.1,10.1,1.1,'2010-01-01','10:00:00','2010-01-01 10:00:00','2010','char10','text10','varchar10','tinytext10','mediumtext10','longtext10',null)"));
		assertEquals(secondcon.commit(),1);
		assertEquals(secondcur.sendQuery("select count(*) from testtable"),1);
		assertEquals(secondcur.getField(0,0),"9");
		assertTrue(con.autoCommitOff());
		secondcon.commit();
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
		assertFalse(cur.sendQuery("select * from testtable order by testtinyint"));
		assertFalse(cur.sendQuery("select * from testtable order by testtinyint"));
		assertFalse(cur.sendQuery("select * from testtable order by testtinyint"));
		assertFalse(cur.sendQuery("select * from testtable order by testtinyint"));
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
