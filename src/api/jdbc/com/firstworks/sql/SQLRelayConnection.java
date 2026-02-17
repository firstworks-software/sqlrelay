package com.firstworks.sql;

import java.sql.*;
import javax.sql.rowset.serial.SerialClob;
import javax.sql.rowset.serial.SerialBlob;

import java.util.Properties;
import java.util.Map;
import java.util.HashMap;
import java.util.concurrent.Executor;

import com.firstworks.sqlrelay.*;

public class SQLRelayConnection implements Connection {

	private final Object	networklock = new Object();

	private	SQLRelayDriver	drv;

	private SQLRConnection	sqlrcon;

	private String		url;
	private String		host;
	private short		port;
	private String		socket;
	private String		user;
	private String		password;
	private boolean		readonly;
	private Properties	clientinfo;
	private boolean		autocommit;
	private boolean		datetotimestamp;

	private Map<String,Class<?>>	typemap;


	public
	SQLRelayConnection(String url,
				String host,
				short port,
				String socket,
				String user,
				String password,
				int retrytime,
				int tries,
				SQLRelayDriver driver)
				throws SQLException {

		this.drv=driver;

		drv.debugFunction(this);

		drv.debugPrintln("url: "+url);
		drv.debugPrintln("host: "+host);
		drv.debugPrintln("port: "+port);
		drv.debugPrintln("socket: "+socket);
		drv.debugPrintln("user: "+user);
		drv.debugPrintln("password: "+password);
		drv.debugPrintln("retrytime: "+retrytime);
		drv.debugPrintln("tries: "+tries);

		this.url=url;
		this.host=host;
		this.port=port;
		this.socket=socket;
		this.user=user;
		this.password=password;

		sqlrcon=new SQLRConnection(host,port,socket,
						user,password,retrytime,tries);
		// FIXME: might not be false, need to get this from server
		readonly=false;
		clientinfo=new Properties();
		// FIXME: might not be false, need to get this from server
		autocommit=false;
		typemap=new HashMap<String,Class<?>>();
		datetotimestamp=false;

		if (drv.debug) {
			//sqlrcon.debugOn();
		}
		drv.debugEnd();
	}

	SQLRConnection getSQLRConnection() {
		return sqlrcon;
	}

	void setDateToTimestamp(boolean datetotimestamp) {
		this.datetotimestamp=datetotimestamp;
	}

	boolean getDateToTimestamp() {
		return datetotimestamp;
	}

	String getURL() {
		drv.debugFunction(this);
		drv.debugPrintln("url: "+url);
		drv.debugEnd();
		return url;
	}

	public
	String getHost() {
		drv.debugFunction(this);
		drv.debugPrintln("host: "+host);
		drv.debugEnd();
		return host;
	}

	public
	short getPort() {
		drv.debugFunction(this);
		drv.debugPrintln("port: "+port);
		drv.debugEnd();
		return port;
	}

	public
	String getSocket() {
		drv.debugFunction(this);
		drv.debugPrintln("socket: "+socket);
		drv.debugEnd();
		return socket;
	}

	public
	String getUser() {
		drv.debugFunction(this);
		drv.debugPrintln("user: "+user);
		drv.debugEnd();
		return user;
	}

	public
	String getPassword() {
		drv.debugFunction(this);
		drv.debugPrintln("password: "+password);
		drv.debugEnd();
		return password;
	}

	public
	void abort(Executor executor) throws SQLException {
		drv.debugFunction(this);
		close();
		drv.debugEnd();
	}

