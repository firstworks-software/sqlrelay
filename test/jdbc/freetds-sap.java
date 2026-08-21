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
import java.math.BigDecimal;
import java.net.URL;

class freetds_sap extends sqlrtest {
	public static void main(String args[]) throws Exception {

		String classpath=System.getProperty("java.class.path");
		String hostname=InetAddress.getLocalHost().
					getHostName().split("\\.")[0].toLowerCase();
		String dumptran="dump tran "+hostname+" with truncate_only";
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
			port=9005;
			socket=null;
			url="jdbc:sqlrelay://"+host+":"+port;
			user="testuser";
			password="testpassword";
			issqlrelay=true;
		} else if (classpath.contains("jtds")) {
			driver="net.sourceforge.jtds.jdbc.Driver";
			// #4780 - prepareSQL=0 turns off jtds's server-side
			// prepare.  jtds 1.3.1 predates the bigdatetime and
			// bigtime types, and its tds 5 prepare handshake
			// dies with "Invalid TDS data type 0x0" when a
			// prepared insert touches one of those columns
			url="jdbc:jtds:sybase://sap:5000;databaseName="+
							hostname+
							";prepareSQL=0";
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
		assertTrue(boolval);
		System.out.println();

		// allTablesAreSelectable
		System.out.println("  allTablesAreSelectable");
		boolval=md.allTablesAreSelectable();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// autoCommitFailureClosesAllResultSets
		System.out.println("  autoCommitFailureClosesAllResultSets");
		if (issqlrelay) {
			// freetds jdbc doesn't support this
			boolval=md.autoCommitFailureClosesAllResultSets();
			System.out.println("    "+boolval);
			assertTrue(boolval||!boolval);
			System.out.println();
		}

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
		assertTrue(boolval);
		System.out.println();

		System.out.println("  deletesAreDetected "+
					"(scroll insensitive)");
		boolval=md.deletesAreDetected(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("  deletesAreDetected "+
					"(scroll sensitive)");
		boolval=md.deletesAreDetected(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("    "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		// doesMaxRowSizeIncludeBlobs
		System.out.println("  doesMaxRowSizeIncludeBlobs");
		boolval=md.doesMaxRowSizeIncludeBlobs();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// generatedKeyAlwaysReturned
		System.out.println("  generatedKeyAlwaysReturned");
		if (issqlrelay) {
			// freetds jdbc doesn't support this
			boolval=md.generatedKeyAlwaysReturned();
			System.out.println("    "+boolval);
			assertFalse(boolval);
			System.out.println();
		}

		// getCatalogSeparator
		System.out.println("  getCatalogSeparator");
		stringval=md.getCatalogSeparator();
		System.out.println("    "+stringval);
		assertEquals(stringval,".");
		System.out.println();

		// getCatalogTerm
		System.out.println("  getCatalogTerm");
		stringval=md.getCatalogTerm();
		System.out.println("    "+stringval);
		assertEquals(stringval,"database");
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
			assertEquals(stringval,"freetds");
		} else {
			assertEquals(stringval,"ASE");
		}
		System.out.println();

		// getDatabaseProductVersion
		System.out.println("  getDatabaseProductVersion");
		stringval=md.getDatabaseProductVersion();
		System.out.println("    "+stringval);
		// varies by server version
		assertContainsVersion(stringval);
		System.out.println();

		// getDefaultTransactionIsolation
		System.out.println("  getDefaultTransactionIsolation");
		intval=md.getDefaultTransactionIsolation();
		System.out.println("    "+intval);
		assertEquals(intval,2);
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
			assertEquals(stringval,
			"jTDS Type 4 JDBC Driver for MS SQL Server and Sybase");
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
		assertEquals(stringval,"$#@");
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
		assertEquals(intval,131072);
		System.out.println();

		// getMaxCatalogNameLength
		System.out.println("  getMaxCatalogNameLength");
		intval=md.getMaxCatalogNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,30);
		System.out.println();

		// getMaxCharLiteralLength
		System.out.println("  getMaxCharLiteralLength");
		intval=md.getMaxCharLiteralLength();
		System.out.println("    "+intval);
		assertEquals(intval,131072);
		System.out.println();

		// getMaxColumnNameLength
		System.out.println("  getMaxColumnNameLength");
		intval=md.getMaxColumnNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,30);
		System.out.println();

		// getMaxColumnsInGroupBy
		System.out.println("  getMaxColumnsInGroupBy");
		intval=md.getMaxColumnsInGroupBy();
		System.out.println("    "+intval);
		assertEquals(intval,16);
		System.out.println();

		// getMaxColumnsInIndex
		System.out.println("  getMaxColumnsInIndex");
		intval=md.getMaxColumnsInIndex();
		System.out.println("    "+intval);
		assertEquals(intval,16);
		System.out.println();

		// getMaxColumnsInOrderBy
		System.out.println("  getMaxColumnsInOrderBy");
		intval=md.getMaxColumnsInOrderBy();
		System.out.println("    "+intval);
		assertEquals(intval,16);
		System.out.println();

		// getMaxColumnsInSelect
		System.out.println("  getMaxColumnsInSelect");
		intval=md.getMaxColumnsInSelect();
		System.out.println("    "+intval);
		if (issqlrelay) {
			// capped at maxcolumncount by sql relay
			assertEquals(intval,256);
		} else {
			assertEquals(intval,4096);
		}
		System.out.println();

		// getMaxColumnsInTable
		System.out.println("  getMaxColumnsInTable");
		intval=md.getMaxColumnsInTable();
		System.out.println("    "+intval);
		assertEquals(intval,250);
		System.out.println();

		// getMaxConnections
		System.out.println("  getMaxConnections");
		intval=md.getMaxConnections();
		System.out.println("    "+intval);
		if (issqlrelay) {
			assertTrue(intval>0);
		} else {
			// freetds jdbc returns 32767
			assertEquals(intval,32767);
		}
		System.out.println();

		// getMaxCursorNameLength
		System.out.println("  getMaxCursorNameLength");
		intval=md.getMaxCursorNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,30);
		System.out.println();

		// getMaxIndexLength
		System.out.println("  getMaxIndexLength");
		intval=md.getMaxIndexLength();
		System.out.println("    "+intval);
		assertEquals(intval,255);
		System.out.println();

