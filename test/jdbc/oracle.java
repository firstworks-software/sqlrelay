// Copyright (c) David Muse
// See the file COPYING for more information.

import java.sql.*;
import com.firstworks.sqlrelay.*;
import com.firstworks.sql.*;
import java.util.Properties;
import java.util.concurrent.Executors;
import java.util.concurrent.Executor;
import java.nio.charset.StandardCharsets;

class oracle {

	private static void checkSuccess(String value,
						String success,
						int length) {
	
		if (success==null) {
			if (value==null) {
				System.out.printf("success ");
				return;
			} else {
				System.out.printf(value+"!="+success+" ");
				System.out.println("failure ");
				System.exit(1);
			}
		}
	
		if (value.regionMatches(0,success,0,length)) {
			System.out.printf("success ");
		} else {
			System.out.printf(value+"!="+success+" ");
			System.out.println("failure ");
			System.exit(1);
		}
	}
	
	private static void checkSuccess(String value, String success) {
	
		if (success==null) {
			if (value==null) {
				System.out.printf("success ");
				return;
			} else {
				System.out.printf(value+"!="+success+" ");
				System.out.println("failure ");
				System.exit(1);
			}
		}
	
		if (value.equals(success)) {
			System.out.printf("success ");
		} else {
			System.out.printf(value+"!="+success+" ");
			System.out.println("failure ");
			System.exit(1);
		}
	}
	
	private static void checkSuccess(byte[] value,
						String success,
						int length) {
	
		if (success==null) {
			if (value==null) {
				System.out.printf("success ");
				return;
			} else {
				System.out.println("failure ");
				System.exit(1);
			}
		}

		byte[]	successvalue=success.getBytes();
	
		for (int index=0; index<length; index++) {
			if (value[index]!=successvalue[index]) {
				System.out.println("failure ");
				System.exit(1);
			}
		}
		System.out.printf("success ");
	}
	
	private static void checkSuccess(long value, int success) {
	
		if (value==success) {
			System.out.printf("success ");
		} else {
			System.out.printf(value+"!="+success+" ");
			System.out.println("failure ");
			System.exit(1);
		}
	}
	
	private static void checkSuccess(double value, double success) {
	
		if (value==success) {
			System.out.printf("success ");
		} else {
			System.out.printf(value+"!="+success+" ");
			System.out.println("failure ");
			System.exit(1);
		}
	}
	
	private static void checkSuccess(boolean value, int success) {
	
		if (((value)?1:0)==success) {
			System.out.printf("success ");
		} else {
			System.out.printf(value+"!="+success+" ");
			System.out.println("failure ");
			System.exit(1);
		}
	}

	private static void printColumns(ResultSetMetaData rsmd)
							throws Exception {
		System.out.println();
		for (int i=1; i<rsmd.getColumnCount()+1; i++) {
			System.out.println(rsmd.getColumnName(i));
		}
	}

	private static void printResultSet(ResultSet rs) throws Exception {
		ResultSetMetaData	rsmd=rs.getMetaData();
		System.out.println();
		while (rs.next()) {
			for (int i=1; i<rsmd.getColumnCount()+1; i++) {
                        	System.out.print(rs.getString(i)+",");
			}
                        System.out.println();
		}
	}

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



		System.out.println("CONNECTION:");

		// getConnection
		System.out.println("getConnection");
		Class.forName(driver);
		Connection	con=DriverManager.getConnection(
						url,user,password);

		// close, isClosed, isValid
		System.out.println("close");
		checkSuccess(con.isClosed(),0);
		checkSuccess(con.isValid(0),1);
		con.close();
		checkSuccess(con.isClosed(),1);
		checkSuccess(con.isValid(0),0);
		con=DriverManager.getConnection(url,user,password);
		System.out.println();

