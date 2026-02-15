package com.firstworks.sql;

import java.sql.*;

import java.io.InputStream;
import java.io.Reader;
import java.io.InputStreamReader;
import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.math.BigDecimal;
import java.util.Calendar;
import java.net.URL;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.GregorianCalendar;

import com.firstworks.sqlrelay.*;

import com.firstworks.sql.SQLRelayParameter.*;

public class SQLRelayPreparedStatement
		extends SQLRelayStatement
		implements PreparedStatement {

	private ArrayList<HashMap<String,SQLRelayParameter>>	batch;
	protected HashMap<String,SQLRelayParameter>		parameters;


	public
	SQLRelayPreparedStatement(SQLRelayDriver driver) {
		super(driver);
		drv.debugFunction(this);
		batch=new ArrayList<HashMap<String,SQLRelayParameter>>();
		parameters=new HashMap<String,SQLRelayParameter>();
		drv.debugEnd();
	}

	public
	void addBatch() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		batch.add(parameters);
		parameters=new HashMap<String,SQLRelayParameter>();
		drv.debugEnd();
	}

	public
	void clearParameters() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		sqlrcur.clearBinds();
		batch.clear();
		parameters.clear();
		drv.debugEnd();
	}

	public
	boolean execute() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();

		// initialize results
		resultset=null;
		updatecount=-1;

		// bind variables
		if (batch.size()==0) {
			bind(parameters);
		} else {
			bind(batch.get(0));
		}

		// execute the query
		boolean	result=false;
		synchronized (networklock) {
			result=sqlrcur.executeQuery();
		}

		drv.debugPrintln("result: "+result);

		// handle results
		if (result) {

			updatecount=(int)sqlrcur.affectedRows();

			drv.debugPrintln("update count: "+updatecount);
			drv.debugPrintln("column count: "+sqlrcur.colCount());

			if (sqlrcur.colCount()>0) {
				resultset=new SQLRelayResultSet(drv);
				resultset.setNetworkLock(networklock);
				resultset.setStatement(this);
				resultset.setConnection(conn);
				resultset.setSQLRCursor(sqlrcur);
			}

			getOutputBindValues();
		} else {
			throwErrorMessageException();
		}

		drv.debugEnd();
		return resultset!=null;
	}

	public
	int[] executeBatch() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();

		// debug
		drv.debugPrintln("batch size: "+batch.size());

		// execute each query in the batch
		int[]	results=new int[batch.size()];
		int	count=0;

		for (HashMap<String,SQLRelayParameter> params: batch) {
			bind(params);
			results[count++]=executeUpdate();
		}

		drv.debugEnd();
		return results;
	}

	public
	ResultSet executeQuery() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();

		// initialize results
		resultset=null;
		updatecount=-1;

		// bind variables
		if (batch.size()==0) {
			bind(parameters);
		} else {
			bind(batch.get(0));
		}

		// execute the query
		boolean	success=false;
		synchronized (networklock) {
			success=sqlrcur.executeQuery();
		}

		// handle results
		if (success) {

			updatecount=(int)sqlrcur.affectedRows();

			drv.debugPrintln("update count: "+updatecount);
			drv.debugPrintln("column count: "+sqlrcur.colCount());

			resultset=new SQLRelayResultSet(drv);
			resultset.setNetworkLock(networklock);
			resultset.setStatement(this);
			resultset.setConnection(conn);
			resultset.setSQLRCursor(sqlrcur);

			getOutputBindValues();
		} else {
			throwErrorMessageException();
		}

		drv.debugEnd();
		return resultset;
	}

	public
	int executeUpdate() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();

		// initialize results
		resultset=null;
		updatecount=-1;

		// bind variables
		if (batch.size()==0) {
			bind(parameters);
		} else {
			bind(batch.get(0));
		}

		// execute the query
		boolean	success=false;
		synchronized (networklock) {
			success=sqlrcur.executeQuery();
		}

		// handle results
		if (success) {
			updatecount=(int)sqlrcur.affectedRows();

			drv.debugPrintln("update count: "+updatecount);

			getOutputBindValues();
		} else {
			throwErrorMessageException();
		}

		drv.debugEnd();
		return updatecount;
	}

	private
	void bind(HashMap<String,SQLRelayParameter> params)
							throws SQLException {
		drv.debugFunction(this);

		sqlrcur.clearBinds();

		if (params==null) {
			drv.debugPrintln("parameters: null");
			drv.debugEnd();
			return;
		}

		drv.debugPrintln("parameter count: "+params.size());

		for (Map.Entry<String,SQLRelayParameter> entry:
							params.entrySet()) {

			String	key=entry.getKey();
			drv.debugStart("parameter: "+key);

			SQLRelayParameter	value=entry.getValue();
			BindType		bindtype=value.getBindType();

			switch (value.getMode()) {
				case ParameterMetaData.parameterModeIn:
					inputBind(key,value,bindtype);
					break;
				case ParameterMetaData.parameterModeInOut:
					// FIXME: implement this...
					break;
				case ParameterMetaData.parameterModeOut:
					defineOutputBind(key,value,bindtype);
					break;
				default:
					break;
			}

			drv.debugEnd();
		}
		drv.debugEnd();
	}

	private
	void inputBind(String key, SQLRelayParameter value,
					BindType bindtype) throws SQLException {
		drv.debugFunction(this);

		Object	o=value.getObject();
		if (o==null) {
			drv.debugPrintln("bind type: Null (value is null)");
			bindtype=BindType.Null;
		}

		switch (bindtype) {
			case Array:
				// FIXME: support this, somehow
				break;
			case AsciiStream:
				sqlrcur.inputBind(key,
					asciiStreamToString((InputStream)o));
				break;
			case AsciiStreamWithIntLength:
			case AsciiStreamWithLongLength:
				sqlrcur.inputBind(key,
						asciiStreamToString(
							(InputStream)o,
							value.getLength()));
				break;
			case BigDecimal:
				sqlrcur.inputBind(key,
					((BigDecimal)o).doubleValue(),
					((BigDecimal)o).precision(),
					((BigDecimal)o).scale());
				break;
			case BinaryStream:
				{
				byte[]	bytes=binaryStreamToBytes(
							(InputStream)o,-1);
				sqlrcur.inputBindBlob(key,bytes,bytes.length);
				}
				break;
			case BinaryStreamWithIntLength:
			case BinaryStreamWithLongLength:
				{
				byte[]	bytes=binaryStreamToBytes(
							(InputStream)o,
							value.getLength());
				sqlrcur.inputBindBlob(key,bytes,bytes.length);
				}
				break;
			case Blob:
				{
				byte[]	bytes=blobToBytes((Blob)o);
				sqlrcur.inputBindBlob(key,bytes,bytes.length);
				}
				break;
			case BlobInputStream:
				{
				byte[]	bytes=binaryStreamToBytes(
							(InputStream)o,-1);
				sqlrcur.inputBindBlob(key,bytes,bytes.length);
				}
				break;
			case BlobInputStreamWithLongLength:
				{
				byte[]	bytes=binaryStreamToBytes(
							(InputStream)o,
							value.getLength());
				sqlrcur.inputBindBlob(key,bytes,bytes.length);
				}
				break;
			case Boolean:
				{
				long	val=(((Boolean)o).
						booleanValue()==true)?1:0;
				sqlrcur.inputBind(key,val);
				}
				break;
			case Byte:
				{
				long	val=((Byte)o).byteValue();
				sqlrcur.inputBind(key,val);
				}
				break;
			case Bytes:
				{
				Byte[]	v=(Byte[])o;
				byte[]	val=new byte[v.length];
				for (int i=0; i<v.length; i++) {
					val[i]=v[i].byteValue();
				}
				sqlrcur.inputBindBlob(key,val,val.length);
				}
				break;
			case CharacterStream:
				sqlrcur.inputBind(
					key,readerToString((Reader)o,-1));
				break;
			case CharacterStreamWithIntLength:
			case CharacterStreamWithLongLength:
				sqlrcur.inputBind(key,
					readerToString((Reader)o,
							value.getLength()));
				break;
			case Clob:
				{
				String	string=clobToString((Clob)o);
				sqlrcur.inputBindClob(key,
						string,string.length());
				}
				break;
			case ClobReader:
				{
				String	string=readerToString((Reader)o,-1);
				sqlrcur.inputBindClob(key,
						string,string.length());
				}
				break;
			case ClobReaderWithLength:
				{
				String	string=readerToString((Reader)o,
							value.getLength());
				sqlrcur.inputBindClob(key,
						string,string.length());
				}
				break;
			case Date:
				{
				Calendar	cal=Calendar.getInstance();
				cal.setTime((Date)o);
				sqlrcur.inputBind(key,
					(short)cal.get(Calendar.YEAR),
					(short)(cal.get(Calendar.MONTH)+1),
					(short)cal.get(Calendar.DAY_OF_MONTH),
					(short)0,
					(short)0,
					(short)0,
					(short)0,
					null,false);
				}
				break;
			case DateWithCalendar:
				{
				Calendar	cal=value.getCalendar();
				cal.setTime((Date)o);
				sqlrcur.inputBind(key,
					(short)cal.get(Calendar.YEAR),
					(short)(cal.get(Calendar.MONTH)+1),
					(short)cal.get(Calendar.DAY_OF_MONTH),
					(short)0,
					(short)0,
					(short)0,
					(short)0,
					null,false);
				}
				break;
			case Double:
				sqlrcur.inputBind(key,
					((Double)o).doubleValue(),15,0);
				break;
			case Float:
				sqlrcur.inputBind(key,
					((Float)o).floatValue(),7,0);
				break;
			case Int:
				sqlrcur.inputBind(key,
					((Integer)o).intValue());
				break;
			case Long:
				sqlrcur.inputBind(key,
					((Long)o).longValue());
				break;
			case NCharStream:
				sqlrcur.inputBind(key,
					readerToString((Reader)o,-1));
				break;
			case NCharStreamWithLength:
				sqlrcur.inputBind(key,
					readerToString((Reader)o,
							value.getLength()));
				break;
			case NClob:
				{
				String	string=nClobToUnicodeString((NClob)o);
				sqlrcur.inputBindClob(key,
						string,string.length());
				}
				break;
			case NClobReader:
				{
				String	string=readerToString((Reader)o,-1);
				sqlrcur.inputBindClob(key,
						string,string.length());
				}
				break;
			case NClobReaderWithLength:
				{
				String	string=readerToString(
						(Reader)o,value.getLength());
				sqlrcur.inputBindClob(key,
						string,string.length());
				}
				break;
			case NString:
				sqlrcur.inputBind(key,(String)o);
				break;
			case Null:
			case NullWithTypeName:
				sqlrcur.inputBind(key,null);
				break;
			case Object:
				// FIXME: support this
				break;
			case ObjectWithTargetType:
				// FIXME: support this
				break;
			case ObjectWithTargetTypeAndScaleOrLength:
				// FIXME: support this
				break;
			case Ref:
				// FIXME: support this
				break;
			case RowId:
				// FIXME: support this
				break;
			case Short:
				sqlrcur.inputBind(key,((Short)o).shortValue());
				break;
			case String:
				sqlrcur.inputBind(key,(String)o);
				break;
			case SQLXML:
				// FIXME: support this
				break;
			case Time:
				{
				Calendar	cal=Calendar.getInstance();
				cal.setTime((Time)o);
				sqlrcur.inputBind(key,
					(short)0,
					(short)0,
					(short)0,
					(short)cal.get(Calendar.HOUR_OF_DAY),
					(short)cal.get(Calendar.MINUTE),
					(short)cal.get(Calendar.SECOND),
					(short)0,
					null,false);
				}
				break;
			case TimeWithCalendar:
				{
				Calendar	cal=value.getCalendar();
				cal.setTime((Time)o);
				sqlrcur.inputBind(key,
					(short)0,
					(short)0,
					(short)0,
					(short)cal.get(Calendar.HOUR_OF_DAY),
					(short)cal.get(Calendar.MINUTE),
					(short)cal.get(Calendar.SECOND),
					(short)0,
					null,false);
				}
				break;
			case Timestamp:
				{
				Calendar	cal=Calendar.getInstance();
				cal.setTime((Timestamp)o);
				sqlrcur.inputBind(key,
					(short)cal.get(Calendar.YEAR),
					(short)(cal.get(Calendar.MONTH)+1),
					(short)cal.get(Calendar.DAY_OF_MONTH),
					(short)cal.get(Calendar.HOUR_OF_DAY),
					(short)cal.get(Calendar.MINUTE),
					(short)cal.get(Calendar.SECOND),
					(short)(((Timestamp)o).getNanos()/1000),
					null,false);
				}
				break;
			case TimestampWithCalendar:
				{
				Calendar	cal=value.getCalendar();
				cal.setTime((Timestamp)o);
				sqlrcur.inputBind(key,
					(short)cal.get(Calendar.YEAR),
					(short)(cal.get(Calendar.MONTH)+1),
					(short)cal.get(Calendar.DAY_OF_MONTH),
					(short)cal.get(Calendar.HOUR_OF_DAY),
					(short)cal.get(Calendar.MINUTE),
					(short)cal.get(Calendar.SECOND),
					(short)(((Timestamp)o).getNanos()/1000),
					null,false);
				}
				break;
			case UnicodeStream:
				sqlrcur.inputBind(key,
					unicodeStreamToString(
						(InputStream)o,
						value.getLength()));
				break;
			case URL:
				sqlrcur.inputBind(key,
					((URL)o).toString());
				break;
		}

		drv.debugEnd();
	}

	private
	void defineOutputBind(String key, SQLRelayParameter value,
					BindType bindtype) throws SQLException {
		drv.debugFunction(this);
		switch (bindtype) {
			case Array:
				// FIXME: support this
				break;
			case AsciiStream:
				// FIXME: make this length configurable
				sqlrcur.defineOutputBindString(key,1024);
				break;
			case AsciiStreamWithIntLength:
			case AsciiStreamWithLongLength:
				// FIXME: lossy conversion to int
				sqlrcur.defineOutputBindString(key,
						(int)value.getLength());
				break;
			case BigDecimal:
				sqlrcur.defineOutputBindDouble(key);
				break;
			case BinaryStream:
			case BinaryStreamWithIntLength:
			case BinaryStreamWithLongLength:
				sqlrcur.defineOutputBindBlob(key);
				break;
			case Blob:
			case BlobInputStream:
			case BlobInputStreamWithLongLength:
				sqlrcur.defineOutputBindBlob(key);
				break;
			case Boolean:
				sqlrcur.defineOutputBindInteger(key);
				break;
			case Byte:
				sqlrcur.defineOutputBindInteger(key);
				break;
			case Bytes:
				sqlrcur.defineOutputBindBlob(key);
				break;
			case CharacterStream:
				// FIXME: make this length configurable
				sqlrcur.defineOutputBindString(key,1024);
				break;
			case CharacterStreamWithIntLength:
			case CharacterStreamWithLongLength:
				// FIXME: lossy conversion to int
				sqlrcur.defineOutputBindString(key,
						(int)value.getLength());
				break;
			case Clob:
			case ClobReader:
			case ClobReaderWithLength:
				sqlrcur.defineOutputBindClob(key);
				break;
			case Date:
			case DateWithCalendar:
				sqlrcur.defineOutputBindDate(key);
				break;
			case Double:
				sqlrcur.defineOutputBindDouble(key);
				break;
			case Float:
				sqlrcur.defineOutputBindDouble(key);
				break;
			case Int:
				sqlrcur.defineOutputBindInteger(key);
				break;
			case Long:
				sqlrcur.defineOutputBindInteger(key);
				break;
			case NCharStream:
				// FIXME: make this length configurable
				sqlrcur.defineOutputBindString(key,1024);
				break;
			case NCharStreamWithLength:
				// FIXME: lossy conversion to int
				sqlrcur.defineOutputBindString(key,
						(int)value.getLength());
				break;
			case NClob:
			case NClobReader:
			case NClobReaderWithLength:
				sqlrcur.defineOutputBindClob(key);
				break;
			case NString:
				// FIXME: make this length configurable
				sqlrcur.defineOutputBindString(key,1024);
				break;
			case Null:
			case NullWithTypeName:
				break;
			case Object:
				// FIXME: support this
				break;
			case ObjectWithTargetType:
				// FIXME: support this
				break;
			case ObjectWithTargetTypeAndScaleOrLength:
				// FIXME: support this
				break;
			case Ref:
				// FIXME: support this
				break;
			case RowId:
				// FIXME: support this
				break;
			case Short:
				sqlrcur.defineOutputBindInteger(key);
				break;
			case String:
				// FIXME: make this length configurable
				sqlrcur.defineOutputBindString(key,1024);
				break;
			case SQLXML:
				// FIXME: support this
				break;
			case Time:
			case TimeWithCalendar:
				sqlrcur.defineOutputBindDate(key);
				break;
			case Timestamp:
			case TimestampWithCalendar:
				sqlrcur.defineOutputBindDate(key);
				break;
			case UnicodeStream:
				// FIXME: make this length configurable
				sqlrcur.defineOutputBindString(key,1024);
				break;
			case URL:
				// FIXME: make this length configurable
				sqlrcur.defineOutputBindString(key,1024);
				break;
		}
		drv.debugEnd();
	}

	private
	void getOutputBindValues() {
		drv.debugFunction(this);

		if (parameters==null) {
			drv.debugPrintln("parameters; null");
			drv.debugEnd();
			return;
		}

		drv.debugPrintln("parameters count: "+parameters.size());

		for (Map.Entry<String,SQLRelayParameter> entry:
							parameters.entrySet()) {

			String			key=entry.getKey();
			SQLRelayParameter	value=entry.getValue();
			int			mode=value.getMode();

			drv.debugPrintln("key: "+key);
			drv.debugPrintln("mode: "+mode);

			if (mode!=ParameterMetaData.parameterModeOut &&
				mode!=ParameterMetaData.parameterModeInOut) {
				continue;
			}

			getOutputBindValue(key,value);
		}

		drv.debugEnd();
	}

	private
	void getOutputBindValue(String key, SQLRelayParameter value) {
		drv.debugFunction(this);
		switch (value.getBindType()) {
			case Array:
				// FIXME: support this
				break;
			case AsciiStream:
			case AsciiStreamWithIntLength:
			case AsciiStreamWithLongLength:
			case CharacterStream:
			case CharacterStreamWithIntLength:
			case CharacterStreamWithLongLength:
			case NCharStream:
			case NCharStreamWithLength:
			case NString:
			case String:
			case UnicodeStream:
			case URL:
				value.setObject(sqlrcur.
					getOutputBindString(key));
				break;
			case BigDecimal:
				value.setObject(
					BigDecimal.valueOf(sqlrcur.
					getOutputBindDouble(key)));
				break;
			case BinaryStream:
			case BinaryStreamWithIntLength:
			case BinaryStreamWithLongLength:
			case Blob:
			case BlobInputStream:
			case BlobInputStreamWithLongLength:
			case Bytes:
				value.setObject(sqlrcur.
					getOutputBindBlob(key));
				break;
			case Boolean:
				value.setObject(
					Boolean.valueOf(sqlrcur.
					getOutputBindInteger(
							key)!=0));
				break;
			case Byte:
				value.setObject(
					Byte.valueOf((byte)sqlrcur.
					getOutputBindInteger(key)));
				break;
			case Clob:
			case ClobReader:
			case ClobReaderWithLength:
				value.setObject(sqlrcur.
					getOutputBindClob(key));
				break;
			case Date:
			case DateWithCalendar:
				value.setObject(new Date(
					sqlrcur.
					getOutputBindDateYear(key)-1900,
					sqlrcur.
					getOutputBindDateMonth(key)-1,
					sqlrcur.
					getOutputBindDateDay(key)));
				break;
			case Double:
				value.setObject(
					Double.valueOf(sqlrcur.
					getOutputBindDouble(key)));
				break;
			case Float:
				value.setObject(
					Float.valueOf((float)sqlrcur.
					getOutputBindDouble(key)));
				break;
			case Int:
				value.setObject(
					Integer.valueOf((int)sqlrcur.
					getOutputBindInteger(key)));
				break;
			case Long:
				value.setObject(
					Long.valueOf(sqlrcur.
					getOutputBindInteger(key)));
				break;
			case NClob:
			case NClobReader:
			case NClobReaderWithLength:
				value.setObject(sqlrcur.
					getOutputBindClob(key));
				break;
			case Null:
			case NullWithTypeName:
				break;
			case Object:
			case ObjectWithTargetType:
			case ObjectWithTargetTypeAndScaleOrLength:
				// FIXME: support this
				break;
			case Ref:
				// FIXME: support this
				break;
			case RowId:
				// FIXME: support this
				break;
			case Short:
				value.setObject(
					Short.valueOf((short)sqlrcur.
					getOutputBindInteger(key)));
				break;
			case SQLXML:
				// FIXME: support this
				break;
			case Time:
			case TimeWithCalendar:
				value.setObject(new Time(
					sqlrcur.
					getOutputBindDateHour(key),
					sqlrcur.
					getOutputBindDateMinute(key),
					sqlrcur.
					getOutputBindDateSecond(key)));
				break;
			case Timestamp:
			case TimestampWithCalendar:
				value.setObject(new Timestamp(
					sqlrcur.
					getOutputBindDateYear(key)-1900,
					sqlrcur.
					getOutputBindDateMonth(key)-1,
					sqlrcur.
					getOutputBindDateDay(key),
					sqlrcur.
					getOutputBindDateHour(key),
					sqlrcur.
					getOutputBindDateMinute(key),
					sqlrcur.
					getOutputBindDateSecond(key),
					sqlrcur.
					getOutputBindDateMicrosecond(
							key)*1000));
				break;
		}
		drv.debugEnd();
	}

	public
	ResultSetMetaData getMetaData() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		ResultSetMetaData	rsmd=
				(resultset!=null)?resultset.getMetaData():null;
		drv.debugEnd();
		return rsmd;
	}

	public
	ParameterMetaData getParameterMetaData() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameterMetaData	pmd=
				new SQLRelayParameterMetaData(drv);
		if (batch.size()==0) {
			pmd.setParameters(parameters);
		} else {
			pmd.setParameters(batch.get(0));
		}
		drv.debugEnd();
		return pmd;
	}

	public
	void setArray(int parameterIndex, Array x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setArray(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setArray(String parameterName, Array x) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		conn.throwFeatureNotSupportedException();

		drv.debugEnd();
	}

	public
	void setAsciiStream(int parameterIndex, InputStream x)
							throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setAsciiStream(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setAsciiStream(String parameterName, InputStream x)
							throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.lang.String");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.VARCHAR);
		param.setTypeName("VARCHAR");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(x);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(true);
		param.setCalendar(null);
		param.setBindType(BindType.AsciiStream);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setAsciiStream(int parameterIndex, InputStream x,
					int length) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setAsciiStream(String.valueOf(parameterIndex),x,length);
		drv.debugEnd();
	}

	public
	void setAsciiStream(String parameterName, InputStream x,
					int length) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.lang.String");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.VARCHAR);
		param.setTypeName("VARCHAR");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(x);
		param.setLength(length);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(true);
		param.setCalendar(null);
		param.setBindType(BindType.AsciiStreamWithIntLength);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setAsciiStream(int parameterIndex, InputStream x,
					long length) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setAsciiStream(String.valueOf(parameterIndex),x,length);
		drv.debugEnd();
	}

	public
	void setAsciiStream(String parameterName, InputStream x,
					long length) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.lang.String");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.VARCHAR);
		param.setTypeName("VARCHAR");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(x);
		param.setLength(length);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(true);
		param.setCalendar(null);
		param.setBindType(BindType.AsciiStreamWithLongLength);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setBigDecimal(int parameterIndex, BigDecimal x)
						throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setBigDecimal(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setBigDecimal(String parameterName, BigDecimal x)
						throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.math.BigDecimal");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.DECIMAL);
		param.setTypeName("DECIMAL");
		param.setPrecision(x.precision());
		param.setScale(x.scale());
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(true);
		param.setObject(x);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.BigDecimal);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setBinaryStream(int parameterIndex, InputStream x)
						throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setBinaryStream(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setBinaryStream(String parameterName, InputStream x)
						throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("[B");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.VARBINARY);
		param.setTypeName("VARBINARY");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(x);
		param.setLength(-1);
		param.setIsBinary(true);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.BinaryStream);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setBinaryStream(int parameterIndex, InputStream x,
					int length) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setBinaryStream(String.valueOf(parameterIndex),x,length);
		drv.debugEnd();
	}

	public
	void setBinaryStream(String parameterName, InputStream x,
					int length) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("[B");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.VARBINARY);
		param.setTypeName("VARBINARY");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(x);
		param.setLength(length);
		param.setIsBinary(true);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.BinaryStreamWithIntLength);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setBinaryStream(int parameterIndex, InputStream x,
					long length) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setBinaryStream(String.valueOf(parameterIndex),x,length);
		drv.debugEnd();
	}

	public
	void setBinaryStream(String parameterName, InputStream x,
					long length) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("[B");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.VARBINARY);
		param.setTypeName("VARBINARY");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(x);
		param.setLength(length);
		param.setIsBinary(true);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.BinaryStreamWithLongLength);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setBlob(int parameterIndex, Blob x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setBlob(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setBlob(String parameterName, Blob x) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.sql.Blob");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.BLOB);
		param.setTypeName("BLOB");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(x);
		param.setLength(-1);
		param.setIsBinary(true);
		param.setIsLob(true);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.Blob);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setBlob(int parameterIndex, InputStream inputStream)
						throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setBlob(String.valueOf(parameterIndex),inputStream);
		drv.debugEnd();
	}

	public
	void setBlob(String parameterName, InputStream inputStream)
						throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.sql.Blob");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.BLOB);
		param.setTypeName("BLOB");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(inputStream);
		param.setLength(-1);
		param.setIsBinary(true);
		param.setIsLob(true);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.BlobInputStream);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setBlob(int parameterIndex, InputStream inputStream,
					long length) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setBlob(String.valueOf(parameterIndex),inputStream,length);
		drv.debugEnd();
	}

	public
	void setBlob(String parameterName, InputStream inputStream,
					long length) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.sql.Blob");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.BLOB);
		param.setTypeName("BLOB");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(inputStream);
		param.setLength(length);
		param.setIsBinary(true);
		param.setIsLob(true);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.BlobInputStreamWithLongLength);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setBoolean(int parameterIndex, boolean x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setBoolean(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setBoolean(String parameterName, boolean x) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);
		drv.debugPrintln("value: "+x);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.lang.Boolean");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.BOOLEAN);
		param.setTypeName("BOOLEAN");
		param.setPrecision(1);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(Boolean.valueOf(x));
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.Boolean);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setByte(int parameterIndex, byte x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setByte(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setByte(String parameterName, byte x) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);
		drv.debugPrintln("value: "+x);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.lang.Byte");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.TINYINT);
		param.setTypeName("TINYINT");
		param.setPrecision(3);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(Byte.valueOf(x));
		param.setLength(-1);
		param.setIsBinary(true);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.Byte);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setBytes(int parameterIndex, byte[] x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setBytes(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setBytes(String parameterName, byte[] x) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		Byte[]	bytes=new Byte[x.length];
		for (int i=0; i<x.length; i++) {
			bytes[i]=x[i];
		}
		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("[B");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.VARBINARY);
		param.setTypeName("VARBINARY");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(bytes);
		param.setLength(-1);
		param.setIsBinary(true);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.Bytes);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setCharacterStream(int parameterIndex, Reader reader)
						throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setCharacterStream(String.valueOf(parameterIndex),reader);
		drv.debugEnd();
	}

	public
	void setCharacterStream(String parameterName, Reader reader)
						throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.lang.String");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.VARCHAR);
		param.setTypeName("VARCHAR");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(reader);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.CharacterStream);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setCharacterStream(int parameterIndex, Reader reader,
					int length) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setCharacterStream(String.valueOf(parameterIndex),reader,length);
		drv.debugEnd();
	}

	public
	void setCharacterStream(String parameterName, Reader reader,
					int length) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.lang.String");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.VARCHAR);
		param.setTypeName("VARCHAR");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(reader);
		param.setLength(length);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.CharacterStreamWithIntLength);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setCharacterStream(int parameterIndex, Reader reader,
					long length) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setCharacterStream(String.valueOf(parameterIndex),reader,length);
		drv.debugEnd();
	}

	public
	void setCharacterStream(String parameterName, Reader reader,
					long length) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.lang.String");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.VARCHAR);
		param.setTypeName("VARCHAR");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(reader);
		param.setLength(length);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.CharacterStreamWithLongLength);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setClob(int parameterIndex, Clob x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setClob(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setClob(String parameterName, Clob x) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.sql.Clob");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.CLOB);
		param.setTypeName("CLOB");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(x);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(true);
		param.setIsAscii(true);
		param.setCalendar(null);
		param.setBindType(BindType.Clob);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setClob(int parameterIndex, Reader reader) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setClob(String.valueOf(parameterIndex),reader);
		drv.debugEnd();
	}

	public
	void setClob(String parameterName, Reader reader) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.sql.Clob");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.CLOB);
		param.setTypeName("CLOB");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(reader);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(true);
		param.setIsAscii(true);
		param.setCalendar(null);
		param.setBindType(BindType.ClobReader);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setClob(int parameterIndex, Reader reader,
					long length) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setClob(String.valueOf(parameterIndex),reader,length);
		drv.debugEnd();
	}

	public
	void setClob(String parameterName, Reader reader,
					long length) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.sql.Clob");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.CLOB);
		param.setTypeName("CLOB");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(reader);
		param.setLength(length);
		param.setIsBinary(false);
		param.setIsLob(true);
		param.setIsAscii(true);
		param.setCalendar(null);
		param.setBindType(BindType.ClobReaderWithLength);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setDate(int parameterIndex, Date x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setDate(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setDate(String parameterName, Date x) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.sql.Date");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.DATE);
		param.setTypeName("DATE");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(x);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.Date);

		parameters.put(parameterName,param);
		drv.debugEnd();
	}

	public
	void setDate(int parameterIndex, Date x, Calendar cal)
						throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setDate(String.valueOf(parameterIndex),x,cal);
		drv.debugEnd();
	}

	public
	void setDate(String parameterName, Date x, Calendar cal)
						throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.sql.Date");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.DATE);
		param.setTypeName("DATE");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(x);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(cal);
		param.setBindType(BindType.DateWithCalendar);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setDouble(int parameterIndex, double x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setDouble(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setDouble(String parameterName, double x) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);
		drv.debugPrintln("value: "+x);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.lang.Double");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.DOUBLE);
		param.setTypeName("DOUBLE");
		param.setPrecision(15);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(true);
		param.setObject(Double.valueOf(x));
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.Double);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setFloat(int parameterIndex, float x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setFloat(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setFloat(String parameterName, float x) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);
		drv.debugPrintln("value: "+x);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.lang.Float");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.FLOAT);
		param.setTypeName("FLOAT");
		param.setPrecision(7);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(true);
		param.setObject(Float.valueOf(x));
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.Float);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setInt(int parameterIndex, int x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setInt(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setInt(String parameterName, int x) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);
		drv.debugPrintln("value: "+x);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.lang.Integer");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.INTEGER);
		param.setTypeName("INTEGER");
		param.setPrecision(10);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(true);
		param.setObject(Integer.valueOf(x));
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.Int);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setLong(int parameterIndex, long x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setLong(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setLong(String parameterName, long x) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);
		drv.debugPrintln("value: "+x);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.lang.Long");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.BIGINT);
		param.setTypeName("BIGINT");
		param.setPrecision(19);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(true);
		param.setObject(Long.valueOf(x));
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.Long);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setNCharacterStream(int parameterIndex, Reader value)
						throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setNCharacterStream(String.valueOf(parameterIndex),value);
		drv.debugEnd();
	}

	public
	void setNCharacterStream(String parameterName, Reader value)
						throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.lang.String");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.NVARCHAR);
		param.setTypeName("NVARCHAR");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(value);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.NCharStream);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setNCharacterStream(int parameterIndex, Reader value,
					long length) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setNCharacterStream(String.valueOf(parameterIndex),value,length);
		drv.debugEnd();
	}

	public
	void setNCharacterStream(String parameterName, Reader value,
					long length) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.lang.String");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.NVARCHAR);
		param.setTypeName("NVARCHAR");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(value);
		param.setLength(length);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.NCharStreamWithLength);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setNClob(int parameterIndex, NClob value) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setNClob(String.valueOf(parameterIndex),value);
		drv.debugEnd();
	}

	public
	void setNClob(String parameterName, NClob value) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.sql.NClob");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.NCLOB);
		param.setTypeName("NCLOB");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(value);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(true);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.NClob);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setNClob(int parameterIndex, Reader reader) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setNClob(String.valueOf(parameterIndex),reader);
		drv.debugEnd();
	}

	public
	void setNClob(String parameterName, Reader reader) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.sql.NClob");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.NCLOB);
		param.setTypeName("NCLOB");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(reader);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(true);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.NClobReader);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setNClob(int parameterIndex, Reader reader,
					long length) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setNClob(String.valueOf(parameterIndex),reader,length);
		drv.debugEnd();
	}

	public
	void setNClob(String parameterName, Reader reader,
					long length) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.sql.NClob");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.NCLOB);
		param.setTypeName("NCLOB");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(reader);
		param.setLength(length);
		param.setIsBinary(false);
		param.setIsLob(true);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.NClobReaderWithLength);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setNString(int parameterIndex, String value) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setNString(String.valueOf(parameterIndex),value);
		drv.debugEnd();
	}

	public
	void setNString(String parameterName, String value) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.lang.String");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.NVARCHAR);
		param.setTypeName("NVARCHAR");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(value);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.NString);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setNull(int parameterIndex, int sqlType) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setNull(String.valueOf(parameterIndex),sqlType);
		drv.debugEnd();
	}

	public
	void setNull(String parameterName, int sqlType) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);
		drv.debugPrintln("sql type: "+sqlType);

		boolean				signed=false;
		boolean				binary=false;
		boolean				lob=false;
		boolean				ascii=false;
		String				classname;
		String				typename;

		switch (sqlType) {
			case Types.ARRAY:
				classname="java.sql.Array";
				typename="ARRAY";
				break;
			case Types.BIGINT:
				signed=true;
				classname="java.lang.Long";
				typename="BIGINT";
				break;
			case Types.BINARY:
			case Types.VARBINARY:
			case Types.LONGVARBINARY:
				binary=true;
				classname="[B";
				typename="VARBINARY";
				break;
			case Types.BIT:
			case Types.BOOLEAN:
				classname="java.lang.Boolean";
				typename="BOOLEAN";
				break;
			case Types.BLOB:
				binary=true;
				lob=true;
				classname="java.sql.Blob";
				typename="BLOB";
				break;
			case Types.CHAR:
			case Types.VARCHAR:
			case Types.LONGVARCHAR:
				ascii=true;
				classname="java.lang.String";
				typename="VARCHAR";
				break;
			case Types.CLOB:
				lob=true;
				ascii=true;
				classname="java.sql.Clob";
				typename="CLOB";
				break;
			case Types.DATE:
				classname="java.sql.Date";
				typename="DATE";
				break;
			case Types.DECIMAL:
			case Types.NUMERIC:
				signed=true;
				classname="java.math.BigDecimal";
				typename="DECIMAL";
				break;
			case Types.DOUBLE:
				signed=true;
				classname="java.lang.Double";
				typename="DOUBLE";
				break;
			case Types.FLOAT:
			case Types.REAL:
				signed=true;
				classname="java.lang.Float";
				typename="FLOAT";
				break;
			case Types.INTEGER:
				signed=true;
				classname="java.lang.Integer";
				typename="INTEGER";
				break;
			case Types.NCHAR:
			case Types.NVARCHAR:
			case Types.LONGNVARCHAR:
				classname="java.lang.String";
				typename="NVARCHAR";
				break;
			case Types.NCLOB:
				lob=true;
				classname="java.sql.NClob";
				typename="NCLOB";
				break;
			case Types.NULL:
				classname="java.lang.Object";
				typename="NULL";
				break;
			case Types.REF:
				classname="java.sql.Ref";
				typename="REF";
				break;
			case Types.ROWID:
				classname="java.sql.RowId";
				typename="ROWID";
				break;
			case Types.SMALLINT:
				signed=true;
				classname="java.lang.Short";
				typename="SMALLINT";
				break;
			case Types.SQLXML:
				classname="java.sql.SQLXML";
				typename="SQLXML";
				break;
			case Types.TIME:
				classname="java.sql.Time";
				typename="TIME";
				break;
			case Types.TIMESTAMP:
				classname="java.sql.Timestamp";
				typename="TIMESTAMP";
				break;
			case Types.TINYINT:
				signed=true;
				classname="java.lang.Byte";
				typename="TINYINT";
				break;
			default:
				ascii=true;
				classname="java.lang.String";
				typename="VARCHAR";
				break;
		}

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName(classname);
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(sqlType);
		param.setTypeName(typename);
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(signed);
		param.setObject(null);
		param.setLength(-1);
		param.setIsBinary(binary);
		param.setIsLob(lob);
		param.setIsAscii(ascii);
		param.setCalendar(null);
		param.setBindType(BindType.Null);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void 	setNull(int parameterIndex, int sqlType,
				String typeName) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setNull(String.valueOf(parameterIndex),sqlType,typeName);
		drv.debugEnd();
	}

	public
	void 	setNull(String parameterName, int sqlType,
				String typeName) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);
		drv.debugPrintln("sql type: "+sqlType);
		drv.debugPrintln("type name: "+typeName);

		boolean				signed=false;
		boolean				binary=false;
		boolean				lob=false;
		boolean				ascii=false;
		String				classname;
		String				tn;

		switch (sqlType) {
			case Types.ARRAY:
				classname="java.sql.Array";
				tn="ARRAY";
				break;
			case Types.BIGINT:
				signed=true;
				classname="java.lang.Long";
				tn="BIGINT";
				break;
			case Types.BINARY:
			case Types.VARBINARY:
			case Types.LONGVARBINARY:
				binary=true;
				classname="[B";
				tn="VARBINARY";
				break;
			case Types.BIT:
			case Types.BOOLEAN:
				classname="java.lang.Boolean";
				tn="BOOLEAN";
				break;
			case Types.BLOB:
				binary=true;
				lob=true;
				classname="java.sql.Blob";
				tn="BLOB";
				break;
			case Types.CHAR:
			case Types.VARCHAR:
			case Types.LONGVARCHAR:
				ascii=true;
				classname="java.lang.String";
				tn="VARCHAR";
				break;
			case Types.CLOB:
				lob=true;
				ascii=true;
				classname="java.sql.Clob";
				tn="CLOB";
				break;
			case Types.DATE:
				classname="java.sql.Date";
				tn="DATE";
				break;
			case Types.DECIMAL:
			case Types.NUMERIC:
				signed=true;
				classname="java.math.BigDecimal";
				tn="DECIMAL";
				break;
			case Types.DOUBLE:
				signed=true;
				classname="java.lang.Double";
				tn="DOUBLE";
				break;
			case Types.FLOAT:
			case Types.REAL:
				signed=true;
				classname="java.lang.Float";
				tn="FLOAT";
				break;
			case Types.INTEGER:
				signed=true;
				classname="java.lang.Integer";
				tn="INTEGER";
				break;
			case Types.NCHAR:
			case Types.NVARCHAR:
			case Types.LONGNVARCHAR:
				classname="java.lang.String";
				tn="NVARCHAR";
				break;
			case Types.NCLOB:
				lob=true;
				classname="java.sql.NClob";
				tn="NCLOB";
				break;
			case Types.NULL:
				classname="java.lang.Object";
				tn="NULL";
				break;
			case Types.REF:
				classname="java.sql.Ref";
				tn="REF";
				break;
			case Types.ROWID:
				classname="java.sql.RowId";
				tn="ROWID";
				break;
			case Types.SMALLINT:
				signed=true;
				classname="java.lang.Short";
				tn="SMALLINT";
				break;
			case Types.SQLXML:
				classname="java.sql.SQLXML";
				tn="SQLXML";
				break;
			case Types.TIME:
				classname="java.sql.Time";
				tn="TIME";
				break;
			case Types.TIMESTAMP:
				classname="java.sql.Timestamp";
				tn="TIMESTAMP";
				break;
			case Types.TINYINT:
				signed=true;
				classname="java.lang.Byte";
				tn="TINYINT";
				break;
			default:
				ascii=true;
				classname="java.lang.String";
				tn="VARCHAR";
				break;
		}

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName(classname);
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(sqlType);
		param.setTypeName(typeName);
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(signed);
		param.setObject(null);
		param.setLength(-1);
		param.setIsBinary(binary);
		param.setIsLob(lob);
		param.setIsAscii(ascii);
		param.setCalendar(null);
		param.setBindType(BindType.NullWithTypeName);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setObject(int parameterIndex, Object x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setObject(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setObject(String parameterName, Object x) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		conn.throwFeatureNotSupportedException();

		drv.debugEnd();
	}

	public
	void setObject(int parameterIndex, Object x,
				int targetSqlType) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setObject(String.valueOf(parameterIndex),x,targetSqlType);
		drv.debugEnd();
	}

	public
	void setObject(String parameterName, Object x,
				int targetSqlType) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		conn.throwFeatureNotSupportedException();

		drv.debugEnd();
	}

	public
	void setObject(int parameterIndex, Object x,
				int targetSqlType, int scaleOrLength)
							throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setObject(String.valueOf(parameterIndex),
					x,targetSqlType,scaleOrLength);
		drv.debugEnd();
	}

	public
	void setObject(String parameterName, Object x,
				int targetSqlType, int scaleOrLength)
							throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		conn.throwFeatureNotSupportedException();

		drv.debugEnd();
	}

	public
	void setRef(int parameterIndex, Ref x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setRef(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setRef(String parameterName, Ref x) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		conn.throwFeatureNotSupportedException();

		drv.debugEnd();
	}

	public
	void setRowId(int parameterIndex, RowId x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setRowId(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setRowId(String parameterName, RowId x) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		conn.throwFeatureNotSupportedException();

		drv.debugEnd();
	}

	public
	void setShort(int parameterIndex, short x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setShort(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setShort(String parameterName, short x) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);
		drv.debugPrintln("value: "+x);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.lang.Short");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.SMALLINT);
		param.setTypeName("SMALLINT");
		param.setPrecision(5);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(true);
		param.setObject(Short.valueOf(x));
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.Short);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setString(int parameterIndex, String x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setString(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setString(String parameterName, String x) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.lang.String");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.VARCHAR);
		param.setTypeName("VARCHAR");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(x);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.String);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setSQLXML(int parameterIndex, SQLXML xmlObject)
						throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setSQLXML(String.valueOf(parameterIndex),xmlObject);
		drv.debugEnd();
	}

	public
	void setSQLXML(String parameterName, SQLXML xmlObject)
						throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		conn.throwFeatureNotSupportedException();

		drv.debugEnd();
	}

	public
	void setTime(int parameterIndex, Time x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setTime(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setTime(String parameterName, Time x) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.sql.Time");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.TIME);
		param.setTypeName("TIME");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(x);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.Time);

		parameters.put(parameterName,param);
		drv.debugEnd();
	}

	public
	void setTime(int parameterIndex, Time x,
					Calendar cal) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setTime(String.valueOf(parameterIndex),x,cal);
		drv.debugEnd();
	}

	public
	void setTime(String parameterName, Time x,
					Calendar cal) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.sql.Time");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.TIME);
		param.setTypeName("TIME");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(x);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(cal);
		param.setBindType(BindType.TimeWithCalendar);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setTimestamp(int parameterIndex, Timestamp x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setTimestamp(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setTimestamp(String parameterName, Timestamp x)
							throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.sql.Timestamp");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.TIMESTAMP);
		param.setTypeName("TIMESTAMP");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(x);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.Timestamp);

		parameters.put(parameterName,param);
		drv.debugEnd();
	}

	public
	void setTimestamp(int parameterIndex, Timestamp x,
					Calendar cal) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setTimestamp(String.valueOf(parameterIndex),x,cal);
		drv.debugEnd();
	}

	public
	void setTimestamp(String parameterName, Timestamp x,
					Calendar cal) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.sql.Timestamp");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.TIMESTAMP);
		param.setTypeName("TIMESTAMP");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(x);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(cal);
		param.setBindType(BindType.TimestampWithCalendar);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setUnicodeStream(int parameterIndex, InputStream x,
					int length) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setUnicodeStream(String.valueOf(parameterIndex),x,length);
		drv.debugEnd();
	}

	public
	void setUnicodeStream(String parameterName, InputStream x,
					int length) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("java.lang.String");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setType(Types.VARCHAR);
		param.setTypeName("VARCHAR");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(x);
		param.setLength(length);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(BindType.UnicodeStream);

		parameters.put(parameterName,param);

		drv.debugEnd();
	}

	public
	void setURL(int parameterIndex, URL x) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("parameter index: "+parameterIndex);
		setURL(String.valueOf(parameterIndex),x);
		drv.debugEnd();
	}

	public
	void setURL(String parameterName, URL x) throws SQLException {
		drv.debugFunction(this);

		throwExceptionIfClosed();

		drv.debugPrintln("parameter name: "+parameterName);
		drv.debugPrintln("url: "+x.toString());

		setString(parameterName,x.toString());

		drv.debugEnd();
	}

	protected
	String asciiStreamToString(InputStream stream) {
		drv.debugFunction(this);
		String	s=streamToString(stream,"US-ASCII");
		drv.debugPrintln("string: "+s);
		drv.debugEnd();
		return s;
	}

	protected
	String asciiStreamToString(InputStream stream, long length) {
		drv.debugFunction(this);
		String	s=streamToString(stream,length,"US-ASCII");
		drv.debugPrintln("string: "+s);
		drv.debugEnd();
		return s;
	}

	protected
	String unicodeStreamToString(InputStream stream, long length) {
		drv.debugFunction(this);
		String	s=streamToString(stream,length,"UTF-8");
		drv.debugPrintln("string: "+s);
		drv.debugEnd();
		return s;
	}

	protected
	String streamToString(InputStream stream, String encoding) {
		drv.debugFunction(this);
		String	s;
		try {
			s=readerToString(new BufferedReader(
						new InputStreamReader(
							stream,encoding)));
		} catch (Exception ex) {
			s=new String("");
		}
		drv.debugPrintln("string: "+s);
		drv.debugEnd();
		return s;
	}

	protected
	String streamToString(InputStream stream, long length,
						String encoding) {
		drv.debugFunction(this);
		String	s;
		try {
			s=readerToString(new BufferedReader(
						new InputStreamReader(
							stream,encoding)),
							length);
		} catch (Exception ex) {
			s=new String("");
		}
		drv.debugPrintln("string: "+s);
		drv.debugEnd();
		return s;
	}

	protected
	String readerToString(Reader reader) {
		drv.debugFunction(this);
		String	s;
		try {
			StringBuilder	stringbuilder=new StringBuilder();
			int	c=0;
			while ((c=reader.read())!=-1) {
				stringbuilder.append((char)c);
			}
			s=stringbuilder.toString();
		} catch (Exception ex) {
			s=new String("");
		}
		drv.debugPrintln("string: "+s);
		drv.debugEnd();
		return s;
	}

	protected
	String readerToString(Reader reader, long length) {
		drv.debugFunction(this);
		String	s;
		try {
			StringBuilder	stringbuilder=new StringBuilder();
			int	c=0;
			for (int i=0; i<length && (c=reader.read())!=-1; i++) {
				stringbuilder.append((char)c);
			}
			s=stringbuilder.toString();
		} catch (Exception ex) {
			s=new String("");
		}
		drv.debugPrintln("string: "+s);
		drv.debugEnd();
		return s;
	}

	protected
	String clobToString(Clob clob) {
		drv.debugFunction(this);
		String	s;
		try {
			s=readerToString(clob.getCharacterStream());
		} catch (Exception ex) {
			s=new String("");
		}
		drv.debugPrintln("string: "+s);
		drv.debugEnd();
		return s;
	}

	protected
	String nClobToUnicodeString(NClob clob) {
		drv.debugFunction(this);
		String	s;
		try {
			s=readerToString(clob.getCharacterStream());
		} catch (Exception ex) {
			s=new String("");
		}
		drv.debugPrintln("string: "+s);
		drv.debugEnd();
		return s;
	}

	protected
	byte[] binaryStreamToBytes(InputStream stream) {
		drv.debugFunction(this);
		byte[]	b;
		try {
			ByteArrayOutputStream	output=
						new ByteArrayOutputStream();
			int	bytesread=0;
			byte[]	buffer=new byte[1024];
			while ((bytesread=
				stream.read(buffer,0,buffer.length))!=-1) {
				output.write(buffer,0,bytesread);
			}
			output.flush();
			b=output.toByteArray();
		} catch (Exception ex) {
			b=new byte[]{0};
		}
		drv.debugEnd();
		return b;
	}

	protected
	byte[] binaryStreamToBytes(InputStream stream, long length) {
		drv.debugFunction(this);
		byte[]	b;
		try {
			ByteArrayOutputStream	output=
						new ByteArrayOutputStream();
			int	bytesread=0;
			byte[]	buffer=new byte[1024];
			for (int i=0;
				i<length &&
				((bytesread=stream.read(buffer,0,
							buffer.length))!=-1);
				i++) {
				output.write(buffer,0,bytesread);
			}
			output.flush();
			b=output.toByteArray();
		} catch (Exception ex) {
			b=new byte[]{0};
		} 
		drv.debugEnd();
		return b;
	}

	protected
	byte[] blobToBytes(Blob blob) {
		drv.debugFunction(this);
		byte[]	b;
		try {
			b=binaryStreamToBytes(blob.getBinaryStream());
		} catch (Exception ex) {
			b=new byte[]{0};
		} 
		drv.debugEnd();
		return b;
	}
}
