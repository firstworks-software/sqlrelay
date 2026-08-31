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

class firebird extends sqlrtest {
	public static void main(String args[]) throws Exception {

		String classpath=System.getProperty("java.class.path");
		String hostname=InetAddress.getLocalHost().
					getHostName().split("\\.")[0].toLowerCase();
		String driver=null;
		String host=null;
		short port=0;
		String socket=null;
		String user=null;
		String password=null;
		String url=null;
		boolean issqlrelay=false;

		if (classpath.contains("sqlrelayjdbc.jar")) {
			driver="com.firstworks.sql.SQLRelayDriver";
			host="localhost";
			port=9009;
			socket=null;
			url="jdbc:sqlrelay://"+host+":"+port;
			user="testuser";
			password="testpassword";
			issqlrelay=true;
		} else if (classpath.contains("jaybird")) {
			driver="org.firebirdsql.jdbc.FBDriver";
			url="jdbc:firebirdsql://firebird:3050//u02/"+
							hostname+".gdb";
			user="testuser";
			password="testpassword";
		}

		Properties	props=new Properties();
		props.setProperty("user",user);
		props.setProperty("password",password);
		if (issqlrelay) {
			// for JDBC spec compliance
			props.setProperty("AutoCommit","yes");
		}

		Connection		con;
		Connection		secondcon;
		Statement		secondstmt;
		ResultSet		secondrs;
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


		// connect
		System.out.println("CONNECTION:");

		// getConnection
		System.out.println("  getConnection");
		DriverManager.getDrivers();
		Class.forName(driver);
		con=DriverManager.getConnection(url,props);
		assertTrue((con!=null));
		System.out.println();

		// close
		System.out.println("  close");
		assertFalse(con.isClosed());
		con.close();
		assertTrue(con.isClosed());
		con=DriverManager.getConnection(url,props);
		assertTrue((con!=null));
		assertFalse(con.isClosed());
		System.out.println();

		// autocommit
		System.out.println("  autocommit");
		con.setAutoCommit(true);
		assertTrue(con.getAutoCommit());
		con.setAutoCommit(false);
		assertFalse(con.getAutoCommit());
		con.setAutoCommit(true);
		System.out.println();

		// warnings
		System.out.println("  warnings");
		con.clearWarnings();
		System.out.println();


		// database meta data
		System.out.println("DATABASE META DATA:");

		// getMetaData
		System.out.println("  getMetaData");
		md=con.getMetaData();
		assertTrue((md!=null));
		System.out.println();

		// getConnection
		System.out.println("  getConnection");
		assertEquals(md.getConnection(),con);
		System.out.println();

		// allProceduresAreCallable
		System.out.println("  allProceduresAreCallable");
		boolval=md.allProceduresAreCallable();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// allTablesAreSelectable
		System.out.println("  allTablesAreSelectable");
		boolval=md.allTablesAreSelectable();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// autoCommitFailureClosesAllResultSets
		System.out.println("  autoCommitFailureClosesAllResultSets");
		boolval=md.autoCommitFailureClosesAllResultSets();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// dataDefinitionCausesTransactionCommit
		System.out.println("  dataDefinitionCausesTransactionCommit");
		boolval=md.dataDefinitionCausesTransactionCommit();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// dataDefinitionIgnoredInTransactions
		System.out.println("  dataDefinitionIgnoredInTransactions");
		boolval=md.dataDefinitionIgnoredInTransactions();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// deletesAreDetected
		System.out.println("  deletesAreDetected "+
					"(forward only)");
		boolval=md.deletesAreDetected(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("  deletesAreDetected "+
					"(scroll insensitive)");
		boolval=md.deletesAreDetected(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("  deletesAreDetected "+
					"(scroll sensitive)");
		boolval=md.deletesAreDetected(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// doesMaxRowSizeIncludeBlobs
		System.out.println("  doesMaxRowSizeIncludeBlobs");
		boolval=md.doesMaxRowSizeIncludeBlobs();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// generatedKeyAlwaysReturned
		System.out.println("  generatedKeyAlwaysReturned");
		boolval=md.generatedKeyAlwaysReturned();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// getCatalogSeparator
		System.out.println("  getCatalogSeparator");
		stringval=md.getCatalogSeparator();
		System.out.println("    "+stringval);
		if (issqlrelay) {
			// sqlrelay jdbc returns an empty string
			assertTrue(stringval.isEmpty());
		} else {
			// sqlrelay jdbc returns null
			assertTrue(stringval==null);
		}
		System.out.println();

		// getCatalogTerm
		System.out.println("  getCatalogTerm");
		stringval=md.getCatalogTerm();
		System.out.println("    "+stringval);
		assertTrue(stringval==null || stringval.isEmpty());
		System.out.println();

		// getDatabaseMajorVersion
		System.out.println("  getDatabaseMajorVersion");
		intval=md.getDatabaseMajorVersion();
		System.out.println("    "+intval);
		assertTrue(intval>=0);
		System.out.println();

		// getDatabaseMinorVersion
		System.out.println("  getDatabaseMinorVersion");
		intval=md.getDatabaseMinorVersion();
		System.out.println("    "+intval);
		assertTrue(intval>=0);
		System.out.println();

		// getDatabaseProductName
		System.out.println("  getDatabaseProductName");
		stringval=md.getDatabaseProductName();
		System.out.println("    "+stringval);
		if (issqlrelay) {
			assertEquals(stringval,"firebird");
		} else {
			// native product name embeds the server version
			assertTrue(stringval.startsWith("Firebird"));
		}
		System.out.println();

		// getDatabaseProductVersion
		System.out.println("  getDatabaseProductVersion");
		stringval=md.getDatabaseProductVersion();
		System.out.println("    "+stringval);
		// varies by server installation
		assertContainsVersion(stringval);
		System.out.println();

		// getDefaultTransactionIsolation
		System.out.println("  getDefaultTransactionIsolation");
		intval=md.getDefaultTransactionIsolation();
		System.out.println("    "+intval);
		assertEquals(intval,Connection.TRANSACTION_READ_COMMITTED);
		System.out.println();

		// getDriverMajorVersion
		System.out.println("  getDriverMajorVersion");
		intval=md.getDriverMajorVersion();
		System.out.println("    "+intval);
		assertTrue(intval>=0);
		System.out.println();

		// getDriverMinorVersion
		System.out.println("  getDriverMinorVersion");
		intval=md.getDriverMinorVersion();
		System.out.println("    "+intval);
		assertTrue(intval>=0);
		System.out.println();

		// getDriverName
		System.out.println("  getDriverName");
		stringval=md.getDriverName();
		System.out.println("    "+stringval);
		if (issqlrelay) {
			assertEquals(stringval,"SQL Relay JDBC driver");
		} else {
			assertEquals(stringval,"Jaybird JCA/JDBC driver");
		}
		System.out.println();

		// getDriverVersion
		System.out.println("  getDriverVersion");
		stringval=md.getDriverVersion();
		System.out.println("    "+stringval);
		// varies by driver version
		assertContainsVersion(stringval);
		System.out.println();

		// getExtraNameCharacters
		System.out.println("  getExtraNameCharacters");
		stringval=md.getExtraNameCharacters();
		System.out.println("    "+stringval);
		assertEquals(stringval,"$");
		System.out.println();

		// getIdentifierQuoteString
		System.out.println("  getIdentifierQuoteString");
		stringval=md.getIdentifierQuoteString();
		System.out.println("    "+stringval);
		assertEquals(stringval,"\"");
		System.out.println();

		// getJDBCMajorVersion
		System.out.println("  getJDBCMajorVersion");
		intval=md.getJDBCMajorVersion();
		System.out.println("    "+intval);
		assertTrue(intval>=0);
		System.out.println();

		// getJDBCMinorVersion
		System.out.println("  getJDBCMinorVersion");
		intval=md.getJDBCMinorVersion();
		System.out.println("    "+intval);
		assertTrue(intval>=0);
		System.out.println();

		// getMaxBinaryLiteralLength
		System.out.println("  getMaxBinaryLiteralLength");
		intval=md.getMaxBinaryLiteralLength();
		System.out.println("    "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxCatalogNameLength
		System.out.println("  getMaxCatalogNameLength");
		intval=md.getMaxCatalogNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxCharLiteralLength
		System.out.println("  getMaxCharLiteralLength");
		intval=md.getMaxCharLiteralLength();
		System.out.println("    "+intval);
		assertEquals(intval,32765);
		System.out.println();

		// getMaxColumnNameLength
		System.out.println("  getMaxColumnNameLength");
		intval=md.getMaxColumnNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,31);
		System.out.println();

		// getMaxColumnsInGroupBy
		System.out.println("  getMaxColumnsInGroupBy");
		intval=md.getMaxColumnsInGroupBy();
		System.out.println("    "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxColumnsInIndex
		System.out.println("  getMaxColumnsInIndex");
		intval=md.getMaxColumnsInIndex();
		System.out.println("    "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxColumnsInOrderBy
		System.out.println("  getMaxColumnsInOrderBy");
		intval=md.getMaxColumnsInOrderBy();
		System.out.println("    "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxColumnsInSelect
		System.out.println("  getMaxColumnsInSelect");
		intval=md.getMaxColumnsInSelect();
		System.out.println("    "+intval);
		if (issqlrelay) {
			// capped at maxcolumncount by sql relay
			assertEquals(intval,256);
		} else {
			assertEquals(intval,0);
		}
		System.out.println();

		// getMaxColumnsInTable
		System.out.println("  getMaxColumnsInTable");
		intval=md.getMaxColumnsInTable();
		System.out.println("    "+intval);
		assertEquals(intval,32767);
		System.out.println();

		// getMaxConnections
		System.out.println("  getMaxConnections");
		intval=md.getMaxConnections();
		System.out.println("    "+intval);
		if (issqlrelay) {
			// varies by sqlrelay config
			assertTrue(intval>0);
		} else {
			// firebird jdbc returns 0 for this
			assertEquals(intval,0);
		}
		System.out.println();

		// getMaxCursorNameLength
		System.out.println("  getMaxCursorNameLength");
		intval=md.getMaxCursorNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,31);
		System.out.println();

		// getMaxIndexLength
		System.out.println("  getMaxIndexLength");
		intval=md.getMaxIndexLength();
		System.out.println("    "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxProcedureNameLength
		System.out.println("  getMaxProcedureNameLength");
		intval=md.getMaxProcedureNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,31);
		System.out.println();

		// getMaxRowSize
		System.out.println("  getMaxRowSize");
		intval=md.getMaxRowSize();
		System.out.println("    "+intval);
		assertEquals(intval,65531);
		System.out.println();

		// getMaxSchemaNameLength
		System.out.println("  getMaxSchemaNameLength");
		intval=md.getMaxSchemaNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxStatementLength
		System.out.println("  getMaxStatementLength");
		intval=md.getMaxStatementLength();
		System.out.println("    "+intval);
		if (issqlrelay) {
			// firebird caps at 65535 (isc_dsql_prepare's
			// length is an unsigned short), below maxquerysize
			assertEquals(intval,65535);
		} else {
			assertEquals(intval,10485760);
		}
		System.out.println();

		// getMaxStatements
		System.out.println("  getMaxStatements");
		intval=md.getMaxStatements();
		System.out.println("    "+intval);
		if (issqlrelay) {
			// capped at maxcursors by sql relay
			assertEquals(intval,5);
		} else {
			assertEquals(intval,0);
		}
		System.out.println();

		// getMaxTableNameLength
		System.out.println("  getMaxTableNameLength");
		intval=md.getMaxTableNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,31);
		System.out.println();

		// getMaxTablesInSelect
		System.out.println("  getMaxTablesInSelect");
		intval=md.getMaxTablesInSelect();
		System.out.println("    "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxUserNameLength
		System.out.println("  getMaxUserNameLength");
		intval=md.getMaxUserNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,31);
		System.out.println();

		// getNumericFunctions
		System.out.println("  getNumericFunctions");
		stringval=md.getNumericFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"TAN,MOD,LOG,COS,ROUND,SQRT,ASIN,ATAN2,COT,POWER,LOG10,ABS,FLOOR,DEGREES,CEILING,ACOS,RADIANS,PI,SIN,SIGN,EXP,ATAN,TRUNCATE");
		System.out.println();

		// getProcedureTerm
		System.out.println("  getProcedureTerm");
		stringval=md.getProcedureTerm();
		System.out.println("    "+stringval);
		assertEquals(stringval,"PROCEDURE");
		System.out.println();

		// getResultSetHoldability
		System.out.println("  getResultSetHoldability");
		intval=md.getResultSetHoldability();
		System.out.println("    "+intval);
		if (issqlrelay) {
			// sqlrelay doesn't support close cursors at commit
			assertEquals(intval,ResultSet.HOLD_CURSORS_OVER_COMMIT);
		} else {
			assertEquals(intval,ResultSet.CLOSE_CURSORS_AT_COMMIT);
		}
		System.out.println();

		// getRowIdLifetime
		System.out.println("  getRowIdLifetime");
		if (issqlrelay) {
			// firebird jdbc doesn't support this
			RowIdLifetime	rowidlifetimeval=md.getRowIdLifetime();
			System.out.println("  "+rowidlifetimeval);
			assertEquals(rowidlifetimeval,
					RowIdLifetime.ROWID_UNSUPPORTED);
			System.out.println();
		}

		// getSchemaTerm
		System.out.println("  getSchemaTerm");
		stringval=md.getSchemaTerm();
		System.out.println("    "+stringval);
		assertTrue(stringval==null || stringval.isEmpty());
		System.out.println();

		// getSearchStringEscape
		System.out.println("  getSearchStringEscape");
		stringval=md.getSearchStringEscape();
		System.out.println("    "+stringval);
		assertEquals(stringval,"\\");
		System.out.println();

		// getSQLKeywords
		System.out.println("  getSQLKeywords");
		stringval=md.getSQLKeywords();
		System.out.println("    "+stringval);
		assertEquals(stringval,"ADD,ADMIN,BIT_LENGTH,CURRENT_CONNECTION,CURRENT_TRANSACTION,DELETING,GDSCODE,INDEX,INSERTING,LONG,OFFSET,PLAN,POST_EVENT,RDB$DB_KEY,RDB$RECORD_VERSION,RECORD_VERSION,RECREATE,RETURNING_VALUES,ROW_COUNT,SQLCODE,UPDATING,VARIABLE,VIEW,WHILE");
		System.out.println();

		// getSQLStateType
		System.out.println("  getSQLStateType");
		intval=md.getSQLStateType();
		System.out.println("    "+intval);
		assertEquals(intval,DatabaseMetaData.sqlStateSQL);
		System.out.println();

		// getStringFunctions
		System.out.println("  getStringFunctions");
		stringval=md.getStringFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"CHARACTER_LENGTH,LEFT,REPEAT,CONCAT,SUBSTRING,LENGTH,UCASE,CHAR,ASCII,SPACE,POSITION,LCASE,LTRIM,RIGHT,INSERT,CHAR_LENGTH,LOCATE,REPLACE,OCTET_LENGTH,RTRIM");
		System.out.println();

		// getSystemFunctions
		System.out.println("  getSystemFunctions");
		stringval=md.getSystemFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"DATABASE,IFNULL,USER");
		System.out.println();

		// getTimeDateFunctions
		System.out.println("  getTimeDateFunctions");
		stringval=md.getTimeDateFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"DAYOFMONTH,MONTHNAME,MONTH,CURRENT_TIMESTAMP,HOUR,DAYOFYEAR,TIMESTAMPADD,DAYOFWEEK,QUARTER,TIMESTAMPDIFF,YEAR,CURTIME,NOW,DAYNAME,MINUTE,SECOND,CURRENT_DATE,CURRENT_TIME,WEEK,CURDATE,EXTRACT");
		System.out.println();

		// getURL
		System.out.println("  getURL");
		stringval=md.getURL();
		System.out.println("    "+stringval);
		assertEquals(stringval,url);
		System.out.println();

		// getUserName
		System.out.println("  getUserName");
		stringval=md.getUserName();
		System.out.println("    "+stringval);
		assertEquals(stringval,"testuser");
		System.out.println();

		// isCatalogAtStart
		System.out.println("  isCatalogAtStart");
		boolval=md.isCatalogAtStart();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// isReadOnly
		System.out.println("  isReadOnly");
		boolval=md.isReadOnly();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// insertsAreDetected
		System.out.println("  insertsAreDetected "+
					"(forward only)");
		boolval=md.insertsAreDetected(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("  insertsAreDetected "+
					"(scroll insensitive)");
		boolval=md.insertsAreDetected(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("  insertsAreDetected "+
					"(scroll sensitive)");
		boolval=md.insertsAreDetected(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// locatorsUpdateCopy
		System.out.println("  locatorsUpdateCopy");
		boolval=md.locatorsUpdateCopy();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// nullPlusNonNullIsNull
		System.out.println("  nullPlusNonNullIsNull");
		boolval=md.nullPlusNonNullIsNull();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// nullsAreSortedAtEnd
		System.out.println("  nullsAreSortedAtEnd");
		boolval=md.nullsAreSortedAtEnd();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// nullsAreSortedAtStart
		System.out.println("  nullsAreSortedAtStart");
		boolval=md.nullsAreSortedAtStart();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// nullsAreSortedHigh
		System.out.println("  nullsAreSortedHigh");
		boolval=md.nullsAreSortedHigh();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// nullsAreSortedLow
		System.out.println("  nullsAreSortedLow");
		boolval=md.nullsAreSortedLow();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// othersDeletesAreVisible
		System.out.println("  othersDeletesAreVisible "+
					"(forward only)");
		boolval=md.othersDeletesAreVisible(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("  othersDeletesAreVisible "+
					"(scroll insensitive)");
		boolval=md.othersDeletesAreVisible(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("  othersDeletesAreVisible "+
					"(scroll sensitive)");
		boolval=md.othersDeletesAreVisible(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// othersInsertsAreVisible
		System.out.println("  othersInsertsAreVisible "+
					"(forward only)");
		boolval=md.othersInsertsAreVisible(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("  othersInsertsAreVisible "+
					"(scroll insensitive)");
		boolval=md.othersInsertsAreVisible(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("  othersInsertsAreVisible "+
					"(scroll sensitive)");
		boolval=md.othersInsertsAreVisible(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// othersUpdatesAreVisible
		System.out.println("  othersUpdatesAreVisible "+
					"(forward only)");
		boolval=md.othersUpdatesAreVisible(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("  othersUpdatesAreVisible "+
					"(scroll insensitive)");
		boolval=md.othersUpdatesAreVisible(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("  othersUpdatesAreVisible "+
					"(scroll sensitive)");
		boolval=md.othersUpdatesAreVisible(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// ownDeletesAreVisible
		System.out.println("  ownDeletesAreVisible "+
					"(forward only)");
		boolval=md.ownDeletesAreVisible(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("  ownDeletesAreVisible "+
					"(scroll insensitive)");
		boolval=md.ownDeletesAreVisible(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("  ownDeletesAreVisible "+
					"(scroll sensitive)");
		boolval=md.ownDeletesAreVisible(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("    "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		// ownInsertsAreVisible
		System.out.println("  ownInsertsAreVisible "+
					"(forward only)");
		boolval=md.ownInsertsAreVisible(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("  ownInsertsAreVisible "+
					"(scroll insensitive)");
		boolval=md.ownInsertsAreVisible(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("  ownInsertsAreVisible "+
					"(scroll sensitive)");
		boolval=md.ownInsertsAreVisible(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("    "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		// ownUpdatesAreVisible
		System.out.println("  ownUpdatesAreVisible "+
					"(forward only)");
		boolval=md.ownUpdatesAreVisible(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("  ownUpdatesAreVisible "+
					"(scroll insensitive)");
		boolval=md.ownUpdatesAreVisible(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("  ownUpdatesAreVisible "+
					"(scroll sensitive)");
		boolval=md.ownUpdatesAreVisible(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("    "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		// storesLowerCaseIdentifiers
		System.out.println("  storesLowerCaseIdentifiers");
		boolval=md.storesLowerCaseIdentifiers();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// storesLowerCaseQuotedIdentifiers
		System.out.println("  storesLowerCaseQuotedIdentifiers");
		boolval=md.storesLowerCaseQuotedIdentifiers();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// storesMixedCaseIdentifiers
		System.out.println("  storesMixedCaseIdentifiers");
		boolval=md.storesMixedCaseIdentifiers();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// storesMixedCaseQuotedIdentifiers
		System.out.println("  storesMixedCaseQuotedIdentifiers");
		boolval=md.storesMixedCaseQuotedIdentifiers();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// storesUpperCaseIdentifiers
		System.out.println("  storesUpperCaseIdentifiers");
		boolval=md.storesUpperCaseIdentifiers();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// storesUpperCaseQuotedIdentifiers
		System.out.println("  storesUpperCaseQuotedIdentifiers");
		boolval=md.storesUpperCaseQuotedIdentifiers();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsAlterTableWithAddColumn
		System.out.println("  supportsAlterTableWithAddColumn");
		boolval=md.supportsAlterTableWithAddColumn();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsAlterTableWithDropColumn
		System.out.println("  supportsAlterTableWithDropColumn");
		boolval=md.supportsAlterTableWithDropColumn();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsANSI92EntryLevelSQL
		System.out.println("  supportsANSI92EntryLevelSQL");
		boolval=md.supportsANSI92EntryLevelSQL();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsANSI92FullSQL
		System.out.println("  supportsANSI92FullSQL");
		boolval=md.supportsANSI92FullSQL();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsANSI92IntermediateSQL
		System.out.println("  supportsANSI92IntermediateSQL");
		boolval=md.supportsANSI92IntermediateSQL();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsBatchUpdates
		System.out.println("  supportsBatchUpdates");
		boolval=md.supportsBatchUpdates();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsCatalogsInDataManipulation
		System.out.println("  supportsCatalogsInDataManipulation");
		boolval=md.supportsCatalogsInDataManipulation();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsCatalogsInIndexDefinitions
		System.out.println("  supportsCatalogsInIndexDefinitions");
		boolval=md.supportsCatalogsInIndexDefinitions();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsCatalogsInPrivilegeDefinitions
		System.out.println("  supportsCatalogsInPrivilegeDefinitions");
		boolval=md.supportsCatalogsInPrivilegeDefinitions();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsCatalogsInProcedureCalls
		System.out.println("  supportsCatalogsInProcedureCalls");
		boolval=md.supportsCatalogsInProcedureCalls();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsCatalogsInTableDefinitions
		System.out.println("  supportsCatalogsInTableDefinitions");
		boolval=md.supportsCatalogsInTableDefinitions();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsColumnAliasing
		System.out.println("  supportsColumnAliasing");
		boolval=md.supportsColumnAliasing();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsConvert
		System.out.println("  supportsConvert");
		boolval=md.supportsConvert();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsConvert (with types)
		System.out.println("  supportsConvert (with types)");
		boolval=md.supportsConvert(Types.INTEGER,Types.VARCHAR);
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsCoreSQLGrammar
		System.out.println("  supportsCoreSQLGrammar");
		boolval=md.supportsCoreSQLGrammar();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsCorrelatedSubqueries
		System.out.println("  supportsCorrelatedSubqueries");
		boolval=md.supportsCorrelatedSubqueries();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsDataManipulationTransactionsOnly
		System.out.println("  supportsDataManipulationTransactionsOnly");
		boolval=md.supportsDataManipulationTransactionsOnly();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsDifferentTableCorrelationNames
		System.out.println("  supportsDifferentTableCorrelationNames");
		boolval=md.supportsDifferentTableCorrelationNames();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsExpressionsInOrderBy
		System.out.println("  supportsExpressionsInOrderBy");
		boolval=md.supportsExpressionsInOrderBy();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsExtendedSQLGrammar
		System.out.println("  supportsExtendedSQLGrammar");
		boolval=md.supportsExtendedSQLGrammar();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsFullOuterJoins
		System.out.println("  supportsFullOuterJoins");
		boolval=md.supportsFullOuterJoins();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsGetGeneratedKeys
		System.out.println("  supportsGetGeneratedKeys");
		boolval=md.supportsGetGeneratedKeys();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsGroupBy
		System.out.println("  supportsGroupBy");
		boolval=md.supportsGroupBy();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsGroupByBeyondSelect
		System.out.println("  supportsGroupByBeyondSelect");
		boolval=md.supportsGroupByBeyondSelect();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsGroupByUnrelated
		System.out.println("  supportsGroupByUnrelated");
		boolval=md.supportsGroupByUnrelated();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsIntegrityEnhancementFacility
		System.out.println("  supportsIntegrityEnhancementFacility");
		boolval=md.supportsIntegrityEnhancementFacility();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsLikeEscapeClause
		System.out.println("  supportsLikeEscapeClause");
		boolval=md.supportsLikeEscapeClause();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsLimitedOuterJoins
		System.out.println("  supportsLimitedOuterJoins");
		boolval=md.supportsLimitedOuterJoins();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsMinimumSQLGrammar
		System.out.println("  supportsMinimumSQLGrammar");
		boolval=md.supportsMinimumSQLGrammar();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsMixedCaseIdentifiers
		System.out.println("  supportsMixedCaseIdentifiers");
		boolval=md.supportsMixedCaseIdentifiers();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsMixedCaseQuotedIdentifiers
		System.out.println("  supportsMixedCaseQuotedIdentifiers");
		boolval=md.supportsMixedCaseQuotedIdentifiers();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsMultipleOpenResults
		System.out.println("  supportsMultipleOpenResults");
		boolval=md.supportsMultipleOpenResults();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsMultipleResultSets
		System.out.println("  supportsMultipleResultSets");
		boolval=md.supportsMultipleResultSets();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsMultipleTransactions
		System.out.println("  supportsMultipleTransactions");
		boolval=md.supportsMultipleTransactions();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsNamedParameters
		System.out.println("  supportsNamedParameters");
		boolval=md.supportsNamedParameters();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsNonNullableColumns
		System.out.println("  supportsNonNullableColumns");
		boolval=md.supportsNonNullableColumns();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsOpenCursorsAcrossCommit
		System.out.println("  supportsOpenCursorsAcrossCommit");
		boolval=md.supportsOpenCursorsAcrossCommit();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsOpenCursorsAcrossRollback
		System.out.println("  supportsOpenCursorsAcrossRollback");
		boolval=md.supportsOpenCursorsAcrossRollback();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsOpenStatementsAcrossCommit
		System.out.println("  supportsOpenStatementsAcrossCommit");
		boolval=md.supportsOpenStatementsAcrossCommit();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsOpenStatementsAcrossRollback
		System.out.println("  supportsOpenStatementsAcrossRollback");
		boolval=md.supportsOpenStatementsAcrossRollback();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsOrderByUnrelated
		System.out.println("  supportsOrderByUnrelated");
		boolval=md.supportsOrderByUnrelated();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsOuterJoins
		System.out.println("  supportsOuterJoins");
		boolval=md.supportsOuterJoins();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsPositionedDelete
		System.out.println("  supportsPositionedDelete");
		boolval=md.supportsPositionedDelete();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsPositionedUpdate
		System.out.println("  supportsPositionedUpdate");
		boolval=md.supportsPositionedUpdate();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsResultSetConcurrency
		System.out.println("  supportsResultSetConcurrency "+
					"(forward only, read only)");
		boolval=md.supportsResultSetConcurrency(
					ResultSet.TYPE_FORWARD_ONLY,
					ResultSet.CONCUR_READ_ONLY);
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("  supportsResultSetConcurrency "+
					"(forward only, updatable)");
		boolval=md.supportsResultSetConcurrency(
					ResultSet.TYPE_FORWARD_ONLY,
					ResultSet.CONCUR_UPDATABLE);
		System.out.println("    "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support CONCUR_UPDATABLE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		System.out.println("  supportsResultSetConcurrency "+
					"(scroll insensitive, read only)");
		boolval=md.supportsResultSetConcurrency(
					ResultSet.TYPE_SCROLL_INSENSITIVE,
					ResultSet.CONCUR_READ_ONLY);
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("  supportsResultSetConcurrency "+
					"(scroll insensitive, updatable)");
		boolval=md.supportsResultSetConcurrency(
					ResultSet.TYPE_SCROLL_INSENSITIVE,
					ResultSet.CONCUR_UPDATABLE);
		System.out.println("    "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support CONCUR_UPDATABLE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		System.out.println("  supportsResultSetConcurrency "+
					"(scroll sensitive, read only)");
		boolval=md.supportsResultSetConcurrency(
					ResultSet.TYPE_SCROLL_SENSITIVE,
					ResultSet.CONCUR_READ_ONLY);
		System.out.println("    "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		System.out.println("  supportsResultSetConcurrency "+
					"(scroll sensitive, updatable)");
		boolval=md.supportsResultSetConcurrency(
					ResultSet.TYPE_SCROLL_SENSITIVE,
					ResultSet.CONCUR_UPDATABLE);
		System.out.println("    "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		// supportsResultSetHoldability
		System.out.println("  supportsResultSetHoldability "+
					"(hold cursors over commit)");
		boolval=md.supportsResultSetHoldability(
					ResultSet.HOLD_CURSORS_OVER_COMMIT);
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("  supportsResultSetHoldability "+
					"(close cursors at commit)");
		boolval=md.supportsResultSetHoldability(
					ResultSet.CLOSE_CURSORS_AT_COMMIT);
		System.out.println("    "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support CLOSE_CURSORS_AT_COMMIT
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		// supportsResultSetType
		System.out.println("  supportsResultSetType "+
					"(forward only)");
		boolval=md.supportsResultSetType(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("  supportsResultSetType "+
					"(scroll insensitive)");
		boolval=md.supportsResultSetType(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("  supportsResultSetType "+
					"(scroll sensitive)");
		boolval=md.supportsResultSetType(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("    "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		// supportsSavepoints
		System.out.println("  supportsSavepoints");
		boolval=md.supportsSavepoints();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSchemasInDataManipulation
		System.out.println("  supportsSchemasInDataManipulation");
		boolval=md.supportsSchemasInDataManipulation();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsSchemasInIndexDefinitions
		System.out.println("  supportsSchemasInIndexDefinitions");
		boolval=md.supportsSchemasInIndexDefinitions();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsSchemasInPrivilegeDefinitions
		System.out.println("  supportsSchemasInPrivilegeDefinitions");
		boolval=md.supportsSchemasInPrivilegeDefinitions();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsSchemasInProcedureCalls
		System.out.println("  supportsSchemasInProcedureCalls");
		boolval=md.supportsSchemasInProcedureCalls();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsSchemasInTableDefinitions
		System.out.println("  supportsSchemasInTableDefinitions");
		boolval=md.supportsSchemasInTableDefinitions();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsSelectForUpdate
		System.out.println("  supportsSelectForUpdate");
		boolval=md.supportsSelectForUpdate();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsStatementPooling
		System.out.println("  supportsStatementPooling");
		boolval=md.supportsStatementPooling();
		System.out.println("    "+boolval);
		if (issqlrelay) {
			// sqlrelay jdbc supports statement pooling
			assertTrue(boolval);
		} else {
			assertFalse(boolval);
		}
		System.out.println();

		// supportsStoredFunctionsUsingCallSyntax
		System.out.println("  supportsStoredFunctionsUsingCallSyntax");
		boolval=md.supportsStoredFunctionsUsingCallSyntax();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsStoredProcedures
		System.out.println("  supportsStoredProcedures");
		boolval=md.supportsStoredProcedures();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSubqueriesInComparisons
		System.out.println("  supportsSubqueriesInComparisons");
		boolval=md.supportsSubqueriesInComparisons();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSubqueriesInExists
		System.out.println("  supportsSubqueriesInExists");
		boolval=md.supportsSubqueriesInExists();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSubqueriesInIns
		System.out.println("  supportsSubqueriesInIns");
		boolval=md.supportsSubqueriesInIns();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSubqueriesInQuantifieds
		System.out.println("  supportsSubqueriesInQuantifieds");
		boolval=md.supportsSubqueriesInQuantifieds();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsTableCorrelationNames
		System.out.println("  supportsTableCorrelationNames");
		boolval=md.supportsTableCorrelationNames();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsTransactionIsolationLevel
		System.out.println("  supportsTransactionIsolationLevel "+
							"(none)");
		boolval=md.supportsTransactionIsolationLevel(
				Connection.TRANSACTION_NONE);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("  supportsTransactionIsolationLevel "+
							"(read uncommitted)");
		boolval=md.supportsTransactionIsolationLevel(
				Connection.TRANSACTION_READ_UNCOMMITTED);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("  supportsTransactionIsolationLevel "+
							"(read committed)");
		boolval=md.supportsTransactionIsolationLevel(
				Connection.TRANSACTION_READ_COMMITTED);
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("  supportsTransactionIsolationLevel "+
							"(repeatable read)");
		boolval=md.supportsTransactionIsolationLevel(
				Connection.TRANSACTION_REPEATABLE_READ);
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("  supportsTransactionIsolationLevel "+
							"(serializable)");
		boolval=md.supportsTransactionIsolationLevel(
				Connection.TRANSACTION_SERIALIZABLE);
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsTransactions
		System.out.println("  supportsTransactions");
		boolval=md.supportsTransactions();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsUnion
		System.out.println("  supportsUnion");
		boolval=md.supportsUnion();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsUnionAll
		System.out.println("  supportsUnionAll");
		boolval=md.supportsUnionAll();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// updatesAreDetected
		System.out.println("  updatesAreDetected "+
					"(forward only)");
		boolval=md.updatesAreDetected(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("  updatesAreDetected "+
					"(scroll insensitive)");
		boolval=md.updatesAreDetected(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("  updatesAreDetected "+
					"(scroll sensitive)");
		boolval=md.updatesAreDetected(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// usesLocalFilePerTable
		System.out.println("  usesLocalFilePerTable");
		boolval=md.usesLocalFilePerTable();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// usesLocalFiles
		System.out.println("  usesLocalFiles");
		boolval=md.usesLocalFiles();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();


		// statement
		System.out.println("STATEMENT:");
		stmt=con.createStatement();
		assertTrue((stmt!=null));
		con.setAutoCommit(false);
		System.out.println();


		// insert
		System.out.println("INSERT:");
		stmt.executeUpdate("delete from testtable");
		// commit the delete so the table starts this test empty from
		// the perspective of other connections; without this, firebird's
		// MVCC will show secondcon (opened later, in COMMIT AND ROLLBACK)
		// whatever rows were in testtable before this run rather than 0
		con.commit();
		assertFalse(stmt.execute(
			"insert into "+
			"	testtable "+
			"values ("+
			"	1, "+
			"	1, "+
			"	1.5, "+
			"	1.5, "+
			"	1.5, "+
			"	1.5, "+
			"	'2001-01-01', "+
			"	'01:00:00', "+
			"	'char1', "+
			"	'varchar1', "+
			"	NULL, "+
			"	'blob1')"));
		assertEquals(stmt.getUpdateCount(),1);
		stmt.close();
		assertTrue(stmt.isClosed());
		System.out.println();


		// bind by position
		System.out.println("BIND BY POSITION:");
		pstmt=con.prepareStatement(
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
			"	?)");
		assertFalse(pstmt.isClosed());
		for (int i=2; i<=4; i++) {
			pstmt.clearParameters();
			pstmt.setInt(1,i);
			pstmt.setInt(2,i);
			pstmt.setDouble(3,i+0.5);
			pstmt.setDouble(4,i+0.5);
			pstmt.setDouble(5,i+0.5);
			pstmt.setDouble(6,i+0.5);

			cal.set(Calendar.YEAR,2000+i);
			cal.set(Calendar.MONTH,Calendar.JANUARY);
			cal.set(Calendar.DAY_OF_MONTH,1);
			cal.set(Calendar.HOUR_OF_DAY,0);
			cal.set(Calendar.MINUTE,0);
			cal.set(Calendar.SECOND,0);
			cal.set(Calendar.MILLISECOND,0);
			pstmt.setDate(7,new java.sql.Date(
						cal.getTimeInMillis()));
			pstmt.setString(8,"0"+i+":00:00");
			pstmt.setString(9,"char"+i);
			pstmt.setString(10,"varchar"+i);
			pstmt.setNull(11,java.sql.Types.TIMESTAMP);
			pstmt.setBytes(12,(new String("blob"+i)).
					getBytes(StandardCharsets.UTF_8));
			assertEquals(pstmt.executeUpdate(),1);
			System.out.println();
		}
		pstmt.close();
		assertTrue(pstmt.isClosed());
		System.out.println();


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
			"	testinteger"));
		rs=stmt.getResultSet();
		assertTrue((rs!=null));
		System.out.println();

		// getMetaData
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		System.out.println();


		// column count
		System.out.println("COLUMN COUNT:");
		assertEquals(rsmd.getColumnCount(),12);
		System.out.println();


		// column names
		System.out.println("COLUMN NAMES:");
		assertEquals(rsmd.getColumnName(1),"TESTINTEGER");
		assertEquals(rsmd.getColumnName(2),"TESTSMALLINT");
		assertEquals(rsmd.getColumnName(3),"TESTDECIMAL");
		assertEquals(rsmd.getColumnName(4),"TESTNUMERIC");
		assertEquals(rsmd.getColumnName(5),"TESTFLOAT");
		assertEquals(rsmd.getColumnName(6),"TESTDOUBLE");
		assertEquals(rsmd.getColumnName(7),"TESTDATE");
		assertEquals(rsmd.getColumnName(8),"TESTTIME");
		assertEquals(rsmd.getColumnName(9),"TESTCHAR");
		assertEquals(rsmd.getColumnName(10),"TESTVARCHAR");
		assertEquals(rsmd.getColumnName(11),"TESTTIMESTAMP");
		assertEquals(rsmd.getColumnName(12),"TESTBLOB");
		System.out.println();


		// column labels
		System.out.println("COLUMN LABELS:");
		assertEquals(rsmd.getColumnLabel(1),"TESTINTEGER");
		assertEquals(rsmd.getColumnLabel(2),"TESTSMALLINT");
		assertEquals(rsmd.getColumnLabel(3),"TESTDECIMAL");
		assertEquals(rsmd.getColumnLabel(4),"TESTNUMERIC");
		assertEquals(rsmd.getColumnLabel(5),"TESTFLOAT");
		assertEquals(rsmd.getColumnLabel(6),"TESTDOUBLE");
		assertEquals(rsmd.getColumnLabel(7),"TESTDATE");
		assertEquals(rsmd.getColumnLabel(8),"TESTTIME");
		assertEquals(rsmd.getColumnLabel(9),"TESTCHAR");
		assertEquals(rsmd.getColumnLabel(10),"TESTVARCHAR");
		assertEquals(rsmd.getColumnLabel(11),"TESTTIMESTAMP");
		assertEquals(rsmd.getColumnLabel(12),"TESTBLOB");
		System.out.println();


		// column types
		System.out.println("COLUMN TYPES:");
		assertEquals(rsmd.getColumnTypeName(1),"INTEGER");
		assertEquals(rsmd.getColumnTypeName(2),"SMALLINT");
		assertEquals(rsmd.getColumnTypeName(3),"DECIMAL");
		assertEquals(rsmd.getColumnTypeName(4),"NUMERIC");
		assertEquals(rsmd.getColumnTypeName(5),"FLOAT");
		assertEquals(rsmd.getColumnTypeName(6),"DOUBLE PRECISION");
		assertEquals(rsmd.getColumnTypeName(7),"DATE");
		assertEquals(rsmd.getColumnTypeName(8),"TIME");
		assertEquals(rsmd.getColumnTypeName(9),"CHAR");
		assertEquals(rsmd.getColumnTypeName(10),"VARCHAR");
		assertEquals(rsmd.getColumnTypeName(11),"TIMESTAMP");
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(12),"BLOB");
		} else {
			assertEquals(rsmd.getColumnTypeName(12),"BLOB SUB_TYPE 0");
		}
		System.out.println();


		// column length
		System.out.println("COLUMN LENGTH:");
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(1),11);
		} else {
			assertEquals(rsmd.getPrecision(1),10);
		}
		assertEquals(rsmd.getPrecision(2),5);
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(3),16);
		} else {
			assertEquals(rsmd.getPrecision(3),10);
		}
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(4),16);
		} else {
			assertEquals(rsmd.getPrecision(4),10);
		}
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(5),0);
		} else {
			assertEquals(rsmd.getPrecision(5),7);
		}
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(6),0);
		} else {
			assertEquals(rsmd.getPrecision(6),15);
		}
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(7),7);
		} else {
			assertEquals(rsmd.getPrecision(7),10);
		}
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(8),7);
		} else {
			assertEquals(rsmd.getPrecision(8),8);
		}
		assertEquals(rsmd.getPrecision(9),50);
		assertEquals(rsmd.getPrecision(10),50);
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(11),7);
		} else {
			assertEquals(rsmd.getPrecision(11),19);
		}
		// sqlrelay JDBC driver returns -1 for blob precision
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(12),-1);
		} else {
			assertEquals(rsmd.getPrecision(12),0);
		}
		System.out.println();


		// longest column
		System.out.println("LONGEST COLUMN:");
		assertTrue(rsmd.getColumnDisplaySize(1)>0);
		assertTrue(rsmd.getColumnDisplaySize(2)>0);
		assertTrue(rsmd.getColumnDisplaySize(3)>0);
		assertTrue(rsmd.getColumnDisplaySize(4)>0);
		assertTrue(rsmd.getColumnDisplaySize(5)>0);
		assertTrue(rsmd.getColumnDisplaySize(6)>0);
		assertTrue(rsmd.getColumnDisplaySize(7)>0);
		assertTrue(rsmd.getColumnDisplaySize(8)>0);
		assertTrue(rsmd.getColumnDisplaySize(9)>0);
		assertTrue(rsmd.getColumnDisplaySize(10)>0);
		assertTrue(rsmd.getColumnDisplaySize(11)>=0);
		assertTrue(rsmd.getColumnDisplaySize(12)>=0);
		System.out.println();


		// fields by index
		System.out.println("FIELDS BY INDEX:");
		for (int i=1; i<=4; i++) {
			assertTrue(rs.next());

			// integer as short
			System.out.println("  row "+i+" - integer as short");
			assertEquals(rs.getShort(1),(short)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// integer as int
			System.out.println("  row "+i+" - integer as int");
			assertEquals(rs.getInt(1),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// integer as long
			System.out.println("  row "+i+" - integer as long");
			assertEquals(rs.getLong(1),(long)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// smallint
			System.out.println("  row "+i+" - smallint");
			assertEquals(rs.getInt(2),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// decimal
			System.out.println("  row "+i+" - decimal");
			assertEquals(rs.getString(3),i+".50");
			assertFalse(rs.wasNull());
			System.out.println();

			// numeric
			System.out.println("  row "+i+" - numeric");
			assertEquals(rs.getString(4),i+".50");
			assertFalse(rs.wasNull());
			System.out.println();

			// float
			System.out.println("  row "+i+" - float");
			assertTrue(rs.getString(5)!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// double
			System.out.println("  row "+i+" - double");
			assertTrue(rs.getString(6)!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// date
			System.out.println("  row "+i+" - date");
			datevar=rs.getDate(7);
			cal.setTime(datevar);
			assertEquals(cal.get(Calendar.YEAR),2000+i);
			assertEquals(cal.get(Calendar.MONTH),Calendar.JANUARY);
			assertEquals(cal.get(Calendar.DAY_OF_MONTH),1);
			assertFalse(rs.wasNull());
			System.out.println();

			// time
			System.out.println("  row "+i+" - time");
			assertEquals(rs.getString(8),"0"+i+":00:00");
			assertFalse(rs.wasNull());
			System.out.println();

			// char as string
			System.out.println("  row "+i+" - char as string");
			assertEquals(rs.getString(9),"char"+i+
					"                                             ");
			assertFalse(rs.wasNull());
			System.out.println();

			// varchar as string
			System.out.println("  row "+i+" - varchar as string");
			assertEquals(rs.getString(10),"varchar"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob as string
			System.out.println("  row "+i+" - blob as string");
			assertEquals(rs.getString(12),"blob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob as clob
			System.out.println("  row "+i+" - blob as clob");
			clob=rs.getClob(12);
			{
				java.io.Reader r=clob.getCharacterStream();
				StringBuilder sb=new StringBuilder();
				int ch;
				while ((ch=r.read())!=-1) {
					sb.append((char)ch);
				}
				assertEquals(sb.toString(),"blob"+i);
			}
			assertFalse(rs.wasNull());
			System.out.println();

			// blob as bytes
			System.out.println("  row "+i+" - blob as bytes");
			assertEquals(new String(
					rs.getBytes(12),"UTF-8"),
					"blob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob as binary stream
			System.out.println("  row "+i+" - blob as binary stream");
			assertEquals(new String(streamToBytes(rs.getBinaryStream(12)),"UTF-8"),
						"blob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

		}
		rs.close();
		assertTrue(rs.isClosed());
		System.out.println();

		System.out.println();


		// fields by name
		System.out.println("FIELDS BY NAME:");
		rs=stmt.executeQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testinteger");
		assertTrue((rs!=null));
		System.out.println();
		for (int i=1; i<=4; i++) {
			assertTrue(rs.next());

			// integer as short
			System.out.println("  row "+i+" - integer as short");
			assertEquals(rs.getShort("TESTINTEGER"),(short)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// integer as int
			System.out.println("  row "+i+" - integer as int");
			assertEquals(rs.getInt("TESTINTEGER"),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// integer as long
			System.out.println("  row "+i+" - integer as long");
			assertEquals(rs.getLong("TESTINTEGER"),(long)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// smallint
			System.out.println("  row "+i+" - smallint");
			assertEquals(rs.getInt("TESTSMALLINT"),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// decimal
			System.out.println("  row "+i+" - decimal");
			assertEquals(rs.getString("TESTDECIMAL"),i+".50");
			assertFalse(rs.wasNull());
			System.out.println();

			// numeric
			System.out.println("  row "+i+" - numeric");
			assertEquals(rs.getString("TESTNUMERIC"),i+".50");
			assertFalse(rs.wasNull());
			System.out.println();

			// float
			System.out.println("  row "+i+" - float");
			assertTrue(rs.getString("TESTFLOAT")!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// double
			System.out.println("  row "+i+" - double");
			assertTrue(rs.getString("TESTDOUBLE")!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// date
			System.out.println("  row "+i+" - date");
			datevar=rs.getDate("TESTDATE");
			cal.setTime(datevar);
			assertEquals(cal.get(Calendar.YEAR),2000+i);
			assertEquals(cal.get(Calendar.MONTH),Calendar.JANUARY);
			assertEquals(cal.get(Calendar.DAY_OF_MONTH),1);
			assertFalse(rs.wasNull());
			System.out.println();

			// time
			System.out.println("  row "+i+" - time");
			assertEquals(rs.getString("TESTTIME"),"0"+i+":00:00");
			assertFalse(rs.wasNull());
			System.out.println();

			// char as string
			System.out.println("  row "+i+" - char as string");
			assertEquals(rs.getString("TESTCHAR"),"char"+i+
					"                                             ");
			assertFalse(rs.wasNull());
			System.out.println();

			// varchar as string
			System.out.println("  row "+i+" - varchar as string");
			assertEquals(rs.getString("TESTVARCHAR"),"varchar"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob as string
			System.out.println("  row "+i+" - blob as string");
			assertEquals(rs.getString("TESTBLOB"),"blob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob as clob
			System.out.println("  row "+i+" - blob as clob");
			clob=rs.getClob("TESTBLOB");
			{
				java.io.Reader r=clob.getCharacterStream();
				StringBuilder sb=new StringBuilder();
				int ch;
				while ((ch=r.read())!=-1) {
					sb.append((char)ch);
				}
				assertEquals(sb.toString(),"blob"+i);
			}
			assertFalse(rs.wasNull());
			System.out.println();

			// blob as bytes
			System.out.println("  row "+i+" - blob as bytes");
			assertEquals(new String(
					rs.getBytes("TESTBLOB"),"UTF-8"),
					"blob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob as binary stream
			System.out.println("  row "+i+" - blob as binary stream");
			assertEquals(new String(streamToBytes(rs.getBinaryStream("TESTBLOB")),"UTF-8"),
						"blob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

		}


		// row count
		System.out.println("ROW COUNT:");
		assertEquals(rs.getRow(),4);
		rs.close();
		stmt.close();
		assertTrue(stmt.isClosed());
		System.out.println();


		// fetch size 0
		System.out.println("FETCH SIZE 0:");
		stmt=con.createStatement(
				ResultSet.TYPE_SCROLL_INSENSITIVE,
				ResultSet.CONCUR_READ_ONLY);
		if (issqlrelay) {
			assertEquals(stmt.getFetchSize(),0);
		} else {
			assertTrue(stmt.getFetchSize()>=0);
		}
		rs=stmt.executeQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testinteger");
		if (issqlrelay) {
			assertEquals(stmt.getFetchSize(),0);
		} else {
			assertTrue(stmt.getFetchSize()>=0);
		}
		System.out.println();

		// jump around wildly
		rs.afterLast();
		assertTrue(rs.isAfterLast());
		assertFalse(rs.next());

		rs.beforeFirst();
		assertTrue(rs.isBeforeFirst());
		assertFalse(rs.previous());

		assertTrue(rs.last());
		assertTrue(rs.isLast());

		assertTrue(rs.first());
		assertTrue(rs.isFirst());

		assertTrue(rs.absolute(3));
		assertEquals(rs.getInt(1),3);
		assertFalse(rs.isBeforeFirst());
		assertFalse(rs.isFirst());
		assertFalse(rs.isLast());
		assertFalse(rs.isAfterLast());

		assertTrue(rs.relative(1));
		assertEquals(rs.getInt(1),4);
		assertFalse(rs.isBeforeFirst());
		assertFalse(rs.isFirst());
		assertTrue(rs.isLast());
		assertFalse(rs.isAfterLast());

		assertTrue(rs.relative(-2));
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
		for (int row=1; row<=3; row++) {
			assertEquals(rs.getInt(1),row);
			assertTrue(rs.next());
		}
		System.out.println();
		assertEquals(rs.getInt(1),4);
		assertTrue(rs.isLast());
		System.out.println();

		// move backwards to the first row
		for (int row=4; row>=2; row--) {
			assertEquals(rs.getInt(1),row);
			assertTrue(rs.previous());
		}
		System.out.println();
		assertEquals(rs.getInt(1),1);
		assertTrue(rs.isFirst());
		System.out.println();

		// move forwards to the last row again
		for (int row=1; row<=3; row++) {
			assertEquals(rs.getInt(1),row);
			assertTrue(rs.next());
		}
		System.out.println();
		assertEquals(rs.getInt(1),4);
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
			"	testinteger");
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
			if (issqlrelay) {
				// sqlrelay returns null when
				// outside of the window
				assertEquals(rs.getString(1),null);
			} else {
				// jaybird can move the window
				assertEquals(rs.getInt(1),row);
			}

			// move forward back into the window
			assertTrue(rs.next());
			row++;
			assertEquals(rs.getInt(1),row);

			// move forward to end of the window
			assertTrue(rs.next());
			row++;
			assertEquals(rs.getInt(1),row);

			System.out.println();

		} while (row<4);

		if (issqlrelay) {
			// sqlrelay jdbc doesn't supported isLast()
			// when fetch size is non-zero
			try {
				rs.isLast();
				assertTrue(false);
			} catch (Exception ex) {
				assertTrue(true);
			}
		} else {
			// jaybird does support isLast()
			// when fetch size is non-zero
			assertTrue(rs.isLast());
		}

		// move past the end of the result set
		assertFalse(rs.next());
		assertTrue(rs.isAfterLast());
		assertFalse(rs.next());

		rs.close();
		stmt.setFetchSize(0);
		if (issqlrelay) {
			assertEquals(stmt.getFetchSize(),0);
		} else {
			assertTrue(stmt.getFetchSize()>=0);
		}

		System.out.println();


		// max rows
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
			"	testinteger");
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
		stmt.close();
		System.out.println();


		// commit and rollback
		System.out.println("COMMIT AND ROLLBACK:");

		// open a second connection to verify cross-connection
		// visibility of commits/rollbacks
		secondcon=DriverManager.getConnection(url,props);
		assertTrue((secondcon!=null));
		assertFalse(secondcon.isClosed());
		secondstmt=secondcon.createStatement();

		// from secondcon: row count should be 0 (con's 4 inserts
		// haven't been committed yet)
		secondrs=secondstmt.executeQuery(
				"select count(*) from testtable");
		assertTrue(secondrs.next());
		assertEquals(secondrs.getInt(1),0);
		secondrs.close();

		// commit on con
		con.commit();

		// from secondcon: row count should be 4
		secondrs=secondstmt.executeQuery(
				"select count(*) from testtable");
		assertTrue(secondrs.next());
		assertEquals(secondrs.getInt(1),4);
		secondrs.close();

		// begin new tx
		// (since autocommit was set off earlier, the commit
		// implicitly started another transaction, so we don't
		// actually need to do anything here)

		// insert another row on con
		stmt=con.createStatement();
		assertEquals(stmt.executeUpdate(
			"insert into "+
			"	testtable "+
			"values ("+
			"	10, "+
			"	10, "+
			"	10.50, "+
			"	10.50, "+
			"	10.50, "+
			"	10.50, "+
			"	'2010-01-01', "+
			"	'10:00:00', "+
			"	'char10', "+
			"	'varchar10', "+
			"	NULL, "+
			"	'blob10')"),1);

		// rollback on con
		con.rollback();

		// from secondcon: row count should still be 4
		secondrs=secondstmt.executeQuery(
				"select count(*) from testtable");
		assertTrue(secondrs.next());
		assertEquals(secondrs.getInt(1),4);
		secondrs.close();

		// switch con to autocommit on; the next insert is
		// auto-committed
		con.setAutoCommit(true);

		// insert another row on con
		assertEquals(stmt.executeUpdate(
			"insert into "+
			"	testtable "+
			"values ("+
			"	10, "+
			"	10, "+
			"	10.50, "+
			"	10.50, "+
			"	10.50, "+
			"	10.50, "+
			"	'2010-01-01', "+
			"	'10:00:00', "+
			"	'char10', "+
			"	'varchar10', "+
			"	NULL, "+
			"	'blob10')"),1);

		// from secondcon: row count should be 5
		secondrs=secondstmt.executeQuery(
				"select count(*) from testtable");
		assertTrue(secondrs.next());
		assertEquals(secondrs.getInt(1),5);
		secondrs.close();

		// clean up secondcon
		secondstmt.close();
		secondcon.close();

		// restore con's autocommit to off and clear the table.
		// autocommit is off now, so the delete has to be committed
		// here - left open, its row locks block every later write to
		// testtable, and firebird waits for them forever
		con.setAutoCommit(false);
		stmt.executeUpdate("delete from testtable");
		con.commit();
		System.out.println();


		// output bind by position
		System.out.println("OUTPUT BIND BY POSITION:");
		cstmt=con.prepareCall(
			"execute procedure testproc ?, ?, ?, ?");
		cstmt.setInt(1,1);
		cstmt.setDouble(2,1.5);
		cstmt.setString(3,"hello");
		cstmt.setBytes(4,"blob".getBytes(StandardCharsets.UTF_8));
		cstmt.registerOutParameter(1,Types.INTEGER);
		cstmt.registerOutParameter(2,Types.DOUBLE);
		cstmt.registerOutParameter(3,Types.VARCHAR);
		cstmt.registerOutParameter(4,Types.BLOB);
		if (issqlrelay) {
			assertFalse(cstmt.execute());
		} else {
			// firebird jdbc returns the output binds as
			// a result set, so execute() returns true
			assertTrue(cstmt.execute());
		}
		assertEquals(cstmt.getInt(1),1);
		assertEquals(cstmt.getString(3).trim(),"hello");
		cstmt.close();
		System.out.println();


		// null and empty clobs and blobs
		System.out.println("NULL AND EMPTY CLOBS AND BLOBS:");
		stmt=con.createStatement();
		stmt.executeUpdate(
			"insert into "+
			"	testtable "+
			"values ("+
			"	1, "+
			"	1, "+
			"	1.5, "+
			"	1.5, "+
			"	1.5, "+
			"	1.5, "+
			"	'2001-01-01', "+
			"	'01:00:00', "+
			"	'char1', "+
			"	'varchar1', "+
			"	NULL, "+
			"	'blob1')");
		rs=stmt.executeQuery(
			"select testblob from testtable "+
			"where testtimestamp is null "+
			"order by testinteger");
		assertTrue((rs!=null));
		// row 1 has blob1 (non-null)
		assertTrue(rs.next());
		assertTrue(rs.getString(1)!=null);
		assertFalse(rs.wasNull());
		rs.close();
		stmt.close();
		System.out.println();


		// long varchar
		System.out.println("LONG VARCHAR:");
		stmt=con.createStatement();
		StringBuilder	sb=new StringBuilder();
		for (int i=0; i<50; i++) {
			sb.append('C');
		}
		String	str=sb.toString();
		stmt.executeUpdate(
			"update testtable "+
			"set testvarchar='"+str+"' "+
			"where testinteger=1");
		rs=stmt.executeQuery(
			"select testvarchar from testtable "+
			"where testinteger=1");
		assertTrue((rs!=null));
		rs.next();
		stringval=rs.getString(1);
		assertEquals(stringval.length(),50);
		assertEquals(stringval,str);
		rs.close();
		stmt.setMaxFieldSize(25);
		assertEquals(stmt.getMaxFieldSize(),25);
		rs=stmt.executeQuery(
			"select testvarchar from testtable "+
			"where testinteger=1");
		assertTrue((rs!=null));
		rs.next();
		stringval=rs.getString(1);
		if (issqlrelay) {
			assertEquals(stringval.length(),25);
			assertEquals(stringval,str.substring(0,25));
		} else {
			// firebird jdbc doesn't support setMaxFieldSize
			assertEquals(stringval.length(),50);
			assertEquals(stringval,str);
		}
		rs.close();
		stmt.setMaxFieldSize(0);
		assertEquals(stmt.getMaxFieldSize(),0);
		// restore original value
		stmt.executeUpdate(
			"update testtable "+
			"set testvarchar='varchar1' "+
			"where testinteger=1");
		stmt.close();
		System.out.println();


		// long clob
		System.out.println("LONG CLOB:");
		stmt=con.createStatement();
		StringBuilder	clobval=new StringBuilder();
		for (int i=0; i<200; i++) {
			clobval.append('C');
		}
		String	clobstr=clobval.toString();
		stmt.executeUpdate(
			"update testtable "+
			"set testblob='"+clobstr+"' "+
			"where testinteger=1");
		rs=stmt.executeQuery(
			"select testblob from testtable "+
			"where testinteger=1");
		assertTrue((rs!=null));
		rs.next();
		clob=rs.getClob(1);
		{
			java.io.Reader r=clob.getCharacterStream();
			StringBuilder csb=new StringBuilder();
			int ch;
			while ((ch=r.read())!=-1) {
				csb.append((char)ch);
			}
			String clobresult=csb.toString();
			assertEquals(clobresult.length(),200);
			assertEquals(clobresult,clobstr);
		}
		rs.close();
		// restore original value
		stmt.executeUpdate(
			"update testtable "+
			"set testblob='blob1' "+
			"where testinteger=1");
		stmt.close();
		System.out.println();


		// long output bind
		System.out.println("LONG OUTPUT BIND:");
		{
			StringBuilder	testval=new StringBuilder();
			for (int i=0; i<20; i++) {
				testval.append('C');
			}
			String	teststr=testval.toString();
			cstmt=con.prepareCall(
				"execute procedure testproc ?, ?, ?, ?");
			cstmt.setInt(1,1);
			cstmt.setDouble(2,1.5);
			cstmt.setString(3,teststr);
			cstmt.setBytes(4,"blob".getBytes(
						StandardCharsets.UTF_8));
			cstmt.registerOutParameter(1,Types.INTEGER);
			cstmt.registerOutParameter(2,Types.DOUBLE);
			cstmt.registerOutParameter(3,Types.VARCHAR);
			cstmt.registerOutParameter(4,Types.BLOB);
			if (issqlrelay) {
				assertFalse(cstmt.execute());
			} else {
				// firebird jdbc returns the output binds as
				// a result set, so execute() returns true
				assertTrue(cstmt.execute());
			}
			assertEquals(cstmt.getString(3).trim(),teststr);
			cstmt.close();
		}
		System.out.println();


		// negative input bind
		System.out.println("NEGATIVE INPUT BIND:");
		pstmt=con.prepareStatement(
			"select testinteger from testtable "+
			"where testinteger=?");
		assertTrue((pstmt!=null));
		pstmt.setInt(1,-1);
		rs=pstmt.executeQuery();
		assertTrue((rs!=null));
		assertFalse(rs.next());
		rs.close();
		pstmt.close();
		pstmt=con.prepareStatement(
			"select testinteger from testtable "+
			"where testinteger=?");
		pstmt.setInt(1,1);
		rs=pstmt.executeQuery();
		assertTrue((rs!=null));
		assertTrue(rs.next());
		assertEquals(rs.getInt(1),1);
		rs.close();
		pstmt.close();
		System.out.println();


		// stored procedures
		System.out.println("STORED PROCEDURES:");
		// return values (procedure pre-created by setup script)
		pstmt=con.prepareStatement("select * from testproc(?,?,?,?)");
		assertTrue((pstmt!=null));
		pstmt.setInt(1,1);
		pstmt.setDouble(2,1.5);
		pstmt.setString(3,"hello");
		pstmt.setBytes(4,(new String("blob")).
				getBytes(StandardCharsets.UTF_8));
		rs=pstmt.executeQuery();
		assertTrue((rs!=null));
		assertTrue(rs.next());
		assertEquals(rs.getString(1),"1");
		assertEquals(rs.getString(3),"hello");
		rs.close();
		pstmt.close();
		System.out.println();


		// rebinding
		System.out.println("REBINDING:");
		cstmt=con.prepareCall(
			"execute procedure testproc ?, ?, ?, ?");
		cstmt.setInt(1,1);
		cstmt.setDouble(2,1.5);
		cstmt.setString(3,"hello");
		cstmt.setBytes(4,"blob".getBytes(StandardCharsets.UTF_8));
		cstmt.registerOutParameter(1,Types.INTEGER);
		if (issqlrelay) {
			assertFalse(cstmt.execute());
		} else {
			// firebird jdbc returns the output binds as
			// a result set, so execute() returns true
			assertTrue(cstmt.execute());
		}
		assertEquals(cstmt.getInt(1),1);
		cstmt.setInt(1,2);
		if (issqlrelay) {
			assertFalse(cstmt.execute());
		} else {
			// firebird jdbc returns the output binds as
			// a result set, so execute() returns true
			assertTrue(cstmt.execute());
		}
		assertEquals(cstmt.getInt(1),2);
		cstmt.setInt(1,3);
		if (issqlrelay) {
			assertFalse(cstmt.execute());
		} else {
			// firebird jdbc returns the output binds as
			// a result set, so execute() returns true
			assertTrue(cstmt.execute());
		}
		assertEquals(cstmt.getInt(1),3);
		cstmt.close();
		System.out.println();


		// client info properties
		System.out.println("CLIENT INFO PROPERTIES:");
		// close the old connection before replacing it - an abandoned
		// connection keeps its session, and any transaction it still
		// has open, alive for the rest of the run
		con.close();
		con=DriverManager.getConnection(url,props);
		md=con.getMetaData();
		// sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getClientInfoProperties();
			assertTrue((rs!=null));
			rsmd=rs.getMetaData();
			assertTrue((rsmd!=null));
			assertEquals(rsmd.getColumnCount(),4);
			col=1;
			assertEquals(rsmd.getColumnName(col++),"NAME");
			assertEquals(rsmd.getColumnName(col++),"MAX_LEN");
			assertTrue(rsmd.getColumnName(col++).
					startsWith("DEFAULT"));
			assertEquals(rsmd.getColumnName(col++),"DESCRIPTION");
			rs.close();
			System.out.println();
		}


		// catalog list
		System.out.println("CATALOG LIST:");
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
		System.out.println("SCHEMA LIST:");
		rs=md.getSchemas();
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),2);
		col=1;
		assertEquals(rsmd.getColumnName(col++),"TABLE_SCHEM");
		assertEquals(rsmd.getColumnName(col++),"TABLE_CATALOG");
		rs.close();
		System.out.println();


		// table type list
		System.out.println("TABLE TYPE LIST:");
		rs=md.getTableTypes();
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),1);
		col=1;
		assertEquals(rsmd.getColumnName(col++),"TABLE_TYPE");
		found=false;
		assertTrue(rs.next());
		assertEquals(rs.getString("TABLE_TYPE"),"GLOBAL TEMPORARY");
		assertTrue(rs.next());
		assertEquals(rs.getString("TABLE_TYPE"),"SYSTEM TABLE");
		assertTrue(rs.next());
		assertEquals(rs.getString("TABLE_TYPE"),"TABLE");
		assertTrue(rs.next());
		assertEquals(rs.getString("TABLE_TYPE"),"VIEW");
		rs.close();
		System.out.println();


		// table list
		System.out.println("TABLE LIST:");
		rs=md.getTables(null,null,"%",
				new String[] {"TABLE"});
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		if (issqlrelay) {
			assertEquals(rsmd.getColumnCount(),10);
		} else {
			// firebird jdbc returns 12 columns
			assertEquals(rsmd.getColumnCount(),12);
		}
		col=1;
		assertEquals(rsmd.getColumnName(col++),"TABLE_CAT");
		assertEquals(rsmd.getColumnName(col++),"TABLE_SCHEM");
		assertEquals(rsmd.getColumnName(col++),"TABLE_NAME");
		assertEquals(rsmd.getColumnName(col++),"TABLE_TYPE");
		assertEquals(rsmd.getColumnName(col++),"REMARKS");
		assertEquals(rsmd.getColumnName(col++),"TYPE_CAT");
		assertEquals(rsmd.getColumnName(col++),"TYPE_SCHEM");
		assertEquals(rsmd.getColumnName(col++),"TYPE_NAME");
		assertEquals(rsmd.getColumnName(col++),
						"SELF_REFERENCING_COL_NAME");
		assertEquals(rsmd.getColumnName(col++),"REF_GENERATION");
		if (!issqlrelay) {
			// firebird jdbc returns these columns too
			assertEquals(rsmd.getColumnName(col++),
							"OWNER_NAME");
			assertEquals(rsmd.getColumnName(col++),
							"JB_RELATION_ID");
		}
		counter=0;
		while (rs.next()) {
			String	name=rs.getString("TABLE_NAME");
			if (name.equals("TESTTABLE") ||
				name.equals("TESTTABLE1") ||
				name.equals("TESTTABLE2") ||
				name.equals("TESTTABLE3")) {
				counter++;
			}
		}
		assertEquals(counter,4);
		rs.close();
		System.out.println();


		// type info list
		System.out.println("TYPE INFO LIST:");
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
		rs.close();
		System.out.println();


		// column list
		System.out.println("COLUMN LIST:");
		stmt=con.createStatement();
		rs=md.getColumns(null,null,"TESTTABLE","%");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertTrue(rsmd.getColumnCount()>=18);
		col=1;
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
		assertTrue(rs.next());
		// #7971 - firebird has no catalogs or schemas
		assertEquals(rs.getString("TABLE_CAT"),(String)null);
		assertEquals(rs.getString("TABLE_SCHEM"),(String)null);
		assertEquals(rs.getString("COLUMN_NAME"),"TESTINTEGER");
		assertEquals(rs.getString("TYPE_NAME"),"INTEGER");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTSMALLINT");
		assertEquals(rs.getString("TYPE_NAME"),"SMALLINT");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTDECIMAL");
		assertEquals(rs.getString("TYPE_NAME"),"DECIMAL");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTNUMERIC");
		assertEquals(rs.getString("TYPE_NAME"),"NUMERIC");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTFLOAT");
		assertEquals(rs.getString("TYPE_NAME"),"FLOAT");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTDOUBLE");
		assertEquals(rs.getString("TYPE_NAME"),"DOUBLE PRECISION");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTDATE");
		assertEquals(rs.getString("TYPE_NAME"),"DATE");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTTIME");
		assertEquals(rs.getString("TYPE_NAME"),"TIME");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTCHAR");
		assertEquals(rs.getString("TYPE_NAME"),"CHAR");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTVARCHAR");
		assertEquals(rs.getString("TYPE_NAME"),"VARCHAR");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTTIMESTAMP");
		assertEquals(rs.getString("TYPE_NAME"),"TIMESTAMP");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTBLOB");
		assertEquals(rs.getString("TYPE_NAME"),"BLOB SUB_TYPE BINARY");
		rs.close();
		System.out.println();


		// column list - is_autoincrement
		// (the pre-existing testtable2 has col1 as an identity
		// primary key and col2 as a plain integer)
		System.out.println("COLUMN LIST - is_autoincrement:");
		rs=md.getColumns(null,null,"TESTTABLE2","%");
		assertTrue((rs!=null));
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"COL1");
		assertEquals(rs.getString("IS_AUTOINCREMENT"),"YES");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"COL2");
		assertEquals(rs.getString("IS_AUTOINCREMENT"),"NO");
		assertFalse(rs.next());
		rs.close();
		System.out.println();


		// primary key list
		System.out.println("PRIMARY KEY LIST:");
		rs=md.getPrimaryKeys(null,null,"TESTTABLE");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),6);
		col=1;
		assertEquals(rsmd.getColumnName(col++),"TABLE_CAT");
		assertEquals(rsmd.getColumnName(col++),"TABLE_SCHEM");
		assertEquals(rsmd.getColumnName(col++),"TABLE_NAME");
		assertEquals(rsmd.getColumnName(col++),"COLUMN_NAME");
		assertEquals(rsmd.getColumnName(col++),"KEY_SEQ");
		assertEquals(rsmd.getColumnName(col++),"PK_NAME");
		// testtable has no primary key
		assertFalse(rs.next());
		rs.close();
		System.out.println();


		// key and index list
		System.out.println("KEY AND INDEX LIST:");
		rs=md.getIndexInfo(null,null,"TESTTABLE",false,true);
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),13);
		col=1;
		assertEquals(rsmd.getColumnName(col++),"TABLE_CAT");
		assertEquals(rsmd.getColumnName(col++),"TABLE_SCHEM");
		assertEquals(rsmd.getColumnName(col++),"TABLE_NAME");
		assertEquals(rsmd.getColumnName(col++),"NON_UNIQUE");
		assertEquals(rsmd.getColumnName(col++),"INDEX_QUALIFIER");
		assertEquals(rsmd.getColumnName(col++),"INDEX_NAME");
		assertEquals(rsmd.getColumnName(col++),"TYPE");
		assertEquals(rsmd.getColumnName(col++),"ORDINAL_POSITION");
		assertEquals(rsmd.getColumnName(col++),"COLUMN_NAME");
		assertEquals(rsmd.getColumnName(col++),"ASC_OR_DESC");
		assertEquals(rsmd.getColumnName(col++),"CARDINALITY");
		assertEquals(rsmd.getColumnName(col++),"PAGES");
		assertEquals(rsmd.getColumnName(col++),"FILTER_CONDITION");
		// testtable has no indexes
		assertFalse(rs.next());
		rs.close();
		System.out.println();


		// procedure list
		System.out.println("PROCEDURE LIST:");
		rs=md.getProcedures(null,null,"%");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		if (issqlrelay) {
			assertEquals(rsmd.getColumnCount(),8);
		} else {
			// firebird jdbc returns 9 columns
			assertEquals(rsmd.getColumnCount(),9);
		}
		col=1;
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
			// firebird jdbc returns these columns
			assertEquals(rsmd.getColumnName(col++),"FUTURE1");
			assertEquals(rsmd.getColumnName(col++),"FUTURE2");
			assertEquals(rsmd.getColumnName(col++),"FUTURE3");
		}
		assertEquals(rsmd.getColumnName(col++),"REMARKS");
		assertEquals(rsmd.getColumnName(col++),"PROCEDURE_TYPE");
		counter=0;
		while (rs.next()) {
			String name=rs.getString("PROCEDURE_NAME");
			if (name.equals("TESTPROC") ||
					name.equals("TESTPROC1")) {
				if (issqlrelay) {
					assertEquals(
						rs.getShort("PROCEDURE_TYPE"),
						DatabaseMetaData.
							procedureNoResult);
				} else {
					// native Jaybird reports the type as
					// procedureReturnsResult
					assertEquals(
						rs.getShort("PROCEDURE_TYPE"),
						DatabaseMetaData.
							procedureReturnsResult);
				}
				counter++;
			}
		}
		assertEquals(counter,2);
		rs.close();
		System.out.println();


		// procedure parameter list
		System.out.println("PROCEDURE PARAMETER LIST:");
		rs=md.getProcedureColumns(null,null,"TESTPROC","%");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),20);
		col=1;
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
		assertEquals(rs.getString("COLUMN_NAME"),"OUT1");
		assertEquals(rs.getString("TYPE_NAME"),"INTEGER");
		assertEquals(rs.getString("ORDINAL_POSITION"),"1");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"OUT2");
		assertEquals(rs.getString("TYPE_NAME"),"FLOAT");
		assertEquals(rs.getString("ORDINAL_POSITION"),"2");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"OUT3");
		assertEquals(rs.getString("TYPE_NAME"),"VARCHAR");
		assertEquals(rs.getString("ORDINAL_POSITION"),"3");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"OUT4");
		assertEquals(rs.getString("TYPE_NAME"),"BLOB SUB_TYPE BINARY");
		assertEquals(rs.getString("ORDINAL_POSITION"),"4");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"IN1");
		assertEquals(rs.getString("TYPE_NAME"),"INTEGER");
		assertEquals(rs.getString("ORDINAL_POSITION"),"1");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"IN2");
		assertEquals(rs.getString("TYPE_NAME"),"FLOAT");
		assertEquals(rs.getString("ORDINAL_POSITION"),"2");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"IN3");
		assertEquals(rs.getString("TYPE_NAME"),"VARCHAR");
		assertEquals(rs.getString("ORDINAL_POSITION"),"3");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"IN4");
		assertEquals(rs.getString("TYPE_NAME"),"BLOB SUB_TYPE BINARY");
		assertEquals(rs.getString("ORDINAL_POSITION"),"4");
		rs.close();
		System.out.println();


		// cursor name
		System.out.println("CURSOR NAME:");
		// native jaybird doesn't implement ResultSet.getCursorName(),
		// so this section only runs against sqlrelay
		if (issqlrelay) {

			boolean	wasautocommit=con.getAutoCommit();

			// the select and the positioned statements have to
			// share a transaction, so autocommit can't be on
			con.setAutoCommit(false);

			// known rows for the positioned statements below
			stmt.executeUpdate("delete from testtable");
			assertEquals(stmt.executeUpdate(
				"insert into "+
				"	testtable "+
				"	(testinteger,testvarchar) "+
				"values ("+
				"	1,'varchar1')"),1);
			assertEquals(stmt.executeUpdate(
				"insert into "+
				"	testtable "+
				"	(testinteger,testvarchar) "+
				"values ("+
				"	2,'varchar2')"),1);
			con.commit();
			System.out.println();

			Statement	cursorstmt=con.createStatement();
			Statement	posstmt=con.createStatement();

			// name the cursor before running the select -
			// firebird refuses a name for a cursor that is
			// already open
			cursorstmt.setCursorName("testcursor");

			// fetch a row at a time, so the backend cursor is
			// left sitting on the row the positioned statements
			// target rather than run off the end of the result set
			cursorstmt.setFetchSize(1);
			ResultSet	cursorrs=cursorstmt.executeQuery(
				"select "+
				"	testinteger, "+
				"	testvarchar "+
				"from "+
				"	testtable "+
				"where "+
				"	testinteger=1 "+
				"for update");
			assertTrue((cursorrs!=null));
			// the jdbc api answers this from the result set, not
			// from the statement the name was set on
			assertEquals(cursorrs.getCursorName(),"testcursor");
			assertTrue(cursorrs.next());
			assertEquals(cursorrs.getInt(1),1);
			System.out.println();

			// positioned update, on a second statement so the
			// select's cursor stays open
			assertEquals(posstmt.executeUpdate(
				"update "+
				"	testtable "+
				"set "+
				"	testvarchar='updated1' "+
				"where current of testcursor"),1);
			// the positioned update hit the row the cursor was on
			rs=posstmt.executeQuery(
				"select "+
				"	testinteger,testvarchar "+
				"from "+
				"	testtable "+
				"order by "+
				"	testinteger");
			assertTrue(rs.next());
			assertEquals(rs.getInt(1),1);
			assertEquals(rs.getString(2),"updated1");
			assertTrue(rs.next());
			assertEquals(rs.getInt(1),2);
			assertEquals(rs.getString(2),"varchar2");
			assertFalse(rs.next());
			rs.close();
			System.out.println();

			// positioned delete, against the other row - the name
			// applies to the next query this statement runs, so it
			// has to be set again here even though it hasn't changed
			cursorrs.close();
			cursorstmt.setCursorName("testcursor");
			cursorrs=cursorstmt.executeQuery(
				"select "+
				"	testinteger, "+
				"	testvarchar "+
				"from "+
				"	testtable "+
				"where "+
				"	testinteger=2 "+
				"for update");
			assertTrue((cursorrs!=null));
			assertEquals(cursorrs.getCursorName(),"testcursor");
			assertTrue(cursorrs.next());
			assertEquals(cursorrs.getInt(1),2);
			assertEquals(posstmt.executeUpdate(
				"delete from "+
				"	testtable "+
				"where current of testcursor"),1);
			// the positioned delete took the row the cursor was on
			rs=posstmt.executeQuery(
				"select "+
				"	testinteger "+
				"from "+
				"	testtable");
			assertTrue(rs.next());
			assertEquals(rs.getInt(1),1);
			assertFalse(rs.next());
			rs.close();
			cursorrs.close();
			System.out.println();

			// leave testtable empty
			posstmt.executeUpdate("delete from testtable");
			con.commit();
			cursorstmt.close();
			posstmt.close();
			con.setAutoCommit(wasautocommit);
		}
		System.out.println();


		// invalid queries
		System.out.println("INVALID QUERIES:");
		try {
			stmt.executeQuery("select * from nonexistent_table");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeUpdate("insert into "+
						"nonexistent_table values (1)");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeUpdate("create table");
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
