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

class mysql extends sqlrtest {

	public static void main(String args[]) throws Exception {

		// This test supports both the sqlrelay jdbc driver and the
		// database native jdbc driver, selecting one based on the
		// classpath.
		String	classpath=System.getProperty("java.class.path");
		String	hostname=InetAddress.getLocalHost().
					getHostName().split("\\.")[0].toLowerCase();
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
			port=9002;
			socket=null;
			url="jdbc:sqlrelay://"+host+":"+port;
			user="testuser";
			password="testpassword";
			issqlrelay=true;
		} else if (classpath.contains("mysql-connector")) {
			driver="com.mysql.cj.jdbc.Driver";
			url="jdbc:mysql://mysql:3306/"+hostname;
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
		System.out.println("  getConnection");
		DriverManager.getDrivers();
		Class.forName(driver);
		con=DriverManager.getConnection(url,props);
		assertTrue((con!=null));
		System.out.println();

		// close
		System.out.println("  close");
		assertFalse(con.isClosed());
		assertTrue(con.isValid(0));
		con.close();
		assertTrue(con.isClosed());
		assertFalse(con.isValid(0));
		con=DriverManager.getConnection(url,props);
		assertTrue((con!=null));
		System.out.println();

		// network timeout
		System.out.println("  network timeout");
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
			System.out.println("  connection");
			SQLRelayConnection	sqlrcon=(SQLRelayConnection)con;
			assertEquals(sqlrcon.getHost(),host);
			assertEquals(sqlrcon.getPort(),port);
			assertEquals(sqlrcon.getSocket(),socket);
			assertEquals(sqlrcon.getUser(),user);
			assertEquals(sqlrcon.getPassword(),password);
			System.out.println();

			// unwrap
			System.out.println("  unwrap");
			assertEquals(
				con.isWrapperFor(SQLRConnection.class),1);
			assertEquals(
				(con.unwrap(SQLRConnection.class)!=null),1);
			System.out.println();
		}

		// catalog
		System.out.println("  catalog");
		String	originalcatalog=con.getCatalog();
		con.setCatalog(hostname);
		con.setCatalog(originalcatalog);
		System.out.println();

		// schema
		System.out.println("  schema");
		String	originalschema=con.getSchema();
		System.out.println();

		// client info
		System.out.println("  client info");
		Properties	inprop=new Properties();
		inprop.setProperty("prop1","value1");
		inprop.setProperty("prop2","value2");
		con.setClientInfo(inprop);
		con.setClientInfo("prop3","value3");
		System.out.println();

		// readonly
		System.out.println("  readonly");
		con.setReadOnly(true);
		assertTrue(con.isReadOnly());
		con.setReadOnly(false);
		assertTrue(!con.isReadOnly());
		System.out.println();

		// autocommit
		System.out.println("  autocommit");
		con.setAutoCommit(true);
		assertTrue(con.getAutoCommit());
		con.setAutoCommit(false);
		assertTrue(!con.getAutoCommit());
		con.setAutoCommit(true);
		System.out.println();

		// holdability
		System.out.println("  holdability");
		con.setHoldability(ResultSet.HOLD_CURSORS_OVER_COMMIT);
		assertEquals(con.getHoldability()==
				ResultSet.HOLD_CURSORS_OVER_COMMIT,1);
		try {
			con.setHoldability(ResultSet.CLOSE_CURSORS_AT_COMMIT);
			if (issqlrelay) {
				assertTrue(false);
			} else {
				// mysql jdbc will happily let you
				// set an unsupported holdability
			}
		} catch (Exception ex) {
			assertTrue(true);
		}
		System.out.println();

		// backend major version (derived from the backend dbVersion);
		// used below to gate features the old mysql 3.23 backend lacks
		int	majorversion=con.getMetaData().getDatabaseMajorVersion();

		// isolation levels
		System.out.println("  isolation levels");

		// mysql before 4.0 doesn't support setting the isolation level
		if (majorversion>3) {
			// mysql supports all four isolation levels
			// (autocommit is on here, so no commit() between changes -
			// the native driver rejects commit() in autocommit mode)
			con.setTransactionIsolation(
				Connection.TRANSACTION_READ_UNCOMMITTED);
			assertEquals(con.getTransactionIsolation(),
				Connection.TRANSACTION_READ_UNCOMMITTED);

			con.setTransactionIsolation(
				Connection.TRANSACTION_READ_COMMITTED);
			assertEquals(con.getTransactionIsolation(),
				Connection.TRANSACTION_READ_COMMITTED);

			con.setTransactionIsolation(
				Connection.
				TRANSACTION_REPEATABLE_READ);
			assertEquals(con.getTransactionIsolation(),
				Connection.TRANSACTION_REPEATABLE_READ);

			con.setTransactionIsolation(
				Connection.TRANSACTION_SERIALIZABLE);
			assertEquals(con.getTransactionIsolation(),
				Connection.TRANSACTION_SERIALIZABLE);

			// reset to default
			con.setTransactionIsolation(
				Connection.TRANSACTION_REPEATABLE_READ);
		}
		System.out.println();

		// warnings
		System.out.println("  warnings");
		assertTrue(con.getWarnings()==null);
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

		// unwrap
		if (issqlrelay) {
			System.out.println("  unwrap");
			assertEquals(md.isWrapperFor(SQLRConnection.class),1);
			System.out.println();
			assertEquals((md.unwrap(SQLRConnection.class)!=null),1);
			System.out.println();
		}

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
		assertTrue(boolval);
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
		assertTrue(boolval);
		System.out.println();

