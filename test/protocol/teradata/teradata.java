// Copyright (c) 2017  David Muse
// See the file COPYING for more information.

import java.sql.*;
import java.text.DateFormat;
import java.text.SimpleDateFormat;

class teradata {

	private static void assertEquals(String actual, String expected) {

		if (expected==null) {
			if (actual==null) {
				System.out.print("success ");
				return;
			} else {
				System.out.print(actual+"!="+expected+" ");
				System.out.print("failure ");
				System.exit(1);
			}
		}

		if (actual.equals(expected)) {
			System.out.print("success ");
		} else {
			System.out.print(actual+"!="+expected+" ");
			System.out.print("failure ");
			System.exit(1);
		}
	}

	private static void assertEquals(long actual, int expected) {

		if (actual==expected) {
			System.out.print("success ");
		} else {
			System.out.print(actual+"!="+expected+" ");
			System.out.print("failure ");
			System.exit(1);
		}
	}

	private static void assertEquals(boolean actual, int expected) {

		if (((actual)?1:0)==expected) {
			System.out.print("success ");
		} else {
			System.out.print(actual+"!="+expected+" ");
			System.out.print("failure ");
			System.exit(1);
		}
	}

	public static void main(String args[]) throws Exception {

		String	host="localhost";
		if (args.length>0) {
			host=args[0];
		}

		Class.forName("com.teradata.jdbc.TeraDriver");


		// connect
		Connection	con=DriverManager.getConnection(
					"jdbc:teradata://"+host+"/",
					"testuser","testpassword");
		Statement	stmt=con.createStatement();


if (false) {
		// help session
		System.out.println("HELP SESSION:");
		ResultSet		rs=stmt.executeQuery("help session");
		ResultSetMetaData	rsmd=rs.getMetaData();
		assertEquals(rs.next(),1);
		assertEquals(rsmd.getColumnCount(),129);
		System.out.println();
		assertEquals(rsmd.getColumnName(1),"User Name");
		assertEquals(rsmd.getColumnName(2),"Account Name");
		assertEquals(rsmd.getColumnName(3),"Logon Date");
		assertEquals(rsmd.getColumnName(4),"Logon Time");
		assertEquals(rsmd.getColumnName(5),"Current DataBase");
		assertEquals(rsmd.getColumnName(6),"Collation");
		assertEquals(rsmd.getColumnName(7),"Character Set");
		assertEquals(rsmd.getColumnName(8),"Transaction Semantics");
		assertEquals(rsmd.getColumnName(9),"Current DateForm");
		assertEquals(rsmd.getColumnName(10),"Session Time Zone");
		assertEquals(rsmd.getColumnName(11),"Default Character Type");
		assertEquals(rsmd.getColumnName(12),"Export Latin");
		assertEquals(rsmd.getColumnName(13),"Export Unicode");
		assertEquals(rsmd.getColumnName(14),"Export Unicode Adjust");
		assertEquals(rsmd.getColumnName(15),"Export KanjiSJIS");
		assertEquals(rsmd.getColumnName(16),"Export Graphic");
		assertEquals(rsmd.getColumnName(17),"Default Date Format");
		assertEquals(rsmd.getColumnName(18),"Radix Separator");
		assertEquals(rsmd.getColumnName(19),"Group Separator");
		assertEquals(rsmd.getColumnName(20),"Grouping Rule");
		assertEquals(rsmd.getColumnName(21),"Currency Radix Separator");
		assertEquals(rsmd.getColumnName(22),"Currency Group Separator");
		assertEquals(rsmd.getColumnName(23),"Currency Grouping Rule");
		assertEquals(rsmd.getColumnName(24),"Currency Name");
		assertEquals(rsmd.getColumnName(25),"Currency");
		assertEquals(rsmd.getColumnName(26),"ISOCurrency");
		assertEquals(rsmd.getColumnName(27),"Dual Currency Name");
		assertEquals(rsmd.getColumnName(28),"Dual Currency");
		assertEquals(rsmd.getColumnName(29),"Dual ISOCurrency");
		assertEquals(rsmd.getColumnName(30),"Default ByteInt format");
		assertEquals(rsmd.getColumnName(31),"Default Integer format");
		assertEquals(rsmd.getColumnName(32),"Default SmallInt format");
		assertEquals(rsmd.getColumnName(33),"Default Numeric format");
		assertEquals(rsmd.getColumnName(34),"Default Real format");
		assertEquals(rsmd.getColumnName(35),"Default Time format");
		assertEquals(rsmd.getColumnName(36),"Default Timestamp format");
		assertEquals(rsmd.getColumnName(37),"Current Role");
		assertEquals(rsmd.getColumnName(38),"Logon Account");
		assertEquals(rsmd.getColumnName(39),"Profile");
		assertEquals(rsmd.getColumnName(40),"LDAP");
		assertEquals(rsmd.getColumnName(41),"Audit Trail Id");
		assertEquals(rsmd.getColumnName(42),"Current Isolation Level");
		assertEquals(rsmd.getColumnName(43),"Default BigInt format");
		assertEquals(rsmd.getColumnName(44),"QueryBand");
		assertEquals(rsmd.getColumnName(45),"Proxy User");
		assertEquals(rsmd.getColumnName(46),"Proxy Role");
		assertEquals(rsmd.getColumnName(47),"Constraint1Name");
		assertEquals(rsmd.getColumnName(48),"Constraint1Value");
		assertEquals(rsmd.getColumnName(49),"Constraint2Name");
		assertEquals(rsmd.getColumnName(50),"Constraint2Value");
		assertEquals(rsmd.getColumnName(51),"Constraint3Name");
		assertEquals(rsmd.getColumnName(52),"Constraint3Value");
		assertEquals(rsmd.getColumnName(53),"Constraint4Name");
		assertEquals(rsmd.getColumnName(54),"Constraint4Value");
		assertEquals(rsmd.getColumnName(55),"Constraint5Name");
		assertEquals(rsmd.getColumnName(56),"Constraint5Value");
		assertEquals(rsmd.getColumnName(57),"Constraint6Name");
		assertEquals(rsmd.getColumnName(58),"Constraint6Value");
		assertEquals(rsmd.getColumnName(59),"Constraint7Name");
		assertEquals(rsmd.getColumnName(60),"Constraint7Value");
		assertEquals(rsmd.getColumnName(61),"Constraint8Name");
		assertEquals(rsmd.getColumnName(62),"Constraint8Value");
		assertEquals(rsmd.getColumnName(63),"Temporal Qualifier");
		assertEquals(rsmd.getColumnName(64),"Calendar");
		assertEquals(rsmd.getColumnName(65),"Export Width Rule Set");
		assertEquals(rsmd.getColumnName(66),"Default Number format");
		assertEquals(rsmd.getColumnName(67),"TTGranularity");
		assertEquals(rsmd.getColumnName(68),"Redrive Participation");
		assertEquals(rsmd.getColumnName(69),"User Dictionary Name");
		assertEquals(rsmd.getColumnName(70),"User SQL Name");
		assertEquals(rsmd.getColumnName(71),"User UEscape");
		assertEquals(rsmd.getColumnName(72),"Account Dictionary Name");
		assertEquals(rsmd.getColumnName(73),"Account SQL Name");
		assertEquals(rsmd.getColumnName(74),"Account UEscape");
		assertEquals(rsmd.getColumnName(75),
					"Current Database Dictionary Name");
		assertEquals(rsmd.getColumnName(76),
					"Current Database SQL Name");
		assertEquals(rsmd.getColumnName(77),
					"Current Database UEscape");
		assertEquals(rsmd.getColumnName(78),
					"Current Role Dictionary Name");
		assertEquals(rsmd.getColumnName(79),
					"Current Role SQL Name");
		assertEquals(rsmd.getColumnName(80),
					"Current Role UEscape");
		assertEquals(rsmd.getColumnName(81),
					"Logon Account Dictionary Name");
		assertEquals(rsmd.getColumnName(82),
					"Logon Account SQL Name");
		assertEquals(rsmd.getColumnName(83),
					"Logon Account UEscape");
		assertEquals(rsmd.getColumnName(84),
					"Profile Dictionary Name");
		assertEquals(rsmd.getColumnName(85),
					"Profile SQL Name");
		assertEquals(rsmd.getColumnName(86),
					"Profile UEscape");
		assertEquals(rsmd.getColumnName(87),
					"Audit Trail Id Dictionary Name");
		assertEquals(rsmd.getColumnName(88),
					"Audit Trail Id SQL Name");
		assertEquals(rsmd.getColumnName(89),
					"Audit Trail Id UEscape");
		assertEquals(rsmd.getColumnName(90),
					"Proxy User Dictionary Name");
		assertEquals(rsmd.getColumnName(91),
					"Proxy User SQL Name");
		assertEquals(rsmd.getColumnName(92),
					"Proxy User UEscape");
		assertEquals(rsmd.getColumnName(93),
					"Proxy Role Dictionary Name");
		assertEquals(rsmd.getColumnName(94),
					"Proxy Role SQL Name");
		assertEquals(rsmd.getColumnName(95),
					"Proxy Role UEscape");
		assertEquals(rsmd.getColumnName(96),
					"Constraint1Name Dictionary Name");
		assertEquals(rsmd.getColumnName(97),
					"Constraint1Name SQL Name");
		assertEquals(rsmd.getColumnName(98),
					"Constraint1Name UEscape");
		assertEquals(rsmd.getColumnName(99),
					"Constraint2Name Dictionary Name");
		assertEquals(rsmd.getColumnName(100),
					"Constraint2Name SQL Name");
		assertEquals(rsmd.getColumnName(101),
					"Constraint2Name UEscape");
		assertEquals(rsmd.getColumnName(102),
					"Constraint3Name Dictionary Name");
		assertEquals(rsmd.getColumnName(103),
					"Constraint3Name SQL Name");
		assertEquals(rsmd.getColumnName(104),
					"Constraint3Name UEscape");
		assertEquals(rsmd.getColumnName(105),
					"Constraint4Name Dictionary Name");
		assertEquals(rsmd.getColumnName(106),
					"Constraint4Name SQL Name");
		assertEquals(rsmd.getColumnName(107),
					"Constraint4Name UEscape");
		assertEquals(rsmd.getColumnName(108),
					"Constraint5Name Dictionary Name");
		assertEquals(rsmd.getColumnName(109),
					"Constraint5Name SQL Name");
		assertEquals(rsmd.getColumnName(110),
					"Constraint5Name UEscape");
		assertEquals(rsmd.getColumnName(111),
					"Constraint6Name Dictionary Name");
		assertEquals(rsmd.getColumnName(112),
					"Constraint6Name SQL Name");
		assertEquals(rsmd.getColumnName(113),
					"Constraint6Name UEscape");
		assertEquals(rsmd.getColumnName(114),
					"Constraint7Name Dictionary Name");
		assertEquals(rsmd.getColumnName(115),
					"Constraint7Name SQL Name");
		assertEquals(rsmd.getColumnName(116),
					"Constraint7Name UEscape");
		assertEquals(rsmd.getColumnName(117),
					"Constraint8Name Dictionary Name");
		assertEquals(rsmd.getColumnName(118),
					"Constraint8Name SQL Name");
		assertEquals(rsmd.getColumnName(119),
					"Constraint8Name UEscape");
		assertEquals(rsmd.getColumnName(120),
					"Zone Name");
		assertEquals(rsmd.getColumnName(121),
					"SearchUIFDBPath");
		assertEquals(rsmd.getColumnName(122),
					"Transaction QueryBand");
		assertEquals(rsmd.getColumnName(123),
					"Session QueryBand");
		assertEquals(rsmd.getColumnName(124),
					"Profile QueryBand");
		assertEquals(rsmd.getColumnName(125),
					"Unicode Pass Through");
		assertEquals(rsmd.getColumnName(126),
					"Default Map Dictionary Name");
		assertEquals(rsmd.getColumnName(127),
					"Default Map SQL Name");
		assertEquals(rsmd.getColumnName(128),
					"Default Map UEscape");
		assertEquals(rsmd.getColumnName(129),
					"Default Override");
		rs.close();
		System.out.println("\n");


		// drop
		try {
			stmt.executeUpdate("drop table testtable");
		} catch (Exception e) {
		}


		// create
		System.out.println("CREATE:");
		assertEquals(stmt.executeUpdate(
					"create table testtable ("+
					"	col1 byteint,"+
					"	col2 smallint,"+
					"	col3 integer,"+
					"	col4 bigint,"+
					"	col5 decimal(10,3),"+
					"	col6 number(10,3),"+
					"	col7 float,"+
					"	col8 char(128),"+
					"	col9 varchar(128),"+
					"	col10 date,"+
					"	col11 time,"+
					"	col12 timestamp"+
					")"),0);
		System.out.println("\n");


		// insert
		System.out.println("INSERT:");
		assertEquals(stmt.executeUpdate(
					"insert into testtable values ("+
					"1,"+
					"1,"+
					"1,"+
					"1,"+
					"1.123,"+
					"1.123,"+
					"1.123,"+
					"'hi1',"+
					"'hello1',"+
					"'2001-01-01',"+
					"'01:01:01',"+
					"'2001-01-01 01:01:01'"+
					")"),1);
		System.out.println("\n");


		// insert/bind
		System.out.println("INSERT BIND:");
		PreparedStatement	pstmt=con.prepareStatement(
					"insert into testtable values ("+
					"	?,?,?,?,?,?,?,?,?,?,?,?"+
					")");
		for (int i=2; i<10; i++) {
			pstmt.setShort(1,(short)i);
			pstmt.setShort(2,(short)i);
			pstmt.setInt(3,i);
			pstmt.setLong(4,(long)i);
			pstmt.setDouble(5,(double)i+0.123);
			pstmt.setDouble(6,(double)i+0.123);
			pstmt.setDouble(7,(double)i+0.123);
			pstmt.setString(8,"hi"+i);
			pstmt.setString(9,"hello"+i);
			DateFormat	fmt=new SimpleDateFormat("yyyy-MM-dd");
			pstmt.setDate(10,new java.sql.Date(
					fmt.parse("200"+i+"-0"+i+"-0"+i).
					getTime()));
			fmt=new SimpleDateFormat("hh:mm:ss");
			pstmt.setTime(11,new java.sql.Time(
					fmt.parse("0"+i+":0"+i+":0"+i).
					getTime()));
			fmt=new SimpleDateFormat("yyyy-MM-dd hh:mm:ss");
			pstmt.setTimestamp(12,new java.sql.Timestamp(
					fmt.parse("200"+i+"-0"+i+"-"+
						"0"+i+" 0"+i+":0"+i+":0"+i).
					getTime()));
			assertEquals(pstmt.execute(),0);
		}
		System.out.println("\n");


		// select
		System.out.println("SELECT:");
		rs=stmt.executeQuery("select * from testtable order by col2");
		rsmd=rs.getMetaData();
		System.out.println("\n");

		System.out.println("SELECT - column count:");
		assertEquals(rsmd.getColumnCount(),12);
		System.out.println("\n");

		System.out.println("SELECT - column info:");
		assertEquals(rsmd.getColumnCount(),12);
		System.out.println();
		assertEquals(rsmd.getColumnName(1),"col1");
		assertEquals(rsmd.getColumnType(1),java.sql.Types.TINYINT);
		assertEquals(rsmd.getColumnTypeName(1),"BYTEINT");
		assertEquals(rsmd.getPrecision(1),3);
		assertEquals(rsmd.getScale(1),0);
		assertEquals(rsmd.isAutoIncrement(1),0);
		assertEquals(rsmd.isCaseSensitive(1),0);
		assertEquals(rsmd.isCurrency(1),0);
		assertEquals(rsmd.isNullable(1),1);
		assertEquals(rsmd.isSigned(1),1);
		System.out.println();
		assertEquals(rsmd.getColumnName(2),"col2");
		assertEquals(rsmd.getColumnType(2),java.sql.Types.SMALLINT);
		assertEquals(rsmd.getColumnTypeName(2),"SMALLINT");
		assertEquals(rsmd.getPrecision(2),5);
		assertEquals(rsmd.getScale(2),0);
		assertEquals(rsmd.isAutoIncrement(2),0);
		assertEquals(rsmd.isCaseSensitive(2),0);
		assertEquals(rsmd.isCurrency(2),0);
		assertEquals(rsmd.isNullable(2),1);
		assertEquals(rsmd.isSigned(2),1);
		System.out.println();
		assertEquals(rsmd.getColumnName(3),"col3");
		assertEquals(rsmd.getColumnType(3),java.sql.Types.INTEGER);
		assertEquals(rsmd.getColumnTypeName(3),"INTEGER");
		assertEquals(rsmd.getPrecision(3),10);
		assertEquals(rsmd.getScale(3),0);
		assertEquals(rsmd.isAutoIncrement(3),0);
		assertEquals(rsmd.isCaseSensitive(3),0);
		assertEquals(rsmd.isCurrency(3),0);
		assertEquals(rsmd.isNullable(3),1);
		assertEquals(rsmd.isSigned(3),1);
		System.out.println();
		assertEquals(rsmd.getColumnName(4),"col4");
		assertEquals(rsmd.getColumnType(4),java.sql.Types.BIGINT);
		assertEquals(rsmd.getColumnTypeName(4),"BIGINT");
		assertEquals(rsmd.getPrecision(4),19);
		assertEquals(rsmd.getScale(4),0);
		assertEquals(rsmd.isAutoIncrement(4),0);
		assertEquals(rsmd.isCaseSensitive(4),0);
		assertEquals(rsmd.isCurrency(4),0);
		assertEquals(rsmd.isNullable(4),1);
		assertEquals(rsmd.isSigned(4),1);
		System.out.println();
		assertEquals(rsmd.getColumnName(5),"col5");
		assertEquals(rsmd.getColumnType(5),java.sql.Types.DECIMAL);
		assertEquals(rsmd.getColumnTypeName(5),"DECIMAL");
		assertEquals(rsmd.getPrecision(5),10);
		assertEquals(rsmd.getScale(5),3);
		assertEquals(rsmd.isAutoIncrement(5),0);
		assertEquals(rsmd.isCaseSensitive(5),0);
		assertEquals(rsmd.isCurrency(5),0);
		assertEquals(rsmd.isNullable(5),1);
		assertEquals(rsmd.isSigned(5),1);
		System.out.println();
		assertEquals(rsmd.getColumnName(6),"col6");
		// Teradata...
		//assertEquals(rsmd.getColumnType(6),java.sql.Types.NUMERIC);
		//assertEquals(rsmd.getColumnTypeName(6),"NUMERIC");
		// SQL Relay...
		// (ODBC backend returns SQL_DECIMAL instead of SQL_NUMERIC)...
		//assertEquals(rsmd.getColumnType(6),java.sql.Types.DECIMAL);
		//assertEquals(rsmd.getColumnTypeName(6),"DECIMAL");
		assertEquals(rsmd.getPrecision(6),10);
		assertEquals(rsmd.getScale(6),3);
		assertEquals(rsmd.isAutoIncrement(6),0);
		assertEquals(rsmd.isCaseSensitive(6),0);
		assertEquals(rsmd.isCurrency(6),0);
		assertEquals(rsmd.isNullable(6),1);
		assertEquals(rsmd.isSigned(6),1);
		System.out.println();
		assertEquals(rsmd.getColumnName(7),"col7");
		assertEquals(rsmd.getColumnType(7),java.sql.Types.FLOAT);
		assertEquals(rsmd.getColumnTypeName(7),"FLOAT");
		assertEquals(rsmd.getPrecision(7),15);
		assertEquals(rsmd.getScale(7),0);
		assertEquals(rsmd.isAutoIncrement(7),0);
		assertEquals(rsmd.isCaseSensitive(7),0);
		assertEquals(rsmd.isCurrency(7),0);
		assertEquals(rsmd.isNullable(7),1);
		assertEquals(rsmd.isSigned(7),1);
		System.out.println();
		assertEquals(rsmd.getColumnName(8),"col8");
		assertEquals(rsmd.getColumnType(8),java.sql.Types.CHAR);
		assertEquals(rsmd.getColumnTypeName(8),"CHAR");
		assertEquals(rsmd.getPrecision(8),128);
		assertEquals(rsmd.getScale(8),0);
		assertEquals(rsmd.isAutoIncrement(8),0);
		assertEquals(rsmd.isCaseSensitive(8),0);
		assertEquals(rsmd.isCurrency(8),0);
		assertEquals(rsmd.isNullable(8),1);
		assertEquals(rsmd.isSigned(8),0);
		System.out.println();
		assertEquals(rsmd.getColumnName(9),"col9");
		assertEquals(rsmd.getColumnType(9),java.sql.Types.VARCHAR);
		assertEquals(rsmd.getColumnTypeName(9),"VARCHAR");
		assertEquals(rsmd.getPrecision(9),128);
		assertEquals(rsmd.getScale(9),0);
		assertEquals(rsmd.isAutoIncrement(9),0);
		assertEquals(rsmd.isCaseSensitive(9),0);
		assertEquals(rsmd.isCurrency(9),0);
		assertEquals(rsmd.isNullable(9),1);
		assertEquals(rsmd.isSigned(9),0);
		System.out.println();
		assertEquals(rsmd.getColumnName(10),"col10");
		assertEquals(rsmd.getColumnType(10),java.sql.Types.DATE);
		assertEquals(rsmd.getColumnTypeName(10),"DATE");
		assertEquals(rsmd.getPrecision(10),10);
		assertEquals(rsmd.getScale(10),0);
		assertEquals(rsmd.isAutoIncrement(10),0);
		assertEquals(rsmd.isCaseSensitive(10),0);
		assertEquals(rsmd.isCurrency(10),0);
		assertEquals(rsmd.isNullable(10),1);
		assertEquals(rsmd.isSigned(10),0);
		System.out.println();
		assertEquals(rsmd.getColumnName(11),"col11");
		assertEquals(rsmd.getColumnType(11),java.sql.Types.TIME);
		assertEquals(rsmd.getColumnTypeName(11),"TIME");
		assertEquals(rsmd.getPrecision(11),15);
		assertEquals(rsmd.getScale(11),6);
		assertEquals(rsmd.isAutoIncrement(11),0);
		assertEquals(rsmd.isCaseSensitive(11),0);
		assertEquals(rsmd.isCurrency(11),0);
		assertEquals(rsmd.isNullable(11),1);
		assertEquals(rsmd.isSigned(11),0);
		System.out.println();
		assertEquals(rsmd.getColumnName(12),"col12");
		assertEquals(rsmd.getColumnType(12),java.sql.Types.TIMESTAMP);
		assertEquals(rsmd.getColumnTypeName(12),"TIMESTAMP");
		assertEquals(rsmd.getPrecision(12),26);
		assertEquals(rsmd.getScale(12),6);
		assertEquals(rsmd.isAutoIncrement(12),0);
		assertEquals(rsmd.isCaseSensitive(12),0);
		assertEquals(rsmd.isCurrency(12),0);
		assertEquals(rsmd.isNullable(12),1);
		assertEquals(rsmd.isSigned(12),0);
		System.out.println("\n");

		System.out.println("SELECT - fields:");
		for (int i=1; i<10; i++) {
			assertEquals(rs.next(),1);
			assertEquals(rs.getString(1),""+i);
			assertEquals(rs.getString(2),""+i);
			assertEquals(rs.getString(3),""+i);
			assertEquals(rs.getString(4),""+i);
			assertEquals(rs.getString(5),i+".123");
			assertEquals(rs.getString(6),i+".123");
			assertEquals(rs.getString(7),i+".123");
			assertEquals(rs.getString(8).trim(),"hi"+i);
			assertEquals(rs.getString(9),"hello"+i);
			assertEquals(rs.getString(10),"200"+i+"-0"+i+"-0"+i);
			assertEquals(rs.getString(11),"0"+i+":0"+i+":0"+i);
			assertEquals(rs.getString(12),"200"+i+"-0"+i+"-0"+i+
							" "+
							"0"+i+":0"+i+":0"+i+
							".0");
			System.out.println();
		}
		rs.close();
		System.out.println();


		// update
		System.out.println("UPDATE:");
		assertEquals(stmt.executeUpdate(
				"update testtable set col1=3 where col1=1"),1);
		assertEquals(stmt.executeUpdate(
				"update testtable set col1=4 where col1=2"),1);
		System.out.println("\n");


		// delete
		System.out.println("DELETE:");
		assertEquals(stmt.executeUpdate("delete from testtable"),9);
		System.out.println("\n");


		// even nulls
		System.out.println("EVEN NULLS:");
		assertEquals(stmt.executeUpdate(
					"insert into testtable values ("+
					"1,"+
					"null,"+
					"1,"+
					"null,"+
					"1.123,"+
					"null,"+
					"1.123,"+
					"null,"+
					"'hello1',"+
					"null,"+
					"'01:01:01',"+
					"null"+
					")"),1);
		System.out.println();
		rs=stmt.executeQuery("select * from testtable");
		rsmd=rs.getMetaData();
		assertEquals(rs.next(),1);
		assertEquals(rs.getString(1),"1");
		assertEquals(rs.getString(2),null);
		assertEquals(rs.getString(3),"1");
		assertEquals(rs.getString(4),null);
		assertEquals(rs.getString(5),"1.123");
		assertEquals(rs.getString(6),null);
		assertEquals(rs.getString(7),"1.123");
		assertEquals(rs.getString(8),null);
		assertEquals(rs.getString(9),"hello1");
		assertEquals(rs.getString(10),null);
		assertEquals(rs.getString(11),"01:01:01");
		assertEquals(rs.getString(12),null);
		System.out.println();
		assertEquals(stmt.executeUpdate("delete from testtable"),1);
		System.out.println("\n");


		// odd nulls
		System.out.println("ODD NULLS:");
		assertEquals(stmt.executeUpdate(
					"insert into testtable values ("+
					"null,"+
					"1,"+
					"null,"+
					"1,"+
					"null,"+
					"1.123,"+
					"null,"+
					"'hi1',"+
					"null,"+
					"'2001-01-01',"+
					"null,"+
					"'2001-01-01 01:01:01'"+
					")"),1);
		System.out.println();
		rs=stmt.executeQuery("select * from testtable");
		rsmd=rs.getMetaData();
		assertEquals(rs.next(),1);
		assertEquals(rs.getString(1),null);
		assertEquals(rs.getString(2),"1");
		assertEquals(rs.getString(3),null);
		assertEquals(rs.getString(4),"1");
		assertEquals(rs.getString(5),null);
		assertEquals(rs.getString(6),"1.123");
		assertEquals(rs.getString(7),null);
		assertEquals(rs.getString(8).trim(),"hi1");
		assertEquals(rs.getString(9),null);
		assertEquals(rs.getString(10),"2001-01-01");
		assertEquals(rs.getString(11),null);
		assertEquals(rs.getString(12),"2001-01-01 01:01:01.0");
		System.out.println();
		assertEquals(stmt.executeUpdate("delete from testtable"),1);
		System.out.println("\n");


		// even null binds
		System.out.println("EVEN NULL BINDS:");
		pstmt=con.prepareStatement(
					"insert into testtable values ("+
					"	?,?,?,?,?,?,?,?,?,?,?,?"+
					")");
		pstmt.setShort(1,(short)1);
		pstmt.setNull(2,java.sql.Types.SMALLINT);
		pstmt.setInt(3,1);
		pstmt.setNull(4,java.sql.Types.BIGINT);
		pstmt.setDouble(5,1.123);
		pstmt.setNull(6,java.sql.Types.CHAR);
		pstmt.setDouble(7,1.123);
		pstmt.setNull(8,java.sql.Types.CHAR);
		pstmt.setString(9,"hello1");
		pstmt.setNull(10,java.sql.Types.DATE);
		DateFormat	fmt=new SimpleDateFormat("hh:mm:ss");
		pstmt.setTime(11,new java.sql.Time(
					fmt.parse("01:01:01").
					getTime()));
		fmt=new SimpleDateFormat("yyyy-MM-dd hh:mm:ss");
		pstmt.setNull(12,java.sql.Types.TIMESTAMP);
		assertEquals(pstmt.execute(),0);
		System.out.println();
		rs=stmt.executeQuery("select * from testtable");
		rsmd=rs.getMetaData();
		assertEquals(rs.next(),1);
		assertEquals(rs.getString(1),"1");
		assertEquals(rs.getString(2),null);
		assertEquals(rs.getString(3),"1");
		assertEquals(rs.getString(4),null);
		assertEquals(rs.getString(5),"1.123");
		assertEquals(rs.getString(6),null);
		assertEquals(rs.getString(7),"1.123");
		assertEquals(rs.getString(8),null);
		assertEquals(rs.getString(9),"hello1");
		assertEquals(rs.getString(10),null);
		assertEquals(rs.getString(11),"01:01:01");
		assertEquals(rs.getString(12),null);
		System.out.println();
		assertEquals(stmt.executeUpdate("delete from testtable"),1);
		System.out.println("\n");


		// odd null binds
		System.out.println("ODD NULL BINDS:");
		pstmt=con.prepareStatement(
					"insert into testtable values ("+
					"	?,?,?,?,?,?,?,?,?,?,?,?"+
					")");
		pstmt.setNull(1,java.sql.Types.SMALLINT);
		pstmt.setShort(2,(short)1);
		pstmt.setNull(3,java.sql.Types.INTEGER);
		pstmt.setLong(4,(long)1);
		pstmt.setNull(5,java.sql.Types.DOUBLE);
		pstmt.setDouble(6,1.123);
		pstmt.setNull(7,java.sql.Types.DOUBLE);
		pstmt.setString(8,"hi1");
		pstmt.setNull(9,java.sql.Types.CHAR);
		fmt=new SimpleDateFormat("yyyy-MM-dd");
		pstmt.setDate(10,new java.sql.Date(
					fmt.parse("2001-01-01").
					getTime()));
		pstmt.setNull(11,java.sql.Types.TIME);
		fmt=new SimpleDateFormat("yyyy-MM-dd hh:mm:ss");
		pstmt.setTimestamp(12,new java.sql.Timestamp(
					fmt.parse("2001-01-01 01:01:01").
					getTime()));
		assertEquals(pstmt.execute(),0);
		System.out.println();
		rs=stmt.executeQuery("select * from testtable");
		rsmd=rs.getMetaData();
		assertEquals(rs.next(),1);
		assertEquals(rs.getString(1),null);
		assertEquals(rs.getString(2),"1");
		assertEquals(rs.getString(3),null);
		assertEquals(rs.getString(4),"1");
		assertEquals(rs.getString(5),null);
		assertEquals(rs.getString(6),"1.123");
		assertEquals(rs.getString(7),null);
		assertEquals(rs.getString(8).trim(),"hi1");
		assertEquals(rs.getString(9),null);
		assertEquals(rs.getString(10),"2001-01-01");
		assertEquals(rs.getString(11),null);
		assertEquals(rs.getString(12),"2001-01-01 01:01:01.0");
		System.out.println();
		assertEquals(stmt.executeUpdate("delete from testtable"),1);
		System.out.println("\n");

/*
		// fastload
		System.out.println("FASTLOAD:");
		//int	count=20000;
		//int	count=5000;
		//int	count=500;
		int	count=10;
		//int	count=2;
		//int	count=1;
		for (int i=1; i<=count; i++) {
			pstmt.setInt(1,(i>127)?127:i);
			pstmt.setInt(2,i);
			pstmt.setInt(3,i);
			pstmt.setInt(4,i);
			pstmt.setDouble(5,2.345);
			pstmt.setDouble(6,2.345);
			pstmt.setDouble(7,2.345);
			pstmt.setString(8,"hi");
			pstmt.setString(9,"hello");
			fmt=new SimpleDateFormat("yyyy-MM-dd");
			pstmt.setDate(10,new java.sql.Date(
						fmt.parse("2002-02-02").
						getTime()));
			fmt=new SimpleDateFormat("hh:mm:ss");
			pstmt.setTime(11,new java.sql.Time(
						fmt.parse("02:02:02").
						getTime()));
			fmt=new SimpleDateFormat("yyyy-MM-dd hh:mm:ss");
			pstmt.setTimestamp(12,new java.sql.Timestamp(
					fmt.parse("2002-02-02 02:02:02").
					getTime()));
			pstmt.addBatch();
		}
		int updatecounts[]=pstmt.executeBatch();

		// select
		System.out.println("SELECT:");
		rs=stmt.executeQuery("select * from testtable");
		displayResult(rs);
		rs.close();
*/


		// close
		stmt.close();


		// drop
		System.out.println("DROP:");
		assertEquals(stmt.executeUpdate("drop table testtable"),26);
		// (teradata returns 26 for drop-table, for some reason)
		System.out.println("\n");
}

		con.close();
	}

	static void displayResult(ResultSet rs) throws Exception {
		int	row=1;
		while (rs.next()) {
			System.out.println(row+" {");
			System.out.println("	"+rs.getInt(1));
			System.out.println("	"+rs.getInt(2));
			System.out.println("	"+rs.getInt(3));
			System.out.println("	"+rs.getInt(4));
			System.out.println("	"+rs.getDouble(5));
			System.out.println("	"+rs.getDouble(6));
			System.out.println("	"+rs.getDouble(7));
			System.out.println("	"+rs.getString(8));
			System.out.println("	"+rs.getString(9));
			System.out.println("	"+rs.getString(10));
			System.out.println("	"+rs.getString(11));
			System.out.println("	"+rs.getString(12));
			System.out.println("}");
			row++;
		}
	}

	static void displayStringResult(ResultSet rs) throws Exception {
		ResultSetMetaData	rsmd=rs.getMetaData();
		int	row=1;
		while (rs.next()) {
			System.out.println(row+" {");
			for (int i=1; i<=rsmd.getColumnCount(); i++) {
				System.out.println("	"+
						rsmd.getColumnName(i)+": "+
						rs.getString(i));
			}
			System.out.println("}");
			row++;
		}
	}
}