		// setNetworkTimeout, getNetworkTimeout
		Executor	executor=Executors.newSingleThreadExecutor();
		con.setNetworkTimeout(executor,1);
		checkSuccess(con.getNetworkTimeout(),1);
		con.setNetworkTimeout(executor,2);
		checkSuccess(con.getNetworkTimeout(),2);
		con.setNetworkTimeout(executor,0);
		checkSuccess(con.getNetworkTimeout(),0);
		System.out.println();

		// SQLRelayConnection
		if (issqlrelay) {
			System.out.println("SQLRelayConnection");
			SQLRelayConnection	sqlrcon=(SQLRelayConnection)con;
			checkSuccess(sqlrcon.getHost(),host);
			checkSuccess(sqlrcon.getPort(),port);
			checkSuccess(sqlrcon.getSocket(),socket);
			checkSuccess(sqlrcon.getUser(),user);
			checkSuccess(sqlrcon.getPassword(),password);
			System.out.println();

			// isWrapperFor, unwrap
			System.out.println("unwrap");
			checkSuccess(
				con.isWrapperFor(SQLRConnection.class),1);
			checkSuccess(
				(con.unwrap(SQLRConnection.class)!=null),1);
			System.out.println();
		}

		// setCatalog, getCatalog
		// FIXME: sqlrelay currently returns the schema, when run
		// against an oracle backend, rather than null, #7914
		if (!issqlrelay) {
			System.out.println("catalog");
			con.setCatalog(user);
			checkSuccess(con.getCatalog(),null);
			System.out.println();
		}

