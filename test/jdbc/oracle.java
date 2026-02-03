// Copyright (c) David Muse
// See the file COPYING for more information.

import java.sql.*;
import com.firstworks.sqlrelay.*;
import com.firstworks.sql.*;
import java.util.Properties;
import java.util.concurrent.Executors;
import java.util.concurrent.Executor;
import java.nio.charset.StandardCharsets;
import java.net.InetAddress;

class oracle extends sqlrtest {

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


		// connection
		System.out.println("CONNECTION:");

		// getConnection
		System.out.println("getConnection");
		Class.forName(driver);
		Connection	con=DriverManager.getConnection(
						url,user,password);
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
			assertTrue((sqlrcon!=null));
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
		// FIXME: sqlrelay currently returns the schema, when run
		// against an oracle backend, rather than null, #7914
		if (!issqlrelay) {
			System.out.println("catalog");
			con.setCatalog(user);
			assertEquals(con.getCatalog(),null);
			System.out.println();
		}

		// schema
		// FIXME: with sqlrelay, somehow this causes oracle to throw:
		// ORA-01031: insufficient privileges
		if (!issqlrelay) {
			System.out.println("schema");
			con.setSchema(user.toUpperCase());
			assertEquals(con.getSchema(),user.toUpperCase());
			System.out.println();
		}

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

		// FIXME: type map

		// warnings
		System.out.println("warnings");
		assertTrue(con.getWarnings()==null);
		con.clearWarnings();
		System.out.println();

		System.out.println();


		// database meta data
		System.out.println("DATABASE META DATA:");

		// getMetaData
		System.out.println("getMetaData");
		DatabaseMetaData	md=con.getMetaData();
		assertTrue((md!=null));
		System.out.println();

		System.out.println("getConnection");
		assertEquals(md.getConnection(),con);
		System.out.println();

		if (issqlrelay) {
			System.out.println("unwrap");
			assertEquals(md.isWrapperFor(SQLRConnection.class),1);
			assertEquals((md.unwrap(SQLRConnection.class)!=null),1);
			System.out.println();
		}

