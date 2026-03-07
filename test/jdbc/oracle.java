// Copyright (c) David Muse
// See the file COPYING for more information.

import java.sql.*;
import com.firstworks.sqlrelay.*;
import com.firstworks.sql.*;
import java.util.Calendar;
import java.util.Properties;
import java.util.concurrent.Executors;
import java.util.concurrent.Executor;
import java.nio.charset.StandardCharsets;
import java.io.InputStream;
import java.io.StringWriter;
import java.net.InetAddress;
import java.net.URL;

class oracle extends sqlrtest {

	public static void main(String args[]) throws Exception {

		// This test supports both the sqlrelay jdbc driver and the
		// databse native jdbc driver.  It detects which to use based
		// on what is included in the classpath, and builds the
		// appropriate url and credentials for each.
		String	classpath=System.getProperty("java.class.path");
		String	hostname=InetAddress.getLocalHost().
					getHostName().split("\\.")[0].
					toUpperCase();
		String	driver=null;
		String	host=null;
		short	port=0;
		String	socket=null;
		String	user=null;
		String	password=null;
		String	url=null;
		boolean	issqlrelay=false;
		if (classpath.contains("sqlrelayjdbc.jar")) {
			driver="com.firstworks.sql.SQLRelayDriver";
			host="localhost";
			port=9000;
			socket=null;
			url="jdbc:sqlrelay://"+host+":"+port;
			user="testuser";
			password="testpassword";
			issqlrelay=true;
		} else if (classpath.contains("ojdbc")) {
			driver="oracle.jdbc.driver.OracleDriver";
			url="jdbc:oracle:thin:@oracle:1521:ora1";
			user=hostname;
			password="testpassword";
		}

		Connection		con;
		DatabaseMetaData	md;
		boolean			boolval;
		String			stringval;
		int			intval;
		ResultSet		rs;
		ResultSetMetaData	rsmd;
		int			col;
		Statement		stmt;
		PreparedStatement	pstmt;
		Clob			clob;
		Blob			blob;
		CallableStatement	cstmt;
		boolean			found;
		int			counter;
		java.sql.Date		datevar;
		Timestamp		tsvar;
		Calendar		cal=Calendar.getInstance();


		// connection
		System.out.println("CONNECTION:");

		// getConnection
		System.out.println("getConnection");
		Class.forName(driver);
		con=DriverManager.getConnection(url,user,password);
		assertTrue((con!=null));
		System.out.println();

		// close
		System.out.println("close");
		assertFalse(con.isClosed());
		assertTrue(con.isValid(0));
		con.close();
		assertTrue(con.isClosed());
		assertFalse(con.isValid(0));
		con=DriverManager.getConnection(url,user,password);
		assertTrue((con!=null));
		System.out.println();

		// network timeout
		System.out.println("network timeout");
		Executor	executor=Executors.newSingleThreadExecutor();
		con.setNetworkTimeout(executor,1);
		assertEquals(con.getNetworkTimeout(),1);
		con.setNetworkTimeout(executor,2);
		assertEquals(con.getNetworkTimeout(),2);
		con.setNetworkTimeout(executor,0);
		assertEquals(con.getNetworkTimeout(),0);
		System.out.println();

		if (issqlrelay) {
			// connection
			System.out.println("connection");
			SQLRelayConnection	sqlrcon=(SQLRelayConnection)con;
			assertEquals(sqlrcon.getHost(),host);
			assertEquals(sqlrcon.getPort(),port);
			assertEquals(sqlrcon.getSocket(),socket);
			assertEquals(sqlrcon.getUser(),user);
			assertEquals(sqlrcon.getPassword(),password);
			System.out.println();

			// unwrap
			System.out.println("unwrap");
			assertEquals(
				con.isWrapperFor(SQLRConnection.class),1);
			assertEquals(
				(con.unwrap(SQLRConnection.class)!=null),1);
			System.out.println();
		}

		// catalog
		System.out.println("catalog");
		String	originalcatalog=con.getCatalog();
		con.setCatalog("dummy");
		assertEquals(con.getCatalog(),null);
		con.setCatalog(originalcatalog);
		System.out.println();

		// schema
		System.out.println("schema");
		String	originalschema=con.getSchema();
		con.setSchema(user.toUpperCase());
		assertEquals(con.getSchema(),user.toUpperCase());
		con.setSchema(originalschema);
		System.out.println();

		// client info
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

		// readonly
		System.out.println("readonly");
		con.setReadOnly(true);
		assertTrue(con.isReadOnly());
		con.setReadOnly(false);
		assertTrue(!con.isReadOnly());
		System.out.println();

		// autocommit
		System.out.println("autocommit");
		con.setAutoCommit(true);
		assertTrue(con.getAutoCommit());
		con.setAutoCommit(false);
		assertTrue(!con.getAutoCommit());
		System.out.println();

		// holdability
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

		// isolation levels
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

		// warnings
		System.out.println("warnings");
		assertTrue(con.getWarnings()==null);
		con.clearWarnings();
		System.out.println();


		// database meta data
		System.out.println("DATABASE META DATA:");

		// getMetaData
		System.out.println("getMetaData");
		md=con.getMetaData();
		assertTrue((md!=null));
		System.out.println();

		// getConnection
		System.out.println("getConnection");
		assertEquals(md.getConnection(),con);
		System.out.println();

		// unwrap
		if (issqlrelay) {
			System.out.println("unwrap");
			assertEquals(md.isWrapperFor(SQLRConnection.class),1);
			System.out.println();
			assertEquals((md.unwrap(SQLRConnection.class)!=null),1);
			System.out.println();
		}

		// allProceduresAreCallable
		System.out.println("allProceduresAreCallable");
		boolval=md.allProceduresAreCallable();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// allTablesAreSelectable
		System.out.println("allTablesAreSelectable");
		boolval=md.allTablesAreSelectable();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// autoCommitFailureClosesAllResultSets
		System.out.println("autoCommitFailureClosesAllResultSets");
		boolval=md.autoCommitFailureClosesAllResultSets();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// dataDefinitionCausesTransactionCommit
		System.out.println("dataDefinitionCausesTransactionCommit");
		boolval=md.dataDefinitionCausesTransactionCommit();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// dataDefinitionIgnoredInTransactions
		System.out.println("dataDefinitionIgnoredInTransactions");
		boolval=md.dataDefinitionIgnoredInTransactions();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// deletesAreDetected
		System.out.println("deletesAreDetected "+
					"(forward only)");
		boolval=md.deletesAreDetected(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("deletesAreDetected "+
					"(scroll insensitive)");
		boolval=md.deletesAreDetected(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("deletesAreDetected "+
					"(scroll sensitive)");
		boolval=md.deletesAreDetected(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("  "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertFalse(boolval);
		}
		System.out.println();

		// doesMaxRowSizeIncludeBlobs
		System.out.println("doesMaxRowSizeIncludeBlobs");
		boolval=md.doesMaxRowSizeIncludeBlobs();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// generatedKeyAlwaysReturned
		System.out.println("generatedKeyAlwaysReturned");
		boolval=md.generatedKeyAlwaysReturned();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// getCatalogSeparator
		System.out.println("getCatalogSeparator");
		stringval=md.getCatalogSeparator();
		System.out.println("  "+stringval);
		assertEquals(stringval,"");
		System.out.println();

		// getCatalogTerm
		System.out.println("getCatalogTerm");
		stringval=md.getCatalogTerm();
		System.out.println("  "+stringval);
		assertEquals(stringval,"");
		System.out.println();

		// getDatabaseMajorVersion
		System.out.println("getDatabaseMajorVersion");
		intval=md.getDatabaseMajorVersion();
		System.out.println("  "+intval);
		assertEquals(intval,12);
		System.out.println();

		// getDatabaseMinorVersion
		System.out.println("getDatabaseMinorVersion");
		intval=md.getDatabaseMinorVersion();
		System.out.println("  "+intval);
		assertEquals(intval,2);
		System.out.println();

		// getDatabaseProductName
		System.out.println("getDatabaseProductName");
		stringval=md.getDatabaseProductName();
		System.out.println("  "+stringval);
		if (issqlrelay) {
			assertEquals(stringval,"oracle");
		} else {
			assertEquals(stringval,"Oracle");
		}
		System.out.println();

		// getDatabaseProductVersion
		System.out.println("getDatabaseProductVersion");
		stringval=md.getDatabaseProductVersion();
		System.out.println("  "+stringval);
		assertEquals(stringval,"Oracle Database 12c Enterprise Edition Release 12.2.0.1.0 - 64bit Production");
		System.out.println();

		// getDefaultTransactionIsolation
		System.out.println("getDefaultTransactionIsolation");
		intval=md.getDefaultTransactionIsolation();
		System.out.println("  "+intval);
		assertEquals(intval,2);
		System.out.println();

		// getDriverMajorVersion
		System.out.println("getDriverMajorVersion");
		intval=md.getDriverMajorVersion();
		System.out.println("  "+intval);
		if (issqlrelay) {
			assertEquals(intval,2);
		} else {
			assertEquals(intval,23);
		}
		System.out.println();

		// getDriverMinorVersion
		System.out.println("getDriverMinorVersion");
		intval=md.getDriverMinorVersion();
		System.out.println("  "+intval);
		if (issqlrelay) {
			assertEquals(intval,1);
		} else {
			assertEquals(intval,26);
		}
		System.out.println();

		// getDriverName
		System.out.println("getDriverName");
		stringval=md.getDriverName();
		System.out.println("  "+stringval);
		if (issqlrelay) {
			assertEquals(stringval,"SQL Relay JDBC driver");
		} else {
			assertEquals(stringval,"Oracle JDBC driver");
		}
		System.out.println();

		// getDriverVersion
		System.out.println("getDriverVersion");
		stringval=md.getDriverVersion();
		System.out.println("  "+stringval);
		// not null and only contains numbers and dots
		assertTrue(stringval!=null && stringval.matches("[0-9.]+$"));
		System.out.println();

		// getExtraNameCharacters
		System.out.println("getExtraNameCharacters");
		stringval=md.getExtraNameCharacters();
		System.out.println("  "+stringval);
		assertEquals(stringval,"$#");
		System.out.println();

		// getIdentifierQuoteString
		System.out.println("getIdentifierQuoteString");
		stringval=md.getIdentifierQuoteString();
		System.out.println("  "+stringval);
		assertEquals(stringval,"\"");
		System.out.println();

		// getJDBCMajorVersion
		System.out.println("getJDBCMajorVersion");
		intval=md.getJDBCMajorVersion();
		System.out.println("  "+intval);
		assertTrue(intval>=1);
		System.out.println();

		// getJDBCMinorVersion
		System.out.println("getJDBCMinorVersion");
		intval=md.getJDBCMinorVersion();
		System.out.println("  "+intval);
		assertTrue(intval>=0);
		System.out.println();

		// getMaxBinaryLiteralLength
		System.out.println("getMaxBinaryLiteralLength");
		intval=md.getMaxBinaryLiteralLength();
		System.out.println("  "+intval);
		assertEquals(intval,1000);
		System.out.println();

		// getMaxCatalogNameLength
		System.out.println("getMaxCatalogNameLength");
		intval=md.getMaxCatalogNameLength();
		System.out.println("  "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxCharLiteralLength
		System.out.println("getMaxCharLiteralLength");
		intval=md.getMaxCharLiteralLength();
		System.out.println("  "+intval);
		assertEquals(intval,2000);
		System.out.println();

		// getMaxColumnNameLength
		System.out.println("getMaxColumnNameLength");
		intval=md.getMaxColumnNameLength();
		System.out.println("  "+intval);
		assertEquals(intval,128);
		System.out.println();

		// getMaxColumnsInGroupBy
		System.out.println("getMaxColumnsInGroupBy");
		intval=md.getMaxColumnsInGroupBy();
		System.out.println("  "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxColumnsInIndex
		System.out.println("getMaxColumnsInIndex");
		intval=md.getMaxColumnsInIndex();
		System.out.println("  "+intval);
		assertEquals(intval,32);
		System.out.println();

		// getMaxColumnsInOrderBy
		System.out.println("getMaxColumnsInOrderBy");
		intval=md.getMaxColumnsInOrderBy();
		System.out.println("  "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxColumnsInSelect
		System.out.println("getMaxColumnsInSelect");
		intval=md.getMaxColumnsInSelect();
		System.out.println("  "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxColumnsInTable
		System.out.println("getMaxColumnsInTable");
		intval=md.getMaxColumnsInTable();
		System.out.println("  "+intval);
		assertEquals(intval,1000);
		System.out.println();

		// getMaxConnections
		System.out.println("getMaxConnections");
		intval=md.getMaxConnections();
		System.out.println("  "+intval);
		if (issqlrelay) {
			assertTrue(intval>0);
		} else {
			// oracle jdbc returns 0 for this
			assertEquals(intval,0);
		}
		System.out.println();

		// getMaxCursorNameLength
		System.out.println("getMaxCursorNameLength");
		intval=md.getMaxCursorNameLength();
		System.out.println("  "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxIndexLength
		System.out.println("getMaxIndexLength");
		intval=md.getMaxIndexLength();
		System.out.println("  "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxProcedureNameLength
		System.out.println("getMaxProcedureNameLength");
		intval=md.getMaxProcedureNameLength();
		System.out.println("  "+intval);
		assertEquals(intval,128);
		System.out.println();

		// getMaxRowSize
		System.out.println("getMaxRowSize");
		intval=md.getMaxRowSize();
		System.out.println("  "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxSchemaNameLength
		System.out.println("getMaxSchemaNameLength");
		intval=md.getMaxSchemaNameLength();
		System.out.println("  "+intval);
		assertEquals(intval,128);
		System.out.println();

		// getMaxStatementLength
		System.out.println("getMaxStatementLength");
		intval=md.getMaxStatementLength();
		System.out.println("  "+intval);
		assertEquals(intval,65535);
		System.out.println();

		// getMaxStatements
		System.out.println("getMaxStatements");
		intval=md.getMaxStatements();
		System.out.println("  "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxTableNameLength
		System.out.println("getMaxTableNameLength");
		intval=md.getMaxTableNameLength();
		System.out.println("  "+intval);
		assertEquals(intval,128);
		System.out.println();

		// getMaxTablesInSelect
		System.out.println("getMaxTablesInSelect");
		intval=md.getMaxTablesInSelect();
		System.out.println("  "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxUserNameLength
		System.out.println("getMaxUserNameLength");
		intval=md.getMaxUserNameLength();
		System.out.println("  "+intval);
		assertEquals(intval,128);
		System.out.println();

		// getNumericFunctions
		System.out.println("getNumericFunctions");
		stringval=md.getNumericFunctions();
		System.out.println("  "+stringval);
		assertEquals(stringval,"ABS,ACOS,ASIN,ATAN,ATAN2,CEILING,COS,EXP,FLOOR,LOG,LOG10,MOD,PI,POWER,ROUND,SIGN,SIN,SQRT,TAN,TRUNCATE");
		System.out.println();

		// getProcedureTerm
		System.out.println("getProcedureTerm");
		stringval=md.getProcedureTerm();
		System.out.println("  "+stringval);
		assertEquals(stringval,"procedure");
		System.out.println();

		// getResultSetHoldability
		System.out.println("getResultSetHoldability");
		intval=md.getResultSetHoldability();
		System.out.println("  "+intval);
		assertEquals(intval,ResultSet.HOLD_CURSORS_OVER_COMMIT);
		System.out.println();

		// getRowIdLifetime
		System.out.println("getRowIdLifetime");
		RowIdLifetime	rowidlifetimeval=md.getRowIdLifetime();
		System.out.println("  "+rowidlifetimeval);
		assertEquals(rowidlifetimeval.name(),"ROWID_VALID_FOREVER");
		System.out.println();

		// getSchemaTerm
		System.out.println("getSchemaTerm");
		stringval=md.getSchemaTerm();
		System.out.println("  "+stringval);
		assertEquals(stringval,"schema");
		System.out.println();

		// getSearchStringEscape
		System.out.println("getSearchStringEscape");
		stringval=md.getSearchStringEscape();
		System.out.println("  "+stringval);
		assertEquals(stringval,"/");
		System.out.println();

		// getSQLKeywords
		System.out.println("getSQLKeywords");
		stringval=md.getSQLKeywords();
		System.out.println("  "+stringval);
		assertEquals(stringval,"ACCESS, ADD, ALTER, AUDIT, CLUSTER, COLUMN, COMMENT, COMPRESS, CONNECT, DATE, DROP, EXCLUSIVE, FILE, IDENTIFIED, IMMEDIATE, INCREMENT, INDEX, INITIAL, INTERSECT, LEVEL, LOCK, LONG, MAXEXTENTS, MINUS, MODE, NOAUDIT, NOCOMPRESS, NOWAIT, NUMBER, OFFLINE, ONLINE, PCTFREE, PRIOR, all_PL_SQL_reserved_ words");
		System.out.println();

		// getSQLStateType
		System.out.println("getSQLStateType");
		intval=md.getSQLStateType();
		System.out.println("  "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getStringFunctions
		System.out.println("getStringFunctions");
		stringval=md.getStringFunctions();
		System.out.println("  "+stringval);
		assertEquals(stringval,"ASCII,CHAR,CHAR_LENGTH,CHARACTER_LENGTH,CONCAT,LCASE,LENGTH,LTRIM,OCTET_LENGTH,REPLACE,RTRIM,SOUNDEX,SUBSTRING,UCASE");
		System.out.println();

		// getSystemFunctions
		System.out.println("getSystemFunctions");
		stringval=md.getSystemFunctions();
		System.out.println("  "+stringval);
		assertEquals(stringval,"USER");
		System.out.println();

		// getTimeDateFunctions
		System.out.println("getTimeDateFunctions");
		stringval=md.getTimeDateFunctions();
		System.out.println("  "+stringval);
		assertEquals(stringval,"CURRENT_DATE,CURRENT_TIMESTAMP,CURDATE,EXTRACT,HOUR,MINUTE,MONTH,SECOND,YEAR");
		System.out.println();

		// getURL
		System.out.println("getURL");
		stringval=md.getURL();
		System.out.println("  "+stringval);
		assertEquals(stringval,url);
		System.out.println();

		// getUserName
		System.out.println("getUserName");
		stringval=md.getUserName();
		System.out.println("  "+stringval);
		assertEquals(stringval,user);
		System.out.println();

		// insertsAreDetected
		System.out.println("insertsAreDetected "+
					"(forward only)");
		boolval=md.insertsAreDetected(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("insertsAreDetected "+
					"(scroll insensitive)");
		boolval=md.insertsAreDetected(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("insertsAreDetected "+
					"(scroll sensitive)");
		boolval=md.insertsAreDetected(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("  "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertFalse(boolval);
		}
		System.out.println();

		// isCatalogAtStart
		System.out.println("isCatalogAtStart");
		boolval=md.isCatalogAtStart();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// isReadOnly
		System.out.println("isReadOnly");
		boolval=md.isReadOnly();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// locatorsUpdateCopy
		System.out.println("locatorsUpdateCopy");
		boolval=md.locatorsUpdateCopy();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// nullPlusNonNullIsNull
		System.out.println("nullPlusNonNullIsNull");
		boolval=md.nullPlusNonNullIsNull();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// nullsAreSortedAtEnd
		System.out.println("nullsAreSortedAtEnd");
		boolval=md.nullsAreSortedAtEnd();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// nullsAreSortedAtStart
		System.out.println("nullsAreSortedAtStart");
		boolval=md.nullsAreSortedAtStart();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// nullsAreSortedHigh
		System.out.println("nullsAreSortedHigh");
		boolval=md.nullsAreSortedHigh();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// nullsAreSortedLow
		System.out.println("nullsAreSortedLow");
		boolval=md.nullsAreSortedLow();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// othersDeletesAreVisible
		System.out.println("othersDeletesAreVisible "+
					"(forward only)");
		boolval=md.othersDeletesAreVisible(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("othersDeletesAreVisible "+
					"(scroll insensitive)");
		boolval=md.othersDeletesAreVisible(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("othersDeletesAreVisible "+
					"(scroll sensitive)");
		boolval=md.othersDeletesAreVisible(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("  "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertFalse(boolval);
		}
		System.out.println();

		// othersInsertsAreVisible
		System.out.println("othersInsertsAreVisible "+
					"(forward only)");
		boolval=md.othersInsertsAreVisible(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("othersInsertsAreVisible "+
					"(scroll insensitive)");
		boolval=md.othersInsertsAreVisible(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("othersInsertsAreVisible "+
					"(scroll sensitive)");
		boolval=md.othersInsertsAreVisible(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("  "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertFalse(boolval);
		}
		System.out.println();

		// othersUpdatesAreVisible
		System.out.println("othersUpdatesAreVisible "+
					"(forward only)");
		boolval=md.othersUpdatesAreVisible(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("othersUpdatesAreVisible "+
					"(scroll insensitive)");
		boolval=md.othersUpdatesAreVisible(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("othersUpdatesAreVisible "+
					"(scroll sensitive)");
		boolval=md.othersUpdatesAreVisible(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("  "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		// ownDeletesAreVisible
		System.out.println("ownDeletesAreVisible "+
					"(forward only)");
		boolval=md.ownDeletesAreVisible(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("ownDeletesAreVisible "+
					"(scroll insensitive)");
		boolval=md.ownDeletesAreVisible(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("ownDeletesAreVisible "+
					"(scroll sensitive)");
		boolval=md.ownDeletesAreVisible(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("  "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		// ownInsertsAreVisible
		System.out.println("ownInsertsAreVisible "+
					"(forward only)");
		boolval=md.ownInsertsAreVisible(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("ownInsertsAreVisible "+
					"(scroll insensitive)");
		boolval=md.ownInsertsAreVisible(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("ownInsertsAreVisible "+
					"(scroll sensitive)");
		boolval=md.ownInsertsAreVisible(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("  "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertFalse(boolval);
		}
		System.out.println();

		// ownUpdatesAreVisible
		System.out.println("ownUpdatesAreVisible "+
					"(forward only)");
		boolval=md.ownUpdatesAreVisible(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		if (issqlrelay) {
			assertTrue(boolval);
		} else {
			// oracle jdbc randomly returns true or
			// false for this, so just don't test it
		}
		System.out.println();

		System.out.println("ownUpdatesAreVisible "+
					"(scroll insensitive)");
		boolval=md.ownUpdatesAreVisible(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("ownUpdatesAreVisible "+
					"(scroll sensitive)");
		boolval=md.ownUpdatesAreVisible(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("  "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		// storesLowerCaseIdentifiers
		System.out.println("storesLowerCaseIdentifiers");
		boolval=md.storesLowerCaseIdentifiers();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// storesLowerCaseQuotedIdentifiers
		System.out.println("storesLowerCaseQuotedIdentifiers");
		boolval=md.storesLowerCaseQuotedIdentifiers();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// storesMixedCaseIdentifiers
		System.out.println("storesMixedCaseIdentifiers");
		boolval=md.storesMixedCaseIdentifiers();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// storesMixedCaseQuotedIdentifiers
		System.out.println("storesMixedCaseQuotedIdentifiers");
		boolval=md.storesMixedCaseQuotedIdentifiers();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// storesUpperCaseIdentifiers
		System.out.println("storesUpperCaseIdentifiers");
		boolval=md.storesUpperCaseIdentifiers();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// storesUpperCaseQuotedIdentifiers
		System.out.println("storesUpperCaseQuotedIdentifiers");
		boolval=md.storesUpperCaseQuotedIdentifiers();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsAlterTableWithAddColumn
		System.out.println("supportsAlterTableWithAddColumn");
		boolval=md.supportsAlterTableWithAddColumn();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsAlterTableWithDropColumn
		System.out.println("supportsAlterTableWithDropColumn");
		boolval=md.supportsAlterTableWithDropColumn();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsANSI92EntryLevelSQL
		System.out.println("supportsANSI92EntryLevelSQL");
		boolval=md.supportsANSI92EntryLevelSQL();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsANSI92FullSQL
		System.out.println("supportsANSI92FullSQL");
		boolval=md.supportsANSI92FullSQL();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsANSI92IntermediateSQL
		System.out.println("supportsANSI92IntermediateSQL");
		boolval=md.supportsANSI92IntermediateSQL();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsBatchUpdates
		System.out.println("supportsBatchUpdates");
		boolval=md.supportsBatchUpdates();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsCatalogsInDataManipulation
		System.out.println("supportsCatalogsInDataManipulation");
		boolval=md.supportsCatalogsInDataManipulation();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsCatalogsInIndexDefinitions
		System.out.println("supportsCatalogsInIndexDefinitions");
		boolval=md.supportsCatalogsInIndexDefinitions();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsCatalogsInPrivilegeDefinitions
		System.out.println("supportsCatalogsInPrivilegeDefinitions");
		boolval=md.supportsCatalogsInPrivilegeDefinitions();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsCatalogsInProcedureCalls
		System.out.println("supportsCatalogsInProcedureCalls");
		boolval=md.supportsCatalogsInProcedureCalls();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsCatalogsInTableDefinitions
		System.out.println("supportsCatalogsInTableDefinitions");
		boolval=md.supportsCatalogsInTableDefinitions();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsColumnAliasing
		System.out.println("supportsColumnAliasing");
		boolval=md.supportsColumnAliasing();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsConvert
		System.out.println("supportsConvert");
		boolval=md.supportsConvert();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsConvert (with types)
		// sqlrelay doesn't support this yet
		System.out.println("supportsConvert (with types)");
		boolval=md.supportsConvert(Types.INTEGER, Types.VARCHAR);
		System.out.println("  "+boolval);
		if (issqlrelay) {
			assertTrue(boolval);
		} else {
			assertFalse(boolval);
		}
		System.out.println();

		// supportsCoreSQLGrammar
		System.out.println("supportsCoreSQLGrammar");
		boolval=md.supportsCoreSQLGrammar();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsCorrelatedSubqueries
		System.out.println("supportsCorrelatedSubqueries");
		boolval=md.supportsCorrelatedSubqueries();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsDataDefinitionAndDataManipulationTransactions
		System.out.println(
		"supportsDataDefinitionAndDataManipulationTransactions");
		boolval=
		md.supportsDataDefinitionAndDataManipulationTransactions();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsDataManipulationTransactionsOnly
		System.out.println("supportsDataManipulationTransactionsOnly");
		boolval=md.supportsDataManipulationTransactionsOnly();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsDifferentTableCorrelationNames
		System.out.println("supportsDifferentTableCorrelationNames");
		boolval=md.supportsDifferentTableCorrelationNames();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsExpressionsInOrderBy
		System.out.println("supportsExpressionsInOrderBy");
		boolval=md.supportsExpressionsInOrderBy();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsExtendedSQLGrammar
		System.out.println("supportsExtendedSQLGrammar");
		boolval=md.supportsExtendedSQLGrammar();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsFullOuterJoins
		System.out.println("supportsFullOuterJoins");
		boolval=md.supportsFullOuterJoins();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsGetGeneratedKeys
		System.out.println("supportsGetGeneratedKeys");
		boolval=md.supportsGetGeneratedKeys();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsGroupBy
		System.out.println("supportsGroupBy");
		boolval=md.supportsGroupBy();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsGroupByBeyondSelect
		System.out.println("supportsGroupByBeyondSelect");
		boolval=md.supportsGroupByBeyondSelect();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsGroupByUnrelated
		System.out.println("supportsGroupByUnrelated");
		boolval=md.supportsGroupByUnrelated();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsIntegrityEnhancementFacility
		System.out.println("supportsIntegrityEnhancementFacility");
		boolval=md.supportsIntegrityEnhancementFacility();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsLikeEscapeClause
		System.out.println("supportsLikeEscapeClause");
		boolval=md.supportsLikeEscapeClause();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsLimitedOuterJoins
		System.out.println("supportsLimitedOuterJoins");
		boolval=md.supportsLimitedOuterJoins();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsMinimumSQLGrammar
		System.out.println("supportsMinimumSQLGrammar");
		boolval=md.supportsMinimumSQLGrammar();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsMixedCaseIdentifiers
		System.out.println("supportsMixedCaseIdentifiers");
		boolval=md.supportsMixedCaseIdentifiers();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsMixedCaseQuotedIdentifiers
		System.out.println("supportsMixedCaseQuotedIdentifiers");
		boolval=md.supportsMixedCaseQuotedIdentifiers();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsMultipleOpenResults
		System.out.println("supportsMultipleOpenResults");
		boolval=md.supportsMultipleOpenResults();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsMultipleResultSets
		System.out.println("supportsMultipleResultSets");
		boolval=md.supportsMultipleResultSets();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsMultipleTransactions
		System.out.println("supportsMultipleTransactions");
		boolval=md.supportsMultipleTransactions();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsNamedParameters
		System.out.println("supportsNamedParameters");
		boolval=md.supportsNamedParameters();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsNonNullableColumns
		System.out.println("supportsNonNullableColumns");
		boolval=md.supportsNonNullableColumns();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsOpenCursorsAcrossCommit
		System.out.println("supportsOpenCursorsAcrossCommit");
		boolval=md.supportsOpenCursorsAcrossCommit();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsOpenCursorsAcrossRollback
		System.out.println("supportsOpenCursorsAcrossRollback");
		boolval=md.supportsOpenCursorsAcrossRollback();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsOpenStatementsAcrossCommit
		System.out.println("supportsOpenStatementsAcrossCommit");
		boolval=md.supportsOpenStatementsAcrossCommit();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsOpenStatementsAcrossRollback
		System.out.println("supportsOpenStatementsAcrossRollback");
		boolval=md.supportsOpenStatementsAcrossRollback();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsOrderByUnrelated
		System.out.println("supportsOrderByUnrelated");
		boolval=md.supportsOrderByUnrelated();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsOuterJoins
		System.out.println("supportsOuterJoins");
		boolval=md.supportsOuterJoins();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsPositionedDelete
		System.out.println("supportsPositionedDelete");
		boolval=md.supportsPositionedDelete();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsPositionedUpdate
		System.out.println("supportsPositionedUpdate");
		boolval=md.supportsPositionedUpdate();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsResultSetConcurrency
		System.out.println("supportsResultSetConcurrency "+
					"(forward only, read only)");
		boolval=md.supportsResultSetConcurrency(
					ResultSet.TYPE_FORWARD_ONLY,
					ResultSet.CONCUR_READ_ONLY);
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("supportsResultSetConcurrency "+
					"(forward only, updatable)");
		boolval=md.supportsResultSetConcurrency(
					ResultSet.TYPE_FORWARD_ONLY,
					ResultSet.CONCUR_UPDATABLE);
		System.out.println("  "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support CONCUR_UPDATEABLE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		System.out.println("supportsResultSetConcurrency "+
					"(scroll insensitive, read only)");
		boolval=md.supportsResultSetConcurrency(
					ResultSet.TYPE_SCROLL_INSENSITIVE,
					ResultSet.CONCUR_READ_ONLY);
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("supportsResultSetConcurrency "+
					"(scroll insensitive, updatable)");
		boolval=md.supportsResultSetConcurrency(
					ResultSet.TYPE_SCROLL_INSENSITIVE,
					ResultSet.CONCUR_UPDATABLE);
		System.out.println("  "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support CONCUR_UPDATEABLE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		System.out.println("supportsResultSetConcurrency "+
					"(scroll sensitive, read only)");
		boolval=md.supportsResultSetConcurrency(
					ResultSet.TYPE_SCROLL_SENSITIVE,
					ResultSet.CONCUR_READ_ONLY);
		System.out.println("  "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		System.out.println("supportsResultSetConcurrency "+
					"(scroll sensitive, updatable)");
		boolval=md.supportsResultSetConcurrency(
					ResultSet.TYPE_SCROLL_SENSITIVE,
					ResultSet.CONCUR_UPDATABLE);
		System.out.println("  "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			// or CONCUR_UPDATABLE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		// supportsResultSetHoldability
		System.out.println("supportsResultSetHoldability "+
					"(hold cursors over commit)");
		boolval=md.supportsResultSetHoldability(
					ResultSet.HOLD_CURSORS_OVER_COMMIT);
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("supportsResultSetHoldability "+
					"(close cursors at commit)");
		boolval=md.supportsResultSetHoldability(
					ResultSet.CLOSE_CURSORS_AT_COMMIT);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsResultSetType
		System.out.println("supportsResultSetType "+
					"(forward only)");
		boolval=md.supportsResultSetType(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("supportsResultSetType "+
					"(scroll insensitive)");
		boolval=md.supportsResultSetType(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("supportsResultSetType "+
					"(scroll sensitive)");
		boolval=md.supportsResultSetType(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("  "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		// supportsSavepoints
		System.out.println("supportsSavepoints");
		boolval=md.supportsSavepoints();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSchemasInDataManipulation
		System.out.println("supportsSchemasInDataManipulation");
		boolval=md.supportsSchemasInDataManipulation();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSchemasInIndexDefinitions
		System.out.println("supportsSchemasInIndexDefinitions");
		boolval=md.supportsSchemasInIndexDefinitions();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSchemasInPrivilegeDefinitions
		System.out.println("supportsSchemasInPrivilegeDefinitions");
		boolval=md.supportsSchemasInPrivilegeDefinitions();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSchemasInProcedureCalls
		System.out.println("supportsSchemasInProcedureCalls");
		boolval=md.supportsSchemasInProcedureCalls();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSchemasInTableDefinitions
		System.out.println("supportsSchemasInTableDefinitions");
		boolval=md.supportsSchemasInTableDefinitions();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSelectForUpdate
		System.out.println("supportsSelectForUpdate");
		boolval=md.supportsSelectForUpdate();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsStatementPooling
		System.out.println("supportsStatementPooling");
		boolval=md.supportsStatementPooling();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsStoredFunctionsUsingCallSyntax
		System.out.println("supportsStoredFunctionsUsingCallSyntax");
		boolval=md.supportsStoredFunctionsUsingCallSyntax();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsStoredProcedures
		System.out.println("supportsStoredProcedures");
		boolval=md.supportsStoredProcedures();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSubqueriesInComparisons
		System.out.println("supportsSubqueriesInComparisons");
		boolval=md.supportsSubqueriesInComparisons();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSubqueriesInExists
		System.out.println("supportsSubqueriesInExists");
		boolval=md.supportsSubqueriesInExists();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSubqueriesInIns
		System.out.println("supportsSubqueriesInIns");
		boolval=md.supportsSubqueriesInIns();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSubqueriesInQuantifieds
		System.out.println("supportsSubqueriesInQuantifieds");
		boolval=md.supportsSubqueriesInQuantifieds();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsTableCorrelationNames
		System.out.println("supportsTableCorrelationNames");
		boolval=md.supportsTableCorrelationNames();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsTransactionIsolationLevel
		System.out.println("supportsTransactionIsolationLevel "+
							"(none)");
		boolval=md.supportsTransactionIsolationLevel(
				Connection.TRANSACTION_NONE);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("supportsTransactionIsolationLevel "+
							"(read uncommitted)");
		boolval=md.supportsTransactionIsolationLevel(
				Connection.TRANSACTION_READ_UNCOMMITTED);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("supportsTransactionIsolationLevel "+
							"(read committed)");
		boolval=md.supportsTransactionIsolationLevel(
				Connection.TRANSACTION_READ_COMMITTED);
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("supportsTransactionIsolationLevel "+
							"(repeatable read)");
		boolval=md.supportsTransactionIsolationLevel(
				Connection.TRANSACTION_REPEATABLE_READ);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("supportsTransactionIsolationLevel "+
							"(serializable)");
		boolval=md.supportsTransactionIsolationLevel(
				Connection.TRANSACTION_SERIALIZABLE);
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsTransactions
		System.out.println("supportsTransactions");
		boolval=md.supportsTransactions();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsUnion
		System.out.println("supportsUnion");
		boolval=md.supportsUnion();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsUnionAll
		System.out.println("supportsUnionAll");
		boolval=md.supportsUnionAll();
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		// updatesAreDetected
		System.out.println("updatesAreDetected "+
					"(forward only)");
		boolval=md.updatesAreDetected(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("updatesAreDetected "+
					"(scroll insensitive)");
		boolval=md.updatesAreDetected(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("updatesAreDetected "+
					"(scroll sensitive)");
		boolval=md.updatesAreDetected(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("  "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertFalse(boolval);
		}
		System.out.println();

		// usesLocalFilePerTable
		System.out.println("usesLocalFilePerTable");
		boolval=md.usesLocalFilePerTable();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// usesLocalFiles
		System.out.println("usesLocalFiles");
		boolval=md.usesLocalFiles();
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();


		// statement
		System.out.println("STATEMENT:");

		// createStatement
		System.out.println("create statement");
		stmt=con.createStatement();
		assertTrue((stmt!=null));
		assertEquals(stmt.getConnection(),con);
                System.out.println();

		// unwrap
		if (issqlrelay) {
			System.out.println("unwrap");
			assertEquals(stmt.isWrapperFor(SQLRCursor.class),1);
			System.out.println();
			assertEquals((stmt.unwrap(SQLRCursor.class)!=null),1);
			System.out.println();
		}

		// query timeouts
		System.out.println("query timeouts");
		stmt.setQueryTimeout(10);
		assertEquals(stmt.getQueryTimeout(),10);
		stmt.setQueryTimeout(0);
		assertEquals(stmt.getQueryTimeout(),0);
		stmt.close();

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


		// drop existing table
		stmt=con.createStatement();
		try {
			stmt.executeUpdate("drop table testtable");
		} catch (Exception ex) {
		}


		// create temptable
		System.out.println("CREATE TEMPTABLE:");
		assertEquals(stmt.executeUpdate(
			"create table testtable ("+
			"	testnumber number, "+
			"	testchar char(40), "+
			"	testvarchar varchar2(40), "+
			"	testdate date, "+
			"	testlong long, "+
			"	testclob clob, "+
			"	testblob blob, "+
			"	testurl varchar2(60))"),0);
		System.out.println();


		// insert
		System.out.println("INSERT:");
		assertFalse(stmt.execute(
			"insert into "+
			"	testtable "+
			"values ("+
			"	1, "+
			"	'testchar1', "+
			"	'testvarchar1', "+
			"	'01-JAN-2001', "+
			"	'testlong1', "+
			"	'testclob1', "+
			"	empty_blob(), "+
			"	'http://www.firstworks.com:8080/testurl1')"));
		assertEquals(stmt.getUpdateCount(),1);
		stmt.close();
		System.out.println();


		// bind by position
		System.out.println("BIND BY POSITION:");
		pstmt=con.prepareStatement(
			"insert into "+
			"	testtable "+
			"values ("+
			"	:var1, "+
			"	:var2, "+
			"	:var3, "+
			"	:var4, "+
			"	:var5, "+
			"	:var6, "+
			"	:var7, "+
			"	:var8)");
		assertFalse(pstmt.isClosed());
		clob=con.createClob();
		blob=con.createBlob();
		for (int i=2; i<=4; i++) {
			pstmt.clearParameters();
			pstmt.setInt(1,i);
			pstmt.setString(2,"testchar"+i);
			pstmt.setString(3,"testvarchar"+i);

			cal.set(Calendar.YEAR,2000+i);
			cal.set(Calendar.MONTH,Calendar.JANUARY);
			cal.set(Calendar.DAY_OF_MONTH,1);
			cal.set(Calendar.HOUR_OF_DAY,0);
			cal.set(Calendar.MINUTE,0);
			cal.set(Calendar.SECOND,0);
			pstmt.setDate(4,new java.sql.Date(
						cal.getTimeInMillis()));

			pstmt.setString(5,"testlong"+i);
			clob.setString(1,"testclob"+i);
			pstmt.setClob(6,clob);
			blob.setBytes(1,(new String("testblob"+i)).
					getBytes(StandardCharsets.UTF_8));
			pstmt.setBlob(7,blob);
			pstmt.setString(8,
				"http://www.firstworks.com:8080/"+
				"testurl"+i);
			assertEquals(pstmt.executeUpdate(),1);
			System.out.println();
		}
		pstmt.close();
		assertTrue(pstmt.isClosed());


		// bind by name
		System.out.println("BIND BY NAME:");
		cstmt=con.prepareCall(
			"insert into "+
			"	testtable "+
			"values ("+
			"	:var1, "+
			"	:var2, "+
			"	:var3, "+
			"	:var4, "+
			"	:var5, "+
			"	:var6, "+
			"	:var7, "+
			"	:var8)");
		assertFalse(cstmt.isClosed());
		for (int i=5; i<=8; i++) {
			cstmt.clearParameters();
			cstmt.setInt("var1",i);
			cstmt.setString("var2","testchar"+i);
			cstmt.setString("var3","testvarchar"+i);

			cal.set(Calendar.YEAR,2000+i);
			cal.set(Calendar.MONTH,Calendar.JANUARY);
			cal.set(Calendar.DAY_OF_MONTH,1);
			cal.set(Calendar.HOUR_OF_DAY,0);
			cal.set(Calendar.MINUTE,0);
			cal.set(Calendar.SECOND,0);
			cstmt.setDate("var4",new java.sql.Date(
						cal.getTimeInMillis()));

			cstmt.setString("var5","testlong"+i);
			clob.setString(1,"testclob"+i);
			cstmt.setClob("var6",clob);
			blob.setBytes(1,(new String("testblob"+i)).
					getBytes(StandardCharsets.UTF_8));
			cstmt.setBlob("var7",blob);
			cstmt.setString("var8",
				"http://www.firstworks.com:8080/"+
				"testurl"+i);
			assertEquals(cstmt.executeUpdate(),1);
			System.out.println();
		}
		cstmt.close();
		assertTrue(cstmt.isClosed());


		// select
		System.out.println("SELECT:");
		stmt=con.createStatement();
		assertTrue((stmt!=null));
		assertTrue(stmt.execute(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testnumber"));
		assertFalse(stmt.isClosed());
		rs=stmt.getResultSet();
		assertTrue((rs!=null));
		System.out.println();

		// unwrap
		if (issqlrelay) {
			System.out.println("unwrap");
			assertEquals(rs.isWrapperFor(SQLRCursor.class),1);
			System.out.println();
			assertEquals((rs.unwrap(SQLRCursor.class)!=null),1);
			System.out.println();
		}

		// getMetaData
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		System.out.println();

		// unwrap
		if (issqlrelay) {
			System.out.println("unwrap");
			assertEquals(rsmd.isWrapperFor(SQLRCursor.class),1);
			System.out.println();
			assertEquals((rsmd.unwrap(SQLRCursor.class)!=null),1);
			System.out.println();
		}


		// column count
		System.out.println("COLUMN COUNT:");
		assertEquals(rsmd.getColumnCount(),8);
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
		assertEquals(rsmd.getColumnName(8),"TESTURL");
		System.out.println();


		// column labels
		System.out.println("COLUMN LABELS:");
		assertEquals(rsmd.getColumnLabel(1),"TESTNUMBER");
		assertEquals(rsmd.getColumnLabel(2),"TESTCHAR");
		assertEquals(rsmd.getColumnLabel(3),"TESTVARCHAR");
		assertEquals(rsmd.getColumnLabel(4),"TESTDATE");
		assertEquals(rsmd.getColumnLabel(5),"TESTLONG");
		assertEquals(rsmd.getColumnLabel(6),"TESTCLOB");
		assertEquals(rsmd.getColumnLabel(7),"TESTBLOB");
		assertEquals(rsmd.getColumnLabel(8),"TESTURL");
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
		assertEquals(rsmd.getColumnTypeName(8),"VARCHAR2");
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
		assertEquals(rsmd.getPrecision(8),60);
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
		assertEquals(rsmd.getColumnDisplaySize(8),60);
		System.out.println();


		// fields by index
		System.out.println("FIELDS BY INDEX:");
		for (int i=1; i<=8; i++) {

			rs.next();

			// number as short
			System.out.println("row "+i+" - number as short");
			assertEquals(rs.getShort(1),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// number as int
			System.out.println("row "+i+" - number as int");
			assertEquals(rs.getInt(1),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// number as long
			System.out.println("row "+i+" - number as long");
			assertEquals(rs.getLong(1),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// char as string
			System.out.println("row "+i+" - char as string");
			assertEquals(rs.getString(2),"testchar"+i+
					"                               ");
			assertFalse(rs.wasNull());
			System.out.println();

			// varchar as string
			System.out.println("row "+i+" - varchar as string");
			assertEquals(rs.getString(3),"testvarchar"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// date
			System.out.println("row "+i+" - date");
			datevar=rs.getDate(4);
			cal.setTime(datevar);
			assertEquals(cal.get(Calendar.YEAR),2000+i);
			assertEquals(cal.get(Calendar.MONTH),Calendar.JANUARY);
			assertEquals(cal.get(Calendar.DAY_OF_MONTH),1);
			assertEquals(cal.get(Calendar.HOUR_OF_DAY),0);
			assertEquals(cal.get(Calendar.MINUTE),0);
			assertEquals(cal.get(Calendar.SECOND),0);
			assertFalse(rs.wasNull());
			System.out.println();

			// date as timestamp
			System.out.println("row "+i+" - date as timestamp");
			tsvar=rs.getTimestamp(4);
			cal.setTime(tsvar);
			assertEquals(cal.get(Calendar.YEAR),2000+i);
			assertEquals(cal.get(Calendar.MONTH),Calendar.JANUARY);
			assertEquals(cal.get(Calendar.DAY_OF_MONTH),1);
			assertEquals(cal.get(Calendar.HOUR_OF_DAY),0);
			assertEquals(cal.get(Calendar.MINUTE),0);
			assertEquals(cal.get(Calendar.SECOND),0);
			assertFalse(rs.wasNull());
			System.out.println();

			// long as string
			System.out.println("row "+i+" - long as string");
			assertEquals(rs.getString(5),"testlong"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob
			System.out.println("row "+i+" - clob");
			clob=rs.getClob(6);
			assertEquals(clob.getSubString(1,(int)clob.length()),
								"testclob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob as string
			System.out.println("row "+i+" - clob as string");
			assertEquals(rs.getString(6),"testclob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob as nstring
			System.out.println("row "+i+" - clob as nstring");
			assertEquals(rs.getNString(6),"testclob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob as ascii stream
			System.out.println("row "+i+" - clob as ascii stream");
			assertEquals(new String(rs.getAsciiStream(6).
							readAllBytes(),"UTF-8"),
							"testclob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob as character stream
			System.out.println("row "+i+
					" - clob as character stream");
			StringWriter sw=new StringWriter();
			rs.getCharacterStream(6).transferTo(sw);
			assertEquals(sw.toString(),"testclob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob as ncharacter stream
			System.out.println("row "+i+
					" - clob as ncharacter stream");
			sw=new StringWriter();
			rs.getNCharacterStream(6).transferTo(sw);
			assertEquals(sw.toString(),"testclob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob as unicode stream
			// oracle jdbc doesn't support fetching
			// clobs or blobs as unicode streams
			if (issqlrelay) {
				System.out.println("row "+i+
					" - clob as unicode stream");
				assertEquals(new String(
						rs.getUnicodeStream(6).
							readAllBytes(),"UTF-8"),
							"testclob"+i);
				assertFalse(rs.wasNull());
				System.out.println();
			}

			// blob
			System.out.println("row "+i+" - blob");
			blob=rs.getBlob(7);
			// the blob in the first row is empty on purpose
			assertEquals(new String(
					blob.getBytes(1,(int)blob.length()),
					"UTF-8"),
					(i==1)?"":"testblob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob as bytes
			System.out.println("row "+i+" - blob as bytes");
			assertEquals(new String(
					rs.getBytes(7),"UTF-8"),
					(i==1)?"":"testblob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob as binary stream
			System.out.println("row "+i+
					" - blob as binary stream");
			assertEquals(new String(rs.getBinaryStream(7).
						readAllBytes(),"UTF-8"),
						(i==1)?"":"testblob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// url
			System.out.println("row "+i+" - url");
			URL	urlvar=rs.getURL(8);
			assertEquals(urlvar.getProtocol(),"http");
			assertEquals(urlvar.getHost(),"www.firstworks.com");
			assertEquals(urlvar.getPort(),8080);
			assertEquals(urlvar.getPath(),"/testurl"+i);
			assertFalse(rs.wasNull());
			System.out.println();
		}
		rs.close();


		// fields by name
		System.out.println("FIELDS BY NAME:");
		rs=stmt.executeQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testnumber");
		assertTrue((rs!=null));
		System.out.println();

		for (int i=1; i<=8; i++) {

			rs.next();

			// number as short
			System.out.println("row "+i+" - number as short");
			assertEquals(rs.getShort("TESTNUMBER"),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// number as int
			System.out.println("row "+i+" - number as int");
			assertEquals(rs.getInt("TESTNUMBER"),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// number as long
			System.out.println("row "+i+" - number as long");
			assertEquals(rs.getLong("TESTNUMBER"),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// char as string
			System.out.println("row "+i+" - char as string");
			assertEquals(rs.getString("TESTCHAR"),"testchar"+i+
					"                               ");
			assertFalse(rs.wasNull());
			System.out.println();

			// varchar as string
			System.out.println("row "+i+" - varchar as string");
			assertEquals(rs.getString("TESTVARCHAR"),
							"testvarchar"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// date
			System.out.println("row "+i+" - date");
			datevar=rs.getDate("TESTDATE");
			cal.setTime(datevar);
			assertEquals(cal.get(Calendar.YEAR),2000+i);
			assertEquals(cal.get(Calendar.MONTH),Calendar.JANUARY);
			assertEquals(cal.get(Calendar.DAY_OF_MONTH),1);
			assertEquals(cal.get(Calendar.HOUR_OF_DAY),0);
			assertEquals(cal.get(Calendar.MINUTE),0);
			assertEquals(cal.get(Calendar.SECOND),0);
			assertFalse(rs.wasNull());
			System.out.println();

			// date as timestamp
			System.out.println("row "+i+" - date as timestamp");
			tsvar=rs.getTimestamp("TESTDATE");
			cal.setTime(tsvar);
			assertEquals(cal.get(Calendar.YEAR),2000+i);
			assertEquals(cal.get(Calendar.MONTH),Calendar.JANUARY);
			assertEquals(cal.get(Calendar.DAY_OF_MONTH),1);
			assertEquals(cal.get(Calendar.HOUR_OF_DAY),0);
			assertEquals(cal.get(Calendar.MINUTE),0);
			assertEquals(cal.get(Calendar.SECOND),0);
			assertFalse(rs.wasNull());
			System.out.println();

			// long as string
			System.out.println("row "+i+" - long as string");
			assertEquals(rs.getString("TESTLONG"),"testlong"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob
			System.out.println("row "+i+" - clob");
			clob=rs.getClob("TESTCLOB");
			assertEquals(clob.getSubString(1,(int)clob.length()),
								"testclob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob as string
			System.out.println("row "+i+" - clob as string");
			assertEquals(rs.getString("TESTCLOB"),"testclob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob as nstring
			System.out.println("row "+i+" - clob as nstring");
			assertEquals(rs.getNString("TESTCLOB"),"testclob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob as ascii stream
			System.out.println("row "+i+
					" - clob as ascii stream");
			assertEquals(new String(rs.getAsciiStream("TESTCLOB").
							readAllBytes(),"UTF-8"),
							"testclob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob as character stream
			System.out.println("row "+i+
					" - clob as character stream");
			StringWriter sw=new StringWriter();
			rs.getCharacterStream("TESTCLOB").transferTo(sw);
			assertEquals(sw.toString(),"testclob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob as ncharacter stream
			System.out.println("row "+i+
					" - clob as ncharacter stream");
			sw=new StringWriter();
			rs.getNCharacterStream("TESTCLOB").transferTo(sw);
			assertEquals(sw.toString(),"testclob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob as unicode stream
			// oracle jdbc doesn't support fetching
			// clobs or blobs as unicode streams
			if (issqlrelay) {
				System.out.println("row "+i+
					" - clob as unicode stream");
				assertEquals(new String(
						rs.getUnicodeStream("TESTCLOB").
							readAllBytes(),"UTF-8"),
							"testclob"+i);
				assertFalse(rs.wasNull());
				System.out.println();
			}

			// blob
			System.out.println("row "+i+" - blob");
			blob=rs.getBlob("TESTBLOB");
			// the blob in the first row is empty on purpose
			assertEquals(new String(
					blob.getBytes(1,(int)blob.length()),
					"UTF-8"),
					(i==1)?"":"testblob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob as bytes
			System.out.println("row "+i+" - blob as bytes");
			assertEquals(new String(
					rs.getBytes("TESTBLOB"),"UTF-8"),
					(i==1)?"":"testblob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob as binary stream
			System.out.println("row "+i+
					" - blob as binary stream");
			assertEquals(new String(rs.getBinaryStream("TESTBLOB").
						readAllBytes(),"UTF-8"),
						(i==1)?"":"testblob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// url
			System.out.println("row "+i+" - url");
			URL	urlvar=rs.getURL("TESTURL");
			assertEquals(urlvar.getProtocol(),"http");
			assertEquals(urlvar.getHost(),"www.firstworks.com");
			assertEquals(urlvar.getPort(),8080);
			assertEquals(urlvar.getPath(),"/testurl"+i);
			assertFalse(rs.wasNull());
			System.out.println();
		}


		// row count
		System.out.println("ROW COUNT:");
		assertEquals(rs.getRow(),8);
		rs.close();
		System.out.println();


		if (issqlrelay) {

			// fetch size 0
			System.out.println("FETCH SIZE 0:");
			assertEquals(stmt.getFetchSize(),0);
			rs=stmt.executeQuery(
				"select "+
				"	* "+
				"from "+
				"	testtable "+
				"order by "+
				"	testnumber");
			assertEquals(rs.getFetchSize(),0);
			System.out.println();

			// jump around wildly
			rs.afterLast();
			assertTrue(rs.isAfterLast());
			rs.beforeFirst();
			assertTrue(rs.isBeforeFirst());
			assertTrue(rs.last());
			assertTrue(rs.isLast());
			assertTrue(rs.first());
			assertTrue(rs.isFirst());
			assertTrue(rs.absolute(4));
			assertEquals(rs.getInt(1),4);
			assertFalse(rs.isBeforeFirst());
			assertFalse(rs.isFirst());
			assertFalse(rs.isLast());
			assertFalse(rs.isAfterLast());
			assertTrue(rs.relative(2));
			assertEquals(rs.getInt(1),6);
			assertFalse(rs.isBeforeFirst());
			assertFalse(rs.isFirst());
			assertFalse(rs.isLast());
			assertFalse(rs.isAfterLast());
			assertTrue(rs.relative(-4));
			assertEquals(rs.getInt(1),2);
			assertFalse(rs.isBeforeFirst());
			assertFalse(rs.isFirst());
			assertFalse(rs.isLast());
			assertFalse(rs.isAfterLast());
			rs.beforeFirst();
			System.out.println();

			// move into the result set
			assertTrue(rs.isBeforeFirst());
			assertTrue(rs.next());
			assertTrue(rs.isFirst());
			System.out.println();

			// move forwards to the last row
			for (int row=1; row<=7; row++) {
				assertEquals(rs.getInt(1),row);
				assertTrue(rs.next());
			}
			System.out.println();
			assertEquals(rs.getInt(1),8);
			assertTrue(rs.isLast());
			System.out.println();

			// move backwards to the first row
			for (int row=8; row>=2; row--) {
				assertEquals(rs.getInt(1),row);
				assertTrue(rs.previous());
			}
			System.out.println();
			assertEquals(rs.getInt(1),1);
			assertTrue(rs.isFirst());
			System.out.println();

			// move fowards to the last row again
			for (int row=1; row<=7; row++) {
				assertEquals(rs.getInt(1),row);
				assertTrue(rs.next());
			}
			System.out.println();
			assertEquals(rs.getInt(1),8);
			assertTrue(rs.isLast());
			System.out.println();

			// move past the end of the result set
			assertFalse(rs.next());
			assertTrue(rs.isAfterLast());
			System.out.println();


			// fetch size 2
			System.out.println("FETCH SIZE 2:");
			stmt.setFetchSize(2);
			assertEquals(stmt.getFetchSize(),2);
			rs=stmt.executeQuery(
				"select "+
				"	* "+
				"from "+
				"	testtable "+
				"order by "+
				"	testnumber");
			assertEquals(rs.getFetchSize(),2);
			System.out.println();

			// rows 1-2 (first window)
			int	row=0;
			assertTrue(rs.isBeforeFirst());
			assertTrue(rs.next());
			row++;
			assertEquals(rs.getInt(1),1);
			assertTrue(rs.next());
			row++;
			assertEquals(rs.getInt(1),2);
			System.out.println();

			do {
				// move forward to trigger fetch of a new window
				assertTrue(rs.next());
				row++;
				assertEquals(rs.getInt(1),row);

				// move forward to end of the window
				assertTrue(rs.next());
				row++;
				assertEquals(rs.getInt(1),row);

				// move backward to beginning of the window
				assertTrue(rs.previous());
				row--;
				assertEquals(rs.getInt(1),row);

				// move backward to before the window
				assertTrue(rs.previous());
				row--;
				assertEquals(rs.getString(1),null);

				// move forward back into the window
				assertTrue(rs.next());
				row++;
				assertEquals(rs.getInt(1),row);

				// move forward to end of the window
				assertTrue(rs.next());
				row++;
				assertEquals(rs.getInt(1),row);

				System.out.println();

			} while (row<8);

			// is last isn't supported when fetch size is non-zero
			try {
				rs.isLast();
				assertTrue(false);
			} catch (Exception ex) {
				assertTrue(true);
			}

			// move past the end of the result set
			assertFalse(rs.next());
			assertTrue(rs.isAfterLast());
			assertFalse(rs.next());

			rs.close();
			stmt.setFetchSize(0);
			assertEquals(stmt.getFetchSize(),0);

			System.out.println();


			// max rows
			// FIXME: this doesn't currently work with oracle jdbc
			// because the result set is forward-only by default.
			// sort this out
			System.out.println("MAX ROWS:");
			assertEquals(stmt.getMaxRows(),0);
			stmt.setMaxRows(4);
			assertEquals(stmt.getMaxRows(),4);
			rs=stmt.executeQuery(
				"select "+
				"	* "+
				"from "+
				"	testtable "+
				"order by "+
				"	testnumber");
			assertTrue(rs.isBeforeFirst());
			assertTrue(rs.next());
			assertEquals(rs.getInt(1),1);
			assertTrue(rs.isFirst());
			assertTrue(rs.next());
			assertEquals(rs.getInt(1),2);
			assertTrue(rs.next());
			assertEquals(rs.getInt(1),3);
			assertTrue(rs.next());
			assertEquals(rs.getInt(1),4);
			assertTrue(rs.isLast());
			assertFalse(rs.next());
			assertTrue(rs.isAfterLast());
			assertTrue(rs.first());
			assertEquals(rs.getInt(1),1);
			assertTrue(rs.isFirst());
			assertTrue(rs.last());
			assertEquals(rs.getInt(1),4);
			assertTrue(rs.isLast());
			rs.beforeFirst();
			assertTrue(rs.isBeforeFirst());
			assertTrue(rs.next());
			assertEquals(rs.getInt(1),1);
			rs.afterLast();
			assertTrue(rs.isAfterLast());
			assertTrue(rs.previous());
			assertEquals(rs.getInt(1),4);
			assertTrue(rs.isLast());
			rs.close();
			stmt.setMaxRows(0);
			assertEquals(stmt.getMaxRows(),0);
			System.out.println();
		}


		// commit
		System.out.println("COMMIT:");
		Connection	secondcon=DriverManager.getConnection(
							url,user,password);
		assertTrue((secondcon!=null));
		Statement	secondstmt=secondcon.createStatement();
		assertTrue((secondstmt!=null));
		ResultSet	secondrs=secondstmt.executeQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	testtable ");
		assertTrue((secondrs!=null));
		assertTrue(secondrs.getStatement()==secondstmt);
		secondrs.next();
		assertEquals(secondrs.getString(1),"0");
		secondrs.close();
		con.commit();
		secondrs=secondstmt.executeQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	testtable ");
		assertTrue((secondrs!=null));
		secondrs.next();
		assertEquals(secondrs.getString(1),"8");
		con.setAutoCommit(true);
		secondrs.close();
		assertEquals(stmt.executeUpdate(
			"insert into "+
			"	testtable "+
			"values ("+
			"	10, "+
			"	'testchar10', "+
			"	'testvarchar10', "+
			"	'01-JAN-2010', "+
			"	'testlong10', "+
			"	'testclob10', "+
			"	NULL, "+
			"	'http://www.firstworks.com:8080/testurl10' "+
			"	)"),1);
		secondrs=secondstmt.executeQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	testtable ");
		assertTrue((secondrs!=null));
		secondrs.next();
		assertEquals(secondrs.getString(1),"9");
		secondrs.close();
		secondstmt.close();
		secondcon.close();
		con.setAutoCommit(false);
		System.out.println();


		// output bind by position
		System.out.println("OUTPUT BIND BY POSITION:");
		cstmt=con.prepareCall(
			"begin "+
			"	:numvar:=1; "+
			"	:stringvar:='hello'; "+
			"	:floatvar:=2.5; "+
			"	:datevar:='03-FEB-2001'; "+
			"	:nullvar:=null; "+
			"end;");
		cstmt.registerOutParameter("1",Types.INTEGER);
		cstmt.registerOutParameter("2",Types.VARCHAR);
		cstmt.registerOutParameter("3",Types.DOUBLE);
		cstmt.registerOutParameter("4",Types.DATE);
		cstmt.registerOutParameter("5",Types.VARCHAR);
		assertFalse(cstmt.execute());
		assertEquals(cstmt.getInt("1"),1);
		assertFalse(cstmt.wasNull());
		assertEquals(cstmt.getString("2"),"hello");
		assertFalse(cstmt.wasNull());
		assertEquals(cstmt.getDouble("3"),2.5);
		assertFalse(cstmt.wasNull());
		datevar=cstmt.getDate("4");
		assertFalse(cstmt.wasNull());
		cal.setTime(datevar);
		assertEquals(cal.get(Calendar.YEAR),2001);
		assertEquals(cal.get(Calendar.MONTH),Calendar.FEBRUARY);
		assertEquals(cal.get(Calendar.DAY_OF_MONTH),3);
		assertEquals(cal.get(Calendar.HOUR_OF_DAY),0);
		assertEquals(cal.get(Calendar.MINUTE),0);
		assertEquals(cal.get(Calendar.SECOND),0);
		assertEquals(cstmt.getString("5"),null);
		assertTrue(cstmt.wasNull());
		cstmt.close();
		System.out.println();


		// output bind by name
		System.out.println("OUTPUT BIND BY NAME:");
		cstmt=con.prepareCall(
			"begin "+
			"	:numvar:=1; "+
			"	:stringvar:='hello'; "+
			"	:floatvar:=2.5; "+
			"	:datevar:='03-FEB-2001'; "+
			"	:nullvar:=null; "+
			"end;");
		cstmt.registerOutParameter("numvar",Types.INTEGER);
		cstmt.registerOutParameter("stringvar",Types.VARCHAR);
		cstmt.registerOutParameter("floatvar",Types.DOUBLE);
		cstmt.registerOutParameter("datevar",Types.DATE);
		cstmt.registerOutParameter("nullvar",Types.VARCHAR);
		assertFalse(cstmt.execute());
		assertEquals(cstmt.getInt("numvar"),1);
		assertFalse(cstmt.wasNull());
		assertEquals(cstmt.getString("stringvar"),"hello");
		assertFalse(cstmt.wasNull());
		assertEquals(cstmt.getDouble("floatvar"),2.5);
		assertFalse(cstmt.wasNull());
		datevar=cstmt.getDate("datevar");
		assertFalse(cstmt.wasNull());
		cal.setTime(datevar);
		assertEquals(cal.get(Calendar.YEAR),2001);
		assertEquals(cal.get(Calendar.MONTH),Calendar.FEBRUARY);
		assertEquals(cal.get(Calendar.DAY_OF_MONTH),3);
		assertEquals(cal.get(Calendar.HOUR_OF_DAY),0);
		assertEquals(cal.get(Calendar.MINUTE),0);
		assertEquals(cal.get(Calendar.SECOND),0);
		assertEquals(cstmt.getString("nullvar"),null);
		assertTrue(cstmt.wasNull());
		cstmt.close();
		System.out.println();


		// clob and blob output bind
		System.out.println("CLOB AND BLOB OUTPUT BIND:");
		try {
			stmt.executeUpdate("drop table testtable1");
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate(
			"create table testtable1 ("+
			"	testclob clob, "+
			"	testblob blob)"),0);
		pstmt=con.prepareStatement(
			"insert into "+
			"	testtable1 "+
			"values ("+
			"	'hello', "+
			"	:var1)");
		assertTrue((pstmt!=null));
		pstmt.setBytes(1,"hello".getBytes(StandardCharsets.UTF_8));
		assertEquals(pstmt.executeUpdate(),1);
		pstmt.close();
		cstmt=con.prepareCall(
			"begin "+
			"	select testclob "+
			"		into :clobvar "+
			"		from testtable1; "+
			"	select testblob "+
			"		into :blobvar "+
			"		from testtable1; "+
			"end;");
		cstmt.registerOutParameter("clobvar",Types.CLOB);
		cstmt.registerOutParameter("blobvar",Types.BLOB);
		assertFalse(cstmt.execute());
		assertEquals(cstmt.getString("clobvar"),"hello");
		assertEquals(new String(cstmt.getBytes("blobvar"),
					StandardCharsets.UTF_8),"hello");
		cstmt.close();
		assertEquals(stmt.executeUpdate("drop table testtable1"),0);
		System.out.println();


		try {
			stmt.executeUpdate("drop table testtable1");
		} catch (Exception ex) {
		}


		// null and empty clobs and blobs
		System.out.println("NULL AND EMPTY CLOBS AND BLOBS:");
		assertEquals(stmt.executeUpdate(
			"create table testtable1 ("+
			"	testclob1 clob, "+
			"	testclob2 clob, "+
			"	testblob1 blob, "+
			"	testblob2 blob)"),0);
		pstmt=con.prepareStatement(
			"insert into "+
			"	testtable1 "+
			"values ("+
			"	:var1, "+
			"	:var2, "+
			"	:var3, "+
			"	:var4)");
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
		assertTrue((rs.getBlob(3)==null));
		assertTrue((rs.getBlob(4)==null));
		rs.close();
		assertEquals(stmt.executeUpdate("drop table testtable1"),0);
		System.out.println();


		try {
			stmt.executeUpdate("drop table testtable2");
		} catch (Exception ex) {
		}


		// long varchar
		System.out.println("LONG VARCHAR:");
		assertEquals(stmt.executeUpdate(
			"create table "+
			"	testtable2 (testvarchar varchar2(1024))"),0);
		pstmt=con.prepareStatement(
			"insert into "+
			"	testtable2 values (:stringval)");
		assertTrue((pstmt!=null));
		StringBuilder	sb=new StringBuilder();
		for (int i=0; i<1024; i++) {
			sb.append('C');
		}
		String	str=sb.toString();
		pstmt.setString(1,str);
		assertEquals(pstmt.executeUpdate(),1);
		pstmt.close();
		rs=stmt.executeQuery("select testvarchar from testtable2");
		assertTrue((rs!=null));
		rs.next();
		stringval=rs.getString(1);
		assertEquals(stringval.length(),1024);
		assertEquals(stringval,str);
		rs.close();
		stmt.setMaxFieldSize(512);
		assertEquals(stmt.getMaxFieldSize(),512);
		rs=stmt.executeQuery("select testvarchar from testtable2");
		assertTrue((rs!=null));
		rs.next();
		stringval=rs.getString(1);
		assertEquals(stringval.length(),512);
		assertEquals(stringval,str.substring(0,512));
		rs.close();
		stmt.setMaxFieldSize(0);
		assertEquals(stmt.getMaxFieldSize(),0);
		assertEquals(stmt.executeUpdate("drop table testtable2"),0);
		System.out.println();


		// long clob
		System.out.println("LONG CLOB:");
		assertEquals(stmt.executeUpdate(
			"create table "+
			"	testtable2 (testclob clob)"),0);
		pstmt=con.prepareStatement(
			"insert into "+
			"	testtable2 values (:clobval)");
		assertTrue((pstmt!=null));
		StringBuilder	clobval=new StringBuilder();
		// oracle jdbc struggles with more than 1024 byte clobs
		for (int i=0; i<1024; i++) {
			clobval.append('C');
		}
		String	clobstr=clobval.toString();
		pstmt.setString(1,clobstr);
		assertEquals(pstmt.executeUpdate(),1);
		pstmt.close();
		rs=stmt.executeQuery("select testclob from testtable2");
		assertTrue((rs!=null));
		rs.next();
		clob=rs.getClob(1);
		assertEquals(clob.length(),1024);
		assertEquals(clob.getSubString(1,(int)clob.length()),clobstr);
		rs.close();
		cstmt=con.prepareCall(
			"begin "+
			"	select testclob into :clobbindval "+
			"	from testtable2; "+
			"end;");
		cstmt.registerOutParameter("clobbindval",Types.CLOB);
		assertFalse(cstmt.execute());
		assertEquals(cstmt.getString("clobbindval"),clobstr);
		cstmt.close();
		assertEquals(stmt.executeUpdate("drop table testtable2"),0);
		System.out.println();


		// long output bind
		System.out.println("LONG OUTPUT BIND:");
		try {
			stmt.executeUpdate("drop table testtable2");
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate(
			"create table "+
			"	testtable2 (testval varchar2(4000))"),0);
		StringBuilder	testval=new StringBuilder();
		for (int i=0; i<4000; i++) {
			testval.append('C');
		}
		String	teststr=testval.toString();
		pstmt=con.prepareStatement(
			"insert into "+
			"	testtable2 values (:testval)");
		assertTrue((pstmt!=null));
		pstmt.setString(1,teststr);
		assertEquals(pstmt.executeUpdate(),1);
		pstmt.close();
		rs=stmt.executeQuery("select testval from testtable2");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),teststr);
		rs.close();
		cstmt=con.prepareCall("begin :bindval:='"+teststr+"'; end;");
		cstmt.registerOutParameter("bindval",Types.VARCHAR);
		assertFalse(cstmt.execute());
		assertEquals(cstmt.getString("bindval"),teststr);
		cstmt.close();
		assertEquals(stmt.executeUpdate("drop table testtable2"),0);
		System.out.println();


		// negative input bind
		System.out.println("NEGATIVE INPUT BIND:");
		try {
			stmt.executeUpdate("drop table testtable2");
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate(
			"create table "+
			"	testtable2 (testval number)"),0);
		pstmt=con.prepareStatement(
			"insert into "+
			"	testtable2 values (:testval)");
		assertTrue((pstmt!=null));
		pstmt.setInt(1,-1);
		assertEquals(pstmt.executeUpdate(),1);
		pstmt.close();
		rs=stmt.executeQuery("select testval from testtable2");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"-1");
		rs.close();
		assertEquals(stmt.executeUpdate("drop table testtable2"),0);
		System.out.println();


		// drop existing table
		stmt.executeUpdate("drop table testtable");


		// temporary tables
		System.out.println("TEMPORARY TABLES:");
		try {
			assertEquals(stmt.executeUpdate(
				"drop table "+hostname+
				"_temptabledelete"),0);
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate(
			"create global "+
			"	temporary table "+
			hostname+
			"_temptabledelete "+
			"(col1 number) "+
			"on commit delete rows"),0);
		assertEquals(stmt.executeUpdate(
			"insert into "+hostname+
			"_temptabledelete "+
			"values (1)"),1);
		rs=stmt.executeQuery(
			"select count(*) from "+
			hostname+"_temptabledelete");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"1");
		rs.close();
		con.commit();
		rs=stmt.executeQuery(
			"select count(*) from "+
			hostname+"_temptabledelete");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"0");
		rs.close();
		assertEquals(stmt.executeUpdate(
			"drop table "+hostname+
			"_temptabledelete"),0);
		System.out.println();
		try {
			stmt.executeUpdate(
				"truncate table "+hostname+
				"_temptablepreserve");
		} catch (Exception ex) {
		}
		try {
			stmt.executeUpdate(
				"drop table "+hostname+
				"_temptablepreserve");
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate(
			"create global "+
			"	temporary table "+
			hostname+
			"_temptablepreserve "+
			"(col1 number) "+
			"on commit preserve rows"),0);
		assertEquals(stmt.executeUpdate(
			"insert into "+hostname+
			"_temptablepreserve "+
			"values (1)"),1);
		rs=stmt.executeQuery(
			"select count(*) from "+
			hostname+"_temptablepreserve");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"1");
		rs.close();
		con.commit();
		rs=stmt.executeQuery(
			"select count(*) from "+
			hostname+"_temptablepreserve");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"1");
		rs.close();
		stmt.close();
		assertTrue(stmt.isClosed());
		con.close();
		System.out.println();
		con=DriverManager.getConnection(url,user,password);
		assertTrue((con!=null));
		stmt=con.createStatement();
		assertTrue((stmt!=null));
		rs=stmt.executeQuery(
			"select count(*) from "+
			hostname+"_temptablepreserve");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"0");
		rs.close();
		assertEquals(stmt.executeUpdate(
			"truncate table "+hostname+
			"_temptablepreserve"),0);
		Thread.sleep(2000);
		assertEquals(stmt.executeUpdate(
			"drop table "+hostname+
			"_temptablepreserve"),0);
		try {
			stmt.executeQuery(
				"select count(*) from "+
				hostname+
				"_temptablepreserve");
			assertTrue(false);
		} catch (Exception ex) {
			assertTrue(true);
		}
		System.out.println();


		// stored procedures
		System.out.println("STORED PROCEDURES:");
		// return no value
		try {
			stmt.executeUpdate("drop function testproc");
		} catch (Exception ex) {
		}
		try {
			stmt.executeUpdate("drop procedure testproc");
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate(
			"create or replace "+
			"procedure testproc("+
			"	in1 in number, "+
			"	in2 in number, "+
			"	in3 in varchar2) "+
			"is "+
			"begin "+
			"	return; "+
			"end;"),0);
		cstmt=con.prepareCall(
			"begin "+
			"	testproc(:in1,:in2,:in3); "+
			"end;");
		cstmt.setInt("in1",1);
		cstmt.setDouble("in2",1.1);
		cstmt.setString("in3","hello");
		assertFalse(cstmt.execute());
		cstmt.close();
		System.out.println();
		// return single value
		try {
			stmt.executeUpdate("drop function testproc");
		} catch (Exception ex) {
		}
		try {
			stmt.executeUpdate("drop procedure testproc");
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate(
			"create or replace "+
			"function testproc("+
			"	in1 in number, "+
			"	in2 in number, "+
			"	in3 in varchar2) "+
			"return number "+
			"is "+
			"begin "+
			"	return in1; "+
			"end;"),0);
		pstmt=con.prepareStatement(
			"select "+
			"	testproc(:in1,:in2,:in3) "+
			"from dual");
		assertTrue((pstmt!=null));
		pstmt.setInt(1,1);
		pstmt.setDouble(2,1.1);
		pstmt.setString(3,"hello");
		rs=pstmt.executeQuery();
		assertTrue((rs!=null));
		assertTrue(rs.getStatement()==pstmt);
		rs.next();
		assertEquals(rs.getString(1),"1");
		rs.close();
		pstmt.close();
		// oracle jdbc struggles with this case, for some reason
		if (issqlrelay) {
			cstmt=con.prepareCall(
				"begin "+
				"	:out1 :=testproc(:in1,:in2,:in3); "+
				"end;");
			cstmt.setInt("in1",1);
			cstmt.setDouble("in2",1.1);
			cstmt.setString("in3","hello");
			cstmt.registerOutParameter("out1",Types.INTEGER);
			assertFalse(cstmt.execute());
			assertEquals(cstmt.getInt("out1"),1);
			cstmt.close();
		}
		System.out.println();
		// return multiple values
		try {
			stmt.executeUpdate("drop function testproc");
		} catch (Exception ex) {
		}
		try {
			stmt.executeUpdate("drop procedure testproc");
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate(
			"create or replace "+
			"procedure testproc("+
			"	in1 in number, "+
			"	in2 in number, "+
			"	in3 in varchar2, "+
			"	out1 out number, "+
			"	out2 out number, "+
			"	out3 out varchar2) "+
			"is "+
			"begin "+
			"	out1:=in1; "+
			"	out2:=in2; "+
			"	out3:=in3; "+
			"end;"),0);
		cstmt=con.prepareCall(
			"begin "+
			"	testproc(:in1,:in2,:in3,"+
			"		:out1,:out2,:out3); "+
			"end;");
		cstmt.setInt("in1",1);
		cstmt.setDouble("in2",1.1);
		cstmt.setString("in3","hello");
		cstmt.registerOutParameter("out1",Types.INTEGER);
		cstmt.registerOutParameter("out2",Types.DOUBLE);
		cstmt.registerOutParameter("out3",Types.VARCHAR);
		assertFalse(cstmt.execute());
		assertEquals(cstmt.getInt("out1"),1);
		assertEquals(cstmt.getDouble("out2"),1.1);
		assertEquals(cstmt.getString("out3"),"hello");
		cstmt.close();
		try {
			stmt.executeUpdate("drop function testproc");
		} catch (Exception ex) {
		}
		try {
			stmt.executeUpdate("drop procedure testproc");
		} catch (Exception ex) {
		}
		System.out.println();


		// rebinding
		System.out.println("REBINDING:");
		try {
			stmt.executeUpdate("drop procedure testproc");
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate(
			"create or replace "+
			"procedure testproc("+
			"	in1 in number, "+
			"	out1 out number) "+
			"is "+
			"begin "+
			"	out1:=in1; "+
			"	return; "+
			"end;"),0);
		cstmt=con.prepareCall(
			"begin "+
			"	testproc(:in,:out); "+
			"end;");
		cstmt.setInt("in",1);
		cstmt.registerOutParameter("out",Types.INTEGER);
		assertFalse(cstmt.execute());
		assertEquals(cstmt.getInt("out"),1);
		cstmt.setInt("in",2);
		assertFalse(cstmt.execute());
		assertEquals(cstmt.getInt("out"),2);
		cstmt.setInt("in",3);
		assertFalse(cstmt.execute());
		assertEquals(cstmt.getInt("out"),3);
		cstmt.close();
		try {
			stmt.executeUpdate("drop procedure testproc");
		} catch (Exception ex) {
		}
		System.out.println();


		// client info properties
		System.out.println("CLIENT INFO PROPERTIES: ");
		con=DriverManager.getConnection(url,user,password);
		md=con.getMetaData();
		// sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getClientInfoProperties();
			assertTrue((rs!=null));
			rsmd=rs.getMetaData();
			assertTrue((rsmd!=null));
			col=1;
			assertEquals(rsmd.getColumnCount(),4);
			assertEquals(rsmd.getColumnName(col++),"NAME");
			assertEquals(rsmd.getColumnName(col++),"MAX_LEN");
			assertEquals(rsmd.getColumnName(col++),"DEFAULT_VALUE");
			assertEquals(rsmd.getColumnName(col++),"DESCRIPTION");
			//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
			rs.close();
			System.out.println();
		}


		// catalog list
		System.out.println("CATALOG LIST: ");
		stmt=con.createStatement();
		rs=md.getCatalogs();
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),1);
		col=1;
		assertEquals(rsmd.getColumnName(col++),"TABLE_CAT");
		assertFalse(rs.next());
		rs.close();
		System.out.println();


		// schema list
		System.out.println("SCHEMA LIST: ");
		rs=md.getSchemas();
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),2);
		col=1;
		assertEquals(rsmd.getColumnName(col++),"TABLE_SCHEM");
		assertEquals(rsmd.getColumnName(col++),"TABLE_CATALOG");
		found=false;
		while (rs.next()) {
			if (rs.getString("TABLE_SCHEM").
					equalsIgnoreCase(hostname)) {
				found=true;
				break;
			}
		}
		assertTrue(found);
		rs.close();
		System.out.println();


		// table type list
		System.out.println("TABLE TYPE LIST: ");
		rs=md.getTableTypes();
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),1);
		col=1;
		assertEquals(rsmd.getColumnName(col++),"TABLE_TYPE");
		if (issqlrelay) {
			assertTrue(rs.next());
			assertEquals(rs.getString("TABLE_TYPE"),"SYNONYM");
			assertTrue(rs.next());
			assertEquals(rs.getString("TABLE_TYPE"),"TABLE");
			assertTrue(rs.next());
			assertEquals(rs.getString("TABLE_TYPE"),"VIEW");
		}
		rs.close();
		System.out.println();


		// table list
		System.out.println("TABLE LIST: ");
		try {
			stmt.executeUpdate("drop table testtable1");
		} catch (Exception ex) {
		}
		try {
			stmt.executeUpdate("drop table testtable2");
		} catch (Exception ex) {
		}
		try {
			stmt.executeUpdate("drop table testtable3");
		} catch (Exception ex) {
		}
		try {
			stmt.executeUpdate("drop table testtable4");
		} catch (Exception ex) {
		}
		stmt.executeUpdate(
			"create table testtable1 ("+
			"	testnumber number, "+
			"	testchar char(40), "+
			"	testvarchar varchar2(40), "+
			"	testdate date, "+
			"	testlong long, "+
			"	testclob clob, "+
			"	testblob blob)");
		stmt.executeUpdate(
			"create table testtable2 ("+
			"	testnumber number, "+
			"	testchar char(40), "+
			"	testvarchar varchar2(40), "+
			"	testdate date, "+
			"	testlong long, "+
			"	testclob clob, "+
			"	testblob blob)");
		stmt.executeUpdate(
			"create table testtable3 ("+
			"	testnumber number, "+
			"	testchar char(40), "+
			"	testvarchar varchar2(40), "+
			"	testdate date, "+
			"	testlong long, "+
			"	testclob clob, "+
			"	testblob blob)");
		stmt.executeUpdate(
			"create table testtable4 ("+
			"	testnumber number, "+
			"	testchar char(40), "+
			"	testvarchar varchar2(40), "+
			"	testdate date, "+
			"	testlong long, "+
			"	testclob clob, "+
			"	testblob blob)");
		rs=md.getTables(null,null,"%",
			new String[] {"SYNONYM","TABLE","VIEW"});
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		if (issqlrelay) {
                	assertEquals(rsmd.getColumnCount(),10);
		} else {
			// oracle jdbc returns 5 columns
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
		counter=0;
		while (rs.next()) {
			String name=rs.getString("TABLE_NAME");
			if (name.equalsIgnoreCase("TESTTABLE1") ||
				name.equalsIgnoreCase("TESTTABLE2") ||
				name.equalsIgnoreCase("TESTTABLE3") ||
				name.equalsIgnoreCase("TESTTABLE4")) {
				counter++;
			}
		}
		assertEquals(counter,4);
		rs.close();
		stmt.executeUpdate("drop table testtable1");
		stmt.executeUpdate("drop table testtable2");
		stmt.executeUpdate("drop table testtable3");
		stmt.executeUpdate("drop table testtable4");
		System.out.println();


		// super table list
		// neither oracle, nor sqlrelay support this
		System.out.println("SUPER TABLE LIST: ");
		try {
			rs=md.getSuperTables(null,null,"%");
			assertFalse(true);
		} catch (Exception ex) {
			assertTrue(true);
		}
		System.out.println();


		// table privilege list
		System.out.println("TABLE PRIVILEGE LIST: ");
		// sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getTablePrivileges(null,null,"%");
			assertTrue((rs!=null));
			rsmd=rs.getMetaData();
			assertTrue((rsmd!=null));
			col=1;
			assertEquals(rsmd.getColumnCount(),7);
			assertEquals(rsmd.getColumnName(col++),"TABLE_CAT");
			assertEquals(rsmd.getColumnName(col++),"TABLE_SCHEM");
			assertEquals(rsmd.getColumnName(col++),"TABLE_NAME");
			assertEquals(rsmd.getColumnName(col++),"GRANTOR");
			assertEquals(rsmd.getColumnName(col++),"GRANTEE");
			assertEquals(rsmd.getColumnName(col++),"PRIVILEGE");
			assertEquals(rsmd.getColumnName(col++),"IS_GRANTABLE");
			//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
			rs.close();
			System.out.println();
		}


		// type info list
		System.out.println("TYPE INFO LIST: ");
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


		// column list
		System.out.println("COLUMN LIST: ");
		try {
			stmt.executeUpdate("drop table testtable");
		} catch (Exception ex) {
		}
		stmt.executeUpdate(
			"create table testtable ("+
			"	testnumber number, "+
			"	testchar char(40), "+
			"	testvarchar varchar2(40), "+
			"	testdate date, "+
			"	testlong long, "+
			"	testclob clob, "+
			"	testblob blob)");
		rs=md.getColumns(null,null,"TESTTABLE","%");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		col=1;
		assertEquals(rsmd.getColumnCount(),24);
		assertEquals(rsmd.getColumnName(col++),"TABLE_CAT");
		assertEquals(rsmd.getColumnName(col++),"TABLE_SCHEM");
		assertEquals(rsmd.getColumnName(col++),"TABLE_NAME");
		assertEquals(rsmd.getColumnName(col++),"COLUMN_NAME");
		assertEquals(rsmd.getColumnName(col++),"DATA_TYPE");
		assertEquals(rsmd.getColumnName(col++),"TYPE_NAME");
		assertEquals(rsmd.getColumnName(col++),"COLUMN_SIZE");
		assertEquals(rsmd.getColumnName(col++),"BUFFER_LENGTH");
		assertEquals(rsmd.getColumnName(col++),"DECIMAL_DIGITS");
		assertEquals(rsmd.getColumnName(col++),"NUM_PREC_RADIX");
		assertEquals(rsmd.getColumnName(col++),"NULLABLE");
		assertEquals(rsmd.getColumnName(col++),"REMARKS");
		assertEquals(rsmd.getColumnName(col++),"COLUMN_DEF");
		assertEquals(rsmd.getColumnName(col++),"SQL_DATA_TYPE");
		assertEquals(rsmd.getColumnName(col++),"SQL_DATETIME_SUB");
		assertEquals(rsmd.getColumnName(col++),"CHAR_OCTET_LENGTH");
		assertEquals(rsmd.getColumnName(col++),"ORDINAL_POSITION");
		assertEquals(rsmd.getColumnName(col++),"IS_NULLABLE");
		assertEquals(rsmd.getColumnName(col++),"SCOPE_CATALOG");
		assertEquals(rsmd.getColumnName(col++),"SCOPE_SCHEMA");
		assertEquals(rsmd.getColumnName(col++),"SCOPE_TABLE");
		assertEquals(rsmd.getColumnName(col++),"SOURCE_DATA_TYPE");
		assertEquals(rsmd.getColumnName(col++),"IS_AUTOINCREMENT");
		assertEquals(rsmd.getColumnName(col++),"IS_GENERATEDCOLUMN");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTNUMBER");
		assertEquals(rs.getString("TYPE_NAME"),"NUMBER");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTCHAR");
		assertEquals(rs.getString("TYPE_NAME"),"CHAR");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTVARCHAR");
		assertEquals(rs.getString("TYPE_NAME"),"VARCHAR2");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTDATE");
		assertEquals(rs.getString("TYPE_NAME"),"DATE");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTLONG");
		assertEquals(rs.getString("TYPE_NAME"),"LONG");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTCLOB");
		assertEquals(rs.getString("TYPE_NAME"),"CLOB");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTBLOB");
		assertEquals(rs.getString("TYPE_NAME"),"BLOB");
		rs.close();
		stmt.executeUpdate("drop table testtable");
		System.out.println();


		// pseudo column list
		System.out.println("PSEUDO COLUMN LIST: ");
		// sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getPseudoColumns(null,null,"%","%");
			assertTrue((rs!=null));
			rsmd=rs.getMetaData();
			assertTrue((rsmd!=null));
			col=1;
			assertEquals(rsmd.getColumnCount(),12);
			assertEquals(rsmd.getColumnName(col++),
							"TABLE_CAT");
			assertEquals(rsmd.getColumnName(col++),
							"TABLE_SCHEM");
			assertEquals(rsmd.getColumnName(col++),
							"TABLE_NAME");
			assertEquals(rsmd.getColumnName(col++),
							"COLUMN_NAME");
			assertEquals(rsmd.getColumnName(col++),
							"DATA_TYPE");
			assertEquals(rsmd.getColumnName(col++),
							"COLUMN_SIZE");
			assertEquals(rsmd.getColumnName(col++),
							"DECIMAL_DIGITS");
			assertEquals(rsmd.getColumnName(col++),
							"NUM_PREC_RADIX");
			assertEquals(rsmd.getColumnName(col++),
							"COLUMN_USAGE");
			assertEquals(rsmd.getColumnName(col++),
							"REMARKS");
			assertEquals(rsmd.getColumnName(col++),
							"CHAR_OCTET_LENGTH");
			assertEquals(rsmd.getColumnName(col++),
							"IS_NULLABLE");
			//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
			rs.close();
			System.out.println();
		}


		// version column list
		System.out.println("VERSION COLUMN LIST: ");
		// sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getVersionColumns(null,null,"%");
			assertTrue((rs!=null));
			rsmd=rs.getMetaData();
			assertTrue((rsmd!=null));
			col=1;
			assertEquals(rsmd.getColumnCount(),8);
			assertEquals(rsmd.getColumnName(col++),
							"SCOPE");
			assertEquals(rsmd.getColumnName(col++),
							"COLUMN_NAME");
			assertEquals(rsmd.getColumnName(col++),
							"DATA_TYPE");
			assertEquals(rsmd.getColumnName(col++),
							"TYPE_NAME");
			assertEquals(rsmd.getColumnName(col++),
							"COLUMN_SIZE");
			assertEquals(rsmd.getColumnName(col++),
							"BUFFER_LENGTH");
			assertEquals(rsmd.getColumnName(col++),
							"DECIMAL_DIGITS");
			assertEquals(rsmd.getColumnName(col++),
							"PSEUDO_COLUMN");
			//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
			rs.close();
			System.out.println();
		}



		// column privilege list
		System.out.println("COLUMN PRIVILEGE LIST: ");
		// sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getColumnPrivileges(null,null,"%","%");
			assertTrue((rs!=null));
			rsmd=rs.getMetaData();
			assertTrue((rsmd!=null));
			col=1;
			assertEquals(rsmd.getColumnCount(),8);
			assertEquals(rsmd.getColumnName(col++),"TABLE_CAT");
			assertEquals(rsmd.getColumnName(col++),"TABLE_SCHEM");
			assertEquals(rsmd.getColumnName(col++),"TABLE_NAME");
			assertEquals(rsmd.getColumnName(col++),"COLUMN_NAME");
			assertEquals(rsmd.getColumnName(col++),"GRANTOR");
			assertEquals(rsmd.getColumnName(col++),"GRANTEE");
			assertEquals(rsmd.getColumnName(col++),"PRIVILEGE");
			assertEquals(rsmd.getColumnName(col++),"IS_GRANTABLE");
			//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
			rs.close();
			System.out.println();
		}


		// best row identifier list
		System.out.println("BEST ROW IDENTIFIER LIST: ");
		// sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getBestRowIdentifier(null,null,"%",
					DatabaseMetaData.bestRowTemporary,
					true);
			assertTrue((rs!=null));
			rsmd=rs.getMetaData();
			assertTrue((rsmd!=null));
			col=1;
			assertEquals(rsmd.getColumnCount(),8);
			assertEquals(rsmd.getColumnName(col++),
							"SCOPE");
			assertEquals(rsmd.getColumnName(col++),
							"COLUMN_NAME");
			assertEquals(rsmd.getColumnName(col++),
							"DATA_TYPE");
			assertEquals(rsmd.getColumnName(col++),
							"TYPE_NAME");
			assertEquals(rsmd.getColumnName(col++),
							"COLUMN_SIZE");
			assertEquals(rsmd.getColumnName(col++),
							"BUFFER_LENGTH");
			assertEquals(rsmd.getColumnName(col++),
							"DECIMAL_DIGITS");
			assertEquals(rsmd.getColumnName(col++),
							"PSEUDO_COLUMN");
			//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
			rs.close();
			System.out.println();
		}


		// primary key list
		System.out.println("PRIMARY KEY LIST: ");
		try {
			stmt.executeUpdate("drop table testtable");
		} catch (Exception ex) {
		}
		stmt.executeUpdate(
			"create table testtable ("+
			"	col1 number primary key, "+
			"	col2 number)");
		rs=md.getPrimaryKeys(null,null,"TESTTABLE");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		col=1;
		assertEquals(rsmd.getColumnCount(),6);
		assertEquals(rsmd.getColumnName(col++),"TABLE_CAT");
		assertEquals(rsmd.getColumnName(col++),"TABLE_SCHEM");
		assertEquals(rsmd.getColumnName(col++),"TABLE_NAME");
		assertEquals(rsmd.getColumnName(col++),"COLUMN_NAME");
		assertEquals(rsmd.getColumnName(col++),"KEY_SEQ");
		assertEquals(rsmd.getColumnName(col++),"PK_NAME");
		assertTrue(rs.next());
		assertEquals(rs.getString("TABLE_NAME"),"TESTTABLE");
		assertEquals(rs.getString("COLUMN_NAME"),"COL1");
		assertEquals(rs.getString("KEY_SEQ"),"1");
		assertTrue(rs.getString("PK_NAME")!=null &&
				rs.getString("PK_NAME").length()>0);
		assertFalse(rs.next());
		rs.close();
		stmt.executeUpdate("drop table testtable");
		System.out.println();


		// key and index list
		System.out.println("KEY AND INDEX LIST: ");
		try {
			stmt.executeUpdate("drop table testtable");
		} catch (Exception ex) {
		}
		stmt.executeUpdate(
			"create table testtable ("+
			"	col1 number primary key, "+
			"	col2 number)");
		// oracle jdbc throws:
		// ORA-17068: Invalid arguments in call
		if (issqlrelay) {
			rs=md.getIndexInfo(null,null,
						"TESTTABLE",false,true);
			assertTrue((rs!=null));
			rsmd=rs.getMetaData();
			assertTrue((rsmd!=null));
			col=1;
			assertEquals(rsmd.getColumnCount(),13);
			assertEquals(rsmd.getColumnName(col++),
							"TABLE_CAT");
			assertEquals(rsmd.getColumnName(col++),
							"TABLE_SCHEM");
			assertEquals(rsmd.getColumnName(col++),
							"TABLE_NAME");
			assertEquals(rsmd.getColumnName(col++),
							"NON_UNIQUE");
			assertEquals(rsmd.getColumnName(col++),
							"INDEX_QUALIFIER");
			assertEquals(rsmd.getColumnName(col++),
							"INDEX_NAME");
			assertEquals(rsmd.getColumnName(col++),
							"TYPE");
			assertEquals(rsmd.getColumnName(col++),
							"ORDINAL_POSITION");
			assertEquals(rsmd.getColumnName(col++),
							"COLUMN_NAME");
			assertEquals(rsmd.getColumnName(col++),
							"ASC_OR_DESC");
			assertEquals(rsmd.getColumnName(col++),
							"CARDINALITY");
			assertEquals(rsmd.getColumnName(col++),
							"PAGES");
			assertEquals(rsmd.getColumnName(col++),
							"FILTER_CONDITION");
			assertTrue(rs.next());
			assertEquals(rs.getString("TABLE_NAME"),"TESTTABLE");
			assertEquals(rs.getString("NON_UNIQUE"),"0");
			assertEquals(rs.getString("ORDINAL_POSITION"),"1");
			assertEquals(rs.getString("COLUMN_NAME"),"COL1");
			assertEquals(rs.getString("ASC_OR_DESC"),"A");
			assertEquals(rs.getString("TYPE"),"3");
			assertTrue(rs.getString("INDEX_NAME")!=null &&
					rs.getString("INDEX_NAME").length()>0);
			assertFalse(rs.next());
			rs.close();
			System.out.println();
		}
		stmt.executeUpdate("drop table testtable");


		// exported key list
		System.out.println("EXPORTED KEY LIST: ");
		// sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getExportedKeys(null,null,"%");
			assertTrue((rs!=null));
			rsmd=rs.getMetaData();
			assertTrue((rsmd!=null));
			col=1;
			assertEquals(rsmd.getColumnCount(),14);
			assertEquals(rsmd.getColumnName(col++),"PKTABLE_CAT");
			assertEquals(rsmd.getColumnName(col++),"PKTABLE_SCHEM");
			assertEquals(rsmd.getColumnName(col++),"PKTABLE_NAME");
			assertEquals(rsmd.getColumnName(col++),"PKCOLUMN_NAME");
			assertEquals(rsmd.getColumnName(col++),"FKTABLE_CAT");
			assertEquals(rsmd.getColumnName(col++),"FKTABLE_SCHEM");
			assertEquals(rsmd.getColumnName(col++),"FKTABLE_NAME");
			assertEquals(rsmd.getColumnName(col++),"FKCOLUMN_NAME");
			assertEquals(rsmd.getColumnName(col++),"KEY_SEQ");
			assertEquals(rsmd.getColumnName(col++),"UPDATE_RULE");
			assertEquals(rsmd.getColumnName(col++),"DELETE_RULE");
			assertEquals(rsmd.getColumnName(col++),"FK_NAME");
			assertEquals(rsmd.getColumnName(col++),"PK_NAME");
			assertEquals(rsmd.getColumnName(col++),"DEFERRABILITY");
			//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
			rs.close();
			System.out.println();
		}


		// imported key list
		System.out.println("IMPORTED KEY LIST: ");
		// sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getImportedKeys(null,null,"%");
			assertTrue((rs!=null));
			rsmd=rs.getMetaData();
			assertTrue((rsmd!=null));
			col=1;
			assertEquals(rsmd.getColumnCount(),14);
			assertEquals(rsmd.getColumnName(col++),"PKTABLE_CAT");
			assertEquals(rsmd.getColumnName(col++),"PKTABLE_SCHEM");
			assertEquals(rsmd.getColumnName(col++),"PKTABLE_NAME");
			assertEquals(rsmd.getColumnName(col++),"PKCOLUMN_NAME");
			assertEquals(rsmd.getColumnName(col++),"FKTABLE_CAT");
			assertEquals(rsmd.getColumnName(col++),"FKTABLE_SCHEM");
			assertEquals(rsmd.getColumnName(col++),"FKTABLE_NAME");
			assertEquals(rsmd.getColumnName(col++),"FKCOLUMN_NAME");
			assertEquals(rsmd.getColumnName(col++),"KEY_SEQ");
			assertEquals(rsmd.getColumnName(col++),"UPDATE_RULE");
			assertEquals(rsmd.getColumnName(col++),"DELETE_RULE");
			assertEquals(rsmd.getColumnName(col++),"FK_NAME");
			assertEquals(rsmd.getColumnName(col++),"PK_NAME");
			assertEquals(rsmd.getColumnName(col++),"DEFERRABILITY");
			//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
			rs.close();
			System.out.println();
		}


		// cross reference list
		System.out.println("CROSS REFERENCE LIST: ");
		// sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getCrossReference(null,null,"%",null,null,"%");
			assertTrue((rs!=null));
			rsmd=rs.getMetaData();
			assertTrue((rsmd!=null));
			col=1;
			assertEquals(rsmd.getColumnCount(),14);
			assertEquals(rsmd.getColumnName(col++),"PKTABLE_CAT");
			assertEquals(rsmd.getColumnName(col++),"PKTABLE_SCHEM");
			assertEquals(rsmd.getColumnName(col++),"PKTABLE_NAME");
			assertEquals(rsmd.getColumnName(col++),"PKCOLUMN_NAME");
			assertEquals(rsmd.getColumnName(col++),"FKTABLE_CAT");
			assertEquals(rsmd.getColumnName(col++),"FKTABLE_SCHEM");
			assertEquals(rsmd.getColumnName(col++),"FKTABLE_NAME");
			assertEquals(rsmd.getColumnName(col++),"FKCOLUMN_NAME");
			assertEquals(rsmd.getColumnName(col++),"KEY_SEQ");
			assertEquals(rsmd.getColumnName(col++),"UPDATE_RULE");
			assertEquals(rsmd.getColumnName(col++),"DELETE_RULE");
			assertEquals(rsmd.getColumnName(col++),"FK_NAME");
			assertEquals(rsmd.getColumnName(col++),"PK_NAME");
			assertEquals(rsmd.getColumnName(col++),"DEFERRABILITY");
			//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
			rs.close();
			System.out.println();
		}


		// procedure list
		System.out.println("PROCEDURE LIST: ");
		try {
			stmt.executeUpdate("drop procedure testproc1");
		} catch (Exception ex) {
		}
		try {
			stmt.executeUpdate("drop procedure testproc2");
		} catch (Exception ex) {
		}
		try {
			stmt.executeUpdate("drop procedure testproc3");
		} catch (Exception ex) {
		}
		try {
			stmt.executeUpdate("drop procedure testproc4");
		} catch (Exception ex) {
		}
		stmt.executeUpdate(
			"create procedure testproc1("+
			"	in1 in number, "+
			"	in2 in char, "+
			"	in3 in varchar2, "+
			"	in4 in date) as "+
			"begin "+
			"	null; "+
			"end;");
		stmt.executeUpdate(
			"create procedure testproc2("+
			"	in1 in number, "+
			"	in2 in char, "+
			"	in3 in varchar2, "+
			"	in4 in date) as "+
			"begin "+
			"	null; "+
			"end;");
		stmt.executeUpdate(
			"create procedure testproc3("+
			"	in1 in number, "+
			"	in2 in char, "+
			"	in3 in varchar2, "+
			"	in4 in date) as "+
			"begin "+
			"	null; "+
			"end;");
		stmt.executeUpdate(
			"create procedure testproc4("+
			"	in1 in number, "+
			"	in2 in char, "+
			"	in3 in varchar2, "+
			"	in4 in date) as "+
			"begin "+
			"	null; "+
			"end;");
		rs=md.getProcedures(null,null,"%");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		col=1;
		if (issqlrelay) {
			assertEquals(rsmd.getColumnCount(),8);
		} else {
			// oracle jdbc returns 9 columns
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
			// oracle jdbc returns
			// NULL for these column names
			assertEquals(rsmd.getColumnName(col++),"NULL");
			assertEquals(rsmd.getColumnName(col++),"NULL");
			assertEquals(rsmd.getColumnName(col++),"NULL");
		}
		assertEquals(rsmd.getColumnName(col++),"REMARKS");
		assertEquals(rsmd.getColumnName(col++),"PROCEDURE_TYPE");
		if (!issqlrelay) {
			// oracle jdbc returns a 9th column
			assertEquals(rsmd.getColumnName(col++),
						"SPECIFIC_NAME");
		}
		counter=0;
		while (rs.next()) {
			String name=rs.getString("PROCEDURE_NAME");
			if (name.equalsIgnoreCase("TESTPROC1") ||
				name.equalsIgnoreCase("TESTPROC2") ||
				name.equalsIgnoreCase("TESTPROC3") ||
				name.equalsIgnoreCase("TESTPROC4")) {
				counter++;
			}
		}
		assertEquals(counter,4);
		rs.close();
		System.out.println();


		// procedure parameter list
		System.out.println("PROCEDURE PARAMETER LIST: ");
		// oracle jdbc throws:
		// ORA-00904: "ARG"."TYPE_OBJECT_TYPE": invalid identifier
		if (issqlrelay) {
			rs=md.getProcedureColumns(null,null,
						"TESTPROC1","%");
			assertTrue((rs!=null));
			rsmd=rs.getMetaData();
			assertTrue((rsmd!=null));
			col=1;
			assertEquals(rsmd.getColumnCount(),20);
			assertEquals(rsmd.getColumnName(col++),
							"PROCEDURE_CAT");
			assertEquals(rsmd.getColumnName(col++),
							"PROCEDURE_SCHEM");
			assertEquals(rsmd.getColumnName(col++),
							"PROCEDURE_NAME");
			assertEquals(rsmd.getColumnName(col++),
							"COLUMN_NAME");
			assertEquals(rsmd.getColumnName(col++),
							"COLUMN_TYPE");
			assertEquals(rsmd.getColumnName(col++),
							"DATA_TYPE");
			assertEquals(rsmd.getColumnName(col++),
							"TYPE_NAME");
			assertEquals(rsmd.getColumnName(col++),
							"PRECISION");
			assertEquals(rsmd.getColumnName(col++),
							"LENGTH");
			assertEquals(rsmd.getColumnName(col++),
							"SCALE");
			assertEquals(rsmd.getColumnName(col++),
							"RADIX");
			assertEquals(rsmd.getColumnName(col++),
							"NULLABLE");
			assertEquals(rsmd.getColumnName(col++),
							"REMARKS");
			assertEquals(rsmd.getColumnName(col++),
							"COLUMN_DEF");
			assertEquals(rsmd.getColumnName(col++),
							"SQL_DATA_TYPE");
			assertEquals(rsmd.getColumnName(col++),
							"SQL_DATETIME_SUB");
			assertEquals(rsmd.getColumnName(col++),
							"CHAR_OCTET_LENGTH");
			assertEquals(rsmd.getColumnName(col++),
							"ORDINAL_POSITION");
			assertEquals(rsmd.getColumnName(col++),
							"IS_NULLABLE");
			assertEquals(rsmd.getColumnName(col++),
							"SPECIFIC_NAME");
			assertTrue(rs.next());
			assertEquals(rs.getString("COLUMN_NAME"),"IN1");
			assertEquals(rs.getString("TYPE_NAME"),
							"NUMBER");
			assertEquals(rs.getString("ORDINAL_POSITION"),
							"1");
			assertTrue(rs.next());
			assertEquals(rs.getString("COLUMN_NAME"),"IN2");
			assertEquals(rs.getString("TYPE_NAME"),"CHAR");
			assertEquals(rs.getString("ORDINAL_POSITION"),
							"2");
			assertTrue(rs.next());
			assertEquals(rs.getString("COLUMN_NAME"),"IN3");
			assertEquals(rs.getString("TYPE_NAME"),
							"VARCHAR2");
			assertEquals(rs.getString("ORDINAL_POSITION"),
							"3");
			assertTrue(rs.next());
			assertEquals(rs.getString("COLUMN_NAME"),"IN4");
			assertEquals(rs.getString("TYPE_NAME"),"DATE");
			assertEquals(rs.getString("ORDINAL_POSITION"),
							"4");
			rs.close();
			System.out.println();
		}
		try {
			stmt.executeUpdate("drop procedure testproc1");
		} catch (Exception ex) {
		}
		try {
			stmt.executeUpdate("drop procedure testproc2");
		} catch (Exception ex) {
		}
		try {
			stmt.executeUpdate("drop procedure testproc3");
		} catch (Exception ex) {
		}
		try {
			stmt.executeUpdate("drop procedure testproc4");
		} catch (Exception ex) {
		}


		// function list
		System.out.println("FUNCTION LIST: ");
		// sqlrelay doesn't support this yet
		if (!issqlrelay) {
                	rs=md.getFunctions(null,null,"%");
                	assertTrue((rs!=null));
                	rsmd=rs.getMetaData();
                	assertTrue((rsmd!=null));
			col=1;
			if (issqlrelay) {
				assertEquals(rsmd.getColumnCount(),8);
			} else {
				// oracle jdbc returns 6 columns
				assertEquals(rsmd.getColumnCount(),6);
			}
                	assertEquals(rsmd.getColumnName(col++),
							"FUNCTION_CAT");
                	assertEquals(rsmd.getColumnName(col++),
							"FUNCTION_SCHEM");
                	assertEquals(rsmd.getColumnName(col++),
							"FUNCTION_NAME");
			// oracle jdbc doesn't return these columns at all
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
			// oracle jdbc returns this extra column
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


		// function parameter list
		System.out.println("FUNCTION PARAMETER LIST: ");
		// sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getFunctionColumns(null,null,"%","%");
			assertTrue((rs!=null));
			rsmd=rs.getMetaData();
			assertTrue((rsmd!=null));
			col=1;
			if (issqlrelay) {
				assertEquals(rsmd.getColumnCount(),17);
			} else {
				// oracle jdbc returns 23 columns
				assertEquals(rsmd.getColumnCount(),23);
			}
			assertEquals(rsmd.getColumnName(col++),
							"FUNCTION_CAT");
			assertEquals(rsmd.getColumnName(col++),
							"FUNCTION_SCHEM");
			assertEquals(rsmd.getColumnName(col++),
							"FUNCTION_NAME");
			assertEquals(rsmd.getColumnName(col++),
							"COLUMN_NAME");
			assertEquals(rsmd.getColumnName(col++),
							"COLUMN_TYPE");
			assertEquals(rsmd.getColumnName(col++),
							"DATA_TYPE");
			assertEquals(rsmd.getColumnName(col++),
							"TYPE_NAME");
			assertEquals(rsmd.getColumnName(col++),
							"PRECISION");
			assertEquals(rsmd.getColumnName(col++),
							"LENGTH");
			assertEquals(rsmd.getColumnName(col++),
							"SCALE");
			assertEquals(rsmd.getColumnName(col++),
							"RADIX");
			assertEquals(rsmd.getColumnName(col++),
							"NULLABLE");
			assertEquals(rsmd.getColumnName(col++),
							"REMARKS");
			if (!issqlrelay) {
				// oracle jdbc returns these columns too
				assertEquals(rsmd.getColumnName(col++),
							"COLUMN_DEF");
				assertEquals(rsmd.getColumnName(col++),
							"SQL_DATA_TYPE");
				assertEquals(rsmd.getColumnName(col++),
							"SQL_DATETIME_SUB");
			}
			assertEquals(rsmd.getColumnName(col++),
							"CHAR_OCTET_LENGTH");
			assertEquals(rsmd.getColumnName(col++),
							"ORDINAL_POSITION");
			assertEquals(rsmd.getColumnName(col++),
							"IS_NULLABLE");
			assertEquals(rsmd.getColumnName(col++),
							"SPECIFIC_NAME");
			if (!issqlrelay) {
				// oracle jdbc returns these columns too
				assertEquals(rsmd.getColumnName(col++),
							"SEQUENCE");
				assertEquals(rsmd.getColumnName(col++),
							"OVERLOAD");
				assertEquals(rsmd.getColumnName(col++),
							"DEFAULT_VALUE");
			}
			//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
			rs.close();
			System.out.println();
		}


		// UDT list
		System.out.println("UDT LIST: ");
		// sqlrelay doesn't support this yet
		// oracle jdbc throws:
		// ORA-08177: can't serialize access for this transaction
		if (false) {
                	rs=md.getUDTs(null,null,"%",null);
                	assertTrue((rs!=null));
                	rsmd=rs.getMetaData();
                	assertTrue((rsmd!=null));
			col=1;
			if (issqlrelay) {
                		assertEquals(rsmd.getColumnCount(),7);
			} else {
				// oracle jdbc returns 6 columns
                		assertEquals(rsmd.getColumnCount(),6);
			}
                	assertEquals(rsmd.getColumnName(col++),"TYPE_CAT");
                	assertEquals(rsmd.getColumnName(col++),"TYPE_SCHEM");
                	assertEquals(rsmd.getColumnName(col++),"TYPE_NAME");
                	assertEquals(rsmd.getColumnName(col++),"CLASS_NAME");
                	assertEquals(rsmd.getColumnName(col++),"DATA_TYPE");
                	assertEquals(rsmd.getColumnName(col++),"REMARKS");
			// oracle jdbc doesn't return this column at all
			if (issqlrelay) {
                		assertEquals(rsmd.getColumnName(col++),
								"BASE_TYPE");
			}
                	//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
                	rs.close();
                	System.out.println();
		}


		// super type list
		// neither oracle, nor sqlrelay support this
		System.out.println("SUPER TYPE LIST: ");
		try {
			rs=md.getSuperTypes(null,null,"%");
			assertFalse(true);
		} catch (Exception ex) {
			assertTrue(true);
		}
		System.out.println();


		// attribute list
		// neither oracle, nor sqlrelay support this
		System.out.println("ATTRIBUTE LIST: ");
		try {
			rs=md.getAttributes(null,null,"%","%");
			assertFalse(true);
		} catch (Exception ex) {
			assertTrue(true);
		}
		System.out.println();


		// FIXME: need tests for Connection methods...
		// rollback


		// FIXME: need tests for Statement methods...
		// addBatch
                // clearBatch
                // executeBatch
		//
                // getWarnings
                // clearWarning
		//
                // closeOnCompletion
		//
                // setFetchDirection
                // getFetchDirection
		//
                // getMoreResults
		//
                // getResultSetConcurrency
                // getResultSetHoldability
                // getResultSetType
		//
                // isCloseOnCompletion
		//
                // setPoolable
                // isPoolable
		//
                // setCursorName


		// FIXME: need tests for PreparedStatement methods...
                // executeQuery
		//
		// addBatch
                // executeBatch
		//
                // getMetaData
                // getParameterMetaData
		//
                // setAsciiStream
                // setBigDecimal
                // setBinaryStream
                // setBoolean
                // setByte
                // setCharacterStream
                // setDouble
                // setFloat
                // setLong
                // setNCharacterStream
                // setNClob
                // setNString
                // setNull
                // setShort
                // setTime
                // setTimestamp
                // setUnicodeStream
                // setURL


		// FIXME: need tests for CallableStatement methods...
                // getBigDecimal
                // getBlob
                // getBoolean
                // getByte
                // getCharacterStream
		// getClob
                // getFloat
                // getLong
                // getNCharacterStream
                // getNClob
                // getNString
                // getShort
                // getTime
                // getTimestamp
                // getURL
		//
		// set*() covered by PreparedStatement


		// FIXME: need tests for Parameter class...
		// FIXME: need tests for ParameterMetaData class...


		// FIXME: need tests for ResultSet methods...
                // getCursorName
		//
                // getType
                // getConcurrency
                // getHoldability
		//
                // getWarnings
		//
                // getFetchDirection
                // setFetchDirection
		//
                // setFetchSize
		//
                // moveToCurrentRow
		//
                // isClosed
		//
                // clearWarnings
		//
                // findColumn
		//
                // getBoolean
                // getByte
		//
                // getBigDecimal
                // getDouble
                // getFloat
		//
                // getTime


		// FIXME: need tests for ResultSetMetaData methods...
		// getCatalogName
                // getSchemaName
                // getTableName
                // getColumnType
                // getColumnClassName
                // getScale
                // isAutoIncrement
                // isCaseSensitive
                // isCurrency
                // isDefinitelyWritable
                // isNullable
                // isSearchable
                // isSigned
                // isWritable


		// invalid queries
		System.out.println("INVALID QUERIES:");
		try {
			stmt.executeQuery(
				"select "+
				"	* "+
				"from "+
				"	testtable "+
				"order by "+
				"	testnumber");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeQuery(
				"select "+
				"	* "+
				"from "+
				"	testtable "+
				"order by "+
				"	testnumber");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeQuery(
				"select "+
				"	* "+
				"from "+
				"	testtable "+
				"order by "+
				"	testnumber");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeQuery(
				"select "+
				"	* "+
				"from "+
				"	testtable "+
				"order by "+
				"	testnumber");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		System.out.println();
		try {
			stmt.executeUpdate(
				"insert into "+
				"	testtable "+
				"values ("+
				"	1, "+
				"	2, "+
				"	3, "+
				"	4)");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeUpdate(
				"insert into "+
				"	testtable "+
				"values ("+
				"	1, "+
				"	2, "+
				"	3, "+
				"	4)");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeUpdate(
				"insert into "+
				"	testtable "+
				"values ("+
				"	1, "+
				"	2, "+
				"	3, "+
				"	4)");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeUpdate(
				"insert into "+
				"	testtable "+
				"values ("+
				"	1, "+
				"	2, "+
				"	3, "+
				"	4)");
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
