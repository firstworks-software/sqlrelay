package com.firstworks.sql;
	
import java.sql.*;
import java.util.regex.*;

import com.firstworks.sqlrelay.*;
	
public class SQLRelayDatabaseMetaData implements DatabaseMetaData {

	private	Object			networklock;

	private	SQLRelayDriver		drv;
	private SQLRelayConnection	conn;


	public SQLRelayDatabaseMetaData(SQLRelayDriver driver) {
		this.drv=driver;
		drv.debugFunction(this);
		conn=null;
		networklock=null;
		drv.debugEnd();
	}

	public
	void setConnection(SQLRelayConnection connection) {
		this.conn=connection;
	}

	public
	void setNetworkLock(Object networklock) {
		this.networklock=networklock;
	}

	public
	boolean allProceduresAreCallable() throws SQLException {
		drv.debugFunction(this);
		// Retrieves whether the current user can call all the
		// procedures returned by the method getProcedures.
		boolean	result=false;
		drv.debugPrintln("all procedures are callable: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean allTablesAreSelectable() throws SQLException {
		drv.debugFunction(this);
		// Retrieves whether the current user can use all the tables
		// returned by the method getTables in a SELECT statement.
		boolean	result=false;
		drv.debugPrintln("all tables are selectable: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean autoCommitFailureClosesAllResultSets() throws SQLException {
		drv.debugFunction(this);
		// Retrieves whether a SQLException while autoCommit is true
		// inidcates that all open ResultSets are closed, even ones
		// that are holdable.
		// FIXME: no idea if this is true or not
		boolean	result=false;
		drv.debugPrintln("auto commit failures closes "+
						"all result sets: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean dataDefinitionCausesTransactionCommit() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	result=false;
		drv.debugPrintln("data definition causes "+
					"transaction commit: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean dataDefinitionIgnoredInTransactions() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	result=false;
		drv.debugPrintln("data definition ignored "+
					"in transactions: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean deletesAreDetected(int type) throws SQLException {
		drv.debugFunction(this);
		// SQL Relay doesn't currenlty support ResultSet.RowDelete
		boolean	result=false;
		drv.debugPrintln("deletes are detected: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean doesMaxRowSizeIncludeBlobs() throws SQLException {
		drv.debugFunction(this);
		boolean	result=false;
		drv.debugPrintln("does max row size include blobs: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean generatedKeyAlwaysReturned() throws SQLException {
		drv.debugFunction(this);
		boolean	result=true;
		drv.debugPrintln("generated key always returned: "+result);
		drv.debugEnd();
		return result;
	}

	public
	ResultSet getAttributes(String catalog,
					String schemaPattern,
					String typeNamePattern,
					String attributeNamePattern)
					throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema pattern: "+
						schemaPattern);
		drv.debugPrintln("type name pattern: "+
						typeNamePattern);
		drv.debugPrintln("attribute name pattern: "+
						attributeNamePattern);
		// FIXME: implement this somehow...
		drv.debugPrintln("FIXME: implement this");
		drv.debugEnd();
		return null;
	}

	public
	ResultSet getBestRowIdentifier(String catalog,
						String schema,
						String table,
						int scope,
						boolean nullable)
						throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("schema: "+schema);
		drv.debugPrintln("table: "+table);
		drv.debugPrintln("scope: "+scope);
		drv.debugPrintln("nullable: "+nullable);
		// FIXME: implement this somehow...
		drv.debugPrintln("FIXME: implement this");
		drv.debugEnd();
		return null;
	}

	public
	ResultSet getCatalogs() throws SQLException {
		drv.debugFunction(this);

		SQLRelayResultSet	resultset=null;
		SQLRelayStatement	stmt=(SQLRelayStatement)
						conn.createStatement();
		SQLRCursor		sqlrcur=stmt.getSQLRCursor();

		boolean	result=false;
		synchronized (networklock) {
			result=sqlrcur.getDatabaseListWithFormat(null,4);
		}

		if (result) {

			drv.debugPrintln("colcount: "+sqlrcur.colCount());
	
			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(drv);
				resultset.setNetworkLock(networklock);
				resultset.setStatement(stmt);
				resultset.setConnection(conn);
				resultset.setSQLRCursor(sqlrcur);
			}
		} else {
			conn.throwException(sqlrcur.errorMessage());
		}

		drv.debugEnd();
		return resultset;
	}

	public
	String getCatalogSeparator() throws SQLException {
		drv.debugFunction(this);
		// FIXME: oracle uses @
		String	separator=".";
		drv.debugPrintln("catalog separator: "+separator);
		drv.debugEnd();
		return separator;
	}

	public
	String getCatalogTerm() throws SQLException {
		drv.debugFunction(this);
		// FIXME: I think SQL Server uses catalog, maybe sybase
		String	term="database";
		drv.debugPrintln("catalog term: "+term);
		drv.debugEnd();
		return term;
	}

	public
	ResultSet getClientInfoProperties() throws SQLException {
		drv.debugFunction(this);
		// FIXME: free form in SQL Relay
		drv.debugEnd();
		return null;
	}

	public
	ResultSet getColumnPrivileges(String catalog,
					String schema,
					String table,
					String columnNamePattern)
					throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema: "+schema);
		drv.debugPrintln("table: "+table);
		drv.debugPrintln("column name pattern: "+columnNamePattern);
		// FIXME: implement this somehow
		drv.debugPrintln("FIXME: implement this");
		drv.debugEnd();
		return null;
	}

	public
	ResultSet getColumns(String catalog,
					String schemaPattern,
					String tableNamePattern,
					String columnNamePattern)
					throws SQLException {
		drv.debugFunction(this);

		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema pattern: "+schemaPattern);
		drv.debugPrintln("table name pattern: "+tableNamePattern);
		drv.debugPrintln("column name pattern: "+columnNamePattern);

		String	wild=buildWild(catalog,schemaPattern,tableNamePattern);
		drv.debugPrintln("wild: "+wild);
		drv.debugPrintln("column name pattern: "+columnNamePattern);

		SQLRelayResultSet	resultset=null;
		SQLRelayStatement	stmt=(SQLRelayStatement)
						conn.createStatement();
		SQLRCursor		sqlrcur=stmt.getSQLRCursor();

		boolean	result=false;
		synchronized (networklock) {
			result=sqlrcur.getColumnListWithFormat(
						wild,columnNamePattern,4);
		}

		if (result) {

			drv.debugPrintln("colcount: "+sqlrcur.colCount());

			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(drv);
				resultset.setNetworkLock(networklock);
				resultset.setStatement(stmt);
				resultset.setConnection(conn);
				resultset.setSQLRCursor(sqlrcur);
			}
		} else {
			conn.throwException(sqlrcur.errorMessage());
		}
		
		drv.debugEnd();
		return resultset;
	}

	public
	SQLRelayConnection getConnection() throws SQLException {
		//drv.debugFunction(this);
		//drv.debugEnd();
		return conn;
	}

	public
	ResultSet getCrossReference(String parentCatalog,
					String parentSchema,
					String parentTable,
					String foreignCatalog,
					String foreignSchema,
					String foreignTable)
					throws SQLException {
		drv.debugFunction(this);

		drv.debugPrintln("parent catalog: "+parentCatalog);
		drv.debugPrintln("parent schema: "+parentSchema);
		drv.debugPrintln("parent table: "+parentTable);
		drv.debugPrintln("foreign catalog: "+foreignCatalog);
		drv.debugPrintln("foreign schema: "+foreignSchema);
		drv.debugPrintln("foreign table: "+foreignTable);

		// FIXME: implement this somehow...
		drv.debugPrintln("FIXME: implement this");
		drv.debugEnd();
		return null;
	}

	public
	int getDatabaseMajorVersion() throws SQLException {
		drv.debugFunction(this);
		int	majorversion=getDatabaseVersion(true);
		drv.debugPrintln("major version: "+majorversion);
		drv.debugEnd();
		return majorversion;
	}

	public
	int getDatabaseMinorVersion() throws SQLException {
		drv.debugFunction(this);
		int	minorversion=getDatabaseVersion(false);
		drv.debugPrintln("minor version: "+minorversion);
		drv.debugEnd();
		return minorversion;
	}

	private int getDatabaseVersion(boolean major) {
		drv.debugFunction(this);
		String	dbversion=null;
		synchronized (networklock) {
			dbversion=conn.getSQLRConnection().dbVersion();
		}
		// FIXME: cache/fetch dbVersion
		Matcher	matcher=Pattern.compile("[0-9]*\\.[0-9]*").
							matcher(dbversion);
		if (matcher.find()) {
			String[]	parts=matcher.group().split("\\.");
			if (parts!=null && parts.length>((major)?0:1)) {
				drv.debugEnd();
				return Integer.parseInt(parts[(major)?0:1]);
			}
		}
		drv.debugEnd();
		return -1;
	}

	public
	String getDatabaseProductName() throws SQLException {
		drv.debugFunction(this);
		// FIXME: cache/fetch identify
		String	id=null;
		synchronized (networklock) {
			id=conn.getSQLRConnection().identify();
		}
		drv.debugPrintln("product name: "+id);
		drv.debugEnd();
		return id;
	}

	public
	String getDatabaseProductVersion() throws SQLException {
		drv.debugFunction(this);
		// FIXME: cache/fetch dbVersion
		String	productversion=null;
		synchronized (networklock) {
			productversion=conn.getSQLRConnection().
								dbVersion();
		}
		drv.debugPrintln("product version: "+productversion);
		drv.debugEnd();
		return productversion;
	}

	public
	int getDefaultTransactionIsolation() throws SQLException {
		drv.debugFunction(this);
		int	isolation=(getDatabaseProductName().equals("mysql"))?
					Connection.TRANSACTION_REPEATABLE_READ:
					Connection.TRANSACTION_READ_COMMITTED;
		drv.debugPrintln("isolation: "+isolation);
		drv.debugEnd();
		return isolation;
	}

	public
	int getDriverMajorVersion() {
		drv.debugFunction(this);
		int		majorversion=-1;
		String[]	parts=conn.
					getSQLRConnection().
					clientVersion().split(".");
		if (parts!=null && parts.length>0) {
			majorversion=Integer.parseInt(parts[0]);
		}
		drv.debugPrintln("major version: "+majorversion);
		drv.debugEnd();
		return majorversion;
	}

	public
	int getDriverMinorVersion() {
		drv.debugFunction(this);
		int		minorversion=-1;
		String[]	parts=conn.
					getSQLRConnection().
					clientVersion().split(".");
		if (parts!=null && parts.length>1) {
			minorversion=Integer.parseInt(parts[1]);
		}
		drv.debugPrintln("minor version: "+minorversion);
		drv.debugEnd();
		return minorversion;
	}

	public
	String getDriverName() throws SQLException {
		drv.debugFunction(this);
		String	drivername="sqlrelay";
		drv.debugPrintln("driver name: "+drivername);
		drv.debugEnd();
		return drivername;
	}

	public
	String getDriverVersion() throws SQLException {
		drv.debugFunction(this);
		String	driverversion=conn.
					getSQLRConnection().
					clientVersion();
		drv.debugPrintln("driver version: "+driverversion);
		drv.debugEnd();
		return driverversion;
	}

	public
	ResultSet getExportedKeys(String catalog,
					String schema,
					String table)
					throws SQLException {

		drv.debugFunction(this);

		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema: "+schema);
		drv.debugPrintln("table: "+table);

		// Retrieves a description of the foreign key columns that
		// reference the given table's primary key columns (the foreign
		// keys exported by a table).
		// FIXME: implement this somehow
		drv.debugPrintln("FIXME: implement this");
		drv.debugEnd();
		return null;
	}

	public
	String getExtraNameCharacters() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		String	extranamechars="#@";
		drv.debugPrintln("extra name characters: "+extranamechars);
		drv.debugEnd();
		return extranamechars;
	}

