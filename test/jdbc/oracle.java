// Copyright (c) David Muse
// See the file COPYING for more information.

import java.sql.*;
import com.firstworks.sqlrelay.*;
import com.firstworks.sql.*;
import java.util.Properties;
import java.util.concurrent.Executors;
import java.util.concurrent.Executor;
import java.nio.charset.StandardCharsets;
import java.net.InetAddress;

class oracle extends sqlrtest {

	public static void main(String args[]) throws Exception {

		String	driver="com.firstworks.sql.SQLRelayDriver";
		String	host="localhost";
		short	port=9000;
		String	socket=null;
		String	user="testuser";
		String	password="testpassword";
		String	url="jdbc:sqlrelay://"+host+":"+port;

		// To use a different jdbc driver, provide the driver, url,
		// username, and password.
		//
		// Eg. for older jdbc:
		// oracle.jdbc.driver.OracleDriver
		// jdbc:oracle:thin:@oracle:1521:ora1
		// fedora40x64
		// testpassword
		if (args.length==4) {
			driver=args[0];
			url=args[1];
			user=args[2];
			password=args[3];
		}

		boolean	issqlrelay=
			driver.equals("com.firstworks.sql.SQLRelayDriver");


		// connection
		System.out.println("CONNECTION:");

		// getConnection
		System.out.println("getConnection");
		Class.forName(driver);
		Connection	con=DriverManager.getConnection(
						url,user,password);
		assertTrue((con!=null));
		System.out.println();

		// close, isClosed, isValid
		System.out.println("close");
		assertFalse(con.isClosed());
		assertTrue(con.isValid(0));
		con.close();
		assertTrue(con.isClosed());
		assertFalse(con.isValid(0));
		con=DriverManager.getConnection(url,user,password);
		assertTrue((con!=null));
		System.out.println();

		// setNetworkTimeout, getNetworkTimeout
		Executor	executor=Executors.newSingleThreadExecutor();
		con.setNetworkTimeout(executor,1);
		assertEquals(con.getNetworkTimeout(),1);
		con.setNetworkTimeout(executor,2);
		assertEquals(con.getNetworkTimeout(),2);
		con.setNetworkTimeout(executor,0);
		assertEquals(con.getNetworkTimeout(),0);
		System.out.println();

		// SQLRelayConnection
		if (issqlrelay) {
			System.out.println("SQLRelayConnection");
			SQLRelayConnection	sqlrcon=(SQLRelayConnection)con;
			assertTrue((sqlrcon!=null));
			assertEquals(sqlrcon.getHost(),host);
			assertEquals(sqlrcon.getPort(),port);
			assertEquals(sqlrcon.getSocket(),socket);
			assertEquals(sqlrcon.getUser(),user);
			assertEquals(sqlrcon.getPassword(),password);
			System.out.println();

			// isWrapperFor, unwrap
			System.out.println("unwrap");
			assertEquals(
				con.isWrapperFor(SQLRConnection.class),1);
			assertEquals(
				(con.unwrap(SQLRConnection.class)!=null),1);
			System.out.println();
		}

		// setCatalog, getCatalog
		// FIXME: sqlrelay currently returns the schema, when run
		// against an oracle backend, rather than null, #7914
		if (!issqlrelay) {
			System.out.println("catalog");
			con.setCatalog(user);
			assertEquals(con.getCatalog(),null);
			System.out.println();
		}

		// setSchema, getSchema
		// FIXME: with sqlrelay, somehow this causes oracle to throw:
		// ORA-01031: insufficient privileges
		if (!issqlrelay) {
			System.out.println("schema");
			con.setSchema(user.toUpperCase());
			assertEquals(con.getSchema(),user.toUpperCase());
			System.out.println();
		}

		// setClientInfo, getClientInfo()
		// Oracle only allows:
		// OCSID.MODULE
		// OCSID.ACTION
		// OCSID.CLIENTID
		// OCSID.ECID (execution context id)
		// OCSID.SEQUENCE_NUMBER
		// OCSID.DBOP (database operation)
		System.out.println("client info");
		Properties	inprop=new Properties();
		inprop.setProperty("OCSID.MODULE","value1");
		inprop.setProperty("OCSID.ACTION","value2");
		con.setClientInfo(inprop);
		con.setClientInfo("OCSID.CLIENTID","value3");
		con.setClientInfo("OCSID.ECID","value4");
		assertEquals(con.getClientInfo("OCSID.MODULE"),"value1");
		assertEquals(con.getClientInfo("OCSID.ACTION"),"value2");
		assertEquals(con.getClientInfo("OCSID.CLIENTID"),"value3");
		assertEquals(con.getClientInfo("OCSID.ECID"),"value4");
		Properties	outprop=con.getClientInfo();
		assertEquals(outprop.getProperty("OCSID.MODULE"),"value1");
		assertEquals(outprop.getProperty("OCSID.ACTION"),"value2");
		assertEquals(outprop.getProperty("OCSID.CLIENTID"),"value3");
		assertEquals(outprop.getProperty("OCSID.ECID"),"value4");
		System.out.println();

		// setReadOnly, isReadOnly
		System.out.println("readonly");
		con.setReadOnly(true);
		assertTrue(con.isReadOnly());
		con.setReadOnly(false);
		assertTrue(!con.isReadOnly());
		System.out.println();

		// setAutoCommit, getAutoCommit
		System.out.println("autocommit");
		con.setAutoCommit(true);
		assertTrue(con.getAutoCommit());
		con.setAutoCommit(false);
		assertTrue(!con.getAutoCommit());
		System.out.println();

		// setHoldability, getHoldability
		System.out.println("holdability");
		con.setHoldability(ResultSet.HOLD_CURSORS_OVER_COMMIT);
		assertEquals(con.getHoldability()==
				ResultSet.HOLD_CURSORS_OVER_COMMIT,1);
		try {
			con.setHoldability(ResultSet.CLOSE_CURSORS_AT_COMMIT);
			assertTrue(false);
		} catch (Exception ex) {
			assertTrue(true);
		}
		System.out.println();

		// setTransactionIsolation
		System.out.println("isolation levels");

		// oracle requires the isolation level to
		// be the first query of the transaction
		con.commit();

		// you can set the isolation level, but to get it, you
		// have to have permisisons to read from sys.v_$session
		// and sys.v_$transaction, so we'll just set them here
		try {
			con.setTransactionIsolation(
				Connection.
				TRANSACTION_READ_UNCOMMITTED);
			assertTrue(false);
		} catch (Exception ex) {
			assertTrue(true);
		}

		con.commit();
		con.setTransactionIsolation(
			Connection.TRANSACTION_READ_COMMITTED);
		assertTrue(true);

		con.commit();
		try {
			con.setTransactionIsolation(
				Connection.
				TRANSACTION_REPEATABLE_READ);
			assertTrue(false);
		} catch (Exception ex) {
			assertTrue(true);
		}

		con.commit();
		con.setTransactionIsolation(
			Connection.TRANSACTION_SERIALIZABLE);
		assertTrue(true);

		con.commit();
		con.setTransactionIsolation(
			Connection.TRANSACTION_READ_COMMITTED);
		assertTrue(true);
		System.out.println();

		// setTypeMap, getTypeMap

		// getWarnings, clearWarnings
		System.out.println("warnings");
		assertTrue(con.getWarnings()==null);
		con.clearWarnings();
		System.out.println();

		System.out.println();



		// DatabaseMetaData


		// database meta data
		System.out.println("DATABASE META DATA:");

		// getMetaData
		System.out.println("getMetaData");
		DatabaseMetaData	md=con.getMetaData();
		assertTrue((md!=null));
		System.out.println();

		// getCatalogs
		System.out.println("catalogs");
		ResultSet	rs=md.getCatalogs();
		assertTrue((rs!=null));
		ResultSetMetaData	rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),1);
		int	col=1;
		assertEquals(rsmd.getColumnName(col++),"TABLE_CAT");
		while (rs.next()) {
			System.out.println(rs.getString("TABLE_CAT"));
		}
		rs.close();
		System.out.println();

		// getSchemas
		System.out.println("schemas");
		rs=md.getSchemas();
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),2);
		col=1;
		assertEquals(rsmd.getColumnName(col++),"TABLE_SCHEM");
		assertEquals(rsmd.getColumnName(col++),"TABLE_CATALOG");
		//System.out.println();
		//printColumns(rsmd);
		//printResultSet(rs);
		rs.close();
		System.out.println();

		// getTableTypes
		System.out.println("table types");
		rs=md.getTableTypes();
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),1);
		col=1;
		assertEquals(rsmd.getColumnName(col++),"TABLE_TYPE");
		//System.out.println();
		//printColumns(rsmd);
		//printResultSet(rs);
		rs.close();
		System.out.println();

		// getTables
