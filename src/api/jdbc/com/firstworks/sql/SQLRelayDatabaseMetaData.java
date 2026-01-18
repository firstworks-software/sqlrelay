package com.firstworks.sql;
	
import java.sql.*;
import java.util.regex.*;

import com.firstworks.sqlrelay.*;
	
public class SQLRelayDatabaseMetaData implements DatabaseMetaData {

	private SQLRelayConnection	connection;
	private	SQLRelayDriver		driver;

	public SQLRelayDatabaseMetaData(SQLRelayDriver driver) {
		this.driver=driver;
		driver.debugFunction(this);
		connection=null;
		// FIXME: set protected member variables?
		driver.debugEnd();
	}

	public synchronized
	void setConnection(SQLRelayConnection connection) {
		this.connection=connection;
	}

	public synchronized
	boolean allProceduresAreCallable() throws SQLException {
		driver.debugFunction(this);
		// Retrieves whether the current user can call all the
		// procedures returned by the method getProcedures.
		boolean	result=false;
		driver.debugPrintln("all procedures are callable: ",result);
		driver.debugEnd();
		return result;
	}

	public synchronized
	boolean allTablesAreSelectable() throws SQLException {
		driver.debugFunction(this);
		// Retrieves whether the current user can use all the tables
		// returned by the method getTables in a SELECT statement.
		boolean	result=false;
		driver.debugPrintln("all tables are selectable: ",result);
		driver.debugEnd();
		return result;
	}

	public synchronized
	boolean autoCommitFailureClosesAllResultSets() throws SQLException {
		driver.debugFunction(this);
		// Retrieves whether a SQLException while autoCommit is true
		// inidcates that all open ResultSets are closed, even ones
		// that are holdable.
		// FIXME: no idea if this is true or not
		boolean	result=false;
		driver.debugPrintln("auto commit failures closes "+
						"all result sets: ",result);
		driver.debugEnd();
		return result;
	}