	public
	ResultSet getFunctionColumns(String catalog,
					String schemaPattern,
					String functionNamePattern,
					String columnNamePattern)
					throws SQLException {
		drv.debugFunction(this);

		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema pattern: "+
						schemaPattern);
		drv.debugPrintln("function name pattern: "+
						functionNamePattern);
		drv.debugPrintln("column name pattern: "+
						columnNamePattern);

		// FIXME: implement with
		drv.debugPrintln("FIXME: implement this");
		// sqlrcur.getProcedureBindAndColumnList()?
		drv.debugEnd();
		return null;
	}

	public
	ResultSet getFunctions(String catalog,
					String schemaPattern,
					String functionNamePattern)
					throws SQLException {
		drv.debugFunction(this);

		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema pattern: "+
						schemaPattern);
		drv.debugPrintln("function name pattern: "+
						functionNamePattern);

		// FIXME: implement this by calling sqlrcur.getProcedures()?
		drv.debugPrintln("FIXME: implement this");
		drv.debugEnd();
		return null;
	}

	public
	String getIdentifierQuoteString() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		// * sqlserver uses braces
		String	identifierquotestring=
			(getDatabaseProductName().equals("mysql"))?"`":"\"";
		drv.debugPrintln("identifier quote string: "+
					identifierquotestring);
		drv.debugEnd();
		return identifierquotestring;
	}

	public
	ResultSet getImportedKeys(String catalog,
					String schema,
					String table)
					throws SQLException {
		drv.debugFunction(this);

		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema: "+schema);
		drv.debugPrintln("table: "+table);

		// Retrieves a description of the primary key columns that are
		// referenced by the given table's foreign key columns (the
		// primary keys imported by a table).
		// FIXME: implement this somehow
		drv.debugPrintln("FIXME: implement this");
		drv.debugEnd();
		return null;
	}

	public
	ResultSet getIndexInfo(String catalog,
					String schema,
					String table,
					boolean unique,
					boolean approximate)
					throws SQLException {
		drv.debugFunction(this);

		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema: "+schema);
		drv.debugPrintln("table: "+table);
		drv.debugPrintln("unique: "+unique);
		drv.debugPrintln("approximate: "+approximate);

		// FIXME: implement using sqlrcur.getKeyAndIndexList() ?
		drv.debugPrintln("FIXME: implement this");
		drv.debugEnd();
		return null;
	}

	public
	int getJDBCMajorVersion() throws SQLException {
		drv.debugFunction(this);
		// FIXME: get this from ???
		int	jdbcmajorversion=4;
		drv.debugPrintln("jdbc major version: "+jdbcmajorversion);
		drv.debugEnd();
		return jdbcmajorversion;
	}

	public
	int getJDBCMinorVersion() throws SQLException {
		drv.debugFunction(this);
		// FIXME: get this from ???
		int	jdbcminorversion=3;
		drv.debugPrintln("jdbc minor version: "+jdbcminorversion);
		drv.debugEnd();
		return jdbcminorversion;
	}

	public
	int getMaxBinaryLiteralLength() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxbinaryliterallength=0;
		drv.debugPrintln("max binary literal length: "+
						maxbinaryliterallength);
		drv.debugEnd();
		return maxbinaryliterallength;
	}

	public
	int getMaxCatalogNameLength() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcatalognamelength=0;
		drv.debugPrintln("max catalog name length: "+
						maxcatalognamelength);
		drv.debugEnd();
		return maxcatalognamelength;
	}

	public
	int getMaxCharLiteralLength() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcharliterallength=0;
		drv.debugPrintln("max char literal length: "+
						maxcharliterallength);
		drv.debugEnd();
		return maxcharliterallength;
	}

	public
	int getMaxColumnNameLength() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcolumnnamelength=0;
		drv.debugPrintln("max column name length: "+
						maxcolumnnamelength);
		drv.debugEnd();
		return maxcolumnnamelength;
	}

	public
	int getMaxColumnsInGroupBy() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcolumnsingroup=0;
		drv.debugPrintln("max columns in group: "+
						maxcolumnsingroup);
		drv.debugEnd();
		return maxcolumnsingroup;
	}

	public
	int getMaxColumnsInIndex() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcolumnsinindex=0;
		drv.debugPrintln("max columns in index: "+
						maxcolumnsinindex);
		drv.debugEnd();
		return maxcolumnsinindex;
	}

	public
	int getMaxColumnsInOrderBy() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcolumnsinorderby=0;
		drv.debugPrintln("max columns in order by: "+
						maxcolumnsinorderby);
		drv.debugEnd();
		return maxcolumnsinorderby;
	}

	public
	int getMaxColumnsInSelect() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcolumnsinselect=0;
		drv.debugPrintln("max columns in select: "+
						maxcolumnsinselect);
		drv.debugEnd();
		return maxcolumnsinselect;
	}

	public
	int getMaxColumnsInTable() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxcolumnsintable=0;
		drv.debugPrintln("max columns in table: "+
						maxcolumnsintable);
		drv.debugEnd();
		return maxcolumnsintable;
	}

	public
	int getMaxConnections() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxconnections=0;
		drv.debugPrintln("max connections: "+maxconnections);
		drv.debugEnd();
		return maxconnections;
	}

	public
	int getMaxCursorNameLength() throws SQLException {
		drv.debugFunction(this);
		int	maxcursornamelength=0;
		drv.debugPrintln("max cursor name length: "+
						maxcursornamelength);
		drv.debugEnd();
		return maxcursornamelength;
	}

	public
	int getMaxIndexLength() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxindexlength=0;
		drv.debugPrintln("max index length: "+maxindexlength);
		drv.debugEnd();
		return maxindexlength;
	}

	public
	int getMaxProcedureNameLength() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxprocedurenamelength=0;
		drv.debugPrintln("max procedure name length: "+
						maxprocedurenamelength);
		drv.debugEnd();
		return maxprocedurenamelength;
	}

	public
	int getMaxRowSize() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxrowsize=0;
		drv.debugPrintln("max row size: "+maxrowsize);
		drv.debugEnd();
		return maxrowsize;
	}

	public
	int getMaxSchemaNameLength() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxschemanamelength=0;
		drv.debugPrintln("max schema name length: "+
						maxschemanamelength);
		drv.debugEnd();
		return maxschemanamelength;
	}

	public
	int getMaxStatementLength() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxstatementlength=0;
		drv.debugPrintln("max statement length: "+
						maxstatementlength);
		drv.debugEnd();
		return maxstatementlength;
	}

	public
	int getMaxStatements() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxstatements=0;
		drv.debugPrintln("max statements: "+maxstatements);
		drv.debugEnd();
		return maxstatements;
	}

	public
	int getMaxTableNameLength() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxtablenamelength=0;
		drv.debugPrintln("max table name length: "+
						maxtablenamelength);
		drv.debugEnd();
		return maxtablenamelength;
	}

	public
	int getMaxTablesInSelect() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxtablesinselect=0;
		drv.debugPrintln("max tables in select: "+
						maxtablesinselect);
		drv.debugEnd();
		return maxtablesinselect;
	}

	public
	int getMaxUserNameLength() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific (0 means no limit or unknown)
		int	maxusernamelength=0;
		drv.debugPrintln("max user name length: "+maxusernamelength);
		drv.debugEnd();
		return maxusernamelength;
	}

	public
	String getNumericFunctions() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		String	numericfunctions=null;
		drv.debugPrintln("numeric functions: "+numericfunctions);
		drv.debugEnd();
		return numericfunctions;
	}

	public
	ResultSet getPrimaryKeys(String catalog,
					String schema,
					String table)
					throws SQLException {
		drv.debugFunction(this);

		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema: "+schema);
		drv.debugPrintln("table: "+table);

		// FIXME: implement this by calling sqlrcon.getPrimaryKeysList()
		drv.debugPrintln("FIXME: implement this");
		drv.debugEnd();
		return null;
	}

	public
	ResultSet getProcedureColumns(String catalog,
					String schemaPattern,
					String procedureNamePattern,
					String columnNamePattern)
					throws SQLException {
		drv.debugFunction(this);

		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema pattern: "+
						schemaPattern);
		drv.debugPrintln("procedure name pattern: "+
						procedureNamePattern);
		drv.debugPrintln("column name pattern: "+
						columnNamePattern);

		// FIXME: implement this by calling
		drv.debugPrintln("FIXME: implement this");
		// sqlrcon.getProcedureBindAndColumnList()
		drv.debugEnd();
		return null;
	}

	public
	ResultSet getProcedures(String catalog,
					String schemaPattern,
					String procedureNamePattern)
					throws SQLException {
		drv.debugFunction(this);

		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema pattern: "+
						schemaPattern);
		drv.debugPrintln("procedure name pattern: "+
						procedureNamePattern);

		// FIXME: We do have some amount of backend support for this,
		// but currently it returns all procedures, in all schemas, in
		// all databases, and doesn't handle wildcards at all.  This
		// tends to generate large result sets, and slow apps down
		// terribly, while simultaneously giving them the wrong info.
		// For now, we'll just disable it.
		drv.debugPrintln("FIXME: implement wildcards "+
					"correctly on the backend");
		drv.debugEnd();
		return null;

