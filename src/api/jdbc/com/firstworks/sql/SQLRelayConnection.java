package com.firstworks.sql;

import java.sql.*;

import java.util.Properties;
import java.util.Map;
import java.util.concurrent.Executor;

import com.firstworks.sqlrelay.*;

public class SQLRelayConnection implements Connection {

	private String		host;
	private short		port;
	private String		socket;
	private String		user;
	private String		password;
	private SQLRConnection	sqlrcon;
	private boolean		readonly;
	private Properties	clientinfo;
	private int		txisolevel;
	private boolean		autocommit;
	private int		networktimeout;

	private Map<String,Class<?>>	typemap;

	private	SQLRelayDriver	driver;

	public SQLRelayConnection(String host,
					short port,
					String socket,
					String user,
					String password,
					int retrytime,
					int tries,
					SQLRelayDriver driver)
					throws SQLException {

		this.driver=driver;

		driver.debugFunction(this);

		this.host=host;
		this.port=port;
		this.socket=socket;
		this.user=user;
		this.password=password;

		driver.debugPrintln("host: ",host);
		driver.debugPrintln("port: ",port);
		driver.debugPrintln("socket: ",socket);
		driver.debugPrintln("user: ",user);
		driver.debugPrintln("password: ",password);
		driver.debugPrintln("retrytime: ",retrytime);
		driver.debugPrintln("tries: ",tries);

		sqlrcon=new SQLRConnection(host,port,socket,
						user,password,retrytime,tries);
		readonly=false;
		clientinfo=new Properties();
		// FIXME: defaults to repeatable read on mysql5+
		txisolevel=Connection.TRANSACTION_READ_COMMITTED;
		// FIXME: might not be false, need to get this from server
		autocommit=false;
		// FIXME: the timeout can also be set using an environment
		// variable, so we should get this from the underlying api
		// instead of tracking it here
		networktimeout=0;
		typemap=null;

		if (driver.debug) {
			//sqlrcon.debugOn();
		}
		driver.debugEnd();
	}

	public synchronized
	String getHost() {
		driver.debugFunction(this);
		driver.debugPrintln("host: ",host);
		driver.debugEnd();
		return host;
	}

	public synchronized
	short getPort() {
		driver.debugFunction(this);
		driver.debugPrintln("port: ",port);
		driver.debugEnd();
		return port;
	}

	public synchronized
	String getSocket() {
		driver.debugFunction(this);
		driver.debugPrintln("socket: ",socket);
		driver.debugEnd();
		return socket;
	}

	public synchronized
	String getUser() {
		driver.debugFunction(this);
		driver.debugPrintln("user: ",user);
		driver.debugEnd();
		return user;
	}

	public synchronized
	String getPassword() {
		driver.debugFunction(this);
		driver.debugPrintln("password: ",password);
		driver.debugEnd();
		return password;
	}

	public synchronized
	void abort(Executor executor) throws SQLException {
		driver.debugFunction(this);
		close();
		driver.debugEnd();
	}

