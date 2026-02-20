package com.firstworks.sql;

import java.sql.*;
import java.util.regex.*;
import java.util.HashMap;

import com.firstworks.sqlrelay.*;

public class SQLRelayDatabaseMetaData implements DatabaseMetaData {

	private	Object			networklock;

	private	SQLRelayDriver		drv;
	private SQLRelayConnection	conn;

	public
	SQLRelayDatabaseMetaData(SQLRelayDriver driver) {
		this.drv=driver;
		drv.debugFunction(this);
		conn=null;
		networklock=null;
		drv.debugEnd();
	}

	private
	String getString(String feature) {
		return getDatabaseFeature(feature);
	}

	private
	int getInt(String feature) {
		String	value=getDatabaseFeature(feature);
		return (value!=null)?Integer.parseInt(value):0;
	}

	private
	boolean getBoolean(String feature) {
		String	value=getDatabaseFeature(feature);
		return (value!=null)?Boolean.parseBoolean(value):false;
	}

	private
	String getDatabaseFeature(String feature) {
		String	value;
		synchronized (networklock) {
			value=conn.getSQLRConnection().
					getDatabaseFeature(feature);
		}
		return value;
	}

	void setConnection(SQLRelayConnection connection) {
		this.conn=connection;
	}

	void setNetworkLock(Object networklock) {
		this.networklock=networklock;
	}

