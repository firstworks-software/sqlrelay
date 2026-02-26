// Copyright (c) David Muse
// See the file COPYING for more information.
package com.firstworks.sql;

import java.sql.*;

import java.io.InputStream;
import java.io.Reader;
import java.math.BigDecimal;
import java.util.Calendar;
import java.util.Map;
import java.net.URL;

import com.firstworks.sqlrelay.*;

import com.firstworks.sql.SQLRelayParameter.*;

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
	Array getArray(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		Array	retval=getArray(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	Array getArray(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		Array	retval=(wasnull)?null:(Array)obj;
		drv.debugPrintln("value: "+
			((wasnull)?"null":retval.getBaseTypeName()));
		drv.debugEnd();
		return retval;
	}

	public
	BigDecimal getBigDecimal(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		BigDecimal	retval=
				getBigDecimal(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	BigDecimal getBigDecimal(int parameterindex, int scale)
							throws SQLException {
		drv.debugFunction(this);
		BigDecimal	retval=getBigDecimal(parameterindex);
		drv.debugEnd();
		return retval;
	}

	public
	BigDecimal getBigDecimal(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		BigDecimal	retval=(wasnull)?null:(BigDecimal)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	Blob getBlob(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		Blob	retval=getBlob(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	Blob getBlob(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		Blob	retval=(wasnull)?null:(Blob)obj;
		drv.debugPrintln("value: "+
			((wasnull)?"null":new String(blobToBytes(retval))));
		drv.debugEnd();
		return retval;
	}

	public
	boolean getBoolean(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		boolean	retval=getBoolean(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	boolean getBoolean(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		boolean	retval=(wasnull)?false:(Boolean)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	byte getByte(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		byte	retval=getByte(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	byte getByte(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		byte	retval=(wasnull)?0:(Byte)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	byte[] getBytes(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		byte[]	retval=getBytes(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	byte[] getBytes(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		byte[]	retval=(wasnull)?null:(byte[])obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	Reader getCharacterStream(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		Reader	retval=getCharacterStream(
					String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	Reader getCharacterStream(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		Reader	retval=(wasnull)?null:(Reader)obj;
		drv.debugPrintln("value: "+
			((wasnull)?"null":readerToString(retval)));
		drv.debugEnd();
		return retval;
	}

	public
	Clob getClob(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		Clob	retval=getClob(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	Clob getClob(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		Clob	retval=(wasnull)?null:(Clob)obj;
		drv.debugPrintln("value: "+
			((wasnull)?"null":clobToString(retval)));
		drv.debugEnd();
		return retval;
	}

	public
	Date getDate(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		Date	retval=getDate(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	Date getDate(int parameterindex, Calendar cal) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		Date	retval=getDate(String.valueOf(parameterindex),cal);
		drv.debugEnd();
		return retval;
	}

	public
	Date getDate(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		Date	retval=(wasnull)?null:(Date)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	Date getDate(String parametername, Calendar cal) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
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
	double getDouble(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		double	retval=getDouble(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	double getDouble(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		double	retval=(wasnull)?0.0:(Double)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	float getFloat(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		float	retval=getFloat(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	float getFloat(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		float	retval=(wasnull)?0.0f:(Float)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	int getInt(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		int	retval=getInt(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	int getInt(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		int	retval=(wasnull)?0:(Integer)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	long getLong(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		long	retval=getLong(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	long getLong(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		long	retval=(wasnull)?0:(Long)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	Reader getNCharacterStream(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		Reader	retval=
			getNCharacterStream(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	Reader getNCharacterStream(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		Reader	retval=(wasnull)?null:(Reader)obj;
		drv.debugPrintln("value: "+
			((wasnull)?"null":readerToString(retval)));
		drv.debugEnd();
		return retval;
	}

	public
	NClob getNClob(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		NClob	retval=getNClob(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	NClob getNClob(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		NClob	retval=(wasnull)?null:(NClob)obj;
		drv.debugPrintln("value: "+
			((wasnull)?"null":clobToString(retval)));
		drv.debugEnd();
		return retval;
	}

	public
	String getNString(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		String	retval=getNString(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	String getNString(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		String	retval=(wasnull)?null:(String)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	Object getObject(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		Object	retval=getObject(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	<T> T getObject(int parameterindex, Class<T> type) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		T	retval=getObject(String.valueOf(parameterindex),type);
		drv.debugEnd();
		return retval;
	}

	public
	Object getObject(int parameterindex, Map<String,Class<?>> map)
							throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		Object	retval=getObject(String.valueOf(parameterindex),map);
		drv.debugEnd();
		return retval;
	}

	public
	Object getObject(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		Object	retval=obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	<T> T getObject(String parametername, Class<T> type)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		T	retval=(wasnull)?null:type.cast(obj);
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	Object getObject(String parametername, Map<String,Class<?>> map)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		Object	retval=obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	Ref getRef(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		Ref	retval=getRef(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	Ref getRef(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		Ref	retval=(wasnull)?null:(Ref)obj;
		drv.debugPrintln("value: "+
			((wasnull)?"null":retval.getBaseTypeName()));
		drv.debugEnd();
		return retval;
	}

	public
	RowId getRowId(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		RowId	retval=getRowId(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	RowId getRowId(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		RowId	retval=(wasnull)?null:(RowId)obj;
		drv.debugPrintln("value: "+
			((wasnull)?"null":retval.toString()));
		drv.debugEnd();
		return retval;
	}

	public
	short getShort(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		short	retval=getShort(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	short getShort(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		short	retval=(wasnull)?0:(Short)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	SQLXML getSQLXML(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		SQLXML	retval=getSQLXML(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	SQLXML getSQLXML(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		SQLXML	retval=(wasnull)?null:(SQLXML)obj;
		drv.debugPrintln("value: "+
			((wasnull)?"null":retval.getString()));
		drv.debugEnd();
		return retval;
	}

	public
	String getString(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		String	retval=getString(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	String getString(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		String	retval=(wasnull)?null:(String)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	Time getTime(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		Time	retval=getTime(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	Time getTime(int parameterindex, Calendar cal) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		Time	retval=getTime(String.valueOf(parameterindex),cal);
		drv.debugEnd();
		return retval;
	}

	public
	Time getTime(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		Time	retval=(wasnull)?null:(Time)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	Time getTime(String parametername, Calendar cal) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
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
	Timestamp getTimestamp(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		Timestamp	retval=
			getTimestamp(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	Timestamp getTimestamp(int parameterindex, Calendar cal)
							throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		Timestamp	retval=
			getTimestamp(String.valueOf(parameterindex),cal);
		drv.debugEnd();
		return retval;
	}

	public
	Timestamp getTimestamp(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		Timestamp	retval=(wasnull)?null:(Timestamp)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	Timestamp getTimestamp(String parametername, Calendar cal)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
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
	URL getURL(int parameterindex) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		URL	retval=getURL(String.valueOf(parameterindex));
		drv.debugEnd();
		return retval;
	}

	public
	URL getURL(String parametername) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("parameter name: "+parametername);
		Object	obj=parameters.get(parametername).getObject();
		wasnull=(obj==null);
		URL	retval=(wasnull)?null:(URL)obj;
		drv.debugPrintln("value: "+((wasnull)?"null":""+retval));
		drv.debugEnd();
		return retval;
	}

	public
	void registerOutParameter(int parameterindex, int sqltype)
							throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		registerOutParameter(String.valueOf(parameterindex),sqltype);
		drv.debugEnd();
	}

	public
	void registerOutParameter(int parameterindex, int sqltype,
							int scale)
							throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterindex);
		registerOutParameter(String.valueOf(parameterindex),
							sqltype,scale);
		drv.debugEnd();
	}

	public
	void registerOutParameter(int parameterindex, int sqltype,
							String typename)
							throws SQLException {
		drv.debugFunction(this);
		registerOutParameter(String.valueOf(parameterindex),
							sqltype,typename);
		drv.debugEnd();
	}

	public
	void registerOutParameter(String parametername, int sqltype)
							throws SQLException {
		drv.debugFunction(this);
		registerOutParameter(parametername,sqltype,0);
		drv.debugEnd();
	}

	public
	void registerOutParameter(String parametername, int sqltype,
							int scale)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parametername);
		drv.debugPrintln("sql type: "+sqltype);
		drv.debugPrintln("scale: "+scale);

		BindType	bindtype;
		boolean		signed=false;
		boolean		binary=false;
		boolean		lob=false;
		boolean		ascii=false;
		String		classname;
		String		typename;

		switch (sqltype) {
			case Types.ARRAY:
				bindtype=BindType.Array;
				classname="java.sql.Array";
				typename="ARRAY";
				break;
			case Types.BIGINT:
				bindtype=BindType.Long;
				signed=true;
				classname="java.lang.Long";
				typename="BIGINT";
				break;
			case Types.BINARY:
			case Types.VARBINARY:
			case Types.LONGVARBINARY:
				bindtype=BindType.Bytes;
				binary=true;
				classname="[B";
				typename="VARBINARY";
				break;
			case Types.BIT:
			case Types.BOOLEAN:
				bindtype=BindType.Boolean;
				classname="java.lang.Boolean";
				typename="BOOLEAN";
				break;
			case Types.BLOB:
				bindtype=BindType.Blob;
				binary=true;
				lob=true;
				classname="java.sql.Blob";
				typename="BLOB";
				break;
			case Types.CHAR:
			case Types.VARCHAR:
			case Types.LONGVARCHAR:
				bindtype=BindType.String;
				ascii=true;
				classname="java.lang.String";
				typename="VARCHAR";
				break;
			case Types.CLOB:
				bindtype=BindType.Clob;
				lob=true;
				ascii=true;
				classname="java.sql.Clob";
				typename="CLOB";
				break;
			case Types.DATE:
				bindtype=BindType.Date;
				classname="java.sql.Date";
				typename="DATE";
				break;
			case Types.DECIMAL:
			case Types.NUMERIC:
				bindtype=BindType.BigDecimal;
				signed=true;
				classname="java.math.BigDecimal";
				typename="DECIMAL";
				break;
			case Types.DOUBLE:
				bindtype=BindType.Double;
				signed=true;
				classname="java.lang.Double";
				typename="DOUBLE";
				break;
			case Types.FLOAT:
			case Types.REAL:
				bindtype=BindType.Float;
				signed=true;
				classname="java.lang.Float";
				typename="FLOAT";
				break;
			case Types.INTEGER:
				bindtype=BindType.Int;
				signed=true;
				classname="java.lang.Integer";
				typename="INTEGER";
				break;
			case Types.NCHAR:
			case Types.NVARCHAR:
			case Types.LONGNVARCHAR:
				bindtype=BindType.NString;
				classname="java.lang.String";
				typename="NVARCHAR";
				break;
			case Types.NCLOB:
				bindtype=BindType.NClob;
				lob=true;
				classname="java.sql.NClob";
				typename="NCLOB";
				break;
			case Types.NULL:
				bindtype=BindType.Null;
				classname="java.lang.Object";
				typename="NULL";
				break;
			case Types.REF:
				bindtype=BindType.Ref;
				classname="java.sql.Ref";
				typename="REF";
				break;
			case Types.ROWID:
				bindtype=BindType.RowId;
				classname="java.sql.RowId";
				typename="ROWID";
				break;
			case Types.SMALLINT:
				bindtype=BindType.Short;
				signed=true;
				classname="java.lang.Short";
				typename="SMALLINT";
				break;
			case Types.SQLXML:
				bindtype=BindType.SQLXML;
				classname="java.sql.SQLXML";
				typename="SQLXML";
				break;
			case Types.TIME:
				bindtype=BindType.Time;
				classname="java.sql.Time";
				typename="TIME";
				break;
			case Types.TIMESTAMP:
				bindtype=BindType.Timestamp;
				classname="java.sql.Timestamp";
				typename="TIMESTAMP";
				break;
			case Types.TINYINT:
				bindtype=BindType.Byte;
				signed=true;
				classname="java.lang.Byte";
				typename="TINYINT";
				break;
			default:
				bindtype=BindType.String;
				ascii=true;
				classname="java.lang.String";
				typename="VARCHAR";
				break;
		}

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName(classname);
		param.setMode(ParameterMetaData.parameterModeOut);
		param.setType(sqltype);
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

		parameters.put(parametername,param);

		drv.debugEnd();
	}

	public
	void registerOutParameter(String parametername, int sqltype,
							String typename)
							throws SQLException {
		drv.debugFunction(this);
		registerOutParameter(parametername,sqltype,0);
		parameters.get(parametername).setTypeName(typename);
		drv.debugEnd();
	}

	public
	void setAsciiStream(String parametername, InputStream x)
							throws SQLException {
		drv.debugFunction(this);
		super.setAsciiStream(parametername,x);
		drv.debugEnd();
	}

	public
	void setAsciiStream(String parametername, InputStream x,
							int length)
							throws SQLException {
		drv.debugFunction(this);
		super.setAsciiStream(parametername,x,length);
		drv.debugEnd();
	}

	public
	void setAsciiStream(String parametername, InputStream x,
							long length)
							throws SQLException {
		drv.debugFunction(this);
		super.setAsciiStream(parametername,x,length);
		drv.debugEnd();
	}

	public
	void setBigDecimal(String parametername, BigDecimal x)
							throws SQLException {
		drv.debugFunction(this);
		super.setBigDecimal(parametername,x);
		drv.debugEnd();
	}

	public
	void setBinaryStream(String parametername, InputStream x)
							throws SQLException {
		drv.debugFunction(this);
		super.setBinaryStream(parametername,x);
		drv.debugEnd();
	}

	public
	void setBinaryStream(String parametername, InputStream x,
							int length)
							throws SQLException {
		drv.debugFunction(this);
		super.setBinaryStream(parametername,x,length);
		drv.debugEnd();
	}

	public
	void setBinaryStream(String parametername, InputStream x,
							long length)
							throws SQLException {
		drv.debugFunction(this);
		super.setBinaryStream(parametername,x,length);
		drv.debugEnd();
	}

	public
	void setBlob(String parametername, Blob x) throws SQLException {
		drv.debugFunction(this);
		super.setBlob(parametername,x);
		drv.debugEnd();
	}

	public
	void setBlob(String parametername, InputStream inputstream)
							throws SQLException {
		drv.debugFunction(this);
		super.setBlob(parametername,inputstream);
		drv.debugEnd();
	}

	public
	void setBlob(String parametername, InputStream inputstream,
						long length)
						throws SQLException {
		drv.debugFunction(this);
		super.setBlob(parametername,inputstream,length);
		drv.debugEnd();
	}

	public
	void setBoolean(String parametername, boolean x) throws SQLException {
		drv.debugFunction(this);
		super.setBoolean(parametername,x);
		drv.debugEnd();
	}

	public
	void setByte(String parametername, byte x) throws SQLException {
		drv.debugFunction(this);
		super.setByte(parametername,x);
		drv.debugEnd();
	}

	public
	void setBytes(String parametername, byte[] x) throws SQLException {
		drv.debugFunction(this);
		super.setBytes(parametername,x);
		drv.debugEnd();
	}

	public
	void setCharacterStream(String parametername, Reader reader)
							throws SQLException {
		drv.debugFunction(this);
		super.setCharacterStream(parametername,reader);
		drv.debugEnd();
	}

	public
	void setCharacterStream(String parametername, Reader reader,
							int length)
							throws SQLException {
		drv.debugFunction(this);
		super.setCharacterStream(parametername,reader,length);
		drv.debugEnd();
	}

	public
	void setCharacterStream(String parametername, Reader reader,
							long length)
							throws SQLException {
		drv.debugFunction(this);
		super.setCharacterStream(parametername,reader,length);
		drv.debugEnd();
	}

	public
	void setClob(String parametername, Clob x) throws SQLException {
		drv.debugFunction(this);
		super.setClob(parametername,x);
		drv.debugEnd();
	}

	public
	void setClob(String parametername, Reader reader) throws SQLException {
		drv.debugFunction(this);
		super.setClob(parametername,reader);
		drv.debugEnd();
	}

	public
	void setClob(String parametername, Reader reader, long length)
							throws SQLException {
		drv.debugFunction(this);
		super.setClob(parametername,reader,length);
		drv.debugEnd();
	}

	public
	void setDate(String parametername, Date x) throws SQLException {
		drv.debugFunction(this);
		super.setDate(parametername,x);
		drv.debugEnd();
	}

	public
	void setDate(String parametername, Date x, Calendar cal)
							throws SQLException {
		drv.debugFunction(this);
		super.setDate(parametername,x,cal);
		drv.debugEnd();
	}

	public
	void setDouble(String parametername, double x) throws SQLException {
		drv.debugFunction(this);
		super.setDouble(parametername,x);
		drv.debugEnd();
	}

	public
	void setFloat(String parametername, float x) throws SQLException {
		drv.debugFunction(this);
		super.setFloat(parametername,x);
		drv.debugEnd();
	}

	public
	void setInt(String parametername, int x) throws SQLException {
		drv.debugFunction(this);
		super.setInt(parametername,x);
		drv.debugEnd();
	}

	public
	void setLong(String parametername, long x) throws SQLException {
		drv.debugFunction(this);
		super.setLong(parametername,x);
		drv.debugEnd();
	}

	public
	void setNCharacterStream(String parametername, Reader value)
							throws SQLException {
		drv.debugFunction(this);
		super.setNCharacterStream(parametername,value);
		drv.debugEnd();
	}

	public
	void setNCharacterStream(String parametername, Reader value,
							long length)
							throws SQLException {
		drv.debugFunction(this);
		super.setNCharacterStream(parametername,value,length);
		drv.debugEnd();
	}

	public
	void setNClob(String parametername, NClob value) throws SQLException {
		drv.debugFunction(this);
		super.setNClob(parametername,value);
		drv.debugEnd();
	}

	public
	void setNClob(String parametername, Reader reader) throws SQLException {
		drv.debugFunction(this);
		super.setNClob(parametername,reader);
		drv.debugEnd();
	}

	public
	void setNClob(String parametername, Reader reader, long length)
							throws SQLException {
		drv.debugFunction(this);
		super.setNClob(parametername,reader,length);
		drv.debugEnd();
	}

	public
	void setNString(String parametername, String value)
							throws SQLException {
		drv.debugFunction(this);
		super.setNString(parametername,value);
		drv.debugEnd();
	}

	public
	void setNull(String parametername, int sqltype) throws SQLException {
		drv.debugFunction(this);
		super.setNull(parametername,sqltype);
		drv.debugEnd();
	}

	public
	void setNull(String parametername, int sqltype, String typename)
							throws SQLException {
		drv.debugFunction(this);
		super.setNull(parametername,sqltype,typename);
		drv.debugEnd();
	}

	public
	void setObject(String parametername, Object x) throws SQLException {
		drv.debugFunction(this);
		super.setObject(parametername,x);
		drv.debugEnd();
	}

	public
	void setObject(String parametername, Object x, int targetsqltype)
							throws SQLException {
		drv.debugFunction(this);
		super.setObject(parametername,x,targetsqltype);
		drv.debugEnd();
	}

	public
	void setObject(String parametername, Object x, int targetsqltype,
							int scale)
							throws SQLException {
		drv.debugFunction(this);
		super.setObject(parametername,x,targetsqltype,scale);
		drv.debugEnd();
	}

	public
	void setRowId(String parametername, RowId x) throws SQLException {
		drv.debugFunction(this);
		super.setRowId(parametername,x);
		drv.debugEnd();
	}

	public
	void setShort(String parametername, short x) throws SQLException {
		drv.debugFunction(this);
		super.setShort(parametername,x);
		drv.debugEnd();
	}

	public
	void setSQLXML(String parametername, SQLXML xmlobject)
							throws SQLException {
		drv.debugFunction(this);
		super.setSQLXML(parametername,xmlobject);
		drv.debugEnd();
	}

	public
	void setString(String parametername, String x) throws SQLException {
		drv.debugFunction(this);
		super.setString(parametername,x);
		drv.debugEnd();
	}

	public
	void setTime(String parametername, Time x) throws SQLException {
		drv.debugFunction(this);
		super.setTime(parametername,x);
		drv.debugEnd();
	}

	public
	void setTime(String parametername, Time x, Calendar cal)
							throws SQLException {
		drv.debugFunction(this);
		super.setTime(parametername,x,cal);
		drv.debugEnd();
	}

	public
	void setTimestamp(String parametername, Timestamp x)
							throws SQLException {
		drv.debugFunction(this);
		super.setTimestamp(parametername,x);
		drv.debugEnd();
	}

	public
	void setTimestamp(String parametername, Timestamp x,
						Calendar cal)
						throws SQLException {
		drv.debugFunction(this);
		super.setTimestamp(parametername,x,cal);
		drv.debugEnd();
	}

	public
	void setURL(String parametername, URL val) throws SQLException {
		drv.debugFunction(this);
		super.setURL(parametername,val);
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