/*
		String	wild=buildWild(catalog,schemaPattern,
						procedureNamePattern);
		drv.debugPrintln("wild: "+wild);

		SQLRelayResultSet	resultset=null;
		SQLRelayStatement	stmt=(SQLRelayStatement)
						conn.createStatement();
		SQLRCursor		sqlrcur=stmt.getSQLRCursor();

		boolean	result=false;
		synchronized (networklock) {
			result=sqlrcur.getProcedureListWithFormat(
						procedureNamePattern,4);
		}

		if (result) {

			drv.debugPrintln("colcount: "+sqlrcur.colCount());

			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(drv);
				resultset.setNetworkLock(networklock);
				resultset.setStatement(stmt);
				resultset.setConnection(conn);
				resultset.setSQLRCursor(sqlrcur);
			}
		} else {
			conn.throwException(sqlrcur.errorMessage());
		}
		
		drv.debugEnd();
		return resultset;
*/
	}

	public
	String getProcedureTerm() throws SQLException {
		drv.debugFunction(this);
		String	procedureterm="procedure";
		drv.debugPrintln("procedure term: "+procedureterm);
		drv.debugEnd();
		return procedureterm;
	}

	public
	ResultSet getPseudoColumns(String catalog,
					String schemaPattern,
					String tableNamePattern,
					String columnNamePattern)
					throws SQLException {
		drv.debugFunction(this);

		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema pattern: "+schemaPattern);
		drv.debugPrintln("table name pattern: "+tableNamePattern);
		drv.debugPrintln("column name pattern: "+columnNamePattern);

		// FIXME: implement this somehow
		drv.debugPrintln("FIXME: implement this");
		drv.debugEnd();
		return null;
	}

	public
	int getResultSetHoldability() throws SQLException {
		drv.debugFunction(this);
		// FIXME: is this correct?
		int	resultsetholdability=ResultSet.CLOSE_CURSORS_AT_COMMIT;
		drv.debugPrintln("result set holdability: "+
						resultsetholdability);
		drv.debugEnd();
		return resultsetholdability;
	}

	public
	RowIdLifetime getRowIdLifetime() throws SQLException {
		drv.debugFunction(this);
		// FIXME: some dbs do support rowid
		RowIdLifetime	rowidlifetime=RowIdLifetime.ROWID_UNSUPPORTED;
		switch (rowidlifetime) {
			case ROWID_UNSUPPORTED:
				drv.debugPrintln("rowid lifetime: "+
							"ROWID_UNSUPPORTED");
				break;
			case ROWID_VALID_OTHER:
				drv.debugPrintln("rowid lifetime: "+
							"ROWID_VALID_OTHER");
				break;
			case ROWID_VALID_TRANSACTION:
				drv.debugPrintln("rowid lifetime: "+
						"ROWID_VALID_TRANSACTION");
				break;
			case ROWID_VALID_SESSION:
				drv.debugPrintln("rowid lifetime: "+
							"ROWID_VALID_SESSION");
				break;
			case ROWID_VALID_FOREVER:
				drv.debugPrintln("rowid lifetime: "+
							"ROWID_VALID_FOREVER");
				break;
		}
		drv.debugEnd();
		return rowidlifetime;
	}

	public
	ResultSet getSchemas() throws SQLException {
		drv.debugFunction(this);
		ResultSet	schemas=getSchemas(null,null);
		drv.debugEnd();
		return schemas;
	}

	public
	ResultSet getSchemas(String catalog,
					String schemaPattern)
					throws SQLException {
		drv.debugFunction(this);

		// FIXME: use catalog
		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema pattern: "+schemaPattern);

		SQLRelayResultSet	resultset=null;
		SQLRelayStatement	stmt=(SQLRelayStatement)
						conn.createStatement();
		SQLRCursor		sqlrcur=stmt.getSQLRCursor();

		boolean	result=false;
		synchronized (networklock) {
			result=sqlrcur.getSchemaListWithFormat(schemaPattern,4);
		}

		if (result) {

			drv.debugPrintln("colcount: "+sqlrcur.colCount());

			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(drv);
				resultset.setNetworkLock(networklock);
				resultset.setStatement(stmt);
				resultset.setConnection(conn);
				resultset.setSQLRCursor(sqlrcur);
			}
		} else {
			conn.throwException(sqlrcur.errorMessage());
		}
	
		drv.debugEnd();
		return resultset;
	}

	public
	String getSchemaTerm() throws SQLException {
		drv.debugFunction(this);
		String	schematerm="schema";
		drv.debugPrintln("schema term: "+schematerm);
		drv.debugEnd();
		return schematerm;
	}

	public
	String getSearchStringEscape() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		String	searchstringescape="\\";
		drv.debugPrintln("search string escape: "+
						searchstringescape);
		drv.debugEnd();
		return searchstringescape;
	}

	public
	String getSQLKeywords() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		String	sqlkeywords=null;
		drv.debugPrintln("sql keywords: "+sqlkeywords);
		drv.debugEnd();
		return sqlkeywords;
	}

	public
	int getSQLStateType() throws SQLException {
		drv.debugFunction(this);
		// FIXME: no idea
		int	sqlstatetype=sqlStateSQL;
		drv.debugPrintln("sql state type: "+sqlstatetype);
		drv.debugEnd();
		return sqlstatetype;
	}

	public
	String getStringFunctions() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		String	stringfunctions=null;
		drv.debugPrintln("string functions: "+stringfunctions);
		drv.debugEnd();
		return stringfunctions;
	}

	public
	ResultSet getSuperTables(String catalog,
					String schemaPattern,
					String tableNamePattern)
					throws SQLException {
		drv.debugFunction(this);
		// few jdbc drivers (or databases) support this
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	ResultSet getSuperTypes(String catalog,
					String schemaPattern,
					String typeNamePattern)
					throws SQLException {
		drv.debugFunction(this);

		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema pattern: "+schemaPattern);
		drv.debugPrintln("type name pattern: "+typeNamePattern);

		// FIXME: implement this somehow
		drv.debugPrintln("FIXME: implement this");
		drv.debugEnd();
		return null;
	}

	public
	String getSystemFunctions() throws SQLException {
		drv.debugFunction(this);
		// FIXME: implement this somehow
		drv.debugPrintln("FIXME: implement this");
		String	systemfunctions=null;
		drv.debugEnd();
		return systemfunctions;
	}

	public
	ResultSet getTablePrivileges(String catalog,
					String schemaPattern,
					String tableNamePattern)
					throws SQLException {
		drv.debugFunction(this);

		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema pattern: "+schemaPattern);
		drv.debugPrintln("table name pattern: "+tableNamePattern);

		// FIXME: implement this somehow
		drv.debugPrintln("FIXME: implement this");
		drv.debugEnd();
		return null;
	}

	public
	ResultSet getTables(String catalog,
				String schemaPattern,
				String tableNamePattern,
				String[] types)
				throws SQLException {
		drv.debugFunction(this);

		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema pattern: "+schemaPattern);
		drv.debugPrintln("table name pattern: "+tableNamePattern);

		int	objecttypes=0;
		if (types==null) {
			drv.debugPrintln("types: null");
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
			drv.debugPrintln("types: "+t);
		}

		String	wild=buildWild(catalog,schemaPattern,tableNamePattern);
		drv.debugPrintln("wild: "+wild);

		SQLRelayResultSet	resultset=null;
		SQLRelayStatement	stmt=(SQLRelayStatement)
						conn.createStatement();
		SQLRCursor		sqlrcur=stmt.getSQLRCursor();

		boolean	result=false;
		synchronized (networklock) {
			result=sqlrcur.getTableListWithFormat(
					tableNamePattern,4,objecttypes);
		}

		if (result) {

			drv.debugPrintln("colcount: "+sqlrcur.colCount());

			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(drv);
				resultset.setNetworkLock(networklock);
				resultset.setStatement(stmt);
				resultset.setConnection(conn);
				resultset.setSQLRCursor(sqlrcur);
			}
		} else {
			conn.throwException(sqlrcur.errorMessage());
		}
		
		drv.debugEnd();
		return resultset;
	}

	private String buildWild(String catalog,
					String schema,
					String object) {
		drv.debugFunction(this);

		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema: "+schema);
		drv.debugPrintln("object: "+object);

		// If object already contains a . then just use it
		// as-is.
		if (object!=null && object.contains(".")) {
			drv.debugEnd();
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
		drv.debugEnd();
		return wild.toString();
	}

	public
	ResultSet getTableTypes() throws SQLException {
		drv.debugFunction(this);

		SQLRelayResultSet	resultset=null;
		SQLRelayStatement	stmt=(SQLRelayStatement)
						conn.createStatement();
		SQLRCursor		sqlrcur=stmt.getSQLRCursor();

		boolean	result=false;
		synchronized (networklock) {
			result=sqlrcur.getTableTypeListWithFormat(null,4);
		}

		if (result) {

			drv.debugPrintln("colcount: "+sqlrcur.colCount());

			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(drv);
				resultset.setNetworkLock(networklock);
				resultset.setStatement(stmt);
				resultset.setConnection(conn);
				resultset.setSQLRCursor(sqlrcur);
			}
		} else {
			conn.throwException(sqlrcur.errorMessage());
		}
	
		drv.debugEnd();
		return resultset;
	}

	public
	String getTimeDateFunctions() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		String	timedatefunctions=null;
		drv.debugPrintln("timedate functions: "+timedatefunctions);
		drv.debugEnd();
		return timedatefunctions;
	}

	public
	ResultSet getTypeInfo() throws SQLException {
		drv.debugFunction(this);

		SQLRelayResultSet	resultset=null;
		SQLRelayStatement	stmt=(SQLRelayStatement)
						conn.createStatement();
		SQLRCursor		sqlrcur=stmt.getSQLRCursor();

		boolean	result=false;
		synchronized (networklock) {
			result=sqlrcur.getTypeInfoListWithFormat("*",null,4);
		}

		if (result) {

			drv.debugPrintln("colcount: "+sqlrcur.colCount());

			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(drv);
				resultset.setNetworkLock(networklock);
				resultset.setStatement(stmt);
				resultset.setConnection(conn);
				resultset.setSQLRCursor(sqlrcur);
			}
		} else {
			conn.throwException(sqlrcur.errorMessage());
		}
	
		drv.debugEnd();
		return resultset;
	}

	public
	ResultSet getUDTs(String catalog,
				String schemaPattern,
				String typeNamePattern,
				int[] types)
				throws SQLException {
		drv.debugFunction(this);

		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema pattern: "+schemaPattern);
		drv.debugPrintln("type name pattern: "+typeNamePattern);
		// FIXME: debug types...

		// FIXME: implement this somehow
		drv.debugPrintln("FIXME: implement this");
		drv.debugEnd();
		return null;
	}

	public
	String getURL() throws SQLException {
		drv.debugFunction(this);

		String	host=conn.getHost();
		short	port=conn.getPort();
		String	socket=conn.getSocket();
		String	user=conn.getUser();
		String	password=conn.getPassword();

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

		drv.debugPrintln("url: "+url);

		drv.debugEnd();
		return url;
	}

	public
	String getUserName() throws SQLException {
		drv.debugFunction(this);
		String	username=conn.getUser();
		drv.debugPrintln("user name: "+username);
		drv.debugEnd();
		return username;
	}

	public
	ResultSet getVersionColumns(String catalog,
					String schema,
					String table)
					throws SQLException {
		drv.debugFunction(this);

		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema: "+schema);
		drv.debugPrintln("table: "+table);

		// FIXME: implement this somehow
		drv.debugPrintln("FIXME: implement this");
		drv.debugEnd();
		return null;
	}

	public
	boolean insertsAreDetected(int type) throws SQLException {
		drv.debugFunction(this);
		boolean	insertsaredetected=false;
		drv.debugPrintln("type: "+type);
		drv.debugPrintln("inserts are detected: "+
						insertsaredetected);
		drv.debugEnd();
		return insertsaredetected;
	}

	public
	boolean isCatalogAtStart() throws SQLException {
		drv.debugFunction(this);
		// FIXME: not in oracle
		boolean	iscatalogatstart=true;
		drv.debugPrintln("is catalog at start: "+iscatalogatstart);
		drv.debugEnd();
		return iscatalogatstart;
	}

	public
	boolean isReadOnly() throws SQLException {
		drv.debugFunction(this);
		// FIXME: implement this somehow
		drv.debugPrintln("FIXME: implement this");
		boolean	isreadonly=false;
		drv.debugPrintln("is read only: "+isreadonly);
		drv.debugEnd();
		return isreadonly;
	}

	public
	boolean locatorsUpdateCopy() throws SQLException {
		drv.debugFunction(this);
		// FIXME: no idea, probably db-specific
		boolean	locatorsupdatecopy=false;
		drv.debugPrintln("locators update copy: "+
						locatorsupdatecopy);
		drv.debugEnd();
		return locatorsupdatecopy;
	}

	public
	boolean nullPlusNonNullIsNull() throws SQLException {
		drv.debugFunction(this);
		// FIXME: generally true, but probably db-specific
		boolean	nullplusnonnullisnull=true;
		drv.debugPrintln("null plus non null is null: "+
						nullplusnonnullisnull);
		drv.debugEnd();
		return nullplusnonnullisnull;
	}

	public
	boolean nullsAreSortedAtEnd() throws SQLException {
		drv.debugFunction(this);
		// FIXME: generally true, but probably db-specific
		boolean	nullsaresortedatend=true;
		drv.debugPrintln("nulls are sorted at end: "+
						nullsaresortedatend);
		drv.debugEnd();
		return nullsaresortedatend;
	}

	public
	boolean nullsAreSortedAtStart() throws SQLException {
		drv.debugFunction(this);
		// FIXME: generally false, but probably db-specific
		boolean	nullsaresortedatstart=false;
		drv.debugPrintln("nulls are sorted at start: "+
						nullsaresortedatstart);
		drv.debugEnd();
		return nullsaresortedatstart;
	}

	public
	boolean nullsAreSortedHigh() throws SQLException {
		drv.debugFunction(this);
		// FIXME: generally true, but probably db-specific
		boolean	nullsaresortedhigh=true;
		drv.debugPrintln("nulls are sorted high: "+
						nullsaresortedhigh);
		drv.debugEnd();
		return nullsaresortedhigh;
	}

	public
	boolean nullsAreSortedLow() throws SQLException {
		drv.debugFunction(this);
		// FIXME: generally false, but probably db-specific
		boolean	nullsaresortedlow=false;
		drv.debugPrintln("nulls are sorted low: "+
						nullsaresortedlow);
		drv.debugEnd();
		return nullsaresortedlow;
	}

	public
	boolean othersDeletesAreVisible(int type) throws SQLException {
		drv.debugFunction(this);
		boolean	othersdeletesarevisible=false;
		drv.debugPrintln("others deletes are visible: "+
						othersdeletesarevisible);
		drv.debugEnd();
		return othersdeletesarevisible;
	}

	public
	boolean othersInsertsAreVisible(int type) throws SQLException {
		drv.debugFunction(this);
		boolean	othersinsertssarevisible=false;
		drv.debugPrintln("others inserts are visible: "+
						othersinsertssarevisible);
		drv.debugEnd();
		return othersinsertssarevisible;
	}

	public
	boolean othersUpdatesAreVisible(int type) throws SQLException {
		drv.debugFunction(this);
		boolean	othersupdatessarevisible=false;
		drv.debugPrintln("others updates are visible: "+
						othersupdatessarevisible);
		drv.debugEnd();
		return othersupdatessarevisible;
	}

	public
	boolean ownDeletesAreVisible(int type) throws SQLException {
		drv.debugFunction(this);
		boolean	owndeletesarevisible=false;
		drv.debugPrintln("own deletes are visible: "+
						owndeletesarevisible);
		drv.debugEnd();
		return owndeletesarevisible;
	}

	public
	boolean ownInsertsAreVisible(int type) throws SQLException {
		drv.debugFunction(this);
		boolean	owninsertsarevisible=false;
		drv.debugPrintln("own inserts are visible: "+
						owninsertsarevisible);
		drv.debugEnd();
		return owninsertsarevisible;
	}

	public
	boolean ownUpdatesAreVisible(int type) throws SQLException {
		drv.debugFunction(this);
		boolean	ownupdatesarevisible=false;
		drv.debugPrintln("own updates are visible: "+
						ownupdatesarevisible);
		drv.debugEnd();
		return ownupdatesarevisible;
	}

	public
	boolean storesLowerCaseIdentifiers() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific but generally false
		// oracle stores upper case identifiers
		// other db's store mixed case identifiers
		boolean	storeslowercaseidentifiers=false;
		drv.debugPrintln("stores lower case identifiers: "+
						storeslowercaseidentifiers);
		drv.debugEnd();
		return storeslowercaseidentifiers;
	}

	public
	boolean storesLowerCaseQuotedIdentifiers() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	storeslowercasequotedidentifiers=false;
		drv.debugPrintln("stores lower case quoted identifiers: "+
					storeslowercasequotedidentifiers);
		drv.debugEnd();
		return storeslowercasequotedidentifiers;
	}

	public
	boolean storesMixedCaseIdentifiers() throws SQLException {
		drv.debugFunction(this);
		// FIXME: generally true, but db-specific, false for oracle
		boolean	storesmixedcaseidentifiers=true;
		drv.debugPrintln("stores mixed case identifiers: "+
						storesmixedcaseidentifiers);
		drv.debugEnd();
		return storesmixedcaseidentifiers;
	}

	public
	boolean storesMixedCaseQuotedIdentifiers() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	storesmixedcasequotedidentifiers=true;
		drv.debugPrintln("stores mixed case quoted identifiers: "+
					storesmixedcasequotedidentifiers);
		drv.debugEnd();
		return storesmixedcasequotedidentifiers;
	}

	public
	boolean storesUpperCaseIdentifiers() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific but generally false
		// oracle stores upper case identifiers
		// other db's store mixed case identifiers
		boolean	storesuppercaseidentifiers=false;
		drv.debugPrintln("stores upper case identifiers: "+
						storesuppercaseidentifiers);
		drv.debugEnd();
		return storesuppercaseidentifiers;
	}

	public
	boolean storesUpperCaseQuotedIdentifiers() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	storesuppercasequotedidentifiers=true;
		drv.debugPrintln("stores upper case quoted identifiers: "+
					storesuppercasequotedidentifiers);
		drv.debugEnd();
		return storesuppercasequotedidentifiers;
	}

	public
	boolean supportsAlterTableWithAddColumn() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsaltertablewithaddcommand=true;
		drv.debugPrintln("supports alter table with add command: "+
					supportsaltertablewithaddcommand);
		drv.debugEnd();
		return supportsaltertablewithaddcommand;
	}

	public
	boolean supportsAlterTableWithDropColumn() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsaltertablewithdropcommand=true;
		drv.debugPrintln("supports alter table with drop command: "+
					supportsaltertablewithdropcommand);
		drv.debugEnd();
		return supportsaltertablewithdropcommand;
	}

	public
	boolean supportsANSI92EntryLevelSQL() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsansi92entrylevelsql=true;
		drv.debugPrintln("supports ansi92 entry level sql: "+
						supportsansi92entrylevelsql);
		drv.debugEnd();
		return supportsansi92entrylevelsql;
	}

	public
	boolean supportsANSI92FullSQL() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsansi92fullsql=true;
		drv.debugPrintln("supports ansi92 full sql: "+
						supportsansi92fullsql);
		drv.debugEnd();
		return supportsansi92fullsql;
	}

	public
	boolean supportsANSI92IntermediateSQL() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsansi92intermediatesql=true;
		drv.debugPrintln("supports ansi92 intermediate sql: "+
						supportsansi92intermediatesql);
		drv.debugEnd();
		return supportsansi92intermediatesql;
	}

	public
	boolean supportsBatchUpdates() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsbatchupdates=false;
		drv.debugPrintln("supports batch updates: "+
						supportsbatchupdates);
		drv.debugEnd();
		return supportsbatchupdates;
	}

	public
	boolean supportsCatalogsInDataManipulation() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscatalogsindatamanipulation=true;
		drv.debugPrintln("supports catalogs in data manipulations: "+
					supportscatalogsindatamanipulation);
		drv.debugEnd();
		return supportscatalogsindatamanipulation;
	}

	public
	boolean supportsCatalogsInIndexDefinitions() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscatalogsinindexdefinitions=true;
		drv.debugPrintln("supports catalogs in index definitions: "+
					supportscatalogsinindexdefinitions);
		drv.debugEnd();
		return supportscatalogsinindexdefinitions;
	}

	public
	boolean supportsCatalogsInPrivilegeDefinitions() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscatalogsinprivilegedefinitions=true;
		drv.debugPrintln("supports catalogs in "+
					"privilege definitions: "+
					supportscatalogsinprivilegedefinitions);
		drv.debugEnd();
		return supportscatalogsinprivilegedefinitions;
	}

	public
	boolean supportsCatalogsInProcedureCalls() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscatalogsinprocedurecalls=true;
		drv.debugPrintln("supports catalogs in procedure calls: "+
					supportscatalogsinprocedurecalls);
		drv.debugEnd();
		return supportscatalogsinprocedurecalls;
	}

	public
	boolean supportsCatalogsInTableDefinitions() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscatalogsintabledefinitions=true;
		drv.debugPrintln("supports catalogs in table definitions: "+
					supportscatalogsintabledefinitions);
		drv.debugEnd();
		return supportscatalogsintabledefinitions;
	}

	public
	boolean supportsColumnAliasing() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscolumnaliasing=true;
		drv.debugPrintln("supports column aliasing: "+
					supportscolumnaliasing);
		drv.debugEnd();
		return supportscolumnaliasing;
	}

	public
	boolean supportsConvert() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsconvert=true;
		drv.debugPrintln("supports convert: "+supportsconvert);
		drv.debugEnd();
		return supportsconvert;
	}

	public
	boolean supportsConvert(int fromType, int toType) throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-and-type-specific
		boolean	supportsconvert=true;
		drv.debugPrintln("from type: "+fromType);
		drv.debugPrintln("to type: "+toType);
		drv.debugPrintln("supports convert: "+supportsconvert);
		drv.debugEnd();
		return supportsconvert;
	}

	public
	boolean supportsCoreSQLGrammar() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscoresqlgrammar=true;
		drv.debugPrintln("supports core sql grammar: "+
						supportscoresqlgrammar);
		drv.debugEnd();
		return supportscoresqlgrammar;
	}

	public
	boolean supportsCorrelatedSubqueries() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportscorrelatedsubqueries=true;
		drv.debugPrintln("supports correlated subqueries: "+
						supportscorrelatedsubqueries);
		drv.debugEnd();
		return supportscorrelatedsubqueries;
	}

	public
	boolean supportsDataDefinitionAndDataManipulationTransactions()
							throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	sddadmt=true;
		drv.debugPrintln("supports data definition "+
					"and data manipulation transactions: "+
					sddadmt);
		drv.debugEnd();
		return sddadmt;
	}

	public
	boolean supportsDataManipulationTransactionsOnly() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	sdmto=false;
		drv.debugPrintln("supports data manipulation "+
					"transactions only: "+sdmto);
		drv.debugEnd();
		return sdmto;
	}

	public
	boolean supportsDifferentTableCorrelationNames() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	sdtcn=true;
		drv.debugPrintln("supports different table "+
					"correlation names: "+sdtcn);
		drv.debugEnd();
		return sdtcn;
	}

	public
	boolean supportsExpressionsInOrderBy() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsexpressionsinorderby=true;
		drv.debugPrintln("supports expressions in order by: "+
						supportsexpressionsinorderby);
		drv.debugEnd();
		return supportsexpressionsinorderby;
	}

	public
	boolean supportsExtendedSQLGrammar() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsextendedsqlgrammar=true;
		drv.debugPrintln("supports extended sql grammar: "+
						supportsextendedsqlgrammar);
		drv.debugEnd();
		return supportsextendedsqlgrammar;
	}

	public
	boolean supportsFullOuterJoins() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsfullouterjoins=true;
		drv.debugPrintln("supports full outer joins: "+
						supportsfullouterjoins);
		drv.debugEnd();
		return supportsfullouterjoins;
	}

	public
	boolean supportsGetGeneratedKeys() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsgetgeneratedkeys=true;
		drv.debugPrintln("supports get generated keys: "+
						supportsgetgeneratedkeys);
		drv.debugEnd();
		return supportsgetgeneratedkeys;
	}

	public
	boolean supportsGroupBy() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsgroupby=true;
		drv.debugPrintln("supports group by: "+supportsgroupby);
		drv.debugEnd();
		return supportsgroupby;
	}

	public
	boolean supportsGroupByBeyondSelect() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsgroupbybeyondselect=true;
		drv.debugPrintln("supports group by beyond select: "+
						supportsgroupbybeyondselect);
		drv.debugEnd();
		return supportsgroupbybeyondselect;
	}

	public
	boolean supportsGroupByUnrelated() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsgroupbyunrelated=true;
		drv.debugPrintln("supports group by unrelated: "+
						supportsgroupbyunrelated);
		drv.debugEnd();
		return supportsgroupbyunrelated;
	}

	public
	boolean supportsIntegrityEnhancementFacility() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsintegrityenhancementfacility=false;
		drv.debugPrintln("supports integrity enhancement facility: "+
					supportsintegrityenhancementfacility);
		drv.debugEnd();
		return supportsintegrityenhancementfacility;
	}

	public
	boolean supportsLikeEscapeClause() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportslikeescapeclause=true;
		drv.debugPrintln("supports like escape clause: "+
						supportslikeescapeclause);
		drv.debugEnd();
		return supportslikeescapeclause;
	}

	public
	boolean supportsLimitedOuterJoins() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportslimitedouterjoins=true;
		drv.debugPrintln("supports limited outer joins: "+
						supportslimitedouterjoins);
		drv.debugEnd();
		return supportslimitedouterjoins;
	}

	public
	boolean supportsMinimumSQLGrammar() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsminimumsqlgrammar=true;
		drv.debugPrintln("supports minimum sql grammar: "+
						supportsminimumsqlgrammar);
		drv.debugEnd();
		return supportsminimumsqlgrammar;
	}

	public
	boolean supportsMixedCaseIdentifiers() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific, oracle doesn't
		boolean	supportsmixedcaseidentifiers=true;
		drv.debugPrintln("supports mixed case identifiers: "+
						supportsmixedcaseidentifiers);
		drv.debugEnd();
		return supportsmixedcaseidentifiers;
	}

	public
	boolean supportsMixedCaseQuotedIdentifiers() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsmixedcasequotedidentifiers=true;
		drv.debugPrintln("supports mixed case quoted identifiers: "+
					supportsmixedcasequotedidentifiers);
		drv.debugEnd();
		return supportsmixedcasequotedidentifiers;
	}

	public
	boolean supportsMultipleOpenResults() throws SQLException {
		drv.debugFunction(this);
		boolean	supportsmultipleopenresults=true;
		drv.debugPrintln("supports multiple open results: "+
						supportsmultipleopenresults);
		drv.debugEnd();
		return supportsmultipleopenresults;
	}

	public
	boolean supportsMultipleResultSets() throws SQLException {
		drv.debugFunction(this);
		// FIXME: in progress...
		boolean	supportsmultipleresultsets=false;
		drv.debugPrintln("supports multiple result sets: "+
						supportsmultipleresultsets);
		drv.debugEnd();
		return supportsmultipleresultsets;
	}

	public
	boolean supportsMultipleTransactions() throws SQLException {
		drv.debugFunction(this);
		boolean	supportsmultipletransactions=false;
		drv.debugPrintln("supports multiple transactions: "+
						supportsmultipletransactions);
		drv.debugEnd();
		return supportsmultipletransactions;
	}

	public
	boolean supportsNamedParameters() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsnamedparameters=true;
		drv.debugPrintln("supports named parameters: "+
						supportsnamedparameters);
		drv.debugEnd();
		return supportsnamedparameters;
	}

	public
	boolean supportsNonNullableColumns() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsnonnullablecolumns=true;
		drv.debugPrintln("supports non-nullable columns: "+
						supportsnonnullablecolumns);
		drv.debugEnd();
		return supportsnonnullablecolumns;
	}

	public
	boolean supportsOpenCursorsAcrossCommit() throws SQLException {
		drv.debugFunction(this);
		// FIXME: not sure
		boolean	supportsopencursorsacrosscommit=false;
		drv.debugPrintln("supports open cursors across commit: "+
					supportsopencursorsacrosscommit);
		drv.debugEnd();
		return supportsopencursorsacrosscommit;
	}

	public
	boolean supportsOpenCursorsAcrossRollback() throws SQLException {
		drv.debugFunction(this);
		// FIXME: not sure
		boolean	supportsopencursorsacrossrollback=false;
		drv.debugPrintln("supports open cursors across rollback: "+
					supportsopencursorsacrossrollback);
		drv.debugEnd();
		return supportsopencursorsacrossrollback;
	}

	public
	boolean supportsOpenStatementsAcrossCommit() throws SQLException {
		drv.debugFunction(this);
		// FIXME: not sure
		boolean	supportsopenstatementsacrosscommit=false;
		drv.debugPrintln("supports open statements across commit: "+
					supportsopenstatementsacrosscommit);
		drv.debugEnd();
		return supportsopenstatementsacrosscommit;
	}

	public
	boolean supportsOpenStatementsAcrossRollback() throws SQLException {
		drv.debugFunction(this);
		// FIXME: not sure
		boolean	supportsopenstatementsacrossrollback=false;
		drv.debugPrintln("supports open statements "+
					"across rollback: "+
					supportsopenstatementsacrossrollback);
		drv.debugEnd();
		return supportsopenstatementsacrossrollback;
	}

	public
	boolean supportsOrderByUnrelated() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsorderbyunrelated=true;
		drv.debugPrintln("supports order by unrelated: "+
					supportsorderbyunrelated);
		drv.debugEnd();
		return supportsorderbyunrelated;
	}

	public
	boolean supportsOuterJoins() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsouterjoins=true;
		drv.debugPrintln("supports outer joins: "+
						supportsouterjoins);
		drv.debugEnd();
		return supportsouterjoins;
	}

	public
	boolean supportsPositionedDelete() throws SQLException {
		drv.debugFunction(this);
		boolean	supportspositioneddelete=false;
		drv.debugPrintln("supports positioned delete: "+
						supportspositioneddelete);
		drv.debugEnd();
		return supportspositioneddelete;
	}

	public
	boolean supportsPositionedUpdate() throws SQLException {
		drv.debugFunction(this);
		boolean	supportspositionedupdate=false;
		drv.debugPrintln("supports positioned update: "+
						supportspositionedupdate);
		drv.debugEnd();
		return supportspositionedupdate;
	}

	public
	boolean supportsResultSetConcurrency(int type,
						int concurrency)
						throws SQLException {
		drv.debugFunction(this);
		boolean	supportsresultsetconcurrency=
				(type==ResultSet.TYPE_FORWARD_ONLY &&
				concurrency==ResultSet.CONCUR_READ_ONLY);
		drv.debugPrintln("supports result set concurrency: "+
						supportsresultsetconcurrency);
		drv.debugEnd();
		return supportsresultsetconcurrency;
	}

	public
	boolean supportsResultSetHoldability(int holdability)
							throws SQLException {
		drv.debugFunction(this);
		boolean	supportsresultsetholdability=
			(holdability==ResultSet.CLOSE_CURSORS_AT_COMMIT);
		drv.debugPrintln("supports result set holdability: "+
						supportsresultsetholdability);
		drv.debugEnd();
		return supportsresultsetholdability;
	}

	public
	boolean supportsResultSetType(int type) throws SQLException {
		drv.debugFunction(this);
		boolean	supportsresultsettype=
			(type==ResultSet.TYPE_FORWARD_ONLY);
		drv.debugPrintln("supports result set type: "+
						supportsresultsettype);
		drv.debugEnd();
		return supportsresultsettype;
	}

	public
	boolean supportsSavepoints() throws SQLException {
		drv.debugFunction(this);
		boolean	supportssavepoints=false;
		drv.debugPrintln("supports savepoints: "+supportssavepoints);
		drv.debugEnd();
		return supportssavepoints;
	}

	public
	boolean supportsSchemasInDataManipulation() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsschemasindatamanipulation=true;
		drv.debugPrintln("supports schemas in data manipulation: "+
					supportsschemasindatamanipulation);
		drv.debugEnd();
		return supportsschemasindatamanipulation;
	}

	public
	boolean supportsSchemasInIndexDefinitions() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsschemasinindexdefinitions=true;
		drv.debugPrintln("supports schemas in index definitions: "+
					supportsschemasinindexdefinitions);
		drv.debugEnd();
		return supportsschemasinindexdefinitions;
	}

	public
	boolean supportsSchemasInPrivilegeDefinitions() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsschemasinprivilegedefinitions=true;
		drv.debugPrintln("supports schemas in "+
					"privilege definitions: "+
					supportsschemasinprivilegedefinitions);
		drv.debugEnd();
		return supportsschemasinprivilegedefinitions;
	}

	public
	boolean supportsSchemasInProcedureCalls() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsschemasinprocedurecalls=true;
		drv.debugPrintln("supports schemas in procedure calls: "+
					supportsschemasinprocedurecalls);
		drv.debugEnd();
		return supportsschemasinprocedurecalls;
	}

	public
	boolean supportsSchemasInTableDefinitions() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsschemasintabledefinitions=true;
		drv.debugPrintln("supports schemas in table definitions: "+
					supportsschemasintabledefinitions);
		drv.debugEnd();
		return supportsschemasintabledefinitions;
	}

	public
	boolean supportsSelectForUpdate() throws SQLException {
		drv.debugFunction(this);
		boolean	supportsselectforupdate=false;
		drv.debugPrintln("supports select for update: "+
						supportsselectforupdate);
		drv.debugEnd();
		return supportsselectforupdate;
	}

	public
	boolean supportsStatementPooling() throws SQLException {
		drv.debugFunction(this);
		boolean	supportsstatementpooling=false;
		drv.debugPrintln("supports statement pooling: "+
						supportsstatementpooling);
		drv.debugEnd();
		return supportsstatementpooling;
	}

	public
	boolean supportsStoredFunctionsUsingCallSyntax() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	ssfucs=false;
		drv.debugPrintln("supports stored functions "+
					"using call syntax: "+ssfucs);
		drv.debugEnd();
		return ssfucs;
	}

	public
	boolean supportsStoredProcedures() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsstoredprocedures=true;
		drv.debugPrintln("supports stored procedures: "+
						supportsstoredprocedures);
		drv.debugEnd();
		return supportsstoredprocedures;
	}

	public
	boolean supportsSubqueriesInComparisons() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportssubqueriesincomparisons=true;
		drv.debugPrintln("supports subqueries in comparisons: "+
					supportssubqueriesincomparisons);
		drv.debugEnd();
		return supportssubqueriesincomparisons;
	}

	public
	boolean supportsSubqueriesInExists() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportssubqueriesinexists=true;
		drv.debugPrintln("supports subqueries in exists: "+
						supportssubqueriesinexists);
		drv.debugEnd();
		return supportssubqueriesinexists;
	}

	public
	boolean supportsSubqueriesInIns() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportssubqueriesinins=true;
		drv.debugPrintln("supports subqueries in ins: "+
						supportssubqueriesinins);
		drv.debugEnd();
		return supportssubqueriesinins;
	}

	public
	boolean supportsSubqueriesInQuantifieds() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportssubqueriesinquantifieds=true;
		drv.debugPrintln("supports subqueries in quantifieds: "+
					supportssubqueriesinquantifieds);
		drv.debugEnd();
		return supportssubqueriesinquantifieds;
	}

	public
	boolean supportsTableCorrelationNames() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportssubqueriesincorrelationnames=true;
		drv.debugPrintln("supports subqueries in "+
					"correlation names: "+
					supportssubqueriesincorrelationnames);
		drv.debugEnd();
		return supportssubqueriesincorrelationnames;
	}

	public
	boolean supportsTransactionIsolationLevel(int level)
						throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportstransactionisolationlevel=true;
		drv.debugPrintln("supports transaction isolation level: "+
					supportstransactionisolationlevel);
		drv.debugEnd();
		return supportstransactionisolationlevel;
	}

	public
	boolean supportsTransactions() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportstransactions=true;
		drv.debugPrintln("supports transactions: "+
						supportstransactions);
		drv.debugEnd();
		return supportstransactions;
	}

	public
	boolean supportsUnion() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsunion=true;
		drv.debugPrintln("supports union: "+supportsunion);
		drv.debugEnd();
		return supportsunion;
	}

	public
	boolean supportsUnionAll() throws SQLException {
		drv.debugFunction(this);
		// FIXME: db-specific
		boolean	supportsunionall=true;
		drv.debugPrintln("supports union all: "+supportsunionall);
		drv.debugEnd();
		return supportsunionall;
	}

	public
	boolean updatesAreDetected(int type) throws SQLException {
		drv.debugFunction(this);
		boolean	updatesaredetected=false;
		drv.debugPrintln("updates are detected: "+
						updatesaredetected);
		drv.debugEnd();
		return updatesaredetected;
	}

	public
	boolean usesLocalFilePerTable() throws SQLException {
		drv.debugFunction(this);
		boolean	useslocalfilepertable=false;
		drv.debugPrintln("uses local file per table: "+
						useslocalfilepertable);
		drv.debugEnd();
		return useslocalfilepertable;
	}

	public
	boolean usesLocalFiles() throws SQLException {
		drv.debugFunction(this);
		boolean	useslocalfiles=false;
		drv.debugPrintln("uses local files: "+useslocalfiles);
		drv.debugEnd();
		return useslocalfiles;
	}

	public
	boolean isWrapperFor(Class<?> iface) throws SQLException {
		drv.debugFunction(this);
		drv.debugEnd();
		return false;
	}

	public
	<T> T unwrap(Class<T> iface) throws SQLException {
		drv.debugFunction(this);
		drv.debugEnd();
		return null;
	}
};