		// database attributes
		boolean		boolval;
		int		intval;
		String		stringval;

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
		System.out.println("deletesAreDetected");
		boolval=md.deletesAreDetected(ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertFalse(boolval);
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
		if (issqlrelay) {
			assertEquals(stringval,"2.1.1");
		} else {
			assertEquals(stringval,"23.26.0.0.0");
		}
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
		assertEquals(intval,4);
		System.out.println();

		// getJDBCMinorVersion
		System.out.println("getJDBCMinorVersion");
		intval=md.getJDBCMinorVersion();
		System.out.println("  "+intval);
		assertEquals(intval,2);
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
		assertEquals(intval,0);
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
		assertEquals(intval,1);
		System.out.println();

		// getRowIdLifetime
		System.out.println("getRowIdLifetime");
		RowIdLifetime	rowidlifetimeval=md.getRowIdLifetime();
		System.out.println("  "+rowidlifetimeval);
		if (issqlrelay) {
			assertEquals(rowidlifetimeval.name(),
						"ROWID_UNSUPPORTED");
		} else {
			assertEquals(rowidlifetimeval.name(),
						"ROWID_VALID_FOREVER");
		}
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
		if (issqlrelay) {
			assertEquals(stringval,"jdbc:sqlrelay://testuser:testpassword@localhost:9000");
		} else {
			assertEquals(stringval,"jdbc:oracle:thin:@oracle:1521:ora1");
		}
		System.out.println();

		// getUserName
		System.out.println("getUserName");
		stringval=md.getUserName();
		System.out.println("  "+stringval);
		if (issqlrelay) {
			assertEquals(stringval,"testuser");
		} else {
			assertEquals(stringval,"FEDORA40X64");
		}
		System.out.println();

		// insertsAreDetected
		System.out.println("insertsAreDetected");
		boolval=md.insertsAreDetected(ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertFalse(boolval);
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
		System.out.println("othersDeletesAreVisible");
		boolval=md.othersDeletesAreVisible(ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// othersInsertsAreVisible
		System.out.println("othersInsertsAreVisible");
		boolval=md.othersInsertsAreVisible(ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// othersUpdatesAreVisible
		System.out.println("othersUpdatesAreVisible");
		boolval=md.othersUpdatesAreVisible(ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// ownDeletesAreVisible
		System.out.println("ownDeletesAreVisible");
		boolval=md.ownDeletesAreVisible(ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// ownInsertsAreVisible
		System.out.println("ownInsertsAreVisible");
		boolval=md.ownInsertsAreVisible(ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertFalse(boolval);
		System.out.println();

		// ownUpdatesAreVisible
		System.out.println("ownUpdatesAreVisible");
		boolval=md.ownUpdatesAreVisible(ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertFalse(boolval);
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
		System.out.println("supportsDataDefinitionAndDataManipulationTransactions");
		boolval=md.supportsDataDefinitionAndDataManipulationTransactions();
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
		System.out.println("supportsResultSetType (forward only)");
		boolval=md.supportsResultSetType(ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("supportsResultSetType (scroll insensitive)");
		boolval=md.supportsResultSetType(ResultSet.TYPE_SCROLL_INSENSITIVE);
		System.out.println("  "+boolval);
		assertTrue(boolval);
		System.out.println();

		System.out.println("supportsResultSetType (scroll sensitive)");
		boolval=md.supportsResultSetType(ResultSet.TYPE_SCROLL_SENSITIVE);
		System.out.println("  "+boolval);
		if (issqlrelay) {
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
		if (issqlrelay) {
			assertFalse(boolval);
		} else {
			assertTrue(boolval);
		}
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
		System.out.println("supportsTransactionIsolationLevel");
		boolval=md.supportsTransactionIsolationLevel(
					Connection.TRANSACTION_READ_COMMITTED);
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
		System.out.println("updatesAreDetected");
		boolval=md.updatesAreDetected(ResultSet.TYPE_FORWARD_ONLY);
		System.out.println("  "+boolval);
		assertFalse(boolval);
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

		// catalogs
		System.out.println("catalogs");
		ResultSet	rs=md.getCatalogs();
		assertTrue((rs!=null));
		ResultSetMetaData	rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),1);
		int	col=1;
		assertEquals(rsmd.getColumnName(col++),"TABLE_CAT");
		while (rs.next()) {
			System.out.println(rs.getString("TABLE_CAT"));
		}
		//System.out.println();
		//printColumns(rsmd);
		//printResultSet(rs);
		rs.close();
		System.out.println();

		// schemas
		System.out.println("schemas");
		rs=md.getSchemas("%","%");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),2);
		col=1;
		assertEquals(rsmd.getColumnName(col++),"TABLE_SCHEM");
		assertEquals(rsmd.getColumnName(col++),"TABLE_CATALOG");
		//System.out.println();
		//printColumns(rsmd);
		//printResultSet(rs);
		rs.close();
		System.out.println();

		// table types
		System.out.println("table types");
		rs=md.getTableTypes();
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		assertEquals(rsmd.getColumnCount(),1);
		col=1;
		assertEquals(rsmd.getColumnName(col++),"TABLE_TYPE");
		//System.out.println();
		//printColumns(rsmd);
		//printResultSet(rs);
		rs.close();
		System.out.println();

		// tables
		System.out.println("tables");
// slow, disabled for now
if (false) {
		rs=md.getTables("%","%","%",
			new String[] {"SYNONYM","TABLE","VIEW"});
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		if (issqlrelay) {
                	assertEquals(rsmd.getColumnCount(),10);
		} else {
			// oracle jdbc (at least v8) returns 5 columns
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
		//System.out.println();
		//printColumns(rsmd);
		//printResultSet(rs);
		rs.close();
}
		System.out.println();

		// type info
		System.out.println("type info");
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

		// procedures
                System.out.println("procedures");
// slow, disabled for now
if (false) {
                rs=md.getProcedures("%","%","%");
                assertTrue((rs!=null));
                rsmd=rs.getMetaData();
                assertTrue((rsmd!=null));
		col=1;
		if (issqlrelay) {
                	assertEquals(rsmd.getColumnCount(),8);
		} else {
			// oracle jdbc (at least v8) returns 9 columns
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
			// oracle jdbc (at least v8) returns
			// NULL for these column names
                	assertEquals(rsmd.getColumnName(col++),"NULL");
                	assertEquals(rsmd.getColumnName(col++),"NULL");
                	assertEquals(rsmd.getColumnName(col++),"NULL");
		}
                assertEquals(rsmd.getColumnName(col++),"REMARKS");
                assertEquals(rsmd.getColumnName(col++),"PROCEDURE_TYPE");
		if (!issqlrelay) {
			// oracle jdbc (at least v8) returns a 9th column
                	assertEquals(rsmd.getColumnName(col++),"SPECIFIC_NAME");
		}
                //System.out.println();
		//printColumns(rsmd);
		//printResultSet(rs);
                rs.close();
}
                System.out.println();

		// functions
                System.out.println("functions");
		// FIXME: sqlrelay doesn't support this yet
		if (!issqlrelay) {
                	rs=md.getFunctions("%","%","%");
                	assertTrue((rs!=null));
                	rsmd=rs.getMetaData();
                	assertTrue((rsmd!=null));
			col=1;
			if (issqlrelay) {
				assertEquals(rsmd.getColumnCount(),8);
			} else {
				// oracle jdbc (at least v8) returns 6 columns
				assertEquals(rsmd.getColumnCount(),6);
			}
                	assertEquals(rsmd.getColumnName(col++),
							"FUNCTION_CAT");
                	assertEquals(rsmd.getColumnName(col++),
							"FUNCTION_SCHEM");
                	assertEquals(rsmd.getColumnName(col++),
							"FUNCTION_NAME");
			// oracle jdbc (at least v8) doesn't
			// return these columns at all
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
			// oracle jdbc (at least v8) returns this extra column
			if (!issqlrelay) {
                		assertEquals(rsmd.getColumnName(col++),
							"SPECIFIC_NAME");
			}
                	//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
                	rs.close();
		}
                System.out.println();

		// UDTs
                System.out.println("UDTs");
		// FIXME: sqlrelay doesn't support this yet
		// oracle jdbc (at least v8) throws:
		// ORA-08177: can't serialize access for this transaction
		if (false) {
                	rs=md.getUDTs("%","%","%",null);
                	assertTrue((rs!=null));
                	rsmd=rs.getMetaData();
                	assertTrue((rsmd!=null));
			col=1;
			if (issqlrelay) {
                		assertEquals(rsmd.getColumnCount(),7);
			} else {
				// oracle jdbc (at least v8) returns 6 columns
                		assertEquals(rsmd.getColumnCount(),6);
			}
                	assertEquals(rsmd.getColumnName(col++),"TYPE_CAT");
                	assertEquals(rsmd.getColumnName(col++),"TYPE_SCHEM");
                	assertEquals(rsmd.getColumnName(col++),"TYPE_NAME");
                	assertEquals(rsmd.getColumnName(col++),"CLASS_NAME");
                	assertEquals(rsmd.getColumnName(col++),"DATA_TYPE");
                	assertEquals(rsmd.getColumnName(col++),"REMARKS");
			if (issqlrelay) {
				// oracle jdbc (at least v8) doesn't
				// return this column at all
                		assertEquals(rsmd.getColumnName(col++),
								"BASE_TYPE");
			}
                	//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
                	rs.close();
		}
                System.out.println();

		// attributes
		// neither oracle, nor sqlrelay support this
		System.out.println("attributes");
		try {
			rs=md.getAttributes("%","%","%","%");
			assertFalse(true);
		} catch (Exception ex) {
			assertTrue(true);
		}
		System.out.println();

		// best row identifier
		System.out.println("best row identifier");
		// FIXME: sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getBestRowIdentifier("%","%","%",
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
		}
		System.out.println();

		// client info properties
		System.out.println("client info properties");
		// FIXME: sqlrelay doesn't support this yet
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
		}
		System.out.println();

		// column privileges
		System.out.println("column privileges");
		// FIXME: sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getColumnPrivileges("%","%","%","%");
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
		}
		System.out.println();

		// columns
		System.out.println("columns");
		rs=md.getColumns("%","%","%","%");
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
		//System.out.println();
		//printColumns(rsmd);
		//printResultSet(rs);
		rs.close();
		System.out.println();

		// cross reference
		System.out.println("cross reference");
		// FIXME: sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getCrossReference("%","%","%","%","%","%");
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
		}
		System.out.println();

		// exported keys
		System.out.println("exported keys");
		// FIXME: sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getExportedKeys("%","%","%");
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
		}
		System.out.println();

		// function columns
		System.out.println("function columns");
// slow, disabled for now
if (false) {
		// FIXME: sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getFunctionColumns("%","%","%","%");
			assertTrue((rs!=null));
			rsmd=rs.getMetaData();
			assertTrue((rsmd!=null));
			col=1;
			if (issqlrelay) {
				assertEquals(rsmd.getColumnCount(),17);
			} else {
				// oracle jdbc (at least v8) returns 23 columns
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
				// oracle jdbc (at least v8)
				// returns these columns too
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
				// oracle jdbc (at least v8)
				// returns these columns too
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
		}
}
		System.out.println();

		// imported keys
		System.out.println("imported keys");
		// FIXME: sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getImportedKeys("%","%","%");
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
		}
		System.out.println();

		// index info
		System.out.println("index info");
		// FIXME: sqlrelay doesn't support this yet
		// oracle jdbc (at least v8) throws:
		// ORA-17068: Invalid arguments in call
		if (false) {
			rs=md.getIndexInfo("%","%","%",false,true);
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
			//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
			rs.close();
		}
		System.out.println();

		// primary keys
		System.out.println("primary keys");
		// FIXME: sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getPrimaryKeys("%","%","%");
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
			//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
			rs.close();
		}
		System.out.println();

