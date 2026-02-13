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

	private boolean	wasnull=false;

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
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		Array	retval=(wasnull)?null:(Array)obj;
		drv.debugPrintln("value: "+
			((wasnull)?"null":retval.getBaseTypeName()));
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
		BigDecimal	retval=getBigDecimal(parameterIndex);
		drv.debugEnd();
		return retval;
	}

	public
	BigDecimal getBigDecimal(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		BigDecimal	retval=(wasnull)?null:(BigDecimal)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
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
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		Blob	retval=(wasnull)?null:(Blob)obj;
		drv.debugPrintln("value: "+
			((wasnull)?"null":new String(blobToBytes(retval))));
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
		wasnull=(obj==null);
		boolean	retval=(wasnull)?false:(Boolean)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
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
		wasnull=(obj==null);
		byte	retval=(wasnull)?0:(Byte)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
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
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		byte[]	retval=(wasnull)?null:(byte[])obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	Reader getCharacterStream(int parameterIndex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		Reader	retval=getCharacterStream(
					String.valueOf(parameterIndex));
		drv.debugEnd();
		return retval;
	}

	public
	Reader getCharacterStream(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		Reader	retval=(wasnull)?null:(Reader)obj;
		drv.debugPrintln("value: "+
			((wasnull)?"null":readerToString(retval)));
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
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		Clob	retval=(wasnull)?null:(Clob)obj;
		drv.debugPrintln("value: "+
			((wasnull)?"null":clobToString(retval)));
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
		Date	retval=getDate(String.valueOf(parameterIndex),cal);
		drv.debugEnd();
		return retval;
	}

	public
	Date getDate(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		Date	retval=(wasnull)?null:(Date)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	Date getDate(String parameterName, Calendar cal) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		Date	retval=null;
		if (!wasnull) {
			if (cal!=null) {
				Calendar defCal=Calendar.getInstance();
				defCal.setTime((Date)obj);
				cal.set(Calendar.YEAR,
					defCal.get(Calendar.YEAR));
				cal.set(Calendar.MONTH,
					defCal.get(Calendar.MONTH));
				cal.set(Calendar.DAY_OF_MONTH,
					defCal.get(Calendar.DAY_OF_MONTH));
				cal.set(Calendar.HOUR_OF_DAY,0);
				cal.set(Calendar.MINUTE,0);
				cal.set(Calendar.SECOND,0);
				cal.set(Calendar.MILLISECOND,0);
				retval=new Date(cal.getTimeInMillis());
			} else {
				retval=(Date)obj;
			}
		}
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
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
		wasnull=(obj==null);
		double	retval=(wasnull)?0.0:(Double)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
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
		wasnull=(obj==null);
		float	retval=(wasnull)?0.0f:(Float)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
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
		wasnull=(obj==null);
		int	retval=(wasnull)?0:(Integer)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
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
		wasnull=(obj==null);
		long	retval=(wasnull)?0:(Long)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
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
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		Reader	retval=(wasnull)?null:(Reader)obj;
		drv.debugPrintln("value: "+
			((wasnull)?"null":readerToString(retval)));
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
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		NClob	retval=(wasnull)?null:(NClob)obj;
		drv.debugPrintln("value: "+
			((wasnull)?"null":clobToString(retval)));
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
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		String	retval=(wasnull)?null:(String)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
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
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		Object	retval=obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	<T> T getObject(String parameterName, Class<T> type)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		T	retval=(wasnull)?null:type.cast(obj);
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	Object getObject(String parameterName, Map<String,Class<?>> map)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		Object	retval=obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
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
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		Ref	retval=(wasnull)?null:(Ref)obj;
		drv.debugPrintln("value: "+
			((wasnull)?"null":retval.getBaseTypeName()));
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
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		RowId	retval=(wasnull)?null:(RowId)obj;
		drv.debugPrintln("value: "+
			((wasnull)?"null":retval.toString()));
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
		wasnull=(obj==null);
		short	retval=(wasnull)?0:(Short)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
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
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		SQLXML	retval=(wasnull)?null:(SQLXML)obj;
		drv.debugPrintln("value: "+
			((wasnull)?"null":retval.getString()));
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
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		String	retval=(wasnull)?null:(String)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
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
		Time	retval=getTime(String.valueOf(parameterIndex),cal);
		drv.debugEnd();
		return retval;
	}

	public
	Time getTime(String parameterName) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		Time	retval=(wasnull)?null:(Time)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	Time getTime(String parameterName, Calendar cal) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		Time	retval=null;
		if (!wasnull) {
			if (cal!=null) {
				Calendar defCal=Calendar.getInstance();
				defCal.setTime((Time)obj);
				cal.set(Calendar.YEAR,1970);
				cal.set(Calendar.MONTH,Calendar.JANUARY);
				cal.set(Calendar.DAY_OF_MONTH,1);
				cal.set(Calendar.HOUR_OF_DAY,
					defCal.get(Calendar.HOUR_OF_DAY));
				cal.set(Calendar.MINUTE,
					defCal.get(Calendar.MINUTE));
				cal.set(Calendar.SECOND,
					defCal.get(Calendar.SECOND));
				cal.set(Calendar.MILLISECOND,
					defCal.get(Calendar.MILLISECOND));
				retval=new Time(cal.getTimeInMillis());
			} else {
				retval=(Time)obj;
			}
		}
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
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
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		Timestamp	retval=(wasnull)?null:(Timestamp)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	Timestamp getTimestamp(String parameterName, Calendar cal)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parameterName);
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		Timestamp	retval=null;
		if (!wasnull) {
			if (cal!=null) {
				Calendar defCal=Calendar.getInstance();
				defCal.setTime((Timestamp)obj);
				cal.set(Calendar.YEAR,
					defCal.get(Calendar.YEAR));
				cal.set(Calendar.MONTH,
					defCal.get(Calendar.MONTH));
				cal.set(Calendar.DAY_OF_MONTH,
					defCal.get(Calendar.DAY_OF_MONTH));
				cal.set(Calendar.HOUR_OF_DAY,
					defCal.get(Calendar.HOUR_OF_DAY));
				cal.set(Calendar.MINUTE,
					defCal.get(Calendar.MINUTE));
				cal.set(Calendar.SECOND,
					defCal.get(Calendar.SECOND));
				cal.set(Calendar.MILLISECOND,
					defCal.get(Calendar.MILLISECOND));
				retval=new Timestamp(cal.getTimeInMillis());
				retval.setNanos(((Timestamp)obj).getNanos());
			} else {
				retval=(Timestamp)obj;
			}
		}
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
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
		Object	obj=parameters.get(parameterName).getObject();
		wasnull=(obj==null);
		URL	retval=(wasnull)?null:(URL)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	void registerOutParameter(int parameterIndex, int sqlType)
							throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		registerOutParameter(String.valueOf(parameterIndex),sqlType);
		drv.debugEnd();
	}

	public
	void registerOutParameter(int parameterIndex, int sqlType,
							int scale)
							throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		registerOutParameter(String.valueOf(parameterIndex),
							sqlType,scale);
		drv.debugEnd();
	}

	public
	void registerOutParameter(int parameterIndex, int sqlType,
							String typeName)
							throws SQLException {
		drv.debugFunction(this);
		registerOutParameter(String.valueOf(parameterIndex),
							sqlType,typeName);
		drv.debugEnd();
	}

	public
	void registerOutParameter(String parameterName, int sqlType)
							throws SQLException {
		drv.debugFunction(this);
		registerOutParameter(parameterName,sqlType,0);
		drv.debugEnd();
	}

	public
	void registerOutParameter(String parameterName, int sqlType,
							int scale)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);
		drv.debugPrintln("sql type: "+sqlType);
		drv.debugPrintln("scale: "+scale);

		SQLRelayParameter.BindType	bindtype;
		boolean				signed=false;
		boolean				binary=false;
		boolean				lob=false;
		boolean				ascii=false;
		String				classname;
		String				typename;

		switch (sqlType) {
			case Types.ARRAY:
				bindtype=SQLRelayParameter.BindType.Array;
				classname="java.sql.Array";
				typename="ARRAY";
				break;
			case Types.BIGINT:
				bindtype=SQLRelayParameter.BindType.Long;
				signed=true;
				classname="java.lang.Long";
				typename="BIGINT";
				break;
			case Types.BINARY:
			case Types.VARBINARY:
			case Types.LONGVARBINARY:
				bindtype=SQLRelayParameter.BindType.Bytes;
				binary=true;
				classname="[B";
				typename="VARBINARY";
				break;
			case Types.BIT:
			case Types.BOOLEAN:
				bindtype=SQLRelayParameter.BindType.Boolean;
				classname="java.lang.Boolean";
				typename="BOOLEAN";
				break;
			case Types.BLOB:
				bindtype=SQLRelayParameter.BindType.Blob;
				binary=true;
				lob=true;
				classname="java.sql.Blob";
				typename="BLOB";
				break;
			case Types.CHAR:
			case Types.VARCHAR:
			case Types.LONGVARCHAR:
				bindtype=SQLRelayParameter.BindType.String;
				ascii=true;
				classname="java.lang.String";
				typename="VARCHAR";
				break;
			case Types.CLOB:
				bindtype=SQLRelayParameter.BindType.Clob;
				lob=true;
				ascii=true;
				classname="java.sql.Clob";
				typename="CLOB";
				break;
			case Types.DATE:
				bindtype=SQLRelayParameter.BindType.Date;
				classname="java.sql.Date";
				typename="DATE";
				break;
			case Types.DECIMAL:
			case Types.NUMERIC:
				bindtype=SQLRelayParameter.BindType.BigDecimal;
				signed=true;
				classname="java.math.BigDecimal";
				typename="DECIMAL";
				break;
			case Types.DOUBLE:
				bindtype=SQLRelayParameter.BindType.Double;
				signed=true;
				classname="java.lang.Double";
				typename="DOUBLE";
				break;
			case Types.FLOAT:
			case Types.REAL:
				bindtype=SQLRelayParameter.BindType.Float;
				signed=true;
				classname="java.lang.Float";
				typename="FLOAT";
				break;
			case Types.INTEGER:
				bindtype=SQLRelayParameter.BindType.Int;
				signed=true;
				classname="java.lang.Integer";
				typename="INTEGER";
				break;
			case Types.NCHAR:
			case Types.NVARCHAR:
			case Types.LONGNVARCHAR:
				bindtype=SQLRelayParameter.BindType.NString;
				classname="java.lang.String";
				typename="NVARCHAR";
				break;
			case Types.NCLOB:
				bindtype=SQLRelayParameter.BindType.NClob;
				lob=true;
				classname="java.sql.NClob";
				typename="NCLOB";
				break;
			case Types.NULL:
				bindtype=SQLRelayParameter.BindType.Null;
				classname="java.lang.Object";
				typename="NULL";
				break;
			case Types.REF:
				bindtype=SQLRelayParameter.BindType.Ref;
				classname="java.sql.Ref";
				typename="REF";
				break;
			case Types.ROWID:
				bindtype=SQLRelayParameter.BindType.RowId;
				classname="java.sql.RowId";
				typename="ROWID";
				break;
			case Types.SMALLINT:
				bindtype=SQLRelayParameter.BindType.Short;
				signed=true;
				classname="java.lang.Short";
				typename="SMALLINT";
				break;
			case Types.SQLXML:
				bindtype=SQLRelayParameter.BindType.SQLXML;
				classname="java.sql.SQLXML";
				typename="SQLXML";
				break;
			case Types.TIME:
				bindtype=SQLRelayParameter.BindType.Time;
				classname="java.sql.Time";
				typename="TIME";
				break;
			case Types.TIMESTAMP:
				bindtype=SQLRelayParameter.BindType.Timestamp;
				classname="java.sql.Timestamp";
				typename="TIMESTAMP";
				break;
			case Types.TINYINT:
				bindtype=SQLRelayParameter.BindType.Byte;
				signed=true;
				classname="java.lang.Byte";
				typename="TINYINT";
				break;
			default:
				bindtype=SQLRelayParameter.BindType.String;
				ascii=true;
				classname="java.lang.String";
				typename="VARCHAR";
				break;
		}

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName(classname);
		param.setMode(ParameterMetaData.parameterModeOut);
		param.setType(sqlType);
		param.setTypeName(typename);
		param.setPrecision(0);
		param.setScale(scale);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(signed);
		param.setObject(null);
		param.setLength(-1);
		param.setIsBinary(binary);
		param.setIsLob(lob);
		param.setIsAscii(ascii);
		param.setCalendar(null);
		param.setBindType(bindtype);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void registerOutParameter(String parameterName, int sqlType,
							String typeName)
							throws SQLException {
		drv.debugFunction(this);
		registerOutParameter(parameterName,sqlType,0);
		parameters.get(parameterName).setTypeName(typeName);
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
		drv.debugPrintln(""+wasnull);
		drv.debugEnd();
		return wasnull;
	}
}
