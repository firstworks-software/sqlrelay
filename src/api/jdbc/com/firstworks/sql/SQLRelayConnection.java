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

		driver.debugFunction();

		this.host=host;
		this.port=port;
		this.socket=socket;
		this.user=user;
		this.password=password;
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
			sqlrcon.debugOn();
		}
		driver.debugEnd();
	}

	public String getHost() {
		driver.debugFunction();
		driver.debugEnd();
		return host;
	}

	public short getPort() {
		driver.debugFunction();
		driver.debugEnd();
		return port;
	}

	public String getSocket() {
		driver.debugFunction();
		driver.debugEnd();
		return socket;
	}

	public String getUser() {
		driver.debugFunction();
		driver.debugEnd();
		return user;
	}

	public String getPassword() {
		driver.debugFunction();
		driver.debugEnd();
		return password;
	}

	public void	abort(Executor executor) throws SQLException {
		driver.debugFunction();
		close();
		driver.debugEnd();
	}

	public void	clearWarnings() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugEnd();
	}

	public void	close() throws SQLException {
		driver.debugFunction();
		sqlrcon.endSession();
		sqlrcon=null;
		driver.debugEnd();
	}

	public void	commit() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		if (!sqlrcon.commit()) {
			throwErrorMessageException();
		}
		driver.debugEnd();
	}

	public Array	createArrayOf(String typeName,
					Object[] elements)
					throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public Blob	createBlob() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this...
		driver.debugEnd();
		return null;
	}

	public Clob	createClob() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this...
		driver.debugEnd();
		return null;
	}

	public NClob	createNClob() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this...
		driver.debugEnd();
		return null;
	}

	public SQLXML	createSQLXML() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public Statement	createStatement() throws SQLException {
		driver.debugFunction();
		Statement	stmt=createStatement(
					ResultSet.TYPE_FORWARD_ONLY,
					ResultSet.CONCUR_READ_ONLY,
					ResultSet.HOLD_CURSORS_OVER_COMMIT);
		driver.debugEnd();
		return stmt;
	}

	public Statement	createStatement(int resultSetType,
						int resultSetConcurrency)
						throws SQLException {
		driver.debugFunction();
		Statement	stmt=createStatement(
					resultSetType,
					resultSetConcurrency,
					ResultSet.HOLD_CURSORS_OVER_COMMIT);
		driver.debugEnd();
		return stmt;
	}

	public Statement	createStatement(int resultSetType,
						int resultSetConcurrency,
						int resultSetHoldability)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();

		// unsupported options
		if (resultSetType==
				ResultSet.TYPE_SCROLL_SENSITIVE ||
			resultSetConcurrency==
				ResultSet.CONCUR_UPDATABLE ||
			resultSetHoldability==
				ResultSet.CLOSE_CURSORS_AT_COMMIT) {
			throwFeatureNotSupportedException();
		}

		// create a cursor
		SQLRCursor	sqlrcur=new SQLRCursor(sqlrcon);
		sqlrcur.getNullsAsNulls();

		// set result set buffer size as appropriate
		switch (resultSetType) {
			case ResultSet.TYPE_FORWARD_ONLY:
				// FIXME: this can probably be set
				// to something bigger than 1
				sqlrcur.setResultSetBufferSize(1);
				break;
			case ResultSet.TYPE_SCROLL_INSENSITIVE:
				sqlrcur.setResultSetBufferSize(0);
				break;
		}

		// create a statement, attach the cursor to the statement
		SQLRelayStatement	sqlrstmt=
					new SQLRelayStatement(driver);
		sqlrstmt.setConnection(this);
		sqlrstmt.setSQLRConnection(sqlrcon);
		sqlrstmt.setSQLRCursor(sqlrcur);

		driver.debugEnd();
		return sqlrstmt;
	}

	public Struct	createStruct(String typeName,
						Object[] attributes)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		return null;
	}

	public boolean	getAutoCommit() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugEnd();
		return autocommit;
	}

	public String	getCatalog() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugEnd();
		return sqlrcon.getCurrentDatabase();
	}

	public Properties	getClientInfo() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugEnd();
		return clientinfo;
	}

	public String	getClientInfo(String name) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		String	prop=getClientInfo().getProperty(name);
		driver.debugEnd();
		return prop;
	}

	public int	getHoldability() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugEnd();
		return ResultSet.HOLD_CURSORS_OVER_COMMIT;
	}

	public DatabaseMetaData	getMetaData() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		SQLRelayDatabaseMetaData	metadata=
				new SQLRelayDatabaseMetaData(driver);
		metadata.setConnection(this);
		driver.debugEnd();
		return metadata;
	}

	public int	getNetworkTimeout() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		// FIXME: the timeout can also be set using an environment
		// variable, so we should get this from the underlying api
		// instead of tracking it here
		driver.debugEnd();
		return networktimeout;
	}

	public String	getSchema() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		String	schema=sqlrcon.getCurrentSchema();
		driver.debugEnd();
		return schema;
	}

	public int	getTransactionIsolation() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugEnd();
		return txisolevel;
	}

	public Map<String,Class<?>>	getTypeMap() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugEnd();
		return typemap;
	}

	public SQLWarning	getWarnings() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		// sqlrelay doesn't support anything like this
		driver.debugEnd();
		return null;
	}

	public boolean	isClosed() throws SQLException {
		driver.debugFunction();
		boolean	isclosed=(sqlrcon==null);
		driver.debugEnd();
		return isclosed;
	}

	public boolean	isReadOnly() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugEnd();
		return readonly;
	}

	public boolean	isValid(int timeout) throws SQLException {
		driver.debugFunction();
		if (isClosed()) {
			driver.debugEnd();
			return false;
		}
		// FIXME: need to get the current response timeout pre-ping
		// and reset it post-ping, but the java api doesn't currently
		// have getResponseTimeout methods
		sqlrcon.setResponseTimeout(timeout,0);
		boolean	ping=sqlrcon.ping();
		driver.debugEnd();
		return ping;
	}

	public String	nativeSQL(String sql) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugEnd();
		return sql;
	}

	public CallableStatement	prepareCall(String sql)
						throws SQLException {
		driver.debugFunction();
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

	public CallableStatement	prepareCall(String sql,
						int resultSetType,
						int resultSetConcurrency)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public CallableStatement	prepareCall(String sql,
						int resultSetType,
						int resultSetConcurrency,
						int resultSetHoldability)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public PreparedStatement	prepareStatement(String sql)
						throws SQLException {
		driver.debugFunction();
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

	public PreparedStatement	prepareStatement(String sql,
						int autoGeneratedKeys)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public PreparedStatement	prepareStatement(String sql,
						int[] columnIndexes)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public PreparedStatement	prepareStatement(String sql,
						int resultSetType,
						int resultSetConcurrency)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public PreparedStatement	prepareStatement(String sql,
						int resultSetType,
						int resultSetConcurrency,
						int resultSetHoldability)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public PreparedStatement	prepareStatement(String sql,
						String[] columnNames)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public void	releaseSavepoint(Savepoint savepoint)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	rollback() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		if (!sqlrcon.rollback()) {
			throwErrorMessageException();
		}
		driver.debugEnd();
	}

	public void	rollback(Savepoint savepoint) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	setAutoCommit(boolean autocommit) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		if (!((autocommit)?sqlrcon.autoCommitOn():
					sqlrcon.autoCommitOff())) {
			throwErrorMessageException();
		}
		this.autocommit=autocommit;
		driver.debugEnd();
	}

	public void	setCatalog(String catalog) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		if (!sqlrcon.selectDatabase(catalog)) {
			throwErrorMessageException();
		}
		driver.debugEnd();
	}

	public void	setClientInfo(Properties properties)
						throws SQLClientInfoException {
		driver.debugFunction();
		if (sqlrcon==null) {
			throwClientInfoException();
		}
		clientinfo.clear();
		clientinfo.putAll(properties);
		setClientInfo();
		driver.debugEnd();
	}

	public void	setClientInfo(String name, String value)
						throws SQLClientInfoException {
		driver.debugFunction();
		if (sqlrcon==null) {
			throwClientInfoException();
		}
		clientinfo.setProperty(name,value);
		setClientInfo();
		driver.debugEnd();
	}

	private void	setClientInfo() {
		driver.debugFunction();
		String	info=new String();
		boolean	first=true;
		for (String name: clientinfo.stringPropertyNames()) {
			if (first) {
				first=false;
			} else {
				info+=",";
			}
			info+=name+":"+clientinfo.getProperty(name);
		}
		sqlrcon.setClientInfo(info);
		driver.debugEnd();
	}

	public void	setHoldability(int holdability) throws SQLException {
		driver.debugFunction();
		if (holdability!=ResultSet.HOLD_CURSORS_OVER_COMMIT) {
			throwFeatureNotSupportedException();
		}
		driver.debugEnd();
	}

	public void	setNetworkTimeout(Executor executor,
						int milliseconds)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
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

	public void	setReadOnly(boolean readonly) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		// FIXME: implement this somehow
		this.readonly=readonly;
		driver.debugEnd();
	}

	public Savepoint	setSavepoint() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public Savepoint	setSavepoint(String name) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public void	setSchema(String schema) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		// FIXME: implement this somehow
		driver.debugEnd();
	}

	public void	setTransactionIsolation(int level) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		switch (level) {
			case Connection.TRANSACTION_READ_UNCOMMITTED:
				// FIXME: implement this somehow
				break;
			case Connection.TRANSACTION_READ_COMMITTED:
				// FIXME: implement this somehow
				break;
			case Connection.TRANSACTION_REPEATABLE_READ:
				// FIXME: implement this somehow
				break;
			case Connection.TRANSACTION_SERIALIZABLE:
				// FIXME: implement this somehow
				break;
			default:
				throwException("Invalid transaction " +
						"isolation level " + level);
		}
		txisolevel=level;
		driver.debugEnd();
	}

	public void	setTypeMap(Map<String,Class<?>> map)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		// FIXME: do something with this
		typemap=map;
		driver.debugEnd();
	}

	public boolean	isWrapperFor(Class<?> iface) throws SQLException {
		driver.debugFunction();
		driver.debugEnd();
		return (iface==SQLRConnection.class);
	}

	@SuppressWarnings({"unchecked"})
	public <T> T	unwrap(Class<T> iface) throws SQLException {
		driver.debugFunction();
		driver.debugEnd();
		return (T)((iface==SQLRConnection.class)?sqlrcon:null);
	}

	protected void	throwClientInfoException()
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

	public SQLRConnection getSQLRConnection() {
		driver.debugFunction();
		driver.debugEnd();
		return sqlrcon;
	}
}
