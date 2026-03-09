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

class db2 extends sqlrtest {
	public static void main(String args[]) throws Exception {

		String classpath=System.getProperty("java.class.path");
		String hostname=InetAddress.getLocalHost().
					getHostName().split("\\.")[0];
		String db2db=hostname.
			replaceAll("^centos","co").
			replaceAll("^debian","db").
			replaceAll("^fedora","fc").
			replaceAll("^freebsd","fb").
			replaceAll("^haiku","hk").
			replaceAll("^openbsd","ob").
			replaceAll("^netbsd","nb").
			replaceAll("^opensuse","os").
			replaceAll("^solaris","sl").
			replaceAll("^syllable","sb").
			replaceAll("^ubuntu","u").
			replaceAll("alpha$","a").
			replaceAll("mipsel$","m32").
			replaceAll("mips64el$","m64").
			replaceAll("hppa$","hp").
			replaceAll("i386$","x86").
			replaceAll("sparc$","s32").
			replaceAll("sparc64$","s64");
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
			user="db2inst1";
			password="testpassword";
			issqlrelay=true;
		} else if (classpath.contains("db2")) {
			driver="com.ibm.db2.jcc.DB2Driver";
			url="jdbc:db2://db2:50000/"+db2db+"";
			user="db2inst1";
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
		System.out.println();

		// warnings
		System.out.println("  warnings");
		con.clearWarnings();
		System.out.println();

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
		// varies by server installation
		assertTrue(intval>=0);
		System.out.println();

		// getDatabaseMinorVersion
		System.out.println("  getDatabaseMinorVersion");
		intval=md.getDatabaseMinorVersion();
		System.out.println("    "+intval);
		// varies by server installation
		assertTrue(intval>=0);
		System.out.println();

		// getDatabaseProductName
		System.out.println("  getDatabaseProductName");
		stringval=md.getDatabaseProductName();
		System.out.println("    "+stringval);
		if (issqlrelay) {
			assertEquals(stringval,"db2");
		} else {
			assertTrue(stringval.startsWith("DB2"));
		}
		System.out.println();

		// getDatabaseProductVersion
		System.out.println("  getDatabaseProductVersion");
		stringval=md.getDatabaseProductVersion();
		System.out.println("    "+stringval);
		// varies by server installation
		assertTrue(stringval!=null);
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
		// varies by driver jar version
		assertTrue(intval>=0);
		System.out.println();

		// getDriverMinorVersion
		System.out.println("  getDriverMinorVersion");
		intval=md.getDriverMinorVersion();
		System.out.println("    "+intval);
		// varies by driver jar version
		assertTrue(intval>=0);
		System.out.println();

		// getDriverName
		System.out.println("  getDriverName");
		stringval=md.getDriverName();
		System.out.println("    "+stringval);
		if (issqlrelay) {
			assertEquals(stringval,"SQL Relay JDBC driver");
		} else {
			// varies by driver jar version
			assertTrue(stringval!=null);
		}
		System.out.println();

		// getDriverVersion
		System.out.println("  getDriverVersion");
		stringval=md.getDriverVersion();
		System.out.println("    "+stringval);
		// not null and only contains numbers and dots
		assertTrue(stringval!=null && stringval.matches("[0-9.]+$"));
		System.out.println();

		// getExtraNameCharacters
		System.out.println("  getExtraNameCharacters");
		stringval=md.getExtraNameCharacters();
		System.out.println("    "+stringval);
		assertEquals(stringval,"@#");
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
		assertEquals(intval,4000);
		System.out.println();

		// getMaxCatalogNameLength
		System.out.println("  getMaxCatalogNameLength");
		intval=md.getMaxCatalogNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,8);
		System.out.println();

		// getMaxCharLiteralLength
		System.out.println("  getMaxCharLiteralLength");
		intval=md.getMaxCharLiteralLength();
		System.out.println("    "+intval);
		assertEquals(intval,32672);
		System.out.println();

