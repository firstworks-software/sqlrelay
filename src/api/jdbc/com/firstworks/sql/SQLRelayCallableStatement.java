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

	public
	SQLRelayCallableStatement(SQLRelayDriver driver) throws SQLException {
		super(driver);
		drv.debugFunction(this);
		drv.debugEnd();
	}

	public
	Array getArray(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Array	retval=(Array)
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+retval.getBaseTypeName());
		drv.debugEnd();
		return retval;
	}

	public
	Array getArray(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	BigDecimal getBigDecimal(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		BigDecimal	retval=(BigDecimal)
				parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	BigDecimal getBigDecimal(int parameterIndex, int scale)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		BigDecimal	retval=(BigDecimal)
				parameters.get(parameterIndex).getObject();
		retval=(retval!=null)?retval.setScale(scale):null;
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	BigDecimal getBigDecimal(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	Blob getBlob(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Blob	retval=(Blob)
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+new String(blobToBytes(retval)));
		drv.debugEnd();
		return retval;
	}

	public
	Blob getBlob(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	boolean getBoolean(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Object	obj=parameters.get(parameterIndex).getObject();
		boolean	retval=(obj!=null)?(Boolean)obj:false;
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	boolean getBoolean(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return false;
	}

	public
	byte getByte(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Object	obj=parameters.get(parameterIndex).getObject();
		byte	retval=(obj!=null)?(Byte)obj:0;
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	byte getByte(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return 0;
	}

	public
	byte[] getBytes(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		byte[]	retval=(byte[])
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	byte[] getBytes(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	Reader getCharacterStream(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Reader	retval=(Reader)
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+readerToString(retval));
		drv.debugEnd();
		return retval;
	}

	public
	Reader getCharacterStream(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	Clob getClob(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Clob	retval=(Clob)
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+clobToString(retval));
		drv.debugEnd();
		return retval;
	}

	public
	Clob getClob(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	Date getDate(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Date	retval=(Date)
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Date getDate(int parameterIndex, Calendar cal) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Date	retval=(Date)
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Date getDate(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	Date getDate(String parameterName, Calendar cal) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	double getDouble(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Object	obj=parameters.get(parameterIndex).getObject();
		double	retval=(obj!=null)?(Double)obj:0.0;
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	double getDouble(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return 0.0;
	}

	public
	float getFloat(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Object	obj=parameters.get(parameterIndex).getObject();
		float	retval=(obj!=null)?(Float)obj:0.0f;
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	float getFloat(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return 0.0f;
	}

	public
	int getInt(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Object	obj=parameters.get(parameterIndex).getObject();
		int	retval=(obj!=null)?(Integer)obj:0;
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	int getInt(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return 0;
	}

	public
	long getLong(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Object	obj=parameters.get(parameterIndex).getObject();
		long	retval=(obj!=null)?(Long)obj:0;
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	long getLong(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return 0;
	}

	public
	Reader getNCharacterStream(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Reader	retval=(Reader)
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+readerToString(retval));
		drv.debugEnd();
		return retval;
	}

	public
	Reader getNCharacterStream(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	NClob getNClob(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		NClob	retval=(NClob)
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+clobToString(retval));
		drv.debugEnd();
		return retval;
	}

	public
	NClob getNClob(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	String getNString(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		String	retval=(String)
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	String getNString(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	Object getObject(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Object	retval=
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	<T> T getObject(int parameterIndex, Class<T> type) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		T	retval=type.cast(
				parameters.get(parameterIndex).getObject());
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Object getObject(int parameterIndex, Map<String,Class<?>> map)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Object	retval=
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Object getObject(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	<T> T getObject(String parameterName, Class<T> type)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	Object getObject(String parameterName, Map<String,Class<?>> map)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	Ref getRef(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Ref	retval=(Ref)
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+retval.getBaseTypeName());
		drv.debugEnd();
		return retval;
	}

	public
	Ref getRef(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	RowId getRowId(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		RowId	retval=(RowId)
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+retval.toString());
		drv.debugEnd();
		return retval;
	}

	public
	RowId getRowId(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	short getShort(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Object	obj=parameters.get(parameterIndex).getObject();
		short	retval=(obj!=null)?(Short)obj:0;
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	short getShort(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return 0;
	}

	public
	SQLXML getSQLXML(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		SQLXML	retval=(SQLXML)
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+retval.getString());
		drv.debugEnd();
		return retval;
	}

	public
	SQLXML getSQLXML(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	String getString(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		String	retval=(String)
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	String getString(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	Time getTime(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Time	retval=(Time)
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Time getTime(int parameterIndex, Calendar cal) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Time	retval=(Time)
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Time getTime(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	Time getTime(String parameterName, Calendar cal) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	Timestamp getTimestamp(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Timestamp	retval=(Timestamp)
				parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Timestamp getTimestamp(int parameterIndex, Calendar cal)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		Timestamp	retval=(Timestamp)
				parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Timestamp getTimestamp(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	Timestamp getTimestamp(String parameterName, Calendar cal)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	URL getURL(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter index: "+parameterIndex);
		URL	retval=(URL)
			parameters.get(parameterIndex).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	URL getURL(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	void registerOutParameter(int parameterIndex, int sqlType)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void registerOutParameter(int parameterIndex, int sqlType,
							int scale)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void registerOutParameter(int parameterIndex, int sqlType,
							String typeName)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void registerOutParameter(String parameterName, int sqlType)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void registerOutParameter(String parameterName, int sqlType,
							int scale)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void registerOutParameter(String parameterName, int sqlType,
							String typeName)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setAsciiStream(String parameterName, InputStream x)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setAsciiStream(String parameterName, InputStream x,
							int length)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setAsciiStream(String parameterName, InputStream x,
							long length)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setBigDecimal(String parameterName, BigDecimal x)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setBinaryStream(String parameterName, InputStream x)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setBinaryStream(String parameterName, InputStream x,
							int length)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setBinaryStream(String parameterName, InputStream x,
							long length)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setBlob(String parameterName, Blob x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setBlob(String parameterName, InputStream inputStream)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setBlob(String parameterName, InputStream inputStream,
						long length)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setBoolean(String parameterName, boolean x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setByte(String parameterName, byte x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setBytes(String parameterName, byte[] x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setCharacterStream(String parameterName, Reader reader)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setCharacterStream(String parameterName, Reader reader,
							int length)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setCharacterStream(String parameterName, Reader reader,
							long length)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setClob(String parameterName, Clob x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setClob(String parameterName, Reader reader) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setClob(String parameterName, Reader reader, long length)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setDate(String parameterName, Date x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setDate(String parameterName, Date x, Calendar cal)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setDouble(String parameterName, double x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setFloat(String parameterName, float x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setInt(String parameterName, int x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setLong(String parameterName, long x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setNCharacterStream(String parameterName, Reader value)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setNCharacterStream(String parameterName, Reader value,
							long length)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setNClob(String parameterName, NClob value) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setNClob(String parameterName, Reader reader) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setNClob(String parameterName, Reader reader, long length)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setNString(String parameterName, String value)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setNull(String parameterName, int sqlType) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setNull(String parameterName, int sqlType, String typeName)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setObject(String parameterName, Object x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setObject(String parameterName, Object x, int targetSqlType)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setObject(String parameterName, Object x, int targetSqlType,
							int scale)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setRowId(String parameterName, RowId x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setShort(String parameterName, short x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setSQLXML(String parameterName, SQLXML xmlObject)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setString(String parameterName, String x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setTime(String parameterName, Time x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setTime(String parameterName, Time x, Calendar cal)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setTimestamp(String parameterName, Timestamp x)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setTimestamp(String parameterName, Timestamp x,
						Calendar cal)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void setURL(String parameterName, URL val) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	boolean wasNull() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return false;
	}
}
