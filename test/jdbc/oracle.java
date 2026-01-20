// Copyright (c) David Muse
// See the file COPYING for more information.

import java.sql.*;
import com.firstworks.sqlrelay.*;
import com.firstworks.sql.*;
import java.util.Properties;
import java.util.concurrent.Executors;
import java.util.concurrent.Executor;

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
			System.out.println("failure ");
			System.exit(1);
		}
	}
	
	private static void checkSuccess(double value, double success) {
	
		if (value==success) {
			System.out.printf("success ");
		} else {
			System.out.println("failure ");
			System.exit(1);
		}
	}
	
	private static void checkSuccess(boolean value, int success) {
	
		if (((value)?1:0)==success) {
			System.out.printf("success ");
		} else {
			System.out.println("failure ");
			System.exit(1);
		}
	}

	public static void main(String args[]) throws Exception {

		String	driver="com.firstworks.sql.SQLRelayDriver";
		//String	host="localhost";
		String	host="fedora40x64";
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

		// Connection
		System.out.println("CONNECTION...");
		Class.forName(driver);
		Connection	con=DriverManager.getConnection(
						url,user,password);

		// close, isClosed, isValid
		System.out.println("CONNECTION - close");
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
		if (driver.equals("com.firstworks.sql.SQLRelayDriver")) {
			System.out.println("CONNECTION - SQLRelayConnection");
			SQLRelayConnection	sqlrcon=(SQLRelayConnection)con;
			checkSuccess(sqlrcon.getHost(),host);
			checkSuccess(sqlrcon.getPort(),port);
			checkSuccess(sqlrcon.getSocket(),socket);
			checkSuccess(sqlrcon.getUser(),user);
			checkSuccess(sqlrcon.getPassword(),password);
			System.out.println();

			// isWrapperFor, unwrap
			System.out.println("CONNECTION - unwrap");
			checkSuccess(
				con.isWrapperFor(SQLRConnection.class),1);
			checkSuccess(
				(con.unwrap(SQLRConnection.class)!=null),1);
			System.out.println();
		}

		// setCatalog, getCatalog
		System.out.println("CONNECTION - catalog");
		con.setCatalog(user);
		checkSuccess(con.getCatalog(),null);
		System.out.println();

		// setSchema, getSchema
		System.out.println("CONNECTION - schema");
		con.setSchema(user.toUpperCase());
		checkSuccess(con.getSchema(),user.toUpperCase());
		System.out.println();

		// setClientInfo, getClientInfo()
		// Oracle only allows:
		// OCSID.MODULE
		// OCSID.ACTION
		// OCSID.CLIENTID
		// OCSID.ECID (execution context id)
		// OCSID.SEQUENCE_NUMBER
		// OCSID.DBOP (database operation)
		System.out.println("CONNECTION - client info");
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
		System.out.println("CONNECTION - readonly");
		con.setReadOnly(true);
		checkSuccess(con.isReadOnly(),1);
		con.setReadOnly(false);
		checkSuccess(!con.isReadOnly(),1);
		System.out.println();

		// setAutoCommit, getAutoCommit
		System.out.println("CONNECTION - autocommit");
		con.setAutoCommit(true);
		checkSuccess(con.getAutoCommit(),1);
		con.setAutoCommit(false);
		checkSuccess(!con.getAutoCommit(),1);
		System.out.println();

		// setHoldability, getHoldability
		System.out.println("CONNECTION - holdability");
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
		// oracle only supports READ_COMMITTED and SERIALIZABLE,
		// the other should throw an exception
		System.out.println("CONNECTION - isolation");
		try {
			con.setTransactionIsolation(
				Connection.TRANSACTION_READ_UNCOMMITTED);
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
				Connection.TRANSACTION_REPEATABLE_READ);
		} catch (Exception ex) {
			checkSuccess(true,1);
		}
		con.setTransactionIsolation(
			Connection.TRANSACTION_SERIALIZABLE);
		checkSuccess(con.getTransactionIsolation(),
			Connection.TRANSACTION_SERIALIZABLE);
		System.out.println();

		// setTypeMap, getTypeMap

		// getWarnings, clearWarnings
		System.out.println("CONNECTION - warnings");
		checkSuccess(con.getWarnings()==null,1);
		con.clearWarnings();
		System.out.println();

		// getMetaData
		System.out.println("DATABASE META DATA...");
		DatabaseMetaData	md=con.getMetaData();
		checkSuccess((md!=null),1);
		System.out.println();

		// getCatalogs
		System.out.println("DATABASE META DATA - catalogs");
		ResultSet	rs=md.getCatalogs();
		checkSuccess((rs!=null),1);
		ResultSetMetaData	rsmd=rs.getMetaData();
		checkSuccess((rsmd!=null),1);
		checkSuccess(rsmd.getColumnCount(),1);
		checkSuccess(rsmd.getColumnName(1),"TABLE_CAT");
		while (rs.next()) {
			System.out.println(rs.getString("TABLE_CAT"));
		}
		rs.close();
		System.out.println();

		// getSchemas
		System.out.println("DATABASE META DATA - schemas");
		rs=md.getSchemas();
		checkSuccess((rs!=null),1);
		rsmd=rs.getMetaData();
		checkSuccess((rsmd!=null),1);
		checkSuccess(rsmd.getColumnCount(),2);
		checkSuccess(rsmd.getColumnName(1),"TABLE_SCHEM");
		checkSuccess(rsmd.getColumnName(2),"TABLE_CATALOG");
		System.out.println();
		while (rs.next()) {
			System.out.println(rs.getString("TABLE_SCHEM")+","+
						rs.getString("TABLE_CATALOG"));
		}
		rs.close();
		System.out.println();

		// getTableTypes
		System.out.println("DATABASE META DATA - table types");
		rs=md.getTableTypes();
		checkSuccess((rs!=null),1);
		rsmd=rs.getMetaData();
		checkSuccess((rsmd!=null),1);
		checkSuccess(rsmd.getColumnCount(),1);
		checkSuccess(rsmd.getColumnName(1),"TABLE_TYPE");
		System.out.println();
		while (rs.next()) {
			System.out.println(rs.getString("TABLE_TYPE"));
		}
		rs.close();
		System.out.println();

		// getTables
		System.out.println("DATABASE META DATA - tables");
		rs=md.getTables("%","FEDORA40X64","A%TO%",
			new String[] {"SYNONYM","TABLE","VIEW"});
		checkSuccess((rs!=null),1);
		rsmd=rs.getMetaData();
		checkSuccess((rsmd!=null),1);
		checkSuccess(rsmd.getColumnCount(),5);
		checkSuccess(rsmd.getColumnName(1),"TABLE_CAT");
		checkSuccess(rsmd.getColumnName(2),"TABLE_SCHEM");
		checkSuccess(rsmd.getColumnName(3),"TABLE_NAME");
		checkSuccess(rsmd.getColumnName(4),"TABLE_TYPE");
		checkSuccess(rsmd.getColumnName(5),"REMARKS");
		System.out.println();
		while (rs.next()) {
			System.out.println(rs.getString("TABLE_CAT")+","+
						rs.getString("TABLE_SCHEM")+","+
						rs.getString("TABLE_NAME")+","+
						rs.getString("TABLE_TYPE")+","+
						rs.getString("REMARKS"));
		}
		rs.close();
		System.out.println();



		// nativeSql

		// createBlob
		// createClob
		// createNClob

		// createArrayOf - unsupported
		// createSQLXML - unsupported
		// createStruct - unsupported

		// setSavepoint...
		// releaseSavepoint

		// commit
		// rollback...

