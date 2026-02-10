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
		drv.debugPrintln("parameter index: "+parameterIndex);
		Array	retval=getArray(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	Array getArray(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Array	retval=(Array)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+retval.getBaseTypeName());
		drv.debugEnd();
		return retval;
	}

	public
	BigDecimal getBigDecimal(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		BigDecimal	retval=
				getBigDecimal(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	BigDecimal getBigDecimal(int parameterIndex, int scale)
							throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		BigDecimal	retval=
				getBigDecimal(String.valueOf(parameterIndex));
		retval=(retval!=null)?retval.setScale(scale):null;
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	BigDecimal getBigDecimal(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		BigDecimal	retval=(BigDecimal)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Blob getBlob(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		Blob	retval=getBlob(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	Blob getBlob(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Blob	retval=(Blob)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+new String(blobToBytes(retval)));
		drv.debugEnd();
		return retval;
	}

	public
	boolean getBoolean(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		boolean	retval=getBoolean(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	boolean getBoolean(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Object	obj=parameters.get(parameterName).getObject();
		boolean	retval=(obj!=null)?(Boolean)obj:false;
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	byte getByte(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		byte	retval=getByte(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	byte getByte(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Object	obj=parameters.get(parameterName).getObject();
		byte	retval=(obj!=null)?(Byte)obj:0;
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	byte[] getBytes(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		byte[]	retval=getBytes(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	byte[] getBytes(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		byte[]	retval=(byte[])
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Reader getCharacterStream(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		Reader	retval=getCharacterStream(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	Reader getCharacterStream(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Reader	retval=(Reader)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+readerToString(retval));
		drv.debugEnd();
		return retval;
	}

	public
	Clob getClob(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		Clob	retval=getClob(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	Clob getClob(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Clob	retval=(Clob)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+clobToString(retval));
		drv.debugEnd();
		return retval;
	}

	public
	Date getDate(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		Date	retval=getDate(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	Date getDate(int parameterIndex, Calendar cal) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		// FIXME: use cal?
		Date	retval=getDate(String.valueOf(parameterIndex),cal);
		drv.debugEnd();
		return retval;
	}

	public
	Date getDate(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Date	retval=(Date)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Date getDate(String parameterName, Calendar cal) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		// FIXME: use cal?
		Date	retval=(Date)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	double getDouble(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		double	retval=getDouble(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	double getDouble(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Object	obj=parameters.get(parameterName).getObject();
		double	retval=(obj!=null)?(Double)obj:0.0;
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	float getFloat(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		float	retval=getFloat(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	float getFloat(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Object	obj=parameters.get(parameterName).getObject();
		float	retval=(obj!=null)?(Float)obj:0.0f;
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	int getInt(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		int	retval=getInt(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	int getInt(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Object	obj=parameters.get(parameterName).getObject();
		int	retval=(obj!=null)?(Integer)obj:0;
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	long getLong(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		long	retval=getLong(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	long getLong(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Object	obj=parameters.get(parameterName).getObject();
		long	retval=(obj!=null)?(Long)obj:0;
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Reader getNCharacterStream(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		Reader	retval=
			getNCharacterStream(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	Reader getNCharacterStream(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Reader	retval=(Reader)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+readerToString(retval));
		drv.debugEnd();
		return retval;
	}

	public
	NClob getNClob(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		NClob	retval=getNClob(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	NClob getNClob(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		NClob	retval=(NClob)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+clobToString(retval));
		drv.debugEnd();
		return retval;
	}

	public
	String getNString(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		String	retval=getNString(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	String getNString(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		String	retval=(String)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Object getObject(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		Object	retval=getObject(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	<T> T getObject(int parameterIndex, Class<T> type) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		T	retval=getObject(String.valueOf(parameterIndex),type);
		drv.debugEnd();
		return retval;
	}

	public
	Object getObject(int parameterIndex, Map<String,Class<?>> map)
							throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		Object	retval=getObject(String.valueOf(parameterIndex),map);
		drv.debugEnd();
		return retval;
	}

	public
	Object getObject(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Object	retval=
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	<T> T getObject(String parameterName, Class<T> type)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		T	retval=type.cast(
			parameters.get(parameterName).getObject());
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Object getObject(String parameterName, Map<String,Class<?>> map)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Object	retval=
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Ref getRef(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		Ref	retval=getRef(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	Ref getRef(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Ref	retval=(Ref)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+retval.getBaseTypeName());
		drv.debugEnd();
		return retval;
	}

	public
	RowId getRowId(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		RowId	retval=getRowId(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	RowId getRowId(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		RowId	retval=(RowId)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+retval.toString());
		drv.debugEnd();
		return retval;
	}

	public
	short getShort(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		short	retval=getShort(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	short getShort(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Object	obj=parameters.get(parameterName).getObject();
		short	retval=(obj!=null)?(Short)obj:0;
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	SQLXML getSQLXML(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		SQLXML	retval=getSQLXML(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	SQLXML getSQLXML(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		SQLXML	retval=(SQLXML)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+retval.getString());
		drv.debugEnd();
		return retval;
	}

	public
	String getString(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		String	retval=getString(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	String getString(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		String	retval=(String)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Time getTime(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		Time	retval=getTime(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	Time getTime(int parameterIndex, Calendar cal) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		// FIXME: use cal?
		Time	retval=getTime(String.valueOf(parameterIndex),cal);
		drv.debugEnd();
		return retval;
	}

	public
	Time getTime(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Time	retval=(Time)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Time getTime(String parameterName, Calendar cal) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		// FIXME: use cal?
		Time	retval=(Time)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Timestamp getTimestamp(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		Timestamp	retval=
			getTimestamp(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	Timestamp getTimestamp(int parameterIndex, Calendar cal)
							throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		// FIXME: use cal?
		Timestamp	retval=
			getTimestamp(String.valueOf(parameterIndex),cal);
		drv.debugEnd();
		return retval;
	}

	public
	Timestamp getTimestamp(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Timestamp	retval=(Timestamp)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	Timestamp getTimestamp(String parameterName, Calendar cal)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		// FIXME: use cal?
		Timestamp	retval=(Timestamp)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	URL getURL(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		URL	retval=getURL(String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	URL getURL(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		URL	retval=(URL)
			parameters.get(parameterName).getObject();
		drv.debugPrintln("value: "+retval);
		drv.debugEnd();
		return retval;
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
		super.setAsciiStream(parameterName,x);
		drv.debugEnd();
	}

	public
	void setAsciiStream(String parameterName, InputStream x,
							int length)
							throws SQLException {
		drv.debugFunction(this);
		super.setAsciiStream(parameterName,x,length);
		drv.debugEnd();
	}

	public
	void setAsciiStream(String parameterName, InputStream x,
							long length)
							throws SQLException {
		drv.debugFunction(this);
		super.setAsciiStream(parameterName,x,length);
		drv.debugEnd();
	}

	public
	void setBigDecimal(String parameterName, BigDecimal x)
							throws SQLException {
		drv.debugFunction(this);
		super.setBigDecimal(parameterName,x);
		drv.debugEnd();
	}

	public
	void setBinaryStream(String parameterName, InputStream x)
							throws SQLException {
		drv.debugFunction(this);
		super.setBinaryStream(parameterName,x);
		drv.debugEnd();
	}

	public
	void setBinaryStream(String parameterName, InputStream x,
							int length)
							throws SQLException {
		drv.debugFunction(this);
		super.setBinaryStream(parameterName,x,length);
		drv.debugEnd();
	}

	public
	void setBinaryStream(String parameterName, InputStream x,
							long length)
							throws SQLException {
		drv.debugFunction(this);
		super.setBinaryStream(parameterName,x,length);
		drv.debugEnd();
	}

	public
	void setBlob(String parameterName, Blob x) throws SQLException {
		drv.debugFunction(this);
		super.setBlob(parameterName,x);
		drv.debugEnd();
	}

	public
	void setBlob(String parameterName, InputStream inputStream)
							throws SQLException {
		drv.debugFunction(this);
		super.setBlob(parameterName,inputStream);
		drv.debugEnd();
	}

	public
	void setBlob(String parameterName, InputStream inputStream,
						long length)
						throws SQLException {
		drv.debugFunction(this);
		super.setBlob(parameterName,inputStream,length);
		drv.debugEnd();
	}

	public
	void setBoolean(String parameterName, boolean x) throws SQLException {
		drv.debugFunction(this);
		super.setBoolean(parameterName,x);
		drv.debugEnd();
	}

	public
	void setByte(String parameterName, byte x) throws SQLException {
		drv.debugFunction(this);
		super.setByte(parameterName,x);
		drv.debugEnd();
	}

	public
	void setBytes(String parameterName, byte[] x) throws SQLException {
		drv.debugFunction(this);
		super.setBytes(parameterName,x);
		drv.debugEnd();
	}

	public
	void setCharacterStream(String parameterName, Reader reader)
							throws SQLException {
		drv.debugFunction(this);
		super.setCharacterStream(parameterName,reader);
		drv.debugEnd();
	}

	public
	void setCharacterStream(String parameterName, Reader reader,
							int length)
							throws SQLException {
		drv.debugFunction(this);
		super.setCharacterStream(parameterName,reader,length);
		drv.debugEnd();
	}

	public
	void setCharacterStream(String parameterName, Reader reader,
							long length)
							throws SQLException {
		drv.debugFunction(this);
		super.setCharacterStream(parameterName,reader,length);
		drv.debugEnd();
	}

	public
	void setClob(String parameterName, Clob x) throws SQLException {
		drv.debugFunction(this);
		super.setClob(parameterName,x);
		drv.debugEnd();
	}

	public
	void setClob(String parameterName, Reader reader) throws SQLException {
		drv.debugFunction(this);
		super.setClob(parameterName,reader);
		drv.debugEnd();
	}

	public
	void setClob(String parameterName, Reader reader, long length)
							throws SQLException {
		drv.debugFunction(this);
		super.setClob(parameterName,reader,length);
		drv.debugEnd();
	}

	public
	void setDate(String parameterName, Date x) throws SQLException {
		drv.debugFunction(this);
		super.setDate(parameterName,x);
		drv.debugEnd();
	}

	public
	void setDate(String parameterName, Date x, Calendar cal)
							throws SQLException {
		drv.debugFunction(this);
		super.setDate(parameterName,x,cal);
		drv.debugEnd();
	}

	public
	void setDouble(String parameterName, double x) throws SQLException {
		drv.debugFunction(this);
		super.setDouble(parameterName,x);
		drv.debugEnd();
	}

	public
	void setFloat(String parameterName, float x) throws SQLException {
		drv.debugFunction(this);
		super.setFloat(parameterName,x);
		drv.debugEnd();
	}

	public
	void setInt(String parameterName, int x) throws SQLException {
		drv.debugFunction(this);
		super.setInt(parameterName,x);
		drv.debugEnd();
	}

	public
	void setLong(String parameterName, long x) throws SQLException {
		drv.debugFunction(this);
		super.setLong(parameterName,x);
		drv.debugEnd();
	}

	public
	void setNCharacterStream(String parameterName, Reader value)
							throws SQLException {
		drv.debugFunction(this);
		super.setNCharacterStream(parameterName,value);
		drv.debugEnd();
	}

	public
	void setNCharacterStream(String parameterName, Reader value,
							long length)
							throws SQLException {
		drv.debugFunction(this);
		super.setNCharacterStream(parameterName,value,length);
		drv.debugEnd();
	}

	public
	void setNClob(String parameterName, NClob value) throws SQLException {
		drv.debugFunction(this);
		super.setNClob(parameterName,value);
		drv.debugEnd();
	}

	public
	void setNClob(String parameterName, Reader reader) throws SQLException {
		drv.debugFunction(this);
		super.setNClob(parameterName,reader);
		drv.debugEnd();
	}

	public
	void setNClob(String parameterName, Reader reader, long length)
							throws SQLException {
		drv.debugFunction(this);
		super.setNClob(parameterName,reader,length);
		drv.debugEnd();
	}

	public
	void setNString(String parameterName, String value)
							throws SQLException {
		drv.debugFunction(this);
		super.setNString(parameterName,value);
		drv.debugEnd();
	}

	public
	void setNull(String parameterName, int sqlType) throws SQLException {
		drv.debugFunction(this);
		super.setNull(parameterName,sqlType);
		drv.debugEnd();
	}

	public
	void setNull(String parameterName, int sqlType, String typeName)
							throws SQLException {
		drv.debugFunction(this);
		super.setNull(parameterName,sqlType,typeName);
		drv.debugEnd();
	}

	public
	void setObject(String parameterName, Object x) throws SQLException {
		drv.debugFunction(this);
		super.setObject(parameterName,x);
		drv.debugEnd();
	}

	public
	void setObject(String parameterName, Object x, int targetSqlType)
							throws SQLException {
		drv.debugFunction(this);
		super.setObject(parameterName,x,targetSqlType);
		drv.debugEnd();
	}

	public
	void setObject(String parameterName, Object x, int targetSqlType,
							int scale)
							throws SQLException {
		drv.debugFunction(this);
		super.setObject(parameterName,x,targetSqlType,scale);
		drv.debugEnd();
	}

	public
	void setRowId(String parameterName, RowId x) throws SQLException {
		drv.debugFunction(this);
		super.setRowId(parameterName,x);
		drv.debugEnd();
	}

	public
	void setShort(String parameterName, short x) throws SQLException {
		drv.debugFunction(this);
		super.setShort(parameterName,x);
		drv.debugEnd();
	}

	public
	void setSQLXML(String parameterName, SQLXML xmlObject)
							throws SQLException {
		drv.debugFunction(this);
		super.setSQLXML(parameterName,xmlObject);
		drv.debugEnd();
	}

	public
	void setString(String parameterName, String x) throws SQLException {
		drv.debugFunction(this);
		super.setString(parameterName,x);
		drv.debugEnd();
	}

	public
	void setTime(String parameterName, Time x) throws SQLException {
		drv.debugFunction(this);
		super.setTime(parameterName,x);
		drv.debugEnd();
	}

	public
	void setTime(String parameterName, Time x, Calendar cal)
							throws SQLException {
		drv.debugFunction(this);
		super.setTime(parameterName,x,cal);
		drv.debugEnd();
	}

	public
	void setTimestamp(String parameterName, Timestamp x)
							throws SQLException {
		drv.debugFunction(this);
		super.setTimestamp(parameterName,x);
		drv.debugEnd();
	}

	public
	void setTimestamp(String parameterName, Timestamp x,
						Calendar cal)
						throws SQLException {
		drv.debugFunction(this);
		super.setTimestamp(parameterName,x,cal);
		drv.debugEnd();
	}

	public
	void setURL(String parameterName, URL val) throws SQLException {
		drv.debugFunction(this);
		super.setURL(parameterName,val);
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
