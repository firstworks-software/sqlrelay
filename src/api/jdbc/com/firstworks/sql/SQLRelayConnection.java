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
	private boolean		autocommit;
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

		driver.debugPrintln("host: "+host);
		driver.debugPrintln("port: "+port);
		driver.debugPrintln("socket: "+socket);
		driver.debugPrintln("user: "+user);
		driver.debugPrintln("password: "+password);
		driver.debugPrintln("retrytime: "+retrytime);
		driver.debugPrintln("tries: "+tries);

		this.host=host;
		this.port=port;
		this.socket=socket;
		this.user=user;
		this.password=password;

		sqlrcon=new SQLRConnection(host,port,socket,
						user,password,retrytime,tries);
		readonly=false;
		clientinfo=new Properties();
		// FIXME: might not be false, need to get this from server
		autocommit=false;
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
		driver.debugPrintln("host: "+host);
		driver.debugEnd();
		return host;
	}

	public
	short getPort() {
		driver.debugFunction(this);
		driver.debugPrintln("port: "+port);
		driver.debugEnd();
		return port;
	}

	public
	String getSocket() {
		driver.debugFunction(this);
		driver.debugPrintln("socket: "+socket);
		driver.debugEnd();
		return socket;
	}

	public
	String getUser() {
		driver.debugFunction(this);
		driver.debugPrintln("user: "+user);
		driver.debugEnd();
		return user;
	}

	public
	String getPassword() {
		driver.debugFunction(this);
		driver.debugPrintln("password: "+password);
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
		// FIXME: we might be able to support this...
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
		// FIXME: we might be able to support this...
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

		// debug
		debugResultSetType(resultSetType);
		debugResultSetConcurrency(resultSetConcurrency);
		debugResultSetHoldability(resultSetHoldability);

		// catch unsupported options
		throwResultSetFeatureNotSupportedException(
						resultSetType,
						resultSetConcurrency,
						resultSetHoldability);

		// create a cursor
		SQLRCursor	sqlrcur=new SQLRCursor(sqlrcon);
		sqlrcur.getNullsAsNulls();

		// set result set buffer size as appropriate
		switch (resultSetType) {
			case ResultSet.TYPE_FORWARD_ONLY:
				sqlrcur.setResultSetBufferSize(10);
				break;
			case ResultSet.TYPE_SCROLL_INSENSITIVE:
				sqlrcur.setResultSetBufferSize(0);
				break;
		}

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
		// FIXME: we might be able to support this...
		driver.debugEnd();
		return null;
	}

	public
	boolean getAutoCommit() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("autocommit: "+autocommit);
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
		driver.debugPrintln("catalog: "+catalog);
		driver.debugEnd();
		return catalog;
	}

	public
	Properties getClientInfo() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("clientinfo: "+clientinfo);
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
		int	retval=
			((sqlrcon.getConnectTimeoutSeconds()*1000000)+
			sqlrcon.getConnectTimeoutMicroseconds())/1000;
		driver.debugPrintln("timeout: "+retval);
		driver.debugEnd();
		return retval;
	}

	public
	String getSchema() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		String	schema=null;
		synchronized (networklock) {
			schema=sqlrcon.getCurrentSchema();
		}
		driver.debugPrintln("schema: "+schema);
		driver.debugEnd();
		return schema;
	}

	public
	int getTransactionIsolation() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		String	level=sqlrcon.getIsolationLevel(3);
		if (level==null) {
			throwErrorMessageException();
		}
		driver.debugPrintln("isolation level: "+level);
		int	retval=Connection.TRANSACTION_NONE;
		switch (level) {
			case "TRANSACTION_READ_UNCOMMITTED":
				retval=Connection.TRANSACTION_READ_UNCOMMITTED;
				break;
			case "TRANSACTION_READ_COMMITTED":
				retval=Connection.TRANSACTION_READ_COMMITTED;
				break;
			case "TRANSACTION_REPEATABLE_READ":
				retval=Connection.TRANSACTION_REPEATABLE_READ;
				break;
			case "TRANSACTION_SERIALIZABLE":
				retval=Connection.TRANSACTION_SERIALIZABLE;
				break;
			default:
				throwException("Invalid transaction " +
						"isolation level "+level);
				break;
		}
		driver.debugEnd();
		return retval;
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
		driver.debugPrintln("isclosed: "+isclosed);
		driver.debugEnd();
		return isclosed;
	}

	public
	boolean isReadOnly() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("read only: "+readonly);
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
		driver.debugPrintln("ping: "+ping);
		driver.debugEnd();
		return ping;
	}

	public
	String nativeSQL(String sql) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("sql: "+sql);
		driver.debugEnd();
		return sql;
	}

	public
	CallableStatement prepareCall(String sql) throws SQLException {
		driver.debugFunction(this);
		CallableStatement	stmt=prepareCall(sql,
					ResultSet.TYPE_FORWARD_ONLY,
					ResultSet.CONCUR_READ_ONLY,
					ResultSet.HOLD_CURSORS_OVER_COMMIT);
		driver.debugEnd();
		return stmt;
	}

	public
	CallableStatement prepareCall(String sql,
					int resultSetType,
					int resultSetConcurrency)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		CallableStatement	stmt=prepareCall(sql,
					resultSetType,
					resultSetConcurrency,
					ResultSet.HOLD_CURSORS_OVER_COMMIT);
		driver.debugEnd();
		return stmt;
	}

	public
	CallableStatement prepareCall(String sql,
					int resultSetType,
					int resultSetConcurrency,
					int resultSetHoldability)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();

		// debug
		driver.debugPrintln("sql: "+sql);
		debugResultSetType(resultSetType);
		debugResultSetConcurrency(resultSetConcurrency);
		debugResultSetHoldability(resultSetHoldability);

		// catch unsupported options
		throwResultSetFeatureNotSupportedException(
						resultSetType,
						resultSetConcurrency,
						resultSetHoldability);

		// create a cursor
		SQLRCursor	sqlrcur=new SQLRCursor(sqlrcon);
		sqlrcur.getNullsAsNulls();
		sqlrcur.prepareQuery(sql);

		// set result set buffer size as appropriate
		switch (resultSetType) {
			case ResultSet.TYPE_FORWARD_ONLY:
				sqlrcur.setResultSetBufferSize(10);
				break;
			case ResultSet.TYPE_SCROLL_INSENSITIVE:
				sqlrcur.setResultSetBufferSize(0);
				break;
		}

		// create a statement, attach the cursor to the statement
		SQLRelayCallableStatement	sqlrstmt=
				new SQLRelayCallableStatement(driver);
		sqlrstmt.setConnection(this);
		sqlrstmt.setNetworkLock(networklock);
		sqlrstmt.setSQLRConnection(sqlrcon);
		sqlrstmt.setSQLRCursor(sqlrcur);

		driver.debugEnd();
		return null;
	}

	public
	PreparedStatement prepareStatement(String sql)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		PreparedStatement	stmt=prepareStatement(sql,
					ResultSet.TYPE_FORWARD_ONLY,
					ResultSet.CONCUR_READ_ONLY,
					ResultSet.HOLD_CURSORS_OVER_COMMIT);
		driver.debugEnd();
		return stmt;
	}

	public
	PreparedStatement prepareStatement(String sql,
					int resultSetType,
					int resultSetConcurrency)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		PreparedStatement	stmt=prepareStatement(sql,
					resultSetType,
					resultSetConcurrency,
					ResultSet.HOLD_CURSORS_OVER_COMMIT);
		driver.debugEnd();
		return stmt;
	}

	public
	PreparedStatement prepareStatement(String sql,
					int resultSetType,
					int resultSetConcurrency,
					int resultSetHoldability)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();

		// debug
		driver.debugPrintln("sql: "+sql);
		debugResultSetType(resultSetType);
		debugResultSetConcurrency(resultSetConcurrency);
		debugResultSetHoldability(resultSetHoldability);

		// catch unsupported options
		throwResultSetFeatureNotSupportedException(
						resultSetType,
						resultSetConcurrency,
						resultSetHoldability);

		// create a cursor
		SQLRCursor	sqlrcur=new SQLRCursor(sqlrcon);
		sqlrcur.getNullsAsNulls();
		sqlrcur.prepareQuery(sql);

		// set result set buffer size as appropriate
		switch (resultSetType) {
			case ResultSet.TYPE_FORWARD_ONLY:
				sqlrcur.setResultSetBufferSize(10);
				break;
			case ResultSet.TYPE_SCROLL_INSENSITIVE:
				sqlrcur.setResultSetBufferSize(0);
				break;
		}

		// create a statement, attach the cursor to the statement
		SQLRelayPreparedStatement	sqlrstmt=
				new SQLRelayPreparedStatement(driver);
		sqlrstmt.setConnection(this);
		sqlrstmt.setNetworkLock(networklock);
		sqlrstmt.setSQLRConnection(sqlrcon);
		sqlrstmt.setSQLRCursor(sqlrcur);
		driver.debugEnd();
		return null;
	}

	public
	PreparedStatement prepareStatement(String sql,
					int autoGeneratedKeys)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();

		// debug
		debugAutoGeneratedKeys(autoGeneratedKeys);

		// catch unsupported options
		throwFeatureNotSupportedException(autoGeneratedKeys);

		// create a prepared statement
		PreparedStatement	stmt=prepareStatement(sql);

		driver.debugEnd();
		return stmt;
	}

	public
	PreparedStatement prepareStatement(String sql,
					int[] columnIndexes)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();

		// debug and catch unsupported options
		boolean	supported=true;
		for (int i=0; i<columnIndexes.length; i++) {
			driver.debugStart("columnIndexes["+i+"]");
			debugAutoGeneratedKeys(columnIndexes[i]);
			throwFeatureNotSupportedException(columnIndexes[i]);
			driver.debugEnd();
		}

		// create a prepared statement
		PreparedStatement	stmt=prepareStatement(sql);
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
		driver.debugPrintln("savepoint: "+savepoint);
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
		driver.debugPrintln("savepoint: "+savepoint);
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public
	void setAutoCommit(boolean autocommit) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("autocommit: "+autocommit);
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
		driver.debugPrintln("catalog: "+catalog);
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
		driver.debugPrintln(name+": "+value);
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
		debugResultSetHoldability(holdability);
		throwResultSetHoldabiltyNotSupportedException(holdability);
		// we only support HOLD_CURSORS_OVER_COMMIT
		driver.debugPrintln("holdability: HOLD_CURSORS_OVER_COMMIT");
		driver.debugEnd();
	}

	public
	void setNetworkTimeout(Executor executor,
					int milliseconds)
					throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("milliseconds: "+milliseconds);
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
		driver.debugEnd();
	}

	public
	void setReadOnly(boolean readonly) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		// FIXME: implement this somehow
		this.readonly=readonly;
		driver.debugPrintln("FIXME: implement this");
		driver.debugPrintln("readonly: "+readonly);
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
		driver.debugPrintln("savepoint: "+name);
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public
	void setSchema(String schema) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("schema: "+schema);
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
				if (!sqlrcon.setIsolationLevel(
					"TRANSACTION_READ_UNCOMMITTED",3)) {
					throwErrorMessageException();
				}
				break;
			case Connection.TRANSACTION_READ_COMMITTED:
				if (!sqlrcon.setIsolationLevel(
					"TRANSACTION_READ_COMMITTED",3)) {
					throwErrorMessageException();
				}
				break;
			case Connection.TRANSACTION_REPEATABLE_READ:
				if (!sqlrcon.setIsolationLevel(
					"TRANSACTION_REPEATABLE_READ",3)) {
					throwErrorMessageException();
				}
				break;
			case Connection.TRANSACTION_SERIALIZABLE:
				if (!sqlrcon.setIsolationLevel(
					"TRANSACTION_SERIALIZABLE",3)) {
					throwErrorMessageException();
				}
				break;
			default:
				throwException("Invalid transaction " +
						"isolation level "+level);
		}
		driver.debugEnd();
	}

	public
	void setTypeMap(Map<String,Class<?>> map) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		// FIXME: we might be able to support this...
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

	private void debugResultSetType(int resultSetType) {
		switch (resultSetType) {
			case ResultSet.TYPE_FORWARD_ONLY:
				driver.debugPrintln("result set type: "+
						"TYPE_FORWARD_ONLY");
				break;
			case ResultSet.TYPE_SCROLL_INSENSITIVE:
				driver.debugPrintln("result set type: "+
						"TYPE_SCROLL_INSENSITIVE");
				break;
			case ResultSet.TYPE_SCROLL_SENSITIVE:
				driver.debugPrintln("result set type: "+
						"TYPE_SCROLL_SENSITIVE");
				break;
			default:
				driver.debugPrintln("result set type: "+
							"unknown - "+
							resultSetType);
				break;
		}
	}

	private void debugResultSetConcurrency(int resultSetConcurrency) {
		switch (resultSetConcurrency) {
			case ResultSet.CONCUR_READ_ONLY:
				driver.debugPrintln("result set concurrency: "+
							"CONCUR_READ_ONLY");
				break;
			case ResultSet.CONCUR_UPDATABLE:
				driver.debugPrintln("result set concurrency: "+
							"CONCUR_UPDATABLE");
				break;
			default:
				driver.debugPrintln("result set concurrency: "+
							"unknown - "+
							resultSetConcurrency);
				break;
		}
	}

	private void debugResultSetHoldability(int resultSetHoldability) {
		switch (resultSetHoldability) {
			case ResultSet.HOLD_CURSORS_OVER_COMMIT:
				driver.debugPrintln("result set holdability: "+
						"HOLD_CURSORS_OVER_COMMIT");
				break;
			case ResultSet.CLOSE_CURSORS_AT_COMMIT:
				driver.debugPrintln("result set holdability: "+
						"CLOSE_CURSORS_AT_COMMIT");
				break;
			default:
				driver.debugPrintln("result set holdability: "+
							"unknown - "+
							resultSetHoldability);
				break;
		}
	}

	private void debugAutoGeneratedKeys(int autoGeneratedKeys) {
		switch (autoGeneratedKeys) {
			case Statement.RETURN_GENERATED_KEYS:
				driver.debugPrintln("auto genrated keys: "+
						"RETURN_GENERATED_KEYS");
				break;
			case Statement.NO_GENERATED_KEYS:
				driver.debugPrintln("auto genrated keys: "+
						"NO_GENERATED_KEYS");
				break;
			default:
				driver.debugPrintln("auto genrated keys: "+
						"unknown - "+
						autoGeneratedKeys);
				break;
		}
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

	private void throwResultSetTypeNotSupportedException(
						int resultSetType)
						throws SQLException {
		if (resultSetType==ResultSet.TYPE_SCROLL_SENSITIVE) {
			driver.debugPrintln("not supported: "+
						"TYPE_SCROLL_SENSITIVE");
			throwFeatureNotSupportedException();
		}
	}

	private void throwResultSetConcurrencyNotSupportedException(
						int resultSetConcurrency)
						throws SQLException {
		if (resultSetConcurrency==ResultSet.CONCUR_UPDATABLE) {
			driver.debugPrintln("not supported: "+
						"CONCUR_UPDATABLE");
			throwFeatureNotSupportedException();
		}
	}

	private void throwResultSetHoldabiltyNotSupportedException(
						int resultSetHoldability)
						throws SQLException {
		if (resultSetHoldability==ResultSet.CLOSE_CURSORS_AT_COMMIT) {
			driver.debugPrintln("not supported: "+
						"CLOSE_CURSORS_AT_COMMIT");
			throwFeatureNotSupportedException();
		}
	}

	private void throwResultSetFeatureNotSupportedException(
						int resultSetType,
						int resultSetConcurrency,
						int resultSetHoldability)
						throws SQLException {
	 	throwResultSetTypeNotSupportedException(
						resultSetConcurrency);
	 	throwResultSetConcurrencyNotSupportedException(
						resultSetConcurrency);
		throwResultSetHoldabiltyNotSupportedException(
						resultSetHoldability);
	}

	private void throwFeatureNotSupportedException(
						int autoGeneratedKeys)
						throws SQLException {
		if (autoGeneratedKeys==Statement.RETURN_GENERATED_KEYS) {
			driver.debugPrintln("not supported: "+
						"RETURN_GENERATED_KEYS");
			throwFeatureNotSupportedException();
		}
	}

	protected void throwFeatureNotSupportedException() throws SQLException {
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