		// procedure columns
		System.out.println("procedure columns");
		// FIXME: sqlrelay doesn't support this yet
		// oracle jdbc (at least v8) throws:
		// ORA-00904: "ARG"."TYPE_OBJECT_TYPE": invalid identifier
		if (false) {
			rs=md.getProcedureColumns("%","%","%","%");
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
			//System.out.println();
			//printColumns(rsmd);
			//printResultSet(rs);
			rs.close();
		}
		System.out.println();

		// pseudo columns
		System.out.println("pseudo columns");
// slow, disabled for now
if (false) {
		// FIXME: sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getPseudoColumns("%","%","%","%");
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
		}
}
		System.out.println();

		// super tables
		// neither oracle, nor sqlrelay support this
		System.out.println("super tables");
		try {
			rs=md.getSuperTables("%","%","%");
			assertFalse(true);
		} catch (Exception ex) {
			assertTrue(true);
		}
		System.out.println();

		// super types
		// neither oracle, nor sqlrelay support this
		System.out.println("super types");
		try {
			rs=md.getSuperTypes("%","%","%");
			assertFalse(true);
		} catch (Exception ex) {
			assertTrue(true);
		}
		System.out.println();

		// table privileges
		System.out.println("table privileges");
// slow, disabled for now
if (false) {
		// FIXME: sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getTablePrivileges("%","%","%");
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
		}
}
		System.out.println();

		// version columns
		System.out.println("version columns");
		// FIXME: sqlrelay doesn't support this yet
		if (!issqlrelay) {
			rs=md.getVersionColumns("%","%","%");
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
		}
		System.out.println();


		// statement
		System.out.println("STATEMENT:");

		// createStatement
		System.out.println("create statement");
		Statement	stmt=con.createStatement();
		assertTrue((stmt!=null));
		stmt.close();
                System.out.println();

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
		System.out.println();


		// drop existing table
		stmt=con.createStatement();
		try {
			stmt.executeUpdate("drop table testtable");
		} catch (Exception ex) {
		}


		// create temptable
		System.out.println("CREATE TEMPTABLE:");
		assertEquals(stmt.executeUpdate("create table testtable (testnumber number, testchar char(40), testvarchar varchar2(40), testdate date, testlong long, testclob clob, testblob blob)"),0);
		System.out.println();


		// insert
		System.out.println("INSERT:");
		assertEquals(stmt.executeUpdate("insert into testtable values (1,'testchar1','testvarchar1','01-JAN-2001','testlong1','testclob1',empty_blob())"),1);
		stmt.close();
		System.out.println();


		// bind by position
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
		assertEquals(pstmt.executeUpdate(),1);
		pstmt.clearParameters();
		pstmt.setInt(1,3);
		pstmt.setString(2,"testchar3");
		pstmt.setString(3,"testvarchar3");
		pstmt.setDate(4,new java.sql.Date(2003,1,1));
		pstmt.setString(5,"testlong3");
		pstmt.setString(6,"testclob3");
		pstmt.setBytes(7,(new String("testblob3")).
				getBytes(StandardCharsets.UTF_8));
		assertEquals(pstmt.executeUpdate(),1);
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
		assertEquals(pstmt.executeUpdate(),1);
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
		assertEquals(pstmt.executeUpdate(),1);
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
		assertEquals(pstmt.executeUpdate(),1);
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
		assertEquals(pstmt.executeUpdate(),1);
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
		assertEquals(pstmt.executeUpdate(),1);
		pstmt.close();
		System.out.println();


		// FIXME: output binds


		// select
		System.out.println("SELECT:");
		stmt=con.createStatement();
		assertTrue((stmt!=null));
		rs=stmt.executeQuery("select * from testtable order by testnumber");
		assertTrue((rs!=null));
		rsmd=rs.getMetaData();
		assertTrue((rsmd!=null));
		System.out.println();


		// column count
		System.out.println("COLUMN COUNT:");
		assertEquals(rsmd.getColumnCount(),7);
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
		System.out.println();


		// fields by index
		System.out.println("FIELDS BY INDEX:");
		rs.next();
		assertEquals(rs.getString(1),"1");
		assertEquals(rs.getString(2),"testchar1                               ");
		assertEquals(rs.getString(3),"testvarchar1");
		if (issqlrelay) {
			assertEquals(rs.getString(4),"01-JAN-01");
		} else {
			// oracle jdbc returns this format, independent
			// of how you set NLS_DATE_FORMAT
			assertEquals(rs.getString(4),"2001-01-01 00:00:00");
		}
		assertEquals(rs.getString(5),"testlong1");
		assertEquals(rs.getString(6),"testclob1");
		// oracle jdbc can't convert a blob directly to a string
		Blob	bl=rs.getBlob(7);
		byte[]	b=null;
		if (issqlrelay) {
			// SerialBlob doesn't like a length of 0.
			//
			// Oracle jdbc returns its own Blob implementation
			// that's tolerant to this.
			//
			// For now we're not implementing our own Blob.
			if (bl.length()==0) {
				b=new byte[0];
			} else {
				b=bl.getBytes(1,(int)bl.length());
			}
		} else {
			b=bl.getBytes(1,(int)bl.length());
		}
		assertEquals(new String(b,"UTF-8"),"");
		System.out.println();
		for (int i=0; i<7; i++) {
			rs.next();
		}
		assertEquals(rs.getString(1),"8");
		assertEquals(rs.getString(2),"testchar8                               ");
		assertEquals(rs.getString(3),"testvarchar8");
		if (issqlrelay) {
			assertEquals(rs.getString(4),"01-JAN-08");
		} else {
			// oracle jdbc returns this format, independent
			// of how you set NLS_DATE_FORMAT
// FIXME: some weird bug causes the date to come back as 3908-01-01 00:00:00
if (false) {
			assertEquals(rs.getString(4),"2008-01-01 00:00:00");
}
		}
		assertEquals(rs.getString(5),"testlong8");
		assertEquals(rs.getString(6),"testclob8");
		// oracle jdbc can't getString() on a blob
		bl=rs.getBlob(7);
		b=bl.getBytes(1,(int)bl.length());
		assertEquals(new String(b,"UTF-8"),"testblob8");
		System.out.println();


		// fields by name
		System.out.println("FIELDS BY NAME:");
		assertTrue((stmt!=null));
		rs=stmt.executeQuery("select * from testtable order by testnumber");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString("TESTNUMBER"),"1");
		assertEquals(rs.getString("TESTCHAR"),"testchar1                               ");
		assertEquals(rs.getString("TESTVARCHAR"),"testvarchar1");
		if (issqlrelay) {
			assertEquals(rs.getString("TESTDATE"),"01-JAN-01");
		} else {
			// oracle jdbc returns this format, independent
			// of how you set NLS_DATE_FORMAT
			assertEquals(rs.getString("TESTDATE"),"2001-01-01 00:00:00");
		}
		assertEquals(rs.getString("TESTLONG"),"testlong1");
		assertEquals(rs.getString("TESTCLOB"),"testclob1");
		// oracle jdbc can't convert a blob directly to a string
		bl=rs.getBlob("TESTBLOB");
		b=null;
		if (issqlrelay) {
			// SerialBlob doesn't like a length of 0.
			//
			// Oracle jdbc returns its own Blob implementation
			// that's tolerant to this.
			//
			// For now we're not implementing our own Blob.
			if (bl.length()==0) {
				b=new byte[0];
			} else {
				b=bl.getBytes(1,(int)bl.length());
			}
		} else {
			b=bl.getBytes(1,(int)bl.length());
		}
		assertEquals(new String(b,"UTF-8"),"");
		System.out.println();
		for (int i=0; i<7; i++) {
			rs.next();
		}
		assertEquals(rs.getString("TESTNUMBER"),"8");
		assertEquals(rs.getString("TESTCHAR"),"testchar8                               ");
		assertEquals(rs.getString("TESTVARCHAR"),"testvarchar8");
		if (issqlrelay) {
			assertEquals(rs.getString("TESTDATE"),"01-JAN-08");
		} else {
			// oracle jdbc returns this format, independent
			// of how you set NLS_DATE_FORMAT
// FIXME: some weird bug causes the date to come back as 3908-01-01 00:00:00
if (false) {
			assertEquals(rs.getString("TESTDATE"),"2008-01-01 00:00:00");
}
		}
		assertEquals(rs.getString("TESTLONG"),"testlong8");
		assertEquals(rs.getString("TESTCLOB"),"testclob8");
		// oracle jdbc can't getString() on a blob
		bl=rs.getBlob("TESTBLOB");
		b=bl.getBytes(1,(int)bl.length());
		assertEquals(new String(b,"UTF-8"),"testblob8");
		System.out.println();


		// row count
		System.out.println("ROW COUNT:");
		assertEquals(rs.getRow(),8);
		rs.close();
		System.out.println();


		// FIXME: output bind

		// FIXME: nulls as nulls

		// FIXME: result set buffer size


		// commit and rollback
		System.out.println("COMMIT AND ROLLBACK:");
		Connection	secondcon=DriverManager.getConnection(
							url,user,password);
		assertTrue((secondcon!=null));
		Statement	secondstmt=secondcon.createStatement();
		assertTrue((secondstmt!=null));
		ResultSet	secondrs=secondstmt.executeQuery("select count(*) from testtable");
		assertTrue((secondrs!=null));
		secondrs.next();
		assertEquals(secondrs.getString(1),"0");
		secondrs.close();
		con.commit();
		secondrs=secondstmt.executeQuery("select count(*) from testtable");
		assertTrue((secondrs!=null));
		secondrs.next();
		assertEquals(secondrs.getString(1),"8");
		con.setAutoCommit(true);
		secondrs.close();
		assertEquals(stmt.executeUpdate("insert into testtable values (10,'testchar10','testvarchar10','01-JAN-2010','testlong10','testclob10',NULL)"),1);
		secondrs=secondstmt.executeQuery("select count(*) from testtable");
		assertTrue((secondrs!=null));
		secondrs.next();
		assertEquals(secondrs.getString(1),"9");
		secondrs.close();
		secondstmt.close();
		con.setAutoCommit(false);
		System.out.println();


		// FIXME: clob and blob outpub bind


		try {
			stmt.executeUpdate("drop table testtable1");
		} catch (Exception ex) {
		}


		// null and empty clobs and blobs
		System.out.println("NULL AND EMPTY CLOBS AND BLOBS:");
		assertEquals(stmt.executeUpdate("create table testtable1 (testclob1 clob, testclob2 clob, testblob1 blob, testblob2 blob)"),0);
		pstmt=con.prepareStatement("insert into testtable1 values (:var1,:var2,:var3,:var4)");
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
		// oracle jdbc can't getString() on a blob
		assertTrue((rs.getBlob(3)==null));
		assertTrue((rs.getBlob(4)==null));
		assertEquals(stmt.executeUpdate("drop table testtable1"),0);
		System.out.println();


		// FIXME: cursor binds


		try {
			stmt.executeUpdate("drop table testtable2");
		} catch (Exception ex) {
		}


		// long clob
		System.out.println("LONG CLOB:");
		assertEquals(stmt.executeUpdate("create table testtable2 (testclob clob)"),0);
		pstmt=con.prepareStatement("insert into testtable2 values (:clobval)");
		assertTrue((pstmt!=null));
		StringBuilder	clobval=new StringBuilder();
		// oracle jdbc struggles with more than 1024 byte clobs
		for (int i=0; i<1024; i++) {
			clobval.append('C');
		}
		String	clobstr=clobval.toString();
		pstmt.setString(1,clobstr);
		assertEquals(pstmt.executeUpdate(),1);
		rs=stmt.executeQuery("select testclob from testtable2");
		assertTrue((rs!=null));
		rs.next();
		Clob	cl=rs.getClob(1);
		assertEquals(clobstr,cl.getSubString(1,(int)cl.length()));
		rs.close();
		// FIXME: use callable statement?
		/*cur->prepareQuery("begin select testclob into :clobbindval from testtable2; end;");
		cur->defineOutputBindClob("clobbindval");
		assertTrue(cur->executeQuery());
		const char	*clobbindvar=cur->getOutputBindClob("clobbindval");
		assertEquals(cur->getOutputBindLength("clobbindval"),8*1024);
		assertEquals(clobval,clobbindvar);
		cur->sendQuery("drop table testtable2");*/
		System.out.println();


		// FIXME: long output bind

		// FIXME: negative input bind


		// drop existing table
		stmt.executeUpdate("drop table testtable");


		// temporary tables
		System.out.println("TEMPORARY TABLES:");
		String	hostname=InetAddress.getLocalHost().
					getHostName().split("\\.")[0];
		try {
			assertEquals(stmt.executeUpdate("drop table "+hostname+"_temptabledelete"),0);
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate("create global temporary table "+hostname+"_temptabledelete (col1 number) on commit delete rows"),0);
		assertEquals(stmt.executeUpdate("insert into "+hostname+"_temptabledelete values (1)"),1);
		rs=stmt.executeQuery("select count(*) from "+hostname+"_temptabledelete");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"1");
		con.commit();
		rs=stmt.executeQuery("select count(*) from "+hostname+"_temptabledelete");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"0");
		assertEquals(stmt.executeUpdate("drop table "+hostname+"_temptabledelete"),0);
		System.out.println();
		try {
			stmt.executeUpdate("truncate table "+hostname+"_temptablepreserve");
		} catch (Exception ex) {
		}
		try {
			stmt.executeUpdate("drop table "+hostname+"_temptablepreserve");
		} catch (Exception ex) {
		}
		assertEquals(stmt.executeUpdate("create global temporary table "+hostname+"_temptablepreserve (col1 number) on commit preserve rows"),0);
		assertEquals(stmt.executeUpdate("insert into "+hostname+"_temptablepreserve values (1)"),1);
		rs=stmt.executeQuery("select count(*) from "+hostname+"_temptablepreserve");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"1");
		con.commit();
		rs=stmt.executeQuery("select count(*) from "+hostname+"_temptablepreserve");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"1");
		con.close();
		System.out.println();
		con=DriverManager.getConnection(url,user,password);
		assertTrue((con!=null));
		stmt=secondcon.createStatement();
		assertTrue((stmt!=null));
		rs=stmt.executeQuery("select count(*) from "+hostname+"_temptablepreserve");
		assertTrue((rs!=null));
		rs.next();
		assertEquals(rs.getString(1),"0");
		assertEquals(stmt.executeUpdate("truncate table "+hostname+"_temptablepreserve"),0);
		Thread.sleep(2000);
		assertEquals(stmt.executeUpdate("drop table "+hostname+"_temptablepreserve"),0);
		try {
			stmt.executeQuery("select count(*) from "+hostname+"_temptablepreserve");
			assertTrue(false);
		} catch (Exception ex) {
			assertTrue(true);
		}
		System.out.println();


		// FIXME: stored procedures

		// FIXME: in/out variables

		// FIXME: rebinding

		// FIXME: need tests for Connection methods...
		// createArrayOf
		// createBlob
		// createClob
		// createNClob
		// createSQLXML
		// createStruct
		// nativeSQL
		// setTypeMap
		// getTypeMap
		// prepareCall
		// setSavepoint
		// releaseSavepoint

		// FIXME: need tests for DatabaseMetaData methods...
                // isWrapperFor
                // unwrap

		// FIXME: need tests for Statement methods...
		// addBatc
                // cancel
                // clearBatch
                // clearWarning
                // closeOnCompletion
                // execute
                // executeBatch
                // getConnection
                // getFetchDirection
                // getFetchSize
                // getGeneratedKeys
                // getMaxFieldSize
                // getMaxRows
                // getMoreResults
                // getQueryTimeout
                // getResultSet
                // getResultSetConcurrency
                // getResultSetHoldability
                // getResultSetType
                // getUpdateCount
                // getWarnings
                // isClosed
                // isCloseOnCompletion
                // isPoolable
                // isWrapperFor
                // setCursorName
                // setEscapeProcessing
                // setFetchDirection
                // setFetchSize
                // setMaxFieldSize
                // setMaxRows
                // setPoolable
                // setQueryTimeout
                // unwrap

		// FIXME: need tests for PreparedStatement methods...
		// addBatch
                // execute
                // executeBatch
                // executeQuery
                // getMetaData
                // getParameterMetaData
                // setArray
                // setAsciiStream
                // setBigDecimal
                // setBinaryStream
                // setBlob
                // setBoolean
                // setByte
                // setCharacterStream
                // setClob
                // setDouble
                // setFloat
                // setLong
                // setNCharacterStream
                // setNClob
                // setNString
                // setNull
                // setObject
                // setRef
                // setRowId
                // setShort
                // setSQLXML
                // setTime
                // setTimestamp
                // setUnicodeStream
                // setURL

		// FIXME: need tests for Parameter class...
		// FIXME: need tests for ParameterMetaData class...

		// FIXME: need tests for ResultSet methods...
		// absolute
                // afterLast
                // beforeFirst
                // cancelRowUpdates
                // clearWarnings
                // deleteRow
                // findColumn
                // first
                // getArray
                // getAsciiStream
                // getBigDecimal
                // getBinaryStream
                // getBoolean
                // getByte
                // getBytes
                // getCharacterStream
                // getConcurrency
                // getCursorName
                // getDate
                // getDouble
                // getFetchDirection
                // getFetchSize
                // getFloat
                // getHoldability
                // getInt
                // getLong
                // getNCharacterStream
                // getNClob
                // getNString
                // getObject
                // getRef
                // getRowId
                // getShort
                // getSQLXML
                // getStatement
                // getTime
                // getTimestamp
                // getType
                // getUnicodeStream
                // getURL
                // getWarnings
                // insertRow
                // isAfterLast
                // isBeforeFirst
                // isClosed
                // isFirst
                // isLast
                // isWrapperFor
                // last
                // moveToCurrentRow
                // moveToInsertRow
                // previous
                // refreshRow
                // relative
                // rowDeleted
                // rowInserted
                // rowUpdated
                // setFetchDirection
                // setFetchSize
                // unwrap
                // updateArray
                // updateAsciiStream
                // updateBigDecimal
                // updateBinaryStream
                // updateBlob
                // updateBoolean
                // updateByte
                // updateBytes
                // updateCharacterStream
                // updateClob
                // updateDate
                // updateDouble
                // updateFloat
                // updateInt
                // updateLong
                // updateNCharacterStream
                // updateNClob
                // updateNString
                // updateNull
                // updateObject
                // updateRef
                // updateRow
                // updateRowId
                // updateShort
                // updateSQLXML
                // updateString
                // updateTime
                // updateTimestamp
                // wasNull

		// FIXME: need tests for ResultSetMetaData methods...
		// getCatalogName
                // getColumnClassName
                // getColumnLabel
                // getColumnType
                // getScale
                // getSchemaName
                // getTableName
                // isAutoIncrement
                // isCaseSensitive
                // isCurrency
                // isDefinitelyWritable
                // isNullable
                // isSearchable
                // isSigned
                // isWrapperFor
                // isWritable
                // unwrap
		

		// invalid queries
		System.out.println("INVALID QUERIES:");
		try {
			stmt.executeQuery("select * from testtable order by testnumber");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeQuery("select * from testtable order by testnumber");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeQuery("select * from testtable order by testnumber");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeQuery("select * from testtable order by testnumber");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		System.out.println();
		try {
			stmt.executeUpdate("insert into testtable values (1,2,3,4)");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeUpdate("insert into testtable values (1,2,3,4)");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeUpdate("insert into testtable values (1,2,3,4)");
			assertTrue(false);
		} catch (Exception e) {
			assertTrue(true);
		}
		try {
			stmt.executeUpdate("insert into testtable values (1,2,3,4)");
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
