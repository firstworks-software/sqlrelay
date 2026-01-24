package com.firstworks.sql;
	
import java.sql.*;
import java.util.regex.*;

import com.firstworks.sqlrelay.*;
	
public class SQLRelayDatabaseMetaData implements DatabaseMetaData {

	private	SQLRelayDriver		driver;
	private SQLRelayConnection	connection;
	private	Object			networklock;

	public SQLRelayDatabaseMetaData(SQLRelayDriver driver) {
		this.driver=driver;
		driver.debugFunction(this);
		connection=null;
		networklock=null;
		driver.debugEnd();
	}

	public
	void setConnection(SQLRelayConnection connection) {
		this.connection=connection;
	}

	public
	void setNetworkLock(Object networklock) {
		this.networklock=networklock;
	}

	public
	boolean allProceduresAreCallable() throws SQLException {
		driver.debugFunction(this);
		// Retrieves whether the current user can call all the
		// procedures returned by the method getProcedures.
		boolean	result=false;
		driver.debugPrintln("all procedures are callable: "+result);
		driver.debugEnd();
		return result;
	}

	public
	boolean allTablesAreSelectable() throws SQLException {
		driver.debugFunction(this);
		// Retrieves whether the current user can use all the tables
		// returned by the method getTables in a SELECT statement.
		boolean	result=false;
		driver.debugPrintln("all tables are selectable: "+result);
		driver.debugEnd();
		return result;
	}

	public
	boolean autoCommitFailureClosesAllResultSets() throws SQLException {
		driver.debugFunction(this);
		// Retrieves whether a SQLException while autoCommit is true
		// inidcates that all open ResultSets are closed, even ones
		// that are holdable.
		// FIXME: no idea if this is true or not
		boolean	result=false;
		driver.debugPrintln("auto commit failures closes "+
						"all result sets: "+result);
		driver.debugEnd();
		return result;
	}