	public synchronized
	void clearWarnings() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugEnd();
	}

	public synchronized
	void close() throws SQLException {
		driver.debugFunction(this);
		sqlrcon.endSession();
		sqlrcon=null;
		driver.debugEnd();
	}

	public synchronized
	void commit() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		if (!sqlrcon.commit()) {
			throwErrorMessageException();
		}
		driver.debugEnd();
	}

	public synchronized
	Array createArrayOf(String typeName,
					Object[] elements)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Blob createBlob() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this...
		driver.debugEnd();
		return null;
	}

	public synchronized
	Clob createClob() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this...
		driver.debugEnd();
		return null;
	}

	public synchronized
	NClob createNClob() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this...
		driver.debugEnd();
		return null;
	}

	public synchronized
	SQLXML createSQLXML() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Statement createStatement() throws SQLException {
		driver.debugFunction(this);
		Statement	stmt=createStatement(
					ResultSet.TYPE_FORWARD_ONLY,
					ResultSet.CONCUR_READ_ONLY,
					ResultSet.HOLD_CURSORS_OVER_COMMIT);
		driver.debugEnd();
		return stmt;
	}

	public synchronized
	Statement createStatement(int resultSetType,
					int resultSetConcurrency)
					throws SQLException {
		driver.debugFunction(this);
		Statement	stmt=createStatement(
					resultSetType,
					resultSetConcurrency,
					ResultSet.HOLD_CURSORS_OVER_COMMIT);
		driver.debugEnd();
		return stmt;
	}

	public synchronized
	Statement createStatement(int resultSetType,
						int resultSetConcurrency,
						int resultSetHoldability)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();

		// unsupported options
		if (resultSetType==ResultSet.TYPE_SCROLL_SENSITIVE) {
			driver.debugPrintln("result set type: ",
						"TYPE_SCROLL_SENSITIVE");
			throwFeatureNotSupportedException();
		}
		if (resultSetConcurrency==ResultSet.CONCUR_UPDATABLE) {
			driver.debugPrintln("result set concurrency: ",
						"CONCUR_UPDATABLE");
			throwFeatureNotSupportedException();
		}
		if (resultSetHoldability==ResultSet.CLOSE_CURSORS_AT_COMMIT) {
			driver.debugPrintln("result set holdability: ",
						"CLOSE_CURSORS_AT_COMMIT");
			throwFeatureNotSupportedException();
		}

		// create a cursor
		SQLRCursor	sqlrcur=new SQLRCursor(sqlrcon);
		sqlrcur.getNullsAsNulls();

		// set result set buffer size as appropriate
		switch (resultSetType) {
			case ResultSet.TYPE_FORWARD_ONLY:
				driver.debugPrintln("result set type: ",
						"TYPE_FORWARD_ONLY");
				// FIXME: this can probably be set
				// to something bigger than 1
				sqlrcur.setResultSetBufferSize(1);
				break;
			case ResultSet.TYPE_SCROLL_INSENSITIVE:
				driver.debugPrintln("result set type: ",
						"TYPE_SCROLL_INSENSITIVE");
				sqlrcur.setResultSetBufferSize(0);
				break;
		}
		driver.debugPrintln("result set concurrency: ",
						"CONCUR_READ_ONLY");
		driver.debugPrintln("result set holdability: ",
						"HOLD_CURSORS_OVER_COMMIT");

		// create a statement, attach the cursor to the statement
		SQLRelayStatement	sqlrstmt=
					new SQLRelayStatement(driver);
		sqlrstmt.setConnection(this);
		sqlrstmt.setSQLRConnection(sqlrcon);
		sqlrstmt.setSQLRCursor(sqlrcur);

		driver.debugEnd();
		return sqlrstmt;
	}

	public synchronized
	Struct createStruct(String typeName, Object[] attributes)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		return null;
	}

	public synchronized
	boolean getAutoCommit() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("autocommit: ",autocommit);
		driver.debugEnd();
		return autocommit;
	}

	public synchronized
	String getCatalog() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		String	catalog=sqlrcon.getCurrentDatabase();
		driver.debugPrintln("catalog: ",catalog);
		driver.debugEnd();
		return catalog;
	}

	public synchronized
	Properties getClientInfo() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("clientinfo: ",clientinfo);
		driver.debugEnd();
		return clientinfo;
	}

	public synchronized
	String getClientInfo(String name) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		String	prop=getClientInfo().getProperty(name);
		driver.debugPrintln(name,": ",prop);
		driver.debugEnd();
		return prop;
	}

	public synchronized
	int getHoldability() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("holdability: HOLD_CURSORS_OVER_COMMIT");
		driver.debugEnd();
		return ResultSet.HOLD_CURSORS_OVER_COMMIT;
	}

	public synchronized
	DatabaseMetaData getMetaData() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayDatabaseMetaData	metadata=
				new SQLRelayDatabaseMetaData(driver);
		metadata.setConnection(this);
		driver.debugEnd();
		return metadata;
	}

	public synchronized
	int getNetworkTimeout() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		// FIXME: the timeout can also be set using an environment
		// variable, so we should get this from the underlying api
		// instead of tracking it here
		driver.debugEnd();
		return networktimeout;
	}

	public synchronized
	String getSchema() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		String	schema=sqlrcon.getCurrentSchema();
		driver.debugPrintln("schema: ",schema);
		driver.debugEnd();
		return schema;
	}

	public synchronized
	int getTransactionIsolation() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("isolation level: ",txisolevel);
		driver.debugEnd();
		return txisolevel;
	}

	public synchronized
	Map<String,Class<?>> getTypeMap() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugEnd();
		return typemap;
	}

	public synchronized
	SQLWarning getWarnings() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		// sqlrelay doesn't support anything like this
		driver.debugEnd();
		return null;
	}

	public synchronized
	boolean isClosed() throws SQLException {
		driver.debugFunction(this);
		boolean	isclosed=(sqlrcon==null);
		driver.debugPrintln("isclosed: ",isclosed);
		driver.debugEnd();
		return isclosed;
	}

	public synchronized
	boolean isReadOnly() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("read only: ",readonly);
		driver.debugEnd();
		return readonly;
	}

	public synchronized
	boolean isValid(int timeout) throws SQLException {
		driver.debugFunction(this);
		if (isClosed()) {
			driver.debugEnd();
			return false;
		}
		// FIXME: need to get the current response timeout pre-ping
		// and reset it post-ping, but the java api doesn't currently
		// have getResponseTimeout methods
		sqlrcon.setResponseTimeout(timeout,0);
		boolean	ping=sqlrcon.ping();
		driver.debugPrintln("ping: ",ping);
		driver.debugEnd();
		return ping;
	}

	public synchronized
	String nativeSQL(String sql) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("sql: ",sql);
		driver.debugEnd();
		return sql;
	}

	public synchronized
	CallableStatement prepareCall(String sql) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRCursor	sqlrcur=new SQLRCursor(sqlrcon);
		sqlrcur.getNullsAsNulls();
		sqlrcur.prepareQuery(sql);
		SQLRelayCallableStatement	sqlrstmt=
				new SQLRelayCallableStatement(driver);
		sqlrstmt.setConnection(this);
		sqlrstmt.setSQLRConnection(sqlrcon);
		sqlrstmt.setSQLRCursor(sqlrcur);
		driver.debugEnd();
		return sqlrstmt;
	}

	public synchronized
	CallableStatement prepareCall(String sql,
					int resultSetType,
					int resultSetConcurrency)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	CallableStatement prepareCall(String sql,
					int resultSetType,
					int resultSetConcurrency,
					int resultSetHoldability)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	PreparedStatement prepareStatement(String sql)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRCursor	sqlrcur=new SQLRCursor(sqlrcon);
		sqlrcur.getNullsAsNulls();
		sqlrcur.prepareQuery(sql);
		SQLRelayPreparedStatement	sqlrstmt=
				new SQLRelayPreparedStatement(driver);
		sqlrstmt.setConnection(this);
		sqlrstmt.setSQLRConnection(sqlrcon);
		sqlrstmt.setSQLRCursor(sqlrcur);
		driver.debugEnd();
		return sqlrstmt;
	}

	public synchronized
	PreparedStatement prepareStatement(String sql,
					int autoGeneratedKeys)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	PreparedStatement prepareStatement(String sql,
					int[] columnIndexes)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	PreparedStatement prepareStatement(String sql,
					int resultSetType,
					int resultSetConcurrency)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	PreparedStatement prepareStatement(String sql,
					int resultSetType,
					int resultSetConcurrency,
					int resultSetHoldability)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	PreparedStatement prepareStatement(String sql,
					String[] columnNames)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	void releaseSavepoint(Savepoint savepoint) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void rollback() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		if (!sqlrcon.rollback()) {
			throwErrorMessageException();
		}
		driver.debugEnd();
	}

	public synchronized
	void rollback(Savepoint savepoint) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setAutoCommit(boolean autocommit) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("autocommit: ",autocommit);
		if (!((autocommit)?sqlrcon.autoCommitOn():
					sqlrcon.autoCommitOff())) {
			throwErrorMessageException();
		}
		driver.debugPrintln("success");
		this.autocommit=autocommit;
		driver.debugEnd();
	}

	public synchronized
	void setCatalog(String catalog) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("catalog: ",catalog);
		if (!sqlrcon.selectDatabase(catalog)) {
			throwErrorMessageException();
		}
		driver.debugPrintln("success");
		driver.debugEnd();
	}

	public synchronized
	void setClientInfo(Properties properties)
					throws SQLClientInfoException {
		driver.debugFunction(this);
		if (sqlrcon==null) {
			throwClientInfoException();
		}
		clientinfo.clear();
		clientinfo.putAll(properties);
		setClientInfo();
		driver.debugEnd();
	}

	public synchronized
	void setClientInfo(String name, String value)
					throws SQLClientInfoException {
		driver.debugFunction(this);
		if (sqlrcon==null) {
			throwClientInfoException();
		}
		clientinfo.setProperty(name,value);
		setClientInfo();
		driver.debugEnd();
	}

	private void setClientInfo() {
		driver.debugFunction(this);
		String	info=new String();
		boolean	first=true;
		for (String name: clientinfo.stringPropertyNames()) {
			if (first) {
				first=false;
			} else {
				info+=",";
			}
			info+=name+":"+clientinfo.getProperty(name);
			driver.debugPrintln(name,": ",
					clientinfo.getProperty(name));
		}
		sqlrcon.setClientInfo(info);
		driver.debugEnd();
	}

	public synchronized
	void setHoldability(int holdability) throws SQLException {
		driver.debugFunction(this);
		if (holdability!=ResultSet.HOLD_CURSORS_OVER_COMMIT) {
			throwFeatureNotSupportedException();
		}
		driver.debugPrintln("holdability: HOLD_CURSORS_OVER_COMMIT");
		driver.debugEnd();
	}

	public synchronized
	void setNetworkTimeout(Executor executor,
					int milliseconds)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("milliseconds: ",milliseconds);
		// we can ignore executor because we have an internal
		// timeout implementation
		if (milliseconds<0) {
			throwException("timeout < 0");
		}
		if (milliseconds==0) {
			sqlrcon.setConnectTimeout(-1,-1);
		} else {
			sqlrcon.setConnectTimeout(milliseconds/1000,
				((milliseconds-(milliseconds/1000))*1000));
		}
		// FIXME: the timeout can also be set using an environment
		// variable, so we should get this from the underlying api
		// instead of tracking it here
		networktimeout=milliseconds;
		driver.debugEnd();
	}

	public synchronized
	void setReadOnly(boolean readonly) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		// FIXME: implement this somehow
		this.readonly=readonly;
		driver.debugPrintln("FIXME: implement this");
		driver.debugPrintln("readonly: ",readonly);
		driver.debugEnd();
	}

	public synchronized
	Savepoint setSavepoint() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Savepoint setSavepoint(String name) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	void setSchema(String schema) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		// FIXME: implement this somehow
		driver.debugPrintln("FIXME: implement this");
		driver.debugPrintln("schema: ",schema);
		driver.debugEnd();
	}

	public synchronized
	void setTransactionIsolation(int level) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		switch (level) {
			case Connection.TRANSACTION_READ_UNCOMMITTED:
				// FIXME: implement this somehow
				driver.debugPrintln("FIXME: implement this");
				driver.debugPrintln("isolation level: ",
						"TRANSACTION_READ_UNCOMMITTED");
				break;
			case Connection.TRANSACTION_READ_COMMITTED:
				// FIXME: implement this somehow
				driver.debugPrintln("FIXME: implement this");
				driver.debugPrintln("isolation level: ",
						"TRANSACTION_READ_COMMITTED");
				break;
			case Connection.TRANSACTION_REPEATABLE_READ:
				// FIXME: implement this somehow
				driver.debugPrintln("FIXME: implement this");
				driver.debugPrintln("isolation level: ",
						"TRANSACTION_REPEATABLE_READ");
				break;
			case Connection.TRANSACTION_SERIALIZABLE:
				// FIXME: implement this somehow
				driver.debugPrintln("FIXME: implement this");
				driver.debugPrintln("isolation level: ",
						"TRANSACTION_SERIALIZABLE");
				break;
			default:
				throwException("Invalid transaction " +
						"isolation level " + level);
		}
		txisolevel=level;
		driver.debugEnd();
	}

	public synchronized
	void setTypeMap(Map<String,Class<?>> map) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		// FIXME: do something with this
		typemap=map;
		driver.debugEnd();
	}

	public synchronized
	boolean isWrapperFor(Class<?> iface) throws SQLException {
		driver.debugFunction(this);
		driver.debugEnd();
		return (iface==SQLRConnection.class);
	}

	@SuppressWarnings({"unchecked"})
	public synchronized
	<T> T unwrap(Class<T> iface) throws SQLException {
		driver.debugFunction(this);
		driver.debugEnd();
		return (T)((iface==SQLRConnection.class)?sqlrcon:null);
	}

	protected void throwClientInfoException()
					throws SQLClientInfoException {
		driver.debugZeroIndent();
		throw new SQLClientInfoException();
	}

	private void throwExceptionIfClosed() throws SQLException {
		if (sqlrcon==null) {
			throwException("connection is closed");
		}
	}

	private void throwErrorMessageException() throws SQLException {
		throwException(sqlrcon.errorMessage());
	}

	private void throwFeatureNotSupportedException() throws SQLException {
		driver.debugZeroIndent();
		throw new SQLFeatureNotSupportedException();
	}

	private void throwException(String reason) throws SQLException {
		driver.debugZeroIndent();
		throw new SQLException(reason);
	}

	public synchronized
	SQLRConnection getSQLRConnection() {
		return sqlrcon;
	}
}
