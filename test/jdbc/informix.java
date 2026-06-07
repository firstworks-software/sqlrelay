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

class informix extends sqlrtest {
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
		} else if (classpath.contains("ifxjdbc")) {
			driver="com.informix.jdbc.IfxDriver";
			url="jdbc:informix-sqli://informix:29756/"+
				hostname+":INFORMIXSERVER=ol_informix1210";
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
		Clob			clob=null;
		Blob			blob=null;
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
		assertTrue(boolval);
		System.out.println();

		// autoCommitFailureClosesAllResultSets
		System.out.println("  autoCommitFailureClosesAllResultSets");
		if (issqlrelay) {
			// informix jdbc doesn't support this
			boolval=md.autoCommitFailureClosesAllResultSets();
			System.out.println("    "+boolval);
			assertFalse(boolval);
		}
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
			// informix jdbc doesn't support this
			boolval=md.generatedKeyAlwaysReturned();
			System.out.println("    "+boolval);
			assertFalse(boolval);
		}
		System.out.println();

		// getCatalogSeparator
		System.out.println("  getCatalogSeparator");
		stringval=md.getCatalogSeparator();
		System.out.println("    "+stringval);
		assertEquals(stringval,":");
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
			assertEquals(stringval,"informix");
		} else {
			assertEquals(stringval,"Informix Dynamic Server");
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
			// varies by driver version
			assertTrue(stringval!=null);
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
		assertEquals(stringval,"$");
		System.out.println();

		// getIdentifierQuoteString
		System.out.println("  getIdentifierQuoteString");
		stringval=md.getIdentifierQuoteString();
		System.out.println("    "+stringval);
		assertEquals(stringval," ");
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
		assertEquals(intval,128);
		System.out.println();

		// getMaxCharLiteralLength
		System.out.println("  getMaxCharLiteralLength");
		intval=md.getMaxCharLiteralLength();
		System.out.println("    "+intval);
		assertEquals(intval,256);
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
		assertEquals(intval,32767);
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
		assertEquals(intval,32767);
		System.out.println();

		// getMaxColumnsInSelect
		System.out.println("  getMaxColumnsInSelect");
		intval=md.getMaxColumnsInSelect();
		System.out.println("    "+intval);
		assertEquals(intval,32767);
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
		assertEquals(intval,255);
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
		assertEquals(intval,32767);
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
		assertEquals(intval,2147483647);
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
		assertEquals(intval,32);
		System.out.println();

		// getNumericFunctions
		System.out.println("  getNumericFunctions");
		stringval=md.getNumericFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"abs,mod,pow,root,round,sqrt,exp,logn,log10,cos,sin,tan,asin,acos,atan,atan2");
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
		assertEquals(intval,2);
		System.out.println();

		// getRowIdLifetime
		System.out.println("  getRowIdLifetime");
		if (issqlrelay) {
			// informix jdbc doesn't support this
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
		assertEquals(stringval,"user");
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
		assertEquals(stringval,"after,ansi,append,attach,audit,before,bitmap,boolean,buffered,byte,cache,call,cluster,clustersize,codeset,database,datafiles,dataskip,datetime,dba,dbdate,dbmoney,debug,define,delimiter,deluxe,detach,dirty,distributions,document,each,elif,exclusive,exit,explain,express,expression,extend,extent,file,fillfactor,foreach,format,fraction,fragment,gk,hash,high,hold,hybrid,if,index,init,labeleq,labelge,labelgt,labelle,labellt,let,listing,lock,log,low,matches,maxerrors,medium,mode,modify,money,mounting,new,nvarchar,off,old,operational,optical,optimization,page,pdqpriority,pload,private,raise,range,raw,recordend,recover,referencing,rejectfile,release,remainder,rename,reserve,resolution,resource,resume,return,returning,returns,ridlist,robin,rollforward,round,row,rowids,sameas,samples,schedule,scratch,serial,share,skall,skinhibit,skshow,smallfloat,stability,standard,start,static,statistics,stdev,step,sync,synonym,system,temp,text,timeout,trace,trigger,units,unlock,variance,wait,while,xload,xunload");
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
		assertEquals(stringval,"trunc,length");
		System.out.println();

		// getSystemFunctions
		System.out.println("  getSystemFunctions");
		stringval=md.getSystemFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"avg,max,min,sum,count,range,stdev,variance,trim,hex,filetoblob,filetoclob,lotofile,lotocopy");
		System.out.println();

		// getTimeDateFunctions
		System.out.println("  getTimeDateFunctions");
		stringval=md.getTimeDateFunctions();
		System.out.println("    "+stringval);
		assertEquals(stringval,"date,day,month,weekday,year,extend,mdy");
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
		assertFalse(boolval);
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
		assertTrue(boolval);
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
		assertTrue(boolval);
		System.out.println();

		// storesLowerCaseQuotedIdentifiers
		System.out.println("  storesLowerCaseQuotedIdentifiers");
		boolval=md.storesLowerCaseQuotedIdentifiers();
		System.out.println("    "+boolval);
		assertTrue(boolval);
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
		assertTrue(boolval);
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
		assertFalse(boolval);
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
		assertFalse(boolval);
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
		assertFalse(boolval);
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
		if (issqlrelay) {
			// informix jdbc doesn't support this
			boolval=md.supportsStoredFunctionsUsingCallSyntax();
			System.out.println("    "+boolval);
			assertFalse(boolval);
		}
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
		stmt=con.createStatement();
		assertTrue((stmt!=null));
		System.out.println();


		// create table
		System.out.println("CREATE TABLE:");
		try {
			stmt.executeUpdate("drop table testtable");
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate(
			"create table testtable ("+
			"	testboolean boolean, "+
			"	testsmallint smallint, "+
			"	testint integer, "+
			"	testbigint bigint, "+
			"	testint8 int8, "+
			"	testdecimal decimal(10,2), "+
			"	testmoney money, "+
			"	testsmallfloat smallfloat, "+
			"	testfloat float, "+
			"	testchar char(40), "+
			"	testnchar nchar(40), "+
			"	testvarchar varchar(40), "+
			"	testnvarchar nvarchar(40), "+
			"	testlvarchar lvarchar(40), "+
			"	testdate date, "+
			"	testdatetime datetime year to second, "+
			"	testtext text, "+
			"	testbyte byte, "+
			"	testurl varchar(60))"),0);
		con.setAutoCommit(false);
		System.out.println();


		// insert
		System.out.println("INSERT:");
		assertFalse(stmt.execute(
			"insert into "+
			"	testtable "+
			"values ("+
			"	't', "+
			"	1, "+
			"	1, "+
			"	1, "+
			"	1, "+
			"	1.5, "+
			"	1.5, "+
			"	1.5, "+
			"	1.5, "+
			"	'char1', "+
			"	'nchar1', "+
			"	'varchar1', "+
			"	'nvarchar1', "+
			"	'lvarchar1', "+
			"	MDY(1,1,2001), "+
			"	'2001-01-01 01:00:00', "+
			"	NULL, "+
			"	NULL, "+
			"	'http://www.firstworks.com:8080/testurl1')"));
		assertEquals(stmt.getUpdateCount(),1);
		stmt.close();
		if (issqlrelay) {
			// informix jdbc doesn't support isClosed
			assertTrue(stmt.isClosed());
		}
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
		if (issqlrelay) {
			// informix jdbc doesn't support isClosed
			assertFalse(pstmt.isClosed());
		}
		if (issqlrelay) {
			// informix jdbc doesn't support createClob/createBlob
			clob=con.createClob();
			blob=con.createBlob();
		}
		for (int i=2; i<=4; i++) {
			pstmt.clearParameters();
			pstmt.setString(1,"t");
			pstmt.setInt(2,i);
			pstmt.setInt(3,i);
			pstmt.setLong(4,(long)i);
			pstmt.setLong(5,(long)i);
			pstmt.setDouble(6,i+0.5);
			pstmt.setDouble(7,i+0.5);
			pstmt.setDouble(8,i+0.5);
			pstmt.setDouble(9,i+0.5);
			pstmt.setString(10,"char"+i);
			pstmt.setString(11,"nchar"+i);
			pstmt.setString(12,"varchar"+i);
			pstmt.setString(13,"nvarchar"+i);
			pstmt.setString(14,"lvarchar"+i);

			cal.set(Calendar.YEAR,2000+i);
			cal.set(Calendar.MONTH,Calendar.JANUARY);
			cal.set(Calendar.DAY_OF_MONTH,1);
			cal.set(Calendar.HOUR_OF_DAY,0);
			cal.set(Calendar.MINUTE,0);
			cal.set(Calendar.SECOND,0);
			cal.set(Calendar.MILLISECOND,0);
			pstmt.setDate(15,new java.sql.Date(
						cal.getTimeInMillis()));

			cal.set(Calendar.YEAR,2000+i);
			cal.set(Calendar.MONTH,Calendar.JANUARY);
			cal.set(Calendar.DAY_OF_MONTH,1);
			cal.set(Calendar.HOUR_OF_DAY,i);
			cal.set(Calendar.MINUTE,0);
			cal.set(Calendar.SECOND,0);
			cal.set(Calendar.MILLISECOND,0);
			pstmt.setTimestamp(16,new Timestamp(
						cal.getTimeInMillis()));
			if (issqlrelay) {
				clob.setString(1,"text"+i);
				pstmt.setClob(17,clob);
				blob.setBytes(1,(new String("byte"+i)).
						getBytes(
						StandardCharsets.UTF_8));
				pstmt.setBlob(18,blob);
			} else {
				pstmt.setString(17,"text"+i);
				pstmt.setBytes(18,(new String("byte"+i)).
						getBytes(
						StandardCharsets.UTF_8));
			}
			pstmt.setString(19,"http://www.firstworks.com:8080/"+
								"testurl"+i);
			assertEquals(pstmt.executeUpdate(),1);
			System.out.println();
		}
		pstmt.close();
		if (issqlrelay) {
			// informix jdbc doesn't support isClosed
			assertTrue(pstmt.isClosed());
		}
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
		assertEquals(rsmd.getColumnCount(),19);
		System.out.println();


		// column names
		System.out.println("COLUMN NAMES:");
		assertEquals(rsmd.getColumnName(1),"testboolean");
		assertEquals(rsmd.getColumnName(2),"testsmallint");
		assertEquals(rsmd.getColumnName(3),"testint");
		assertEquals(rsmd.getColumnName(4),"testbigint");
		assertEquals(rsmd.getColumnName(5),"testint8");
		assertEquals(rsmd.getColumnName(6),"testdecimal");
		assertEquals(rsmd.getColumnName(7),"testmoney");
		assertEquals(rsmd.getColumnName(8),"testsmallfloat");
		assertEquals(rsmd.getColumnName(9),"testfloat");
		assertEquals(rsmd.getColumnName(10),"testchar");
		assertEquals(rsmd.getColumnName(11),"testnchar");
		assertEquals(rsmd.getColumnName(12),"testvarchar");
		assertEquals(rsmd.getColumnName(13),"testnvarchar");
		assertEquals(rsmd.getColumnName(14),"testlvarchar");
		assertEquals(rsmd.getColumnName(15),"testdate");
		assertEquals(rsmd.getColumnName(16),"testdatetime");
		assertEquals(rsmd.getColumnName(17),"testtext");
		assertEquals(rsmd.getColumnName(18),"testbyte");
		assertEquals(rsmd.getColumnName(19),"testurl");
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
		assertTrue(rsmd.getColumnTypeName(17)!=null);
		assertTrue(rsmd.getColumnTypeName(18)!=null);
		assertTrue(rsmd.getColumnTypeName(19)!=null);
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
		assertTrue(rsmd.getPrecision(17)>=0);
		assertTrue(rsmd.getPrecision(18)>=0);
		assertTrue(rsmd.getPrecision(19)>=0);
		System.out.println();


		// fields by index
		System.out.println("FIELDS BY INDEX:");
		for (int i=1; i<=4; i++) {

			assertTrue(rs.next());
			System.out.println();

			// boolean as string
			System.out.println("  row "+i+" - boolean as string");
			if (issqlrelay) {
				// sqlrelay returns "1" for true
				assertEquals(rs.getString(1),"1");
			} else {
				assertEquals(rs.getString(1),"t");
			}
			assertFalse(rs.wasNull());
			System.out.println();

			// smallint as short
			System.out.println("  row "+i+" - smallint as short");
			assertEquals(rs.getShort(2),(short)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// smallint as int
			System.out.println("  row "+i+" - smallint as int");
			assertEquals(rs.getInt(2),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// int
			System.out.println("  row "+i+" - int");
			assertEquals(rs.getInt(3),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// bigint
			System.out.println("  row "+i+" - bigint");
			assertEquals(rs.getLong(4),(long)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// int8
			System.out.println("  row "+i+" - int8");
			assertEquals(rs.getLong(5),(long)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// decimal
			System.out.println("  row "+i+" - decimal");
			assertEquals(rs.getString(6),i+".50");
			assertFalse(rs.wasNull());
			System.out.println();

			// money
			System.out.println("  row "+i+" - money");
			String moneyval=rs.getString(7);
			// apparently inserting a literal value causes the
			// field to be populated without a leading $, but
			// inserting a value via bind causes it to be populated
			// with a leading $
			assertTrue(moneyval.equals(i+".50") ||
					moneyval.equals("$"+i+".50"));
			assertFalse(rs.wasNull());
			System.out.println();

			// smallfloat
			System.out.println("  row "+i+" - smallfloat");
			assertTrue(rs.getString(8)!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// float
			System.out.println("  row "+i+" - float");
			assertTrue(rs.getString(9)!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// char as string
			System.out.println("  row "+i+" - char as string");
			assertEquals(rs.getString(10),"char"+i+
					"                                   ");
			assertFalse(rs.wasNull());
			System.out.println();

			// nchar as string
			System.out.println("  row "+i+" - nchar as string");
			assertEquals(rs.getString(11),"nchar"+i+
					"                                  ");
			assertFalse(rs.wasNull());
			System.out.println();

			// varchar as string
			System.out.println("  row "+i+" - varchar as string");
			assertEquals(rs.getString(12),"varchar"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// nvarchar as string
			System.out.println("  row "+i+" - nvarchar as string");
			assertEquals(rs.getString(13),"nvarchar"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// lvarchar as string
			System.out.println("  row "+i+" - lvarchar as string");
			assertEquals(rs.getString(14),"lvarchar"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// date
			System.out.println("  row "+i+" - date");
			datevar=rs.getDate(15);
			cal.setTime(datevar);
			assertEquals(cal.get(Calendar.YEAR),2000+i);
			assertEquals(cal.get(Calendar.MONTH),Calendar.JANUARY);
			assertEquals(cal.get(Calendar.DAY_OF_MONTH),1);
			assertFalse(rs.wasNull());
			System.out.println();

			// datetime as timestamp
			System.out.println("  row "+i+
					" - datetime as timestamp");
			tsvar=rs.getTimestamp(16);
			cal.setTime(tsvar);
			assertEquals(cal.get(Calendar.YEAR),2000+i);
			assertEquals(cal.get(Calendar.MONTH),Calendar.JANUARY);
			assertEquals(cal.get(Calendar.DAY_OF_MONTH),1);
			assertEquals(cal.get(Calendar.HOUR_OF_DAY),i);
			assertEquals(cal.get(Calendar.MINUTE),0);
			assertEquals(cal.get(Calendar.SECOND),0);
			assertFalse(rs.wasNull());
			System.out.println();

			// text
			System.out.println("  row "+i+" - text");
			if (i==1) {
				assertEquals(rs.getString(17),null);
				assertTrue(rs.wasNull());
			} else {
				assertEquals(rs.getString(17),"text"+i);
				assertFalse(rs.wasNull());
			}
			System.out.println();

			// byte
			System.out.println("  row "+i+" - byte");
			if (i==1) {
				assertEquals(rs.getString(18),null);
				assertTrue(rs.wasNull());
			} else {
				assertEquals(new String(rs.getBytes(18),
					StandardCharsets.UTF_8),"byte"+i);
				assertFalse(rs.wasNull());
			}
			System.out.println();

			// url
			System.out.println("  row "+i+" - url");
			if (issqlrelay) {
				URL	urlvar=rs.getURL(19);
				assertEquals(urlvar.getProtocol(),"http");
				assertEquals(urlvar.getHost(),
					"www.firstworks.com");
				assertEquals(urlvar.getPort(),8080);
				assertEquals(urlvar.getPath(),
					"/testurl"+i);
				assertFalse(rs.wasNull());
			}
			System.out.println();
		}
		rs.close();
		if (issqlrelay) {
			// informix jdbc doesn't support isClosed
			assertTrue(rs.isClosed());
		}
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
			"	testsmallint");
		assertTrue((rs!=null));
		System.out.println();
		for (int i=1; i<=4; i++) {

			assertTrue(rs.next());
			System.out.println();

			// boolean as string
			System.out.println("  row "+i+" - boolean as string");
			if (issqlrelay) {
				// sqlrelay returns "1" for true
				assertEquals(rs.getString("testboolean"),"1");
			} else {
				assertEquals(rs.getString("testboolean"),"t");
			}
			assertFalse(rs.wasNull());
			System.out.println();

			// smallint as short
			System.out.println("  row "+i+" - smallint as short");
			assertEquals(rs.getShort("testsmallint"),(short)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// smallint as int
			System.out.println("  row "+i+" - smallint as int");
			assertEquals(rs.getInt("testsmallint"),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// int
			System.out.println("  row "+i+" - int");
			assertEquals(rs.getInt("testint"),i);
			assertFalse(rs.wasNull());
			System.out.println();

			// bigint
			System.out.println("  row "+i+" - bigint");
			assertEquals(rs.getLong("testbigint"),(long)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// int8
			System.out.println("  row "+i+" - int8");
			assertEquals(rs.getLong("testint8"),(long)i);
			assertFalse(rs.wasNull());
			System.out.println();

			// decimal
			System.out.println("  row "+i+" - decimal");
			assertEquals(rs.getString("testdecimal"),i+".50");
			assertFalse(rs.wasNull());
			System.out.println();

			// money
			System.out.println("  row "+i+" - money");
			// apparently, via the informix odbc driver, which
			// sqlrelay uses on the backend, inserting a literal
			// value causes the field to be populated without a
			// leading $, but inserting a value via bind causes it
			// to be populated with a leading $
			String moneyval=rs.getString("testmoney");
			assertTrue(moneyval.equals(i+".50") ||
					moneyval.equals("$"+i+".50"));
			assertFalse(rs.wasNull());
			System.out.println();

			// smallfloat
			System.out.println("  row "+i+" - smallfloat");
			assertTrue(rs.getString("testsmallfloat")!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// float
			System.out.println("  row "+i+" - float");
			assertTrue(rs.getString("testfloat")!=null);
			assertFalse(rs.wasNull());
			System.out.println();

			// char as string
			System.out.println("  row "+i+" - char as string");
			assertEquals(rs.getString("testchar"),"char"+i+
					"                                   ");
			assertFalse(rs.wasNull());
			System.out.println();

			// nchar as string
			System.out.println("  row "+i+" - nchar as string");
			assertEquals(rs.getString("testnchar"),"nchar"+i+
					"                                  ");
			assertFalse(rs.wasNull());
			System.out.println();

			// varchar as string
			System.out.println("  row "+i+" - varchar as string");
			assertEquals(rs.getString("testvarchar"),"varchar"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// nvarchar as string
			System.out.println("  row "+i+" - nvarchar as string");
			assertEquals(rs.getString("testnvarchar"),"nvarchar"+i);
			assertFalse(rs.wasNull());
			System.out.println();

			// lvarchar as string
			System.out.println("  row "+i+" - lvarchar as string");
			assertEquals(rs.getString("testlvarchar"),"lvarchar"+i);
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

			// datetime as timestamp
			System.out.println("  row "+i+
					" - datetime as timestamp");
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

			// text
			System.out.println("  row "+i+" - text");
			if (i==1) {
				assertEquals(rs.getString("testtext"),null);
				assertTrue(rs.wasNull());
			} else {
				assertEquals(rs.getString("testtext"),"text"+i);
				assertFalse(rs.wasNull());
			}
			System.out.println();

			// byte
			System.out.println("  row "+i+" - byte");
			if (i==1) {
				assertEquals(rs.getString("testbyte"),null);
				assertTrue(rs.wasNull());
			} else {
				assertEquals(new String(
					rs.getBytes("testbyte"),
					StandardCharsets.UTF_8),"byte"+i);
				assertFalse(rs.wasNull());
			}
			System.out.println();

			// url
			System.out.println("  row "+i+" - url");
			if (issqlrelay) {
				URL	urlvar=rs.getURL("testurl");
				assertEquals(urlvar.getProtocol(),"http");
				assertEquals(urlvar.getHost(),
					"www.firstworks.com");
				assertEquals(urlvar.getPort(),8080);
				assertEquals(urlvar.getPath(),
					"/testurl"+i);
				assertFalse(rs.wasNull());
			}
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
		// Informix has no MVCC; under default committed-read isolation,
		// secondstmt's catalog/data read errors with "Cannot get system
		// information for table" while con holds row locks from the
		// in-flight tx.  Use dirty-read on secondcon so it sees the
		// uncommitted writes -- the test then verifies dirty-read
		// semantics instead of MVCC visibility.
		secondstmt.executeUpdate("set isolation to dirty read");

		// from secondcon: row count should be 4 (con's 4 inserts are
		// uncommitted but visible via dirty read)
		secondrs=secondstmt.executeQuery(
				"select count(*) from testtable");
		assertTrue(secondrs.next());
		assertEquals(secondrs.getInt(1),4);
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
			"	't', "+
			"	10, "+
			"	10, "+
			"	10, "+
			"	10, "+
			"	10.50, "+
			"	10.50, "+
			"	10.50, "+
			"	10.50, "+
			"	'char10', "+
			"	'nchar10', "+
			"	'varchar10', "+
			"	'nvarchar10', "+
			"	'lvarchar10', "+
			"	MDY(1,1,2010), "+
			"	'2010-01-01 10:00:00', "+
			"	NULL, "+
			"	NULL, "+
			"	'http://www.firstworks.com:8080/testurl10')"),1);

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
			"	't', "+
			"	10, "+
			"	10, "+
			"	10, "+
			"	10, "+
			"	10.50, "+
			"	10.50, "+
			"	10.50, "+
			"	10.50, "+
			"	'char10', "+
			"	'nchar10', "+
			"	'varchar10', "+
			"	'nvarchar10', "+
			"	'lvarchar10', "+
			"	MDY(1,1,2010), "+
			"	'2010-01-01 10:00:00', "+
			"	NULL, "+
			"	NULL, "+
			"	'http://www.firstworks.com:8080/testurl10')"),1);

		// from secondcon: row count should be 5
		secondrs=secondstmt.executeQuery(
				"select count(*) from testtable");
		assertTrue(secondrs.next());
		assertEquals(secondrs.getInt(1),5);
		secondrs.close();

		// clean up secondcon
		secondstmt.close();
		secondcon.close();

		// restore con's autocommit to off and drop the table
		con.setAutoCommit(false);
		stmt.executeUpdate("drop table testtable");
		stmt.close();
		if (issqlrelay) {
			// informix jdbc doesn't support isClosed
			assertTrue(stmt.isClosed());
		}
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
			"	in1 int, "+
			"	in2 float, "+
			"	in3 varchar(20), "+
			"	out out1 int, "+
			"	out out2 float, "+
			"	out out3 varchar(20)) "+
			"let out1 = in1; "+
			"let out2 = in2; "+
			"let out3 = in3; "+
			"end procedure;"),0);
		cstmt=con.prepareCall("{call testproc2(?,?,?,?,?,?)}");
		cstmt.setInt(1,1);
		cstmt.setDouble(2,2.5);
		cstmt.setString(3,"hello");
		cstmt.registerOutParameter(4,Types.INTEGER);
		cstmt.registerOutParameter(5,Types.DOUBLE);
		cstmt.registerOutParameter(6,Types.VARCHAR);
		assertFalse(cstmt.execute());
		assertEquals(cstmt.getInt(4),1);
		assertEquals(cstmt.getDouble(5),2.5);
		assertEquals(cstmt.getString(6),"hello");
		cstmt.close();
		stmt.executeUpdate("drop procedure testproc2");
		System.out.println();


		// stored procedures
		System.out.println("STORED PROCEDURES:");
		try {
			stmt.executeUpdate("drop procedure testproc");
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate(
			"create procedure testproc("+
			"	in1 int, "+
			"	in2 float, "+
			"	in3 varchar(20), "+
			"	out out1 int, "+
			"	out out2 float, "+
			"	out out3 varchar(20)) "+
			"let out1 = in1; "+
			"let out2 = in2; "+
			"let out3 = in3; "+
			"end procedure;"),0);
		cstmt=con.prepareCall("{call testproc(?,?,?,?,?,?)}");
		cstmt.setInt(1,1);
		cstmt.setDouble(2,2.5);
		cstmt.setString(3,"hello");
		cstmt.registerOutParameter(4,Types.INTEGER);
		cstmt.registerOutParameter(5,Types.DOUBLE);
		cstmt.registerOutParameter(6,Types.VARCHAR);
		cstmt.execute();
		assertEquals(cstmt.getInt(4),1);
		assertEquals(cstmt.getString(6),"hello");
		cstmt.close();
		stmt.executeUpdate("drop procedure testproc");
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
			assertEquals(rsmd.getColumnName(col++),"table_cat");
		}
		found=false;
		while (rs.next()) {
			String	tcat=rs.getString("table_cat");
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
		if (issqlrelay) {
			assertEquals(rsmd.getColumnName(col++),"TABLE_SCHEM");
			assertEquals(rsmd.getColumnName(col++),"TABLE_CATALOG");
		} else {
			assertEquals(rsmd.getColumnName(col++),"table_schem");
			assertEquals(rsmd.getColumnName(col++),"table_catalog");
		}
		found=false;
		while (rs.next()) {
			String	tschem=rs.getString("table_schem");
			if (tschem!=null && tschem.equals("informix")) {
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
		if (issqlrelay) {
			assertEquals(rsmd.getColumnName(col++),"TABLE_TYPE");
		} else {
			assertEquals(rsmd.getColumnName(col++),"table_type");
		}
		assertTrue(rs.next());
		assertEquals(rs.getString("table_type"),"SYSTEM TABLE");
		assertTrue(rs.next());
		assertEquals(rs.getString("table_type"),"TABLE");
		assertTrue(rs.next());
		assertEquals(rs.getString("table_type"),"VIEW");
		rs.close();
		System.out.println();


		// table list
		System.out.println("TABLE LIST:");
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
			"	col1 integer, "+
			"	col2 integer)");
		stmt.executeUpdate(
			"create table testtable2 ("+
			"	col1 integer, "+
			"	col2 integer)");
		stmt.executeUpdate(
			"create table testtable3 ("+
			"	col1 integer, "+
			"	col2 integer)");
		stmt.executeUpdate(
			"create table testtable4 ("+
			"	col1 integer, "+
			"	col2 integer)");
		rs=md.getTables(null,null,"%",
				new String[] {"TABLE"});
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),10);
		col=1;
		if (issqlrelay) {
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
			assertEquals(rsmd.getColumnName(col++),
						"REF_GENERATION");
		} else {
			assertEquals(rsmd.getColumnName(col++),"table_cat");
			assertEquals(rsmd.getColumnName(col++),"table_schem");
			assertEquals(rsmd.getColumnName(col++),"table_name");
			assertEquals(rsmd.getColumnName(col++),"table_type");
			assertEquals(rsmd.getColumnName(col++),"remarks");
			assertEquals(rsmd.getColumnName(col++),"type_cat");
			assertEquals(rsmd.getColumnName(col++),"type_schem");
			assertEquals(rsmd.getColumnName(col++),"type_name");
			assertEquals(rsmd.getColumnName(col++),
						"self_referencing_col_name");
			assertEquals(rsmd.getColumnName(col++),
						"ref_generation");
		}
		counter=0;
		while (rs.next()) {
			String name=rs.getString("table_name");
			if (name.equals("testtable1") ||
					name.equals("testtable2") ||
					name.equals("testtable3") ||
					name.equals("testtable4")) {
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


		// type info list
		System.out.println("TYPE INFO LIST:");
		rs=md.getTypeInfo();
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),18);
		col=1;
		if (issqlrelay) {
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
			assertEquals(rsmd.getColumnName(col++),
						"type_name");
			assertEquals(rsmd.getColumnName(col++),
						"data_type");
			assertEquals(rsmd.getColumnName(col++),
						"precision");
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
						"fixed_prec_scale");
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
		}
		rs.close();
		System.out.println();


		// column list
		System.out.println("COLUMN LIST:");
		stmt=con.createStatement();
		try {
			stmt.executeUpdate("drop table testtable");
		} catch (Exception ex) {
		}
		stmt.executeUpdate(
			"create table testtable ("+
			"	testboolean boolean, "+
			"	testsmallint smallint, "+
			"	testint integer, "+
			"	testbigint bigint, "+
			"	testint8 int8, "+
			"	testdecimal decimal(10,2), "+
			"	testmoney money, "+
			"	testsmallfloat smallfloat, "+
			"	testfloat float, "+
			"	testchar char(40), "+
			"	testnchar nchar(40), "+
			"	testvarchar varchar(40), "+
			"	testnvarchar nvarchar(40), "+
			"	testlvarchar lvarchar(40), "+
			"	testdate date, "+
			"	testdatetime datetime year to second, "+
			"	testtext text, "+
			"	testbyte byte, "+
			"	testurl varchar(60))");
		rs=md.getColumns(null,null,"testtable","%");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertTrue(rsmd.getColumnCount()>=18);
		col=1;
		if (issqlrelay) {
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
						"TYPE_NAME");
			assertEquals(rsmd.getColumnName(col++),
						"COLUMN_SIZE");
			assertEquals(rsmd.getColumnName(col++),
						"BUFFER_LENGTH");
			assertEquals(rsmd.getColumnName(col++),
						"DECIMAL_DIGITS");
			assertEquals(rsmd.getColumnName(col++),
						"NUM_PREC_RADIX");
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
		} else {
			assertEquals(rsmd.getColumnName(col++),
						"table_cat");
			assertEquals(rsmd.getColumnName(col++),
						"table_schem");
			assertEquals(rsmd.getColumnName(col++),
						"table_name");
			assertEquals(rsmd.getColumnName(col++),
						"column_name");
			assertEquals(rsmd.getColumnName(col++),
						"data_type");
			assertEquals(rsmd.getColumnName(col++),
						"type_name");
			assertEquals(rsmd.getColumnName(col++),
						"column_size");
			assertEquals(rsmd.getColumnName(col++),
						"buffer_length");
			assertEquals(rsmd.getColumnName(col++),
						"decimal_digits");
			assertEquals(rsmd.getColumnName(col++),
						"num_prec_radix");
			assertEquals(rsmd.getColumnName(col++),
						"nullable");
			assertEquals(rsmd.getColumnName(col++),
						"remarks");
			assertEquals(rsmd.getColumnName(col++),
						"column_def");
			assertEquals(rsmd.getColumnName(col++),
						"sql_data_type");
			assertEquals(rsmd.getColumnName(col++),
						"sql_datetime_sub");
			assertEquals(rsmd.getColumnName(col++),
						"char_octet_length");
			assertEquals(rsmd.getColumnName(col++),
						"ordinal_position");
			assertEquals(rsmd.getColumnName(col++),
						"is_nullable");
		}
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testboolean");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("BOOLEAN"));
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testsmallint");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("SMALLINT"));
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testint");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("INTEGER"));
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testbigint");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("BIGINT"));
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testint8");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("INT8"));
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testdecimal");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("DECIMAL"));
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testmoney");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("MONEY"));
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testsmallfloat");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("SMALLFLOAT"));
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testfloat");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("FLOAT"));
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testchar");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("CHAR"));
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testnchar");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("NCHAR"));
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testvarchar");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("VARCHAR"));
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testnvarchar");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("NVARCHAR"));
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testlvarchar");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("LVARCHAR"));
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testdate");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("DATE"));
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testdatetime");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("DATETIME"));
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testtext");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("TEXT"));
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testbyte");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("BYTE"));
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"testurl");
		assertTrue(rs.getString("type_name").
					equalsIgnoreCase("VARCHAR"));
		rs.close();
		stmt.executeUpdate("drop table testtable");
		System.out.println();


		// primary key list
		System.out.println("PRIMARY KEY LIST:");
		try {
			stmt.executeUpdate("drop table testtable");
		} catch (Exception ex) {
		}
		stmt.executeUpdate(
			"create table testtable ("+
			"	col1 integer primary key, "+
			"	col2 integer)");
		rs=md.getPrimaryKeys(null,null,"testtable");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),6);
		col=1;
		if (issqlrelay) {
			assertEquals(rsmd.getColumnName(col++),"TABLE_CAT");
			assertEquals(rsmd.getColumnName(col++),"TABLE_SCHEM");
			assertEquals(rsmd.getColumnName(col++),"TABLE_NAME");
			assertEquals(rsmd.getColumnName(col++),"COLUMN_NAME");
			assertEquals(rsmd.getColumnName(col++),"KEY_SEQ");
			assertEquals(rsmd.getColumnName(col++),"PK_NAME");
		} else {
			assertEquals(rsmd.getColumnName(col++),"table_cat");
			assertEquals(rsmd.getColumnName(col++),"table_schem");
			assertEquals(rsmd.getColumnName(col++),"table_name");
			assertEquals(rsmd.getColumnName(col++),"column_name");
			assertEquals(rsmd.getColumnName(col++),"key_seq");
			assertEquals(rsmd.getColumnName(col++),"pk_name");
		}
		assertTrue(rs.next());
		assertTrue(rs.getString("table_name").
					equalsIgnoreCase("testtable"));
		assertTrue(rs.getString("column_name").
					equalsIgnoreCase("col1"));
		assertEquals(rs.getString("key_seq"),"1");
		assertTrue(rs.getString("pk_name")!=null &&
				rs.getString("pk_name").length()>0);
		assertFalse(rs.next());
		rs.close();
		stmt.executeUpdate("drop table testtable");
		System.out.println();


		// key and index list
		System.out.println("KEY AND INDEX LIST:");
		try {
			stmt.executeUpdate("drop table testtable");
		} catch (Exception ex) {
		}
		stmt.executeUpdate(
			"create table testtable ("+
			"	col1 integer primary key, "+
			"	col2 integer)");
		rs=md.getIndexInfo(null,null,"testtable",false,true);
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),13);
		col=1;
		if (issqlrelay) {
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
		} else {
			assertEquals(rsmd.getColumnName(col++),
						"table_cat");
			assertEquals(rsmd.getColumnName(col++),
						"table_schem");
			assertEquals(rsmd.getColumnName(col++),
						"table_name");
			assertEquals(rsmd.getColumnName(col++),
						"non_unique");
			assertEquals(rsmd.getColumnName(col++),
						"index_qualifier");
			assertEquals(rsmd.getColumnName(col++),
						"index_name");
			assertEquals(rsmd.getColumnName(col++),
						"type");
			assertEquals(rsmd.getColumnName(col++),
						"ordinal_position");
			assertEquals(rsmd.getColumnName(col++),
						"column_name");
			assertEquals(rsmd.getColumnName(col++),
						"asc_or_desc");
			assertEquals(rsmd.getColumnName(col++),
						"cardinality");
			assertEquals(rsmd.getColumnName(col++),
						"pages");
			assertEquals(rsmd.getColumnName(col++),
						"filter_condition");
		}
		assertTrue(rs.next());
		assertEquals(rs.getString("table_name"),"testtable");
		assertEquals(rs.getString("non_unique"),"0");
		assertEquals(rs.getString("ordinal_position"),"1");
		assertEquals(rs.getString("column_name"),"col1");
		if (issqlrelay) {
			assertEquals(rs.getString("asc_or_desc"),"A");
		} else {
			// informix jdbc returns null for this
			assertEquals(rs.getString("asc_or_desc"),null);
		}
		assertEquals(rs.getString("type"),"3");
		assertTrue(rs.getString("index_name")!=null &&
				rs.getString("index_name").length()>0);
		assertFalse(rs.next());
		rs.close();
		stmt.executeUpdate("drop table testtable");
		System.out.println();


		// procedure list
		System.out.println("PROCEDURE LIST:");
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
			"	in1 integer, "+
			"	in2 char(20), "+
			"	in3 varchar(20), "+
			"	in4 date) "+
			"define x integer; "+
			"let x = 1; "+
			"end procedure;");
		stmt.executeUpdate(
			"create procedure testproc2("+
			"	in1 integer, "+
			"	in2 char(20), "+
			"	in3 varchar(20), "+
			"	in4 date) "+
			"define x integer; "+
			"let x = 1; "+
			"end procedure;");
		stmt.executeUpdate(
			"create procedure testproc3("+
			"	in1 integer, "+
			"	in2 char(20), "+
			"	in3 varchar(20), "+
			"	in4 date) "+
			"define x integer; "+
			"let x = 1; "+
			"end procedure;");
		stmt.executeUpdate(
			"create procedure testproc4("+
			"	in1 integer, "+
			"	in2 char(20), "+
			"	in3 varchar(20), "+
			"	in4 date) "+
			"define x integer; "+
			"let x = 1; "+
			"end procedure;");
		rs=md.getProcedures(null,null,"%");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		if (issqlrelay) {
			assertEquals(rsmd.getColumnCount(),8);
		} else {
			assertTrue(rsmd.getColumnCount()>=8);
		}
		col=1;
		if (issqlrelay) {
			assertEquals(rsmd.getColumnName(col++),
						"PROCEDURE_CAT");
			assertEquals(rsmd.getColumnName(col++),
						"PROCEDURE_SCHEM");
			assertEquals(rsmd.getColumnName(col++),
						"PROCEDURE_NAME");
			assertEquals(rsmd.getColumnName(col++),
						"NUM_INPUT_PARAMS");
			assertEquals(rsmd.getColumnName(col++),
						"NUM_OUTPUT_PARAMS");
			assertEquals(rsmd.getColumnName(col++),
						"NUM_RESULT_SETS");
			assertEquals(rsmd.getColumnName(col++),
						"REMARKS");
			assertEquals(rsmd.getColumnName(col++),
						"PROCEDURE_TYPE");
		} else {
			assertEquals(rsmd.getColumnName(col++),
						"procedure_cat");
			assertEquals(rsmd.getColumnName(col++),
						"procedure_schem");
			assertEquals(rsmd.getColumnName(col++),
						"procedure_name");
			col+=3;
			assertEquals(rsmd.getColumnName(col++),
						"remarks");
			assertEquals(rsmd.getColumnName(col++),
						"procedure_type");
		}
		counter=0;
		while (rs.next()) {
			String name=rs.getString("procedure_name");
			if (name.equalsIgnoreCase("testproc1") ||
					name.equalsIgnoreCase("testproc2") ||
					name.equalsIgnoreCase("testproc3") ||
					name.equalsIgnoreCase("testproc4")) {
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
		if (issqlrelay) {
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
		} else {
			assertEquals(rsmd.getColumnName(col++),
						"procedure_cat");
			assertEquals(rsmd.getColumnName(col++),
						"procedure_schem");
			assertEquals(rsmd.getColumnName(col++),
						"procedure_name");
			assertEquals(rsmd.getColumnName(col++),
						"column_name");
			assertEquals(rsmd.getColumnName(col++),
						"column_type");
			assertEquals(rsmd.getColumnName(col++),
						"data_type");
			assertEquals(rsmd.getColumnName(col++),
						"type_name");
			assertEquals(rsmd.getColumnName(col++),
						"precision");
			assertEquals(rsmd.getColumnName(col++),
						"length");
			assertEquals(rsmd.getColumnName(col++),
						"scale");
			assertEquals(rsmd.getColumnName(col++),
						"radix");
			assertEquals(rsmd.getColumnName(col++),
						"nullable");
			assertEquals(rsmd.getColumnName(col++),
						"remarks");
			assertEquals(rsmd.getColumnName(col++),
						"column_def");
			assertEquals(rsmd.getColumnName(col++),
						"sql_data_type");
			assertEquals(rsmd.getColumnName(col++),
						"sql_datetime_sub");
			assertEquals(rsmd.getColumnName(col++),
						"char_octet_length");
			assertEquals(rsmd.getColumnName(col++),
						"ordinal_position");
			assertEquals(rsmd.getColumnName(col++),
						"is_nullable");
			assertEquals(rsmd.getColumnName(col++),
						"specific_name");
		}
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"in1");
		assertEquals(rs.getString("type_name"),"integer");
		if (issqlrelay) {
			assertEquals(rs.getString("ordinal_position"),"1");
		} else {
			// the native informix jdbc driver returns
			// 0-based ordinal positions
			assertEquals(rs.getString("ordinal_position"),"0");
		}
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"in2");
		assertEquals(rs.getString("type_name"),"char");
		if (issqlrelay) {
			assertEquals(rs.getString("ordinal_position"),"2");
		} else {
			assertEquals(rs.getString("ordinal_position"),"1");
		}
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"in3");
		assertEquals(rs.getString("type_name"),"varchar");
		if (issqlrelay) {
			assertEquals(rs.getString("ordinal_position"),"3");
		} else {
			assertEquals(rs.getString("ordinal_position"),"2");
		}
		assertTrue(rs.next());
		assertEquals(rs.getString("column_name"),"in4");
		assertEquals(rs.getString("type_name"),"date");
		if (issqlrelay) {
			assertEquals(rs.getString("ordinal_position"),"4");
		} else {
			assertEquals(rs.getString("ordinal_position"),"3");
		}
		rs.close();
		stmt.executeUpdate("drop procedure testproc1");
		stmt.executeUpdate("drop procedure testproc2");
		stmt.executeUpdate("drop procedure testproc3");
		stmt.executeUpdate("drop procedure testproc4");
		System.out.println();


		// function list
		// neither informix, nor sqlrelay support this
		// informix throws not-supported
		// sqlrelay just returns an empty result set
		System.out.println("FUNCTION LIST:");
		if (!issqlrelay) {
			try {
				rs=md.getFunctions(null,null,"%");
			} catch (Exception ex) {
				assertTrue(true);
			}
			System.out.println();
		}


		// function parameter list
		// neither informix, nor sqlrelay support this
		// informix throws not-supported
		// sqlrelay just returns an empty result set
		System.out.println("FUNCTION PARAMETER LIST:");
		if (!issqlrelay) {
			try {
				rs=md.getFunctionColumns(null,null,"%","%");
				assertFalse(true);
			} catch (Exception ex) {
				assertTrue(true);
			}
			System.out.println();
		}


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