	public
	boolean dataDefinitionCausesTransactionCommit() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	result=false;
		driver.debugPrintln("data definition causes "+
					"transaction commit: "+result);
		driver.debugEnd();
		return result;
	}

	public
	boolean dataDefinitionIgnoredInTransactions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	result=false;
		driver.debugPrintln("data definition ignored "+
					"in transactions: "+result);
		driver.debugEnd();
		return result;
	}

	public
	boolean deletesAreDetected(int type) throws SQLException {
		driver.debugFunction(this);
		// SQL Relay doesn't currenlty support ResultSet.RowDelete
		boolean	result=false;
		driver.debugPrintln("deletes are detected: "+result);
		driver.debugEnd();
		return result;
	}

	public
	boolean doesMaxRowSizeIncludeBlobs() throws SQLException {
		driver.debugFunction(this);
		boolean	result=false;
		driver.debugPrintln("does max row size include blobs: "+result);
		driver.debugEnd();
		return result;
	}

	public
	boolean generatedKeyAlwaysReturned() throws SQLException {
		driver.debugFunction(this);
		boolean	result=true;
		driver.debugPrintln("generated key always returned: "+result);
		driver.debugEnd();
		return result;
	}

	public
	ResultSet getAttributes(String catalog,
					String schemaPattern,
					String typeNamePattern,
					String attributeNamePattern)
					throws SQLException {
		driver.debugFunction(this);
		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema pattern: "+
						schemaPattern);
		driver.debugPrintln("type name pattern: "+
						typeNamePattern);
		driver.debugPrintln("attribute name pattern: "+
						attributeNamePattern);
		// FIXME: implement this somehow...
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public
	ResultSet getBestRowIdentifier(String catalog,
						String schema,
						String table,
						int scope,
						boolean nullable)
						throws SQLException {
		driver.debugFunction(this);
		driver.debugPrintln("schema: "+schema);
		driver.debugPrintln("table: "+table);
		driver.debugPrintln("scope: "+scope);
		driver.debugPrintln("nullable: "+nullable);
		// FIXME: implement this somehow...
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public
	ResultSet getCatalogs() throws SQLException {
		driver.debugFunction(this);

		SQLRelayResultSet	resultset=null;
		SQLRelayStatement	stmt=(SQLRelayStatement)
						connection.createStatement();
		SQLRCursor		sqlrcur=stmt.getSQLRCursor();

		boolean	result=false;
		synchronized (networklock) {
			result=sqlrcur.getDatabaseListWithFormat(null,4);
		}

		if (result) {

			driver.debugPrintln("colcount: "+sqlrcur.colCount());
	
			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(driver);
				resultset.setNetworkLock(networklock);
				resultset.setStatement(stmt);
				resultset.setSQLRCursor(sqlrcur);
			}
		} else {
			throwErrorMessageException(sqlrcur);
		}

		driver.debugEnd();
		return resultset;
	}

	public
	String getCatalogSeparator() throws SQLException {
		driver.debugFunction(this);
		// FIXME: oracle uses @
		String	separator=".";
		driver.debugPrintln("catalog separator: "+separator);
		driver.debugEnd();
		return separator;
	}

	public
	String getCatalogTerm() throws SQLException {
		driver.debugFunction(this);
		// FIXME: I think SQL Server uses catalog, maybe sybase
		String	term="database";
		driver.debugPrintln("catalog term: "+term);
		driver.debugEnd();
		return term;
	}

	public
	ResultSet getClientInfoProperties() throws SQLException {
		driver.debugFunction(this);
		// FIXME: free form in SQL Relay
		driver.debugEnd();
		return null;
	}

	public
	ResultSet getColumnPrivileges(String catalog,
					String schema,
					String table,
					String columnNamePattern)
					throws SQLException {
		driver.debugFunction(this);
		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema: "+schema);
		driver.debugPrintln("table: "+table);
		driver.debugPrintln("column name pattern: "+columnNamePattern);
		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public
	ResultSet getColumns(String catalog,
					String schemaPattern,
					String tableNamePattern,
					String columnNamePattern)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema pattern: "+schemaPattern);
		driver.debugPrintln("table name pattern: "+tableNamePattern);
		driver.debugPrintln("column name pattern: "+columnNamePattern);

		String	wild=buildWild(catalog,schemaPattern,tableNamePattern);
		driver.debugPrintln("wild: "+wild);
		driver.debugPrintln("column name pattern: "+columnNamePattern);

		SQLRelayResultSet	resultset=null;
		SQLRelayStatement	stmt=(SQLRelayStatement)
						connection.createStatement();
		SQLRCursor		sqlrcur=stmt.getSQLRCursor();

		boolean	result=false;
		synchronized (networklock) {
			result=sqlrcur.getColumnListWithFormat(
						wild,columnNamePattern,4);
		}

		if (result) {

			driver.debugPrintln("colcount: "+sqlrcur.colCount());

			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(driver);
				resultset.setNetworkLock(networklock);
				resultset.setStatement(stmt);
				resultset.setSQLRCursor(sqlrcur);
			}
		} else {
			throwErrorMessageException(sqlrcur);
		}
		
		driver.debugEnd();
		return resultset;
	}

	public
	Connection getConnection() throws SQLException {
		//driver.debugFunction(this);
		//driver.debugEnd();
		return connection;
	}

	public
	ResultSet getCrossReference(String parentCatalog,
					String parentSchema,
					String parentTable,
					String foreignCatalog,
					String foreignSchema,
					String foreignTable)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("parent catalog: "+parentCatalog);
		driver.debugPrintln("parent schema: "+parentSchema);
		driver.debugPrintln("parent table: "+parentTable);
		driver.debugPrintln("foreign catalog: "+foreignCatalog);
		driver.debugPrintln("foreign schema: "+foreignSchema);
		driver.debugPrintln("foreign table: "+foreignTable);

		// FIXME: implement this somehow...
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public
	int getDatabaseMajorVersion() throws SQLException {
		driver.debugFunction(this);
		int	majorversion=getDatabaseVersion(true);
		driver.debugPrintln("major version: "+majorversion);
		driver.debugEnd();
		return majorversion;
	}

	public
	int getDatabaseMinorVersion() throws SQLException {
		driver.debugFunction(this);
		int	minorversion=getDatabaseVersion(false);
		driver.debugPrintln("minor version: "+minorversion);
		driver.debugEnd();
		return minorversion;
	}

	private int getDatabaseVersion(boolean major) {
		driver.debugFunction(this);
		String	dbversion=null;
		synchronized (networklock) {
			dbversion=connection.getSQLRConnection().dbVersion();
		}
		// FIXME: cache/fetch dbVersion
		Matcher	matcher=Pattern.compile("[0-9]*\\.[0-9]*").
							matcher(dbversion);
		if (matcher.find()) {
			String[]	parts=matcher.group().split("\\.");
			if (parts!=null && parts.length>((major)?0:1)) {
				driver.debugEnd();
				return Integer.parseInt(parts[(major)?0:1]);
			}
		}
		driver.debugEnd();
		return -1;
	}

	public
	String getDatabaseProductName() throws SQLException {
		driver.debugFunction(this);
		// FIXME: cache/fetch identify
		String	id=null;
		synchronized (networklock) {
			id=connection.getSQLRConnection().identify();
		}
		driver.debugPrintln("product name: "+id);
		driver.debugEnd();
		return id;
	}

	public
	String getDatabaseProductVersion() throws SQLException {
		driver.debugFunction(this);
		// FIXME: cache/fetch dbVersion
		String	productversion=null;
		synchronized (networklock) {
			productversion=connection.getSQLRConnection().
								dbVersion();
		}
		driver.debugPrintln("product version: "+productversion);
		driver.debugEnd();
		return productversion;
	}

	public
	int getDefaultTransactionIsolation() throws SQLException {
		driver.debugFunction(this);
		int	isolation=(getDatabaseProductName().equals("mysql"))?
					Connection.TRANSACTION_REPEATABLE_READ:
					Connection.TRANSACTION_READ_COMMITTED;
		driver.debugPrintln("isolation: "+isolation);
		driver.debugEnd();
		return isolation;
	}

	public
	int getDriverMajorVersion() {
		driver.debugFunction(this);
		int		majorversion=-1;
		String[]	parts=connection.
					getSQLRConnection().
					clientVersion().split(".");
		if (parts!=null && parts.length>0) {
			majorversion=Integer.parseInt(parts[0]);
		}
		driver.debugPrintln("major version: "+majorversion);
		driver.debugEnd();
		return majorversion;
	}

	public
	int getDriverMinorVersion() {
		driver.debugFunction(this);
		int		minorversion=-1;
		String[]	parts=connection.
					getSQLRConnection().
					clientVersion().split(".");
		if (parts!=null && parts.length>1) {
			minorversion=Integer.parseInt(parts[1]);
		}
		driver.debugPrintln("minor version: "+minorversion);
		driver.debugEnd();
		return minorversion;
	}

	public
	String getDriverName() throws SQLException {
		driver.debugFunction(this);
		String	drivername="sqlrelay";
		driver.debugPrintln("driver name: "+drivername);
		driver.debugEnd();
		return drivername;
	}

	public
	String getDriverVersion() throws SQLException {
		driver.debugFunction(this);
		String	driverversion=connection.
					getSQLRConnection().
					clientVersion();
		driver.debugPrintln("driver version: "+driverversion);
		driver.debugEnd();
		return driverversion;
	}

	public
	ResultSet getExportedKeys(String catalog,
					String schema,
					String table)
					throws SQLException {

		driver.debugFunction(this);

		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema: "+schema);
		driver.debugPrintln("table: "+table);

		// Retrieves a description of the foreign key columns that
		// reference the given table's primary key columns (the foreign
		// keys exported by a table).
		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public
	String getExtraNameCharacters() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		String	extranamechars="#@";
		driver.debugPrintln("extra name characters: "+extranamechars);
		driver.debugEnd();
		return extranamechars;
	}

	public
	ResultSet getFunctionColumns(String catalog,
					String schemaPattern,
					String functionNamePattern,
					String columnNamePattern)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema pattern: "+
						schemaPattern);
		driver.debugPrintln("function name pattern: "+
						functionNamePattern);
		driver.debugPrintln("column name pattern: "+
						columnNamePattern);

		// FIXME: implement with
		driver.debugPrintln("FIXME: implement this");
		// sqlrcur.getProcedureBindAndColumnList()?
		driver.debugEnd();
		return null;
	}

	public
	ResultSet getFunctions(String catalog,
					String schemaPattern,
					String functionNamePattern)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema pattern: "+
						schemaPattern);
		driver.debugPrintln("function name pattern: "+
						functionNamePattern);

		// FIXME: implement this by calling sqlrcur.getProcedures()?
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public
	String getIdentifierQuoteString() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		// * sqlserver uses braces
		String	identifierquotestring=
			(getDatabaseProductName().equals("mysql"))?"`":"\"";
		driver.debugPrintln("identifier quote string: "+
					identifierquotestring);
		driver.debugEnd();
		return identifierquotestring;
	}

	public
	ResultSet getImportedKeys(String catalog,
					String schema,
					String table)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema: "+schema);
		driver.debugPrintln("table: "+table);

		// Retrieves a description of the primary key columns that are
		// referenced by the given table's foreign key columns (the
		// primary keys imported by a table).
		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public
	ResultSet getIndexInfo(String catalog,
					String schema,
					String table,
					boolean unique,
					boolean approximate)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema: "+schema);
		driver.debugPrintln("table: "+table);
		driver.debugPrintln("unique: "+unique);
		driver.debugPrintln("approximate: "+approximate);

		// FIXME: implement using sqlrcur.getKeyAndIndexList() ?
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public
	int getJDBCMajorVersion() throws SQLException {
		driver.debugFunction(this);
		// FIXME: get this from ???
		int	jdbcmajorversion=4;
		driver.debugPrintln("jdbc major version: "+jdbcmajorversion);
		driver.debugEnd();
		return jdbcmajorversion;
	}

	public
	int getJDBCMinorVersion() throws SQLException {
		driver.debugFunction(this);
		// FIXME: get this from ???
		int	jdbcminorversion=3;
		driver.debugPrintln("jdbc minor version: "+jdbcminorversion);
		driver.debugEnd();
		return jdbcminorversion;
	}

	public
	int getMaxBinaryLiteralLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxbinaryliterallength=0;
		driver.debugPrintln("max binary literal length: "+
						maxbinaryliterallength);
		driver.debugEnd();
		return maxbinaryliterallength;
	}

	public
	int getMaxCatalogNameLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcatalognamelength=0;
		driver.debugPrintln("max catalog name length: "+
						maxcatalognamelength);
		driver.debugEnd();
		return maxcatalognamelength;
	}

	public
	int getMaxCharLiteralLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcharliterallength=0;
		driver.debugPrintln("max char literal length: "+
						maxcharliterallength);
		driver.debugEnd();
		return maxcharliterallength;
	}

	public
	int getMaxColumnNameLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcolumnnamelength=0;
		driver.debugPrintln("max column name length: "+
						maxcolumnnamelength);
		driver.debugEnd();
		return maxcolumnnamelength;
	}

	public
	int getMaxColumnsInGroupBy() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcolumnsingroup=0;
		driver.debugPrintln("max columns in group: "+
						maxcolumnsingroup);
		driver.debugEnd();
		return maxcolumnsingroup;
	}

	public
	int getMaxColumnsInIndex() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcolumnsinindex=0;
		driver.debugPrintln("max columns in index: "+
						maxcolumnsinindex);
		driver.debugEnd();
		return maxcolumnsinindex;
	}

	public
	int getMaxColumnsInOrderBy() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcolumnsinorderby=0;
		driver.debugPrintln("max columns in order by: "+
						maxcolumnsinorderby);
		driver.debugEnd();
		return maxcolumnsinorderby;
	}

	public
	int getMaxColumnsInSelect() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcolumnsinselect=0;
		driver.debugPrintln("max columns in select: "+
						maxcolumnsinselect);
		driver.debugEnd();
		return maxcolumnsinselect;
	}

	public
	int getMaxColumnsInTable() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcolumnsintable=0;
		driver.debugPrintln("max columns in table: "+
						maxcolumnsintable);
		driver.debugEnd();
		return maxcolumnsintable;
	}

	public
	int getMaxConnections() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxconnections=0;
		driver.debugPrintln("max connections: "+maxconnections);
		driver.debugEnd();
		return maxconnections;
	}

	public
	int getMaxCursorNameLength() throws SQLException {
		driver.debugFunction(this);
		int	maxcursornamelength=0;
		driver.debugPrintln("max cursor name length: "+
						maxcursornamelength);
		driver.debugEnd();
		return maxcursornamelength;
	}

	public
	int getMaxIndexLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxindexlength=0;
		driver.debugPrintln("max index length: "+maxindexlength);
		driver.debugEnd();
		return maxindexlength;
	}

	public
	int getMaxProcedureNameLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxprocedurenamelength=0;
		driver.debugPrintln("max procedure name length: "+
						maxprocedurenamelength);
		driver.debugEnd();
		return maxprocedurenamelength;
	}

	public
	int getMaxRowSize() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxrowsize=0;
		driver.debugPrintln("max row size: "+maxrowsize);
		driver.debugEnd();
		return maxrowsize;
	}

	public
	int getMaxSchemaNameLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxschemanamelength=0;
		driver.debugPrintln("max schema name length: "+
						maxschemanamelength);
		driver.debugEnd();
		return maxschemanamelength;
	}

	public
	int getMaxStatementLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxstatementlength=0;
		driver.debugPrintln("max statement length: "+
						maxstatementlength);
		driver.debugEnd();
		return maxstatementlength;
	}

	public
	int getMaxStatements() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxstatements=0;
		driver.debugPrintln("max statements: "+maxstatements);
		driver.debugEnd();
		return maxstatements;
	}

	public
	int getMaxTableNameLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxtablenamelength=0;
		driver.debugPrintln("max table name length: "+
						maxtablenamelength);
		driver.debugEnd();
		return maxtablenamelength;
	}

	public
	int getMaxTablesInSelect() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxtablesinselect=0;
		driver.debugPrintln("max tables in select: "+
						maxtablesinselect);
		driver.debugEnd();
		return maxtablesinselect;
	}

	public
	int getMaxUserNameLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxusernamelength=0;
		driver.debugPrintln("max user name length: "+maxusernamelength);
		driver.debugEnd();
		return maxusernamelength;
	}

	public
	String getNumericFunctions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		String	numericfunctions=null;
		driver.debugPrintln("numeric functions: "+numericfunctions);
		driver.debugEnd();
		return numericfunctions;
	}

	public
	ResultSet getPrimaryKeys(String catalog,
					String schema,
					String table)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema: "+schema);
		driver.debugPrintln("table: "+table);

		// FIXME: implement this by calling sqlrcon.getPrimaryKeysList()
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public
	ResultSet getProcedureColumns(String catalog,
					String schemaPattern,
					String procedureNamePattern,
					String columnNamePattern)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema pattern: "+
						schemaPattern);
		driver.debugPrintln("procedure name pattern: "+
						procedureNamePattern);
		driver.debugPrintln("column name pattern: "+
						columnNamePattern);

		// FIXME: implement this by calling
		driver.debugPrintln("FIXME: implement this");
		// sqlrcon.getProcedureBindAndColumnList()
		driver.debugEnd();
		return null;
	}

	public
	ResultSet getProcedures(String catalog,
					String schemaPattern,
					String procedureNamePattern)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema pattern: "+
						schemaPattern);
		driver.debugPrintln("procedure name pattern: "+
						procedureNamePattern);

		String	wild=buildWild(catalog,schemaPattern,
						procedureNamePattern);
		driver.debugPrintln("wild: "+wild);

		SQLRelayResultSet	resultset=null;
		SQLRelayStatement	stmt=(SQLRelayStatement)
						connection.createStatement();
		SQLRCursor		sqlrcur=stmt.getSQLRCursor();

		boolean	result=false;
		synchronized (networklock) {
			result=sqlrcur.getProcedureListWithFormat(
						procedureNamePattern,4);
		}

		if (result) {

			driver.debugPrintln("colcount: "+sqlrcur.colCount());

			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(driver);
				resultset.setNetworkLock(networklock);
				resultset.setStatement(stmt);
				resultset.setSQLRCursor(sqlrcur);
			}
		} else {
			throwErrorMessageException(sqlrcur);
		}
		
		driver.debugEnd();
		return resultset;
	}

	public
	String getProcedureTerm() throws SQLException {
		driver.debugFunction(this);
		String	procedureterm="procedure";
		driver.debugPrintln("procedure term: "+procedureterm);
		driver.debugEnd();
		return procedureterm;
	}

	public
	ResultSet getPseudoColumns(String catalog,
					String schemaPattern,
					String tableNamePattern,
					String columnNamePattern)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema pattern: "+schemaPattern);
		driver.debugPrintln("table name pattern: "+tableNamePattern);
		driver.debugPrintln("column name pattern: "+columnNamePattern);

		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public
	int getResultSetHoldability() throws SQLException {
		driver.debugFunction(this);
		// FIXME: is this correct?
		int	resultsetholdability=ResultSet.CLOSE_CURSORS_AT_COMMIT;
		driver.debugPrintln("result set holdability: "+
						resultsetholdability);
		driver.debugEnd();
		return resultsetholdability;
	}

	public
	RowIdLifetime getRowIdLifetime() throws SQLException {
		driver.debugFunction(this);
		// FIXME: some dbs do support rowid
		RowIdLifetime	rowidlifetime=RowIdLifetime.ROWID_UNSUPPORTED;
		switch (rowidlifetime) {
			case ROWID_UNSUPPORTED:
				driver.debugPrintln("rowid lifetime: "+
							"ROWID_UNSUPPORTED");
				break;
			case ROWID_VALID_OTHER:
				driver.debugPrintln("rowid lifetime: "+
							"ROWID_VALID_OTHER");
				break;
			case ROWID_VALID_TRANSACTION:
				driver.debugPrintln("rowid lifetime: "+
						"ROWID_VALID_TRANSACTION");
				break;
			case ROWID_VALID_SESSION:
				driver.debugPrintln("rowid lifetime: "+
							"ROWID_VALID_SESSION");
				break;
			case ROWID_VALID_FOREVER:
				driver.debugPrintln("rowid lifetime: "+
							"ROWID_VALID_FOREVER");
				break;
		}
		driver.debugEnd();
		return rowidlifetime;
	}

	public
	ResultSet getSchemas() throws SQLException {
		driver.debugFunction(this);
		ResultSet	schemas=getSchemas(null,null);
		driver.debugEnd();
		return schemas;
	}

	public
	ResultSet getSchemas(String catalog,
					String schemaPattern)
					throws SQLException {
		driver.debugFunction(this);

		// FIXME: use catalog
		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema pattern: "+schemaPattern);

		SQLRelayResultSet	resultset=null;
		SQLRelayStatement	stmt=(SQLRelayStatement)
						connection.createStatement();
		SQLRCursor		sqlrcur=stmt.getSQLRCursor();

		boolean	result=false;
		synchronized (networklock) {
			result=sqlrcur.getSchemaListWithFormat(schemaPattern,4);
		}

		if (result) {

			driver.debugPrintln("colcount: "+sqlrcur.colCount());

			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(driver);
				resultset.setNetworkLock(networklock);
				resultset.setStatement(stmt);
				resultset.setSQLRCursor(sqlrcur);
			}
		} else {
			throwErrorMessageException(sqlrcur);
		}
	
		driver.debugEnd();
		return resultset;
	}

	public
	String getSchemaTerm() throws SQLException {
		driver.debugFunction(this);
		String	schematerm="schema";
		driver.debugPrintln("schema term: "+schematerm);
		driver.debugEnd();
		return schematerm;
	}

	public
	String getSearchStringEscape() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		String	searchstringescape="\\";
		driver.debugPrintln("search string escape: "+
						searchstringescape);
		driver.debugEnd();
		return searchstringescape;
	}

	public
	String getSQLKeywords() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		String	sqlkeywords=null;
		driver.debugPrintln("sql keywords: "+sqlkeywords);
		driver.debugEnd();
		return sqlkeywords;
	}

	public
	int getSQLStateType() throws SQLException {
		driver.debugFunction(this);
		// FIXME: no idea
		int	sqlstatetype=sqlStateSQL;
		driver.debugPrintln("sql state type: "+sqlstatetype);
		driver.debugEnd();
		return sqlstatetype;
	}

	public
	String getStringFunctions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		String	stringfunctions=null;
		driver.debugPrintln("string functions: "+stringfunctions);
		driver.debugEnd();
		return stringfunctions;
	}

	public
	ResultSet getSuperTables(String catalog,
					String schemaPattern,
					String tableNamePattern)
					throws SQLException {
		driver.debugFunction(this);
		// few jdbc drivers (or databases) support this
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public
	ResultSet getSuperTypes(String catalog,
					String schemaPattern,
					String typeNamePattern)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema pattern: "+schemaPattern);
		driver.debugPrintln("type name pattern: "+typeNamePattern);

		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public
	String getSystemFunctions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		String	systemfunctions=null;
		driver.debugEnd();
		return systemfunctions;
	}

	public
	ResultSet getTablePrivileges(String catalog,
					String schemaPattern,
					String tableNamePattern)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema pattern: "+schemaPattern);
		driver.debugPrintln("table name pattern: "+tableNamePattern);

		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public
	ResultSet getTables(String catalog,
				String schemaPattern,
				String tableNamePattern,
				String[] types)
				throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema pattern: "+schemaPattern);
		driver.debugPrintln("table name pattern: "+tableNamePattern);

		int	objecttypes=0;
		if (types==null) {
			driver.debugPrintln("types: null");
			objecttypes=1|2|3|4;
		} else {
			String	t="";
			for (String type: types) {
				t=t+type+",";
				if (type.equals("TABLE") ||
					type.equals("SYSTEM TABLE") ||
					type.equals("GLOBAL TEMPORARY") ||
					type.equals("LOCAL TEMPORARY")) {
					objecttypes|=1;
				} else if (type.equals("VIEW")) {
					objecttypes|=2;
				} else if (type.equals("ALIAS")) {
					objecttypes|=3;
				} else if (type.equals("SYNONYM")) {
					objecttypes|=4;
				}
			}
			driver.debugPrintln("types: "+t);
		}

		String	wild=buildWild(catalog,schemaPattern,tableNamePattern);
		driver.debugPrintln("wild: "+wild);

		SQLRelayResultSet	resultset=null;
		SQLRelayStatement	stmt=(SQLRelayStatement)
						connection.createStatement();
		SQLRCursor		sqlrcur=stmt.getSQLRCursor();

		boolean	result=false;
		synchronized (networklock) {
			result=sqlrcur.getTableListWithFormat(
					tableNamePattern,4,objecttypes);
		}

		if (result) {

			driver.debugPrintln("colcount: "+sqlrcur.colCount());

			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(driver);
				resultset.setNetworkLock(networklock);
				resultset.setStatement(stmt);
				resultset.setSQLRCursor(sqlrcur);
			}
		} else {
			throwErrorMessageException(sqlrcur);
		}
		
		driver.debugEnd();
		return resultset;
	}

	private String buildWild(String catalog,
					String schema,
					String object) {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema: "+schema);
		driver.debugPrintln("object: "+object);

		// If object already contains a . then just use it
		// as-is.
		if (object!=null && object.contains(".")) {
			driver.debugEnd();
			return object;
		}

		// Concatenate parts until wild is one of the following formats:
		// * table
		// * schema.table
		// * catalog.schema.table

		StringBuilder	wild=new StringBuilder();
		if (catalog!=null) {
			if (catalog.equals("")) {
				// retrieve objects without a catalog
				// FIXME: how???
			} else {
				wild.append(catalog).append('.');
			}
		}
		if (schema!=null) {
			if (schema.equals("")) {
				// retrieve objects without a schema
				// FIXME: how???
			} else {
				wild.append(schema).append('.');
			}
		} else if (wild.length()>0) {
			// if schema was null, but a catalog was
			// specified then include all schemas
			wild.append("%.");
		}
		if (object!=null && !object.equals("")) {
			wild.append(object);
		} else {
			wild.append('%');
		}
		driver.debugEnd();
		return wild.toString();
	}

	public
	ResultSet getTableTypes() throws SQLException {
		driver.debugFunction(this);

		SQLRelayResultSet	resultset=null;
		SQLRelayStatement	stmt=(SQLRelayStatement)
						connection.createStatement();
		SQLRCursor		sqlrcur=stmt.getSQLRCursor();

		boolean	result=false;
		synchronized (networklock) {
			result=sqlrcur.getTableTypeListWithFormat(null,4);
		}

		if (result) {

			driver.debugPrintln("colcount: "+sqlrcur.colCount());

			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(driver);
				resultset.setNetworkLock(networklock);
				resultset.setStatement(stmt);
				resultset.setSQLRCursor(sqlrcur);
			}
		} else {
			throwErrorMessageException(sqlrcur);
		}
	
		driver.debugEnd();
		return resultset;
	}

	public
	String getTimeDateFunctions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		String	timedatefunctions=null;
		driver.debugPrintln("timedate functions: "+timedatefunctions);
		driver.debugEnd();
		return timedatefunctions;
	}

	public
	ResultSet getTypeInfo() throws SQLException {
		driver.debugFunction(this);

		SQLRelayResultSet	resultset=null;
		SQLRelayStatement	stmt=(SQLRelayStatement)
						connection.createStatement();
		SQLRCursor		sqlrcur=stmt.getSQLRCursor();

		boolean	result=false;
		synchronized (networklock) {
			result=sqlrcur.getTypeInfoListWithFormat("*",null,4);
		}

		if (result) {

			driver.debugPrintln("colcount: "+sqlrcur.colCount());

			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(driver);
				resultset.setNetworkLock(networklock);
				resultset.setStatement(stmt);
				resultset.setSQLRCursor(sqlrcur);
			}
		} else {
			throwErrorMessageException(sqlrcur);
		}
	
		driver.debugEnd();
		return resultset;
	}

	public
	ResultSet getUDTs(String catalog,
				String schemaPattern,
				String typeNamePattern,
				int[] types)
				throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema pattern: "+schemaPattern);
		driver.debugPrintln("type name pattern: "+typeNamePattern);
		// FIXME: debug types...

		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public
	String getURL() throws SQLException {
		driver.debugFunction(this);

		String	host=connection.getHost();
		short	port=connection.getPort();
		String	socket=connection.getSocket();
		String	user=connection.getUser();
		String	password=connection.getPassword();

		String	url="jdbc:sqlrelay://";
		if (user!=null && !user.equals("")) {
			url=url+user;
			if (password!=null && !password.equals("")) {
				url=url+":"+password;
			}
			url=url+"@";
		}
		url=url+host+":"+port;
		if (socket!=null && !socket.equals("")) {
			url=url+":"+socket;
		}

		driver.debugPrintln("url: "+url);

		driver.debugEnd();
		return url;
	}

	public
	String getUserName() throws SQLException {
		driver.debugFunction(this);
		String	username=connection.getUser();
		driver.debugPrintln("user name: "+username);
		driver.debugEnd();
		return username;
	}

	public
	ResultSet getVersionColumns(String catalog,
					String schema,
					String table)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: "+catalog);
		driver.debugPrintln("schema: "+schema);
		driver.debugPrintln("table: "+table);

		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public
	boolean insertsAreDetected(int type) throws SQLException {
		driver.debugFunction(this);
		boolean	insertsaredetected=false;
		driver.debugPrintln("type: "+type);
		driver.debugPrintln("inserts are detected: "+
						insertsaredetected);
		driver.debugEnd();
		return insertsaredetected;
	}

	public
	boolean isCatalogAtStart() throws SQLException {
		driver.debugFunction(this);
		// FIXME: not in oracle
		boolean	iscatalogatstart=true;
		driver.debugPrintln("is catalog at start: "+iscatalogatstart);
		driver.debugEnd();
		return iscatalogatstart;
	}

	public
	boolean isReadOnly() throws SQLException {
		driver.debugFunction(this);
		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		boolean	isreadonly=false;
		driver.debugPrintln("is read only: "+isreadonly);
		driver.debugEnd();
		return isreadonly;
	}

	public
	boolean locatorsUpdateCopy() throws SQLException {
		driver.debugFunction(this);
		// FIXME: no idea, probably db-specific
		boolean	locatorsupdatecopy=false;
		driver.debugPrintln("locators update copy: "+
						locatorsupdatecopy);
		driver.debugEnd();
		return locatorsupdatecopy;
	}

	public
	boolean nullPlusNonNullIsNull() throws SQLException {
		driver.debugFunction(this);
		// FIXME: generally true, but probably db-specific
		boolean	nullplusnonnullisnull=true;
		driver.debugPrintln("null plus non null is null: "+
						nullplusnonnullisnull);
		driver.debugEnd();
		return nullplusnonnullisnull;
	}

	public
	boolean nullsAreSortedAtEnd() throws SQLException {
		driver.debugFunction(this);
		// FIXME: generally true, but probably db-specific
		boolean	nullsaresortedatend=true;
		driver.debugPrintln("nulls are sorted at end: "+
						nullsaresortedatend);
		driver.debugEnd();
		return nullsaresortedatend;
	}

	public
	boolean nullsAreSortedAtStart() throws SQLException {
		driver.debugFunction(this);
		// FIXME: generally false, but probably db-specific
		boolean	nullsaresortedatstart=false;
		driver.debugPrintln("nulls are sorted at start: "+
						nullsaresortedatstart);
		driver.debugEnd();
		return nullsaresortedatstart;
	}

	public
	boolean nullsAreSortedHigh() throws SQLException {
		driver.debugFunction(this);
		// FIXME: generally true, but probably db-specific
		boolean	nullsaresortedhigh=true;
		driver.debugPrintln("nulls are sorted high: "+
						nullsaresortedhigh);
		driver.debugEnd();
		return nullsaresortedhigh;
	}

	public
	boolean nullsAreSortedLow() throws SQLException {
		driver.debugFunction(this);
		// FIXME: generally false, but probably db-specific
		boolean	nullsaresortedlow=false;
		driver.debugPrintln("nulls are sorted low: "+
						nullsaresortedlow);
		driver.debugEnd();
		return nullsaresortedlow;
	}

	public
	boolean othersDeletesAreVisible(int type) throws SQLException {
		driver.debugFunction(this);
		boolean	othersdeletesarevisible=false;
		driver.debugPrintln("others deletes are visible: "+
						othersdeletesarevisible);
		driver.debugEnd();
		return othersdeletesarevisible;
	}

	public
	boolean othersInsertsAreVisible(int type) throws SQLException {
		driver.debugFunction(this);
		boolean	othersinsertssarevisible=false;
		driver.debugPrintln("others inserts are visible: "+
						othersinsertssarevisible);
		driver.debugEnd();
		return othersinsertssarevisible;
	}

	public
	boolean othersUpdatesAreVisible(int type) throws SQLException {
		driver.debugFunction(this);
		boolean	othersupdatessarevisible=false;
		driver.debugPrintln("others updates are visible: "+
						othersupdatessarevisible);
		driver.debugEnd();
		return othersupdatessarevisible;
	}

	public
	boolean ownDeletesAreVisible(int type) throws SQLException {
		driver.debugFunction(this);
		boolean	owndeletesarevisible=false;
		driver.debugPrintln("own deletes are visible: "+
						owndeletesarevisible);
		driver.debugEnd();
		return owndeletesarevisible;
	}

	public
	boolean ownInsertsAreVisible(int type) throws SQLException {
		driver.debugFunction(this);
		boolean	owninsertsarevisible=false;
		driver.debugPrintln("own inserts are visible: "+
						owninsertsarevisible);
		driver.debugEnd();
		return owninsertsarevisible;
	}

	public
	boolean ownUpdatesAreVisible(int type) throws SQLException {
		driver.debugFunction(this);
		boolean	ownupdatesarevisible=false;
		driver.debugPrintln("own updates are visible: "+
						ownupdatesarevisible);
		driver.debugEnd();
		return ownupdatesarevisible;
	}

	public
	boolean storesLowerCaseIdentifiers() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific but generally false
		// oracle stores upper case identifiers
		// other db's store mixed case identifiers
		boolean	storeslowercaseidentifiers=false;
		driver.debugPrintln("stores lower case identifiers: "+
						storeslowercaseidentifiers);
		driver.debugEnd();
		return storeslowercaseidentifiers;
	}

	public
	boolean storesLowerCaseQuotedIdentifiers() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	storeslowercasequotedidentifiers=false;
		driver.debugPrintln("stores lower case quoted identifiers: "+
					storeslowercasequotedidentifiers);
		driver.debugEnd();
		return storeslowercasequotedidentifiers;
	}

	public
	boolean storesMixedCaseIdentifiers() throws SQLException {
		driver.debugFunction(this);
		// FIXME: generally true, but db-specific, false for oracle
		boolean	storesmixedcaseidentifiers=true;
		driver.debugPrintln("stores mixed case identifiers: "+
						storesmixedcaseidentifiers);
		driver.debugEnd();
		return storesmixedcaseidentifiers;
	}

	public
	boolean storesMixedCaseQuotedIdentifiers() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	storesmixedcasequotedidentifiers=true;
		driver.debugPrintln("stores mixed case quoted identifiers: "+
					storesmixedcasequotedidentifiers);
		driver.debugEnd();
		return storesmixedcasequotedidentifiers;
	}

	public
	boolean storesUpperCaseIdentifiers() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific but generally false
		// oracle stores upper case identifiers
		// other db's store mixed case identifiers
		boolean	storesuppercaseidentifiers=false;
		driver.debugPrintln("stores upper case identifiers: "+
						storesuppercaseidentifiers);
		driver.debugEnd();
		return storesuppercaseidentifiers;
	}

	public
	boolean storesUpperCaseQuotedIdentifiers() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	storesuppercasequotedidentifiers=true;
		driver.debugPrintln("stores upper case quoted identifiers: "+
					storesuppercasequotedidentifiers);
		driver.debugEnd();
		return storesuppercasequotedidentifiers;
	}

	public
	boolean supportsAlterTableWithAddColumn() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsaltertablewithaddcommand=true;
		driver.debugPrintln("supports alter table with add command: "+
					supportsaltertablewithaddcommand);
		driver.debugEnd();
		return supportsaltertablewithaddcommand;
	}

	public
	boolean supportsAlterTableWithDropColumn() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsaltertablewithdropcommand=true;
		driver.debugPrintln("supports alter table with drop command: "+
					supportsaltertablewithdropcommand);
		driver.debugEnd();
		return supportsaltertablewithdropcommand;
	}

	public
	boolean supportsANSI92EntryLevelSQL() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsansi92entrylevelsql=true;
		driver.debugPrintln("supports ansi92 entry level sql: "+
						supportsansi92entrylevelsql);
		driver.debugEnd();
		return supportsansi92entrylevelsql;
	}

	public
	boolean supportsANSI92FullSQL() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsansi92fullsql=true;
		driver.debugPrintln("supports ansi92 full sql: "+
						supportsansi92fullsql);
		driver.debugEnd();
		return supportsansi92fullsql;
	}

	public
	boolean supportsANSI92IntermediateSQL() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsansi92intermediatesql=true;
		driver.debugPrintln("supports ansi92 intermediate sql: "+
						supportsansi92intermediatesql);
		driver.debugEnd();
		return supportsansi92intermediatesql;
	}

	public
	boolean supportsBatchUpdates() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsbatchupdates=false;
		driver.debugPrintln("supports batch updates: "+
						supportsbatchupdates);
		driver.debugEnd();
		return supportsbatchupdates;
	}

	public
	boolean supportsCatalogsInDataManipulation() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscatalogsindatamanipulation=true;
		driver.debugPrintln("supports catalogs in data manipulations: "+
					supportscatalogsindatamanipulation);
		driver.debugEnd();
		return supportscatalogsindatamanipulation;
	}

	public
	boolean supportsCatalogsInIndexDefinitions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscatalogsinindexdefinitions=true;
		driver.debugPrintln("supports catalogs in index definitions: "+
					supportscatalogsinindexdefinitions);
		driver.debugEnd();
		return supportscatalogsinindexdefinitions;
	}

	public
	boolean supportsCatalogsInPrivilegeDefinitions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscatalogsinprivilegedefinitions=true;
		driver.debugPrintln("supports catalogs in "+
					"privilege definitions: "+
					supportscatalogsinprivilegedefinitions);
		driver.debugEnd();
		return supportscatalogsinprivilegedefinitions;
	}

	public
	boolean supportsCatalogsInProcedureCalls() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscatalogsinprocedurecalls=true;
		driver.debugPrintln("supports catalogs in procedure calls: "+
					supportscatalogsinprocedurecalls);
		driver.debugEnd();
		return supportscatalogsinprocedurecalls;
	}

	public
	boolean supportsCatalogsInTableDefinitions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscatalogsintabledefinitions=true;
		driver.debugPrintln("supports catalogs in table definitions: "+
					supportscatalogsintabledefinitions);
		driver.debugEnd();
		return supportscatalogsintabledefinitions;
	}

	public
	boolean supportsColumnAliasing() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscolumnaliasing=true;
		driver.debugPrintln("supports column aliasing: "+
					supportscolumnaliasing);
		driver.debugEnd();
		return supportscolumnaliasing;
	}

	public
	boolean supportsConvert() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsconvert=true;
		driver.debugPrintln("supports convert: "+supportsconvert);
		driver.debugEnd();
		return supportsconvert;
	}

	public
	boolean supportsConvert(int fromType, int toType) throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-and-type-specific
		boolean	supportsconvert=true;
		driver.debugPrintln("from type: "+fromType);
		driver.debugPrintln("to type: "+toType);
		driver.debugPrintln("supports convert: "+supportsconvert);
		driver.debugEnd();
		return supportsconvert;
	}

	public
	boolean supportsCoreSQLGrammar() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscoresqlgrammar=true;
		driver.debugPrintln("supports core sql grammar: "+
						supportscoresqlgrammar);
		driver.debugEnd();
		return supportscoresqlgrammar;
	}

	public
	boolean supportsCorrelatedSubqueries() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscorrelatedsubqueries=true;
		driver.debugPrintln("supports correlated subqueries: "+
						supportscorrelatedsubqueries);
		driver.debugEnd();
		return supportscorrelatedsubqueries;
	}

	public
	boolean supportsDataDefinitionAndDataManipulationTransactions()
							throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	sddadmt=true;
		driver.debugPrintln("supports data definition "+
					"and data manipulation transactions: "+
					sddadmt);
		driver.debugEnd();
		return sddadmt;
	}

	public
	boolean supportsDataManipulationTransactionsOnly() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	sdmto=false;
		driver.debugPrintln("supports data manipulation "+
					"transactions only: "+sdmto);
		driver.debugEnd();
		return sdmto;
	}

	public
	boolean supportsDifferentTableCorrelationNames() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	sdtcn=true;
		driver.debugPrintln("supports different table "+
					"correlation names: "+sdtcn);
		driver.debugEnd();
		return sdtcn;
	}

	public
	boolean supportsExpressionsInOrderBy() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsexpressionsinorderby=true;
		driver.debugPrintln("supports expressions in order by: "+
						supportsexpressionsinorderby);
		driver.debugEnd();
		return supportsexpressionsinorderby;
	}

	public
	boolean supportsExtendedSQLGrammar() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsextendedsqlgrammar=true;
		driver.debugPrintln("supports extended sql grammar: "+
						supportsextendedsqlgrammar);
		driver.debugEnd();
		return supportsextendedsqlgrammar;
	}

	public
	boolean supportsFullOuterJoins() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsfullouterjoins=true;
		driver.debugPrintln("supports full outer joins: "+
						supportsfullouterjoins);
		driver.debugEnd();
		return supportsfullouterjoins;
	}

	public
	boolean supportsGetGeneratedKeys() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsgetgeneratedkeys=true;
		driver.debugPrintln("supports get generated keys: "+
						supportsgetgeneratedkeys);
		driver.debugEnd();
		return supportsgetgeneratedkeys;
	}

	public
	boolean supportsGroupBy() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsgroupby=true;
		driver.debugPrintln("supports group by: "+supportsgroupby);
		driver.debugEnd();
		return supportsgroupby;
	}

	public
	boolean supportsGroupByBeyondSelect() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsgroupbybeyondselect=true;
		driver.debugPrintln("supports group by beyond select: "+
						supportsgroupbybeyondselect);
		driver.debugEnd();
		return supportsgroupbybeyondselect;
	}

	public
	boolean supportsGroupByUnrelated() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsgroupbyunrelated=true;
		driver.debugPrintln("supports group by unrelated: "+
						supportsgroupbyunrelated);
		driver.debugEnd();
		return supportsgroupbyunrelated;
	}

	public
	boolean supportsIntegrityEnhancementFacility() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsintegrityenhancementfacility=false;
		driver.debugPrintln("supports integrity enhancement facility: "+
					supportsintegrityenhancementfacility);
		driver.debugEnd();
		return supportsintegrityenhancementfacility;
	}

	public
	boolean supportsLikeEscapeClause() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportslikeescapeclause=true;
		driver.debugPrintln("supports like escape clause: "+
						supportslikeescapeclause);
		driver.debugEnd();
		return supportslikeescapeclause;
	}

	public
	boolean supportsLimitedOuterJoins() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportslimitedouterjoins=true;
		driver.debugPrintln("supports limited outer joins: "+
						supportslimitedouterjoins);
		driver.debugEnd();
		return supportslimitedouterjoins;
	}

	public
	boolean supportsMinimumSQLGrammar() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsminimumsqlgrammar=true;
		driver.debugPrintln("supports minimum sql grammar: "+
						supportsminimumsqlgrammar);
		driver.debugEnd();
		return supportsminimumsqlgrammar;
	}

	public
	boolean supportsMixedCaseIdentifiers() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific, oracle doesn't
		boolean	supportsmixedcaseidentifiers=true;
		driver.debugPrintln("supports mixed case identifiers: "+
						supportsmixedcaseidentifiers);
		driver.debugEnd();
		return supportsmixedcaseidentifiers;
	}

	public
	boolean supportsMixedCaseQuotedIdentifiers() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsmixedcasequotedidentifiers=true;
		driver.debugPrintln("supports mixed case quoted identifiers: "+
					supportsmixedcasequotedidentifiers);
		driver.debugEnd();
		return supportsmixedcasequotedidentifiers;
	}

	public
	boolean supportsMultipleOpenResults() throws SQLException {
		driver.debugFunction(this);
		boolean	supportsmultipleopenresults=true;
		driver.debugPrintln("supports multiple open results: "+
						supportsmultipleopenresults);
		driver.debugEnd();
		return supportsmultipleopenresults;
	}

	public
	boolean supportsMultipleResultSets() throws SQLException {
		driver.debugFunction(this);
		// FIXME: in progress...
		boolean	supportsmultipleresultsets=false;
		driver.debugPrintln("supports multiple result sets: "+
						supportsmultipleresultsets);
		driver.debugEnd();
		return supportsmultipleresultsets;
	}

	public
	boolean supportsMultipleTransactions() throws SQLException {
		driver.debugFunction(this);
		boolean	supportsmultipletransactions=false;
		driver.debugPrintln("supports multiple transactions: "+
						supportsmultipletransactions);
		driver.debugEnd();
		return supportsmultipletransactions;
	}

	public
	boolean supportsNamedParameters() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsnamedparameters=true;
		driver.debugPrintln("supports named parameters: "+
						supportsnamedparameters);
		driver.debugEnd();
		return supportsnamedparameters;
	}

	public
	boolean supportsNonNullableColumns() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsnonnullablecolumns=true;
		driver.debugPrintln("supports non-nullable columns: "+
						supportsnonnullablecolumns);
		driver.debugEnd();
		return supportsnonnullablecolumns;
	}

	public
	boolean supportsOpenCursorsAcrossCommit() throws SQLException {
		driver.debugFunction(this);
		// FIXME: not sure
		boolean	supportsopencursorsacrosscommit=false;
		driver.debugPrintln("supports open cursors across commit: "+
					supportsopencursorsacrosscommit);
		driver.debugEnd();
		return supportsopencursorsacrosscommit;
	}

	public
	boolean supportsOpenCursorsAcrossRollback() throws SQLException {
		driver.debugFunction(this);
		// FIXME: not sure
		boolean	supportsopencursorsacrossrollback=false;
		driver.debugPrintln("supports open cursors across rollback: "+
					supportsopencursorsacrossrollback);
		driver.debugEnd();
		return supportsopencursorsacrossrollback;
	}

	public
	boolean supportsOpenStatementsAcrossCommit() throws SQLException {
		driver.debugFunction(this);
		// FIXME: not sure
		boolean	supportsopenstatementsacrosscommit=false;
		driver.debugPrintln("supports open statements across commit: "+
					supportsopenstatementsacrosscommit);
		driver.debugEnd();
		return supportsopenstatementsacrosscommit;
	}

	public
	boolean supportsOpenStatementsAcrossRollback() throws SQLException {
		driver.debugFunction(this);
		// FIXME: not sure
		boolean	supportsopenstatementsacrossrollback=false;
		driver.debugPrintln("supports open statements "+
					"across rollback: "+
					supportsopenstatementsacrossrollback);
		driver.debugEnd();
		return supportsopenstatementsacrossrollback;
	}

	public
	boolean supportsOrderByUnrelated() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsorderbyunrelated=true;
		driver.debugPrintln("supports order by unrelated: "+
					supportsorderbyunrelated);
		driver.debugEnd();
		return supportsorderbyunrelated;
	}

	public
	boolean supportsOuterJoins() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsouterjoins=true;
		driver.debugPrintln("supports outer joins: "+
						supportsouterjoins);
		driver.debugEnd();
		return supportsouterjoins;
	}

	public
	boolean supportsPositionedDelete() throws SQLException {
		driver.debugFunction(this);
		boolean	supportspositioneddelete=false;
		driver.debugPrintln("supports positioned delete: "+
						supportspositioneddelete);
		driver.debugEnd();
		return supportspositioneddelete;
	}

	public
	boolean supportsPositionedUpdate() throws SQLException {
		driver.debugFunction(this);
		boolean	supportspositionedupdate=false;
		driver.debugPrintln("supports positioned update: "+
						supportspositionedupdate);
		driver.debugEnd();
		return supportspositionedupdate;
	}

	public
	boolean supportsResultSetConcurrency(int type,
						int concurrency)
						throws SQLException {
		driver.debugFunction(this);
		boolean	supportsresultsetconcurrency=
				(type==ResultSet.TYPE_FORWARD_ONLY &&
				concurrency==ResultSet.CONCUR_READ_ONLY);
		driver.debugPrintln("supports result set concurrency: "+
						supportsresultsetconcurrency);
		driver.debugEnd();
		return supportsresultsetconcurrency;
	}

	public
	boolean supportsResultSetHoldability(int holdability)
							throws SQLException {
		driver.debugFunction(this);
		boolean	supportsresultsetholdability=
			(holdability==ResultSet.CLOSE_CURSORS_AT_COMMIT);
		driver.debugPrintln("supports result set holdability: "+
						supportsresultsetholdability);
		driver.debugEnd();
		return supportsresultsetholdability;
	}

	public
	boolean supportsResultSetType(int type) throws SQLException {
		driver.debugFunction(this);
		boolean	supportsresultsettype=
			(type==ResultSet.TYPE_FORWARD_ONLY);
		driver.debugPrintln("supports result set type: "+
						supportsresultsettype);
		driver.debugEnd();
		return supportsresultsettype;
	}

	public
	boolean supportsSavepoints() throws SQLException {
		driver.debugFunction(this);
		boolean	supportssavepoints=false;
		driver.debugPrintln("supports savepoints: "+supportssavepoints);
		driver.debugEnd();
		return supportssavepoints;
	}

	public
	boolean supportsSchemasInDataManipulation() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsschemasindatamanipulation=true;
		driver.debugPrintln("supports schemas in data manipulation: "+
					supportsschemasindatamanipulation);
		driver.debugEnd();
		return supportsschemasindatamanipulation;
	}

	public
	boolean supportsSchemasInIndexDefinitions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsschemasinindexdefinitions=true;
		driver.debugPrintln("supports schemas in index definitions: "+
					supportsschemasinindexdefinitions);
		driver.debugEnd();
		return supportsschemasinindexdefinitions;
	}

	public
	boolean supportsSchemasInPrivilegeDefinitions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsschemasinprivilegedefinitions=true;
		driver.debugPrintln("supports schemas in "+
					"privilege definitions: "+
					supportsschemasinprivilegedefinitions);
		driver.debugEnd();
		return supportsschemasinprivilegedefinitions;
	}

	public
	boolean supportsSchemasInProcedureCalls() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsschemasinprocedurecalls=true;
		driver.debugPrintln("supports schemas in procedure calls: "+
					supportsschemasinprocedurecalls);
		driver.debugEnd();
		return supportsschemasinprocedurecalls;
	}

	public
	boolean supportsSchemasInTableDefinitions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsschemasintabledefinitions=true;
		driver.debugPrintln("supports schemas in table definitions: "+
					supportsschemasintabledefinitions);
		driver.debugEnd();
		return supportsschemasintabledefinitions;
	}

	public
	boolean supportsSelectForUpdate() throws SQLException {
		driver.debugFunction(this);
		boolean	supportsselectforupdate=false;
		driver.debugPrintln("supports select for update: "+
						supportsselectforupdate);
		driver.debugEnd();
		return supportsselectforupdate;
	}

	public
	boolean supportsStatementPooling() throws SQLException {
		driver.debugFunction(this);
		boolean	supportsstatementpooling=false;
		driver.debugPrintln("supports statement pooling: "+
						supportsstatementpooling);
		driver.debugEnd();
		return supportsstatementpooling;
	}

	public
	boolean supportsStoredFunctionsUsingCallSyntax() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	ssfucs=false;
		driver.debugPrintln("supports stored functions "+
					"using call syntax: "+ssfucs);
		driver.debugEnd();
		return ssfucs;
	}

	public
	boolean supportsStoredProcedures() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsstoredprocedures=true;
		driver.debugPrintln("supports stored procedures: "+
						supportsstoredprocedures);
		driver.debugEnd();
		return supportsstoredprocedures;
	}

	public
	boolean supportsSubqueriesInComparisons() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportssubqueriesincomparisons=true;
		driver.debugPrintln("supports subqueries in comparisons: "+
					supportssubqueriesincomparisons);
		driver.debugEnd();
		return supportssubqueriesincomparisons;
	}

	public
	boolean supportsSubqueriesInExists() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportssubqueriesinexists=true;
		driver.debugPrintln("supports subqueries in exists: "+
						supportssubqueriesinexists);
		driver.debugEnd();
		return supportssubqueriesinexists;
	}

	public
	boolean supportsSubqueriesInIns() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportssubqueriesinins=true;
		driver.debugPrintln("supports subqueries in ins: "+
						supportssubqueriesinins);
		driver.debugEnd();
		return supportssubqueriesinins;
	}

	public
	boolean supportsSubqueriesInQuantifieds() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportssubqueriesinquantifieds=true;
		driver.debugPrintln("supports subqueries in quantifieds: "+
					supportssubqueriesinquantifieds);
		driver.debugEnd();
		return supportssubqueriesinquantifieds;
	}

	public
	boolean supportsTableCorrelationNames() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportssubqueriesincorrelationnames=true;
		driver.debugPrintln("supports subqueries in "+
					"correlation names: "+
					supportssubqueriesincorrelationnames);
		driver.debugEnd();
		return supportssubqueriesincorrelationnames;
	}

	public
	boolean supportsTransactionIsolationLevel(int level)
						throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportstransactionisolationlevel=true;
		driver.debugPrintln("supports transaction isolation level: "+
					supportstransactionisolationlevel);
		driver.debugEnd();
		return supportstransactionisolationlevel;
	}

	public
	boolean supportsTransactions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportstransactions=true;
		driver.debugPrintln("supports transactions: "+
						supportstransactions);
		driver.debugEnd();
		return supportstransactions;
	}

	public
	boolean supportsUnion() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsunion=true;
		driver.debugPrintln("supports union: "+supportsunion);
		driver.debugEnd();
		return supportsunion;
	}

	public
	boolean supportsUnionAll() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsunionall=true;
		driver.debugPrintln("supports union all: "+supportsunionall);
		driver.debugEnd();
		return supportsunionall;
	}

	public
	boolean updatesAreDetected(int type) throws SQLException {
		driver.debugFunction(this);
		boolean	updatesaredetected=false;
		driver.debugPrintln("updates are detected: "+
						updatesaredetected);
		driver.debugEnd();
		return updatesaredetected;
	}

	public
	boolean usesLocalFilePerTable() throws SQLException {
		driver.debugFunction(this);
		boolean	useslocalfilepertable=false;
		driver.debugPrintln("uses local file per table: "+
						useslocalfilepertable);
		driver.debugEnd();
		return useslocalfilepertable;
	}

	public
	boolean usesLocalFiles() throws SQLException {
		driver.debugFunction(this);
		boolean	useslocalfiles=false;
		driver.debugPrintln("uses local files: "+useslocalfiles);
		driver.debugEnd();
		return useslocalfiles;
	}

	protected void throwErrorMessageException(SQLRCursor sqlrcur)
							throws SQLException {
		driver.debugPrintln("exception: "+sqlrcur.errorMessage());
		driver.debugZeroIndent();
		throw new SQLException(sqlrcur.errorMessage());
	}

	protected void throwFeatureNotSupportedException() throws SQLException {
		driver.debugPrintln(
			"exception: SQLFeatureNotSupportedException");
		driver.debugZeroIndent();
		throw new SQLFeatureNotSupportedException();
	}

	public
	boolean isWrapperFor(Class<?> iface) throws SQLException {
		driver.debugFunction(this);
		driver.debugEnd();
		return false;
	}

	public
	<T> T unwrap(Class<T> iface) throws SQLException {
		driver.debugFunction(this);
		driver.debugEnd();
		return null;
	}
};
