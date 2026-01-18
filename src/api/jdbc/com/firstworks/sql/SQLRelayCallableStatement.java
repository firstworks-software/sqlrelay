package com.firstworks.sql;

import java.sql.*;

import java.io.InputStream;
import java.io.Reader;
import java.math.BigDecimal;
import java.util.Calendar;
import java.util.Map;
import java.net.URL;

import com.firstworks.sqlrelay.*;

public class SQLRelayCallableStatement
		extends SQLRelayPreparedStatement
		implements CallableStatement {

	public SQLRelayCallableStatement(SQLRelayDriver driver)
							throws SQLException {
		super(driver);
		driver.debugFunction(this);
		driver.debugEnd();
	}

	public synchronized
	Array getArray(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Array getArray(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	BigDecimal getBigDecimal(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	BigDecimal getBigDecimal(int parameterIndex, int scale)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	BigDecimal getBigDecimal(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Blob getBlob(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Blob getBlob(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	boolean getBoolean(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return false;
	}

	public synchronized
	boolean getBoolean(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return false;
	}

	public synchronized
	byte getByte(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return 0;
	}

	public synchronized
	byte getByte(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return 0;
	}

	public synchronized
	byte[] getBytes(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	byte[] getBytes(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Reader getCharacterStream(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Reader getCharacterStream(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Clob getClob(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Clob getClob(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Date getDate(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Date getDate(int parameterIndex, Calendar cal) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Date getDate(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Date getDate(String parameterName, Calendar cal) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	double getDouble(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return 0.0;
	}

	public synchronized
	double getDouble(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return 0.0;
	}

	public synchronized
	float getFloat(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return 0.0f;
	}

	public synchronized
	float getFloat(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return 0.0f;
	}

	public synchronized
	int getInt(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return 0;
	}

	public synchronized
	int getInt(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return 0;
	}

	public synchronized
	long getLong(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return 0;
	}

	public synchronized
	long getLong(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return 0;
	}

	public synchronized
	Reader getNCharacterStream(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Reader getNCharacterStream(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	NClob getNClob(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	NClob getNClob(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	String getNString(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	String getNString(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Object getObject(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	<T> T getObject(int parameterIndex, Class<T> type) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Object getObject(int parameterIndex, Map<String,Class<?>> map)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Object getObject(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	<T> T getObject(String parameterName, Class<T> type)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Object getObject(String parameterName, Map<String,Class<?>> map)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Ref getRef(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Ref getRef(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	RowId getRowId(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	RowId getRowId(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	short getShort(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return 0;
	}

	public synchronized
	short getShort(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return 0;
	}

	public synchronized
	SQLXML getSQLXML(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	SQLXML getSQLXML(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	String getString(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	String getString(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Time getTime(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Time getTime(int parameterIndex, Calendar cal) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Time getTime(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Time getTime(String parameterName, Calendar cal) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Timestamp getTimestamp(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Timestamp getTimestamp(int parameterIndex, Calendar cal)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Timestamp getTimestamp(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Timestamp getTimestamp(String parameterName, Calendar cal)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	URL getURL(int parameterIndex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	URL getURL(String parameterName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	void 	registerOutParameter(int parameterIndex, int sqlType)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void 	registerOutParameter(int parameterIndex, int sqlType,
							int scale)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void 	registerOutParameter(int parameterIndex, int sqlType,
							String typeName)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void 	registerOutParameter(String parameterName, int sqlType)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void 	registerOutParameter(String parameterName, int sqlType,
							int scale)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void 	registerOutParameter(String parameterName, int sqlType,
							String typeName)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setAsciiStream(String parameterName, InputStream x)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setAsciiStream(String parameterName, InputStream x,
							int length)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setAsciiStream(String parameterName, InputStream x,
							long length)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setBigDecimal(String parameterName, BigDecimal x)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setBinaryStream(String parameterName, InputStream x)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setBinaryStream(String parameterName, InputStream x,
							int length)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setBinaryStream(String parameterName, InputStream x,
							long length)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setBlob(String parameterName, Blob x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setBlob(String parameterName, InputStream inputStream)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setBlob(String parameterName, InputStream inputStream,
						long length)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setBoolean(String parameterName, boolean x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setByte(String parameterName, byte x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setBytes(String parameterName, byte[] x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setCharacterStream(String parameterName, Reader reader)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setCharacterStream(String parameterName, Reader reader,
							int length)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setCharacterStream(String parameterName, Reader reader,
							long length)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setClob(String parameterName, Clob x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setClob(String parameterName, Reader reader) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setClob(String parameterName, Reader reader, long length)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setDate(String parameterName, Date x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setDate(String parameterName, Date x, Calendar cal)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setDouble(String parameterName, double x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setFloat(String parameterName, float x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setInt(String parameterName, int x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setLong(String parameterName, long x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setNCharacterStream(String parameterName, Reader value)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setNCharacterStream(String parameterName, Reader value,
							long length)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setNClob(String parameterName, NClob value) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setNClob(String parameterName, Reader reader) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setNClob(String parameterName, Reader reader, long length)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setNString(String parameterName, String value)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setNull(String parameterName, int sqlType) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setNull(String parameterName, int sqlType, String typeName)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setObject(String parameterName, Object x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setObject(String parameterName, Object x, int targetSqlType)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setObject(String parameterName, Object x, int targetSqlType,
							int scale)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setRowId(String parameterName, RowId x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setShort(String parameterName, short x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setSQLXML(String parameterName, SQLXML xmlObject)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setString(String parameterName, String x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setTime(String parameterName, Time x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setTime(String parameterName, Time x, Calendar cal)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setTimestamp(String parameterName, Timestamp x)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setTimestamp(String parameterName, Timestamp x,
						Calendar cal)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void setURL(String parameterName, URL val) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	boolean  wasNull() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return false;
	}
}