		// setSchema, getSchema
		// FIXME: with sqlrelay, somehow this causes oracle to throw:
		// ORA-01031: insufficient privileges
		if (!issqlrelay) {
			System.out.println("schema");
			con.setSchema(user.toUpperCase());
			checkSuccess(con.getSchema(),user.toUpperCase());
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
		checkSuccess(con.getClientInfo("OCSID.MODULE"),"value1");
		checkSuccess(con.getClientInfo("OCSID.ACTION"),"value2");
		checkSuccess(con.getClientInfo("OCSID.CLIENTID"),"value3");
		checkSuccess(con.getClientInfo("OCSID.ECID"),"value4");
		Properties	outprop=con.getClientInfo();
		checkSuccess(outprop.getProperty("OCSID.MODULE"),"value1");
		checkSuccess(outprop.getProperty("OCSID.ACTION"),"value2");
		checkSuccess(outprop.getProperty("OCSID.CLIENTID"),"value3");
		checkSuccess(outprop.getProperty("OCSID.ECID"),"value4");
		System.out.println();

		// setReadOnly, isReadOnly
		System.out.println("readonly");
		con.setReadOnly(true);
		checkSuccess(con.isReadOnly(),1);
		con.setReadOnly(false);
		checkSuccess(!con.isReadOnly(),1);
		System.out.println();

		// setAutoCommit, getAutoCommit
		System.out.println("autocommit");
		con.setAutoCommit(true);
		checkSuccess(con.getAutoCommit(),1);
		con.setAutoCommit(false);
		checkSuccess(!con.getAutoCommit(),1);
		System.out.println();

		// setHoldability, getHoldability
		System.out.println("holdability");
		con.setHoldability(ResultSet.HOLD_CURSORS_OVER_COMMIT);
		checkSuccess(con.getHoldability()==
				ResultSet.HOLD_CURSORS_OVER_COMMIT,1);
		try {
			con.setHoldability(ResultSet.CLOSE_CURSORS_AT_COMMIT);
			checkSuccess(false,1);
		} catch (Exception ex) {
			checkSuccess(true,1);
		}
		System.out.println();

		// setTransactionIsolation, getTransactionIsolation
		// FIXME: sqlrelay doesn't support this yet
		if (!issqlrelay) {
			System.out.println("isolation");
			try {
				con.setTransactionIsolation(
					Connection.
					TRANSACTION_READ_UNCOMMITTED);
				checkSuccess(false,1);
			} catch (Exception ex) {
				checkSuccess(true,1);
			}
			con.setTransactionIsolation(
				Connection.TRANSACTION_READ_COMMITTED);
			checkSuccess(con.getTransactionIsolation(),
				Connection.TRANSACTION_READ_COMMITTED);
			try {
				con.setTransactionIsolation(
					Connection.
					TRANSACTION_REPEATABLE_READ);
			} catch (Exception ex) {
				checkSuccess(true,1);
			}
			con.setTransactionIsolation(
				Connection.TRANSACTION_SERIALIZABLE);
			checkSuccess(con.getTransactionIsolation(),
				Connection.TRANSACTION_SERIALIZABLE);
			System.out.println();
		}

		// setTypeMap, getTypeMap

		// getWarnings, clearWarnings
		System.out.println("warnings");
		checkSuccess(con.getWarnings()==null,1);
		con.clearWarnings();
		System.out.println();

		System.out.println();



		// DatabaseMetaData
		System.out.println("DATABASE META DATA:");

		// getMetaData
		System.out.println("getMetaData");
		DatabaseMetaData	md=con.getMetaData();
		checkSuccess((md!=null),1);
		System.out.println();

		// getCatalogs
		System.out.println("catalogs");
		ResultSet	rs=md.getCatalogs();
		checkSuccess((rs!=null),1);
		ResultSetMetaData	rsmd=rs.getMetaData();
		checkSuccess((rsmd!=null),1);
		checkSuccess(rsmd.getColumnCount(),1);
		int	col=1;
		checkSuccess(rsmd.getColumnName(col++),"TABLE_CAT");
		while (rs.next()) {
			System.out.println(rs.getString("TABLE_CAT"));
		}
		rs.close();
		System.out.println();

		// getSchemas
		System.out.println("schemas");
		rs=md.getSchemas();
		checkSuccess((rs!=null),1);
		rsmd=rs.getMetaData();
		checkSuccess((rsmd!=null),1);
		checkSuccess(rsmd.getColumnCount(),2);
		col=1;
		checkSuccess(rsmd.getColumnName(col++),"TABLE_SCHEM");
		checkSuccess(rsmd.getColumnName(col++),"TABLE_CATALOG");
		//System.out.println();
		//printColumns(rsmd);
		//printResultSet(rs);
		rs.close();
		System.out.println();

		// getTableTypes
		System.out.println("table types");
		rs=md.getTableTypes();
		checkSuccess((rs!=null),1);
		rsmd=rs.getMetaData();
		checkSuccess((rsmd!=null),1);
		checkSuccess(rsmd.getColumnCount(),1);
		col=1;
		checkSuccess(rsmd.getColumnName(col++),"TABLE_TYPE");
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
		checkSuccess((rs!=null),1);
		rsmd=rs.getMetaData();
		checkSuccess((rsmd!=null),1);
		if (issqlrelay) {
                	checkSuccess(rsmd.getColumnCount(),10);
		} else {
			// oracle jdbc (at least v8) returns 5 columns
                	checkSuccess(rsmd.getColumnCount(),5);
		}
		col=1;
		checkSuccess(rsmd.getColumnName(col++),"TABLE_CAT");
		checkSuccess(rsmd.getColumnName(col++),"TABLE_SCHEM");
		checkSuccess(rsmd.getColumnName(col++),"TABLE_NAME");
		checkSuccess(rsmd.getColumnName(col++),"TABLE_TYPE");
		checkSuccess(rsmd.getColumnName(col++),"REMARKS");
		if (issqlrelay) {
			checkSuccess(rsmd.getColumnName(col++),"TYPE_CAT");
			checkSuccess(rsmd.getColumnName(col++),"TYPE_SCHEM");
			checkSuccess(rsmd.getColumnName(col++),"TYPE_NAME");
			checkSuccess(rsmd.getColumnName(col++),
						"SELF_REFERENCING_COL_NAME");
			checkSuccess(rsmd.getColumnName(col++),
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
		checkSuccess((rs!=null),1);
		rsmd=rs.getMetaData();
		checkSuccess((rsmd!=null),1);
		checkSuccess(rsmd.getColumnCount(),18);
		col=1;
		checkSuccess(rsmd.getColumnName(col++),"TYPE_NAME");
		checkSuccess(rsmd.getColumnName(col++),"DATA_TYPE");
		checkSuccess(rsmd.getColumnName(col++),"PRECISION");
		checkSuccess(rsmd.getColumnName(col++),"LITERAL_PREFIX");
		checkSuccess(rsmd.getColumnName(col++),"LITERAL_SUFFIX");
		checkSuccess(rsmd.getColumnName(col++),"CREATE_PARAMS");
		checkSuccess(rsmd.getColumnName(col++),"NULLABLE");
		checkSuccess(rsmd.getColumnName(col++),"CASE_SENSITIVE");
		checkSuccess(rsmd.getColumnName(col++),"SEARCHABLE");
		checkSuccess(rsmd.getColumnName(col++),"UNSIGNED_ATTRIBUTE");
		checkSuccess(rsmd.getColumnName(col++),"FIXED_PREC_SCALE");
		checkSuccess(rsmd.getColumnName(col++),"AUTO_INCREMENT");
		checkSuccess(rsmd.getColumnName(col++),"LOCAL_TYPE_NAME");
		checkSuccess(rsmd.getColumnName(col++),"MINIMUM_SCALE");
		checkSuccess(rsmd.getColumnName(col++),"MAXIMUM_SCALE");
		checkSuccess(rsmd.getColumnName(col++),"SQL_DATA_TYPE");
		checkSuccess(rsmd.getColumnName(col++),"SQL_DATETIME_SUB");
		checkSuccess(rsmd.getColumnName(col++),"NUM_PREC_RADIX");
		//System.out.println();
		//printColumns(rsmd);
		//printResultSet(rs);
		rs.close();
		System.out.println();

		// getProcedures
                System.out.println("procedures");
                rs=md.getProcedures("%","%","%");
                checkSuccess((rs!=null),1);
                rsmd=rs.getMetaData();
                checkSuccess((rsmd!=null),1);
		col=1;
		if (issqlrelay) {
                	checkSuccess(rsmd.getColumnCount(),8);
		} else {
			// oracle jdbc (at least v8) returns 9 columns
                	checkSuccess(rsmd.getColumnCount(),9);
		}
                checkSuccess(rsmd.getColumnName(col++),"PROCEDURE_CAT");
                checkSuccess(rsmd.getColumnName(col++),"PROCEDURE_SCHEM");
                checkSuccess(rsmd.getColumnName(col++),"PROCEDURE_NAME");
		if (issqlrelay) {
                	checkSuccess(rsmd.getColumnName(col++),
						"NUM_INPUT_PARAMS");
                	checkSuccess(rsmd.getColumnName(col++),
						"NUM_OUTPUT_PARAMS");
                	checkSuccess(rsmd.getColumnName(col++),
						"NUM_RESULT_SETS");
		} else {
			// oracle jdbc (at least v8) returns
			// NULL for these column names
                	checkSuccess(rsmd.getColumnName(col++),"NULL");
                	checkSuccess(rsmd.getColumnName(col++),"NULL");
                	checkSuccess(rsmd.getColumnName(col++),"NULL");
		}
                checkSuccess(rsmd.getColumnName(col++),"REMARKS");
                checkSuccess(rsmd.getColumnName(col++),"PROCEDURE_TYPE");
		if (!issqlrelay) {
			// oracle jdbc (at least v8) returns a 9th column
                	checkSuccess(rsmd.getColumnName(col++),"SPECIFIC_NAME");
		}
                //System.out.println();
		//printColumns(rsmd);
		//printResultSet(rs);
                rs.close();
                System.out.println();

		// getFunctions
		// FIXME: sqlrelay doesn't support this yet
		if (!issqlrelay) {
                	System.out.println("functions");
                	rs=md.getFunctions("%","%","%");
                	checkSuccess((rs!=null),1);
                	rsmd=rs.getMetaData();
                	checkSuccess((rsmd!=null),1);
			col=1;
			if (issqlrelay) {
				checkSuccess(rsmd.getColumnCount(),8);
			} else {
				// oracle jdbc (at least v8) returns 6 columns
				checkSuccess(rsmd.getColumnCount(),6);
			}
                	checkSuccess(rsmd.getColumnName(col++),
							"FUNCTION_CAT");
                	checkSuccess(rsmd.getColumnName(col++),
							"FUNCTION_SCHEM");
                	checkSuccess(rsmd.getColumnName(col++),
							"FUNCTION_NAME");
			// oracle jdbc (at least v8) doesn't
			// return these columns at all
			if (issqlrelay) {
                		checkSuccess(rsmd.getColumnName(col++),
							"NUM_INPUT_PARAMS");
                		checkSuccess(rsmd.getColumnName(col++),
							"NUM_OUTPUT_PARAMS");
                		checkSuccess(rsmd.getColumnName(col++),
							"NUM_RESULT_SETS");
			}
                	checkSuccess(rsmd.getColumnName(col++),"REMARKS");
                	checkSuccess(rsmd.getColumnName(col++),"FUNCTION_TYPE");
			// oracle jdbc (at least v8) returns this extra column
			if (!issqlrelay) {
                		checkSuccess(rsmd.getColumnName(col++),
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
                	checkSuccess((rs!=null),1);
                	rsmd=rs.getMetaData();
                	checkSuccess((rsmd!=null),1);
			col=1;
			if (issqlrelay) {
                		checkSuccess(rsmd.getColumnCount(),7);
			} else {
				// oracle jdbc (at least v8) returns 6 columns
                		checkSuccess(rsmd.getColumnCount(),6);
			}
                	checkSuccess(rsmd.getColumnName(col++),"TYPE_CAT");
                	checkSuccess(rsmd.getColumnName(col++),"TYPE_SCHEM");
                	checkSuccess(rsmd.getColumnName(col++),"TYPE_NAME");
                	checkSuccess(rsmd.getColumnName(col++),"CLASS_NAME");
                	checkSuccess(rsmd.getColumnName(col++),"DATA_TYPE");
                	checkSuccess(rsmd.getColumnName(col++),"REMARKS");
			if (issqlrelay) {
				// oracle jdbc (at least v8) doesn't
				// return this column at all
                		checkSuccess(rsmd.getColumnName(col++),
								"BASE_TYPE");
			}
                	//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
                	rs.close();
                	System.out.println();
		}



		// Statement
		System.out.println("STATEMENT:");

		// createStatement
		System.out.println("create statement");
		Statement	stmt=con.createStatement();
		checkSuccess((stmt!=null),1);
		stmt.close();
                System.out.println();

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
					checkSuccess((stmt!=null),1);
					stmt.close();
				} else {
					try {
						stmt=con.
						createStatement(
							rstype[r],
							concurrency[c]);
						checkSuccess(false,1);
					} catch (Exception ex) {
						checkSuccess(true,1);
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
						checkSuccess((stmt!=null),1);
						stmt.close();
					} else {
						try {
							stmt=con.
							createStatement(
								rstype[r],
								concurrency[c],
								holdability[h]);
							checkSuccess(false,1);
						} catch (Exception ex) {
							checkSuccess(true,1);
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
		stmt.close();

		System.out.println("CREATE TEMPTABLE:");
		stmt=con.createStatement();
		checkSuccess(stmt.executeUpdate("create table testtable (testnumber number, testchar char(40), testvarchar varchar2(40), testdate date, testlong long, testclob clob, testblob blob)"),0);
		System.out.println();

		System.out.println("INSERT:");
		checkSuccess(stmt.executeUpdate("insert into testtable values (1,'testchar1','testvarchar1','01-JAN-2001','testlong1','testclob1',empty_blob())"),1);
		stmt.close();
		System.out.println();

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
		checkSuccess(pstmt.executeUpdate(),1);
		pstmt.clearParameters();
		pstmt.setInt(1,3);
		pstmt.setString(2,"testchar3");
		pstmt.setString(3,"testvarchar3");
		pstmt.setDate(4,new java.sql.Date(2003,1,1));
		pstmt.setString(5,"testlong3");
		pstmt.setString(6,"testclob3");
		pstmt.setBytes(7,(new String("testblob3")).
				getBytes(StandardCharsets.UTF_8));
		checkSuccess(pstmt.executeUpdate(),1);
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
		checkSuccess(pstmt.executeUpdate(),1);
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
		checkSuccess(pstmt.executeUpdate(),1);
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
		checkSuccess(pstmt.executeUpdate(),1);
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
		checkSuccess(pstmt.executeUpdate(),1);
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
		checkSuccess(pstmt.executeUpdate(),1);
		pstmt.close();
		System.out.println();

		// FIXME: output binds

		System.out.println("SELECT:");
		stmt=con.createStatement();
		rs=stmt.executeQuery("select * from testtable order by testnumber");
		checkSuccess((rs!=null),1);
		rsmd=rs.getMetaData();
		checkSuccess((rsmd!=null),1);
		System.out.println();

		System.out.println("COLUMN COUNT:");
		checkSuccess(rsmd.getColumnCount(),7);
		System.out.println();

		System.out.println("COLUMN NAMES:");
		checkSuccess(rsmd.getColumnName(1),"TESTNUMBER");
		checkSuccess(rsmd.getColumnName(2),"TESTCHAR");
		checkSuccess(rsmd.getColumnName(3),"TESTVARCHAR");
		checkSuccess(rsmd.getColumnName(4),"TESTDATE");
		checkSuccess(rsmd.getColumnName(5),"TESTLONG");
		checkSuccess(rsmd.getColumnName(6),"TESTCLOB");
		checkSuccess(rsmd.getColumnName(7),"TESTBLOB");
		System.out.println();

		System.out.println("COLUMN TYPES:");
		checkSuccess(rsmd.getColumnTypeName(1),"NUMBER");
		checkSuccess(rsmd.getColumnTypeName(2),"CHAR");
		checkSuccess(rsmd.getColumnTypeName(3),"VARCHAR2");
		checkSuccess(rsmd.getColumnTypeName(4),"DATE");
		checkSuccess(rsmd.getColumnTypeName(5),"LONG");
		checkSuccess(rsmd.getColumnTypeName(6),"CLOB");
		checkSuccess(rsmd.getColumnTypeName(7),"BLOB");
		System.out.println();

		System.out.println("COLUMN LENGTH:");
		checkSuccess(rsmd.getPrecision(1),22);
		checkSuccess(rsmd.getPrecision(2),40);
		checkSuccess(rsmd.getPrecision(3),40);
		checkSuccess(rsmd.getPrecision(4),7);
		checkSuccess(rsmd.getPrecision(5),0);
		checkSuccess(rsmd.getPrecision(6),0);
		checkSuccess(rsmd.getPrecision(7),0);
		System.out.println();

		System.out.println("LONGEST COLUMN:");
		checkSuccess(rsmd.getColumnDisplaySize(1),1);
		checkSuccess(rsmd.getColumnDisplaySize(2),40);
		checkSuccess(rsmd.getColumnDisplaySize(3),12);
		checkSuccess(rsmd.getColumnDisplaySize(4),9);
		checkSuccess(rsmd.getColumnDisplaySize(5),9);
		checkSuccess(rsmd.getColumnDisplaySize(6),9);
		checkSuccess(rsmd.getColumnDisplaySize(7),9);
		System.out.println();

		// drop existing table
		//stmt=con.createStatement();
		//stmt.executeUpdate("drop table testtable");

		stmt.close();
		con.close();

		System.exit(0);
	}
}
