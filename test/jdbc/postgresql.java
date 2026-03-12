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

class postgresql extends sqlrtest {
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
		} else if (classpath.contains("postgresql")) {
			driver="org.postgresql.Driver";
			url="jdbc:postgresql://postgresql:5432/"+hostname+"";
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
		assertTrue(boolval);
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
			assertEquals(stringval,"postgresql");
		} else {
			assertEquals(stringval,"PostgreSQL");
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
		assertEquals(stringval,"");
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
		assertEquals(intval,0);
		System.out.println();

		// getMaxCatalogNameLength
		System.out.println("  getMaxCatalogNameLength");
		intval=md.getMaxCatalogNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,63);
		System.out.println();

		// getMaxCharLiteralLength
		System.out.println("  getMaxCharLiteralLength");
		intval=md.getMaxCharLiteralLength();
		System.out.println("    "+intval);
		assertEquals(intval,0);
		System.out.println();

		// getMaxColumnNameLength
		System.out.println("  getMaxColumnNameLength");
		intval=md.getMaxColumnNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,63);
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
		assertEquals(intval,32);
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
		assertEquals(intval,0);
		System.out.println();

		// getMaxColumnsInTable
		System.out.println("  getMaxColumnsInTable");
		intval=md.getMaxColumnsInTable();
		System.out.println("    "+intval);
		assertEquals(intval,1600);
		System.out.println();

		// getMaxConnections
		System.out.println("  getMaxConnections");
		intval=md.getMaxConnections();
		System.out.println("    "+intval);
		if (issqlrelay) {
			// varies by sqlrelay config
			assertTrue(intval>0);
		} else {
			// postgresql jdbc returns 8192 for this
			assertEquals(intval,8192);
		}
		System.out.println();

		// getMaxCursorNameLength
		System.out.println("  getMaxCursorNameLength");
		intval=md.getMaxCursorNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,63);
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
		assertEquals(intval,63);
		System.out.println();

		// getMaxRowSize
		System.out.println("  getMaxRowSize");
		intval=md.getMaxRowSize();
		System.out.println("    "+intval);
		assertEquals(intval,1073741824);
		System.out.println();

		// getMaxSchemaNameLength
		System.out.println("  getMaxSchemaNameLength");
		intval=md.getMaxSchemaNameLength();
		System.out.println("    "+intval);
		assertEquals(intval,63);
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
		assertEquals(intval,63);
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
		assertEquals(intval,63);
		System.out.println();

		// getNumericFunctions
		System.out.println("  getNumericFunctions");
		stringval=md.getNumericFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"abs,acos,asin,atan,atan2,ceiling,cos,cot,degrees,exp,floor,log,log10,mod,pi,power,radians,round,sign,sin,sqrt,tan,truncate");
		System.out.println();

		// getProcedureTerm
		System.out.println("  getProcedureTerm");
		stringval=md.getProcedureTerm();
		System.out.println("    "+stringval);
		assertEquals(stringval,"function");
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
		assertEquals(stringval,"abort,access,aggregate,also,analyse,analyze,attach,backward,bit,cache,checkpoint,class,cluster,columns,comment,comments,concurrently,configuration,conflict,connection,content,conversion,copy,cost,csv,current_catalog,current_schema,database,delimiter,delimiters,depends,detach,dictionary,disable,discard,do,document,enable,encoding,encrypted,enum,event,exclusive,explain,extension,family,force,forward,freeze,functions,generated,greatest,groups,handler,header,if,ilike,immutable,implicit,import,include,index,indexes,inherit,inherits,inline,instead,isnull,label,leakproof,least,limit,listen,load,location,lock,locked,logged,mapping,materialized,mode,move,nothing,notify,notnull,nowait,off,offset,oids,operator,owned,owner,parallel,parser,passing,password,plans,policy,prepared,procedural,procedures,program,publication,quote,reassign,recheck,refresh,reindex,rename,replace,replica,reset,restrict,returning,routines,rule,schemas,sequences,server,setof,share,show,skip,snapshot,stable,standalone,statistics,stdin,stdout,storage,stored,strict,strip,subscription,support,sysid,tables,tablespace,temp,template,text,truncate,trusted,types,unencrypted,unlisten,unlogged,until,vacuum,valid,validate,validator,variadic,verbose,version,views,volatile,whitespace,wrapper,xml,xmlattributes,xmlconcat,xmlelement,xmlexists,xmlforest,xmlnamespaces,xmlparse,xmlpi,xmlroot,xmlserialize,xmltable,yes");
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
		assertEquals(stringval,"ascii,char,concat,lcase,left,length,ltrim,repeat,rtrim,space,substring,ucase,replace");
		System.out.println();

		// getSystemFunctions
		System.out.println("  getSystemFunctions");
		stringval=md.getSystemFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"database,ifnull,user");
		System.out.println();

		// getTimeDateFunctions
		System.out.println("  getTimeDateFunctions");
		stringval=md.getTimeDateFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"curdate,curtime,dayname,dayofmonth,dayofweek,dayofyear,hour,minute,month,monthname,now,quarter,second,week,year,timestampadd");
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
		assertTrue(boolval);
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
		
		// supportsConvert (with types)
		System.out.println("  supportsConvert (with types)");
		boolval=md.supportsConvert(Types.INTEGER,Types.VARCHAR);
		System.out.println("    "+boolval);
		assertFalse(boolval);
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
		assertFalse(boolval);
		System.out.println();

		// supportsPositionedUpdate
		System.out.println("  supportsPositionedUpdate");
		boolval=md.supportsPositionedUpdate();
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


		// statement
		System.out.println("STATEMENT:");
		stmt=con.createStatement();
		assertTrue((stmt!=null));
		System.out.println();


		// create table
		System.out.println("CREATE TABLE:");
		stmt.executeUpdate("drop table if exists testtable");
		assertEquals(stmt.executeUpdate(
			"create table testtable ("+
			"	testint int, "+
			"	testfloat float, "+
			"	testreal real, "+
			"	testsmallint smallint, "+
			"	testchar char(40), "+
			"	testvarchar varchar(40), "+
			"	testdate date, "+
			"	testtime time, "+
			"	testtimestamp timestamp, "+
			"	testtext text, "+
			"	testbytea bytea, "+
			"	testurl varchar(60))"),0);
		System.out.println();


		// insert
		System.out.println("INSERT:");
		assertFalse(stmt.execute(
			"insert into "+
			"	testtable "+
			"values ("+
			"	1, "+
			"	1.1, "+
			"	1.1, "+
			"	1, "+
			"	'char1', "+
			"	'varchar1', "+
			"	'2001-01-01', "+
			"	'01:00:00', "+
			"	NULL, "+
			"	'text1', "+
			"	'bytea1', "+
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
				"	$1, "+
				"	$2, "+
				"	$3, "+
				"	$4, "+
				"	$5, "+
				"	$6, "+
				"	$7, "+
				"	$8, "+
				"	NULL, "+
				"	$9, "+
				"	$10, "+
				"	$11)");
		} else {
			// postgresql jdbc requires ? format
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
				"	NULL, "+
				"	?, "+
				"	?, "+
				"	?)");
		}
		assertFalse(pstmt.isClosed());
		for (int i=2; i<=4; i++) {
			pstmt.clearParameters();
			pstmt.setInt(1,i);
			pstmt.setDouble(2,i+0.1);
			pstmt.setDouble(3,i+0.1);
			pstmt.setInt(4,i);
			pstmt.setString(5,"char"+i);
			pstmt.setString(6,"varchar"+i);

			cal.set(Calendar.YEAR,2000+i);
			cal.set(Calendar.MONTH,Calendar.JANUARY);
			cal.set(Calendar.DAY_OF_MONTH,1);
			cal.set(Calendar.HOUR_OF_DAY,0);
			cal.set(Calendar.MINUTE,0);
			cal.set(Calendar.SECOND,0);
			cal.set(Calendar.MILLISECOND,0);
			pstmt.setDate(7,new java.sql.Date(
						cal.getTimeInMillis()));
			pstmt.setTime(8,java.sql.Time.valueOf(
						"0"+i+":00:00"));
			pstmt.setString(9,"text"+i);
			pstmt.setBytes(10,
				(new String("bytea"+i)).
				getBytes(StandardCharsets.UTF_8));
			pstmt.setString(11,
				"http://www.firstworks.com:8080/testurl"+i);
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
		assertEquals(rsmd.getColumnCount(),12);
		System.out.println();


		// column names
		System.out.println("COLUMN NAMES:");
		assertEquals(rsmd.getColumnName(1),"testint");
		assertEquals(rsmd.getColumnName(2),"testfloat");
		assertEquals(rsmd.getColumnName(3),"testreal");
		assertEquals(rsmd.getColumnName(4),"testsmallint");
		assertEquals(rsmd.getColumnName(5),"testchar");
		assertEquals(rsmd.getColumnName(6),"testvarchar");
		assertEquals(rsmd.getColumnName(7),"testdate");
		assertEquals(rsmd.getColumnName(8),"testtime");
		assertEquals(rsmd.getColumnName(9),"testtimestamp");
		assertEquals(rsmd.getColumnName(10),"testtext");
		assertEquals(rsmd.getColumnName(11),"testbytea");
		assertEquals(rsmd.getColumnName(12),"testurl");
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

			// float
			System.out.println("  row "+i+" - float");
			assertTrue(rs.getString(2)!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// real
			System.out.println("  row "+i+" - real");
			assertTrue(rs.getString(3)!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// smallint
			System.out.println("  row "+i+" - smallint");
			assertEquals(rs.getInt(4),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// char as string
			System.out.println("  row "+i+" - char as string");
			assertEquals(rs.getString(5),"char"+i+
					"                                   ");
			assertFalse(rs.wasNull());
			System.out.println();

			// varchar as string
			System.out.println("  row "+i+" - varchar as string");
			assertEquals(rs.getString(6),"varchar"+i);
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

			// text as string
			System.out.println("  row "+i+" - text as string");
			assertEquals(rs.getString(10),"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// text as clob
			System.out.println("  row "+i+" - text as clob");
			if (issqlrelay) {
				// postgresql jdbc fails with:
				// "Bad value for long"
				clob=rs.getClob(10);
				assertEquals(clob.getSubString(
						1,(int)clob.length()),
						"text"+i);
				assertFalse(rs.wasNull());
			}
			System.out.println();

			// text as ascii stream
			System.out.println("  row "+i+
						" - text as ascii stream");
			assertEquals(new String(rs.getAsciiStream(10).
						readAllBytes(),"UTF-8"),
						"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// text as character stream
			System.out.println("  row "+i
						+" - text as character stream");
			StringWriter sw=new StringWriter();
			rs.getCharacterStream(10).transferTo(sw);
			assertEquals(sw.toString(),"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// bytea as bytes
			System.out.println("  row "+i+" - bytea as bytes");
			if (issqlrelay) {
				// sqlrelay jdbc returns hex format
				assertEquals(new String(
					rs.getBytes(11),"UTF-8"),
					"\\x6279746561"+
					Integer.toHexString(48+i));
			} else {
				// postgresql jdbc returns raw bytes
				assertEquals(new String(
					rs.getBytes(11),"UTF-8"),
					"bytea"+i);
			}
			assertFalse(rs.wasNull());
			System.out.println();

			// bytea as binary stream
			System.out.println("  row "+i+
						" - bytea as binary stream");
			if (issqlrelay) {
				// sqlrelay jdbc returns hex format
				assertEquals(new String(
					rs.getBinaryStream(11).
					readAllBytes(),"UTF-8"),
					"\\x6279746561"+
					Integer.toHexString(48+i));
			} else {
				// postgresql jdbc returns raw bytes
				assertEquals(new String(
					rs.getBinaryStream(11).
					readAllBytes(),"UTF-8"),
					"bytea"+i);
			}
			assertFalse(rs.wasNull());
			System.out.println();

			// url
			System.out.println("  row "+i+" - url");
			if (issqlrelay) {
				// postgresql jdbc doesn't support getURL
				URL	urlvar=rs.getURL(12);
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

			// float
			System.out.println("  row "+i+" - float");
			assertTrue(rs.getString("testfloat")!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// real
			System.out.println("  row "+i+" - real");
			assertTrue(rs.getString("testreal")!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// smallint
			System.out.println("  row "+i+" - smallint");
			assertEquals(rs.getInt("testsmallint"),i);
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

			// text as string
			System.out.println("  row "+i+" - text as string");
			assertEquals(rs.getString("testtext"),"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// text as clob
			System.out.println("  row "+i+" - text as clob");
			if (issqlrelay) {
				// postgresql jdbc fails with:
				// "Bad value for long"
				clob=rs.getClob("testtext");
				assertEquals(clob.getSubString(
						1,(int)clob.length()),
						"text"+i);
				assertFalse(rs.wasNull());
			}
			System.out.println();

			// text as ascii stream
			System.out.println("  row "+i+
						" - text as ascii stream");
			assertEquals(new String(rs.getAsciiStream("testtext").
						readAllBytes(),"UTF-8"),
						"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// text as character stream
			System.out.println("  row "+i
						+" - text as character stream");
			StringWriter sw=new StringWriter();
			rs.getCharacterStream("testtext").transferTo(sw);
			assertEquals(sw.toString(),"text"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// bytea as bytes
			System.out.println("  row "+i+" - bytea as bytes");
			if (issqlrelay) {
				// sqlrelay jdbc returns hex format
				assertEquals(new String(
					rs.getBytes("testbytea"),"UTF-8"),
					"\\x6279746561"+
					Integer.toHexString(48+i));
			} else {
				// postgresql jdbc returns raw bytes
				assertEquals(new String(
					rs.getBytes("testbytea"),"UTF-8"),
					"bytea"+i);
			}
			assertFalse(rs.wasNull());
			System.out.println();

			// bytea as binary stream
			System.out.println("  row "+i
						+" - bytea as binary stream");
			if (issqlrelay) {
				// sqlrelay jdbc returns hex format
				assertEquals(new String(
					rs.getBinaryStream("testbytea").
					readAllBytes(),"UTF-8"),
					"\\x6279746561"+
					Integer.toHexString(48+i));
			} else {
				// postgresql jdbc returns raw bytes
				assertEquals(new String(
					rs.getBinaryStream("testbytea").
					readAllBytes(),"UTF-8"),
					"bytea"+i);
			}
			assertFalse(rs.wasNull());
			System.out.println();

			// url
			System.out.println("  row "+i+" - url");
			if (issqlrelay) {
				// postgresql jdbc doesn't support getURL
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
		System.out.println();


		// commit
		System.out.println("COMMIT:");
		con.commit();
		stmt.executeUpdate("drop table testtable");
		stmt.close();
		assertTrue(stmt.isClosed());
		System.out.println();


		// stored procedures
		System.out.println("STORED PROCEDURES:");
		stmt=con.createStatement();
		// return no value
		stmt.executeUpdate("drop function if exists "+
					"testfunc(int,float,char)");
		assertEquals(stmt.executeUpdate(
			"create function testfunc(int,float,char(20)) "+
			"	returns void as "+
			"'declare "+
			"	in1 int; "+
			"	in2 float; "+
			"	in3 char(20); "+
			"begin "+
			"	in1:=$1; "+
			"	in2:=$2; "+
			"	in3:=$3; "+
			"	return; "+
			"end;' "+
			"language plpgsql"),0);
		if (issqlrelay) {
			pstmt=con.prepareStatement(
				"select testfunc($1,$2,$3)");
		} else {
			// postgresql jdbc requires ? format
			pstmt=con.prepareStatement(
				"select testfunc(?,?,?)");
		}
		assertTrue((pstmt!=null));
		pstmt.setInt(1,1);
		pstmt.setDouble(2,1.1);
		pstmt.setString(3,"hello");
		pstmt.execute();
		pstmt.close();
		stmt.executeUpdate("drop function if exists "+
					"testfunc(int,float,char)");
		System.out.println();
		// return single value (function)
		assertEquals(stmt.executeUpdate(
			"create function testfunc(int,float,char(20)) "+
			"	returns int as "+
			"'begin "+
			"	return $1; "+
			"end;' "+
			"language plpgsql"),0);
		if (issqlrelay) {
			pstmt=con.prepareStatement(
				"select testfunc($1,$2,$3)");
		} else {
			// postgresql jdbc requires ? format
			pstmt=con.prepareStatement(
				"select testfunc(?,?,?)");
		}
		assertTrue((pstmt!=null));
		pstmt.setInt(1,1);
		pstmt.setDouble(2,1.1);
		pstmt.setString(3,"hello");
		rs=pstmt.executeQuery();
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"1");
		rs.close();
		pstmt.close();
		stmt.executeUpdate("drop function testfunc(int,float,char)");
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
			String	tcat=rs.getString("TABLE_CAT");
			if (tcat!=null && tcat.equals(hostname)) {
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
		assertTrue(rs.next());
		assertEquals(rs.getString("TABLE_SCHEM"),"information_schema");
		assertTrue(rs.next());
		assertEquals(rs.getString("TABLE_SCHEM"),"pg_catalog");
		assertTrue(rs.next());
		assertEquals(rs.getString("TABLE_SCHEM"),"public");
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
			if (ttname!=null && ttname.equalsIgnoreCase("TABLE")) {
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
		rs=md.getTables(null,null,"%",
				new String[] {"TABLE"});
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),10);
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
		counter=0;
		while (rs.next()) {
			String name=rs.getString("TABLE_NAME");
			if (name.equalsIgnoreCase("testtable1") ||
					name.equalsIgnoreCase("testtable2") ||
					name.equalsIgnoreCase("testtable3") ||
					name.equalsIgnoreCase("testtable4")) {
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
		stmt=con.createStatement();
		stmt.executeUpdate("drop table if exists testtable");
		stmt.executeUpdate(
			"create table testtable ("+
			"	testint int, "+
			"	testfloat float, "+
			"	testreal real, "+
			"	testsmallint smallint, "+
			"	testchar char(40), "+
			"	testvarchar varchar(40), "+
			"	testdate date, "+
			"	testtime time, "+
			"	testtimestamp timestamp, "+
			"	testtext text, "+
			"	testbytea bytea, "+
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
		assertEquals(rs.getString("COLUMN_NAME"),"testint");
		assertEquals(rs.getString("TYPE_NAME"),"int4");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testfloat");
		assertEquals(rs.getString("TYPE_NAME"),"float8");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testreal");
		assertEquals(rs.getString("TYPE_NAME"),"float4");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testsmallint");
		assertEquals(rs.getString("TYPE_NAME"),"int2");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testchar");
		assertEquals(rs.getString("TYPE_NAME"),"bpchar");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testvarchar");
		assertEquals(rs.getString("TYPE_NAME"),"varchar");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testdate");
		assertEquals(rs.getString("TYPE_NAME"),"date");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testtime");
		assertEquals(rs.getString("TYPE_NAME"),"time");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testtimestamp");
		assertEquals(rs.getString("TYPE_NAME"),"timestamp");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testtext");
		assertEquals(rs.getString("TYPE_NAME"),"text");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testbytea");
		assertEquals(rs.getString("TYPE_NAME"),"bytea");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"testurl");
		assertEquals(rs.getString("TYPE_NAME"),"varchar");
		rs.close();
		stmt.executeUpdate("drop table testtable");
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
		assertTrue(rs.getString("TABLE_NAME").
					equalsIgnoreCase("testtable"));
		assertTrue(rs.getString("COLUMN_NAME").
					equalsIgnoreCase("col1"));
		assertEquals(rs.getString("KEY_SEQ"),"1");
		assertTrue(rs.getString("PK_NAME")!=null &&
				rs.getString("PK_NAME").length()>0);
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
		assertEquals(rsmd.getColumnCount(),14);
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
		assertEquals(rsmd.getColumnName(col++),
						"REMARKS");
		assertTrue(rs.next());
		assertTrue(rs.getString("TABLE_NAME").
					equalsIgnoreCase("testtable"));
		assertEquals(rs.getString("NON_UNIQUE"),"f");
		assertEquals(rs.getString("ORDINAL_POSITION"),"1");
		assertTrue(rs.getString("COLUMN_NAME").
					equalsIgnoreCase("col1"));
		assertEquals(rs.getString("ASC_OR_DESC"),"A");
		assertEquals(rs.getString("TYPE"),"3");
		assertTrue(rs.getString("INDEX_NAME")!=null &&
				rs.getString("INDEX_NAME").length()>0);
		assertFalse(rs.next());
		rs.close();
		stmt.executeUpdate("drop table if exists testtable");
		System.out.println();


		// procedure list
		System.out.println("PROCEDURE LIST:");
		stmt.executeUpdate("drop function if exists "+
					"testproc1(int,char,varchar,date)");
		stmt.executeUpdate("drop function if exists "+
					"testproc2(int,char,varchar,date)");
		stmt.executeUpdate("drop function if exists "+
					"testproc3(int,char,varchar,date)");
		stmt.executeUpdate("drop function if exists "+
					"testproc4(int,char,varchar,date)");
		stmt.executeUpdate(
			"create function testproc1("+
			"	in1 int, "+
			"	in2 char(20), "+
			"	in3 varchar(20), "+
			"	in4 date) "+
			"returns void "+
			"as 'begin end;' "+
			"language plpgsql");
		stmt.executeUpdate(
			"create function testproc2("+
			"	in1 int, "+
			"	in2 char(20), "+
			"	in3 varchar(20), "+
			"	in4 date) "+
			"returns void "+
			"as 'begin end;' "+
			"language plpgsql");
		stmt.executeUpdate(
			"create function testproc3("+
			"	in1 int, "+
			"	in2 char(20), "+
			"	in3 varchar(20), "+
			"	in4 date) "+
			"returns void "+
			"as 'begin end;' "+
			"language plpgsql");
		stmt.executeUpdate(
			"create function testproc4("+
			"	in1 int, "+
			"	in2 char(20), "+
			"	in3 varchar(20), "+
			"	in4 date) "+
			"returns void "+
			"as 'begin end;' "+
			"language plpgsql");
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
			// postgresql jdbc returns unnamed columns
			assertEquals(rsmd.getColumnName(col++),"?column?");
			assertEquals(rsmd.getColumnName(col++),"?column?");
			assertEquals(rsmd.getColumnName(col++),"?column?");
		}
		assertEquals(rsmd.getColumnName(col++),"REMARKS");
		assertEquals(rsmd.getColumnName(col++),"PROCEDURE_TYPE");
		if (!issqlrelay) {
			// postgresql jdbc returns this column
			assertEquals(rsmd.getColumnName(col++),"SPECIFIC_NAME");
		}
		counter=0;
		while (rs.next()) {
			String name=rs.getString("PROCEDURE_NAME");
			if (name.equals("testproc1") ||
					name.equals("testproc2") ||
					name.equals("testproc3") ||
					name.equals("testproc4")) {
				counter++;
			}
		}
		if (issqlrelay) {
			assertEquals(counter,4);
		} else {
			// postgresl jdbc returns an empty result set (bug?)
			assertEquals(counter,0);
		}
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
		assertEquals(rs.getString("COLUMN_NAME"),"returnValue");
		assertEquals(rs.getString("TYPE_NAME"),"void");
		assertEquals(rs.getString("ORDINAL_POSITION"),"0");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"in1");
		assertEquals(rs.getString("TYPE_NAME"),"int4");
		assertEquals(rs.getString("ORDINAL_POSITION"),"1");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"in2");
		assertEquals(rs.getString("TYPE_NAME"),"bpchar");
		assertEquals(rs.getString("ORDINAL_POSITION"),"2");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"in3");
		assertEquals(rs.getString("TYPE_NAME"),"varchar");
		assertEquals(rs.getString("ORDINAL_POSITION"),"3");
		assertTrue(rs.next());
		assertEquals(rs.getString("COLUMN_NAME"),"in4");
		assertEquals(rs.getString("TYPE_NAME"),"date");
		assertEquals(rs.getString("ORDINAL_POSITION"),"4");
		rs.close();
		stmt.executeUpdate("drop function "+
					"testproc1(int,char,varchar,date)");
		stmt.executeUpdate("drop function "+
					"testproc2(int,char,varchar,date)");
		stmt.executeUpdate("drop function "+
					"testproc3(int,char,varchar,date)");
		stmt.executeUpdate("drop function "+
					"testproc4(int,char,varchar,date)");
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