	public synchronized
	boolean dataDefinitionCausesTransactionCommit() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	result=false;
		driver.debugPrintln("data definition causes "+
					"transaction commit: ",result);
		driver.debugEnd();
		return result;
	}

	public synchronized
	boolean dataDefinitionIgnoredInTransactions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	result=false;
		driver.debugPrintln("data definition ignored "+
					"in transactions: ",result);
		driver.debugEnd();
		return result;
	}

	public synchronized
	boolean deletesAreDetected(int type) throws SQLException {
		driver.debugFunction(this);
		// SQL Relay doesn't currenlty support ResultSet.RowDelete
		boolean	result=false;
		driver.debugPrintln("deletes are detected: ",result);
		driver.debugEnd();
		return result;
	}

	public synchronized
	boolean doesMaxRowSizeIncludeBlobs() throws SQLException {
		driver.debugFunction(this);
		boolean	result=false;
		driver.debugPrintln("does max row size include blobs: ",result);
		driver.debugEnd();
		return result;
	}

	public synchronized
	boolean generatedKeyAlwaysReturned() throws SQLException {
		driver.debugFunction(this);
		boolean	result=true;
		driver.debugPrintln("generated key always returned: ",result);
		driver.debugEnd();
		return result;
	}

	public synchronized
	ResultSet getAttributes(String catalog,
					String schemaPattern,
					String typeNamePattern,
					String attributeNamePattern)
					throws SQLException {
		driver.debugFunction(this);
		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema pattern: ",
						schemaPattern);
		driver.debugPrintln("type name pattern: ",
						typeNamePattern);
		driver.debugPrintln("attribute name pattern: ",
						attributeNamePattern);
		// FIXME: implement this somehow...
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public synchronized
	ResultSet getBestRowIdentifier(String catalog,
						String schema,
						String table,
						int scope,
						boolean nullable)
						throws SQLException {
		driver.debugFunction(this);
		driver.debugPrintln("schema: ",schema);
		driver.debugPrintln("table: ",table);
		driver.debugPrintln("scope: ",scope);
		driver.debugPrintln("nullable: ",nullable);
		// FIXME: implement this somehow...
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public synchronized
	ResultSet getCatalogs() throws SQLException {
		driver.debugFunction(this);

		SQLRelayResultSet	resultset=null;
		SQLRelayStatement	stmt=(SQLRelayStatement)
						connection.createStatement();
		SQLRCursor		sqlrcur=stmt.getSQLRCursor();

		boolean	result=sqlrcur.getDatabaseListWithFormat(null,3);

		if (result) {

			driver.debugPrintln("colcount: ",sqlrcur.colCount());

			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(driver);
				resultset.setStatement(stmt);
				resultset.setSQLRCursor(sqlrcur);
			}
		} else {
			throwErrorMessageException(sqlrcur);
		}
		
		driver.debugEnd();
		return resultset;
	}

	public synchronized
	String getCatalogSeparator() throws SQLException {
		driver.debugFunction(this);
		// FIXME: oracle uses @
		String	separator=".";
		driver.debugPrintln("catalog separator: ",separator);
		driver.debugEnd();
		return separator;
	}

	public synchronized
	String getCatalogTerm() throws SQLException {
		driver.debugFunction(this);
		// FIXME: I think SQL Server uses catalog, maybe sybase
		String	term="database";
		driver.debugPrintln("catalog term: ",term);
		driver.debugEnd();
		return term;
	}

	public synchronized
	ResultSet getClientInfoProperties() throws SQLException {
		driver.debugFunction(this);
		// FIXME: free form in SQL Relay
		driver.debugEnd();
		return null;
	}

	public synchronized
	ResultSet getColumnPrivileges(String catalog,
					String schema,
					String table,
					String columnNamePattern)
					throws SQLException {
		driver.debugFunction(this);
		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema: ",schema);
		driver.debugPrintln("table: ",table);
		driver.debugPrintln("column name pattern: ",columnNamePattern);
		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public synchronized
	ResultSet getColumns(String catalog,
					String schemaPattern,
					String tableNamePattern,
					String columnNamePattern)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema pattern: ",
						schemaPattern);
		driver.debugPrintln("table name pattern: ",
						tableNamePattern);
		driver.debugPrintln("column name pattern: ",
						columnNamePattern);

		String	wild=buildWild(catalog,schemaPattern,tableNamePattern);
		driver.debugPrintln("wild: ",wild);
		driver.debugPrintln("column name pattern: ",columnNamePattern);

		SQLRelayResultSet	resultset=null;
		SQLRelayStatement	stmt=(SQLRelayStatement)
						connection.createStatement();
		SQLRCursor		sqlrcur=stmt.getSQLRCursor();

		boolean	result=sqlrcur.getColumnListWithFormat(
						wild,columnNamePattern,3);

		if (result) {

			driver.debugPrintln("colcount: ",sqlrcur.colCount());

			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(driver);
				resultset.setStatement(stmt);
				resultset.setSQLRCursor(sqlrcur);
			}
		} else {
			throwErrorMessageException(sqlrcur);
		}
		
		driver.debugEnd();
		return resultset;
	}

	public synchronized
	Connection getConnection() throws SQLException {
		//driver.debugFunction(this);
		//driver.debugEnd();
		return connection;
	}

	public synchronized
	ResultSet getCrossReference(String parentCatalog,
					String parentSchema,
					String parentTable,
					String foreignCatalog,
					String foreignSchema,
					String foreignTable)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("parent catalog: ",parentCatalog);
		driver.debugPrintln("parent schema: ",parentSchema);
		driver.debugPrintln("parent table: ",parentTable);
		driver.debugPrintln("foreign catalog: ",foreignCatalog);
		driver.debugPrintln("foreign schema: ",foreignSchema);
		driver.debugPrintln("foreign table: ",foreignTable);

		// FIXME: implement this somehow...
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public synchronized
	int getDatabaseMajorVersion() throws SQLException {
		driver.debugFunction(this);
		int	majorversion=getDatabaseVersion(true);
		driver.debugPrintln("major version: ",majorversion);
		driver.debugEnd();
		return majorversion;
	}

	public synchronized
	int getDatabaseMinorVersion() throws SQLException {
		driver.debugFunction(this);
		int	minorversion=getDatabaseVersion(false);
		driver.debugPrintln("minor version: ",minorversion);
		driver.debugEnd();
		return minorversion;
	}

	private int getDatabaseVersion(boolean major) {
		// FIXME: cache/fetch dbVersion
		Matcher	matcher=Pattern.compile("[0-9]*\\.[0-9]*").
			matcher(connection.getSQLRConnection().dbVersion());
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

	public synchronized
	String getDatabaseProductName() throws SQLException {
		driver.debugFunction(this);
		// FIXME: cache/fetch identify
		String	id=connection.getSQLRConnection().identify();
		driver.debugPrintln("product name: ",id);
		driver.debugEnd();
		return id;
	}

	public synchronized
	String getDatabaseProductVersion() throws SQLException {
		driver.debugFunction(this);
		// FIXME: cache/fetch dbVersion
		String	productversion=
				connection.getSQLRConnection().dbVersion();
		driver.debugPrintln("product version: ",productversion);
		driver.debugEnd();
		return productversion;
	}

	public synchronized
	int getDefaultTransactionIsolation() throws SQLException {
		driver.debugFunction(this);
		int	isolation=(getDatabaseProductName().equals("mysql"))?
					Connection.TRANSACTION_REPEATABLE_READ:
					Connection.TRANSACTION_READ_COMMITTED;
		driver.debugPrintln("isolation: ",isolation);
		driver.debugEnd();
		return isolation;
	}

	public synchronized
	int getDriverMajorVersion() {
		driver.debugFunction(this);
		int		majorversion=-1;
		String[]	parts=connection.
					getSQLRConnection().
					clientVersion().split(".");
		if (parts!=null && parts.length>0) {
			majorversion=Integer.parseInt(parts[0]);
		}
		driver.debugPrintln("major version: ",majorversion);
		driver.debugEnd();
		return majorversion;
	}

	public synchronized
	int getDriverMinorVersion() {
		driver.debugFunction(this);
		int		minorversion=-1;
		String[]	parts=connection.
					getSQLRConnection().
					clientVersion().split(".");
		if (parts!=null && parts.length>1) {
			minorversion=Integer.parseInt(parts[1]);
		}
		driver.debugPrintln("minor version: ",minorversion);
		driver.debugEnd();
		return minorversion;
	}

	public synchronized
	String getDriverName() throws SQLException {
		driver.debugFunction(this);
		String	drivername="sqlrelay";
		driver.debugPrintln("driver name: ",drivername);
		driver.debugEnd();
		return drivername;
	}

	public synchronized
	String getDriverVersion() throws SQLException {
		driver.debugFunction(this);
		String	driverversion=connection.
					getSQLRConnection().
					clientVersion();
		driver.debugPrintln("driver version: ",driverversion);
		driver.debugEnd();
		return driverversion;
	}

	public synchronized
	ResultSet getExportedKeys(String catalog,
					String schema,
					String table)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema: ",schema);
		driver.debugPrintln("table: ",table);

		// Retrieves a description of the foreign key columns that
		// reference the given table's primary key columns (the foreign
		// keys exported by a table).
		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public synchronized
	String getExtraNameCharacters() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		String	extranamechars="#@";
		driver.debugPrintln("extra name characters: ",extranamechars);
		driver.debugEnd();
		return extranamechars;
	}

	public synchronized
	ResultSet getFunctionColumns(String catalog,
					String schemaPattern,
					String functionNamePattern,
					String columnNamePattern)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema pattern: ",
						schemaPattern);
		driver.debugPrintln("function name pattern: ",
						functionNamePattern);
		driver.debugPrintln("column name pattern: ",
						columnNamePattern);

		// FIXME: implement with
		driver.debugPrintln("FIXME: implement this");
		// sqlrcur.getProcedureBindAndColumnList()?
		driver.debugEnd();
		return null;
	}

	public synchronized
	ResultSet getFunctions(String catalog,
					String schemaPattern,
					String functionNamePattern)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema pattern: ",
						schemaPattern);
		driver.debugPrintln("function name pattern: ",
						functionNamePattern);

		// FIXME: implement this by calling sqlrcur.getProcedures()?
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public synchronized
	String getIdentifierQuoteString() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		// * sqlserver uses braces
		String	identifierquotestring=
			(getDatabaseProductName().equals("mysql"))?"`":"\"";
		driver.debugPrintln("identifier quote string: ",
					identifierquotestring);
		driver.debugEnd();
		return identifierquotestring;
	}

	public synchronized
	ResultSet getImportedKeys(String catalog,
					String schema,
					String table)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema: ",schema);
		driver.debugPrintln("table: ",table);

		// Retrieves a description of the primary key columns that are
		// referenced by the given table's foreign key columns (the
		// primary keys imported by a table).
		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public synchronized
	ResultSet getIndexInfo(String catalog,
					String schema,
					String table,
					boolean unique,
					boolean approximate)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema: ",schema);
		driver.debugPrintln("table: ",table);
		driver.debugPrintln("unique: ",unique);
		driver.debugPrintln("approximate: ",approximate);

		// FIXME: implement using sqlrcur.getKeyAndIndexList() ?
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public synchronized
	int getJDBCMajorVersion() throws SQLException {
		driver.debugFunction(this);
		// FIXME: get this from ???
		int	jdbcmajorversion=4;
		driver.debugPrintln("jdbc major version: ",jdbcmajorversion);
		driver.debugEnd();
		return jdbcmajorversion;
	}

	public synchronized
	int getJDBCMinorVersion() throws SQLException {
		driver.debugFunction(this);
		// FIXME: get this from ???
		int	jdbcminorversion=3;
		driver.debugPrintln("jdbc minor version: ",jdbcminorversion);
		driver.debugEnd();
		return jdbcminorversion;
	}

	public synchronized
	int getMaxBinaryLiteralLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxbinaryliterallength=0;
		driver.debugPrintln("max binary literal length: ",
						maxbinaryliterallength);
		driver.debugEnd();
		return maxbinaryliterallength;
	}

	public synchronized
	int getMaxCatalogNameLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcatalognamelength=0;
		driver.debugPrintln("max catalog name length: ",
						maxcatalognamelength);
		driver.debugEnd();
		return maxcatalognamelength;
	}

	public synchronized
	int getMaxCharLiteralLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcharliterallength=0;
		driver.debugPrintln("max char literal length: ",
						maxcharliterallength);
		driver.debugEnd();
		return maxcharliterallength;
	}

	public synchronized
	int getMaxColumnNameLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcolumnnamelength=0;
		driver.debugPrintln("max column name length: ",
						maxcolumnnamelength);
		driver.debugEnd();
		return maxcolumnnamelength;
	}

	public synchronized
	int getMaxColumnsInGroupBy() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcolumnsingroup=0;
		driver.debugPrintln("max columns in group: ",
						maxcolumnsingroup);
		driver.debugEnd();
		return maxcolumnsingroup;
	}

	public synchronized
	int getMaxColumnsInIndex() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcolumnsinindex=0;
		driver.debugPrintln("max columns in index: ",
						maxcolumnsinindex);
		driver.debugEnd();
		return maxcolumnsinindex;
	}

	public synchronized
	int getMaxColumnsInOrderBy() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcolumnsinorderby=0;
		driver.debugPrintln("max columns in order by: ",
						maxcolumnsinorderby);
		driver.debugEnd();
		return maxcolumnsinorderby;
	}

	public synchronized
	int getMaxColumnsInSelect() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcolumnsinselect=0;
		driver.debugPrintln("max columns in select: ",
						maxcolumnsinselect);
		driver.debugEnd();
		return maxcolumnsinselect;
	}

	public synchronized
	int getMaxColumnsInTable() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcolumnsintable=0;
		driver.debugPrintln("max columns in table: ",
						maxcolumnsintable);
		driver.debugEnd();
		return maxcolumnsintable;
	}

	public synchronized
	int getMaxConnections() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxconnections=0;
		driver.debugPrintln("max connections: ",maxconnections);
		driver.debugEnd();
		return maxconnections;
	}

	public synchronized
	int getMaxCursorNameLength() throws SQLException {
		driver.debugFunction(this);
		int	maxcursornamelength=0;
		driver.debugPrintln("max cursor name length: ",
						maxcursornamelength);
		driver.debugEnd();
		return maxcursornamelength;
	}

	public synchronized
	int getMaxIndexLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxindexlength=0;
		driver.debugPrintln("max index length: ",maxindexlength);
		driver.debugEnd();
		return maxindexlength;
	}

	public synchronized
	int getMaxProcedureNameLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxprocedurenamelength=0;
		driver.debugPrintln("max procedure name length: ",
						maxprocedurenamelength);
		driver.debugEnd();
		return maxprocedurenamelength;
	}

	public synchronized
	int getMaxRowSize() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxrowsize=0;
		driver.debugPrintln("max row size: ",maxrowsize);
		driver.debugEnd();
		return maxrowsize;
	}

	public synchronized
	int getMaxSchemaNameLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxschemanamelength=0;
		driver.debugPrintln("max schema name length: ",
						maxschemanamelength);
		driver.debugEnd();
		return maxschemanamelength;
	}

	public synchronized
	int getMaxStatementLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxstatementlength=0;
		driver.debugPrintln("max statement length: ",
						maxstatementlength);
		driver.debugEnd();
		return maxstatementlength;
	}

	public synchronized
	int getMaxStatements() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxstatements=0;
		driver.debugPrintln("max statements: ",maxstatements);
		driver.debugEnd();
		return maxstatements;
	}

	public synchronized
	int getMaxTableNameLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxtablenamelength=0;
		driver.debugPrintln("max table name length: ",
						maxtablenamelength);
		driver.debugEnd();
		return maxtablenamelength;
	}

	public synchronized
	int getMaxTablesInSelect() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxtablesinselect=0;
		driver.debugPrintln("max tables in select: ",
						maxtablesinselect);
		driver.debugEnd();
		return maxtablesinselect;
	}

	public synchronized
	int getMaxUserNameLength() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxusernamelength=0;
		driver.debugPrintln("max user name length: ",maxusernamelength);
		driver.debugEnd();
		return maxusernamelength;
	}

	public synchronized
	String getNumericFunctions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		String	numericfunctions=null;
		driver.debugPrintln("numeric functions: ",numericfunctions);
		driver.debugEnd();
		return numericfunctions;
	}

	public synchronized
	ResultSet getPrimaryKeys(String catalog,
					String schema,
					String table)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema: ",schema);
		driver.debugPrintln("table: ",table);

		// FIXME: implement this by calling sqlrcon.getPrimaryKeysList()
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public synchronized
	ResultSet getProcedureColumns(String catalog,
					String schemaPattern,
					String procedureNamePattern,
					String columnNamePattern)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema pattern: ",
						schemaPattern);
		driver.debugPrintln("procedure name pattern: ",
						procedureNamePattern);
		driver.debugPrintln("column name pattern: ",
						columnNamePattern);

		// FIXME: implement this by calling
		driver.debugPrintln("FIXME: implement this");
		// sqlrcon.getProcedureBindAndColumnList()
		driver.debugEnd();
		return null;
	}

	public synchronized
	ResultSet getProcedures(String catalog,
					String schemaPattern,
					String procedureNamePattern)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema pattern: ",
						schemaPattern);
		driver.debugPrintln("procedure name pattern: ",
						procedureNamePattern);

		// FIXME: implement this by calling sqlrcon.getProcedureList()
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public synchronized
	String getProcedureTerm() throws SQLException {
		driver.debugFunction(this);
		String	procedureterm="procedure";
		driver.debugPrintln("procedure term: ",procedureterm);
		driver.debugEnd();
		return procedureterm;
	}

	public synchronized
	ResultSet getPseudoColumns(String catalog,
					String schemaPattern,
					String tableNamePattern,
					String columnNamePattern)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema pattern: ",schemaPattern);
		driver.debugPrintln("table name pattern: ",tableNamePattern);
		driver.debugPrintln("column name pattern: ",columnNamePattern);

		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public synchronized
	int getResultSetHoldability() throws SQLException {
		driver.debugFunction(this);
		// FIXME: is this correct?
		int	resultsetholdability=ResultSet.CLOSE_CURSORS_AT_COMMIT;
		driver.debugPrintln("result set holdability: ",
						resultsetholdability);
		driver.debugEnd();
		return resultsetholdability;
	}

	public synchronized
	RowIdLifetime getRowIdLifetime() throws SQLException {
		driver.debugFunction(this);
		// FIXME: some dbs do support rowid
		RowIdLifetime	rowidlifetime=RowIdLifetime.ROWID_UNSUPPORTED;
		switch (rowidlifetime) {
			case ROWID_UNSUPPORTED:
				driver.debugPrintln("rowid lifetime: ",
							"ROWID_UNSUPPORTED");
				break;
			case ROWID_VALID_OTHER:
				driver.debugPrintln("rowid lifetime: ",
							"ROWID_VALID_OTHER");
				break;
			case ROWID_VALID_TRANSACTION:
				driver.debugPrintln("rowid lifetime: ",
						"ROWID_VALID_TRANSACTION");
				break;
			case ROWID_VALID_SESSION:
				driver.debugPrintln("rowid lifetime: ",
							"ROWID_VALID_SESSION");
				break;
			case ROWID_VALID_FOREVER:
				driver.debugPrintln("rowid lifetime: ",
							"ROWID_VALID_FOREVER");
				break;
		}
		driver.debugEnd();
		return rowidlifetime;
	}

	public synchronized
	ResultSet getSchemas() throws SQLException {
		driver.debugFunction(this);
		ResultSet	schemas=getSchemas(null,null);
		driver.debugEnd();
		return schemas;
	}

	public synchronized
	ResultSet getSchemas(String catalog,
					String schemaPattern)
					throws SQLException {
		driver.debugFunction(this);

		// FIXME: use catalog
		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema pattern: ",schemaPattern);

		SQLRelayResultSet	resultset=null;
		SQLRelayStatement	stmt=(SQLRelayStatement)
						connection.createStatement();
		SQLRCursor		sqlrcur=stmt.getSQLRCursor();

		boolean	result=sqlrcur.getSchemaListWithFormat(schemaPattern,3);

		if (result) {

			driver.debugPrintln("colcount: ",sqlrcur.colCount());

			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(driver);
				resultset.setStatement(stmt);
				resultset.setSQLRCursor(sqlrcur);
			}
		} else {
			throwErrorMessageException(sqlrcur);
		}
		
		driver.debugEnd();
		return resultset;
	}

	public synchronized
	String getSchemaTerm() throws SQLException {
		driver.debugFunction(this);
		String	schematerm="schema";
		driver.debugPrintln("schema term: ",schematerm);
		driver.debugEnd();
		return schematerm;
	}

	public synchronized
	String getSearchStringEscape() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		String	searchstringescape="\\";
		driver.debugPrintln("search string escape: ",
						searchstringescape);
		driver.debugEnd();
		return searchstringescape;
	}

	public synchronized
	String getSQLKeywords() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		String	sqlkeywords=null;
		driver.debugPrintln("sql keywords: ",sqlkeywords);
		driver.debugEnd();
		return sqlkeywords;
	}

	public synchronized
	int getSQLStateType() throws SQLException {
		driver.debugFunction(this);
		// FIXME: no idea
		int	sqlstatetype=sqlStateSQL;
		driver.debugPrintln("sql state type: ",sqlstatetype);
		driver.debugEnd();
		return sqlstatetype;
	}

	public synchronized
	String getStringFunctions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		String	stringfunctions=null;
		driver.debugPrintln("string functions: ",stringfunctions);
		driver.debugEnd();
		return stringfunctions;
	}

	public synchronized
	ResultSet getSuperTables(String catalog,
					String schemaPattern,
					String tableNamePattern)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema pattern: ",schemaPattern);
		driver.debugPrintln("table name pattern: ",tableNamePattern);

		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public synchronized
	ResultSet getSuperTypes(String catalog,
					String schemaPattern,
					String typeNamePattern)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema pattern: ",schemaPattern);
		driver.debugPrintln("type name pattern: ",typeNamePattern);

		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public synchronized
	String getSystemFunctions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		String	systemfunctions=null;
		driver.debugEnd();
		return systemfunctions;
	}

	public synchronized
	ResultSet getTablePrivileges(String catalog,
					String schemaPattern,
					String tableNamePattern)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema pattern: ",schemaPattern);
		driver.debugPrintln("table name pattern: ",tableNamePattern);

		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public synchronized
	ResultSet getTables(String catalog,
				String schemaPattern,
				String tableNamePattern,
				String[] types)
				throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema pattern: ",schemaPattern);
		driver.debugPrintln("table name pattern: ",tableNamePattern);

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
			driver.debugPrintln("types: ",t);
		}

		String	wild=buildWild(catalog,schemaPattern,tableNamePattern);
		driver.debugPrintln("wild: ",wild);

		SQLRelayResultSet	resultset=null;
		SQLRelayStatement	stmt=(SQLRelayStatement)
						connection.createStatement();
		SQLRCursor		sqlrcur=stmt.getSQLRCursor();

		boolean	result=sqlrcur.getTableListWithFormat(
						tableNamePattern,3,objecttypes);

		if (result) {

			driver.debugPrintln("colcount: ",sqlrcur.colCount());

			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(driver);
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

		// If object already contains a . then just use it
		// as-is.
		if (object.contains(".")) {
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

	public synchronized
	ResultSet getTableTypes() throws SQLException {
		driver.debugFunction(this);
		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public synchronized
	String getTimeDateFunctions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		String	timedatefunctions=null;
		driver.debugPrintln("timedate functions: ",timedatefunctions);
		driver.debugEnd();
		return timedatefunctions;
	}

	public synchronized
	ResultSet getTypeInfo() throws SQLException {
		driver.debugFunction(this);
		// FIXME: implement this by calling sqlrcur.getTypeInfoList()
		// TYPE_NAME
		//	String
		//	Name of the data type.
		// DATA_TYPE
		//	int
		//	Integer value representing this datatype.
		// PRECISION
		//	int
		//	Maximum precision of this datatype.
		// LITERAL_PREFIX
		//	String
		//	Prefix used to quote a string literal.
		// LITERAL_SUFFIX
		//	String
		//	suffix used to quote a string literal.
		// CASE_SENSITIVE
		//	boolean
		//	Determines whether this datatype is case sensitive
		// UNSIGNED_ATTRIBUTE
		//	boolean
		//	Determines whether this datatype is an un-signed
		//	attribute.
		// FIXED_PREC_SCALE
		//	boolean
		//	Determines whether the current datatype can be used as
		//	a value of currency.
		// AUTO_INCREMENT
		//	boolean
		//	Determines whether the current datatype can be used for
		//	auto-increment.
		// LOCAL_TYPE_NAME
		//	String
		//	Localized version of this datatype.
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public synchronized
	ResultSet getUDTs(String catalog,
				String schemaPattern,
				String typeNamePattern,
				int[] types)
				throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema pattern: ",schemaPattern);
		driver.debugPrintln("type name pattern: ",typeNamePattern);
		// FIXME: debug types...

		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public synchronized
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

		driver.debugPrintln("url: ",url);

		driver.debugEnd();
		return url;
	}

	public synchronized
	String getUserName() throws SQLException {
		driver.debugFunction(this);
		String	username=connection.getUser();
		driver.debugPrintln("user name: ",username);
		driver.debugEnd();
		return username;
	}

	public synchronized
	ResultSet getVersionColumns(String catalog,
					String schema,
					String table)
					throws SQLException {
		driver.debugFunction(this);

		driver.debugPrintln("catalog: ",catalog);
		driver.debugPrintln("schema: ",schema);
		driver.debugPrintln("table: ",table);

		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugEnd();
		return null;
	}

	public synchronized
	boolean insertsAreDetected(int type) throws SQLException {
		driver.debugFunction(this);
		boolean	insertsaredetected=false;
		driver.debugPrintln("type: ",type);
		driver.debugPrintln("inserts are detected: ",
						insertsaredetected);
		driver.debugEnd();
		return insertsaredetected;
	}

	public synchronized
	boolean isCatalogAtStart() throws SQLException {
		driver.debugFunction(this);
		// FIXME: not in oracle
		boolean	iscatalogatstart=true;
		driver.debugPrintln("is catalog at start: ",iscatalogatstart);
		driver.debugEnd();
		return iscatalogatstart;
	}

	public synchronized
	boolean isReadOnly() throws SQLException {
		driver.debugFunction(this);
		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		boolean	isreadonly=false;
		driver.debugPrintln("is read only: ",isreadonly);
		driver.debugEnd();
		return isreadonly;
	}

	public synchronized
	boolean locatorsUpdateCopy() throws SQLException {
		driver.debugFunction(this);
		// FIXME: no idea, probably db-specific
		boolean	locatorsupdatecopy=false;
		driver.debugPrintln("locators update copy: ",
						locatorsupdatecopy);
		driver.debugEnd();
		return locatorsupdatecopy;
	}

	public synchronized
	boolean nullPlusNonNullIsNull() throws SQLException {
		driver.debugFunction(this);
		// FIXME: generally true, but probably db-specific
		boolean	nullplusnonnullisnull=true;
		driver.debugPrintln("null plus non null is null: ",
						nullplusnonnullisnull);
		driver.debugEnd();
		return nullplusnonnullisnull;
	}

	public synchronized
	boolean nullsAreSortedAtEnd() throws SQLException {
		driver.debugFunction(this);
		// FIXME: generally true, but probably db-specific
		boolean	nullsaresortedatend=true;
		driver.debugPrintln("nulls are sorted at end: ",
						nullsaresortedatend);
		driver.debugEnd();
		return nullsaresortedatend;
	}

	public synchronized
	boolean nullsAreSortedAtStart() throws SQLException {
		driver.debugFunction(this);
		// FIXME: generally false, but probably db-specific
		boolean	nullsaresortedatstart=false;
		driver.debugPrintln("nulls are sorted at start: ",
						nullsaresortedatstart);
		driver.debugEnd();
		return nullsaresortedatstart;
	}

	public synchronized
	boolean nullsAreSortedHigh() throws SQLException {
		driver.debugFunction(this);
		// FIXME: generally true, but probably db-specific
		boolean	nullsaresortedhigh=true;
		driver.debugPrintln("nulls are sorted high: ",
						nullsaresortedhigh);
		driver.debugEnd();
		return nullsaresortedhigh;
	}

	public synchronized
	boolean nullsAreSortedLow() throws SQLException {
		driver.debugFunction(this);
		// FIXME: generally false, but probably db-specific
		boolean	nullsaresortedlow=false;
		driver.debugPrintln("nulls are sorted low: ",
						nullsaresortedlow);
		driver.debugEnd();
		return nullsaresortedlow;
	}

	public synchronized
	boolean othersDeletesAreVisible(int type) throws SQLException {
		driver.debugFunction(this);
		boolean	othersdeletesarevisible=false;
		driver.debugPrintln("others deletes are visible: ",
						othersdeletesarevisible);
		driver.debugEnd();
		return othersdeletesarevisible;
	}

	public synchronized
	boolean othersInsertsAreVisible(int type) throws SQLException {
		driver.debugFunction(this);
		boolean	othersinsertssarevisible=false;
		driver.debugPrintln("others inserts are visible: ",
						othersinsertssarevisible);
		driver.debugEnd();
		return othersinsertssarevisible;
	}

	public synchronized
	boolean othersUpdatesAreVisible(int type) throws SQLException {
		driver.debugFunction(this);
		boolean	othersupdatessarevisible=false;
		driver.debugPrintln("others updates are visible: ",
						othersupdatessarevisible);
		driver.debugEnd();
		return othersupdatessarevisible;
	}

	public synchronized
	boolean ownDeletesAreVisible(int type) throws SQLException {
		driver.debugFunction(this);
		boolean	owndeletesarevisible=false;
		driver.debugPrintln("own deletes are visible: ",
						owndeletesarevisible);
		driver.debugEnd();
		return owndeletesarevisible;
	}

	public synchronized
	boolean ownInsertsAreVisible(int type) throws SQLException {
		driver.debugFunction(this);
		boolean	owninsertsarevisible=false;
		driver.debugPrintln("own inserts are visible: ",
						owninsertsarevisible);
		driver.debugEnd();
		return owninsertsarevisible;
	}

	public synchronized
	boolean ownUpdatesAreVisible(int type) throws SQLException {
		driver.debugFunction(this);
		boolean	ownupdatesarevisible=false;
		driver.debugPrintln("own updates are visible: ",
						ownupdatesarevisible);
		driver.debugEnd();
		return ownupdatesarevisible;
	}

	public synchronized
	boolean storesLowerCaseIdentifiers() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific but generally false
		// oracle stores upper case identifiers
		// other db's store mixed case identifiers
		boolean	storeslowercaseidentifiers=false;
		driver.debugPrintln("stores lower case identifiers: ",
						storeslowercaseidentifiers);
		driver.debugEnd();
		return storeslowercaseidentifiers;
	}

	public synchronized
	boolean storesLowerCaseQuotedIdentifiers() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	storeslowercasequotedidentifiers=false;
		driver.debugPrintln("stores lower case quoted identifiers: ",
					storeslowercasequotedidentifiers);
		driver.debugEnd();
		return storeslowercasequotedidentifiers;
	}

	public synchronized
	boolean storesMixedCaseIdentifiers() throws SQLException {
		driver.debugFunction(this);
		// FIXME: generally true, but db-specific, false for oracle
		boolean	storesmixedcaseidentifiers=true;
		driver.debugPrintln("stores mixed case identifiers: ",
						storesmixedcaseidentifiers);
		driver.debugEnd();
		return storesmixedcaseidentifiers;
	}

	public synchronized
	boolean storesMixedCaseQuotedIdentifiers() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	storesmixedcasequotedidentifiers=true;
		driver.debugPrintln("stores mixed case quoted identifiers: ",
					storesmixedcasequotedidentifiers);
		driver.debugEnd();
		return storesmixedcasequotedidentifiers;
	}

	public synchronized
	boolean storesUpperCaseIdentifiers() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific but generally false
		// oracle stores upper case identifiers
		// other db's store mixed case identifiers
		boolean	storesuppercaseidentifiers=false;
		driver.debugPrintln("stores upper case identifiers: ",
						storesuppercaseidentifiers);
		driver.debugEnd();
		return storesuppercaseidentifiers;
	}

	public synchronized
	boolean storesUpperCaseQuotedIdentifiers() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	storesuppercasequotedidentifiers=true;
		driver.debugPrintln("stores upper case quoted identifiers: ",
					storesuppercasequotedidentifiers);
		driver.debugEnd();
		return storesuppercasequotedidentifiers;
	}

	public synchronized
	boolean supportsAlterTableWithAddColumn() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsaltertablewithaddcommand=true;
		driver.debugPrintln("supports alter table with add command: ",
					supportsaltertablewithaddcommand);
		driver.debugEnd();
		return supportsaltertablewithaddcommand;
	}

	public synchronized
	boolean supportsAlterTableWithDropColumn() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsaltertablewithdropcommand=true;
		driver.debugPrintln("supports alter table with drop command: ",
					supportsaltertablewithdropcommand);
		driver.debugEnd();
		return supportsaltertablewithdropcommand;
	}

	public synchronized
	boolean supportsANSI92EntryLevelSQL() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsansi92entrylevelsql=true;
		driver.debugPrintln("supports ansi92 entry level sql: ",
						supportsansi92entrylevelsql);
		driver.debugEnd();
		return supportsansi92entrylevelsql;
	}

	public synchronized
	boolean supportsANSI92FullSQL() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsansi92fullsql=true;
		driver.debugPrintln("supports ansi92 full sql: ",
						supportsansi92fullsql);
		driver.debugEnd();
		return supportsansi92fullsql;
	}

	public synchronized
	boolean supportsANSI92IntermediateSQL() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsansi92intermediatesql=true;
		driver.debugPrintln("supports ansi92 intermediate sql: ",
						supportsansi92intermediatesql);
		driver.debugEnd();
		return supportsansi92intermediatesql;
	}

	public synchronized
	boolean supportsBatchUpdates() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsbatchupdates=false;
		driver.debugPrintln("supports batch updates: ",
						supportsbatchupdates);
		driver.debugEnd();
		return supportsbatchupdates;
	}

	public synchronized
	boolean supportsCatalogsInDataManipulation() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscatalogsindatamanipulation=true;
		driver.debugPrintln("supports catalogs in data manipulations: ",
					supportscatalogsindatamanipulation);
		driver.debugEnd();
		return supportscatalogsindatamanipulation;
	}

	public synchronized
	boolean supportsCatalogsInIndexDefinitions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscatalogsinindexdefinitions=true;
		driver.debugPrintln("supports catalogs in index definitions: ",
					supportscatalogsinindexdefinitions);
		driver.debugEnd();
		return supportscatalogsinindexdefinitions;
	}

	public synchronized
	boolean supportsCatalogsInPrivilegeDefinitions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscatalogsinprivilegedefinitions=true;
		driver.debugPrintln("supports catalogs in "+
					"privilege definitions: ",
					supportscatalogsinprivilegedefinitions);
		driver.debugEnd();
		return supportscatalogsinprivilegedefinitions;
	}

	public synchronized
	boolean supportsCatalogsInProcedureCalls() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscatalogsinprocedurecalls=true;
		driver.debugPrintln("supports catalogs in procedure calls: ",
					supportscatalogsinprocedurecalls);
		driver.debugEnd();
		return supportscatalogsinprocedurecalls;
	}

	public synchronized
	boolean supportsCatalogsInTableDefinitions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscatalogsintabledefinitions=true;
		driver.debugPrintln("supports catalogs in table definitions: ",
					supportscatalogsintabledefinitions);
		driver.debugEnd();
		return supportscatalogsintabledefinitions;
	}

	public synchronized
	boolean supportsColumnAliasing() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscolumnaliasing=true;
		driver.debugPrintln("supports column aliasing: ",
					supportscolumnaliasing);
		driver.debugEnd();
		return supportscolumnaliasing;
	}

	public synchronized
	boolean supportsConvert() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsconvert=true;
		driver.debugPrintln("supports convert: ",supportsconvert);
		driver.debugEnd();
		return supportsconvert;
	}

	public synchronized
	boolean supportsConvert(int fromType, int toType) throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-and-type-specific
		boolean	supportsconvert=true;
		driver.debugPrintln("from type: ",fromType);
		driver.debugPrintln("to type: ",toType);
		driver.debugPrintln("supports convert: ",supportsconvert);
		driver.debugEnd();
		return supportsconvert;
	}

	public synchronized
	boolean supportsCoreSQLGrammar() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscoresqlgrammar=true;
		driver.debugPrintln("supports core sql grammar: ",
						supportscoresqlgrammar);
		driver.debugEnd();
		return supportscoresqlgrammar;
	}

	public synchronized
	boolean supportsCorrelatedSubqueries() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscorrelatedsubqueries=true;
		driver.debugPrintln("supports correlated subqueries: ",
						supportscorrelatedsubqueries);
		driver.debugEnd();
		return supportscorrelatedsubqueries;
	}

	public synchronized
	boolean supportsDataDefinitionAndDataManipulationTransactions()
							throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	sddadmt=true;
		driver.debugPrintln("supports data definition ",
					"and data manipulation transactions: ",
					sddadmt);
		driver.debugEnd();
		return sddadmt;
	}

	public synchronized
	boolean supportsDataManipulationTransactionsOnly() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	sdmto=false;
		driver.debugPrintln("supports data manipulation ",
					"transactions only: ",sdmto);
		driver.debugEnd();
		return sdmto;
	}

	public synchronized
	boolean supportsDifferentTableCorrelationNames() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	sdtcn=true;
		driver.debugPrintln("supports different table ",
					"correlation names: ",sdtcn);
		driver.debugEnd();
		return sdtcn;
	}

	public synchronized
	boolean supportsExpressionsInOrderBy() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsexpressionsinorderby=true;
		driver.debugPrintln("supports expressions in order by: ",
						supportsexpressionsinorderby);
		driver.debugEnd();
		return supportsexpressionsinorderby;
	}

	public synchronized
	boolean supportsExtendedSQLGrammar() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsextendedsqlgrammar=true;
		driver.debugPrintln("supports extended sql grammar: ",
						supportsextendedsqlgrammar);
		driver.debugEnd();
		return supportsextendedsqlgrammar;
	}

	public synchronized
	boolean supportsFullOuterJoins() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsfullouterjoins=true;
		driver.debugPrintln("supports full outer joins: ",
						supportsfullouterjoins);
		driver.debugEnd();
		return supportsfullouterjoins;
	}

	public synchronized
	boolean supportsGetGeneratedKeys() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsgetgeneratedkeys=true;
		driver.debugPrintln("supports get generated keys: ",
						supportsgetgeneratedkeys);
		driver.debugEnd();
		return supportsgetgeneratedkeys;
	}

	public synchronized
	boolean supportsGroupBy() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsgroupby=true;
		driver.debugPrintln("supports group by: ",supportsgroupby);
		driver.debugEnd();
		return supportsgroupby;
	}

	public synchronized
	boolean supportsGroupByBeyondSelect() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsgroupbybeyondselect=true;
		driver.debugPrintln("supports group by beyond select: ",
						supportsgroupbybeyondselect);
		driver.debugEnd();
		return supportsgroupbybeyondselect;
	}

	public synchronized
	boolean supportsGroupByUnrelated() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsgroupbyunrelated=true;
		driver.debugPrintln("supports group by unrelated: ",
						supportsgroupbyunrelated);
		driver.debugEnd();
		return supportsgroupbyunrelated;
	}

	public synchronized
	boolean supportsIntegrityEnhancementFacility() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsintegrityenhancementfacility=false;
		driver.debugPrintln("supports integrity enhancement facility: ",
					supportsintegrityenhancementfacility);
		driver.debugEnd();
		return supportsintegrityenhancementfacility;
	}

	public synchronized
	boolean supportsLikeEscapeClause() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportslikeescapeclause=true;
		driver.debugPrintln("supports like escape clause: ",
						supportslikeescapeclause);
		driver.debugEnd();
		return supportslikeescapeclause;
	}

	public synchronized
	boolean supportsLimitedOuterJoins() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportslimitedouterjoins=true;
		driver.debugPrintln("supports limited outer joins: ",
						supportslimitedouterjoins);
		driver.debugEnd();
		return supportslimitedouterjoins;
	}

	public synchronized
	boolean supportsMinimumSQLGrammar() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsminimumsqlgrammar=true;
		driver.debugPrintln("supports minimum sql grammar: ",
						supportsminimumsqlgrammar);
		driver.debugEnd();
		return supportsminimumsqlgrammar;
	}

	public synchronized
	boolean supportsMixedCaseIdentifiers() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific, oracle doesn't
		boolean	supportsmixedcaseidentifiers=true;
		driver.debugPrintln("supports mixed case identifiers: ",
						supportsmixedcaseidentifiers);
		driver.debugEnd();
		return supportsmixedcaseidentifiers;
	}

	public synchronized
	boolean supportsMixedCaseQuotedIdentifiers() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsmixedcasequotedidentifiers=true;
		driver.debugPrintln("supports mixed case quoted identifiers: ",
					supportsmixedcasequotedidentifiers);
		driver.debugEnd();
		return supportsmixedcasequotedidentifiers;
	}

	public synchronized
	boolean supportsMultipleOpenResults() throws SQLException {
		driver.debugFunction(this);
		boolean	supportsmultipleopenresults=true;
		driver.debugPrintln("supports multiple open results: ",
						supportsmultipleopenresults);
		driver.debugEnd();
		return supportsmultipleopenresults;
	}

	public synchronized
	boolean supportsMultipleResultSets() throws SQLException {
		driver.debugFunction(this);
		// FIXME: in progress...
		boolean	supportsmultipleresultsets=false;
		driver.debugPrintln("supports multiple result sets: ",
						supportsmultipleresultsets);
		driver.debugEnd();
		return supportsmultipleresultsets;
	}

	public synchronized
	boolean supportsMultipleTransactions() throws SQLException {
		driver.debugFunction(this);
		boolean	supportsmultipletransactions=false;
		driver.debugPrintln("supports multiple transactions: ",
						supportsmultipletransactions);
		driver.debugEnd();
		return supportsmultipletransactions;
	}

	public synchronized
	boolean supportsNamedParameters() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsnamedparameters=true;
		driver.debugPrintln("supports named parameters: ",
						supportsnamedparameters);
		driver.debugEnd();
		return supportsnamedparameters;
	}

	public synchronized
	boolean supportsNonNullableColumns() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsnonnullablecolumns=true;
		driver.debugPrintln("supports non-nullable columns: ",
						supportsnonnullablecolumns);
		driver.debugEnd();
		return supportsnonnullablecolumns;
	}

	public synchronized
	boolean supportsOpenCursorsAcrossCommit() throws SQLException {
		driver.debugFunction(this);
		// FIXME: not sure
		boolean	supportsopencursorsacrosscommit=false;
		driver.debugPrintln("supports open cursors across commit: ",
					supportsopencursorsacrosscommit);
		driver.debugEnd();
		return supportsopencursorsacrosscommit;
	}

	public synchronized
	boolean supportsOpenCursorsAcrossRollback() throws SQLException {
		driver.debugFunction(this);
		// FIXME: not sure
		boolean	supportsopencursorsacrossrollback=false;
		driver.debugPrintln("supports open cursors across rollback: ",
					supportsopencursorsacrossrollback);
		driver.debugEnd();
		return supportsopencursorsacrossrollback;
	}

	public synchronized
	boolean supportsOpenStatementsAcrossCommit() throws SQLException {
		driver.debugFunction(this);
		// FIXME: not sure
		boolean	supportsopenstatementsacrosscommit=false;
		driver.debugPrintln("supports open statements across commit: ",
					supportsopenstatementsacrosscommit);
		driver.debugEnd();
		return supportsopenstatementsacrosscommit;
	}

	public synchronized
	boolean supportsOpenStatementsAcrossRollback() throws SQLException {
		driver.debugFunction(this);
		// FIXME: not sure
		boolean	supportsopenstatementsacrossrollback=false;
		driver.debugPrintln("supports open statements "+
					"across rollback: ",
					supportsopenstatementsacrossrollback);
		driver.debugEnd();
		return supportsopenstatementsacrossrollback;
	}

	public synchronized
	boolean supportsOrderByUnrelated() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsorderbyunrelated=true;
		driver.debugPrintln("supports order by unrelated: ",
					supportsorderbyunrelated);
		driver.debugEnd();
		return supportsorderbyunrelated;
	}

	public synchronized
	boolean supportsOuterJoins() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsouterjoins=true;
		driver.debugPrintln("supports outer joins: ",
						supportsouterjoins);
		driver.debugEnd();
		return supportsouterjoins;
	}

	public synchronized
	boolean supportsPositionedDelete() throws SQLException {
		driver.debugFunction(this);
		boolean	supportspositioneddelete=false;
		driver.debugPrintln("supports positioned delete: ",
						supportspositioneddelete);
		driver.debugEnd();
		return supportspositioneddelete;
	}

	public synchronized
	boolean supportsPositionedUpdate() throws SQLException {
		driver.debugFunction(this);
		boolean	supportspositionedupdate=false;
		driver.debugPrintln("supports positioned update: ",
						supportspositionedupdate);
		driver.debugEnd();
		return supportspositionedupdate;
	}

	public synchronized
	boolean supportsResultSetConcurrency(int type,
						int concurrency)
						throws SQLException {
		driver.debugFunction(this);
		boolean	supportsresultsetconcurrency=
				(type==ResultSet.TYPE_FORWARD_ONLY &&
				concurrency==ResultSet.CONCUR_READ_ONLY);
		driver.debugPrintln("supports result set concurrency: ",
						supportsresultsetconcurrency);
		driver.debugEnd();
		return supportsresultsetconcurrency;
	}

	public synchronized
	boolean supportsResultSetHoldability(int holdability)
							throws SQLException {
		driver.debugFunction(this);
		boolean	supportsresultsetholdability=
			(holdability==ResultSet.CLOSE_CURSORS_AT_COMMIT);
		driver.debugPrintln("supports result set holdability: ",
						supportsresultsetholdability);
		driver.debugEnd();
		return supportsresultsetholdability;
	}

	public synchronized
	boolean supportsResultSetType(int type) throws SQLException {
		driver.debugFunction(this);
		boolean	supportsresultsettype=
			(type==ResultSet.TYPE_FORWARD_ONLY);
		driver.debugPrintln("supports result set type: ",
						supportsresultsettype);
		driver.debugEnd();
		return supportsresultsettype;
	}

	public synchronized
	boolean supportsSavepoints() throws SQLException {
		driver.debugFunction(this);
		boolean	supportssavepoints=false;
		driver.debugPrintln("supports savepoints: ",supportssavepoints);
		driver.debugEnd();
		return supportssavepoints;
	}

	public synchronized
	boolean supportsSchemasInDataManipulation() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsschemasindatamanipulation=true;
		driver.debugPrintln("supports schemas in data manipulation: ",
					supportsschemasindatamanipulation);
		driver.debugEnd();
		return supportsschemasindatamanipulation;
	}

	public synchronized
	boolean supportsSchemasInIndexDefinitions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsschemasinindexdefinitions=true;
		driver.debugPrintln("supports schemas in index definitions: ",
					supportsschemasinindexdefinitions);
		driver.debugEnd();
		return supportsschemasinindexdefinitions;
	}

	public synchronized
	boolean supportsSchemasInPrivilegeDefinitions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsschemasinprivilegedefinitions=true;
		driver.debugPrintln("supports schemas in "+
					"privilege definitions: ",
					supportsschemasinprivilegedefinitions);
		driver.debugEnd();
		return supportsschemasinprivilegedefinitions;
	}

	public synchronized
	boolean supportsSchemasInProcedureCalls() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsschemasinprocedurecalls=true;
		driver.debugPrintln("supports schemas in procedure calls: ",
					supportsschemasinprocedurecalls);
		driver.debugEnd();
		return supportsschemasinprocedurecalls;
	}

	public synchronized
	boolean supportsSchemasInTableDefinitions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsschemasintabledefinitions=true;
		driver.debugPrintln("supports schemas in table definitions: ",
					supportsschemasintabledefinitions);
		driver.debugEnd();
		return supportsschemasintabledefinitions;
	}

	public synchronized
	boolean supportsSelectForUpdate() throws SQLException {
		driver.debugFunction(this);
		boolean	supportsselectforupdate=false;
		driver.debugPrintln("supports select for update: ",
						supportsselectforupdate);
		driver.debugEnd();
		return supportsselectforupdate;
	}

	public synchronized
	boolean supportsStatementPooling() throws SQLException {
		driver.debugFunction(this);
		boolean	supportsstatementpooling=false;
		driver.debugPrintln("supports statement pooling: ",
						supportsstatementpooling);
		driver.debugEnd();
		return supportsstatementpooling;
	}

	public synchronized
	boolean supportsStoredFunctionsUsingCallSyntax() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	ssfucs=false;
		driver.debugPrintln("supports stored functions "+
					"using call syntax: ",ssfucs);
		driver.debugEnd();
		return ssfucs;
	}

	public synchronized
	boolean supportsStoredProcedures() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsstoredprocedures=true;
		driver.debugPrintln("supports stored procedures: ",
						supportsstoredprocedures);
		driver.debugEnd();
		return supportsstoredprocedures;
	}

	public synchronized
	boolean supportsSubqueriesInComparisons() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportssubqueriesincomparisons=true;
		driver.debugPrintln("supports subqueries in comparisons: ",
					supportssubqueriesincomparisons);
		driver.debugEnd();
		return supportssubqueriesincomparisons;
	}

	public synchronized
	boolean supportsSubqueriesInExists() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportssubqueriesinexists=true;
		driver.debugPrintln("supports subqueries in exists: ",
						supportssubqueriesinexists);
		driver.debugEnd();
		return supportssubqueriesinexists;
	}

	public synchronized
	boolean supportsSubqueriesInIns() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportssubqueriesinins=true;
		driver.debugPrintln("supports subqueries in ins: ",
						supportssubqueriesinins);
		driver.debugEnd();
		return supportssubqueriesinins;
	}

	public synchronized
	boolean supportsSubqueriesInQuantifieds() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportssubqueriesinquantifieds=true;
		driver.debugPrintln("supports subqueries in quantifieds: ",
					supportssubqueriesinquantifieds);
		driver.debugEnd();
		return supportssubqueriesinquantifieds;
	}

	public synchronized
	boolean supportsTableCorrelationNames() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportssubqueriesincorrelationnames=true;
		driver.debugPrintln("supports subqueries in "+
					"correlation names: ",
					supportssubqueriesincorrelationnames);
		driver.debugEnd();
		return supportssubqueriesincorrelationnames;
	}

	public synchronized
	boolean supportsTransactionIsolationLevel(int level)
						throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportstransactionisolationlevel=true;
		driver.debugPrintln("supports transaction isolation level: ",
					supportstransactionisolationlevel);
		driver.debugEnd();
		return supportstransactionisolationlevel;
	}

	public synchronized
	boolean supportsTransactions() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportstransactions=true;
		driver.debugPrintln("supports transactions: ",
						supportstransactions);
		driver.debugEnd();
		return supportstransactions;
	}

	public synchronized
	boolean supportsUnion() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsunion=true;
		driver.debugPrintln("supports union: ",supportsunion);
		driver.debugEnd();
		return supportsunion;
	}

	public synchronized
	boolean supportsUnionAll() throws SQLException {
		driver.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsunionall=true;
		driver.debugPrintln("supports union all: ",supportsunionall);
		driver.debugEnd();
		return supportsunionall;
	}

	public synchronized
	boolean updatesAreDetected(int type) throws SQLException {
		driver.debugFunction(this);
		boolean	updatesaredetected=false;
		driver.debugPrintln("updates are detected: ",
						updatesaredetected);
		driver.debugEnd();
		return updatesaredetected;
	}

	public synchronized
	boolean usesLocalFilePerTable() throws SQLException {
		driver.debugFunction(this);
		boolean	useslocalfilepertable=false;
		driver.debugPrintln("uses local file per table: ",
						useslocalfilepertable);
		driver.debugEnd();
		return useslocalfilepertable;
	}

	public synchronized
	boolean usesLocalFiles() throws SQLException {
		driver.debugFunction(this);
		boolean	useslocalfiles=false;
		driver.debugPrintln("uses local files: ",useslocalfiles);
		driver.debugEnd();
		return useslocalfiles;
	}

	protected void throwErrorMessageException(SQLRCursor sqlrcur)
							throws SQLException {
		driver.debugZeroIndent();
		throw new SQLException(sqlrcur.errorMessage());
	}

	public synchronized
	boolean isWrapperFor(Class<?> iface) throws SQLException {
		driver.debugFunction(this);
		driver.debugEnd();
		return false;
	}

	public synchronized
	<T> T unwrap(Class<T> iface) throws SQLException {
		driver.debugFunction(this);
		driver.debugEnd();
		return null;
	}
};