	public
	boolean allProceduresAreCallable() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("all_procedures_are_callable");
		drv.debugPrintln("all procedures are callable: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean allTablesAreSelectable() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("all_tables_are_selectable");
		drv.debugPrintln("all tables are selectable: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean autoCommitFailureClosesAllResultSets() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
				"auto_commit_failure_closes_all_result_sets");
		drv.debugPrintln("auto commit failures closes "+
						"all result sets: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean dataDefinitionCausesTransactionCommit() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
				"data_definition_causes_transaction_commit");
		drv.debugPrintln("data definition causes "+
					"transaction commit: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean dataDefinitionIgnoredInTransactions() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
				"data_definition_ignored_in_transactions");
		drv.debugPrintln("data definition ignored "+
					"in transactions: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean deletesAreDetected(int type) throws SQLException {
		drv.debugFunction(this);
		conn.debugResultSetType(type);
		boolean	result=
			conn.isResultSetTypeSupported(type) &&
			containsType("deletes_are_detected",type);
		drv.debugPrintln("deletes are detected: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean doesMaxRowSizeIncludeBlobs() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("does_max_row_size_include_blobs");
		drv.debugPrintln("does max row size include blobs: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean generatedKeyAlwaysReturned() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("generated_key_always_returned");
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
		// few jdbc drivers (or databases) support this
		conn.throwFeatureNotSupportedException();
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
		String	separator=getString("catalog_separator");
		drv.debugPrintln("catalog separator: "+separator);
		drv.debugEnd();
		return separator;
	}

	public
	String getCatalogTerm() throws SQLException {
		drv.debugFunction(this);
		String	term=getString("catalog_term");
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
		int	result=getDatabaseVersion(true);
		drv.debugPrintln("major version: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getDatabaseMinorVersion() throws SQLException {
		drv.debugFunction(this);
		int	result=getDatabaseVersion(false);
		drv.debugPrintln("minor version: "+result);
		drv.debugEnd();
		return result;
	}

	private
	int getDatabaseVersion(boolean major) {
		drv.debugFunction(this);
		String	dbversion=null;
		synchronized (networklock) {
			dbversion=conn.getSQLRConnection().dbVersion();
		}
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
		String	result=null;
		synchronized (networklock) {
			result=conn.getSQLRConnection().dbVersion();
		}
		drv.debugPrintln("product version: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getDefaultTransactionIsolation() throws SQLException {
		drv.debugFunction(this);
		int	isolation=getTransactionIsolationLevel(
				getString("default_isolation_level"));
		drv.debugPrintln("isolation: "+isolation);
		drv.debugEnd();
		return isolation;
	}

	public
	int getDriverMajorVersion() {
		drv.debugFunction(this);
		int		majorversion=-1;
		String[]	parts=conn.getSQLRConnection().
						clientVersion().split("\\.");
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
		String[]	parts=conn.getSQLRConnection().
						clientVersion().split("\\.");
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
		String	drivername="SQL Relay JDBC driver";
		drv.debugPrintln("driver name: "+drivername);
		drv.debugEnd();
		return drivername;
	}

	public
	String getDriverVersion() throws SQLException {
		drv.debugFunction(this);
		String	result=conn.getSQLRConnection().clientVersion();
		drv.debugPrintln("driver version: "+result);
		drv.debugEnd();
		return result;
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
		String	result=getString("extra_name_characters");
		drv.debugPrintln("extra name characters: "+result);
		drv.debugEnd();
		return result;
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
		// sqlrcur.getProcedureBindAndColumnList()?
		drv.debugPrintln("FIXME: implement this");
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

		// FIXME: implement with sqlrcur.getProcedures()?
		drv.debugPrintln("FIXME: implement this");
		drv.debugEnd();
		return null;
	}

	public
	String getIdentifierQuoteString() throws SQLException {
		drv.debugFunction(this);
		String	result=getString("identifier_quote_string");
		drv.debugPrintln("identifier quote string: "+result);
		drv.debugEnd();
		return result;
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

		// FIXME: implement with sqlrcur.getKeyAndIndexList() ?
		drv.debugPrintln("FIXME: implement this");
		drv.debugEnd();
		return null;
	}

	public
	int getJDBCMajorVersion() throws SQLException {
		drv.debugFunction(this);
		int	result=4;
		drv.debugPrintln("jdbc major version: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getJDBCMinorVersion() throws SQLException {
		drv.debugFunction(this);
		int	result=2;
		drv.debugPrintln("jdbc minor version: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxBinaryLiteralLength() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_binary_literal_length");
		drv.debugPrintln("max binary literal length: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxCatalogNameLength() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_catalog_name_length");
		drv.debugPrintln("max catalog name length: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxCharLiteralLength() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_char_literal_length");
		drv.debugPrintln("max char literal length: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxColumnNameLength() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_column_name_length");
		drv.debugPrintln("max column name length: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxColumnsInGroupBy() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_columns_in_group_by");
		drv.debugPrintln("max columns in group: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxColumnsInIndex() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_columns_in_index");
		drv.debugPrintln("max columns in index: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxColumnsInOrderBy() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_columns_in_order_by");
		drv.debugPrintln("max columns in order by: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxColumnsInSelect() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_columns_in_select");
		drv.debugPrintln("max columns in select: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxColumnsInTable() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_columns_in_table");
		drv.debugPrintln("max columns in table: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxConnections() throws SQLException {
		// FIXME: shouldn't this be sqlrelay's max connections?
		drv.debugFunction(this);
		int	result=getInt("max_connections");
		drv.debugPrintln("max connections: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxCursorNameLength() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_cursor_name_length");
		drv.debugPrintln("max cursor name length: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxIndexLength() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_index_length");
		drv.debugPrintln("max index length: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxProcedureNameLength() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_procedure_name_length");
		drv.debugPrintln("max procedure name length: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxRowSize() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_row_size");
		drv.debugPrintln("max row size: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxSchemaNameLength() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_schema_name_length");
		drv.debugPrintln("max schema name length: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxStatementLength() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_statement_length");
		drv.debugPrintln("max statement length: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxStatements() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_statements");
		drv.debugPrintln("max statements: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxTableNameLength() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_table_name_length");
		drv.debugPrintln("max table name length: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxTablesInSelect() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_tables_in_select");
		drv.debugPrintln("max tables in select: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getMaxUserNameLength() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("max_user_name_length");
		drv.debugPrintln("max user name length: "+result);
		drv.debugEnd();
		return result;
	}

	public
	String getNumericFunctions() throws SQLException {
		drv.debugFunction(this);
		String	result=getString("numeric_functions");
		drv.debugPrintln("numeric functions: "+result);
		drv.debugEnd();
		return result;
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

		// FIXME: implement with sqlrcon.getPrimaryKeysList()
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

		// FIXME: implement with sqlrcon.getProcedureBindAndColumnList()
		drv.debugPrintln("FIXME: implement this");
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
		String	result=getString("procedure_term");
		drv.debugPrintln("procedure term: "+result);
		drv.debugEnd();
		return result;
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
		int	result=getInt("result_set_holdability");
		drv.debugPrintln("result set holdability: "+result);
		drv.debugEnd();
		return result;
	}

	public
	RowIdLifetime getRowIdLifetime() throws SQLException {
		drv.debugFunction(this);
		String	result=getString("row_id_lifetime");
		drv.debugPrintln("rowid lifetime string: "+result);
		RowIdLifetime	rowidlifetime=RowIdLifetime.ROWID_UNSUPPORTED;
		switch (getString("row_id_lifetime")) {
			case "ROWID_VALID_OTHER":
				rowidlifetime=
					RowIdLifetime.ROWID_VALID_OTHER;
				break;
			case "ROWID_VALID_TRANSACTION":
				rowidlifetime=
					RowIdLifetime.ROWID_VALID_TRANSACTION;
				break;
			case "ROWID_VALID_SESSION":
				rowidlifetime=
					RowIdLifetime.ROWID_VALID_SESSION;
				break;
			case "ROWID_VALID_FOREVER":
				rowidlifetime=
					RowIdLifetime.ROWID_VALID_FOREVER;
				break;
		}
		debugRowIdLifetime(rowidlifetime);
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
		String	result=getString("schema_term");
		drv.debugPrintln("schema term: "+result);
		drv.debugEnd();
		return result;
	}

	public
	String getSearchStringEscape() throws SQLException {
		drv.debugFunction(this);
		String	result=getString("search_string_escape");
		drv.debugPrintln("search string escape: "+result);
		drv.debugEnd();
		return result;
	}

	public
	String getSQLKeywords() throws SQLException {
		drv.debugFunction(this);
		String	result=getString("sql_keywords");
		drv.debugPrintln("sql keywords: "+result);
		drv.debugEnd();
		return result;
	}

	public
	int getSQLStateType() throws SQLException {
		drv.debugFunction(this);
		int	result=getInt("sql_state_type");
		drv.debugPrintln("sql state type: "+result);
		drv.debugEnd();
		return result;
	}

	public
	String getStringFunctions() throws SQLException {
		drv.debugFunction(this);
		String	result=getString("string_functions");
		drv.debugPrintln("string functions: "+result);
		drv.debugEnd();
		return result;
	}

	public
	ResultSet getSuperTables(String catalog,
					String schemaPattern,
					String tableNamePattern)
					throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("catalog: "+catalog);
		drv.debugPrintln("schema pattern: "+schemaPattern);
		drv.debugPrintln("type name pattern: "+tableNamePattern);
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
		// few jdbc drivers (or databases) support this
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	String getSystemFunctions() throws SQLException {
		drv.debugFunction(this);
		String	result=getString("system_functions");
		drv.debugPrintln("system functions: "+result);
		drv.debugEnd();
		return result;
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

	private
	String buildWild(String catalog, String schema, String object) {
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
		String	result=getString("time_date_functions");
		drv.debugPrintln("timedate functions: "+result);
		drv.debugEnd();
		return result;
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
		String	url=conn.getURL();
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
		conn.debugResultSetType(type);
		boolean	result=
			conn.isResultSetTypeSupported(type) &&
			containsType("inserts_are_detected",type);
		drv.debugPrintln("inserts are detected: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean isCatalogAtStart() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("is_catalog_at_start");
		drv.debugPrintln("is catalog at start: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean isReadOnly() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("is_read_only");
		drv.debugPrintln("is read only: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean locatorsUpdateCopy() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("locators_update_copy");
		drv.debugPrintln("locators update copy: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean nullPlusNonNullIsNull() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("null_plus_non_null_is_null");
		drv.debugPrintln("null plus non null is null: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean nullsAreSortedAtEnd() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("nulls_are_sorted_at_end");
		drv.debugPrintln("nulls are sorted at end: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean nullsAreSortedAtStart() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("nulls_are_sorted_at_start");
		drv.debugPrintln("nulls are sorted at start: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean nullsAreSortedHigh() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("nulls_are_sorted_high");
		drv.debugPrintln("nulls are sorted high: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean nullsAreSortedLow() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("nulls_are_sorted_low");
		drv.debugPrintln("nulls are sorted low: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean othersDeletesAreVisible(int type) throws SQLException {
		drv.debugFunction(this);
		conn.debugResultSetType(type);
		boolean	result=
			conn.isResultSetTypeSupported(type) &&
			containsType("others_deletes_are_visible",type);
		drv.debugPrintln("others deletes are visible: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean othersInsertsAreVisible(int type) throws SQLException {
		drv.debugFunction(this);
		conn.debugResultSetType(type);
		boolean	result=
			conn.isResultSetTypeSupported(type) &&
			containsType("others_inserts_are_visible",type);
		drv.debugPrintln("others inserts are visible: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean othersUpdatesAreVisible(int type) throws SQLException {
		drv.debugFunction(this);
		conn.debugResultSetType(type);
		boolean	result=
			conn.isResultSetTypeSupported(type) &&
			containsType("others_updates_are_visible",type);
		drv.debugPrintln("others updates are visible: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean ownDeletesAreVisible(int type) throws SQLException {
		drv.debugFunction(this);
		conn.debugResultSetType(type);
		boolean	result=
			conn.isResultSetTypeSupported(type) &&
			containsType("own_deletes_are_visible",type);
		drv.debugPrintln("own deletes are visible: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean ownInsertsAreVisible(int type) throws SQLException {
		drv.debugFunction(this);
		conn.debugResultSetType(type);
		boolean	result=
			conn.isResultSetTypeSupported(type) &&
			containsType("own_inserts_are_visible",type);
		drv.debugPrintln("own inserts are visible: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean ownUpdatesAreVisible(int type) throws SQLException {
		drv.debugFunction(this);
		conn.debugResultSetType(type);
		boolean	result=
			conn.isResultSetTypeSupported(type) &&
			containsType("own_updates_are_visible",type);
		drv.debugPrintln("own updates are visible: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean storesLowerCaseIdentifiers() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("stores_lower_case_identifiers");
		drv.debugPrintln("stores lower case identifiers: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean storesLowerCaseQuotedIdentifiers() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"stores_lower_case_quoted_identifiers");
		drv.debugPrintln(
			"stores lower case quoted identifiers: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean storesMixedCaseIdentifiers() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("stores_mixed_case_identifiers");
		drv.debugPrintln("stores mixed case identifiers: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean storesMixedCaseQuotedIdentifiers() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"stores_mixed_case_quoted_identifiers");
		drv.debugPrintln(
			"stores mixed case quoted identifiers: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean storesUpperCaseIdentifiers() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("stores_upper_case_identifiers");
		drv.debugPrintln("stores upper case identifiers: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean storesUpperCaseQuotedIdentifiers() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"stores_upper_case_quoted_identifiers");
		drv.debugPrintln(
			"stores upper case quoted identifiers: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsAlterTableWithAddColumn() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_alter_table_with_add_column");
		drv.debugPrintln(
			"supports alter table with add command: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsAlterTableWithDropColumn() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_alter_table_with_drop_column");
		drv.debugPrintln(
			"supports alter table with drop command: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsANSI92EntryLevelSQL() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_ansi92_entry_level_sql");
		drv.debugPrintln("supports ansi92 entry level sql: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsANSI92FullSQL() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_ansi92_full_sql");
		drv.debugPrintln("supports ansi92 full sql: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsANSI92IntermediateSQL() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_ansi92_intermediate_sql");
		drv.debugPrintln("supports ansi92 intermediate sql: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsBatchUpdates() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_batch_updates");
		drv.debugPrintln("supports batch updates: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsCatalogsInDataManipulation() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_catalogs_in_data_manipulation");
		drv.debugPrintln(
			"supports catalogs in data manipulations: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsCatalogsInIndexDefinitions() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_catalogs_in_index_definitions");
		drv.debugPrintln(
			"supports catalogs in index definitions: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsCatalogsInPrivilegeDefinitions() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_catalogs_in_privilege_definitions");
		drv.debugPrintln(
			"supports catalogs in privilege definitions: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsCatalogsInProcedureCalls() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_catalogs_in_procedure_calls");
		drv.debugPrintln(
			"supports catalogs in procedure calls: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsCatalogsInTableDefinitions() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_catalogs_in_table_definitions");
		drv.debugPrintln(
			"supports catalogs in table definitions: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsColumnAliasing() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_column_aliasing");
		drv.debugPrintln("supports column aliasing: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsConvert() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_convert");
		drv.debugPrintln("supports convert: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsConvert(int fromType, int toType) throws SQLException {
		drv.debugFunction(this);

		drv.debugPrintln("from type: "+fromType);
		drv.debugPrintln("to type: "+toType);

		boolean	result=true;
		// FIXME: query the db directly for each, cache
		drv.debugPrintln("supports convert: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsCoreSQLGrammar() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_core_sql_grammar");
		drv.debugPrintln("supports core sql grammar: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsCorrelatedSubqueries() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_correlated_subqueries");
		drv.debugPrintln("supports correlated subqueries: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsDataDefinitionAndDataManipulationTransactions()
							throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
		"supports_data_definition_and_data_manipulation_transactions");
		drv.debugPrintln(
		"supports data definition and data manipulation transactions: "+
		result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsDataManipulationTransactionsOnly() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
		"supports_data_manipulation_transactions_only");
		drv.debugPrintln(
		"supports data manipulation transactions only: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsDifferentTableCorrelationNames() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_different_table_correlation_names");
		drv.debugPrintln(
			"supports different table correlation names: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsExpressionsInOrderBy() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_expressions_in_order_by");
		drv.debugPrintln("supports expressions in order by: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsExtendedSQLGrammar() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_extended_sql_grammar");
		drv.debugPrintln("supports extended sql grammar: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsFullOuterJoins() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_full_outer_joins");
		drv.debugPrintln("supports full outer joins: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsGetGeneratedKeys() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_get_generated_keys");
		drv.debugPrintln("supports get generated keys: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsGroupBy() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_group_by");
		drv.debugPrintln("supports group by: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsGroupByBeyondSelect() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_group_by_beyond_select");
		drv.debugPrintln("supports group by beyond select: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsGroupByUnrelated() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_group_by_unrelated");
		drv.debugPrintln("supports group by unrelated: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsIntegrityEnhancementFacility() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_integrity_enhancement_facility");
		drv.debugPrintln(
			"supports integrity enhancement facility: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsLikeEscapeClause() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_like_escape_clause");
		drv.debugPrintln("supports like escape clause: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsLimitedOuterJoins() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_limited_outer_joins");
		drv.debugPrintln("supports limited outer joins: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsMinimumSQLGrammar() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_minimum_sql_grammar");
		drv.debugPrintln("supports minimum sql grammar: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsMixedCaseIdentifiers() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_mixed_case_identifiers");
		drv.debugPrintln("supports mixed case identifiers: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsMixedCaseQuotedIdentifiers() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_mixed_case_quoted_identifiers");
		drv.debugPrintln(
			"supports mixed case quoted identifiers: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsMultipleOpenResults() throws SQLException {
		drv.debugFunction(this);
		// SQL Relay doesn't currenlty support multiple open results,
		// except via bind cursors, but I don't think that's what this
		// refers to
		boolean	result=false;
		drv.debugPrintln("supports multiple open results: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsMultipleResultSets() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_multiple_result_sets");
		drv.debugPrintln("supports multiple result sets: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsMultipleTransactions() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_multiple_transactions");
		drv.debugPrintln("supports multiple transactions: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsNamedParameters() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_named_parameters");
		drv.debugPrintln("supports named parameters: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsNonNullableColumns() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_non_nullable_columns");
		drv.debugPrintln("supports non-nullable columns: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsOpenCursorsAcrossCommit() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_open_cursors_across_commit");
		drv.debugPrintln(
			"supports open cursors across commit: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsOpenCursorsAcrossRollback() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_open_cursors_across_rollback");
		drv.debugPrintln(
			"supports open cursors across rollback: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsOpenStatementsAcrossCommit() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_open_statements_across_commit");
		drv.debugPrintln(
			"supports open statements across commit: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsOpenStatementsAcrossRollback() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_open_statements_across_rollback");
		drv.debugPrintln(
			"supports open statements across rollback: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsOrderByUnrelated() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_order_by_unrelated");
		drv.debugPrintln("supports order by unrelated: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsOuterJoins() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_outer_joins");
		drv.debugPrintln("supports outer joins: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsPositionedDelete() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_positioned_delete");
		drv.debugPrintln("supports positioned delete: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsPositionedUpdate() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_positioned_update");
		drv.debugPrintln("supports positioned update: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsResultSetConcurrency(int type,
						int concurrency)
						throws SQLException {
		drv.debugFunction(this);
		conn.debugResultSetType(type);
		conn.debugResultSetConcurrency(concurrency);
		boolean	result=
			conn.isResultSetTypeSupported(type) &&
			conn.isResultSetConcurrencySupported(concurrency) &&
			containsConcurrency("supports_result_set_concurrency",
							type,concurrency);
		drv.debugPrintln("supports result set concurrency: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsResultSetHoldability(int holdability)
							throws SQLException {
		drv.debugFunction(this);
		conn.debugResultSetType(holdability);
		boolean	result=
			conn.isResultSetHoldabilitySupported(holdability) &&
			containsHoldability("supports_result_set_holdability",
								holdability);
		drv.debugPrintln("supports result set holdability: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsResultSetType(int type) throws SQLException {
		drv.debugFunction(this);
		boolean	result=
			conn.isResultSetTypeSupported(type) &&
			containsType("supports_result_set_type",type);
		drv.debugPrintln("supports result set type: "+type);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsSavepoints() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_savepoints");
		drv.debugPrintln("supports savepoints: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsSchemasInDataManipulation() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_schemas_in_data_manipulation");
		drv.debugPrintln(
			"supports schemas in data manipulation: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsSchemasInIndexDefinitions() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_schemas_in_index_definitions");
		drv.debugPrintln(
			"supports schemas in index definitions: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsSchemasInPrivilegeDefinitions() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_schemas_in_privilege_definitions");
		drv.debugPrintln(
			"supports schemas in privilege definitions: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsSchemasInProcedureCalls() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_schemas_in_procedure_calls");
		drv.debugPrintln(
			"supports schemas in procedure calls: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsSchemasInTableDefinitions() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_schemas_in_table_definitions");
		drv.debugPrintln(
			"supports schemas in table definitions: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsSelectForUpdate() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_select_for_update");
		drv.debugPrintln("supports select for update: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsStatementPooling() throws SQLException {
		drv.debugFunction(this);
		boolean	result=true;
		drv.debugPrintln("supports statement pooling: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsStoredFunctionsUsingCallSyntax() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean(
			"supports_stored_functions_using_call_syntax");
		drv.debugPrintln(
			"supports stored functions using call syntax: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsStoredProcedures() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_stored_procedures");
		drv.debugPrintln("supports stored procedures: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsSubqueriesInComparisons() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_subqueries_in_comparisons");
		drv.debugPrintln("supports subqueries in comparisons: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsSubqueriesInExists() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_subqueries_in_exists");
		drv.debugPrintln("supports subqueries in exists: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsSubqueriesInIns() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_subqueries_in_ins");
		drv.debugPrintln("supports subqueries in ins: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsSubqueriesInQuantifieds() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_subqueries_in_quantifieds");
		drv.debugPrintln("supports subqueries in quantifieds: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsTableCorrelationNames() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_table_correlation_names");
		drv.debugPrintln("supports table correlation names: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsTransactionIsolationLevel(int level)
						throws SQLException {
		drv.debugFunction(this);
		String	value=getDatabaseFeature(
				"supports_transaction_isolation_level");
		if (level==Connection.TRANSACTION_NONE) {
			return (value==null || value.isEmpty());
		}
		String	levelname=
			getTransactionIsolationLevelName(level);
		if (value==null || levelname.isEmpty()) {
			return false;
		}
		for (String part : value.split(",")) {
			if (part.trim().equals(levelname)) {
				return true;
			}
		}
		return false;
	}

	public
	boolean supportsTransactions() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_transactions");
		drv.debugPrintln("supports transactions: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsUnion() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_union");
		drv.debugPrintln("supports union: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean supportsUnionAll() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("supports_union_all");
		drv.debugPrintln("supports union all: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean updatesAreDetected(int type) throws SQLException {
		drv.debugFunction(this);
		conn.debugResultSetType(type);
		boolean	result=
			conn.isResultSetTypeSupported(type) &&
			containsType("updates_are_detected",type);
		drv.debugPrintln("updates are detected: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean usesLocalFilePerTable() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("uses_local_file_per_table");
		drv.debugPrintln("uses local file per table: "+result);
		drv.debugEnd();
		return result;
	}

	public
	boolean usesLocalFiles() throws SQLException {
		drv.debugFunction(this);
		boolean	result=getBoolean("uses_local_files");
		drv.debugPrintln("uses local files: "+result);
		drv.debugEnd();
		return result;
	}

	private
	String getTypeName(int type) {
		switch (type) {
			case ResultSet.TYPE_FORWARD_ONLY:
				return "FORWARD_ONLY";
			case ResultSet.TYPE_SCROLL_INSENSITIVE:
				return "SCROLL_INSENSITIVE";
			case ResultSet.TYPE_SCROLL_SENSITIVE:
				return "SCROLL_SENSITIVE";
			default:
				return "";
		}
	}

	private
	String getConcurrencyName(int concurrency) {
		switch (concurrency) {
			case ResultSet.CONCUR_READ_ONLY:
				return "READ_ONLY";
			case ResultSet.CONCUR_UPDATABLE:
				return "UPDATABLE";
			default:
				return "";
		}
	}

	private
	String getHoldabilityName(int holdability) {
		switch (holdability) {
			case ResultSet.HOLD_CURSORS_OVER_COMMIT:
				return "HOLD_CURSORS_OVER_COMMIT";
			case ResultSet.CLOSE_CURSORS_AT_COMMIT:
				return "CLOSE_CURSORS_AT_COMMIT";
			default:
				return "";
		}
	}

	private
	boolean containsType(String feature, int type) {
		String	value=getDatabaseFeature(feature);
		if (value==null || value.isEmpty()) {
			return false;
		}
		String	typename=getTypeName(type);
		if (typename.isEmpty()) {
			return false;
		}
		String[]	parts=value.split(",");
		for (String part : parts) {
			if (part.trim().equals(typename)) {
				return true;
			}
		}
		return false;
	}

	private
	boolean containsConcurrency(String feature,
					int type,int concurrency) {
		String	value=getDatabaseFeature(feature);
		String	typename=getTypeName(type);
		String	concurrencyname=getConcurrencyName(concurrency);
		if (value==null ||
			typename.isEmpty() ||
			concurrencyname.isEmpty()) {
			return false;
		}
		String	pair=typename+"/"+concurrencyname;
		for (String part : value.split(",")) {
			if (part.trim().equals(pair)) {
				return true;
			}
		}
		return false;
	}

	private
	boolean containsHoldability(String feature, int holdability) {
		String	value=getDatabaseFeature(feature);
		String	holdabilityname=getHoldabilityName(holdability);
		if (value==null || holdabilityname.isEmpty()) {
			return false;
		}
		for (String part : value.split(",")) {
			if (part.trim().equals(holdabilityname)) {
				return true;
			}
		}
		return false;
	}

	private
	String getTransactionIsolationLevelName(int level) {
		switch (level) {
			case Connection.TRANSACTION_READ_UNCOMMITTED:
				return "READ_UNCOMMITTED";
			case Connection.TRANSACTION_READ_COMMITTED:
				return "READ_COMMITTED";
			case Connection.TRANSACTION_REPEATABLE_READ:
				return "REPEATABLE_READ";
			case Connection.TRANSACTION_SERIALIZABLE:
				return "SERIALIZABLE";
			default:
				return "";
		}
	}

	private
	int getTransactionIsolationLevel(String level) {
		if (level==null) {
			return Connection.TRANSACTION_NONE;
		}
		switch (level) {
			case "READ_UNCOMMITTED":
				return Connection.TRANSACTION_READ_UNCOMMITTED;
			case "READ_COMMITTED":
				return Connection.TRANSACTION_READ_COMMITTED;
			case "REPEATABLE_READ":
				return Connection.TRANSACTION_REPEATABLE_READ;
			case "SERIALIZABLE":
				return Connection.TRANSACTION_SERIALIZABLE;
			default:
				return Connection.TRANSACTION_NONE;
		}
	}

	private
	void debugRowIdLifetime(RowIdLifetime rowidlifetime) {
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
			default:
				drv.debugPrintln("rowid lifetime: "+
							"unknown - "+
							rowidlifetime);
		}
	}

	public
	boolean isWrapperFor(Class<?> iface) throws SQLException {
		drv.debugFunction(this);
		drv.debugEnd();
		return (iface==SQLRConnection.class);
	}

	@SuppressWarnings({"unchecked"})
	public
	<T> T unwrap(Class<T> iface) throws SQLException {
		drv.debugFunction(this);
		drv.debugEnd();
		return (T)((iface==SQLRConnection.class)?conn:null);
	}
};
