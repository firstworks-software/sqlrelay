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

		resultset=null;
		updatecount=-1;

		sqlrcur.clearBinds();
		if (batch.size()==0) {
			bind(parameters);
		} else {
			bind(batch.get(0));
		}

		// FIXME: handle timeout

		boolean	result=false;
		synchronized (networklock) {
			result=sqlrcur.executeQuery();
		}

		drv.debugPrintln("result: "+result);

		if (result) {

			updatecount=(int)sqlrcur.affectedRows();

			drv.debugPrintln("update count: "+updatecount);
			drv.debugPrintln("column count: "+sqlrcur.colCount());

			resultset=new SQLRelayResultSet(drv);
			resultset.setNetworkLock(networklock);
			resultset.setStatement(this);
			resultset.setConnection(conn);
			resultset.setSQLRCursor(sqlrcur);
		} else {
			throwErrorMessageException();
		}

		drv.debugEnd();
		return result;
	}

	public
	int[] executeBatch() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();

		drv.debugPrintln("batch size: "+batch.size());

		int[]	results=new int[batch.size()];
		int	count=0;

		for (HashMap<String,SQLRelayParameter> params: batch) {
			sqlrcur.clearBinds();
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

		resultset=null;
		updatecount=-1;

		sqlrcur.clearBinds();
		if (batch.size()==0) {
			bind(parameters);
		} else {
			bind(batch.get(0));
		}

		// FIXME: handle timeout

		boolean	success=false;
		synchronized (networklock) {
			success=sqlrcur.executeQuery();
		}

		if (success) {

			updatecount=(int)sqlrcur.affectedRows();

			drv.debugPrintln("update count: "+updatecount);
			drv.debugPrintln("column count: "+sqlrcur.colCount());

			resultset=new SQLRelayResultSet(drv);
			resultset.setNetworkLock(networklock);
			resultset.setStatement(this);
			resultset.setConnection(conn);
			resultset.setSQLRCursor(sqlrcur);
		} else {
			throwErrorMessageException();
		}

		drv.debugEnd();
		return resultset;
	}

	public
	int executeUpdate() throws SQLException {
drv.debug=true;
		drv.debugFunction(this);
		throwExceptionIfClosed();

		resultset=null;
		updatecount=-1;

		sqlrcur.clearBinds();
		if (batch.size()==0) {
			bind(parameters);
		} else {
			bind(batch.get(0));
		}

		// FIXME: handle timeout

		boolean	success=false;
		synchronized (networklock) {
conn.getSQLRConnection().debugOn();
			success=sqlrcur.executeQuery();
		}

		if (success) {
			updatecount=(int)sqlrcur.affectedRows();

			drv.debugPrintln("update count: "+updatecount);
		} else {
			throwErrorMessageException();
		}

		drv.debugEnd();
drv.debug=false;
		return updatecount;
	}

	private
	void bind(HashMap<String,SQLRelayParameter> params)
							throws SQLException {
		drv.debugFunction(this);

		if (params==null) {
			drv.debugEnd();
			return;
		}

		for (Map.Entry<String,SQLRelayParameter> entry:
							params.entrySet()) {

			String			key=entry.getKey();
			SQLRelayParameter	value=entry.getValue();

			SQLRelayParameter.BindType	bindtype=
							value.getBindType();

			if (value.getObject()==null) {
				bindtype=SQLRelayParameter.BindType.Null;
			}

			switch (bindtype) {
				case Array:
					// FIXME: support this, somehow
					break;
				case AsciiStream:
					sqlrcur.inputBind(
						key,
						asciiStreamToString(
							(InputStream)
							value.getObject()));
					break;
				case AsciiStreamWithIntLength:
				case AsciiStreamWithLongLength:
					sqlrcur.inputBind(
						key,
						asciiStreamToString(
							(InputStream)
							value.getObject(),
							value.getLength()));
					break;
				case BigDecimal:
					sqlrcur.inputBind(
						key,
						((BigDecimal)value.getObject()).
								doubleValue(),
								0,0);
					// FIXME: set precision and scale
					break;
				case BinaryStream:
					{
					byte[]	bytes=binaryStreamToBytes(
							(InputStream)
							value.getObject(),-1);
					sqlrcur.inputBindBlob(key,bytes,
								bytes.length);
					}
					break;
				case BinaryStreamWithIntLength:
				case BinaryStreamWithLongLength:
					{
					byte[]	bytes=binaryStreamToBytes(
							(InputStream)
							value.getObject(),
							value.getLength());
					sqlrcur.inputBindBlob(key,bytes,
								bytes.length);
					}
					break;
				case Blob:
					{
					byte[]	bytes=blobToBytes((Blob)
							value.getObject());
					sqlrcur.inputBindBlob(key,bytes,
								bytes.length);
					}
					break;
				case BlobInputStream:
					{
					byte[]	bytes=binaryStreamToBytes(
							(InputStream)
							value.getObject(),-1);
					sqlrcur.inputBindBlob(key,bytes,
								bytes.length);
					}
					break;
				case BlobInputStreamWithLongLength:
					{
					byte[]	bytes=binaryStreamToBytes(
							(InputStream)
							value.getObject(),
							value.getLength());
					sqlrcur.inputBindBlob(key,bytes,
								bytes.length);
					}
					break;
				case Boolean:
					{
					long	val=(((Boolean)value.
							getObject()).
							booleanValue()==true)?
							1:0;
					sqlrcur.inputBind(key,val);
					}
					break;
				case Byte:
					{
					long	val=((Byte)value.
							getObject()).
							byteValue();
					sqlrcur.inputBind(key,val);
					}
					break;
				case Bytes:
					{
					Byte[]	v=(Byte[])value.getObject();
					byte[]	val=new byte[v.length];
					for (int i=0; i<v.length; i++) {
						val[i]=v[i].byteValue();
					}
					sqlrcur.inputBindBlob(key,val,
								val.length);
					}
					break;
				case CharacterStream:
					sqlrcur.inputBind(
						key,
						readerToString(
							(Reader)
							value.getObject(),
							-1));
					break;
				case CharacterStreamWithIntLength:
				case CharacterStreamWithLongLength:
					sqlrcur.inputBind(
						key,
						readerToString(
							(Reader)
							value.getObject(),
							value.getLength()));
					break;
				case Clob:
					{
					String	string=clobToString((Clob)
							value.getObject());
					sqlrcur.inputBindClob(
							key,string,
							string.length());
					}
					break;
				case ClobReader:
					{
					String	string=readerToString(
							(Reader)
							value.getObject(),
							-1);
					sqlrcur.inputBindClob(
							key,string,
							string.length());
					}
					break;
				case ClobReaderWithLength:
					{
					String	string=readerToString(
							(Reader)
							value.getObject(),
							value.getLength());
					sqlrcur.inputBindClob(
							key,string,
							string.length());
					}
					break;
				case Date:
					{
					Calendar	cal=
						Calendar.getInstance();
					cal.setTime((Date)value.getObject());
					sqlrcur.inputBind(key,
						(short)cal.get(
							Calendar.YEAR),
						(short)cal.get(
							Calendar.MONTH),
						(short)cal.get(
							Calendar.DAY_OF_MONTH),
						(short)0,
						(short)0,
						(short)0,
						(short)0,
						null,false);
					}
					break;
				case DateWithCalendar:
					{
					Calendar	cal=
						Calendar.getInstance();
					cal.setTime((Date)value.getObject());
					sqlrcur.inputBind(key,
						(short)cal.get(
							Calendar.YEAR),
						(short)cal.get(
							Calendar.MONTH),
						(short)cal.get(
							Calendar.DAY_OF_MONTH),
						(short)0,
						(short)0,
						(short)0,
						(short)0,
						null,false);
					}
					break;
				case Double:
					sqlrcur.inputBind(
						key,
						((Double)value.getObject()).
								doubleValue(),
								0,0);
					// FIXME: set precision and scale
					break;
				case Float:
					sqlrcur.inputBind(
						key,
						((Float)value.getObject()).
								floatValue(),
								0,0);
					// FIXME: set precision and scale
					break;
				case Int:
					sqlrcur.inputBind(
						key,
						((Integer)value.getObject()).
								intValue());
					break;
				case Long:
					sqlrcur.inputBind(
						key,
						((Long)value.getObject()).
								longValue());
					break;
				case NCharStream:
					sqlrcur.inputBind(
						key,
						readerToString(
							(Reader)
							value.getObject(),
							-1));
					break;
				case NCharStreamWithLength:
					sqlrcur.inputBind(
						key,
						readerToString(
							(Reader)
							value.getObject(),
							value.getLength()));
					break;
				case NClob:
					{
					String	string=nClobToUnicodeString(
							(NClob)
							value.getObject());
					sqlrcur.inputBindClob(
							key,string,
							string.length());
					}
					break;
				case NClobReader:
					{
					String	string=readerToString(
							(Reader)
							value.getObject(),
							-1);
					sqlrcur.inputBindClob(
							key,string,
							string.length());
					}
					break;
				case NClobReaderWithLength:
					{
					String	string=readerToString(
							(Reader)
							value.getObject(),
							value.getLength());
					sqlrcur.inputBindClob(
							key,string,
							string.length());
					}
					break;
				case NString:
					sqlrcur.inputBind(key,
						(String)value.getObject());
					break;
				case Null:
				case NullWithTypeName:
					sqlrcur.inputBind(key,null);
					break;
				case Object:
					// not supported
					break;
				case ObjectWithTargetType:
					// not supported
					break;
				case ObjectWithTargetTypeAndScaleOrLength:
					// not supported
					break;
				case Ref:
					// not supported
					break;
				case RowId:
					// not supported
					break;
				case Short:
					sqlrcur.inputBind(
						key,
						((Short)value.getObject()).
								shortValue());
					break;
				case String:
					sqlrcur.inputBind(key,
						(String)value.getObject());
					break;
				case SQLXML:
					// not supported
					break;
				case Time:
					// FIXME: support this
					break;
				case TimeWithCalendar:
					// FIXME: support this
					break;
				case Timestamp:
					// FIXME: support this
					break;
				case TimestampWithCalendar:
					// FIXME: support this
					break;
				case UnicodeStream:
					sqlrcur.inputBind(
						key,
						unicodeStreamToString(
							(InputStream)
							value.getObject(),
							value.getLength()));
					break;
				case URL:
					sqlrcur.inputBind(
						key,
						((URL)value.getObject()).
								toString());
					break;
			}
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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(SQLRelayParameter.BindType.AsciiStream);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.AsciiStreamWithIntLength);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.AsciiStreamWithLongLength);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(true);
		param.setObject(x);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(
			SQLRelayParameter.BindType.BigDecimal);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.BinaryStream);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.BinaryStreamWithIntLength);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.BinaryStreamWithLongLength);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.Blob);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.BlobInputStream);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.
				BlobInputStreamWithLongLength);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(Boolean.valueOf(x));
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(
			SQLRelayParameter.BindType.Boolean);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(Byte.valueOf(x));
		param.setLength(-1);
		param.setIsBinary(true);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(
			SQLRelayParameter.BindType.Byte);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.Bytes);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.CharacterStream);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.
				CharacterStreamWithIntLength);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.
				CharacterStreamWithLongLength);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.Clob);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.ClobReader);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.ClobReaderWithLength);

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
		setDate(parameterName,x,new GregorianCalendar());
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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.DateWithCalendar);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(true);
		param.setObject(Double.valueOf(x));
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(
			SQLRelayParameter.BindType.Double);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(true);
		param.setObject(Float.valueOf(x));
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(
			SQLRelayParameter.BindType.Float);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(true);
		param.setObject(Integer.valueOf(x));
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(
			SQLRelayParameter.BindType.Int);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(true);
		param.setObject(Long.valueOf(x));
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(
			SQLRelayParameter.BindType.Long);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.NCharStream);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.NCharStreamWithLength);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.NClob);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.NClobReader);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.NClobReaderWithLength);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.NString);

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

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(null);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(
			SQLRelayParameter.BindType.Null);

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

		SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(false);
		param.setObject(null);
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(
			SQLRelayParameter.BindType.NullWithTypeName);

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
		setObject(String.valueOf(parameterIndex),x,targetSqlType,scaleOrLength);
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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
		param.setPrecision(0);
		param.setScale(0);
		param.setIsNullable(ParameterMetaData.parameterNullable);
		param.setIsSigned(true);
		param.setObject(Short.valueOf(x));
		param.setLength(-1);
		param.setIsBinary(false);
		param.setIsLob(false);
		param.setIsAscii(false);
		param.setCalendar(null);
		param.setBindType(
			SQLRelayParameter.BindType.Short);

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.String);

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
		setTime(parameterName,x,new GregorianCalendar());
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

		conn.throwFeatureNotSupportedException();

		// FIXME: support this...
		/*SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.TimeWithCalendar);
		parameters.put(parameterName,param);*/

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
	void setTimestamp(String parameterName, Timestamp x) throws SQLException {
		drv.debugFunction(this);
		setTimestamp(parameterName,x,new GregorianCalendar());
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

		conn.throwFeatureNotSupportedException();

		// FIXME: support this...
		/*SQLRelayParameter	param=new SQLRelayParameter(drv);
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.TimestampWithCalendar);
		parameters.put(parameterName,param);*/

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
		param.setClassName("FIXME");
		param.setMode(ParameterMetaData.parameterModeIn);
		param.setTypeName("FIXME");
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
		param.setBindType(
			SQLRelayParameter.BindType.UnicodeStream);

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