		// getMaxColumnNameLength
		System.out.println("  getMaxColumnNameLength");
		intval=md.getMaxColumnNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,128);
		System.out.println();

		// getMaxColumnsInGroupBy
		System.out.println("  getMaxColumnsInGroupBy");
		intval=md.getMaxColumnsInGroupBy();
		System.out.println("    "+intval);
		assertEquals(intval,1012);
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
		assertEquals(intval,1012);
		System.out.println();

		// getMaxColumnsInSelect
		System.out.println("  getMaxColumnsInSelect");
		intval=md.getMaxColumnsInSelect();
		System.out.println("    "+intval);
		assertEquals(intval,1012);
		System.out.println();

		// getMaxColumnsInTable
		System.out.println("  getMaxColumnsInTable");
		intval=md.getMaxColumnsInTable();
		System.out.println("    "+intval);
		assertEquals(intval,1012);
		System.out.println();

		// getMaxConnections
		System.out.println("  getMaxConnections");
		intval=md.getMaxConnections();
		System.out.println("    "+intval);
		if (issqlrelay) {
			// varies by sqlrelay config
			assertTrue(intval>0);
		} else {
			// informix jdbc returns 0 for this
			assertEquals(intval,0);
		}
		System.out.println();

		// getMaxCursorNameLength
		System.out.println("  getMaxCursorNameLength");
		intval=md.getMaxCursorNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,128);
		System.out.println();

		// getMaxIndexLength
		System.out.println("  getMaxIndexLength");
		intval=md.getMaxIndexLength();
		System.out.println("    "+intval);
		assertEquals(intval,1024);
		System.out.println();

		// getMaxProcedureNameLength
		System.out.println("  getMaxProcedureNameLength");
		intval=md.getMaxProcedureNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,128);
		System.out.println();

		// getMaxRowSize
		System.out.println("  getMaxRowSize");
		intval=md.getMaxRowSize();
		System.out.println("    "+intval);
		assertEquals(intval,32677);
		System.out.println();

		// getMaxSchemaNameLength
		System.out.println("  getMaxSchemaNameLength");
		intval=md.getMaxSchemaNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,128);
		System.out.println();

		// getMaxStatementLength
		System.out.println("  getMaxStatementLength");
		intval=md.getMaxStatementLength();
		System.out.println("    "+intval);
		assertEquals(intval,2097152);
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
		assertEquals(intval,128);
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
		assertEquals(intval,30);
		System.out.println();

		// getNumericFunctions
		System.out.println("  getNumericFunctions");
		stringval=md.getNumericFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"ABS,ACOS,ASIN,ATAN,ATAN2,CEILING,COS,COT,DEGREES,EXP,FLOOR,LOG,LOG10,MOD,POWER,RADIANS,RAND,ROUND,SIGN,SIN,SQRT,TAN,TRUNCATE");
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

		// getSchemaTerm
		System.out.println("  getSchemaTerm");
		stringval=md.getSchemaTerm();
		System.out.println("    "+stringval);
		assertEquals(stringval,"schema");
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
		assertEquals(stringval,"AFTER,ALIAS,ALLOW,APPLICATION,ASSOCIATE,ASUTIME,AUDIT,AUX,AUXILIARY,BEFORE,BINARY,BUFFERPOOL,CACHE,CALL,CALLED,CAPTURE,CARDINALITY,CCSID,CLUSTER,COLLECTION,COLLID,COMMENT,CONCAT,CONDITION,CONTAINS,COUNT_BIG,CURRENT_LC_CTYPE,CURRENT_PATH,CURRENT_SERVER,CURRENT_TIMEZONE,CYCLE,DATA,DATABASE,DAYS,DB2GENERAL,DB2GENRL,DB2SQL,DBINFO,DEFAULTS,DEFINITION,DETERMINISTIC,DISALLOW,DO,DSNHATTR,DSSIZE,DYNAMIC,EACH,EDITPROC,ELSEIF,ENCODING,END-EXEC1,ERASE,EXCLUDING,EXIT,FENCED,FIELDPROC,FILE,FINAL,FREE,FUNCTION,GENERAL,GENERATED,GRAPHIC,HANDLER,HOLD,HOURS,IF,INCLUDING,INCREMENT,INDEX,INHERIT,INOUT,INTEGRITY,ISOBID,ITERATE,JAR,JAVA,LABEL,LC_CTYPE,LEAVE,LINKTYPE,LOCALE,LOCATOR,LOCATORS,LOCK,LOCKMAX,LOCKSIZE,LONG,LOOP,MAXVALUE,MICROSECOND,MICROSECONDS,MINUTES,MINVALUE,MODE,MODIFIES,MONTHS,NEW,NEW_TABLE,NOCACHE,NOCYCLE,NODENAME,NODENUMBER,NOMAXVALUE,NOMINVALUE,NOORDER,NULLS,NUMPARTS,OBID,OLD,OLD_TABLE,OPTIMIZATION,OPTIMIZE,OUT,OVERRIDING,PACKAGE,PARAMETER,PART,PARTITION,PATH,PIECESIZE,PLAN,PRIQTY,PROGRAM,PSID,QUERYNO,READS,RECOVERY,REFERENCING,RELEASE,RENAME,REPEAT,RESET,RESIGNAL,RESTART,RESULT,RESULT_SET_LOCATOR,RETURN,RETURNS,ROUTINE,ROW,RRN,RUN,SAVEPOINT,SCRATCHPAD,SECONDS,SECQTY,SECURITY,SENSITIVE,SIGNAL,SIMPLE,SOURCE,SPECIFIC,SQLID,STANDARD,START,STATIC,STAY,STOGROUP,STORES,STYLE,SUBPAGES,SYNONYM,SYSFUN,SYSIBM,SYSPROC,SYSTEM,TABLESPACE,TRIGGER,TYPE,UNDO,UNTIL,VALIDPROC,VARIABLE,VARIANT,VCAT,VOLUMES,WHILE,WLM,YEARS");
		System.out.println();

		// getSQLStateType
		System.out.println("  getSQLStateType");
		intval=md.getSQLStateType();
		System.out.println("    "+intval);
		assertEquals(intval,2);
		System.out.println();

		// getStringFunctions
		System.out.println("  getStringFunctions");
		stringval=md.getStringFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"ASCII,CHAR,CONCAT,DIFFERENCE,INSERT,LCASE,LEFT,LENGTH,LOCATE,LTRIM,REPEAT,REPLACE,RIGHT,RTRIM,SOUNDEX,SPACE,SUBSTRING,UCASE");
		System.out.println();

		// getSystemFunctions
		System.out.println("  getSystemFunctions");
		stringval=md.getSystemFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"");
		System.out.println();

		// getTimeDateFunctions
		System.out.println("  getTimeDateFunctions");
		stringval=md.getTimeDateFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"DAYNAME,DAYOFWEEK,DAYOFYEAR,HOUR,MINUTE,MONTH,MONTHNAME,QUARTER,SECOND,TIMESTAMPDIFF,WEEK,YEAR,CURDATE,CURTIME,NOW");
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
		assertEquals(stringval,user);
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
		assertTrue(boolval);
		System.out.println();

		// nullsAreSortedLow
		System.out.println("  nullsAreSortedLow");
		boolval=md.nullsAreSortedLow();
		System.out.println("    "+boolval);
		assertFalse(boolval);
		System.out.println();

		// statement

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
		assertFalse(boolval);
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
		assertFalse(boolval);
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
		if (issqlrelay) {
			// sqlrelay jdbc doesn't support this
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

		System.out.println("STATEMENT:");
		stmt=con.createStatement();
		assertTrue((stmt!=null));
		System.out.println();

		// drop existing table
		try {
			stmt.executeUpdate("drop table testtable");
		} catch (Exception ex) {
		}

		// create table
		System.out.println("CREATE TABLE:");
		assertEquals(stmt.executeUpdate(
			"create table testtable ("+
			"	testsmallint smallint, "+
			"	testint integer, "+
			"	testbigint bigint, "+
			"	testdecimal decimal(10,2), "+
			"	testreal real, "+
			"	testdouble double, "+
			"	testchar char(40), "+
			"	testvarchar varchar(40), "+
			"	testdate date, "+
			"	testtime time, "+
			"	testtimestamp timestamp, "+
			"	testclob clob, "+
			"	testblob blob, "+
			"	testurl varchar(60))"),0);
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
			"	'char1', "+
			"	'varchar1', "+
			"	'2001-01-01', "+
			"	'01:00:00', "+
			"	NULL, "+
			"	'clob1', "+
			"	blob('blob1'), "+
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
			"	?)");
		assertFalse(pstmt.isClosed());
		for (int i=2; i<=4; i++) {
			pstmt.clearParameters();
			pstmt.setInt(1,i);
			pstmt.setInt(2,i);
			pstmt.setLong(3,(long)i);
			pstmt.setDouble(4,i+0.1);
			pstmt.setDouble(5,i+0.1);
			pstmt.setDouble(6,i+0.1);
			pstmt.setString(7,"char"+i);
			pstmt.setString(8,"varchar"+i);

			cal.set(Calendar.YEAR,2000+i);
			cal.set(Calendar.MONTH,Calendar.JANUARY);
			cal.set(Calendar.DAY_OF_MONTH,1);
			cal.set(Calendar.HOUR_OF_DAY,0);
			cal.set(Calendar.MINUTE,0);
			cal.set(Calendar.SECOND,0);
			pstmt.setDate(9,new java.sql.Date(
						cal.getTimeInMillis()));
			pstmt.setString(10,"0"+i+":00:00");
			pstmt.setNull(11,java.sql.Types.TIMESTAMP);
			pstmt.setString(12,"clob"+i);
			pstmt.setBytes(13,(new String("blob"+i)).
					getBytes(StandardCharsets.UTF_8));
			pstmt.setString(14,
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
			"	testsmallint"));
		rs=stmt.getResultSet();
		assertTrue((rs!=null));
		System.out.println();

		// getMetaData
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		System.out.println();

		// column count
		System.out.println("COLUMN COUNT:");
		assertEquals(rsmd.getColumnCount(),14);
		System.out.println();

		// column names
		System.out.println("COLUMN NAMES:");
		assertEquals(rsmd.getColumnName(1),"TESTSMALLINT");
		assertEquals(rsmd.getColumnName(2),"TESTINT");
		assertEquals(rsmd.getColumnName(3),"TESTBIGINT");
		assertEquals(rsmd.getColumnName(4),"TESTDECIMAL");
		assertEquals(rsmd.getColumnName(5),"TESTREAL");
		assertEquals(rsmd.getColumnName(6),"TESTDOUBLE");
		assertEquals(rsmd.getColumnName(7),"TESTCHAR");
		assertEquals(rsmd.getColumnName(8),"TESTVARCHAR");
		assertEquals(rsmd.getColumnName(9),"TESTDATE");
		assertEquals(rsmd.getColumnName(10),"TESTTIME");
		assertEquals(rsmd.getColumnName(11),"TESTTIMESTAMP");
		assertEquals(rsmd.getColumnName(12),"TESTCLOB");
		assertEquals(rsmd.getColumnName(13),"TESTBLOB");
		assertEquals(rsmd.getColumnName(14),"TESTURL");
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
		System.out.println();

		// fields by index
		System.out.println("FIELDS BY INDEX:");
		for (int i=1; i<=4; i++) {

			assertTrue(rs.next());
			System.out.println();

			// smallint as short
			System.out.println("  row "+i+" - smallint as short");
			assertEquals(rs.getShort(1),(short)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// smallint as int
			System.out.println("  row "+i+" - smallint as int");
			assertEquals(rs.getInt(1),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// int
			System.out.println("  row "+i+" - int");
			assertEquals(rs.getInt(2),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// bigint
			System.out.println("  row "+i+" - bigint");
			assertEquals(rs.getLong(3),(long)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// decimal
			System.out.println("  row "+i+" - decimal");
			assertEquals(rs.getString(4),i+".10");
			assertFalse(rs.wasNull());
			System.out.println();

			// real
			System.out.println("  row "+i+" - real");
			assertTrue(rs.getString(5)!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// double
			System.out.println("  row "+i+" - double");
			assertTrue(rs.getString(6)!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// char as string
			System.out.println("  row "+i+" - char as string");
			assertEquals(rs.getString(7),"char"+i+
					"                                   ");
			assertFalse(rs.wasNull());
			System.out.println();

			// varchar as string
			System.out.println("  row "+i+" - varchar as string");
			assertEquals(rs.getString(8),"varchar"+i);
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

			// clob as string
			System.out.println("  row "+i+" - clob as string");
			assertEquals(rs.getString(12),"clob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob
			System.out.println("  row "+i+" - clob");
			clob=rs.getClob(12);
			assertEquals(clob.getSubString(1,(int)clob.length()),
								"clob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob as ascii stream
			System.out.println("  row "+i+
						" - clob as ascii stream");
			assertEquals(new String(rs.getAsciiStream(12).
						readAllBytes(),"UTF-8"),
						"clob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob as character stream
			System.out.println("  row "+i+
						" - clob as character stream");
			StringWriter sw=new StringWriter();
			rs.getCharacterStream(12).transferTo(sw);
			assertEquals(sw.toString(),"clob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob
			System.out.println("  row "+i+" - blob");
			blob=rs.getBlob(13);
			assertEquals(new String(
					blob.getBytes(1,(int)blob.length()),
					"UTF-8"),
					"blob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob as bytes
			System.out.println("  row "+i+" - blob as bytes");
			assertEquals(new String(
					rs.getBytes(13),"UTF-8"),
					"blob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob as binary stream
			System.out.println("  row "+i+
						" - blob as binary stream");
			assertEquals(new String(rs.getBinaryStream(13).
						readAllBytes(),"UTF-8"),
						"blob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// url
			System.out.println("  row "+i+" - url");
			if (issqlrelay) {
				URL	urlvar=rs.getURL(14);
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
			"	testsmallint");
		assertTrue((rs!=null));
		System.out.println();
		for (int i=1; i<=4; i++) {

			assertTrue(rs.next());
			System.out.println();

			// smallint as short
			System.out.println("  row "+i+" - smallint as short");
			assertEquals(rs.getShort("TESTSMALLINT"),(short)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// smallint as int
			System.out.println("  row "+i+" - smallint as int");
			assertEquals(rs.getInt("TESTSMALLINT"),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// int
			System.out.println("  row "+i+" - int");
			assertEquals(rs.getInt("TESTINT"),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// bigint
			System.out.println("  row "+i+" - bigint");
			assertEquals(rs.getLong("TESTBIGINT"),(long)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// decimal
			System.out.println("  row "+i+" - decimal");
			assertEquals(rs.getString("TESTDECIMAL"),i+".10");
			assertFalse(rs.wasNull());
			System.out.println();

			// real
			System.out.println("  row "+i+" - real");
			assertTrue(rs.getString("TESTREAL")!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// double
			System.out.println("  row "+i+" - double");
			assertTrue(rs.getString("TESTDOUBLE")!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// char as string
			System.out.println("  row "+i+" - char as string");
			assertEquals(rs.getString("TESTCHAR"),"char"+i+
					"                                   ");
			assertFalse(rs.wasNull());
			System.out.println();

			// varchar as string
			System.out.println("  row "+i+" - varchar as string");
			assertEquals(rs.getString("TESTVARCHAR"),"varchar"+i);
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

			// clob as string
			System.out.println("  row "+i+" - clob as string");
			assertEquals(rs.getString("TESTCLOB"),"clob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob
			System.out.println("  row "+i+" - clob");
			clob=rs.getClob("TESTCLOB");
			assertEquals(clob.getSubString(1,(int)clob.length()),
								"clob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob as ascii stream
			System.out.println("  row "+i+
						" - clob as ascii stream");
			assertEquals(new String(rs.getAsciiStream("TESTCLOB").
						readAllBytes(),"UTF-8"),
						"clob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// clob as character stream
			System.out.println("  row "+i+
						" - clob as character stream");
			StringWriter sw=new StringWriter();
			rs.getCharacterStream("TESTCLOB").transferTo(sw);
			assertEquals(sw.toString(),"clob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// blob
			System.out.println("  row "+i+" - blob");
			blob=rs.getBlob("TESTBLOB");
			assertEquals(new String(
					blob.getBytes(1,(int)blob.length()),
					"UTF-8"),
					"blob"+i);
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
			System.out.println("  row "+i+
						" - blob as binary stream");
			assertEquals(new String(rs.getBinaryStream("TESTBLOB").
						readAllBytes(),"UTF-8"),
						"blob"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// url
			System.out.println("  row "+i+" - url");
			if (issqlrelay) {
				URL	urlvar=rs.getURL("TESTURL");
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
		try {
			stmt.executeUpdate("drop procedure testproc2");
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate(
			"create procedure testproc2("+
			"	in in1 int, "+
			"	in in2 double, "+
			"	in in3 varchar(20), "+
			"	out out1 int, "+
			"	out out2 double, "+
			"	out out3 varchar(20)) "+
			"language sql "+
			"begin "+
			"	set out1 = in1; "+
			"	set out2 = in2; "+
			"	set out3 = in3; "+
			"end"),0);
		cstmt=con.prepareCall("call testproc2(?,?,?,?,?,?)");
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
		cstmt.close();
		stmt.executeUpdate("drop procedure testproc2");
		System.out.println();


		// output bind by name
		System.out.println("OUTPUT BIND BY NAME:");
		assertEquals(stmt.executeUpdate(
			"create procedure testproc2("+
			"	in in1 int, "+
			"	in in2 double, "+
			"	in in3 varchar(20), "+
			"	out out1 int, "+
			"	out out2 double, "+
			"	out out3 varchar(20)) "+
			"language sql "+
			"begin "+
			"	set out1 = in1; "+
			"	set out2 = in2; "+
			"	set out3 = in3; "+
			"end"),0);
		cstmt=con.prepareCall("call testproc2(?,?,?,?,?,?)");
		cstmt.setInt(1,1);
		cstmt.setDouble(2,1.1);
		cstmt.setString(3,"hello");
		cstmt.registerOutParameter("out1",Types.INTEGER);
		cstmt.registerOutParameter("out2",Types.DOUBLE);
		cstmt.registerOutParameter("out3",Types.VARCHAR);
		assertFalse(cstmt.execute());
		assertEquals(cstmt.getInt("out1"),1);
		assertEquals(cstmt.getDouble("out2"),1.1);
		assertEquals(cstmt.getString("out3"),"hello");
		cstmt.close();
		stmt.executeUpdate("drop procedure testproc2");
		System.out.println();


		// clob and blob output bind
		System.out.println("CLOB AND BLOB OUTPUT BIND:");
		try {
			stmt.executeUpdate("drop procedure testproc2");
		} catch (Exception ex) {
		}
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
			"	?, "+
			"	?)");
		assertTrue((pstmt!=null));
		pstmt.setString(1,"hello");
		pstmt.setBytes(2,"hello".getBytes(StandardCharsets.UTF_8));
		assertEquals(pstmt.executeUpdate(),1);
		pstmt.close();
		assertEquals(stmt.executeUpdate(
			"create procedure testproc2("+
			"	out out1 clob, "+
			"	out out2 blob) "+
			"language sql "+
			"begin "+
			"	select testclob into out1 from testtable1; "+
			"	select testblob into out2 from testtable1; "+
			"end"),0);
		cstmt=con.prepareCall("call testproc2(?,?)");
		cstmt.registerOutParameter(1,Types.CLOB);
		cstmt.registerOutParameter(2,Types.BLOB);
		assertFalse(cstmt.execute());
		assertEquals(cstmt.getString(1),"hello");
		assertEquals(new String(cstmt.getBytes(2),
					StandardCharsets.UTF_8),"hello");
		cstmt.close();
		stmt.executeUpdate("drop procedure testproc2");
		stmt.executeUpdate("drop table testtable1");
		System.out.println();

		// stored procedures
		System.out.println("STORED PROCEDURES:");
		// return values
		try {
			stmt.executeUpdate("drop procedure testproc");
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate(
			"create procedure testproc("+
			"	in in1 int, "+
			"	in in2 double, "+
			"	in in3 varchar(20), "+
			"	out out1 int, "+
			"	out out2 double, "+
			"	out out3 varchar(20)) "+
			"language sql "+
			"begin "+
			"	set out1 = in1; "+
			"	set out2 = in2; "+
			"	set out3 = in3; "+
			"end"),0);
		cstmt=con.prepareCall("call testproc(?,?,?,?,?,?)");
		cstmt.setInt(1,1);
		cstmt.setDouble(2,1.1);
		cstmt.setString(3,"hello");
		cstmt.registerOutParameter(4,Types.INTEGER);
		cstmt.registerOutParameter(5,Types.DOUBLE);
		cstmt.registerOutParameter(6,Types.VARCHAR);
		assertFalse(cstmt.execute());
		assertEquals(cstmt.getInt(4),1);
		assertEquals(cstmt.getString(6),"hello");
		cstmt.close();
		stmt.executeUpdate("drop procedure testproc");
		System.out.println();

		// catalog list
		System.out.println("CATALOG LIST: ");
		rs=md.getCatalogs();
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		rs.close();
		System.out.println();

		// schema list
		System.out.println("SCHEMA LIST: ");
		rs=md.getSchemas();
		assertTrue((rs!=null));
		rs.close();
		System.out.println();

		// table type list
		System.out.println("TABLE TYPE LIST: ");
		rs=md.getTableTypes();
		assertTrue((rs!=null));
		rs.close();
		System.out.println();

		// table list
		System.out.println("TABLE LIST: ");
		rs=md.getTables(null,null,"%",null);
		assertTrue((rs!=null));
		rs.close();
		System.out.println();

		// type info list
		System.out.println("TYPE INFO LIST: ");
		rs=md.getTypeInfo();
		assertTrue((rs!=null));
		rs.close();
		System.out.println();

		// column list
		System.out.println("COLUMN LIST: ");
		stmt=con.createStatement();
		try {
			stmt.executeUpdate("drop table testtable");
		} catch (Exception ex) {
		}
		stmt.executeUpdate(
			"create table testtable ("+
			"	testsmallint smallint, "+
			"	testint integer, "+
			"	testbigint bigint, "+
			"	testdecimal decimal(10,2), "+
			"	testreal real, "+
			"	testdouble double, "+
			"	testchar char(40), "+
			"	testvarchar varchar(40), "+
			"	testdate date, "+
			"	testtime time, "+
			"	testtimestamp timestamp, "+
			"	testclob clob, "+
			"	testblob blob, "+
			"	testurl varchar(60))");
		rs=md.getColumns(null,null,"TESTTABLE","%");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTSMALLINT");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTINT");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTBIGINT");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTDECIMAL");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTREAL");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTDOUBLE");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTCHAR");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTVARCHAR");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTDATE");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTTIME");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTTIMESTAMP");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTCLOB");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTBLOB");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"TESTURL");
		rs.close();
		stmt.executeUpdate("drop table testtable");
		System.out.println();

		// primary key list
		System.out.println("PRIMARY KEY LIST: ");
		rs=md.getPrimaryKeys(null,null,"TESTTABLE");
		assertTrue((rs!=null));
		rs.close();
		System.out.println();

		// key and index list
		System.out.println("KEY AND INDEX LIST: ");
		rs=md.getIndexInfo(null,null,"TESTTABLE",false,false);
		assertTrue((rs!=null));
		rs.close();
		System.out.println();

		// procedure list
		System.out.println("PROCEDURE LIST: ");
		rs=md.getProcedures(null,null,"%");
		assertTrue((rs!=null));
		rs.close();
		System.out.println();

		// procedure parameter list
		System.out.println("PROCEDURE PARAMETER LIST: ");
		rs=md.getProcedureColumns(null,null,"%","%");
		assertTrue((rs!=null));
		rs.close();
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
			stmt.executeUpdate("insert into nonexistent_table values (1)");
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
		con.rollback();
		con.close();

		reportTestStatus();

		System.exit(status);
	}
}