/*
		// createStatement
		System.out.println("CONNECTION - create statement");
		Statement	stmt=con.createStatement();
		checkSuccess((stmt!=null),1);
		stmt.close();
		stmt=con.createStatement(
				ResultSet.TYPE_FORWARD_ONLY,
				ResultSet.CONCUR_READ_ONLY,
				ResultSet.HOLD_CURSORS_OVER_COMMIT);
		checkSuccess((stmt!=null),1);
		stmt.close();
		stmt=con.createStatement(
				ResultSet.TYPE_SCROLL_INSENSITIVE,
				ResultSet.CONCUR_READ_ONLY,
				ResultSet.HOLD_CURSORS_OVER_COMMIT);
		checkSuccess((stmt!=null),1);
		stmt.close();
		try {
			stmt=con.createStatement(
				ResultSet.TYPE_SCROLL_SENSITIVE,
				ResultSet.CONCUR_UPDATABLE,
				ResultSet.HOLD_CURSORS_OVER_COMMIT);
			checkSuccess(false,1);
		} catch (Exception ex) {
			checkSuccess(true,1);
		}
		try {
			stmt=con.createStatement(
				ResultSet.TYPE_FORWARD_ONLY,
				ResultSet.CONCUR_UPDATABLE,
				ResultSet.HOLD_CURSORS_OVER_COMMIT);
			checkSuccess(false,1);
		} catch (Exception ex) {
			checkSuccess(true,1);
		}
		try {
			stmt=con.createStatement(
				ResultSet.TYPE_FORWARD_ONLY,
				ResultSet.CONCUR_READ_ONLY,
				ResultSet.CLOSE_CURSORS_AT_COMMIT);
			checkSuccess(false,1);
		} catch (Exception ex) {
			checkSuccess(true,1);
		}
		System.out.println();

		// prepareCall...

		// prepareStatement...
		System.out.println("\n");


		// Statement
		System.out.println("STATEMENT...");
		stmt=con.createStatement();
		System.out.println("\n");


		// ResultSet
		System.out.println("RESULTSET...");
		rs=stmt.executeQuery("select 1 from dual");
		checkSuccess((rs!=null),1);
		rs.next();
		checkSuccess(rs.getInt(1),1);
		rs.close();
		System.out.println("\n");


		stmt.close();
*/
		con.close();

		System.exit(0);
	}
}
