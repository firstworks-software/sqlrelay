import java.sql.*;
import java.math.BigDecimal;
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

class sap extends sqlrtest {
	public static void main(String args[]) throws Exception {

		String classpath=System.getProperty("java.class.path");
		String hostname=InetAddress.getLocalHost().
					getHostName().split("\\.")[0];
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
			port=9000;
			socket=null;
			url="jdbc:sqlrelay://"+host+":"+port;
			user="testuser";
			password="testpassword";
			issqlrelay=true;
		} else if (classpath.contains("jconn")) {
			driver="com.sybase.jdbc4.jdbc.SybDriver";
			url="jdbc:sybase:Tds:sap:5000/"+hostname+"";
			user="testuser";
			password="testpassword";
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
		con=DriverManager.getConnection(url,user,password);
		assertTrue((con!=null));
		System.out.println();

		// close
		System.out.println("  close");
		assertFalse(con.isClosed());
		con.close();
		assertTrue(con.isClosed());
		con=DriverManager.getConnection(url,user,password);
		assertTrue((con!=null));
		assertFalse(con.isClosed());
		con.setAutoCommit(false);
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
		if (issqlrelay) {
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
		// varies by server version
		assertTrue(intval>=0);
		System.out.println();

		// getDatabaseMinorVersion
		System.out.println("  getDatabaseMinorVersion");
		intval=md.getDatabaseMinorVersion();
		System.out.println("    "+intval);
		// varies by server version
		assertTrue(intval>=0);
		System.out.println();

		// getDatabaseProductName
		System.out.println("  getDatabaseProductName");
		stringval=md.getDatabaseProductName();
		System.out.println("    "+stringval);
		if (issqlrelay) {
			assertEquals(stringval,"sap");
		} else {
			assertEquals(stringval,"Adaptive Server Enterprise");
		}
		System.out.println();

		// getDatabaseProductVersion
		System.out.println("  getDatabaseProductVersion");
		stringval=md.getDatabaseProductVersion();
		System.out.println("    "+stringval);
		// varies by server version
		assertTrue(stringval!=null||stringval==null);
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
		// varies by driver version
		assertTrue(intval>=0);
		System.out.println();

		// getDriverMinorVersion
		System.out.println("  getDriverMinorVersion");
		intval=md.getDriverMinorVersion();
		System.out.println("    "+intval);
		// varies by driver version
		assertTrue(intval>=0);
		System.out.println();

		// getDriverName
		System.out.println("  getDriverName");
		stringval=md.getDriverName();
		System.out.println("    "+stringval);
		if (issqlrelay) {
			assertEquals(stringval,"SQL Relay JDBC driver");
		} else {
			assertEquals(stringval,"jConnect (TM) for JDBC (TM)");
		}
		System.out.println();

		// getDriverVersion
		System.out.println("  getDriverVersion");
		stringval=md.getDriverVersion();
		System.out.println("    "+stringval);
		// varies by driver version
		assertTrue(stringval!=null);
		System.out.println();

		// getExtraNameCharacters
		System.out.println("  getExtraNameCharacters");
		stringval=md.getExtraNameCharacters();
		System.out.println("    "+stringval);
		assertEquals(stringval,"@#$£¥");
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
		// varies by driver version
		assertTrue(intval>=0);
		System.out.println();

		// getJDBCMinorVersion
		System.out.println("  getJDBCMinorVersion");
		intval=md.getJDBCMinorVersion();
		System.out.println("    "+intval);
		// varies by driver version
		assertTrue(intval>=0);
		System.out.println();

		// getMaxBinaryLiteralLength
		System.out.println("  getMaxBinaryLiteralLength");
		intval=md.getMaxBinaryLiteralLength();
		System.out.println("    "+intval);
		assertEquals(intval,255);
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
		assertEquals(intval,255);
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
		assertEquals(intval,0);
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
			// varies by sqlrelay config
			assertTrue(intval>0);
		} else {
			// sap jdbc returns 39985 for this
			assertEquals(intval,39985);
		}
		assertTrue(intval>=0);
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
		assertEquals(intval,0);
		System.out.println();

		// getMaxStatements
		System.out.println("  getMaxStatements");
		intval=md.getMaxStatements();
		System.out.println("    "+intval);
		assertEquals(intval,0);
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
		assertEquals(intval,256);
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
		assertEquals(stringval,"abs,acos,asin,atan,atan2,ceiling,cos,cot,degrees,exp,floor,log,log10,pi,power,radians,rand,round,sign,sin,sqrt,tan");
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
		RowIdLifetime	rowidlifetimeval=md.getRowIdLifetime();
		System.out.println("  "+rowidlifetimeval);
		assertEquals(rowidlifetimeval,RowIdLifetime.ROWID_UNSUPPORTED);
		System.out.println();

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
		assertEquals(stringval,"ARITH_OVERFLOW,BREAK,BROWSE,BULK,CHAR_CONVERT,CHECKPOINT,CLUSTERED,COMPRESSED,COMPUTE,CONFIRM,CONTROLROW,COUNT_BIG,DATABASE,DBCC,DECRYPT,DECRYPT_DEFAULT,DETERMINISTIC,DISK,DUAL_CONTROL,DUMMY,DUMP,ENCRYPT,ENDTRAN,ERRLVL,ERRORDATA,ERROREXIT,EXCLUSIVE,EXIT,EXP_ROW_SIZE,FILLFACTOR,HOLDLOCK,IDENTITY_GAP,IDENTITY_START,IF,INDEX,INOUT,INSTALL,JAR,KILL,LINENO,LOAD,LOB_COMPRESSION,LOCK,MANAGE,MATERIALIZED,MAX_ROWS_PER_PAGE,MIRROR,MIRROREXIT,MODIFY,NOHOLDLOCK,NONCLUSTERED,NUMERIC_TRUNCATION,OFF,OFFSETS,ONCE,ONLINE,OUT,OVER,PARTITION,PERM,PERMANENT,PLAN,PRINT,PROC,PROCESSEXIT,PROXY_TABLE,QUIESCE,RAISERROR,READPAST,READTEXT,RECONFIGURE,RELEASE_LOCKS_ON_CLOSE,REMOVE,REORG,REPLACE,REPLICATION,RESERVEPAGEGAP,RETURN,RETURNS,ROLE,ROWCOUNT,RULE,SAVE,SEMI_SENSITIVE,SETUSER,SHARED,SHUTDOWN,STATISTICS,STRINGSIZE,STRIPE,SYB_IDENTITY,SYB_RESTREE,SYB_TERMINATE,TEMP,TEXTSIZE,TRACEFILE,TRAN,TRIGGER,TRUNCATE,TSEQUAL,UNPARTITION,USE,USER_OPTION,WAITFOR,WHILE,WRITETEXT,XMLEXTRACT,XMLPARSE,XMLTEST");
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
		assertEquals(stringval,"ascii,char,char_length,character_length,concat,difference,insert,length,lcase,ltrim,octet_length,position,repeat,right,rtrim,soundex,space,substring,ucase");
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
		assertEquals(stringval,"curdate,curtime,current_date,current_time,current_timestamp,dayname,dayofmonth,dayofweek,dayofyear,extract,hour,minute,month,monthname,now,quarter,second,timestampadd,timestampdiff,week,year");
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
		assertTrue(boolval);
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
		assertTrue(boolval);
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
		assertTrue(boolval);
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
		assertTrue(boolval);
		System.out.println();

		System.out.println("  ownDeletesAreVisible "+
					"(scroll insensitive)");
		boolval=md.ownDeletesAreVisible(
					ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		System.out.println("  ownDeletesAreVisible "+
					"(scroll sensitive)");
		boolval=md.ownDeletesAreVisible(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("    "+boolval);
		assertFalse(boolval);
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
		assertFalse(boolval);
		System.out.println();

		System.out.println("  ownInsertsAreVisible "+
					"(scroll sensitive)");
		boolval=md.ownInsertsAreVisible(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("    "+boolval);
		assertFalse(boolval);
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
		assertFalse(boolval);
		System.out.println();

		System.out.println("  ownUpdatesAreVisible "+
					"(scroll sensitive)");
		boolval=md.ownUpdatesAreVisible(
					ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("    "+boolval);
		assertFalse(boolval);
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
		assertFalse(boolval);
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
		if (issqlrelay) {
			assertTrue(boolval);
		} else {
			assertFalse(boolval);
		}
		System.out.println();

		// supportsCoreSQLGrammar
		System.out.println("  supportsCoreSQLGrammar");
		boolval=md.supportsCoreSQLGrammar();
		System.out.println("    "+boolval);
		assertFalse(boolval);
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
		assertFalse(boolval);
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
		assertTrue(boolval);
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
		if (issqlrelay) {
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
		assertFalse(boolval);
		System.out.println();

		System.out.println("  supportsResultSetConcurrency "+
					"(scroll sensitive, updatable)");
		boolval=md.supportsResultSetConcurrency(
					ResultSet.TYPE_SCROLL_SENSITIVE,
					ResultSet.CONCUR_UPDATABLE);
		System.out.println("    "+boolval);
		assertFalse(boolval);
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
		assertFalse(boolval);
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
		assertFalse(boolval);
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
		assertFalse(boolval);
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
			"	testtext text, "+
			"	testurl varchar(60))"),0);
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
			"	1.1, "+
			"	1.1, "+
			"	1.1, "+
			"	1.1, "+
			"	1.10, "+
			"	1.10, "+
			"	'2001-01-01 01:00:00', "+
			"	'2001-01-01 01:00:00', "+
			"	'char1', "+
			"	'varchar1', "+
			"	1, "+
			"	'text1', "+
			"	'http://www.firstworks.com:8080/testurl1')"));
		assertEquals(stmt.getUpdateCount(),1);
		stmt.close();
		assertTrue(stmt.isClosed());
		System.out.println();


		// bind by position
		System.out.println("BIND BY POSITION:");
		if (issqlrelay) {
			pstmt=con.prepareStatement(
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
				"	@var16)");
		} else {
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
				"	?)");
		}
		assertFalse(pstmt.isClosed());
		for (int i=2; i<=4; i++) {
			pstmt.clearParameters();
			pstmt.setInt(1,i);
			pstmt.setInt(2,i);
			pstmt.setInt(3,i);
			pstmt.setDouble(4,i+0.1);
			pstmt.setDouble(5,i+0.1);
			pstmt.setBigDecimal(6,new BigDecimal(i+".1"));
			pstmt.setBigDecimal(7,new BigDecimal(i+".1"));
			pstmt.setBigDecimal(8,new BigDecimal(i+".10"));
			pstmt.setBigDecimal(9,new BigDecimal(i+".10"));

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
			pstmt.setString(15,"text"+i);
			pstmt.setString(16,
				"http://www.firstworks.com:8080/"+
				"testurl"+i);
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
		assertEquals(rsmd.getColumnCount(),16);
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
		assertEquals(rsmd.getColumnName(15),"testtext");
		assertEquals(rsmd.getColumnName(16),"testurl");
		System.out.println();


		// column types
		System.out.println("COLUMN TYPES:");
		assertTrue(rsmd.getColumnTypeName(1)!=null);
		assertTrue(rsmd.getColumnTypeName(2)!=null);
		assertTrue(rsmd.getColumnTypeName(3)!=null);
		assertTrue(rsmd.getColumnTypeName(4)!=null);
		assertTrue(rsmd.getColumnTypeName(5)!=null);
		assertTrue(rsmd.getColumnTypeName(6)!=null);
		assertTrue(rsmd.getColumnTypeName(7)!=null);
		assertTrue(rsmd.getColumnTypeName(8)!=null);
		assertTrue(rsmd.getColumnTypeName(9)!=null);
		assertTrue(rsmd.getColumnTypeName(10)!=null);
		assertTrue(rsmd.getColumnTypeName(11)!=null);
		assertTrue(rsmd.getColumnTypeName(12)!=null);
		assertTrue(rsmd.getColumnTypeName(13)!=null);
		assertTrue(rsmd.getColumnTypeName(14)!=null);
		assertTrue(rsmd.getColumnTypeName(15)!=null);
		assertTrue(rsmd.getColumnTypeName(16)!=null);
		System.out.println();


		// column length
		System.out.println("COLUMN LENGTH:");
		assertTrue(rsmd.getPrecision(1)>=0);
		assertTrue(rsmd.getPrecision(2)>=0);
		assertTrue(rsmd.getPrecision(3)>=0);
		assertTrue(rsmd.getPrecision(4)>=0);
		assertTrue(rsmd.getPrecision(5)>=0);
		assertTrue(rsmd.getPrecision(6)>=0);
		assertTrue(rsmd.getPrecision(7)>=0);
		assertTrue(rsmd.getPrecision(8)>=0);
		assertTrue(rsmd.getPrecision(9)>=0);
		assertTrue(rsmd.getPrecision(10)>=0);
		assertTrue(rsmd.getPrecision(11)>=0);
		assertTrue(rsmd.getPrecision(12)>=0);
		assertTrue(rsmd.getPrecision(13)>=0);
		assertTrue(rsmd.getPrecision(14)>=0);
		assertTrue(rsmd.getPrecision(15)>=0);
		assertTrue(rsmd.getPrecision(16)>=0);
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
			assertEquals(rs.getString(6),i+".1");
			assertFalse(rs.wasNull());
			System.out.println();

			// numeric
			System.out.println("  row "+i+" - numeric");
			assertEquals(rs.getString(7),i+".1");
			assertFalse(rs.wasNull());
			System.out.println();

			// money
			System.out.println("  row "+i+" - money");
			assertEquals(rs.getString(8),
				i+((issqlrelay)?".10":".1000"));
			assertFalse(rs.wasNull());
			System.out.println();

			// smallmoney
			System.out.println("  row "+i+" - smallmoney");
			assertEquals(rs.getString(9),
				i+((issqlrelay)?".10":".1000"));
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

			// text as string
			System.out.println("  row "+i+" - text as string");
			assertEquals(rs.getString(15),"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// text as clob
			System.out.println("  row "+i+" - text as clob");
			clob=rs.getClob(15);
			assertEquals(clob.getSubString(1,(int)clob.length()),
								"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// text as ascii stream
			System.out.println("  row "+i+
					" - text as ascii stream");
			assertEquals(new String(streamToBytes(rs.getAsciiStream(15)),"UTF-8"),
						"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// text as character stream
			System.out.println("  row "+i+
					" - text as character stream");
			StringWriter sw=new StringWriter();
			readerToWriter(rs.getCharacterStream(15),sw);
			assertEquals(sw.toString(),"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// url
			System.out.println("  row "+i+" - url");
			if (issqlrelay) {
				// sap jdbc doesn't implement getURL
				URL	urlvar=rs.getURL(16);
				assertEquals(urlvar.getProtocol(),"http");
				assertEquals(urlvar.getHost(),
						"www.firstworks.com");
				assertEquals(urlvar.getPort(),8080);
				assertEquals(urlvar.getPath(),"/testurl"+i);
				assertFalse(rs.wasNull());
			}
			System.out.println();
		}
		rs.close();
		assertTrue(rs.isClosed());
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
			assertEquals(rs.getString("testdecimal"),i+".1");
			assertFalse(rs.wasNull());
			System.out.println();

			// numeric
			System.out.println("  row "+i+" - numeric");
			assertEquals(rs.getString("testnumeric"),i+".1");
			assertFalse(rs.wasNull());
			System.out.println();

			// money
			System.out.println("  row "+i+" - money");
			assertEquals(rs.getString("testmoney"),
				i+((issqlrelay)?".10":".1000"));
			assertFalse(rs.wasNull());
			System.out.println();

			// smallmoney
			System.out.println("  row "+i+" - smallmoney");
			assertEquals(rs.getString("testsmallmoney"),
				i+((issqlrelay)?".10":".1000"));
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

			// text as string
			System.out.println("  row "+i+" - text as string");
			assertEquals(rs.getString("testtext"),"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// text as clob
			System.out.println("  row "+i+" - text as clob");
			clob=rs.getClob("testtext");
			assertEquals(clob.getSubString(1,(int)clob.length()),
								"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// text as ascii stream
			System.out.println("  row "+i+
					" - text as ascii stream");
			assertEquals(new String(streamToBytes(rs.getAsciiStream("testtext")),"UTF-8"),
						"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// text as character stream
			System.out.println("  row "+i+
					" - text as character stream");
			StringWriter sw=new StringWriter();
			readerToWriter(rs.getCharacterStream("testtext"),sw);
			assertEquals(sw.toString(),"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// url
			System.out.println("  row "+i+" - url");
			if (issqlrelay) {
				// sap jdbc doesn't implement getURL
				URL	urlvar=rs.getURL("testurl");
				assertEquals(urlvar.getProtocol(),"http");
				assertEquals(urlvar.getHost(),
						"www.firstworks.com");
				assertEquals(urlvar.getPort(),8080);
				assertEquals(urlvar.getPath(),"/testurl"+i);
				assertFalse(rs.wasNull());
			}
			System.out.println();
		}


		// row count
		System.out.println("ROW COUNT:");
		assertEquals(rs.getRow(),4);
		rs.close();
		stmt.close();
		assertTrue(stmt.isClosed());
		System.out.println();


		// commit
		System.out.println("COMMIT:");
		con.commit();
		System.out.println();


		// output bind by position
		System.out.println("OUTPUT BIND BY POSITION:");
		stmt=con.createStatement();
		con.setAutoCommit(true);
		try {
			stmt.executeUpdate("drop procedure testproc2");
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate(
			"create procedure testproc2 "+
			"	@in1 int, "+
			"	@in2 float, "+
			"	@in3 varchar(20), "+
			"	@out1 int output, "+
			"	@out2 float output, "+
			"	@out3 varchar(20) output "+
			"as "+
			"	select @out1=@in1, "+
			"	@out2=@in2, "+
			"	@out3=@in3"),0);
		if (issqlrelay) {
			cstmt=con.prepareCall("exec testproc2");
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
		} else {
			cstmt=con.prepareCall(
				"exec testproc2 ?,?,?,?,?,?");
			cstmt.setInt(1,1);
			cstmt.setDouble(2,1.1);
			cstmt.setString(3,"hello");
			cstmt.registerOutParameter(4,Types.INTEGER);
			cstmt.registerOutParameter(5,Types.DOUBLE);
			cstmt.registerOutParameter(6,Types.VARCHAR);
			assertFalse(cstmt.execute());
			assertEquals(cstmt.getInt(4),1);
			assertEquals(cstmt.getDouble(5),1.1);
			assertEquals(cstmt.getString(6),"hello");
		}
		cstmt.close();
		stmt.executeUpdate("drop procedure testproc2");
		con.setAutoCommit(false);
		System.out.println();


		// output bind by name
		System.out.println("OUTPUT BIND BY NAME:");
		if (issqlrelay) {
			con.setAutoCommit(true);
			try {
				stmt.executeUpdate(
						"drop procedure testproc2");
			} catch (Exception ex) {
			}
			assertEquals(stmt.executeUpdate(
				"create procedure testproc2 "+
				"	@in1 int, "+
				"	@in2 float, "+
				"	@in3 varchar(20), "+
				"	@out1 int output, "+
				"	@out2 float output, "+
				"	@out3 varchar(20) output "+
				"as "+
				"	select @out1=@in1, "+
				"	@out2=@in2, "+
				"	@out3=@in3"),0);
			cstmt=con.prepareCall("exec testproc2");
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
			stmt.executeUpdate("drop procedure testproc2");
			con.setAutoCommit(false);
		} else {
			// sap jdbc doesn't allow mixing parameter
			// setting by index and by name
			assertTrue(true);
		}
		System.out.println();


		// stored procedures
		System.out.println("STORED PROCEDURES:");
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
		if (issqlrelay) {
			cstmt=con.prepareCall("exec testproc");
			cstmt.setInt("in1",1);
			cstmt.setDouble("in2",1.1);
			cstmt.setString("in3","hello");
			cstmt.registerOutParameter("out1",Types.INTEGER);
			cstmt.registerOutParameter("out2",Types.DOUBLE);
			cstmt.registerOutParameter("out3",Types.VARCHAR);
			cstmt.execute();
			assertEquals(cstmt.getInt("out1"),1);
			assertEquals(cstmt.getString("out3"),"hello");
		} else {
			cstmt=con.prepareCall(
				"{call testproc(?,?,?,?,?,?)}");
			cstmt.setInt(1,1);
			cstmt.setDouble(2,1.1);
			cstmt.setString(3,"hello");
			cstmt.registerOutParameter(4,Types.INTEGER);
			cstmt.registerOutParameter(5,Types.DOUBLE);
			cstmt.registerOutParameter(6,Types.VARCHAR);
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
		if (issqlrelay) {
			assertEquals(rsmd.getColumnName(col++),"TABLE_CAT");
		} else {
			// sap jdbc names this column differently
			assertEquals(rsmd.getColumnName(col++),"name");
		}
		found=false;
		while (rs.next()) {
			String	tschem=rs.getString(1);
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
		if (issqlrelay) {
			assertEquals(rsmd.getColumnName(col++),"TABLE_SCHEM");
		} else {
			// sap jdbc names this column differently
			assertEquals(rsmd.getColumnName(col++),"name");
		}
		assertEquals(rsmd.getColumnName(col++),"TABLE_CATALOG");
		found=false;
		while (rs.next()) {
			String	tschem=rs.getString(1);
			if (tschem!=null && tschem.equals(user)) {
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
		counter=0;
		while (rs.next()) {
			String ttname=rs.getString(1).trim();
			if (ttname.equals("TABLE") ||
				ttname.equals("SYSTEM TABLE") ||
				ttname.equals("VIEW")) {
				counter++;
			}
		}
		assertEquals(counter,3);
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
		if (issqlrelay) {
			assertEquals(rsmd.getColumnCount(),18);
			col=1;
			assertEquals(rsmd.getColumnName(col++),
						"TYPE_NAME");
			assertEquals(rsmd.getColumnName(col++),
						"DATA_TYPE");
			assertEquals(rsmd.getColumnName(col++),
						"PRECISION");
			assertEquals(rsmd.getColumnName(col++),
						"LITERAL_PREFIX");
			assertEquals(rsmd.getColumnName(col++),
						"LITERAL_SUFFIX");
			assertEquals(rsmd.getColumnName(col++),
						"CREATE_PARAMS");
			assertEquals(rsmd.getColumnName(col++),
						"NULLABLE");
			assertEquals(rsmd.getColumnName(col++),
						"CASE_SENSITIVE");
			assertEquals(rsmd.getColumnName(col++),
						"SEARCHABLE");
			assertEquals(rsmd.getColumnName(col++),
						"UNSIGNED_ATTRIBUTE");
			assertEquals(rsmd.getColumnName(col++),
						"FIXED_PREC_SCALE");
			assertEquals(rsmd.getColumnName(col++),
						"AUTO_INCREMENT");
			assertEquals(rsmd.getColumnName(col++),
						"LOCAL_TYPE_NAME");
			assertEquals(rsmd.getColumnName(col++),
						"MINIMUM_SCALE");
			assertEquals(rsmd.getColumnName(col++),
						"MAXIMUM_SCALE");
			assertEquals(rsmd.getColumnName(col++),
						"SQL_DATA_TYPE");
			assertEquals(rsmd.getColumnName(col++),
						"SQL_DATETIME_SUB");
			assertEquals(rsmd.getColumnName(col++),
						"NUM_PREC_RADIX");
		} else {
			// sap jdbc returns 19 columns with different names
			assertEquals(rsmd.getColumnCount(),19);
			col=1;
			assertEquals(rsmd.getColumnName(col++),
						"TYPE_NAME");
			assertEquals(rsmd.getColumnName(col++),
						"data_type");
			assertEquals(rsmd.getColumnName(col++),
						"PRECISION");
			assertEquals(rsmd.getColumnName(col++),
						"literal_prefix");
			assertEquals(rsmd.getColumnName(col++),
						"literal_suffix");
			assertEquals(rsmd.getColumnName(col++),
						"create_params");
			assertEquals(rsmd.getColumnName(col++),
						"nullable");
			assertEquals(rsmd.getColumnName(col++),
						"case_sensitive");
			assertEquals(rsmd.getColumnName(col++),
						"searchable");
			assertEquals(rsmd.getColumnName(col++),
						"unsigned_attribute");
			assertEquals(rsmd.getColumnName(col++),
						"money");
			assertEquals(rsmd.getColumnName(col++),
						"auto_increment");
			assertEquals(rsmd.getColumnName(col++),
						"local_type_name");
			assertEquals(rsmd.getColumnName(col++),
						"minimum_scale");
			assertEquals(rsmd.getColumnName(col++),
						"maximum_scale");
			assertEquals(rsmd.getColumnName(col++),
						"sql_data_type");
			assertEquals(rsmd.getColumnName(col++),
						"sql_datetime_sub");
			assertEquals(rsmd.getColumnName(col++),
						"num_prec_radix");
			assertEquals(rsmd.getColumnName(col++),
						"interval_precision");
		}
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
			"	testurl varchar(60))");
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
		assertEquals(rs.getString("COLUMN_NAME"),"testint");
		assertEquals(rs.getString("TYPE_NAME"),
					"int");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testsmallint");
		assertEquals(rs.getString("TYPE_NAME"),
					"smallint");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testtinyint");
		assertEquals(rs.getString("TYPE_NAME"),
					"tinyint");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testreal");
		assertEquals(rs.getString("TYPE_NAME"),
					"real");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testfloat");
		assertEquals(rs.getString("TYPE_NAME"),
					"float");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testdecimal");
		assertEquals(rs.getString("TYPE_NAME"),
					"decimal");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testnumeric");
		assertEquals(rs.getString("TYPE_NAME"),
					"numeric");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testmoney");
		assertEquals(rs.getString("TYPE_NAME"),
					"money");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testsmallmoney");
		assertEquals(rs.getString("TYPE_NAME"),
					"smallmoney");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testdatetime");
		assertEquals(rs.getString("TYPE_NAME"),
					"datetime");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testsmalldatetime");
		assertEquals(rs.getString("TYPE_NAME"),
				"smalldatetime");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testchar");
		assertEquals(rs.getString("TYPE_NAME"),
					"char");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testvarchar");
		assertEquals(rs.getString("TYPE_NAME"),
					"varchar");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testbit");
		assertEquals(rs.getString("TYPE_NAME"),
					"bit");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testurl");
		assertEquals(rs.getString("TYPE_NAME"),
					"varchar");
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
			// sap jdbc may return null for PK_NAME
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
		// skip statistics rows (TYPE=0) returned by sap jdbc
		boolean foundindex=false;
		while (rs.next()) {
			if (!(rs.getString("TYPE").equals("0"))) {
				foundindex=true;
				break;
			}
		}
		assertTrue(foundindex);
		assertEquals(rs.getString("TABLE_NAME"),"testtable");
		assertEquals(rs.getString("NON_UNIQUE"),"FALSE");
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
		if (issqlrelay) {
			assertEquals(rsmd.getColumnCount(),8);
		} else {
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
			// sap jdbc returns these columns with lowercase names
			assertEquals(rsmd.getColumnName(col++),
						"num_input_params");
			assertEquals(rsmd.getColumnName(col++),
						"num_output_params");
			assertEquals(rsmd.getColumnName(col++),
						"num_result_sets");
		}
		assertEquals(rsmd.getColumnName(col++),"REMARKS");
		assertEquals(rsmd.getColumnName(col++),"PROCEDURE_TYPE");
		if (!issqlrelay) {
			// sap jdbc returns this column too
			assertEquals(rsmd.getColumnName(col++),"SPECIFIC_NAME");
		}
		counter=0;
		while (rs.next()) {
			String name=rs.getString("PROCEDURE_NAME");
			if (issqlrelay) {
				if (name.equals("testproc1") ||
					name.equals("testproc2") ||
					name.equals("testproc3") ||
					name.equals("testproc4")) {
					counter++;
				}
			} else {
				// sap jdbc may append version
				// suffixes like ";1"
				if (name.startsWith("testproc1") ||
					name.startsWith("testproc2") ||
					name.startsWith("testproc3") ||
					name.startsWith("testproc4")) {
					counter++;
				}
			}
		}
		assertEquals(counter,4);
		rs.close();
		System.out.println();


		// procedure parameter list
		System.out.println("PROCEDURE PARAMETER LIST:");
		rs=md.getProcedureColumns(null,null,"testproc1","%");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),20);
		col=1;
		assertEquals(rsmd.getColumnName(col++),"PROCEDURE_CAT");
		assertEquals(rsmd.getColumnName(col++),"PROCEDURE_SCHEM");
		assertEquals(rsmd.getColumnName(col++),"PROCEDURE_NAME");
		assertEquals(rsmd.getColumnName(col++),"COLUMN_NAME");
		assertEquals(rsmd.getColumnName(col++),"COLUMN_TYPE");
		assertEquals(rsmd.getColumnName(col++),"DATA_TYPE");
		assertEquals(rsmd.getColumnName(col++),"TYPE_NAME");
		assertEquals(rsmd.getColumnName(col++),"PRECISION");
		assertEquals(rsmd.getColumnName(col++),"LENGTH");
		assertEquals(rsmd.getColumnName(col++),"SCALE");
		assertEquals(rsmd.getColumnName(col++),"RADIX");
		assertEquals(rsmd.getColumnName(col++),"NULLABLE");
		assertEquals(rsmd.getColumnName(col++),"REMARKS");
		assertEquals(rsmd.getColumnName(col++),"COLUMN_DEF");
		assertEquals(rsmd.getColumnName(col++),"SQL_DATA_TYPE");
		assertEquals(rsmd.getColumnName(col++),"SQL_DATETIME_SUB");
		assertEquals(rsmd.getColumnName(col++),"CHAR_OCTET_LENGTH");
		assertEquals(rsmd.getColumnName(col++),"ORDINAL_POSITION");
		assertEquals(rsmd.getColumnName(col++),"IS_NULLABLE");
		assertEquals(rsmd.getColumnName(col++),"SPECIFIC_NAME");
		if (!issqlrelay) {
			// skip RETURN_VALUE row returned by sap jdbc
			assertTrue(rs.next());
		}
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"@in1");
		assertEquals(rs.getString("TYPE_NAME"),"int");
		if (issqlrelay) {
			// sap jdbc doesn't return this column
			assertEquals(rs.getString("ORDINAL_POSITION"),"1");
		}
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"@in2");
		assertEquals(rs.getString("TYPE_NAME"),"char");
		if (issqlrelay) {
			// sap jdbc doesn't return this column
			assertEquals(rs.getString("ORDINAL_POSITION"),"2");
		}
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"@in3");
		assertEquals(rs.getString("TYPE_NAME"),"varchar");
		if (issqlrelay) {
			// sap jdbc doesn't return this column
			assertEquals(rs.getString("ORDINAL_POSITION"),"3");
		}
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"@in4");
		assertEquals(rs.getString("TYPE_NAME"),"datetime");
		if (issqlrelay) {
			// sap jdbc doesn't return this column
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