		// generatedKeyAlwaysReturned
		System.out.println("  generatedKeyAlwaysReturned");
		boolval=md.generatedKeyAlwaysReturned();
		System.out.println("    "+boolval);
		assertTrue(boolval);
		System.out.println();

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
			assertEquals(stringval,"mysql");
		} else {
			assertEquals(stringval,"MySQL");
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
		assertEquals(intval,4);
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
			assertEquals(stringval,"MySQL Connector/J");
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
		assertEquals(stringval,"`");
		System.out.println();

		// getJDBCMajorVersion
		System.out.println("  getJDBCMajorVersion");
		intval=md.getJDBCMajorVersion();
		System.out.println("    "+intval);
		// varies by driver version
		assertTrue(intval>=1);
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
		assertEquals(intval,16777208);
		System.out.println();

		// getMaxCatalogNameLength
		System.out.println("  getMaxCatalogNameLength");
		intval=md.getMaxCatalogNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,32);
		System.out.println();

		// getMaxCharLiteralLength
		System.out.println("  getMaxCharLiteralLength");
		intval=md.getMaxCharLiteralLength();
		System.out.println("    "+intval);
		assertEquals(intval,16777208);
		System.out.println();

		// getMaxColumnNameLength
		System.out.println("  getMaxColumnNameLength");
		intval=md.getMaxColumnNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,64);
		System.out.println();

		// getMaxColumnsInGroupBy
		System.out.println("  getMaxColumnsInGroupBy");
		intval=md.getMaxColumnsInGroupBy();
		System.out.println("    "+intval);
		assertEquals(intval,64);
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
		assertEquals(intval,64);
		System.out.println();

		// getMaxColumnsInSelect
		System.out.println("  getMaxColumnsInSelect");
		intval=md.getMaxColumnsInSelect();
		System.out.println("    "+intval);
		assertEquals(intval,256);
		System.out.println();

		// getMaxColumnsInTable
		System.out.println("  getMaxColumnsInTable");
		intval=md.getMaxColumnsInTable();
		System.out.println("    "+intval);
		assertEquals(intval,512);
		System.out.println();

		// getMaxConnections
		System.out.println("  getMaxConnections");
		intval=md.getMaxConnections();
		System.out.println("    "+intval);
		if (issqlrelay) {
			// varies by sqlrelay config
			assertTrue(intval>0);
		} else {
			// oracle jdbc returns 0 for this
			assertEquals(intval,0);
		}
		System.out.println();

		// getMaxCursorNameLength
		System.out.println("  getMaxCursorNameLength");
		intval=md.getMaxCursorNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,64);
		System.out.println();

		// getMaxIndexLength
		System.out.println("  getMaxIndexLength");
		intval=md.getMaxIndexLength();
		System.out.println("    "+intval);
		assertEquals(intval,256);
		System.out.println();

		// getMaxProcedureNameLength
		System.out.println("  getMaxProcedureNameLength");
		intval=md.getMaxProcedureNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxRowSize
		System.out.println("  getMaxRowSize");
		intval=md.getMaxRowSize();
		System.out.println("    "+intval);
		assertEquals(intval,2147483639);
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
		assertEquals(intval,65531);
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
		assertEquals(intval,64);
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
		assertEquals(intval,16);
		System.out.println();

		// getNumericFunctions
		System.out.println("  getNumericFunctions");
		stringval=md.getNumericFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"ABS,ACOS,ASIN,ATAN,ATAN2,BIT_COUNT,CEILING,COS,COT,DEGREES,EXP,FLOOR,LOG,LOG10,MAX,MIN,MOD,PI,POW,POWER,RADIANS,RAND,ROUND,SIN,SQRT,TAN,TRUNCATE");
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
		assertEquals(intval,ResultSet.HOLD_CURSORS_OVER_COMMIT);
		System.out.println();

		// getRowIdLifetime
		System.out.println("  getRowIdLifetime");
		RowIdLifetime	rowidlifetimeval=md.getRowIdLifetime();
		System.out.println("  "+rowidlifetimeval);
		// varies by driver
		assertTrue(rowidlifetimeval!=null);
		System.out.println();

		// getSchemaTerm
		System.out.println("  getSchemaTerm");
		stringval=md.getSchemaTerm();
		System.out.println("    "+stringval);
		assertEquals(stringval,"");
		System.out.println();

		// getSearchStringEscape
		System.out.println("  getSearchStringEscape");
		stringval=md.getSearchStringEscape();
		System.out.println("    "+stringval);
		// varies by driver
		assertTrue(stringval!=null && stringval.length()>0);
		System.out.println();

		// getSQLKeywords
		System.out.println("  getSQLKeywords");
		stringval=md.getSQLKeywords();
		System.out.println("    "+stringval);
		assertEquals(stringval,"ACCESSIBLE,ADD,ANALYZE,ASC,BEFORE,CASCADE,CHANGE,CONTINUE,DATABASE,DATABASES,DAY_HOUR,DAY_MICROSECOND,DAY_MINUTE,DAY_SECOND,DELAYED,DESC,DISTINCTROW,DIV,DUAL,ELSEIF,EMPTY,ENCLOSED,ESCAPED,EXIT,EXPLAIN,FIRST_VALUE,FLOAT4,FLOAT8,FORCE,FULLTEXT,GENERATED,GROUPS,HIGH_PRIORITY,HOUR_MICROSECOND,HOUR_MINUTE,HOUR_SECOND,IF,IGNORE,INDEX,INFILE,INT1,INT2,INT3,INT4,INT8,IO_AFTER_GTIDS,IO_BEFORE_GTIDS,ITERATE,JSON_TABLE,KEY,KEYS,KILL,LAG,LAST_VALUE,LEAD,LEAVE,LIMIT,LINEAR,LINES,LOAD,LOCK,LONG,LONGBLOB,LONGTEXT,LOOP,LOW_PRIORITY,MANUAL,MASTER_BIND,MASTER_SSL_VERIFY_SERVER_CERT,MAXVALUE,MEDIUMBLOB,MEDIUMINT,MEDIUMTEXT,MIDDLEINT,MINUTE_MICROSECOND,MINUTE_SECOND,NO_WRITE_TO_BINLOG,NTH_VALUE,NTILE,OPTIMIZE,OPTIMIZER_COSTS,OPTION,OPTIONALLY,OUTFILE,PARALLEL,PURGE,QUALIFY,READ,READ_WRITE,REGEXP,RENAME,REPEAT,REPLACE,REQUIRE,RESIGNAL,RESTRICT,RLIKE,SCHEMA,SCHEMAS,SECOND_MICROSECOND,SEPARATOR,SHOW,SIGNAL,SPATIAL,SQL_BIG_RESULT,SQL_CALC_FOUND_ROWS,SQL_SMALL_RESULT,SSL,STARTING,STORED,STRAIGHT_JOIN,TERMINATED,TINYBLOB,TINYINT,TINYTEXT,UNDO,UNLOCK,UNSIGNED,USAGE,USE,UTC_DATE,UTC_TIME,UTC_TIMESTAMP,VARBINARY,VARCHARACTER,VIRTUAL,WHILE,WRITE,XOR,YEAR_MONTH,ZEROFILL");
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
		assertEquals(stringval,"ASCII,BIN,BIT_LENGTH,CHAR,CHARACTER_LENGTH,CHAR_LENGTH,CONCAT,CONCAT_WS,CONV,ELT,EXPORT_SET,FIELD,FIND_IN_SET,HEX,INSERT,INSTR,LCASE,LEFT,LENGTH,LOAD_FILE,LOCATE,LOCATE,LOWER,LPAD,LTRIM,MAKE_SET,MATCH,MID,OCT,OCTET_LENGTH,ORD,POSITION,QUOTE,REPEAT,REPLACE,REVERSE,RIGHT,RPAD,RTRIM,SOUNDEX,SPACE,STRCMP,SUBSTRING,SUBSTRING,SUBSTRING,SUBSTRING,SUBSTRING_INDEX,TRIM,UCASE,UPPER");
		System.out.println();

		// getSystemFunctions
		System.out.println("  getSystemFunctions");
		stringval=md.getSystemFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"DATABASE,USER,SYSTEM_USER,SESSION_USER,PASSWORD,ENCRYPT,LAST_INSERT_ID,VERSION");
		System.out.println();

		// getTimeDateFunctions
		System.out.println("  getTimeDateFunctions");
		stringval=md.getTimeDateFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"DAYOFWEEK,WEEKDAY,DAYOFMONTH,DAYOFYEAR,MONTH,DAYNAME,MONTHNAME,QUARTER,WEEK,YEAR,HOUR,MINUTE,SECOND,PERIOD_ADD,PERIOD_DIFF,TO_DAYS,FROM_DAYS,DATE_FORMAT,TIME_FORMAT,CURDATE,CURRENT_DATE,CURTIME,CURRENT_TIME,NOW,SYSDATE,CURRENT_TIMESTAMP,UNIX_TIMESTAMP,FROM_UNIXTIME,SEC_TO_TIME,TIME_TO_SEC");
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
		if (issqlrelay) {
			assertEquals(stringval,user);
		} else {
			// mysql returns user@host
			assertTrue(stringval.startsWith(user));
		}
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
		assertFalse(boolval);
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
		if (issqlrelay) {
			// SQL Relay reports SENSITIVE identifier storage,
			// which is mutually exclusive with mixed-case storage;
			// the native MariaDB driver reports both
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
		System.out.println();

		// storesMixedCaseQuotedIdentifiers
		System.out.println("  storesMixedCaseQuotedIdentifiers");
		boolval=md.storesMixedCaseQuotedIdentifiers();
		System.out.println("    "+boolval);
		if (issqlrelay) {
			// SQL Relay reports SENSITIVE quoted-identifier storage,
			// which is mutually exclusive with mixed-case storage;
			// the native MariaDB driver reports both
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
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
		assertFalse(boolval);
		System.out.println();

		// supportsConvert (with types)
		System.out.println("  supportsConvert (with types)");
		boolval=md.supportsConvert(Types.INTEGER, Types.VARCHAR);
		System.out.println("    "+boolval);
		if (issqlrelay) {
			// sqlrelay jdbc doesn't currently support this
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
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

		// supportsDataDefinitionAndDataManipulationTransactions
		System.out.println(
		"supportsDataDefinitionAndDataManipulationTransactions");
		boolval=
		md.supportsDataDefinitionAndDataManipulationTransactions();
		System.out.println("    "+boolval);
		assertFalse(boolval);
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
		assertTrue(boolval);
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
			// sqlrelay doesn't support multiple open result sets
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
		assertFalse(boolval);
		System.out.println();

		// supportsOpenStatementsAcrossRollback
		System.out.println("  supportsOpenStatementsAcrossRollback");
		boolval=md.supportsOpenStatementsAcrossRollback();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// supportsOrderByUnrelated
		System.out.println("  supportsOrderByUnrelated");
		boolval=md.supportsOrderByUnrelated();
		System.out.println("    "+boolval);
		assertFalse(boolval);
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
		assertFalse(boolval);
		System.out.println();

		// supportsPositionedUpdate
		System.out.println("  supportsPositionedUpdate");
		boolval=md.supportsPositionedUpdate();
		System.out.println("    "+boolval);
		assertFalse(boolval);
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
			// sqlrelay doesn't support CONCUR_UPDATEABLE
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
			// sqlrelay doesn't support CONCUR_UPDATEABLE
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
		assertTrue(boolval);
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

		// createStatement
		System.out.println("  create statement");
		stmt=con.createStatement();
		assertTrue((stmt!=null));
		assertEquals(stmt.getConnection(),con);
                System.out.println();

		// unwrap
		if (issqlrelay) {
			System.out.println("  unwrap");
			assertEquals(stmt.isWrapperFor(SQLRCursor.class),1);
			System.out.println();
			assertEquals((stmt.unwrap(SQLRCursor.class)!=null),1);
			System.out.println();
		}

		// query timeouts
		System.out.println("  query timeouts");
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
		rstypesupported[2]=false;
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
		// mysql jdbc supports updatable cursors but
		// sqlrelay doesn't
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
					"  create statement - "+
					rstypename[r]+", "+
					concurrencyname[c]);
				System.out.println(
					"  create statement - "+
					rstypesupported[r]+", "+
					concurrencysupported[c]);
				boolean	supported=
					(rstypesupported[r] &&
					concurrencysupported[c]);
				if (supported) {
					stmt=con.createStatement(
							rstype[r],
							concurrency[c]);
					assertTrue((stmt!=null));
					stmt.close();
				} else {
					try {
						stmt=con.
						createStatement(
							rstype[r],
							concurrency[c]);
						if (issqlrelay) {
							assertTrue(supported);
						} else {
							// mysql jdbc will
							// happily create
							// statement with
							// unsupported
							// parameters
						}
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
						"  create statement - "+
						rstypename[r]+", "+
						concurrencyname[c]+", "+
						holdabilityname[h]);
					System.out.println(
						"  create statement - "+
						rstypesupported[r]+", "+
						concurrencysupported[c]+", "+
						holdabilitysupported[h]);
					boolean	supported=
						(rstypesupported[r] &&
						concurrencysupported[c] &&
						holdabilitysupported[h]);
					if (supported) {
						stmt=con.createStatement(
							rstype[r],
							concurrency[c],
							holdability[h]);
						assertTrue((stmt!=null));
						stmt.close();
					} else {
						try {
							stmt=con.
							createStatement(
								rstype[r],
								concurrency[c],
								holdability[h]);
							if (issqlrelay) {
								assertTrue(
								supported);
							} else {
								// mysql jdbc
								// will happily
								// create a
								// statement
								// with
								// unsupported
								// parameters
							}
						} catch (Exception ex) {
							assertFalse(supported);
						}
					}
					System.out.println();
				}
			}
		}


		// create table
		System.out.println("CREATE TABLE:");
		stmt=con.createStatement();
		assertFalse(stmt.isClosed());
		stmt.executeUpdate("drop table if exists testtable");
		assertEquals(stmt.executeUpdate(
			"create table testtable ("+
			"	testtinyint tinyint, "+
			"	testsmallint smallint, "+
			"	testmediumint mediumint, "+
			"	testint int, "+
			"	testbigint bigint, "+
			"	testfloat float, "+
			"	testreal real, "+
			"	testdecimal decimal(2,1), "+
			"	testdate date, "+
			"	testtime time, "+
			"	testdatetime datetime, "+
			"	testyear year, "+
			"	testchar char(40), "+
			"	testvarchar varchar(40), "+
			"	testtext text, "+
			"	testtinytext tinytext, "+
			"	testmediumtext mediumtext, "+
			"	testlongtext longtext, "+
			"	testblob blob, "+
			"	testtinyblob tinyblob, "+
			"	testmediumblob mediumblob, "+
			"	testlongblob longblob, "+
			"	testtimestamp timestamp, "+
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
			"	1, "+
			"	1, "+
			"	1.5, "+
			"	1.5, "+
			"	1.5, "+
			"	'2001-01-01', "+
			"	'01:00:00', "+
			"	'2001-01-01 01:00:00', "+
			"	'2001', "+
			"	'char1', "+
			"	'varchar1', "+
			"	'text1', "+
			"	'tinytext1', "+
			"	'mediumtext1', "+
			"	'longtext1', "+
			"	'blob1', "+
			"	'tinyblob1', "+
			"	'mediumblob1', "+
			"	'longblob1', "+
			"	NULL, "+
			"	'http://www.firstworks.com:8080/testurl1')"));
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
			"	?, "+
			"	?, "+
			"	?, "+
			"	?, "+
			"	NULL, "+
			"	?)");
		assertFalse(pstmt.isClosed());
		for (int i=2; i<=8; i++) {
			pstmt.clearParameters();
			pstmt.setInt(1,i);
			pstmt.setInt(2,i);
			pstmt.setInt(3,i);
			pstmt.setInt(4,i);
			pstmt.setLong(5,i);
			pstmt.setDouble(6,i+0.5);
			pstmt.setDouble(7,i+0.5);
			pstmt.setDouble(8,i+0.5);

			cal.set(Calendar.YEAR,2000+i);
			cal.set(Calendar.MONTH,Calendar.JANUARY);
			cal.set(Calendar.DAY_OF_MONTH,1);
			cal.set(Calendar.HOUR_OF_DAY,0);
			cal.set(Calendar.MINUTE,0);
			cal.set(Calendar.SECOND,0);
			cal.set(Calendar.MILLISECOND,0);
			pstmt.setDate(9,new java.sql.Date(
						cal.getTimeInMillis()));

			pstmt.setString(10,"0"+i+":00:00");

			cal.set(Calendar.YEAR,2000+i);
			cal.set(Calendar.MONTH,Calendar.JANUARY);
			cal.set(Calendar.DAY_OF_MONTH,1);
			cal.set(Calendar.HOUR_OF_DAY,i);
			cal.set(Calendar.MINUTE,0);
			cal.set(Calendar.SECOND,0);
			cal.set(Calendar.MILLISECOND,0);
			pstmt.setTimestamp(11,new Timestamp(
						cal.getTimeInMillis()));

			pstmt.setString(12,""+(2000+i));
			pstmt.setString(13,"char"+i);
			pstmt.setString(14,"varchar"+i);
			clob=con.createClob();
			clob.setString(1,"text"+i);
			pstmt.setClob(15,clob);
			clob=con.createClob();
			clob.setString(1,"tinytext"+i);
			pstmt.setClob(16,clob);
			clob=con.createClob();
			clob.setString(1,"mediumtext"+i);
			pstmt.setClob(17,clob);
			clob=con.createClob();
			clob.setString(1,"longtext"+i);
			pstmt.setClob(18,clob);
			blob=con.createBlob();
			blob.setBytes(1,(new String("blob"+i)).
					getBytes(StandardCharsets.UTF_8));
			pstmt.setBlob(19,blob);
			blob=con.createBlob();
			blob.setBytes(1,(new String("tinyblob"+i)).
					getBytes(StandardCharsets.UTF_8));
			pstmt.setBlob(20,blob);
			blob=con.createBlob();
			blob.setBytes(1,(new String("mediumblob"+i)).
					getBytes(StandardCharsets.UTF_8));
			pstmt.setBlob(21,blob);
			blob=con.createBlob();
			blob.setBytes(1,(new String("longblob"+i)).
					getBytes(StandardCharsets.UTF_8));
			pstmt.setBlob(22,blob);
			pstmt.setString(23,
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
			"	testtinyint"));
		assertFalse(stmt.isClosed());
		rs=stmt.getResultSet();
		assertTrue((rs!=null));
		System.out.println();

		// unwrap
		if (issqlrelay) {
			System.out.println("  unwrap");
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
			System.out.println("  unwrap");
			assertEquals(rsmd.isWrapperFor(SQLRCursor.class),1);
			System.out.println();
			assertEquals((rsmd.unwrap(SQLRCursor.class)!=null),1);
			System.out.println();
		}


		// column count
		System.out.println("COLUMN COUNT:");
		assertEquals(rsmd.getColumnCount(),24);
		System.out.println();


		// column names
		System.out.println("COLUMN NAMES:");
		assertEquals(rsmd.getColumnName(1),"testtinyint");
		assertEquals(rsmd.getColumnName(2),"testsmallint");
		assertEquals(rsmd.getColumnName(3),"testmediumint");
		assertEquals(rsmd.getColumnName(4),"testint");
		assertEquals(rsmd.getColumnName(5),"testbigint");
		assertEquals(rsmd.getColumnName(6),"testfloat");
		assertEquals(rsmd.getColumnName(7),"testreal");
		assertEquals(rsmd.getColumnName(8),"testdecimal");
		assertEquals(rsmd.getColumnName(9),"testdate");
		assertEquals(rsmd.getColumnName(10),"testtime");
		assertEquals(rsmd.getColumnName(11),"testdatetime");
		assertEquals(rsmd.getColumnName(12),"testyear");
		assertEquals(rsmd.getColumnName(13),"testchar");
		assertEquals(rsmd.getColumnName(14),"testvarchar");
		assertEquals(rsmd.getColumnName(15),"testtext");
		assertEquals(rsmd.getColumnName(16),"testtinytext");
		assertEquals(rsmd.getColumnName(17),"testmediumtext");
		assertEquals(rsmd.getColumnName(18),"testlongtext");
		assertEquals(rsmd.getColumnName(19),"testblob");
		assertEquals(rsmd.getColumnName(20),"testtinyblob");
		assertEquals(rsmd.getColumnName(21),"testmediumblob");
		assertEquals(rsmd.getColumnName(22),"testlongblob");
		assertEquals(rsmd.getColumnName(23),"testtimestamp");
		assertEquals(rsmd.getColumnName(24),"testurl");
		System.out.println();


		// column labels
		System.out.println("COLUMN LABELS:");
		assertEquals(rsmd.getColumnLabel(1),"testtinyint");
		assertEquals(rsmd.getColumnLabel(2),"testsmallint");
		assertEquals(rsmd.getColumnLabel(3),"testmediumint");
		assertEquals(rsmd.getColumnLabel(4),"testint");
		assertEquals(rsmd.getColumnLabel(5),"testbigint");
		assertEquals(rsmd.getColumnLabel(6),"testfloat");
		assertEquals(rsmd.getColumnLabel(7),"testreal");
		assertEquals(rsmd.getColumnLabel(8),"testdecimal");
		assertEquals(rsmd.getColumnLabel(9),"testdate");
		assertEquals(rsmd.getColumnLabel(10),"testtime");
		assertEquals(rsmd.getColumnLabel(11),"testdatetime");
		assertEquals(rsmd.getColumnLabel(12),"testyear");
		assertEquals(rsmd.getColumnLabel(13),"testchar");
		assertEquals(rsmd.getColumnLabel(14),"testvarchar");
		assertEquals(rsmd.getColumnLabel(15),"testtext");
		assertEquals(rsmd.getColumnLabel(16),"testtinytext");
		assertEquals(rsmd.getColumnLabel(17),"testmediumtext");
		assertEquals(rsmd.getColumnLabel(18),"testlongtext");
		assertEquals(rsmd.getColumnLabel(19),"testblob");
		assertEquals(rsmd.getColumnLabel(20),"testtinyblob");
		assertEquals(rsmd.getColumnLabel(21),"testmediumblob");
		assertEquals(rsmd.getColumnLabel(22),"testlongblob");
		assertEquals(rsmd.getColumnLabel(23),"testtimestamp");
		assertEquals(rsmd.getColumnLabel(24),"testurl");
		System.out.println();


		// column types
		System.out.println("COLUMN TYPES:");
		assertEquals(rsmd.getColumnTypeName(1),"TINYINT");
		assertEquals(rsmd.getColumnTypeName(2),"SMALLINT");
		assertEquals(rsmd.getColumnTypeName(3),"MEDIUMINT");
		assertEquals(rsmd.getColumnTypeName(4),"INT");
		assertEquals(rsmd.getColumnTypeName(5),"BIGINT");
		assertEquals(rsmd.getColumnTypeName(6),"FLOAT");
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(7),"REAL");
		} else {
			assertEquals(rsmd.getColumnTypeName(7),"DOUBLE");
		}
		assertEquals(rsmd.getColumnTypeName(8),"DECIMAL");
		assertEquals(rsmd.getColumnTypeName(9),"DATE");
		assertEquals(rsmd.getColumnTypeName(10),"TIME");
		assertEquals(rsmd.getColumnTypeName(11),"DATETIME");
		assertEquals(rsmd.getColumnTypeName(12),"YEAR");
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(13),"STRING");
			assertEquals(rsmd.getColumnTypeName(14),"VARSTRING");
			assertEquals(rsmd.getColumnTypeName(15),"TEXT");
			assertEquals(rsmd.getColumnTypeName(16),"TINYTEXT");
			assertEquals(rsmd.getColumnTypeName(17),"MEDIUMTEXT");
			assertEquals(rsmd.getColumnTypeName(18),"LONGTEXT");
		} else {
			assertEquals(rsmd.getColumnTypeName(13),"CHAR");
			assertEquals(rsmd.getColumnTypeName(14),"VARCHAR");
			assertEquals(rsmd.getColumnTypeName(15),"TEXT");
			assertEquals(rsmd.getColumnTypeName(16),"TINYTEXT");
			assertEquals(rsmd.getColumnTypeName(17),"MEDIUMTEXT");
			assertEquals(rsmd.getColumnTypeName(18),"LONGTEXT");
		}
		assertEquals(rsmd.getColumnTypeName(19),"BLOB");
		assertEquals(rsmd.getColumnTypeName(20),"TINYBLOB");
		assertEquals(rsmd.getColumnTypeName(21),"MEDIUMBLOB");
		assertEquals(rsmd.getColumnTypeName(22),"LONGBLOB");
		assertEquals(rsmd.getColumnTypeName(23),"TIMESTAMP");
		if (issqlrelay) {
			assertEquals(rsmd.getColumnTypeName(24),"VARSTRING");
		} else {
			assertEquals(rsmd.getColumnTypeName(24),"VARCHAR");
		}
		System.out.println();


		// column length
		System.out.println("COLUMN LENGTH:");
		// these vary so much that it's not worth sorting out
		System.out.println();


		// longest column
		System.out.println("LONGEST COLUMN:");
		// these vary so much that it's not worth sorting out
		System.out.println();


		// fields by index
		System.out.println("FIELDS BY INDEX:");
		for (int i=1; i<=8; i++) {

			rs.next();

			// tinyint as short
			System.out.println("  row "+i+" - tinyint as short");
			assertEquals(rs.getShort(1),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// tinyint as int
			System.out.println("  row "+i+" - tinyint as int");
			assertEquals(rs.getInt(1),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// tinyint as long
			System.out.println("  row "+i+" - tinyint as long");
			assertEquals(rs.getLong(1),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// smallint
			System.out.println("  row "+i+" - smallint");
			assertEquals(rs.getInt(2),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// mediumint
			System.out.println("  row "+i+" - mediumint");
			assertEquals(rs.getInt(3),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// int
			System.out.println("  row "+i+" - int");
			assertEquals(rs.getInt(4),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// bigint
			System.out.println("  row "+i+" - bigint");
			assertEquals(rs.getLong(5),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// float
			System.out.println("  row "+i+" - float");
			assertTrue(rs.getString(6)!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// real
			System.out.println("  row "+i+" - real");
			assertEquals(rs.getString(7),i+".5");
			assertFalse(rs.wasNull());
			System.out.println();

			// decimal
			System.out.println("  row "+i+" - decimal");
			assertEquals(rs.getString(8),i+".5");
			assertFalse(rs.wasNull());
			System.out.println();

			// date
			System.out.println("  row "+i+" - date");
			datevar=rs.getDate(9);
			cal.setTime(datevar);
			assertEquals(cal.get(Calendar.YEAR),2000+i);
			assertEquals(cal.get(Calendar.MONTH),Calendar.JANUARY);
			assertEquals(cal.get(Calendar.DAY_OF_MONTH),1);
			assertFalse(rs.wasNull());
			System.out.println();

			// time
			System.out.println("  row "+i+" - time");
			assertEquals(rs.getString(10),"0"+i+":00:00");
			assertFalse(rs.wasNull());
			System.out.println();

			// datetime
			System.out.println("  row "+i+" - datetime");
			tsvar=rs.getTimestamp(11);
			cal.setTime(tsvar);
			assertEquals(cal.get(Calendar.YEAR),2000+i);
			assertEquals(cal.get(Calendar.MONTH),Calendar.JANUARY);
			assertEquals(cal.get(Calendar.DAY_OF_MONTH),1);
                        assertEquals(cal.get(Calendar.HOUR_OF_DAY),i);
                        assertEquals(cal.get(Calendar.MINUTE),0);
                        assertEquals(cal.get(Calendar.SECOND),0);
			assertFalse(rs.wasNull());
			System.out.println();

			// year
			System.out.println("  row "+i+" - year");
			// mysql jdbc returns this as a
			// date instead of just the year
			if (issqlrelay) {
				assertEquals(rs.getString(12),""+(2000+i));
			}
			assertFalse(rs.wasNull());
			System.out.println();

			// char as string
			System.out.println("  row "+i+" - char as string");
			assertEquals(rs.getString(13),"char"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// varchar as string
			System.out.println("  row "+i+" - varchar as string");
			assertEquals(rs.getString(14),"varchar"+i);
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
			System.out.println("  row "+i+" - text as ascii stream");
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

// FIXME: mysql jdbc can't do these unless the field's charset is utf-8
if (issqlrelay) {
			// text as ncharacter stream
			System.out.println("  row "+i+
					" - text as ncharacter stream");
			sw=new StringWriter();
			readerToWriter(rs.getNCharacterStream(15),sw);
			assertEquals(sw.toString(),"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// text as nstring
			System.out.println("  row "+i+" - text as nstring");
			assertEquals(rs.getNString(15),"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// text as unicode stream
			System.out.println("  row "+i+
					" - text as unicode stream");
			assertEquals(new String(
					streamToBytes(rs.getUnicodeStream(15)),"UTF-8"),
						"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();
}

			// tinytext
			System.out.println("  row "+i+" - tinytext");
			assertEquals(rs.getString(16),"tinytext"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// mediumtext
			System.out.println("  row "+i+" - mediumtext");
			assertEquals(rs.getString(17),"mediumtext"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// longtext
			System.out.println("  row "+i+" - longtext");
			assertEquals(rs.getString(18),"longtext"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob
			System.out.println("  row "+i+" - blob");
			blob=rs.getBlob(19);
			assertEquals(new String(
					blob.getBytes(1,(int)blob.length()),
					"UTF-8"),
					"blob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob as bytes
			System.out.println("  row "+i+" - blob as bytes");
			assertEquals(new String(
					rs.getBytes(19),"UTF-8"),
					"blob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob as binary stream
			System.out.println("  row "+i+
					" - blob as binary stream");
			assertEquals(new String(streamToBytes(rs.getBinaryStream(19)),"UTF-8"),
						"blob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// tinyblob
			System.out.println("  row "+i+" - tinyblob");
			assertEquals(new String(
					rs.getBytes(20),"UTF-8"),
					"tinyblob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// mediumblob
			System.out.println("  row "+i+" - mediumblob");
			assertEquals(new String(
					rs.getBytes(21),"UTF-8"),
					"mediumblob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// longblob
			System.out.println("  row "+i+" - longblob");
			assertEquals(new String(
					rs.getBytes(22),"UTF-8"),
					"longblob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// url
			System.out.println("  row "+i+" - url");
			URL	urlvar=rs.getURL(24);
			assertEquals(urlvar.getProtocol(),"http");
			assertEquals(urlvar.getHost(),"www.firstworks.com");
			assertEquals(urlvar.getPort(),8080);
			assertEquals(urlvar.getPath(),"/testurl"+i);
			assertFalse(rs.wasNull());
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
			"	testtinyint");
		assertTrue((rs!=null));
		System.out.println();

		for (int i=1; i<=8; i++) {

			rs.next();

			// tinyint
			System.out.println("  row "+i+" - tinyint");
			assertEquals(rs.getShort("testtinyint"),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// smallint
			System.out.println("  row "+i+" - smallint");
			assertEquals(rs.getInt("testsmallint"),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// mediumint
			System.out.println("  row "+i+" - mediumint");
			assertEquals(rs.getInt("testmediumint"),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// int
			System.out.println("  row "+i+" - int");
			assertEquals(rs.getInt("testint"),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// bigint
			System.out.println("  row "+i+" - bigint");
			assertEquals(rs.getLong("testbigint"),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// float
			System.out.println("  row "+i+" - float");
			assertEquals(rs.getString("testfloat"),i+".5");
			assertFalse(rs.wasNull());
			System.out.println();

			// real
			System.out.println("  row "+i+" - real");
			assertEquals(rs.getString("testreal"),i+".5");
			assertFalse(rs.wasNull());
			System.out.println();

			// decimal
			System.out.println("  row "+i+" - decimal");
			assertEquals(rs.getString("testdecimal"),i+".5");
			assertFalse(rs.wasNull());
			System.out.println();

			// date
			System.out.println("  row "+i+" - date");
			datevar=rs.getDate("testdate");
			cal.setTime(datevar);
			assertEquals(cal.get(Calendar.YEAR),2000+i);
			assertEquals(cal.get(Calendar.MONTH),Calendar.JANUARY);
			assertEquals(cal.get(Calendar.DAY_OF_MONTH),1);
			assertFalse(rs.wasNull());
			System.out.println();

			// time
			System.out.println("  row "+i+" - time");
			assertEquals(rs.getString("testtime"),"0"+i+":00:00");
			assertFalse(rs.wasNull());
			System.out.println();

			// datetime
			System.out.println("  row "+i+" - datetime");
			tsvar=rs.getTimestamp("testdatetime");
			cal.setTime(tsvar);
			assertEquals(cal.get(Calendar.YEAR),2000+i);
			assertEquals(cal.get(Calendar.MONTH),Calendar.JANUARY);
			assertEquals(cal.get(Calendar.DAY_OF_MONTH),1);
                        assertEquals(cal.get(Calendar.HOUR_OF_DAY),i);
                        assertEquals(cal.get(Calendar.MINUTE),0);
                        assertEquals(cal.get(Calendar.SECOND),0);
			assertFalse(rs.wasNull());
			System.out.println();

			// year
			System.out.println("  row "+i+" - year");
			// mysql jdbc returns this as a
			// date instead of just the year
			if (issqlrelay) {
				assertEquals(rs.getString("testyear"),
								""+(2000+i));
			}
			assertFalse(rs.wasNull());
			System.out.println();

			// char as string
			System.out.println("  row "+i+" - char as string");
			assertEquals(rs.getString("testchar"),"char"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// varchar as string
			System.out.println("  row "+i+" - varchar as string");
			assertEquals(rs.getString("testvarchar"),
							"varchar"+i);
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


// FIXME: mysql jdbc can't do these unless the field's charset is utf-8
if (issqlrelay) {
			// text as ncharacter stream
			System.out.println("  row "+i+
					" - text as ncharacter stream");
			sw=new StringWriter();
			readerToWriter(rs.getNCharacterStream("testtext"),sw);
			assertEquals(sw.toString(),"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// text as nstring
			System.out.println("  row "+i+" - text as nstring");
			assertEquals(rs.getNString("testtext"),"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// text as unicode stream
			if (issqlrelay) {
				System.out.println("  row "+i+
					" - text as unicode stream");
				assertEquals(new String(
						streamToBytes(rs.getUnicodeStream("testtext")),"UTF-8"),
							"text"+i);
				assertFalse(rs.wasNull());
				System.out.println();
			}
}

			// tinytext
			System.out.println("  row "+i+" - tinytext");
			assertEquals(rs.getString("testtinytext"),"tinytext"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// mediumtext
			System.out.println("  row "+i+" - mediumtext");
			assertEquals(rs.getString("testmediumtext"),"mediumtext"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// longtext
			System.out.println("  row "+i+" - longtext");
			assertEquals(rs.getString("testlongtext"),"longtext"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob
			System.out.println("  row "+i+" - blob");
			blob=rs.getBlob("testblob");
			assertEquals(new String(
					blob.getBytes(1,(int)blob.length()),
					"UTF-8"),
					"blob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob as bytes
			System.out.println("  row "+i+" - blob as bytes");
			assertEquals(new String(
					rs.getBytes("testblob"),"UTF-8"),
					"blob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob as binary stream
			System.out.println("  row "+i+
					" - blob as binary stream");
			assertEquals(new String(streamToBytes(rs.getBinaryStream("testblob")),"UTF-8"),
						"blob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// tinyblob
			System.out.println("  row "+i+" - tinyblob");
			assertEquals(new String(
					rs.getBytes("testtinyblob"),"UTF-8"),
					"tinyblob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// mediumblob
			System.out.println("  row "+i+" - mediumblob");
			assertEquals(new String(
					rs.getBytes("testmediumblob"),"UTF-8"),
					"mediumblob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// longblob
			System.out.println("  row "+i+" - longblob");
			assertEquals(new String(
					rs.getBytes("testlongblob"),"UTF-8"),
					"longblob"+i);
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
		}


		// row count
		System.out.println("ROW COUNT:");
		assertEquals(rs.getRow(),8);
		rs.close();
		stmt.close();
		assertTrue(stmt.isClosed());
		System.out.println();


		// fetch size 0
		stmt=con.createStatement(ResultSet.TYPE_SCROLL_INSENSITIVE,
						ResultSet.CONCUR_READ_ONLY);
		System.out.println("FETCH SIZE 0:");
		assertEquals(stmt.getFetchSize(),0);
		rs=stmt.executeQuery(
			"select "+
			"	* "+
			"from "+
			"	testtable "+
			"order by "+
			"	testtinyint");
		assertEquals(stmt.getFetchSize(),0);
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
			"	testtinyint");
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
				// oracle can move the window
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

		} while (row<8);

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
			// oracle jdbc does support isLast()
			// when fetch size is non-zero
			assertTrue(rs.isLast());
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
			"	testtinyint");
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
		// relies on transactional isolation (repeatable-read snapshots,
		// rollback discarding an insert); mysql before 4.0 has no
		// transactional storage engine
		if (majorversion>3) {
		Connection	secondcon=DriverManager.getConnection(
							url,props);
		assertTrue((secondcon!=null));
		Statement	secondstmt=secondcon.createStatement();
		assertTrue((secondstmt!=null));
		// mysql uses repeatable read by default,
		// so the second connection needs to commit
		// to start a new transaction to see changes
		secondcon.setAutoCommit(false);
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
		// under repeatable read, second con must commit
		// to start a new snapshot
		secondcon.commit();
		secondrs=secondstmt.executeQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	testtable ");
		assertTrue((secondrs!=null));
		secondrs.next();
		assertEquals(secondrs.getString(1),"8");
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
			"	10, "+
			"	10, "+
			"	10.5, "+
			"	10.5, "+
			"	1.0, "+
			"	'2010-01-01', "+
			"	'10:00:00', "+
			"	'2010-01-01 10:00:00', "+
			"	'2010', "+
			"	'char10', "+
			"	'varchar10', "+
			"	'text10', "+
			"	'tinytext10', "+
			"	'mediumtext10', "+
			"	'longtext10', "+
			"	'blob10', "+
			"	'tinyblob10', "+
			"	'mediumblob10', "+
			"	'longblob10', "+
			"	NULL, "+
			"	'http://www.firstworks.com:8080/testurl10' "+
			"	)"),1);

		// rollback on con
		con.rollback();

		// from secondcon: row count should still be 8
		// (release snapshot first under repeatable read)
		secondcon.commit();
		secondrs=secondstmt.executeQuery(
			"select "+
			"	count(*) "+
			"from "+
			"	testtable ");
		assertTrue((secondrs!=null));
		secondrs.next();
		assertEquals(secondrs.getString(1),"8");
		secondrs.close();

		// switch con to autocommit on; the next insert is
		// auto-committed
		con.setAutoCommit(true);
		assertEquals(stmt.executeUpdate(
			"insert into "+
			"	testtable "+
			"values ("+
			"	10, "+
			"	10, "+
			"	10, "+
			"	10, "+
			"	10, "+
			"	10.5, "+
			"	10.5, "+
			"	1.0, "+
			"	'2010-01-01', "+
			"	'10:00:00', "+
			"	'2010-01-01 10:00:00', "+
			"	'2010', "+
			"	'char10', "+
			"	'varchar10', "+
			"	'text10', "+
			"	'tinytext10', "+
			"	'mediumtext10', "+
			"	'longtext10', "+
			"	'blob10', "+
			"	'tinyblob10', "+
			"	'mediumblob10', "+
			"	'longblob10', "+
			"	NULL, "+
			"	'http://www.firstworks.com:8080/testurl10' "+
			"	)"),1);

		// from secondcon: row count should be 9
		secondcon.commit();
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
		stmt.executeUpdate("drop table testtable");
		}
		// re-open a statement for the following sections (the gated
		// block above creates and closes its own) and make sure
		// testtable is gone (the gated block drops it otherwise)
		stmt=con.createStatement();
		stmt.executeUpdate("drop table if exists testtable");
		System.out.println();


		// null and empty clobs and blobs
		System.out.println("NULL AND EMPTY CLOBS AND BLOBS:");
		stmt.executeUpdate("drop table if exists testtable1");
		assertEquals(stmt.executeUpdate(
			"create table testtable1 ("+
			"	testtext1 text, "+
			"	testtext2 text, "+
			"	testblob1 blob, "+
			"	testblob2 blob)"),0);
		pstmt=con.prepareStatement(
			"insert into "+
			"	testtable1 "+
			"values ("+
			"	?, "+
			"	?, "+
			"	?, "+
			"	?)");
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
		assertEquals(rs.getString(1),"");
		assertEquals(rs.getString(2),null);
		assertEquals(rs.getBlob(3).length(),0);
		assertEquals(rs.getBlob(4),null);
		rs.close();
		stmt.executeUpdate("drop table if exists testtable1");
		System.out.println();


		// long varchar
		System.out.println("LONG VARCHAR:");
		stmt.executeUpdate("drop table if exists testtable2");
		assertEquals(stmt.executeUpdate(
			"create table "+
			"	testtable2 (testvarchar varchar(1024))"),0);
		pstmt=con.prepareStatement(
			"insert into "+
			"	testtable2 values (?)");
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
		// mysql jdbc doesn't support max field size
		if (issqlrelay) {
			stmt.setMaxFieldSize(512);
			assertEquals(stmt.getMaxFieldSize(),512);
			rs=stmt.executeQuery(
				"select testvarchar from testtable2");
			assertTrue((rs!=null));
			rs.next();
			stringval=rs.getString(1);
			assertEquals(stringval.length(),512);
			assertEquals(stringval,str.substring(0,512));
			rs.close();
			stmt.setMaxFieldSize(0);
			assertEquals(stmt.getMaxFieldSize(),0);
		}
		stmt.executeUpdate("drop table if exists testtable2");
		System.out.println();


		// long text
		System.out.println("LONG TEXT:");
		stmt.executeUpdate("drop table if exists testtable2");
		assertEquals(stmt.executeUpdate(
			"create table "+
			"	testtable2 (testtext text)"),0);
		pstmt=con.prepareStatement(
			"insert into "+
			"	testtable2 values (?)");
		assertTrue((pstmt!=null));
		StringBuilder	textval=new StringBuilder();
		for (int i=0; i<1024; i++) {
			textval.append('C');
		}
		String	textstr=textval.toString();
		pstmt.setString(1,textstr);
		assertEquals(pstmt.executeUpdate(),1);
		pstmt.close();
		rs=stmt.executeQuery("select testtext from testtable2");
		assertTrue((rs!=null));
		rs.next();
		clob=rs.getClob(1);
		assertEquals(clob.length(),1024);
		assertEquals(clob.getSubString(1,(int)clob.length()),textstr);
		rs.close();
		stmt.executeUpdate("drop table if exists testtable2");
		System.out.println();


		// negative input bind
		System.out.println("NEGATIVE INPUT BIND:");
		stmt.executeUpdate("drop table if exists testtable2");
		assertEquals(stmt.executeUpdate(
			"create table "+
			"	testtable2 (testval int)"),0);
		pstmt=con.prepareStatement(
			"insert into "+
			"	testtable2 values (?)");
		assertTrue((pstmt!=null));
		pstmt.setInt(1,-1);
		assertEquals(pstmt.executeUpdate(),1);
		pstmt.close();
		rs=stmt.executeQuery("select testval from testtable2");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"-1");
		rs.close();
		stmt.executeUpdate("drop table if exists testtable2");
		System.out.println();


		// temporary tables
		System.out.println("TEMPORARY TABLES:");
		stmt.executeUpdate("drop table if exists temptable");
		assertEquals(stmt.executeUpdate(
			"create temporary table "+
			"temptable "+
			"(col1 int)"),0);
		assertEquals(stmt.executeUpdate(
			"insert into "+
			"temptable "+
			"values (1)"),1);
		rs=stmt.executeQuery(
			"select count(*) from "+
			"temptable");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"1");
		rs.close();
		// temp table persists across commits
		con.commit();
		rs=stmt.executeQuery(
			"select count(*) from "+
			"temptable");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"1");
		rs.close();
		// temp table is gone after close/reconnect
		con.close();
		System.out.println();
		con=DriverManager.getConnection(url,props);
		assertTrue((con!=null));
		con.setAutoCommit(false);
		stmt=con.createStatement();
		assertTrue((stmt!=null));
		try {
			stmt.executeQuery(
				"select count(*) from "+
				"temptable");
			assertTrue(false);
		} catch (Exception ex) {
			assertTrue(true);
		}
		System.out.println();


		// stored procedures
		System.out.println("STORED PROCEDURES:");
		// mysql before 5.0 has no stored procedures or functions
		if (majorversion>3) {
			// return no value
			stmt.executeUpdate("drop procedure if exists testproc");
			assertEquals(stmt.executeUpdate(
				"create procedure testproc("+
				"	in in1 int, "+
				"	in in2 float, "+
				"	in in3 char(20)) "+
				"begin "+
				"	select in1, in2, in3; "+
				"end"),0);
			cstmt=con.prepareCall(
				"call testproc(?,?,?)");
			cstmt.setInt(1,1);
			cstmt.setDouble(2,2.5);
			cstmt.setString(3,"hello");
			assertTrue(cstmt.execute());
			rs=cstmt.getResultSet();
			assertTrue((rs!=null));
			assertTrue(rs.next());
			assertEquals(rs.getString(1),"1");
			assertEquals(rs.getString(2),"2.5");
			assertEquals(rs.getString(3),"hello");
			rs.close();
			cstmt.close();
			stmt.executeUpdate("drop procedure if exists testproc");
			System.out.println();
			// return single value (function)
			stmt.executeUpdate("drop function if exists testfunc");
			assertEquals(stmt.executeUpdate(
				"create function testfunc("+
				"	in1 int, "+
				"	in2 int) "+
				"returns int "+
				"deterministic "+
				"return in1+in2"),0);
			pstmt=con.prepareStatement(
				"select testfunc(?,?)");
			assertTrue((pstmt!=null));
			pstmt.setInt(1,1);
			pstmt.setInt(2,2);
			rs=pstmt.executeQuery();
			assertTrue((rs!=null));
			assertTrue(rs.getStatement()==pstmt);
			rs.next();
			assertEquals(rs.getString(1),"3");
			rs.close();
			pstmt.close();
			stmt.executeUpdate("drop function if exists testfunc");
			System.out.println();
			// return values
			assertEquals(stmt.executeUpdate(
				"create procedure testproc("+
				"	out out1 int, "+
				"	out out2 float, "+
				"	out out3 char(20)) "+
				"begin "+
				"	select 1, 2.5, 'hello' "+
				"		into out1, out2, out3; "+
				"end"),0);
			stmt.executeUpdate("set @out1=0, @out2=0.0, @out3=''");
			stmt.executeUpdate("call testproc(@out1,@out2,@out3)");
			rs=stmt.executeQuery("select @out1, @out2, @out3");
			assertTrue((rs!=null));
			assertTrue(rs.next());
			assertEquals(rs.getString(1),"1");
			assertEquals(rs.getString(2),"2.5");
			assertEquals(rs.getString(3),"hello");
			rs.close();
			stmt.executeUpdate("drop procedure if exists testproc");
			System.out.println();
		}


		// client info properties
		System.out.println("CLIENT INFO PROPERTIES:");
		con=DriverManager.getConnection(url,props);
		con.setAutoCommit(false);
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
			assertEquals(rsmd.getColumnName(col++),"DEFAULT_VALUE");
			assertEquals(rsmd.getColumnName(col++),"DESCRIPTION");
			rs.close();
			System.out.println();
		}


		stmt=con.createStatement();

		// these metadata queries hit information_schema, which mysql
		// before 5.0 doesn't have
		if (majorversion>3) {

		// catalog list
		System.out.println("CATALOG LIST:");
		rs=md.getCatalogs();
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),1);
		col=1;
		assertEquals(rsmd.getColumnName(col++),"TABLE_CAT");
		// the catalogs are the databases
		found=false;
		while (rs.next()) {
			String catname=rs.getString("TABLE_CAT");
			if (catname!=null &&
				catname.equals(hostname)) {
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
		// mysql has no schemas
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
		while (rs.next()) {
			String ttname=rs.getString("TABLE_TYPE");
			if (ttname!=null &&
				ttname.equals("TABLE")) {
				found=true;
				break;
			}
		}
		assertTrue(found);
		rs.close();
		System.out.println();


		// table list
		System.out.println("TABLE LIST:");
		stmt.executeUpdate("drop table if exists testtable1");
		stmt.executeUpdate("drop table if exists testtable2");
		stmt.executeUpdate("drop table if exists testtable3");
		stmt.executeUpdate("drop table if exists testtable4");
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
		if (issqlrelay) {
			rs=md.getTables(null,null,"%",
						new String[] {"TABLE"});
		} else {
			// mysql jdbc returns tables for all catalogs if the
			// catalog parameter is null, it also expects you to
			// pass the schema in for the catalog, and returns the
			// schema in TABLE_CAT
			rs=md.getTables(hostname,null,"%",
						new String[] {"TABLE"});
		}
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),10);
		col=1;
		assertEquals(rsmd.getColumnName(col++),
						"TABLE_CAT");
		assertEquals(rsmd.getColumnName(col++),
						"TABLE_SCHEM");
		assertEquals(rsmd.getColumnName(col++),
						"TABLE_NAME");
		assertEquals(rsmd.getColumnName(col++),
						"TABLE_TYPE");
		assertEquals(rsmd.getColumnName(col++),
						"REMARKS");
		assertEquals(rsmd.getColumnName(col++),
						"TYPE_CAT");
		assertEquals(rsmd.getColumnName(col++),
						"TYPE_SCHEM");
		assertEquals(rsmd.getColumnName(col++),
						"TYPE_NAME");
		assertEquals(rsmd.getColumnName(col++),
						"SELF_REFERENCING_COL_NAME");
		assertEquals(rsmd.getColumnName(col++),
						"REF_GENERATION");
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
		stmt.executeUpdate("drop table if exists testtable1");
		stmt.executeUpdate("drop table if exists testtable2");
		stmt.executeUpdate("drop table if exists testtable3");
		stmt.executeUpdate("drop table if exists testtable4");
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
		stmt.executeUpdate("drop table if exists testtable");
		stmt.executeUpdate(
			"create table testtable ("+
			"	testtinyint tinyint, "+
			"	testsmallint smallint, "+
			"	testmediumint mediumint, "+
			"	testint int, "+
			"	testbigint bigint, "+
			"	testfloat float, "+
			"	testreal real, "+
			"	testdecimal decimal(2,1), "+
			"	testdate date, "+
			"	testtime time, "+
			"	testdatetime datetime, "+
			"	testyear year, "+
			"	testchar char(40), "+
			"	testvarchar varchar(40), "+
			"	testtext text, "+
			"	testtinytext tinytext, "+
			"	testmediumtext mediumtext, "+
			"	testlongtext longtext, "+
			"	testblob blob, "+
			"	testtinyblob tinyblob, "+
			"	testmediumblob mediumblob, "+
			"	testlongblob longblob, "+
			"	testtimestamp timestamp, "+
			"	testurl varchar(60))");
		rs=md.getColumns(null,null,"testtable","%");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),24);
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
		assertEquals(rsmd.getColumnName(col++),"SCOPE_CATALOG");
		assertEquals(rsmd.getColumnName(col++),"SCOPE_SCHEMA");
		assertEquals(rsmd.getColumnName(col++),"SCOPE_TABLE");
		assertEquals(rsmd.getColumnName(col++),"SOURCE_DATA_TYPE");
		assertEquals(rsmd.getColumnName(col++),"IS_AUTOINCREMENT");
		assertEquals(rsmd.getColumnName(col++),"IS_GENERATEDCOLUMN");
		assertTrue(rs.next());
		// #7971 - catalog/schema track the connection
		assertEquals(rs.getString("TABLE_CAT"),con.getCatalog());
		assertEquals(rs.getString("TABLE_SCHEM"),con.getSchema());
		assertEquals(rs.getString("COLUMN_NAME"),"testtinyint");
		assertEquals(rs.getString("TYPE_NAME"),"TINYINT");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testsmallint");
		assertEquals(rs.getString("TYPE_NAME"),"SMALLINT");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testmediumint");
		assertEquals(rs.getString("TYPE_NAME"),"MEDIUMINT");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testint");
		assertEquals(rs.getString("TYPE_NAME"),"INT");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testbigint");
		assertEquals(rs.getString("TYPE_NAME"),"BIGINT");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testfloat");
		assertEquals(rs.getString("TYPE_NAME"),"FLOAT");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testreal");
		assertEquals(rs.getString("TYPE_NAME"),"DOUBLE");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testdecimal");
		assertEquals(rs.getString("TYPE_NAME"),"DECIMAL");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testdate");
		assertEquals(rs.getString("TYPE_NAME"),"DATE");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testtime");
		assertEquals(rs.getString("TYPE_NAME"),"TIME");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testdatetime");
		assertEquals(rs.getString("TYPE_NAME"),"DATETIME");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testyear");
		assertEquals(rs.getString("TYPE_NAME"),"YEAR");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testchar");
		assertEquals(rs.getString("TYPE_NAME"),"CHAR");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testvarchar");
		assertEquals(rs.getString("TYPE_NAME"),"VARCHAR");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testtext");
		assertEquals(rs.getString("TYPE_NAME"),"TEXT");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testtinytext");
		assertEquals(rs.getString("TYPE_NAME"),"TINYTEXT");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testmediumtext");
		assertEquals(rs.getString("TYPE_NAME"),"MEDIUMTEXT");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testlongtext");
		assertEquals(rs.getString("TYPE_NAME"),"LONGTEXT");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testblob");
		assertEquals(rs.getString("TYPE_NAME"),"BLOB");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testtinyblob");
		assertEquals(rs.getString("TYPE_NAME"),"TINYBLOB");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testmediumblob");
		assertEquals(rs.getString("TYPE_NAME"),"MEDIUMBLOB");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testlongblob");
		assertEquals(rs.getString("TYPE_NAME"),"LONGBLOB");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testtimestamp");
		assertEquals(rs.getString("TYPE_NAME"),"TIMESTAMP");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testurl");
		assertEquals(rs.getString("TYPE_NAME"),"VARCHAR");
		rs.close();
		stmt.executeUpdate("drop table if exists testtable");
		System.out.println();


		// column list - is_autoincrement
		System.out.println("COLUMN LIST - is_autoincrement:");
		stmt.executeUpdate("drop table if exists testtable");
		stmt.executeUpdate(
			"create table testtable ("+
			"	col1 int auto_increment primary key, "+
			"	col2 int)");
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
		stmt.executeUpdate("drop table if exists testtable");
		System.out.println();


		// primary key list
		System.out.println("PRIMARY KEY LIST:");
		stmt.executeUpdate("drop table if exists testtable");
		stmt.executeUpdate(
			"create table testtable ("+
			"	col1 int primary key, "+
			"	col2 int)");
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
		assertEquals(rs.getString("PK_NAME"),"PRIMARY");
		assertFalse(rs.next());
		rs.close();
		stmt.executeUpdate("drop table if exists testtable");
		System.out.println();


		// key and index list
		System.out.println("KEY AND INDEX LIST:");
		stmt.executeUpdate("drop table if exists testtable");
		stmt.executeUpdate(
			"create table testtable ("+
			"	col1 int primary key, "+
			"	col2 int)");
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
		assertTrue(rs.next());
		assertEquals(rs.getString("TABLE_NAME"),"testtable");
		assertEquals(rs.getString("NON_UNIQUE"),"false");
		assertEquals(rs.getString("ORDINAL_POSITION"),"1");
		assertEquals(rs.getString("COLUMN_NAME"),"col1");
		assertEquals(rs.getString("ASC_OR_DESC"),"A");
		assertEquals(rs.getString("TYPE"),"3");
		assertEquals(rs.getString("INDEX_NAME"),"PRIMARY");
		assertFalse(rs.next());
		rs.close();
		stmt.executeUpdate("drop table if exists testtable");
		System.out.println();


		// procedure list
		System.out.println("PROCEDURE LIST:");
		stmt.executeUpdate("drop procedure if exists testproc1");
		stmt.executeUpdate("drop procedure if exists testproc2");
		stmt.executeUpdate("drop procedure if exists testproc3");
		stmt.executeUpdate("drop procedure if exists testproc4");
		stmt.executeUpdate(
			"create procedure testproc1("+
			"	in in1 int, "+
			"	in in2 char(20), "+
			"	in in3 varchar(20), "+
			"	in in4 date) "+
			"begin end");
		stmt.executeUpdate(
			"create procedure testproc2("+
			"	in in1 int, "+
			"	in in2 char(20), "+
			"	in in3 varchar(20), "+
			"	in in4 date) "+
			"begin end");
		stmt.executeUpdate(
			"create procedure testproc3("+
			"	in in1 int, "+
			"	in in2 char(20), "+
			"	in in3 varchar(20), "+
			"	in in4 date) "+
			"begin end");
		stmt.executeUpdate(
			"create procedure testproc4("+
			"	in in1 int, "+
			"	in in2 char(20), "+
			"	in in3 varchar(20), "+
			"	in in4 date) "+
			"begin end");
		rs=md.getProcedures(null,null,"%");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		if (issqlrelay) {
			assertEquals(rsmd.getColumnCount(),8);
		} else {
			// mysql jdbc returns 9 columns
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
			// mysql jdbc returns reserved columns (in lower case)
			assertEquals(rsmd.getColumnName(col++),"reserved1");
			assertEquals(rsmd.getColumnName(col++),"reserved2");
			assertEquals(rsmd.getColumnName(col++),"reserved3");
		}
		assertEquals(rsmd.getColumnName(col++),"REMARKS");
		assertEquals(rsmd.getColumnName(col++),"PROCEDURE_TYPE");
		if (!issqlrelay) {
			// mysql jdbc returns this column too
			assertEquals(rsmd.getColumnName(col++),
						"SPECIFIC_NAME");
		}
		counter=0;
		while (rs.next()) {
			String name=rs.getString("PROCEDURE_NAME");
			if (name.equals("testproc1") ||
				name.equals("testproc2") ||
				name.equals("testproc3") ||
				name.equals("testproc4")) {
				// created to return no result set
				assertEquals(rs.getShort("PROCEDURE_TYPE"),
					DatabaseMetaData.procedureNoResult);
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
		assertEquals(rs.getString("COLUMN_NAME"),"in1");
		assertEquals(rs.getString("TYPE_NAME"),"INT");
		assertEquals(rs.getString("ORDINAL_POSITION"),"1");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"in2");
		assertEquals(rs.getString("TYPE_NAME"),"CHAR");
		assertEquals(rs.getString("ORDINAL_POSITION"),"2");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"in3");
		assertEquals(rs.getString("TYPE_NAME"),"VARCHAR");
		assertEquals(rs.getString("ORDINAL_POSITION"),"3");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"in4");
		assertEquals(rs.getString("TYPE_NAME"),"DATE");
		assertEquals(rs.getString("ORDINAL_POSITION"),"4");
		rs.close();
		stmt.executeUpdate("drop procedure if exists testproc1");
		stmt.executeUpdate("drop procedure if exists testproc2");
		stmt.executeUpdate("drop procedure if exists testproc3");
		stmt.executeUpdate("drop procedure if exists testproc4");
		System.out.println();

		}


		// invalid queries
		System.out.println("INVALID QUERIES:");
		try {
			stmt.executeQuery(
				"select "+
				"	* "+
				"from "+
				"	testtable "+
				"order by "+
				"	testtinyint");
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
				"	testtinyint");
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
				"	testtinyint");
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
				"	testtinyint");
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