	public
	void clearWarnings() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugEnd();
	}

	public
	void close() throws SQLException {
		drv.debugFunction(this);
		synchronized (networklock) {
			sqlrcon.endSession();
		}
		sqlrcon=null;
		drv.debugEnd();
	}

	public
	void commit() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		boolean	success=false;
		synchronized (networklock) {
			success=sqlrcon.commit();
		}
		if (!success) {
			throwErrorMessageException();
		}
		drv.debugEnd();
	}

	public
	Array createArrayOf(String typeName,
					Object[] elements)
					throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	Blob createBlob() throws SQLException {
		drv.debugFunction(this);
		Blob	retval=new SerialBlob(new byte[0]);
		drv.debugEnd();
		return retval;
	}

	public
	Clob createClob() throws SQLException {
		drv.debugFunction(this);
		Clob	retval=new SerialClob(new char[0]);
		drv.debugEnd();
		return retval;
	}

	public
	NClob createNClob() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
                // FIXME: we need an SQLRelayNClob implementation
                // to support this
		drv.debugEnd();
		return null;
	}

	public
	SQLXML createSQLXML() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
                // FIXME: we need an SQLRelaySQLXML implementation
                // to support this, which needs to be able to store a String
                // and return it as an InputStream, Reader, Source, and String
		drv.debugEnd();
		return null;
	}

	public
	Statement createStatement() throws SQLException {
		drv.debugFunction(this);
		Statement	stmt=createStatement(
					ResultSet.TYPE_FORWARD_ONLY,
					ResultSet.CONCUR_READ_ONLY,
					ResultSet.HOLD_CURSORS_OVER_COMMIT);
		drv.debugEnd();
		return stmt;
	}

	public
	Statement createStatement(int resultSetType,
					int resultSetConcurrency)
					throws SQLException {
		drv.debugFunction(this);
		Statement	stmt=createStatement(
					resultSetType,
					resultSetConcurrency,
					ResultSet.HOLD_CURSORS_OVER_COMMIT);
		drv.debugEnd();
		return stmt;
	}

	public
	Statement createStatement(int resultSetType,
						int resultSetConcurrency,
						int resultSetHoldability)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();

		// debug
		debugResultSetType(resultSetType);
		debugResultSetConcurrency(resultSetConcurrency);
		debugResultSetHoldability(resultSetHoldability);

		// catch unsupported options
		// we only support:
		// 	ResultSet.TYPE_FORWARD_ONLY
		//	ResultSet.CONCUR_READ_ONLY
		//	ResultSet.HOLD_CURSORS_OVER_COMMIT
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
					new SQLRelayStatement(drv);
		sqlrstmt.setConnection(this);
		sqlrstmt.setNetworkLock(networklock);
		sqlrstmt.setSQLRConnection(sqlrcon);
		sqlrstmt.setSQLRCursor(sqlrcur);

		drv.debugEnd();
		return sqlrstmt;
	}

	public
	Struct createStruct(String typeName, Object[] attributes)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this...
		drv.debugEnd();
		return null;
	}

	public
	boolean getAutoCommit() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("autocommit: "+autocommit);
		drv.debugEnd();
		return autocommit;
	}

	public
	String getCatalog() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		String	catalog=null;
		synchronized (networklock) {
			catalog=sqlrcon.getCurrentDatabase();
		}
		drv.debugPrintln("catalog: "+catalog);
		drv.debugEnd();
		return catalog;
	}

	public
	Properties getClientInfo() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("clientinfo: "+clientinfo);
		drv.debugEnd();
		return clientinfo;
	}

	public
	String getClientInfo(String name) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		String	prop=getClientInfo().getProperty(name);
		drv.debugPrintln(name,": ",prop);
		drv.debugEnd();
		return prop;
	}

	public
	int getHoldability() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("holdability: HOLD_CURSORS_OVER_COMMIT");
		drv.debugEnd();
		return ResultSet.HOLD_CURSORS_OVER_COMMIT;
	}

	public
	DatabaseMetaData getMetaData() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayDatabaseMetaData	metadata=
				new SQLRelayDatabaseMetaData(drv);
		metadata.setConnection(this);
		metadata.setNetworkLock(networklock);
		drv.debugEnd();
		return metadata;
	}

	public
	int getNetworkTimeout() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		int	sec=sqlrcon.getConnectTimeoutSeconds();
		int	usec=sqlrcon.getConnectTimeoutMicroseconds();
		drv.debugPrintln("sec: "+sec);
		drv.debugPrintln("usec: "+usec);
		if (sec==-1 || usec==-1) {
			sec=0;
			usec=0;
		}
		int	retval=((sec*1000000)+usec)/1000;
		drv.debugPrintln("timeout: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	String getSchema() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		String	schema=null;
		synchronized (networklock) {
			schema=sqlrcon.getCurrentSchema();
		}
		drv.debugPrintln("schema: "+schema);
		drv.debugEnd();
		return schema;
	}

	public
	int getTransactionIsolation() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		String	level=sqlrcon.getIsolationLevel(3);
		if (level==null) {
			throwErrorMessageException();
		}
		drv.debugPrintln("isolation level: "+level);
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
		drv.debugEnd();
		return retval;
	}

	public
	Map<String,Class<?>> getTypeMap() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugEnd();
		return typemap;
	}

	public
	SQLWarning getWarnings() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		// sqlrelay doesn't support anything like this
		drv.debugEnd();
		return null;
	}

	public
	boolean isClosed() throws SQLException {
		drv.debugFunction(this);
		boolean	isclosed=(sqlrcon==null);
		drv.debugPrintln("isclosed: "+isclosed);
		drv.debugEnd();
		return isclosed;
	}

	public
	boolean isReadOnly() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		// FIXME: get this from the server
		drv.debugPrintln("read only: "+readonly);
		drv.debugEnd();
		return readonly;
	}

	public
	boolean isValid(int timeout) throws SQLException {
		drv.debugFunction(this);
		if (isClosed()) {
			drv.debugEnd();
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
		drv.debugPrintln("ping: "+ping);
		drv.debugEnd();
		return ping;
	}

	public
	String nativeSQL(String sql) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("sql: "+sql);
		// FIXME: convert JDBC escape clauses
		drv.debugEnd();
		return sql;
	}

	public
	CallableStatement prepareCall(String sql) throws SQLException {
		drv.debugFunction(this);
		CallableStatement	stmt=prepareCall(sql,
					ResultSet.TYPE_FORWARD_ONLY,
					ResultSet.CONCUR_READ_ONLY,
					ResultSet.HOLD_CURSORS_OVER_COMMIT);
		drv.debugEnd();
		return stmt;
	}

	public
	CallableStatement prepareCall(String sql,
					int resultSetType,
					int resultSetConcurrency)
					throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		CallableStatement	stmt=prepareCall(sql,
					resultSetType,
					resultSetConcurrency,
					ResultSet.HOLD_CURSORS_OVER_COMMIT);
		drv.debugEnd();
		return stmt;
	}

	public
	CallableStatement prepareCall(String sql,
					int resultSetType,
					int resultSetConcurrency,
					int resultSetHoldability)
					throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();

		// debug
		drv.debugPrintln("sql: "+sql);
		debugResultSetType(resultSetType);
		debugResultSetConcurrency(resultSetConcurrency);
		debugResultSetHoldability(resultSetHoldability);

		// catch unsupported options
		// we only support:
		// 	ResultSet.TYPE_FORWARD_ONLY
		//	ResultSet.CONCUR_READ_ONLY
		//	ResultSet.HOLD_CURSORS_OVER_COMMIT
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
				new SQLRelayCallableStatement(drv);
		sqlrstmt.setConnection(this);
		sqlrstmt.setNetworkLock(networklock);
		sqlrstmt.setSQLRConnection(sqlrcon);
		sqlrstmt.setSQLRCursor(sqlrcur);

		drv.debugEnd();
		return sqlrstmt;
	}

	public
	PreparedStatement prepareStatement(String sql)
					throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		PreparedStatement	stmt=prepareStatement(sql,
					ResultSet.TYPE_FORWARD_ONLY,
					ResultSet.CONCUR_READ_ONLY,
					ResultSet.HOLD_CURSORS_OVER_COMMIT);
		drv.debugEnd();
		return stmt;
	}

	public
	PreparedStatement prepareStatement(String sql,
					int resultSetType,
					int resultSetConcurrency)
					throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		PreparedStatement	stmt=prepareStatement(sql,
					resultSetType,
					resultSetConcurrency,
					ResultSet.HOLD_CURSORS_OVER_COMMIT);
		drv.debugEnd();
		return stmt;
	}

	public
	PreparedStatement prepareStatement(String sql,
					int resultSetType,
					int resultSetConcurrency,
					int resultSetHoldability)
					throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();

		// debug
		drv.debugPrintln("sql: "+sql);
		debugResultSetType(resultSetType);
		debugResultSetConcurrency(resultSetConcurrency);
		debugResultSetHoldability(resultSetHoldability);

		// catch unsupported options
		// we only support:
		// 	ResultSet.TYPE_FORWARD_ONLY
		//	ResultSet.CONCUR_READ_ONLY
		//	ResultSet.HOLD_CURSORS_OVER_COMMIT
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
				new SQLRelayPreparedStatement(drv);
		sqlrstmt.setConnection(this);
		sqlrstmt.setNetworkLock(networklock);
		sqlrstmt.setSQLRConnection(sqlrcon);
		sqlrstmt.setSQLRCursor(sqlrcur);

		drv.debugEnd();
		return sqlrstmt;
	}

	public
	PreparedStatement prepareStatement(String sql,
					int autoGeneratedKeys)
					throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();

		debugAutoGeneratedKeys(autoGeneratedKeys);

		// catch unsupported options
		// we only support Statement.NO_GENERATED_KEYS,
		// but we can support this call if autoGeneratedKeys isn't
		// Statement.RETURN_GENERATED_KEYS
		throwAutoGeneratedKeysNotSupportedException(autoGeneratedKeys);

		// create a prepared statement
		PreparedStatement	stmt=prepareStatement(sql);

		drv.debugEnd();
		return stmt;
	}

	public
	PreparedStatement prepareStatement(String sql,
					int[] columnIndexes)
					throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();

		// catch unsupported options
		// we only support Statement.NO_GENERATED_KEYS,
		// but we can support this call if there are no columnIndexes
		if (columnIndexes!=null && columnIndexes.length>0) {
			throwAutoGeneratedKeysNotSupportedException(
					Statement.RETURN_GENERATED_KEYS);
		}

		// create a prepared statement
		PreparedStatement	stmt=prepareStatement(sql);

		drv.debugEnd();
		return stmt;
	}

	public
	PreparedStatement prepareStatement(String sql,
					String[] columnNames)
					throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();

		// catch unsupported options
		// we only support Statement.NO_GENERATED_KEYS,
		// but we can support this call if there are no columnNames
		if (columnNames!=null && columnNames.length>0) {
			throwAutoGeneratedKeysNotSupportedException(
					Statement.RETURN_GENERATED_KEYS);
		}

		// create a prepared statement
		PreparedStatement	stmt=prepareStatement(sql);

		drv.debugEnd();
		return stmt;
	}

	public
	void releaseSavepoint(Savepoint savepoint) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("savepoint: "+savepoint);
		throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void rollback() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		boolean	success=false;
		synchronized (networklock) {
			success=sqlrcon.rollback();
		}
		if (!success) {
			throwErrorMessageException();
		}
		drv.debugEnd();
	}

	public
	void rollback(Savepoint savepoint) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("savepoint: "+savepoint);
		throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setAutoCommit(boolean autocommit) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("autocommit: "+autocommit);
		boolean	success=false;
		synchronized (networklock) {
			success=(autocommit)?sqlrcon.autoCommitOn():
						sqlrcon.autoCommitOff();
		}
		if (!success) {
			throwErrorMessageException();
		}
		drv.debugPrintln("success");
		this.autocommit=autocommit;
		drv.debugEnd();
	}

	public
	void setCatalog(String catalog) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("catalog: "+catalog);
		boolean	success=false;
		synchronized (networklock) {
			success=sqlrcon.selectDatabase(catalog);
		}
		if (!success) {
			throwErrorMessageException();
		}
		drv.debugPrintln("success");
		drv.debugEnd();
	}

	public
	void setClientInfo(Properties properties)
					throws SQLClientInfoException {
		drv.debugFunction(this);
		if (sqlrcon==null) {
			throwClientInfoException();
		}
		clientinfo.clear();
		clientinfo.putAll(properties);
		setClientInfo();
		drv.debugEnd();
	}

	public
	void setClientInfo(String name, String value)
					throws SQLClientInfoException {
		drv.debugFunction(this);
		if (sqlrcon==null) {
			throwClientInfoException();
		}
		drv.debugPrintln(name+": "+value);
		clientinfo.setProperty(name,value);
		setClientInfo();
		drv.debugEnd();
	}

	private
	void setClientInfo() {
		drv.debugFunction(this);
		String	info=new String();
		boolean	first=true;
		for (String name: clientinfo.stringPropertyNames()) {
			if (first) {
				first=false;
			} else {
				info+=",";
			}
			info+=name+":"+clientinfo.getProperty(name);
			drv.debugPrintln(name,": ",
					clientinfo.getProperty(name));
		}
		sqlrcon.setClientInfo(info);
		drv.debugEnd();
	}

	public
	void setHoldability(int holdability) throws SQLException {
		drv.debugFunction(this);
		debugResultSetHoldability(holdability);
		// we only support HOLD_CURSORS_OVER_COMMIT
		throwResultSetHoldabiltyNotSupportedException(holdability);
		drv.debugPrintln("holdability: HOLD_CURSORS_OVER_COMMIT");
		drv.debugEnd();
	}

	public
	void setNetworkTimeout(Executor executor,
					int milliseconds)
					throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("milliseconds: "+milliseconds);
		// we can ignore executor because we have an internal
		// timeout implementation
		if (milliseconds<0) {
			throwException("timeout < 0");
		}
		if (milliseconds==0) {
			drv.debugPrintln("sec: -1");
			drv.debugPrintln("usec: -1");
			sqlrcon.setConnectTimeout(-1,-1);
		} else {
			int	sec=milliseconds/1000;
			int	usec=(milliseconds-(milliseconds/1000))*1000;
			drv.debugPrintln("sec: "+sec);
			drv.debugPrintln("usec: "+usec);
			sqlrcon.setConnectTimeout(sec,usec);
		}
		drv.debugEnd();
	}

	public
	void setReadOnly(boolean readonly) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		// FIXME: implement this somehow
		this.readonly=readonly;
		drv.debugPrintln("FIXME: implement this");
		drv.debugPrintln("readonly: "+readonly);
		drv.debugEnd();
	}

	public
	Savepoint setSavepoint() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	Savepoint setSavepoint(String name) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("savepoint: "+name);
		throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	void setSchema(String schema) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("schema: "+schema);
		boolean	success=false;
		synchronized (networklock) {
			success=sqlrcon.selectSchema(schema);
		}
		if (!success) {
			throwErrorMessageException();
		}
		drv.debugPrintln("success");
		drv.debugEnd();
	}

	public
	void setTransactionIsolation(int level) throws SQLException {
		drv.debugFunction(this);
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
		drv.debugEnd();
	}

	public
	void setTypeMap(Map<String,Class<?>> map) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		typemap=map;
		drv.debugEnd();
	}

	void debugResultSetType(int type) {
		switch (type) {
			case ResultSet.TYPE_FORWARD_ONLY:
				drv.debugPrintln("result set type: "+
						"TYPE_FORWARD_ONLY");
				break;
			case ResultSet.TYPE_SCROLL_INSENSITIVE:
				drv.debugPrintln("result set type: "+
						"TYPE_SCROLL_INSENSITIVE");
				break;
			case ResultSet.TYPE_SCROLL_SENSITIVE:
				drv.debugPrintln("result set type: "+
						"TYPE_SCROLL_SENSITIVE");
				break;
			default:
				drv.debugPrintln("result set type: "+
							"unknown - "+
							type);
				break;
		}
	}

	void debugResultSetConcurrency(int concurrency) {
		switch (concurrency) {
			case ResultSet.CONCUR_READ_ONLY:
				drv.debugPrintln("result set concurrency: "+
							"CONCUR_READ_ONLY");
				break;
			case ResultSet.CONCUR_UPDATABLE:
				drv.debugPrintln("result set concurrency: "+
							"CONCUR_UPDATABLE");
				break;
			default:
				drv.debugPrintln("result set concurrency: "+
							"unknown - "+
							concurrency);
				break;
		}
	}

	void debugResultSetHoldability(int holdability) {
		switch (holdability) {
			case ResultSet.HOLD_CURSORS_OVER_COMMIT:
				drv.debugPrintln("result set holdability: "+
						"HOLD_CURSORS_OVER_COMMIT");
				break;
			case ResultSet.CLOSE_CURSORS_AT_COMMIT:
				drv.debugPrintln("result set holdability: "+
						"CLOSE_CURSORS_AT_COMMIT");
				break;
			default:
				drv.debugPrintln("result set holdability: "+
							"unknown - "+
							holdability);
				break;
		}
	}

	void debugAutoGeneratedKeys(int autoGeneratedKeys) {
		switch (autoGeneratedKeys) {
			case Statement.RETURN_GENERATED_KEYS:
				drv.debugPrintln("auto generated keys: "+
						"RETURN_GENERATED_KEYS");
				break;
			case Statement.NO_GENERATED_KEYS:
				drv.debugPrintln("auto generated keys: "+
						"NO_GENERATED_KEYS");
				break;
			default:
				drv.debugPrintln("auto generated keys: "+
							"unknown - "+
							autoGeneratedKeys);
				break;
		}
	}

	boolean isResultSetTypeSupported(int type) {
		switch (type) {
			case ResultSet.TYPE_FORWARD_ONLY:
				return true;
			case ResultSet.TYPE_SCROLL_INSENSITIVE:
				return true;
			case ResultSet.TYPE_SCROLL_SENSITIVE:
				drv.debugPrintln("not supported: "+
						"TYPE_SCROLL_SENSITIVE");
				return false;
			default:
				drv.debugPrintln("not supported: "+
							"unknown - "+
							type);
				return false;
		}
	}

	boolean isResultSetConcurrencySupported(int concurrency) {
		switch (concurrency) {
			case ResultSet.CONCUR_READ_ONLY:
				return true;
			case ResultSet.CONCUR_UPDATABLE:
				drv.debugPrintln("not supported: "+
						"CONCUR_UPDATABLE");
				return false;
			default:
				drv.debugPrintln("not supported: "+
							"unknown - "+
							concurrency);
				return false;
		}
	}

	boolean isResultSetHoldabilitySupported(int holdability) {
		switch (holdability) {
			case ResultSet.HOLD_CURSORS_OVER_COMMIT:
				return true;
			case ResultSet.CLOSE_CURSORS_AT_COMMIT:
				drv.debugPrintln("not supported: "+
						"CLOSE_CURSORS_AT_COMMIT");
				return false;
			default:
				drv.debugPrintln("not supported: "+
							"unknown - "+
							holdability);
				return false;
		}
	}

	void throwExceptionIfClosed() throws SQLException {
		if (sqlrcon==null) {
			throwException("connection is closed");
		}
	}

	void throwErrorMessageException() throws SQLException {
		throwException(sqlrcon.errorMessage());
	}

	protected
	void throwClientInfoException()
					throws SQLClientInfoException {
		drv.debugPrintln("exception: SQLClientInfoException");
		drv.debugZeroIndent();
		throw new SQLClientInfoException();
	}

	void throwResultSetTypeNotSupportedException(
						int type)
						throws SQLException {
		switch (type) {
			case ResultSet.TYPE_FORWARD_ONLY:
				break;
			case ResultSet.TYPE_SCROLL_INSENSITIVE:
				break;
			case ResultSet.TYPE_SCROLL_SENSITIVE:
				drv.debugPrintln("not supported: "+
						"TYPE_SCROLL_SENSITIVE");
				throwFeatureNotSupportedException();
				break;
			default:
				drv.debugPrintln("not supported: "+
							"unknown - "+
							type);
				throwFeatureNotSupportedException();
				break;
		}
	}

	void throwResultSetConcurrencyNotSupportedException(
						int concurrency)
						throws SQLException {
		switch (concurrency) {
			case ResultSet.CONCUR_READ_ONLY:
				break;
			case ResultSet.CONCUR_UPDATABLE:
				drv.debugPrintln("not supported: "+
						"CONCUR_UPDATABLE");
				throwFeatureNotSupportedException();
				break;
			default:
				drv.debugPrintln("not supported: "+
							"unknown - "+
							concurrency);
				throwFeatureNotSupportedException();
				break;
		}
	}

	void throwResultSetHoldabiltyNotSupportedException(
						int holdability)
						throws SQLException {
		switch (holdability) {
			case ResultSet.HOLD_CURSORS_OVER_COMMIT:
				break;
			case ResultSet.CLOSE_CURSORS_AT_COMMIT:
				drv.debugPrintln("not supported: "+
						"CLOSE_CURSORS_AT_COMMIT");
				throwFeatureNotSupportedException();
				break;
			default:
				drv.debugPrintln("not supported: "+
							"unknown - "+
							holdability);
				throwFeatureNotSupportedException();
				break;
		}
	}

	void throwResultSetFeatureNotSupportedException(
						int type,
						int concurrency,
						int holdability)
						throws SQLException {
	 	throwResultSetTypeNotSupportedException(type);
	 	throwResultSetConcurrencyNotSupportedException(concurrency);
		throwResultSetHoldabiltyNotSupportedException(holdability);
	}

	void throwAutoGeneratedKeysNotSupportedException(
						int autoGeneratedKeys)
						throws SQLException {
		switch (autoGeneratedKeys) {
			case Statement.NO_GENERATED_KEYS:
				break;
			case Statement.RETURN_GENERATED_KEYS:
				drv.debugPrintln("not supported: "+
						"RETURN_GENERATED_KEYS");
				throwFeatureNotSupportedException();
				break;
			default:
				drv.debugPrintln("not supported: "+
							"unknown - "+
							autoGeneratedKeys);
				throwFeatureNotSupportedException();
				break;
		}
	}

	void throwFeatureNotSupportedException() throws SQLException {
		drv.debugPrintln(
			"exception: SQLFeatureNotSupportedException");
		drv.debugZeroIndent();
		throw new SQLFeatureNotSupportedException();
	}

	void throwException(String reason) throws SQLException {
		drv.debugPrintln("exception: "+reason);
		drv.debugZeroIndent();
		throw new SQLException(reason);
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
		return (T)((iface==SQLRConnection.class)?sqlrcon:null);
	}
}
