package com.firstworks.sql;

import java.sql.*;

import java.util.Properties;
import java.util.Map;
import java.util.concurrent.Executor;

import com.firstworks.sqlrelay.*;

public class SQLRelayConnection implements Connection {

	private	SQLRelayDriver	driver;
	private final Object	networklock = new Object();
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
	private boolean		datetotimestamp;

	private Map<String,Class<?>>	typemap;

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
		datetotimestamp=false;

		if (driver.debug) {
			//sqlrcon.debugOn();
		}
		driver.debugEnd();
	}

	public
	void setDateToTimestamp(boolean datetotimestamp) {
		this.datetotimestamp=datetotimestamp;
	}

	public
	boolean getDateToTimestamp() {
		return datetotimestamp;
	}

	public
	String getHost() {
		driver.debugFunction(this);
		driver.debugPrintln("host: ",host);
		driver.debugEnd();
		return host;
	}

	public
	short getPort() {
		driver.debugFunction(this);
		driver.debugPrintln("port: ",port);
		driver.debugEnd();
		return port;
	}

	public
	String getSocket() {
		driver.debugFunction(this);
		driver.debugPrintln("socket: ",socket);
		driver.debugEnd();
		return socket;
	}

	public
	String getUser() {
		driver.debugFunction(this);
		driver.debugPrintln("user: ",user);
		driver.debugEnd();
		return user;
	}

	public
	String getPassword() {
		driver.debugFunction(this);
		driver.debugPrintln("password: ",password);
		driver.debugEnd();
		return password;
	}

	public
	void abort(Executor executor) throws SQLException {
		driver.debugFunction(this);
		close();
		driver.debugEnd();
	}

	public
	void clearWarnings() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugEnd();
	}

	public
	void close() throws SQLException {
		driver.debugFunction(this);
		synchronized (networklock) {
			sqlrcon.endSession();
		}
		sqlrcon=null;
		driver.debugEnd();
	}

	public
	void commit() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		boolean	success=false;
		synchronized (networklock) {
			success=sqlrcon.commit();
		}
		if (!success) {
			throwErrorMessageException();
		}
		driver.debugEnd();
	}

	public
	Array createArrayOf(String typeName,
					Object[] elements)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public
	Blob createBlob() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this...
		driver.debugEnd();
		return null;
	}

	public
	Clob createClob() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this...
		driver.debugEnd();
		return null;
	}

	public
	NClob createNClob() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this...
		driver.debugEnd();
		return null;
	}

	public
	SQLXML createSQLXML() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public
	Statement createStatement() throws SQLException {
		driver.debugFunction(this);
		Statement	stmt=createStatement(
					ResultSet.TYPE_FORWARD_ONLY,
					ResultSet.CONCUR_READ_ONLY,
					ResultSet.HOLD_CURSORS_OVER_COMMIT);
		driver.debugEnd();
		return stmt;
	}

	public
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

	public
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
		sqlrstmt.setNetworkLock(networklock);
		sqlrstmt.setSQLRConnection(sqlrcon);
		sqlrstmt.setSQLRCursor(sqlrcur);

		driver.debugEnd();
		return sqlrstmt;
	}

	public
	Struct createStruct(String typeName, Object[] attributes)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		return null;
	}

	public
	boolean getAutoCommit() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("autocommit: ",autocommit);
		driver.debugEnd();
		return autocommit;
	}

	public
	String getCatalog() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		String	catalog=null;
		synchronized (networklock) {
			catalog=sqlrcon.getCurrentDatabase();
		}
		driver.debugPrintln("catalog: ",catalog);
		driver.debugEnd();
		return catalog;
	}

	public
	Properties getClientInfo() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("clientinfo: ",clientinfo);
		driver.debugEnd();
		return clientinfo;
	}

	public
	String getClientInfo(String name) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		String	prop=getClientInfo().getProperty(name);
		driver.debugPrintln(name,": ",prop);
		driver.debugEnd();
		return prop;
	}

	public
	int getHoldability() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("holdability: HOLD_CURSORS_OVER_COMMIT");
		driver.debugEnd();
		return ResultSet.HOLD_CURSORS_OVER_COMMIT;
	}

	public
	DatabaseMetaData getMetaData() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayDatabaseMetaData	metadata=
				new SQLRelayDatabaseMetaData(driver);
		metadata.setConnection(this);
		metadata.setNetworkLock(networklock);
		driver.debugEnd();
		return metadata;
	}

	public
	int getNetworkTimeout() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		// FIXME: the timeout can also be set using an environment
		// variable, so we should get this from the underlying api
		// instead of tracking it here
		driver.debugEnd();
		return networktimeout;
	}

	public
	String getSchema() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		String	schema=null;
		synchronized (networklock) {
			schema=sqlrcon.getCurrentSchema();
		}
		driver.debugPrintln("schema: ",schema);
		driver.debugEnd();
		return schema;
	}

	public
	int getTransactionIsolation() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("isolation level: ",txisolevel);
		driver.debugEnd();
		return txisolevel;
	}

	public
	Map<String,Class<?>> getTypeMap() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugEnd();
		return typemap;
	}

	public
	SQLWarning getWarnings() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		// sqlrelay doesn't support anything like this
		driver.debugEnd();
		return null;
	}

	public
	boolean isClosed() throws SQLException {
		driver.debugFunction(this);
		boolean	isclosed=(sqlrcon==null);
		driver.debugPrintln("isclosed: ",isclosed);
		driver.debugEnd();
		return isclosed;
	}

	public
	boolean isReadOnly() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("read only: ",readonly);
		driver.debugEnd();
		return readonly;
	}

	public
	boolean isValid(int timeout) throws SQLException {
		driver.debugFunction(this);
		if (isClosed()) {
			driver.debugEnd();
			return false;
		}
		int	rtsec=sqlrcon.getResponseTimeoutSeconds();
		int	rtusec=sqlrcon.getResponseTimeoutMicroseconds();
		if (timeout>0) {
			sqlrcon.setResponseTimeout(timeout,0);
		}
		boolean	ping=false;
		synchronized (networklock) {
			ping=sqlrcon.ping();
		}
		if (timeout>0) {
			sqlrcon.setResponseTimeout(rtsec,rtusec);
		}
		driver.debugPrintln("ping: ",ping);
		driver.debugEnd();
		return ping;
	}

	public
	String nativeSQL(String sql) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("sql: ",sql);
		driver.debugEnd();
		return sql;
	}

	public
	CallableStatement prepareCall(String sql) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRCursor	sqlrcur=new SQLRCursor(sqlrcon);
		sqlrcur.getNullsAsNulls();
		sqlrcur.prepareQuery(sql);
		SQLRelayCallableStatement	sqlrstmt=
				new SQLRelayCallableStatement(driver);
		sqlrstmt.setConnection(this);
		sqlrstmt.setNetworkLock(networklock);
		sqlrstmt.setSQLRConnection(sqlrcon);
		sqlrstmt.setSQLRCursor(sqlrcur);
		driver.debugEnd();
		return sqlrstmt;
	}

	public
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

	public
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

	public
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
		sqlrstmt.setNetworkLock(networklock);
		sqlrstmt.setSQLRConnection(sqlrcon);
		sqlrstmt.setSQLRCursor(sqlrcur);
		driver.debugEnd();
		return sqlrstmt;
	}

	public
	PreparedStatement prepareStatement(String sql,
					int autoGeneratedKeys)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public
	PreparedStatement prepareStatement(String sql,
					int[] columnIndexes)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public
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

	public
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

	public
	PreparedStatement prepareStatement(String sql,
					String[] columnNames)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public
	void releaseSavepoint(Savepoint savepoint) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public
	void rollback() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		boolean	success=false;
		synchronized (networklock) {
			success=sqlrcon.rollback();
		}
		if (!success) {
			throwErrorMessageException();
		}
		driver.debugEnd();
	}

	public
	void rollback(Savepoint savepoint) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public
	void setAutoCommit(boolean autocommit) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("autocommit: ",autocommit);
		boolean	success=false;
		synchronized (networklock) {
			success=(autocommit)?sqlrcon.autoCommitOn():
						sqlrcon.autoCommitOff();
		}
		if (!success) {
			throwErrorMessageException();
		}
		driver.debugPrintln("success");
		this.autocommit=autocommit;
		driver.debugEnd();
	}

	public
	void setCatalog(String catalog) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("catalog: ",catalog);
		boolean	success=false;
		synchronized (networklock) {
			success=sqlrcon.selectDatabase(catalog);
		}
		if (!success) {
			throwErrorMessageException();
		}
		driver.debugPrintln("success");
		driver.debugEnd();
	}

	public
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

	public
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

	public
	void setHoldability(int holdability) throws SQLException {
		driver.debugFunction(this);
		if (holdability!=ResultSet.HOLD_CURSORS_OVER_COMMIT) {
			throwFeatureNotSupportedException();
		}
		driver.debugPrintln("holdability: HOLD_CURSORS_OVER_COMMIT");
		driver.debugEnd();
	}

	public
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

	public
	void setReadOnly(boolean readonly) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		// FIXME: implement this somehow
		this.readonly=readonly;
		driver.debugPrintln("FIXME: implement this");
		driver.debugPrintln("readonly: ",readonly);
		driver.debugEnd();
	}

	public
	Savepoint setSavepoint() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public
	Savepoint setSavepoint(String name) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public
	void setSchema(String schema) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("schema: ",schema);
		boolean	success=false;
		synchronized (networklock) {
			success=sqlrcon.selectSchema(schema);
		}
		if (!success) {
			throwErrorMessageException();
		}
		driver.debugPrintln("success");
		driver.debugEnd();
	}

	public
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

	public
	void setTypeMap(Map<String,Class<?>> map) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		// FIXME: do something with this
		typemap=map;
		driver.debugEnd();
	}

	public
	boolean isWrapperFor(Class<?> iface) throws SQLException {
		driver.debugFunction(this);
		driver.debugEnd();
		return (iface==SQLRConnection.class);
	}

	@SuppressWarnings({"unchecked"})
	public
	<T> T unwrap(Class<T> iface) throws SQLException {
		driver.debugFunction(this);
		driver.debugEnd();
		return (T)((iface==SQLRConnection.class)?sqlrcon:null);
	}

	protected void throwClientInfoException()
					throws SQLClientInfoException {
		driver.debugPrintln("exception: SQLClientInfoException");
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
		driver.debugPrintln(
			"exception: SQLFeatureNotSupportedException");
		driver.debugZeroIndent();
		throw new SQLFeatureNotSupportedException();
	}

	private void throwException(String reason) throws SQLException {
		driver.debugPrintln("exception: "+reason);
		driver.debugZeroIndent();
		throw new SQLException(reason);
	}

	public
	SQLRConnection getSQLRConnection() {
		return sqlrcon;
	}
}