// takes a while, disable for now
if (false) {
		System.out.println("tables");
		rs=md.getTables("%","%","%",
			new String[] {"SYNONYM","TABLE","VIEW"});
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		if (issqlrelay) {
                	assertEquals(rsmd.getColumnCount(),10);
		} else {
			// oracle jdbc (at least v8) returns 5 columns
                	assertEquals(rsmd.getColumnCount(),5);
		}
		col=1;
		assertEquals(rsmd.getColumnName(col++),"TABLE_CAT");
		assertEquals(rsmd.getColumnName(col++),"TABLE_SCHEM");
		assertEquals(rsmd.getColumnName(col++),"TABLE_NAME");
		assertEquals(rsmd.getColumnName(col++),"TABLE_TYPE");
		assertEquals(rsmd.getColumnName(col++),"REMARKS");
		if (issqlrelay) {
			assertEquals(rsmd.getColumnName(col++),"TYPE_CAT");
			assertEquals(rsmd.getColumnName(col++),"TYPE_SCHEM");
			assertEquals(rsmd.getColumnName(col++),"TYPE_NAME");
			assertEquals(rsmd.getColumnName(col++),
						"SELF_REFERENCING_COL_NAME");
			assertEquals(rsmd.getColumnName(col++),
						"REF_GENERATION");
		}
		//System.out.println();
		//printColumns(rsmd);
		//printResultSet(rs);
		rs.close();
		System.out.println();
}

		// getSuperTables (oracle jdbc doesn't supported this)

		// getTypeInfo
		System.out.println("type info");
		rs=md.getTypeInfo();
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),18);
		col=1;
		assertEquals(rsmd.getColumnName(col++),"TYPE_NAME");
		assertEquals(rsmd.getColumnName(col++),"DATA_TYPE");
		assertEquals(rsmd.getColumnName(col++),"PRECISION");
		assertEquals(rsmd.getColumnName(col++),"LITERAL_PREFIX");
		assertEquals(rsmd.getColumnName(col++),"LITERAL_SUFFIX");
		assertEquals(rsmd.getColumnName(col++),"CREATE_PARAMS");
		assertEquals(rsmd.getColumnName(col++),"NULLABLE");
		assertEquals(rsmd.getColumnName(col++),"CASE_SENSITIVE");
		assertEquals(rsmd.getColumnName(col++),"SEARCHABLE");
		assertEquals(rsmd.getColumnName(col++),"UNSIGNED_ATTRIBUTE");
		assertEquals(rsmd.getColumnName(col++),"FIXED_PREC_SCALE");
		assertEquals(rsmd.getColumnName(col++),"AUTO_INCREMENT");
		assertEquals(rsmd.getColumnName(col++),"LOCAL_TYPE_NAME");
		assertEquals(rsmd.getColumnName(col++),"MINIMUM_SCALE");
		assertEquals(rsmd.getColumnName(col++),"MAXIMUM_SCALE");
		assertEquals(rsmd.getColumnName(col++),"SQL_DATA_TYPE");
		assertEquals(rsmd.getColumnName(col++),"SQL_DATETIME_SUB");
		assertEquals(rsmd.getColumnName(col++),"NUM_PREC_RADIX");
		//System.out.println();
		//printColumns(rsmd);
		//printResultSet(rs);
		rs.close();
		System.out.println();

		// getProcedures