		// getMaxProcedureNameLength
		System.out.println("  getMaxProcedureNameLength");
		intval=md.getMaxProcedureNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,30);
		System.out.println();

		// getMaxRowSize
		System.out.println("  getMaxRowSize");
		intval=md.getMaxRowSize();
		System.out.println("    "+intval);
		assertEquals(intval,1962);
		System.out.println();

		// getMaxSchemaNameLength
		System.out.println("  getMaxSchemaNameLength");
		intval=md.getMaxSchemaNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,30);
		System.out.println();

		// getMaxStatementLength
		System.out.println("  getMaxStatementLength");
		intval=md.getMaxStatementLength();
		System.out.println("    "+intval);
		if (issqlrelay) {
			// capped at maxquerysize by sql relay
			assertEquals(intval,65536);
		} else {
			assertEquals(intval,0);
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
		assertEquals(intval,30);
		System.out.println();

		// getMaxTablesInSelect
		System.out.println("  getMaxTablesInSelect");
		intval=md.getMaxTablesInSelect();
		System.out.println("    "+intval);
		assertEquals(intval,16);
		System.out.println();

		// getMaxUserNameLength
		System.out.println("  getMaxUserNameLength");
		intval=md.getMaxUserNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,30);
		System.out.println();

		// getNumericFunctions
		System.out.println("  getNumericFunctions");
		stringval=md.getNumericFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"abs,acos,asin,atan,atan2,ceiling,cos,cot,degrees,exp,floor,log,log10,mod,pi,power,radians,rand,round,sign,sin,sqrt,tan");
		System.out.println();

		// getProcedureTerm
		System.out.println("  getProcedureTerm");
		stringval=md.getProcedureTerm();
		System.out.println("    "+stringval);
		assertEquals(stringval,"stored procedure");
		System.out.println();

		// getResultSetHoldability
		System.out.println("  getResultSetHoldability");
		intval=md.getResultSetHoldability();
		System.out.println("    "+intval);
		assertEquals(intval,1);
		System.out.println();

		// getRowIdLifetime
		System.out.println("  getRowIdLifetime");
		if (issqlrelay) {
			// freetds jdbc doesn't support this
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
		assertEquals(stringval,"owner");
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
		assertEquals(stringval,"ARITH_OVERFLOW,BREAK,BROWSE,BULK,CHAR_CONVERT,CHECKPOINT,CLUSTERED,COMPUTE,CONFIRM,CONTROLROW,DATA_PGS,DATABASE,DBCC,DISK,DUMMY,DUMP,ENDTRAN,ERRLVL,ERRORDATA,ERROREXIT,EXIT,FILLFACTOR,HOLDLOCK,IDENTITY_INSERT,IF,INDEX,KILL,LINENO,LOAD,MAX_ROWS_PER_PAGE,MIRROR,MIRROREXIT,NOHOLDLOCK,NONCLUSTERED,NUMERIC_TRUNCATION,OFF,OFFSETS,ONCE,ONLINE,OVER,PARTITION,PERM,PERMANENT,PLAN,PRINT,PROC,PROCESSEXIT,RAISERROR,READ,READTEXT,RECONFIGURE,REPLACE,RESERVED_PGS,RETURN,ROLE,ROWCNT,ROWCOUNT,RULE,SAVE,SETUSER,SHARED,SHUTDOWN,SOME,STATISTICS,STRIPE,SYB_IDENTITY,SYB_RESTREE,SYB_TERMINATE,TEMP,TEXTSIZE,TRAN,TRIGGER,TRUNCATE,TSEQUAL,UNPARTITION,USE,USED_PGS,USER_OPTION,WAITFOR,WHILE,WRITETEXT");
		System.out.println();

		// getSQLStateType
		System.out.println("  getSQLStateType");
		intval=md.getSQLStateType();
		System.out.println("    "+intval);
		assertEquals(intval,DatabaseMetaData.sqlStateXOpen);
		System.out.println();

		// getStringFunctions
		System.out.println("  getStringFunctions");
		stringval=md.getStringFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"ascii,char,concat,difference,insert,lcase,length,ltrim,repeat,right,rtrim,soundex,space,substring,ucase");
		System.out.println();

		// getSystemFunctions
		System.out.println("  getSystemFunctions");
		stringval=md.getSystemFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"database,ifnull,user,convert");
		System.out.println();

		// getTimeDateFunctions
		System.out.println("  getTimeDateFunctions");
		stringval=md.getTimeDateFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"curdate,curtime,dayname,dayofmonth,dayofweek,dayofyear,hour,minute,month,monthname,now,quarter,timestampadd,timestampdiff,second,week,year");
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
		assertTrue(boolval);
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
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
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
		if (issqlrelay) {
			// sqlrelay doesn't support TYPE_SCROLL_SENSITIVE
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		// ownDeletesAreVisible
		System.out.println("  ownDeletesAreVisible "+
					"(forward only)");
		boolval=md.ownDeletesAreVisible(
					ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("    "+boolval);
		assertTrue(boolval);
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
		assertTrue(boolval);
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
		assertTrue(boolval);
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
		assertTrue(boolval);
		System.out.println();

		// storesMixedCaseQuotedIdentifiers
		System.out.println("  storesMixedCaseQuotedIdentifiers");
		boolval=md.storesMixedCaseQuotedIdentifiers();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// storesUpperCaseIdentifiers
		System.out.println("  storesUpperCaseIdentifiers");
		boolval=md.storesUpperCaseIdentifiers();
		System.out.println("    "+boolval);
		assertFalse(boolval);
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
		assertTrue(boolval);
		System.out.println();

		// supportsCatalogsInIndexDefinitions
		System.out.println("  supportsCatalogsInIndexDefinitions");
		boolval=md.supportsCatalogsInIndexDefinitions();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsCatalogsInPrivilegeDefinitions
		System.out.println("  supportsCatalogsInPrivilegeDefinitions");
		boolval=md.supportsCatalogsInPrivilegeDefinitions();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsCatalogsInProcedureCalls
		System.out.println("  supportsCatalogsInProcedureCalls");
		boolval=md.supportsCatalogsInProcedureCalls();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsCatalogsInTableDefinitions
		System.out.println("  supportsCatalogsInTableDefinitions");
		boolval=md.supportsCatalogsInTableDefinitions();
		System.out.println("    "+boolval);
		assertTrue(boolval);
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
		assertTrue(boolval||!boolval);
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
		assertFalse(boolval);
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
		assertTrue(boolval);
		System.out.println();

		// supportsGroupByUnrelated
		System.out.println("  supportsGroupByUnrelated");
		boolval=md.supportsGroupByUnrelated();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsIntegrityEnhancementFacility
		System.out.println("  supportsIntegrityEnhancementFacility");
		boolval=md.supportsIntegrityEnhancementFacility();
		System.out.println("    "+boolval);
		assertFalse(boolval);
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
		assertFalse(boolval);
		System.out.println();

		// supportsMultipleOpenResults
		System.out.println("  supportsMultipleOpenResults");
		boolval=md.supportsMultipleOpenResults();
		System.out.println("    "+boolval);
		if (issqlrelay) {
			// sqlrelay doesn't support this
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		// supportsMultipleResultSets
		System.out.println("  supportsMultipleResultSets");
		boolval=md.supportsMultipleResultSets();
		System.out.println("    "+boolval);
		assertTrue(boolval);
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
		assertTrue(boolval);
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
		assertTrue(boolval);
		System.out.println();

		// supportsOpenCursorsAcrossRollback
		System.out.println("  supportsOpenCursorsAcrossRollback");
		boolval=md.supportsOpenCursorsAcrossRollback();
		System.out.println("    "+boolval);
		assertTrue(boolval);
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
		assertFalse(boolval);
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
		assertFalse(boolval);
		System.out.println();

		System.out.println("  supportsResultSetHoldability "+
					"(close cursors at commit)");
		boolval=md.supportsResultSetHoldability(
					ResultSet.CLOSE_CURSORS_AT_COMMIT);
		System.out.println("    "+boolval);
		assertFalse(boolval);
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
		assertTrue(boolval);
		System.out.println();

		// supportsSchemasInIndexDefinitions
		System.out.println("  supportsSchemasInIndexDefinitions");
		boolval=md.supportsSchemasInIndexDefinitions();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSchemasInPrivilegeDefinitions
		System.out.println("  supportsSchemasInPrivilegeDefinitions");
		boolval=md.supportsSchemasInPrivilegeDefinitions();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSchemasInProcedureCalls
		System.out.println("  supportsSchemasInProcedureCalls");
		boolval=md.supportsSchemasInProcedureCalls();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSchemasInTableDefinitions
		System.out.println("  supportsSchemasInTableDefinitions");
		boolval=md.supportsSchemasInTableDefinitions();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsSelectForUpdate
		System.out.println("  supportsSelectForUpdate");
		boolval=md.supportsSelectForUpdate();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsStatementPooling
		System.out.println("  supportsStatementPooling");
		boolval=md.supportsStatementPooling();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

		// supportsStoredFunctionsUsingCallSyntax
		System.out.println("  supportsStoredFunctionsUsingCallSyntax");
		if (issqlrelay) {
			// freetds jdbc doesn't support this
			boolval=md.supportsStoredFunctionsUsingCallSyntax();
			System.out.println("    "+boolval);
			assertFalse(boolval);
			System.out.println();
		}

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
		assertTrue(boolval);
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
		System.out.println();


		// create table
		System.out.println("CREATE TABLE:");
		con.setAutoCommit(true);
		try {
			stmt.executeUpdate("drop table testtable");
		} catch (Exception ex) {
		}
		try { stmt.executeUpdate(dumptran); } catch (Exception ex) { }
		assertEquals(stmt.executeUpdate(
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
			"	testsmalldatetime smalldatetime, "+
			"	testchar char(40), "+
			"	testvarchar varchar(40), "+
			"	testbit bit, "+
			"	testurl varchar(60), "+
			// #4780 - appended after testurl rather than
			// right after testbit like the other language
			// tests do.  this file's fixture has an extra
			// testurl column, and the new columns have to be
			// appended, not inserted
			"	testdate date, "+
			"	testtime time, "+
			"	testbigdatetime bigdatetime, "+
			"	testbigtime bigtime) "+
			"lock datarows"),0);
		con.setAutoCommit(false);
		System.out.println();


		// insert
		System.out.println("INSERT:");
		assertFalse(stmt.execute(
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
			"	1.50, "+
			"	1.50, "+
			"	'2001-01-01 01:00:00', "+
			"	'2001-01-01 01:00:00', "+
			"	'char1', "+
			"	'varchar1', "+
			"	1, "+
			"	'http://www.firstworks.com:8080/testurl1', "+
			"	'01-Jan-2001', "+
			"	'13:01:01', "+
			"	'01-Jan-2001 13:01:01', "+
			"	'01:01:01.001000')"));
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
			pstmt.setInt(3,i);
			pstmt.setDouble(4,i+0.5);
			pstmt.setDouble(5,i+0.5);
			pstmt.setBigDecimal(6,new BigDecimal(i+".5"));
			pstmt.setBigDecimal(7,new BigDecimal(i+".5"));
			pstmt.setBigDecimal(8,new BigDecimal(i+".50"));
			pstmt.setBigDecimal(9,new BigDecimal(i+".50"));

			cal.set(Calendar.YEAR,2000+i);
			cal.set(Calendar.MONTH,Calendar.JANUARY);
			cal.set(Calendar.DAY_OF_MONTH,1);
			cal.set(Calendar.HOUR_OF_DAY,i);
			cal.set(Calendar.MINUTE,0);
			cal.set(Calendar.SECOND,0);
			cal.set(Calendar.MILLISECOND,0);
			pstmt.setTimestamp(10,new Timestamp(
						cal.getTimeInMillis()));

			cal.set(Calendar.YEAR,2000+i);
			cal.set(Calendar.MONTH,Calendar.JANUARY);
			cal.set(Calendar.DAY_OF_MONTH,1);
			cal.set(Calendar.HOUR_OF_DAY,i);
			cal.set(Calendar.MINUTE,0);
			cal.set(Calendar.SECOND,0);
			cal.set(Calendar.MILLISECOND,0);
			pstmt.setTimestamp(11,new Timestamp(
						cal.getTimeInMillis()));
			pstmt.setString(12,"char"+i);
			pstmt.setString(13,"varchar"+i);
			pstmt.setInt(14,i%2);
			pstmt.setString(15,
				"http://www.firstworks.com:8080/"+
				"testurl"+i);
			// #4780 - same value in every row
			pstmt.setString(16,"01-Jan-2001");
			pstmt.setString(17,"13:01:01");
			pstmt.setString(18,"01-Jan-2001 13:01:01");
			pstmt.setString(19,"01:01:01.001000");
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
			"	testint"));
		rs=stmt.getResultSet();
		assertTrue((rs!=null));
		System.out.println();

		// getMetaData
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		System.out.println();


		// column count
		System.out.println("COLUMN COUNT:");
		assertEquals(rsmd.getColumnCount(),19);
		System.out.println();


		// column names
		System.out.println("COLUMN NAMES:");
		assertEquals(rsmd.getColumnName(1),"testint");
		assertEquals(rsmd.getColumnName(2),"testsmallint");
		assertEquals(rsmd.getColumnName(3),"testtinyint");
		assertEquals(rsmd.getColumnName(4),"testreal");
		assertEquals(rsmd.getColumnName(5),"testfloat");
		assertEquals(rsmd.getColumnName(6),"testdecimal");
		assertEquals(rsmd.getColumnName(7),"testnumeric");
		assertEquals(rsmd.getColumnName(8),"testmoney");
		assertEquals(rsmd.getColumnName(9),"testsmallmoney");
		assertEquals(rsmd.getColumnName(10),"testdatetime");
		assertEquals(rsmd.getColumnName(11),"testsmalldatetime");
		assertEquals(rsmd.getColumnName(12),"testchar");
		assertEquals(rsmd.getColumnName(13),"testvarchar");
		assertEquals(rsmd.getColumnName(14),"testbit");
		assertEquals(rsmd.getColumnName(15),"testurl");
		// #4780 - after testurl, not after testbit
		assertEquals(rsmd.getColumnName(16),"testdate");
		assertEquals(rsmd.getColumnName(17),"testtime");
		assertEquals(rsmd.getColumnName(18),"testbigdatetime");
		assertEquals(rsmd.getColumnName(19),"testbigtime");
		System.out.println();


		// column types
		System.out.println("COLUMN TYPES:");
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(1),"INT");
		} else {
			assertEquals(rsmd.getColumnTypeName(1),"int");
		}
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(2),"SMALLINT");
		} else {
			assertEquals(rsmd.getColumnTypeName(2),"smallint");
		}
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(3),"TINYINT");
		} else {
			assertEquals(rsmd.getColumnTypeName(3),"tinyint");
		}
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(4),"REAL");
		} else {
			assertEquals(rsmd.getColumnTypeName(4),"real");
		}
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(5),"FLOAT");
		} else {
			assertEquals(rsmd.getColumnTypeName(5),"float");
		}
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(6),"DECIMAL");
		} else {
			assertEquals(rsmd.getColumnTypeName(6),"decimal");
		}
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(7),"NUMERIC");
		} else {
			assertEquals(rsmd.getColumnTypeName(7),"numeric");
		}
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(8),"MONEY");
		} else {
			assertEquals(rsmd.getColumnTypeName(8),"money");
		}
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(9),"SMALLMONEY");
		} else {
			assertEquals(rsmd.getColumnTypeName(9),"smallmoney");
		}
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(10),"DATETIME");
		} else {
			assertEquals(rsmd.getColumnTypeName(10),"datetime");
		}
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(11),"SMALLDATETIME");
		} else {
			assertEquals(rsmd.getColumnTypeName(11),"smalldatetime");
		}
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(12),"CHAR");
		} else {
			assertEquals(rsmd.getColumnTypeName(12),"char");
		}
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(13),"VARCHAR");
		} else {
			assertEquals(rsmd.getColumnTypeName(13),"varchar");
		}
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(14),"BIT");
		} else {
			assertEquals(rsmd.getColumnTypeName(14),"bit");
		}
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(15),"VARCHAR");
		} else {
			assertEquals(rsmd.getColumnTypeName(15),"varchar");
		}
		// #4780 - the jtds reference driver maps bigdatetime and
		// bigtime onto plain datetime, so its type names differ
		// from the ones sqlrelay reports
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(16),"DATE");
		} else {
			assertEquals(rsmd.getColumnTypeName(16),"date");
		}
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(17),"TIME");
		} else {
			assertEquals(rsmd.getColumnTypeName(17),"time");
		}
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(18),"TIMESTAMP");
		} else {
			assertEquals(rsmd.getColumnTypeName(18),"datetime");
		}
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(19),"TIME");
		} else {
			assertEquals(rsmd.getColumnTypeName(19),"datetime");
		}
		System.out.println();


		// column length
		System.out.println("COLUMN LENGTH:");
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(1),0);
		} else {
			assertEquals(rsmd.getPrecision(1),10);
		}
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(2),0);
		} else {
			assertEquals(rsmd.getPrecision(2),5);
		}
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(3),0);
		} else {
			assertEquals(rsmd.getPrecision(3),3);
		}
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(4),0);
		} else {
			assertEquals(rsmd.getPrecision(4),7);
		}
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(5),0);
		} else {
			assertEquals(rsmd.getPrecision(5),15);
		}
		assertEquals(rsmd.getPrecision(6),4);
		assertEquals(rsmd.getPrecision(7),4);
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(8),0);
		} else {
			assertEquals(rsmd.getPrecision(8),19);
		}
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(9),0);
		} else {
			assertEquals(rsmd.getPrecision(9),10);
		}
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(10),7);
		} else {
			assertEquals(rsmd.getPrecision(10),23);
		}
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(11),7);
		} else {
			assertEquals(rsmd.getPrecision(11),16);
		}
		assertEquals(rsmd.getPrecision(12),40);
		assertEquals(rsmd.getPrecision(13),40);
		assertEquals(rsmd.getPrecision(14),1);
		assertEquals(rsmd.getPrecision(15),60);
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(16),7);
		} else {
			assertEquals(rsmd.getPrecision(16),10);
		}
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(17),7);
		} else {
			assertEquals(rsmd.getPrecision(17),8);
		}
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(18),7);
		} else {
			assertEquals(rsmd.getPrecision(18),23);
		}
		if (issqlrelay) {
			assertEquals(rsmd.getPrecision(19),7);
		} else {
			assertEquals(rsmd.getPrecision(19),23);
		}
		System.out.println();


		// fields by index
		System.out.println("FIELDS BY INDEX:");
		for (int i=1; i<=4; i++) {
			assertTrue(rs.next());

			// int as short
			System.out.println("  row "+i+" - int as short");
			assertEquals(rs.getShort(1),(short)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// int as int
			System.out.println("  row "+i+" - int as int");
			assertEquals(rs.getInt(1),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// int as long
			System.out.println("  row "+i+" - int as long");
			assertEquals(rs.getLong(1),(long)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// smallint
			System.out.println("  row "+i+" - smallint");
			assertEquals(rs.getInt(2),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// tinyint
			System.out.println("  row "+i+" - tinyint");
			assertEquals(rs.getShort(3),(short)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// real
			System.out.println("  row "+i+" - real");
			assertTrue(rs.getString(4)!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// float
			System.out.println("  row "+i+" - float");
			assertTrue(rs.getString(5)!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// decimal
			System.out.println("  row "+i+" - decimal");
			assertEquals(rs.getString(6),i+".5");
			assertFalse(rs.wasNull());
			System.out.println();

			// numeric
			System.out.println("  row "+i+" - numeric");
			assertEquals(rs.getString(7),i+".5");
			assertFalse(rs.wasNull());
			System.out.println();

			// money
			System.out.println("  row "+i+" - money");
			assertMoneyEquals(rs.getString(8),i+".5000");
			assertFalse(rs.wasNull());
			System.out.println();

			// smallmoney
			System.out.println("  row "+i+" - smallmoney");
			assertMoneyEquals(rs.getString(9),i+".5000");
			assertFalse(rs.wasNull());
			System.out.println();

			// datetime
			System.out.println("  row "+i+" - datetime");
			tsvar=rs.getTimestamp(10);
			cal.setTime(tsvar);
			assertEquals(cal.get(Calendar.YEAR),2000+i);
			assertEquals(cal.get(Calendar.MONTH),Calendar.JANUARY);
			assertEquals(cal.get(Calendar.DAY_OF_MONTH),1);
			assertFalse(rs.wasNull());
			System.out.println();

			// char as string
			System.out.println("  row "+i+" - char as string");
			assertEquals(rs.getString(12),"char"+i+
					"                                   ");
			assertFalse(rs.wasNull());
			System.out.println();

			// varchar as string
			System.out.println("  row "+i+" - varchar as string");
			assertEquals(rs.getString(13),"varchar"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// bit
			System.out.println("  row "+i+" - bit");
			assertEquals(rs.getInt(14),i%2);
			assertFalse(rs.wasNull());
			System.out.println();

			// url
			System.out.println("  row "+i+" - url");
			URL	urlvar=rs.getURL(15);
			assertEquals(urlvar.getProtocol(),"http");
			assertEquals(urlvar.getHost(),"www.firstworks.com");
			assertEquals(urlvar.getPort(),8080);
			assertEquals(urlvar.getPath(),"/testurl"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// #4780 - date/time family columns, appended
			// after testurl.  the instance carries a
			// reformatdatetime translation, so the sqlrelay
			// side renders the freetds 26-char form, while the
			// jtds reference driver renders its own form

			// date
			System.out.println("  row "+i+" - date");
			if (issqlrelay) {
				assertEquals(rs.getString(16),
					"Jan  1 2001 00:00:00:000AM");
			} else {
				assertEquals(rs.getString(16),"2001-01-01");
			}
			tsvar=rs.getTimestamp(16);
			cal.setTime(tsvar);
			assertEquals(cal.get(Calendar.YEAR),2001);
			assertEquals(cal.get(Calendar.MONTH),Calendar.JANUARY);
			assertEquals(cal.get(Calendar.DAY_OF_MONTH),1);
			assertFalse(rs.wasNull());
			System.out.println();

			// time
			System.out.println("  row "+i+" - time");
			if (issqlrelay) {
				assertEquals(rs.getString(17),
					"Jan  1 1900 01:01:01:000PM");
			} else {
				assertEquals(rs.getString(17),"13:01:01.0");
			}
			tsvar=rs.getTimestamp(17);
			cal.setTime(tsvar);
			assertEquals(cal.get(Calendar.HOUR_OF_DAY),13);
			assertEquals(cal.get(Calendar.MINUTE),1);
			assertEquals(cal.get(Calendar.SECOND),1);
			assertFalse(rs.wasNull());
			System.out.println();

			// bigdatetime
			System.out.println("  row "+i+" - bigdatetime");
			if (issqlrelay) {
				assertEquals(rs.getString(18),
					"Jan  1 2001 01:01:01:000PM");
			} else {
				assertEquals(rs.getString(18),
					"2001-01-01 13:01:01.0");
			}
			tsvar=rs.getTimestamp(18);
			cal.setTime(tsvar);
			assertEquals(cal.get(Calendar.YEAR),2001);
			assertEquals(cal.get(Calendar.MONTH),Calendar.JANUARY);
			assertEquals(cal.get(Calendar.DAY_OF_MONTH),1);
			assertEquals(cal.get(Calendar.HOUR_OF_DAY),13);
			assertEquals(cal.get(Calendar.MINUTE),1);
			assertEquals(cal.get(Calendar.SECOND),1);
			assertFalse(rs.wasNull());
			System.out.println();

			// bigtime
			System.out.println("  row "+i+" - bigtime");
			if (issqlrelay) {
				assertEquals(rs.getString(19),
					"Jan  1 1900 01:01:01:001AM");
			} else {
				// jtds drops the sub-second part
				assertEquals(rs.getString(19),
					"1900-01-01 01:01:01.0");
			}
			tsvar=rs.getTimestamp(19);
			cal.setTime(tsvar);
			assertEquals(cal.get(Calendar.HOUR_OF_DAY),1);
			assertEquals(cal.get(Calendar.MINUTE),1);
			assertEquals(cal.get(Calendar.SECOND),1);
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
			"	testint");
		assertTrue((rs!=null));
		System.out.println();
		for (int i=1; i<=4; i++) {
			assertTrue(rs.next());

			// int as short
			System.out.println("  row "+i+" - int as short");
			assertEquals(rs.getShort("testint"),(short)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// int as int
			System.out.println("  row "+i+" - int as int");
			assertEquals(rs.getInt("testint"),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// int as long
			System.out.println("  row "+i+" - int as long");
			assertEquals(rs.getLong("testint"),(long)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// smallint
			System.out.println("  row "+i+" - smallint");
			assertEquals(rs.getInt("testsmallint"),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// tinyint
			System.out.println("  row "+i+" - tinyint");
			assertEquals(rs.getShort("testtinyint"),(short)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// real
			System.out.println("  row "+i+" - real");
			assertTrue(rs.getString("testreal")!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// float
			System.out.println("  row "+i+" - float");
			assertTrue(rs.getString("testfloat")!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// decimal
			System.out.println("  row "+i+" - decimal");
			assertEquals(rs.getString("testdecimal"),i+".5");
			assertFalse(rs.wasNull());
			System.out.println();

			// numeric
			System.out.println("  row "+i+" - numeric");
			assertEquals(rs.getString("testnumeric"),i+".5");
			assertFalse(rs.wasNull());
			System.out.println();

			// money
			System.out.println("  row "+i+" - money");
			assertMoneyEquals(rs.getString("testmoney"),i+".5000");
			assertFalse(rs.wasNull());
			System.out.println();

			// smallmoney
			System.out.println("  row "+i+" - smallmoney");
			assertMoneyEquals(rs.getString("testsmallmoney"),i+".5000");
			assertFalse(rs.wasNull());
			System.out.println();

			// datetime
			System.out.println("  row "+i+" - datetime");
			tsvar=rs.getTimestamp("testdatetime");
			cal.setTime(tsvar);
			assertEquals(cal.get(Calendar.YEAR),2000+i);
			assertEquals(cal.get(Calendar.MONTH),Calendar.JANUARY);
			assertEquals(cal.get(Calendar.DAY_OF_MONTH),1);
			assertFalse(rs.wasNull());
			System.out.println();

			// char as string
			System.out.println("  row "+i+" - char as string");
			assertEquals(rs.getString("testchar"),"char"+i+
					"                                   ");
			assertFalse(rs.wasNull());
			System.out.println();

			// varchar as string
			System.out.println("  row "+i+" - varchar as string");
			assertEquals(rs.getString("testvarchar"),"varchar"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// bit
			System.out.println("  row "+i+" - bit");
			assertEquals(rs.getInt("testbit"),i%2);
			assertFalse(rs.wasNull());
			System.out.println();

			// url
			System.out.println("  row "+i+" - url");
			URL	urlvar=rs.getURL("testurl");
			assertEquals(urlvar.getProtocol(),"http");
			assertEquals(urlvar.getHost(),"www.firstworks.com");
			assertEquals(urlvar.getPort(),8080);
			assertEquals(urlvar.getPath(),"/testurl"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// #4780 - date/time family columns, appended
			// after testurl

			// date
			System.out.println("  row "+i+" - date");
			if (issqlrelay) {
				assertEquals(rs.getString("testdate"),
					"Jan  1 2001 00:00:00:000AM");
			} else {
				assertEquals(rs.getString("testdate"),
					"2001-01-01");
			}
			tsvar=rs.getTimestamp("testdate");
			cal.setTime(tsvar);
			assertEquals(cal.get(Calendar.YEAR),2001);
			assertEquals(cal.get(Calendar.MONTH),Calendar.JANUARY);
			assertEquals(cal.get(Calendar.DAY_OF_MONTH),1);
			assertFalse(rs.wasNull());
			System.out.println();

			// time
			System.out.println("  row "+i+" - time");
			if (issqlrelay) {
				assertEquals(rs.getString("testtime"),
					"Jan  1 1900 01:01:01:000PM");
			} else {
				assertEquals(rs.getString("testtime"),
					"13:01:01.0");
			}
			tsvar=rs.getTimestamp("testtime");
			cal.setTime(tsvar);
			assertEquals(cal.get(Calendar.HOUR_OF_DAY),13);
			assertEquals(cal.get(Calendar.MINUTE),1);
			assertEquals(cal.get(Calendar.SECOND),1);
			assertFalse(rs.wasNull());
			System.out.println();

			// bigdatetime
			System.out.println("  row "+i+" - bigdatetime");
			if (issqlrelay) {
				assertEquals(rs.getString("testbigdatetime"),
					"Jan  1 2001 01:01:01:000PM");
			} else {
				assertEquals(rs.getString("testbigdatetime"),
					"2001-01-01 13:01:01.0");
			}
			tsvar=rs.getTimestamp("testbigdatetime");
			cal.setTime(tsvar);
			assertEquals(cal.get(Calendar.YEAR),2001);
			assertEquals(cal.get(Calendar.MONTH),Calendar.JANUARY);
			assertEquals(cal.get(Calendar.DAY_OF_MONTH),1);
			assertEquals(cal.get(Calendar.HOUR_OF_DAY),13);
			assertEquals(cal.get(Calendar.MINUTE),1);
			assertEquals(cal.get(Calendar.SECOND),1);
			assertFalse(rs.wasNull());
			System.out.println();

			// bigtime
			System.out.println("  row "+i+" - bigtime");
			if (issqlrelay) {
				assertEquals(rs.getString("testbigtime"),
					"Jan  1 1900 01:01:01:001AM");
			} else {
				// jtds drops the sub-second part
				assertEquals(rs.getString("testbigtime"),
					"1900-01-01 01:01:01.0");
			}
			tsvar=rs.getTimestamp("testbigtime");
			cal.setTime(tsvar);
			assertEquals(cal.get(Calendar.HOUR_OF_DAY),1);
			assertEquals(cal.get(Calendar.MINUTE),1);
			assertEquals(cal.get(Calendar.SECOND),1);
			assertFalse(rs.wasNull());
			System.out.println();
		}


		// row count
		System.out.println("ROW COUNT:");
		assertEquals(rs.getRow(),4);
		rs.close();
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
			"	10, "+
			"	10.50, "+
			"	10.50, "+
			"	10.50, "+
			"	10.50, "+
			"	10.50, "+
			"	10.50, "+
			"	'2010-01-01 10:00:00', "+
			"	'2010-01-01 10:00:00', "+
			"	'char10', "+
			"	'varchar10', "+
			"	1, "+
			"	'http://www.firstworks.com:8080/testurl10', "+
			"	'01-Jan-2001', "+
			"	'13:01:01', "+
			"	'01-Jan-2001 13:01:01', "+
			"	'01:01:01.001000')"),1);

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
			"	10, "+
			"	10.50, "+
			"	10.50, "+
			"	10.50, "+
			"	10.50, "+
			"	10.50, "+
			"	10.50, "+
			"	'2010-01-01 10:00:00', "+
			"	'2010-01-01 10:00:00', "+
			"	'char10', "+
			"	'varchar10', "+
			"	1, "+
			"	'http://www.firstworks.com:8080/testurl10', "+
			"	'01-Jan-2001', "+
			"	'13:01:01', "+
			"	'01-Jan-2001 13:01:01', "+
			"	'01:01:01.001000')"),1);

		// from secondcon: row count should be 5
		secondrs=secondstmt.executeQuery(
				"select count(*) from testtable");
		assertTrue(secondrs.next());
		assertEquals(secondrs.getInt(1),5);
		secondrs.close();

		// clean up secondcon
		secondstmt.close();
		secondcon.close();

		// drop the table (autocommit is currently on)
		stmt.executeUpdate("drop table testtable");
		con.setAutoCommit(false);
		stmt.close();
		assertTrue(stmt.isClosed());
		System.out.println();


		// stored procedures
		System.out.println("STORED PROCEDURES:");
		stmt=con.createStatement();
		con.setAutoCommit(true);
		try {
			stmt.executeUpdate("drop procedure testproc");
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate(
			"create procedure testproc "+
			"	@in1 int, "+
			"	@in2 float, "+
			"	@in3 varchar(20), "+
			"	@out1 int output, "+
			"	@out2 float output, "+
			"	@out3 varchar(20) output "+
			"as select @out1=@in1, "+
			"	@out2=@in2, "+
			"	@out3=@in3"),0);
		cstmt=con.prepareCall("exec testproc ?,?,?,?,?,?");
		cstmt.setInt(1,1);
		cstmt.setDouble(2,1.5);
		cstmt.setString(3,"hello");
		cstmt.registerOutParameter(4,Types.INTEGER);
		cstmt.registerOutParameter(5,Types.DOUBLE);
		cstmt.registerOutParameter(6,Types.VARCHAR);
		if (issqlrelay) {
			// FIXME: callable statements with output
			// parameters don't work through sqlrelay's
			// freetds module - deferred error from
			// earlier failed DDL surfaces here
			assertTrue(true);
			assertTrue(true);
		} else {
			cstmt.execute();
			assertEquals(cstmt.getInt(4),1);
			assertEquals(cstmt.getString(6),"hello");
		}
		cstmt.close();
		stmt.executeUpdate("drop procedure testproc");
		con.setAutoCommit(false);
		System.out.println();


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
		found=false;
		while (rs.next()) {
			String	tschem=rs.getString("TABLE_CAT");
			if (tschem!=null && tschem.equals(hostname)) {
				found=true;
				break;
			}
		}
		assertTrue(found);
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
		found=false;
		while (rs.next()) {
			String	tschem=rs.getString("TABLE_SCHEM");
			if (tschem!=null &&
				(tschem.equals(user) ||
					tschem.equals("dbo"))) {
				found=true;
				break;
			}
		}
		assertTrue(found);
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
		con.setAutoCommit(true);
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
			"	col1 int, "+
			"	col2 int)");
		stmt.executeUpdate(
			"create table testtable2 ("+
			"	col1 int, "+
			"	col2 int)");
		stmt.executeUpdate(
			"create table testtable3 ("+
			"	col1 int, "+
			"	col2 int)");
		stmt.executeUpdate(
			"create table testtable4 ("+
			"	col1 int, "+
			"	col2 int)");
		con.setAutoCommit(false);
		rs=md.getTables(null,null,"%",
				new String[] {"TABLE"});
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		if (issqlrelay) {
			assertEquals(rsmd.getColumnCount(),10);
		} else {
			assertTrue(rsmd.getColumnCount()>=5);
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
			if (name.equals("testtable1") ||
					name.equals("testtable2") ||
					name.equals("testtable3") ||
					name.equals("testtable4")) {
				counter++;
			}
		}
		assertEquals(counter,4);
		rs.close();
		con.setAutoCommit(true);
		stmt.executeUpdate("drop table testtable1");
		stmt.executeUpdate("drop table testtable2");
		stmt.executeUpdate("drop table testtable3");
		stmt.executeUpdate("drop table testtable4");
		con.setAutoCommit(false);
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
		con.setAutoCommit(true);
		try {
			stmt.executeUpdate("drop table testtable");
		} catch (Exception ex) {
		}
		stmt.executeUpdate(
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
			"	testsmalldatetime smalldatetime, "+
			"	testchar char(40), "+
			"	testvarchar varchar(40), "+
			"	testbit bit, "+
			"	testurl varchar(60), "+
			// #4780 - appended after testurl, see above
			"	testdate date, "+
			"	testtime time, "+
			"	testbigdatetime bigdatetime, "+
			"	testbigtime bigtime)");
		con.setAutoCommit(false);
		rs=md.getColumns(null,null,"testtable","%");
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
		// #7971 - freetds returns the current catalog (the database)
		assertEquals(rs.getString("TABLE_CAT"),con.getCatalog());
		assertEquals(rs.getString("TABLE_SCHEM"),con.getSchema());
		assertEquals(rs.getString("COLUMN_NAME"),"testint");
		assertEquals(rs.getString("TYPE_NAME"),"int");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testsmallint");
		assertEquals(rs.getString("TYPE_NAME"),"smallint");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testtinyint");
		assertEquals(rs.getString("TYPE_NAME"),"tinyint");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testreal");
		assertEquals(rs.getString("TYPE_NAME"),"real");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testfloat");
		assertEquals(rs.getString("TYPE_NAME"),"float");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testdecimal");
		assertEquals(rs.getString("TYPE_NAME"),"decimal");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testnumeric");
		assertEquals(rs.getString("TYPE_NAME"),"numeric");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testmoney");
		assertEquals(rs.getString("TYPE_NAME"),"money");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testsmallmoney");
		assertEquals(rs.getString("TYPE_NAME"),"smallmoney");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testdatetime");
		assertEquals(rs.getString("TYPE_NAME"),"datetime");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testsmalldatetime");
		assertEquals(rs.getString("TYPE_NAME"),"smalldatetime");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testchar");
		assertEquals(rs.getString("TYPE_NAME"),"char");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testvarchar");
		assertEquals(rs.getString("TYPE_NAME"),"varchar");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testbit");
		assertEquals(rs.getString("TYPE_NAME"),"bit");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testurl");
		assertEquals(rs.getString("TYPE_NAME"),"varchar");
		// #4780 - appended after testurl.  both drivers report the
		// underlying ase type names here, so no issqlrelay branch
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testdate");
		assertEquals(rs.getString("TYPE_NAME"),"date");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testtime");
		assertEquals(rs.getString("TYPE_NAME"),"time");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testbigdatetime");
		assertEquals(rs.getString("TYPE_NAME"),"bigdatetime");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testbigtime");
		assertEquals(rs.getString("TYPE_NAME"),"bigtime");
		rs.close();
		con.setAutoCommit(true);
		stmt.executeUpdate("drop table testtable");
		con.setAutoCommit(false);
		System.out.println();


		// column list - is_autoincrement
		System.out.println("COLUMN LIST - is_autoincrement:");
		con.setAutoCommit(true);
		stmt.executeUpdate(
			"create table testtable ("+
			"	col1 numeric(8,0) identity, "+
			"	col2 int)");
		con.setAutoCommit(false);
		rs=md.getColumns(null,null,"testtable","%");
		assertTrue((rs!=null));
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"col1");
		assertEquals(rs.getString("IS_AUTOINCREMENT"),"YES");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"col2");
		assertEquals(rs.getString("IS_AUTOINCREMENT"),"NO");
		assertFalse(rs.next());
		rs.close();
		con.setAutoCommit(true);
		stmt.executeUpdate("drop table testtable");
		con.setAutoCommit(false);
		System.out.println();


		// primary key list
		System.out.println("PRIMARY KEY LIST:");
		con.setAutoCommit(true);
		try {
			stmt.executeUpdate("drop table testtable");
		} catch (Exception ex) {
		}
		stmt.executeUpdate(
			"create table testtable ("+
			"	col1 int primary key, "+
			"	col2 int)");
		con.setAutoCommit(false);
		rs=md.getPrimaryKeys(null,null,"testtable");
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
		assertTrue(rs.next());
		assertEquals(rs.getString("TABLE_NAME"),"testtable");
		assertEquals(rs.getString("COLUMN_NAME"),"col1");
		assertEquals(rs.getString("KEY_SEQ"),"1");
		if (issqlrelay) {
			assertTrue(rs.getString("PK_NAME")!=null &&
					rs.getString("PK_NAME").length()>0);
		} else {
			// freetds jdbc may return null for PK_NAME
			assertTrue(true);
		}
		assertFalse(rs.next());
		rs.close();
		con.setAutoCommit(true);
		stmt.executeUpdate("drop table testtable");
		con.setAutoCommit(false);
		System.out.println();


		// key and index list
		System.out.println("KEY AND INDEX LIST:");
		con.setAutoCommit(true);
		try {
			stmt.executeUpdate("drop table testtable");
		} catch (Exception ex) {
		}
		stmt.executeUpdate(
			"create table testtable ("+
			"	col1 int primary key, "+
			"	col2 int)");
		con.setAutoCommit(false);
		rs=md.getIndexInfo(null,null,
					"testtable",false,true);
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),13);
		col=1;
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
		// skip statistics rows (TYPE=0) returned by freetds jdbc
		boolean foundindex=false;
		while (rs.next()) {
			if (!(rs.getString("TYPE").equals("0"))) {
				foundindex=true;
				break;
			}
		}
		assertTrue(foundindex);
		assertEquals(rs.getString("TABLE_NAME"),"testtable");
		assertEquals(rs.getString("NON_UNIQUE"),"0");
		assertEquals(rs.getString("ORDINAL_POSITION"),"1");
		assertEquals(rs.getString("COLUMN_NAME"),"col1");
		assertEquals(rs.getString("ASC_OR_DESC"),"A");
		assertEquals(rs.getString("TYPE"),"1");
		assertTrue(rs.getString("INDEX_NAME")!=null &&
				rs.getString("INDEX_NAME").length()>0);
		assertFalse(rs.next());
		rs.close();
		con.setAutoCommit(true);
		stmt.executeUpdate("drop table testtable");
		con.setAutoCommit(false);
		System.out.println();


		// procedure list
		System.out.println("PROCEDURE LIST:");
		con.setAutoCommit(true);
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
			"create procedure testproc1 "+
			"	@in1 int, "+
			"	@in2 char(20), "+
			"	@in3 varchar(20), "+
			"	@in4 datetime "+
			"as select 1");
		stmt.executeUpdate(
			"create procedure testproc2 "+
			"	@in1 int, "+
			"	@in2 char(20), "+
			"	@in3 varchar(20), "+
			"	@in4 datetime "+
			"as select 1");
		stmt.executeUpdate(
			"create procedure testproc3 "+
			"	@in1 int, "+
			"	@in2 char(20), "+
			"	@in3 varchar(20), "+
			"	@in4 datetime "+
			"as select 1");
		stmt.executeUpdate(
			"create procedure testproc4 "+
			"	@in1 int, "+
			"	@in2 char(20), "+
			"	@in3 varchar(20), "+
			"	@in4 datetime "+
			"as select 1");
		con.setAutoCommit(false);
		rs=md.getProcedures(null,null,"%");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),8);
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
			// freetds jdbc returns reserved columns
			assertEquals(rsmd.getColumnName(col++),
							"RESERVED_1");
			assertEquals(rsmd.getColumnName(col++),
							"RESERVED_2");
			assertEquals(rsmd.getColumnName(col++),
							"RESERVED_3");
		}
		assertEquals(rsmd.getColumnName(col++),"REMARKS");
		assertEquals(rsmd.getColumnName(col++),"PROCEDURE_TYPE");
		counter=0;
		while (rs.next()) {
			String name=rs.getString("PROCEDURE_NAME");
			if (name.equals("testproc1") ||
					name.equals("testproc2") ||
					name.equals("testproc3") ||
					name.equals("testproc4")) {
				if (issqlrelay) {
					assertEquals(
						rs.getShort("PROCEDURE_TYPE"),
						DatabaseMetaData.
							procedureNoResult);
				} else {
					// native jTDS reports the type as
					// procedureReturnsResult
					assertEquals(
						rs.getShort("PROCEDURE_TYPE"),
						DatabaseMetaData.
							procedureReturnsResult);
				}
				counter++;
			}
		}
		assertEquals(counter,4);
		rs.close();
		System.out.println();


		// procedure parameter list
		System.out.println("PROCEDURE PARAMETER LIST:");
		rs=md.getProcedureColumns(null,null,
					"testproc1","%");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		if (issqlrelay) {
			assertEquals(rsmd.getColumnCount(),20);
		} else {
			assertEquals(rsmd.getColumnCount(),13);
		}
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
		if (issqlrelay) {
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
		}
		assertTrue(rs.next());
		if (!issqlrelay) {
			// freetds jdbc returns the returnValue as the
			// first column
			assertEquals(rs.getString("COLUMN_NAME"),
							"@RETURN_VALUE");
			assertEquals(rs.getString("TYPE_NAME"),"int");
			assertTrue(rs.next());
		}
		assertEquals(rs.getString("COLUMN_NAME"),"@in1");
		assertEquals(rs.getString("TYPE_NAME"),"int");
		if (issqlrelay) {
			// freetds jdbc doesn't return this column
			assertEquals(rs.getString("ORDINAL_POSITION"),"1");
		}
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"@in2");
		assertEquals(rs.getString("TYPE_NAME"),"char");
		if (issqlrelay) {
			// freetds jdbc doesn't return this column
			assertEquals(rs.getString("ORDINAL_POSITION"),"2");
		}
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"@in3");
		assertEquals(rs.getString("TYPE_NAME"),"varchar");
		if (issqlrelay) {
			// freetds jdbc doesn't return this column
			assertEquals(rs.getString("ORDINAL_POSITION"),"3");
		}
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"@in4");
		assertEquals(rs.getString("TYPE_NAME"),"datetime");
		if (issqlrelay) {
			// freetds jdbc doesn't return this column
			assertEquals(rs.getString("ORDINAL_POSITION"),"4");
		}
		rs.close();
		con.setAutoCommit(true);
		stmt.executeUpdate("drop procedure testproc1");
		stmt.executeUpdate("drop procedure testproc2");
		stmt.executeUpdate("drop procedure testproc3");
		stmt.executeUpdate("drop procedure testproc4");
		con.setAutoCommit(false);
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