// takes a while, disable for now
if (false) {
                System.out.println("procedures");
                rs=md.getProcedures("%","%","%");
                assertTrue((rs!=null));
                rsmd=rs.getMetaData();
                assertTrue((rsmd!=null));
		col=1;
		if (issqlrelay) {
                	assertEquals(rsmd.getColumnCount(),8);
		} else {
			// oracle jdbc (at least v8) returns 9 columns
                	assertEquals(rsmd.getColumnCount(),9);
		}
                assertEquals(rsmd.getColumnName(col++),"PROCEDURE_CAT");
                assertEquals(rsmd.getColumnName(col++),"PROCEDURE_SCHEM");
                assertEquals(rsmd.getColumnName(col++),"PROCEDURE_NAME");
		if (issqlrelay) {
                	assertEquals(rsmd.getColumnName(col++),
						"NUM_INPUT_PARAMS");
                	assertEquals(rsmd.getColumnName(col++),
						"NUM_OUTPUT_PARAMS");
                	assertEquals(rsmd.getColumnName(col++),
						"NUM_RESULT_SETS");
		} else {
			// oracle jdbc (at least v8) returns
			// NULL for these column names
                	assertEquals(rsmd.getColumnName(col++),"NULL");
                	assertEquals(rsmd.getColumnName(col++),"NULL");
                	assertEquals(rsmd.getColumnName(col++),"NULL");
		}
                assertEquals(rsmd.getColumnName(col++),"REMARKS");
                assertEquals(rsmd.getColumnName(col++),"PROCEDURE_TYPE");
		if (!issqlrelay) {
			// oracle jdbc (at least v8) returns a 9th column
                	assertEquals(rsmd.getColumnName(col++),"SPECIFIC_NAME");
		}
                //System.out.println();
		//printColumns(rsmd);
		//printResultSet(rs);
                rs.close();
                System.out.println();
}

		// getFunctions
		// FIXME: sqlrelay doesn't support this yet
		if (!issqlrelay) {
                	System.out.println("functions");
                	rs=md.getFunctions("%","%","%");
                	assertTrue((rs!=null));
                	rsmd=rs.getMetaData();
                	assertTrue((rsmd!=null));
			col=1;
			if (issqlrelay) {
				assertEquals(rsmd.getColumnCount(),8);
			} else {
				// oracle jdbc (at least v8) returns 6 columns
				assertEquals(rsmd.getColumnCount(),6);
			}
                	assertEquals(rsmd.getColumnName(col++),
							"FUNCTION_CAT");
                	assertEquals(rsmd.getColumnName(col++),
							"FUNCTION_SCHEM");
                	assertEquals(rsmd.getColumnName(col++),
							"FUNCTION_NAME");
			// oracle jdbc (at least v8) doesn't
			// return these columns at all
			if (issqlrelay) {
                		assertEquals(rsmd.getColumnName(col++),
							"NUM_INPUT_PARAMS");
                		assertEquals(rsmd.getColumnName(col++),
							"NUM_OUTPUT_PARAMS");
                		assertEquals(rsmd.getColumnName(col++),
							"NUM_RESULT_SETS");
			}
                	assertEquals(rsmd.getColumnName(col++),"REMARKS");
                	assertEquals(rsmd.getColumnName(col++),"FUNCTION_TYPE");
			// oracle jdbc (at least v8) returns this extra column
			if (!issqlrelay) {
                		assertEquals(rsmd.getColumnName(col++),
							"SPECIFIC_NAME");
			}
                	//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
                	rs.close();
                	System.out.println();
		}

		// getUDTs
		// FIXME: sqlrelay doesn't support this yet
		// oracle jdbc (at least v8) throws:
		// ORA-08177: can't serialize access for this transaction
		if (false) {
                	System.out.println("UDTs");
                	rs=md.getUDTs("%","%","%",null);
                	assertTrue((rs!=null));
                	rsmd=rs.getMetaData();
                	assertTrue((rsmd!=null));
			col=1;
			if (issqlrelay) {
                		assertEquals(rsmd.getColumnCount(),7);
			} else {
				// oracle jdbc (at least v8) returns 6 columns
                		assertEquals(rsmd.getColumnCount(),6);
			}
                	assertEquals(rsmd.getColumnName(col++),"TYPE_CAT");
                	assertEquals(rsmd.getColumnName(col++),"TYPE_SCHEM");
                	assertEquals(rsmd.getColumnName(col++),"TYPE_NAME");
                	assertEquals(rsmd.getColumnName(col++),"CLASS_NAME");
                	assertEquals(rsmd.getColumnName(col++),"DATA_TYPE");
                	assertEquals(rsmd.getColumnName(col++),"REMARKS");
			if (issqlrelay) {
				// oracle jdbc (at least v8) doesn't
				// return this column at all
                		assertEquals(rsmd.getColumnName(col++),
								"BASE_TYPE");
			}
                	//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
                	rs.close();
                	System.out.println();
		}


		// statement
		System.out.println("STATEMENT:");

		// createStatement
		System.out.println("create statement");
		Statement	stmt=con.createStatement();
		assertTrue((stmt!=null));
		stmt.close();
                System.out.println();

		// result set types
		int[]	rstype={
			ResultSet.TYPE_FORWARD_ONLY,
			ResultSet.TYPE_SCROLL_INSENSITIVE,
			ResultSet.TYPE_SCROLL_SENSITIVE
		};
		boolean[]	rstypesupported=new boolean[3];
		rstypesupported[0]=true;
		rstypesupported[1]=true;
		// oracle jdbc supports scroll-sensitive cursors
		rstypesupported[2]=(issqlrelay)?false:true;
		String[]	rstypename={
			"forward only",
			"scroll insensitive",
			"scroll sensitive"
		};

		// result set concurrency
		int[]	concurrency={
			ResultSet.CONCUR_READ_ONLY,
			ResultSet.CONCUR_UPDATABLE
		};
		boolean[]	concurrencysupported=new boolean[2];
		concurrencysupported[0]=true;
		// oracle jdbc supports updatable cursors
		concurrencysupported[1]=(issqlrelay)?false:true;
		String[]	concurrencyname={
			"read only",
			"updatable"
		};

		// result set holdability
		int[]	holdability={
			ResultSet.HOLD_CURSORS_OVER_COMMIT,
			ResultSet.CLOSE_CURSORS_AT_COMMIT
		};
		boolean[]	holdabilitysupported={
			true,false
		};
		String[]	holdabilityname={
			"hold cursors",
			"close cursors"
		};

		// test all combinations
		for (int r=0; r<rstype.length; r++) {
			for (int c=0; c<concurrency.length; c++) {
				System.out.println(
					"create statement - "+
					rstypename[r]+", "+
					concurrencyname[c]);
				if (rstypesupported[r] &&
					concurrencysupported[c]) {
					stmt=con.createStatement(
							rstype[r],
							concurrency[c]);
					assertTrue((stmt!=null));
					stmt.close();
				} else {
					boolean	supported=
						(rstypesupported[r] &&
						concurrencysupported[c]);
					try {
						stmt=con.
						createStatement(
							rstype[r],
							concurrency[c]);
						assertTrue(supported);
					} catch (Exception ex) {
						assertFalse(supported);
					}
				}
				System.out.println();
			}
		}
		for (int r=0; r<rstype.length; r++) {
			for (int c=0; c<concurrency.length; c++) {
				for (int h=0; h<holdability.length; h++) {
					System.out.println(
						"create statement - "+
						rstypename[r]+", "+
						concurrencyname[c]+", "+
						holdabilityname[h]);
					if (rstypesupported[r] &&
						concurrencysupported[c] &&
						holdabilitysupported[h]) {
						stmt=con.createStatement(
							rstype[r],
							concurrency[c],
							holdability[h]);
						assertTrue((stmt!=null));
						stmt.close();
					} else {
						boolean	supported=
						(rstypesupported[r] &&
						concurrencysupported[c] &&
						holdabilitysupported[h]);
						try {
							stmt=con.
							createStatement(
								rstype[r],
								concurrency[c],
								holdability[h]);
							assertTrue(supported);
						} catch (Exception ex) {
							assertFalse(supported);
						}
					}
					System.out.println();
				}
			}
		}
		System.out.println();



		// drop existing table
		stmt=con.createStatement();
		try {
			stmt.executeUpdate("drop table testtable");
		} catch (Exception ex) {
		}


		// create temptable
		System.out.println("CREATE TEMPTABLE:");
		assertEquals(stmt.executeUpdate("create table testtable (testnumber number, testchar char(40), testvarchar varchar2(40), testdate date, testlong long, testclob clob, testblob blob)"),0);
		System.out.println();


		// insert
		System.out.println("INSERT:");
		assertEquals(stmt.executeUpdate("insert into testtable values (1,'testchar1','testvarchar1','01-JAN-2001','testlong1','testclob1',empty_blob())"),1);
		stmt.close();
		System.out.println();


		// bind by position
		System.out.println("BIND BY POSITION:");
		PreparedStatement	pstmt=con.prepareStatement("insert into testtable values (:var1,:var2,:var3,:var4,:var5,:var6,:var7)");
		pstmt.setInt(1,2);
		pstmt.setString(2,"testchar2");
		pstmt.setString(3,"testvarchar2");
		pstmt.setDate(4,new java.sql.Date(2002,1,1));
		pstmt.setString(5,"testlong2");
		pstmt.setString(6,"testclob2");
		pstmt.setBytes(7,(new String("testblob2")).
				getBytes(StandardCharsets.UTF_8));
		assertEquals(pstmt.executeUpdate(),1);
		pstmt.clearParameters();
		pstmt.setInt(1,3);
		pstmt.setString(2,"testchar3");
		pstmt.setString(3,"testvarchar3");
		pstmt.setDate(4,new java.sql.Date(2003,1,1));
		pstmt.setString(5,"testlong3");
		pstmt.setString(6,"testclob3");
		pstmt.setBytes(7,(new String("testblob3")).
				getBytes(StandardCharsets.UTF_8));
		assertEquals(pstmt.executeUpdate(),1);
		System.out.println();
		pstmt.clearParameters();
		pstmt.setInt(1,4);
		pstmt.setString(2,"testchar4");
		pstmt.setString(3,"testvarchar4");
		pstmt.setDate(4,new java.sql.Date(2004,1,1));
		pstmt.setString(5,"testlong4");
		pstmt.setString(6,"testclob4");
		pstmt.setBytes(7,(new String("testblob4")).
				getBytes(StandardCharsets.UTF_8));
		assertEquals(pstmt.executeUpdate(),1);
		System.out.println();
		pstmt.clearParameters();
		pstmt.setInt(1,5);
		pstmt.setString(2,"testchar5");
		pstmt.setString(3,"testvarchar5");
		pstmt.setDate(4,new java.sql.Date(2005,1,1));
		pstmt.setString(5,"testlong5");
		pstmt.setString(6,"testclob5");
		pstmt.setBytes(7,(new String("testblob5")).
				getBytes(StandardCharsets.UTF_8));
		assertEquals(pstmt.executeUpdate(),1);
		System.out.println();
		pstmt.clearParameters();
		pstmt.setInt(1,6);
		pstmt.setString(2,"testchar6");
		pstmt.setString(3,"testvarchar6");
		pstmt.setDate(4,new java.sql.Date(2006,1,1));
		pstmt.setString(5,"testlong6");
		pstmt.setString(6,"testclob6");
		pstmt.setBytes(7,(new String("testblob6")).
				getBytes(StandardCharsets.UTF_8));
		assertEquals(pstmt.executeUpdate(),1);
		System.out.println();
		pstmt.clearParameters();
		pstmt.setInt(1,7);
		pstmt.setString(2,"testchar7");
		pstmt.setString(3,"testvarchar7");
		pstmt.setDate(4,new java.sql.Date(2007,1,1));
		pstmt.setString(5,"testlong7");
		pstmt.setString(6,"testclob7");
		pstmt.setBytes(7,(new String("testblob7")).
				getBytes(StandardCharsets.UTF_8));
		assertEquals(pstmt.executeUpdate(),1);
		System.out.println();
		pstmt.clearParameters();
		pstmt.setInt(1,8);
		pstmt.setString(2,"testchar8");
		pstmt.setString(3,"testvarchar8");
		pstmt.setDate(4,new java.sql.Date(2008,1,1));
		pstmt.setString(5,"testlong8");
		pstmt.setString(6,"testclob8");
		pstmt.setBytes(7,(new String("testblob8")).
				getBytes(StandardCharsets.UTF_8));
		assertEquals(pstmt.executeUpdate(),1);
		pstmt.close();
		System.out.println();


		// OUTPUT BINDS


		// select
		System.out.println("SELECT:");
		stmt=con.createStatement();
		assertTrue((stmt!=null));
		rs=stmt.executeQuery("select * from testtable order by testnumber");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		System.out.println();


		// column count
		System.out.println("COLUMN COUNT:");
		assertEquals(rsmd.getColumnCount(),7);
		System.out.println();


		// column names
		System.out.println("COLUMN NAMES:");
		assertEquals(rsmd.getColumnName(1),"TESTNUMBER");
		assertEquals(rsmd.getColumnName(2),"TESTCHAR");
		assertEquals(rsmd.getColumnName(3),"TESTVARCHAR");
		assertEquals(rsmd.getColumnName(4),"TESTDATE");
		assertEquals(rsmd.getColumnName(5),"TESTLONG");
		assertEquals(rsmd.getColumnName(6),"TESTCLOB");
		assertEquals(rsmd.getColumnName(7),"TESTBLOB");
		System.out.println();


		// column types
		System.out.println("COLUMN TYPES:");
		assertEquals(rsmd.getColumnTypeName(1),"NUMBER");
		assertEquals(rsmd.getColumnTypeName(2),"CHAR");
		assertEquals(rsmd.getColumnTypeName(3),"VARCHAR2");
		assertEquals(rsmd.getColumnTypeName(4),"DATE");
		assertEquals(rsmd.getColumnTypeName(5),"LONG");
		assertEquals(rsmd.getColumnTypeName(6),"CLOB");
		assertEquals(rsmd.getColumnTypeName(7),"BLOB");
		System.out.println();


		// column length
		System.out.println("COLUMN LENGTH:");
		assertEquals(rsmd.getPrecision(1),0);
		assertEquals(rsmd.getPrecision(2),40);
		assertEquals(rsmd.getPrecision(3),40);
		assertEquals(rsmd.getPrecision(4),7);
		assertEquals(rsmd.getPrecision(5),2147483647);
		assertEquals(rsmd.getPrecision(6),-1);
		assertEquals(rsmd.getPrecision(7),-1);
		System.out.println();


		// longest column
		System.out.println("LONGEST COLUMN:");
		assertEquals(rsmd.getColumnDisplaySize(1),39);
		assertEquals(rsmd.getColumnDisplaySize(2),40);
		assertEquals(rsmd.getColumnDisplaySize(3),40);
		assertEquals(rsmd.getColumnDisplaySize(4),7);
		assertEquals(rsmd.getColumnDisplaySize(5),0);
		assertEquals(rsmd.getColumnDisplaySize(6),4000);
		assertEquals(rsmd.getColumnDisplaySize(7),4000);
		System.out.println();


		// fields by index
		System.out.println("FIELDS BY INDEX:");
		rs.next();
		assertEquals(rs.getString(1),"1");
		assertEquals(rs.getString(2),"testchar1                               ");
		assertEquals(rs.getString(3),"testvarchar1");
		if (issqlrelay) {
			assertEquals(rs.getString(4),"01-JAN-01");
		} else {
			// oracle jdbc returns this format, independent
			// of how you set NLS_DATE_FORMAT
			assertEquals(rs.getString(4),"2001-01-01 00:00:00");
		}
		assertEquals(rs.getString(5),"testlong1");
		assertEquals(rs.getString(6),"testclob1");
		// oracle jdbc can't convert a blob directly to a string
		Blob	bl=rs.getBlob(7);
		byte[]	b=null;
		if (issqlrelay) {
			// SerialBlob doesn't like a length of 0.
			//
			// Oracle jdbc returns its own Blob implementation
			// that's tolerant to this.
			//
			// For now we're not implementing our own Blob.
			if (bl.length()==0) {
				b=new byte[0];
			} else {
				b=bl.getBytes(1,(int)bl.length());
			}
		} else {
			b=bl.getBytes(1,(int)bl.length());
		}
		assertEquals(new String(b,"UTF-8"),"");
		System.out.println();
		for (int i=0; i<7; i++) {
			rs.next();
		}
		assertEquals(rs.getString(1),"8");
		assertEquals(rs.getString(2),"testchar8                               ");
		assertEquals(rs.getString(3),"testvarchar8");
		if (issqlrelay) {
			assertEquals(rs.getString(4),"01-JAN-08");
		} else {
			// oracle jdbc returns this format, independent
			// of how you set NLS_DATE_FORMAT
// FIXME: some weird bug causes the date to come back as 3908-01-01 00:00:00
if (false) {
			assertEquals(rs.getString(4),"2008-01-01 00:00:00");
}
		}
		assertEquals(rs.getString(5),"testlong8");
		assertEquals(rs.getString(6),"testclob8");
		// oracle jdbc can't getString() on a blob
		bl=rs.getBlob(7);
		b=bl.getBytes(1,(int)bl.length());
		assertEquals(new String(b,"UTF-8"),"testblob8");
		System.out.println();


		// fields by name
		System.out.println("FIELDS BY NAME:");
		assertTrue((stmt!=null));
		rs=stmt.executeQuery("select * from testtable order by testnumber");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString("TESTNUMBER"),"1");
		assertEquals(rs.getString("TESTCHAR"),"testchar1                               ");
		assertEquals(rs.getString("TESTVARCHAR"),"testvarchar1");
		if (issqlrelay) {
			assertEquals(rs.getString("TESTDATE"),"01-JAN-01");
		} else {
			// oracle jdbc returns this format, independent
			// of how you set NLS_DATE_FORMAT
			assertEquals(rs.getString("TESTDATE"),"2001-01-01 00:00:00");
		}
		assertEquals(rs.getString("TESTLONG"),"testlong1");
		assertEquals(rs.getString("TESTCLOB"),"testclob1");
		// oracle jdbc can't convert a blob directly to a string
		bl=rs.getBlob("TESTBLOB");
		b=null;
		if (issqlrelay) {
			// SerialBlob doesn't like a length of 0.
			//
			// Oracle jdbc returns its own Blob implementation
			// that's tolerant to this.
			//
			// For now we're not implementing our own Blob.
			if (bl.length()==0) {
				b=new byte[0];
			} else {
				b=bl.getBytes(1,(int)bl.length());
			}
		} else {
			b=bl.getBytes(1,(int)bl.length());
		}
		assertEquals(new String(b,"UTF-8"),"");
		System.out.println();
		for (int i=0; i<7; i++) {
			rs.next();
		}
		assertEquals(rs.getString("TESTNUMBER"),"8");
		assertEquals(rs.getString("TESTCHAR"),"testchar8                               ");
		assertEquals(rs.getString("TESTVARCHAR"),"testvarchar8");
		if (issqlrelay) {
			assertEquals(rs.getString("TESTDATE"),"01-JAN-08");
		} else {
			// oracle jdbc returns this format, independent
			// of how you set NLS_DATE_FORMAT
// FIXME: some weird bug causes the date to come back as 3908-01-01 00:00:00
if (false) {
			assertEquals(rs.getString("TESTDATE"),"2008-01-01 00:00:00");
}
		}
		assertEquals(rs.getString("TESTLONG"),"testlong8");
		assertEquals(rs.getString("TESTCLOB"),"testclob8");
		// oracle jdbc can't getString() on a blob
		bl=rs.getBlob("TESTBLOB");
		b=bl.getBytes(1,(int)bl.length());
		assertEquals(new String(b,"UTF-8"),"testblob8");
		System.out.println();


		// row count
		System.out.println("ROW COUNT:");
		assertEquals(rs.getRow(),8);
		rs.close();
		System.out.println();


		// OUTPUT BIND


		// commit and rollback
		System.out.println("COMMIT AND ROLLBACK:");
		Connection	secondcon=DriverManager.getConnection(
							url,user,password);
		assertTrue((secondcon!=null));
		Statement	secondstmt=secondcon.createStatement();
		assertTrue((secondstmt!=null));
		ResultSet	secondrs=secondstmt.executeQuery("select count(*) from testtable");
		assertTrue((secondrs!=null));
		secondrs.next();
		assertEquals(secondrs.getString(1),"0");
		secondrs.close();
		con.commit();
		secondrs=secondstmt.executeQuery("select count(*) from testtable");
		assertTrue((secondrs!=null));
		secondrs.next();
		assertEquals(secondrs.getString(1),"8");
		con.setAutoCommit(true);
		secondrs.close();
		assertEquals(stmt.executeUpdate("insert into testtable values (10,'testchar10','testvarchar10','01-JAN-2010','testlong10','testclob10',NULL)"),1);
		secondrs=secondstmt.executeQuery("select count(*) from testtable");
		assertTrue((secondrs!=null));
		secondrs.next();
		assertEquals(secondrs.getString(1),"9");
		secondrs.close();
		secondstmt.close();
		con.setAutoCommit(false);
		System.out.println();


		// CLOB AND BLOB OUTPUT BIND


		try {
			stmt.executeUpdate("drop table testtable1");
		} catch (Exception ex) {
		}


		// null and empty clobs and blobs
		System.out.println("NULL AND EMPTY CLOBS AND BLOBS:");
		assertEquals(stmt.executeUpdate("create table testtable1 (testclob1 clob, testclob2 clob, testblob1 blob, testblob2 blob)"),0);
		pstmt=con.prepareStatement("insert into testtable1 values (:var1,:var2,:var3,:var4)");
		assertTrue((pstmt!=null));
		pstmt.setString(1,"");
		pstmt.setString(2,null);
		pstmt.setString(3,"");
		pstmt.setString(4,null);
		assertEquals(pstmt.executeUpdate(),1);
		pstmt.close();
		rs=stmt.executeQuery("select * from testtable1");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),null);
		assertEquals(rs.getString(2),null);
		// oracle jdbc can't getString() on a blob
		assertTrue((rs.getBlob(3)==null));
		assertTrue((rs.getBlob(4)==null));
		assertEquals(stmt.executeUpdate("drop table testtable1"),0);
		System.out.println();


		// CURSOR BINDS


		try {
			stmt.executeUpdate("drop table testtable2");
		} catch (Exception ex) {
		}


		// long clob
		System.out.println("LONG CLOB:");
		assertEquals(stmt.executeUpdate("create table testtable2 (testclob clob)"),0);
		pstmt=con.prepareStatement("insert into testtable2 values (:clobval)");
		assertTrue((pstmt!=null));
		StringBuilder	clobval=new StringBuilder();
		// oracle jdbc struggles with more than 1024 byte clobs
		for (int i=0; i<1024; i++) {
			clobval.append('C');
		}
		String	clobstr=clobval.toString();
		pstmt.setString(1,clobstr);
		assertEquals(pstmt.executeUpdate(),1);
		rs=stmt.executeQuery("select testclob from testtable2");
		assertTrue((rs!=null));
		rs.next();
		Clob	cl=rs.getClob(1);
		assertEquals(clobstr,cl.getSubString(1,(int)cl.length()));
		rs.close();
		// FIXME: use callable statement?
		/*cur->prepareQuery("begin select testclob into :clobbindval from testtable2; end;");
		cur->defineOutputBindClob("clobbindval");
		assertTrue(cur->executeQuery());
		const char	*clobbindvar=cur->getOutputBindClob("clobbindval");
		assertEquals(cur->getOutputBindLength("clobbindval"),8*1024);
		assertEquals(clobval,clobbindvar);
		cur->sendQuery("drop table testtable2");*/
		System.out.println();


		// LONG OUTPUT BIND



		// drop existing table
		stmt.executeUpdate("drop table testtable");


		// temporary tables
		System.out.println("TEMPORARY TABLES:");
		String	hostname=InetAddress.getLocalHost().
					getHostName().split("\\.")[0];
		try {
			assertEquals(stmt.executeUpdate("drop table "+hostname+"_temptabledelete"),0);
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate("create global temporary table "+hostname+"_temptabledelete (col1 number) on commit delete rows"),0);
		assertEquals(stmt.executeUpdate("insert into "+hostname+"_temptabledelete values (1)"),1);
		rs=stmt.executeQuery("select count(*) from "+hostname+"_temptabledelete");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"1");
		con.commit();
		rs=stmt.executeQuery("select count(*) from "+hostname+"_temptabledelete");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"0");
		assertEquals(stmt.executeUpdate("drop table "+hostname+"_temptabledelete"),0);
		System.out.println();
		try {
			stmt.executeUpdate("truncate table "+hostname+"_temptablepreserve");
		} catch (Exception ex) {
		}
		try {
			stmt.executeUpdate("drop table "+hostname+"_temptablepreserve");
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate("create global temporary table "+hostname+"_temptablepreserve (col1 number) on commit preserve rows"),0);
		assertEquals(stmt.executeUpdate("insert into "+hostname+"_temptablepreserve values (1)"),1);
		rs=stmt.executeQuery("select count(*) from "+hostname+"_temptablepreserve");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"1");
		con.commit();
		rs=stmt.executeQuery("select count(*) from "+hostname+"_temptablepreserve");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"1");
		con.close();
		System.out.println();
		con=DriverManager.getConnection(url,user,password);
		assertTrue((con!=null));
		stmt=secondcon.createStatement();
		assertTrue((stmt!=null));
		rs=stmt.executeQuery("select count(*) from "+hostname+"_temptablepreserve");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"0");
		assertEquals(stmt.executeUpdate("truncate table "+hostname+"_temptablepreserve"),0);
		Thread.sleep(2000);
		assertEquals(stmt.executeUpdate("drop table "+hostname+"_temptablepreserve"),0);
		try {
			stmt.executeQuery("select count(*) from "+hostname+"_temptablepreserve");
			assertTrue(false);
		} catch (Exception ex) {
			assertTrue(true);
		}
		System.out.println();


		// STORED PROCEDURES


		// IN/OUT VARIABLES


		// REBINDING


		// invalid queries
		System.out.println("INVALID QUERIES:");
		try {
			stmt.executeQuery("select * from testtable order by testnumber");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeQuery("select * from testtable order by testnumber");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeQuery("select * from testtable order by testnumber");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeQuery("select * from testtable order by testnumber");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		System.out.println();
		try {
			stmt.executeUpdate("insert into testtable values (1,2,3,4)");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeUpdate("insert into testtable values (1,2,3,4)");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeUpdate("insert into testtable values (1,2,3,4)");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeUpdate("insert into testtable values (1,2,3,4)");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		System.out.println();
		try {
			stmt.executeUpdate("create table testtable");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeUpdate("create table testtable");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeUpdate("create table testtable");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeUpdate("create table testtable");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		System.out.println();

		stmt.close();
		con.close();

		reportTestStatus();

		System.exit(status);
	}
}
