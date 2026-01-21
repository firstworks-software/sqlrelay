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

import com.firstworks.sqlrelay.*;

public class SQLRelayPreparedStatement
		extends SQLRelayStatement
		implements PreparedStatement {

	private ArrayList<HashMap<Integer,SQLRelayParameter>>	batch;
	private HashMap<Integer,SQLRelayParameter>		parameters;

	public SQLRelayPreparedStatement(SQLRelayDriver driver) {
		super(driver);
		driver.debugFunction(this);
		batch=new ArrayList<HashMap<Integer,SQLRelayParameter>>();
		parameters=new HashMap<Integer,SQLRelayParameter>();
		driver.debugEnd();
	}

	public
	void addBatch() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		batch.add(parameters);
		parameters=new HashMap<Integer,SQLRelayParameter>();
		driver.debugEnd();
	}

	public
	void clearParameters() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		sqlrcur.clearBinds();
		batch.clear();
		parameters.clear();
		driver.debugEnd();
	}

	public
	boolean execute() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		// FIXME: handle timeout
		resultset=null;
		updatecount=-1;
		sqlrcur.clearBinds();
		if (batch.size()==0) {
			bind(parameters);
		} else {
			bind(batch.get(0));
		}
		boolean	result=false;
		synchronized (networklock) {
			result=sqlrcur.executeQuery();
		}
		if (result) {
			resultset=new SQLRelayResultSet(driver);
			resultset.setNetworkLock(networklock);
			resultset.setStatement(this);
			resultset.setSQLRCursor(sqlrcur);
		} else {
			throwErrorMessageException();
		}
		driver.debugEnd();
		return result;
	}

	public
	int[] executeBatch() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		// FIXME: handle timeout
		int[]	results=new int[batch.size()];
		int	count=0;
		for (HashMap<Integer,SQLRelayParameter> params: batch) {
			sqlrcur.clearBinds();
			bind(params);
			results[count++]=executeUpdate();
		}
		driver.debugEnd();
		return results;
	}

	public
	ResultSet executeQuery() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		// FIXME: handle timeout
		resultset=null;
		updatecount=-1;
		sqlrcur.clearBinds();
		if (batch.size()==0) {
			bind(parameters);
		} else {
			bind(batch.get(0));
		}
		boolean	success=false;
		synchronized (networklock) {
			success=sqlrcur.executeQuery();
		}
		if (success) {
			resultset=new SQLRelayResultSet(driver);
			resultset.setNetworkLock(networklock);
			resultset.setStatement(this);
			resultset.setSQLRCursor(sqlrcur);
		} else {
			throwErrorMessageException();
		}
		driver.debugEnd();
		return resultset;
	}

	public
	int executeUpdate() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		// FIXME: handle timeout
		resultset=null;
		updatecount=-1;
		sqlrcur.clearBinds();
		if (batch.size()==0) {
			bind(parameters);
		} else {
			bind(batch.get(0));
		}
		boolean	success=false;
		synchronized (networklock) {
			success=sqlrcur.executeQuery();
		}
		if (success) {
			updatecount=(int)sqlrcur.affectedRows();
		} else {
			throwErrorMessageException();
		}
		driver.debugEnd();
		return updatecount;
	}

	private void bind(HashMap<Integer,SQLRelayParameter> params)
							throws SQLException {
		driver.debugFunction(this);

		if (params==null) {
			driver.debugEnd();
			return;
		}

		for (Map.Entry<Integer,SQLRelayParameter> entry:
							params.entrySet()) {

			String			key=entry.getKey().toString();
			SQLRelayParameter	value=entry.getValue();

			SQLRelayParameter.BindType	bindtype=
							value.getBindType();

			if (value.getObject()==null) {
				bindtype=SQLRelayParameter.BindType.Null;
			}

			switch (bindtype) {
				case Array:
					// not supported
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
					// not supported
					break;
				case DateWithCalendar:
					// not supported
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
					// not supported
					break;
				case TimeWithCalendar:
					// not supported
					break;
				case Timestamp:
					// not supported
					break;
				case TimestampWithCalendar:
					// not supported
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
					// not supported
					break;
			}
		}
		driver.debugEnd();
	}

	public
	ResultSetMetaData getMetaData() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		ResultSetMetaData	rsmd=
				(resultset!=null)?resultset.getMetaData():null;
		driver.debugEnd();
		return rsmd;
	}

	public
	ParameterMetaData getParameterMetaData() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameterMetaData	pmd=
				new SQLRelayParameterMetaData(driver);
		if (batch.size()==0) {
			pmd.setParameters(parameters);
		} else {
			pmd.setParameters(batch.get(0));
		}
		driver.debugEnd();
		return pmd;
	}

	public
	void setArray(int parameterIndex, Array x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public
	void setAsciiStream(int parameterIndex, InputStream x)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setAsciiStream(int parameterIndex, InputStream x,
					int length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setAsciiStream(int parameterIndex, InputStream x,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setBigDecimal(int parameterIndex, BigDecimal x)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setBinaryStream(int parameterIndex, InputStream x)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setBinaryStream(int parameterIndex, InputStream x,
					int length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setBinaryStream(int parameterIndex, InputStream x,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setBlob(int parameterIndex, Blob x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setBlob(int parameterIndex, InputStream inputStream)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setBlob(int parameterIndex, InputStream inputStream,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setBoolean(int parameterIndex, boolean x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setByte(int parameterIndex, byte x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setBytes(int parameterIndex, byte[] x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		Byte[]	bytes=new Byte[x.length];
		for (int i=0; i<x.length; i++) {
			bytes[i]=x[i];
		}
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setCharacterStream(int parameterIndex, Reader reader)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setCharacterStream(int parameterIndex, Reader reader,
					int length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setCharacterStream(int parameterIndex, Reader reader,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setClob(int parameterIndex, Clob x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setClob(int parameterIndex, Reader reader) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setClob(int parameterIndex, Reader reader,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setDate(int parameterIndex, Date x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		// FIXME: support this...
		/*SQLRelayParameter	param=new SQLRelayParameter(driver);
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
			SQLRelayParameter.BindType.Date);
		parameters.put(parameterIndex,param);*/
		driver.debugEnd();
	}

	public
	void setDate(int parameterIndex, Date x, Calendar cal)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		// FIXME: support this...
		/*SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);*/
		driver.debugEnd();
	}

	public
	void setDouble(int parameterIndex, double x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setFloat(int parameterIndex, float x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setInt(int parameterIndex, int x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setLong(int parameterIndex, long x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setNCharacterStream(int parameterIndex, Reader value)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setNCharacterStream(int parameterIndex, Reader value,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setNClob(int parameterIndex, NClob value) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setNClob(int parameterIndex, Reader reader) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setNClob(int parameterIndex, Reader reader,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setNString(int parameterIndex, String value) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setNull(int parameterIndex, int sqlType) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void 	setNull(int parameterIndex, int sqlType,
				String typeName) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setObject(int parameterIndex, Object x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public
	void setObject(int parameterIndex, Object x,
				int targetSqlType) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public
	void setObject(int parameterIndex, Object x,
				int targetSqlType, int scaleOrLength)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public
	void setRef(int parameterIndex, Ref x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public
	void setRowId(int parameterIndex, RowId x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public
	void setShort(int parameterIndex, short x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setString(int parameterIndex, String x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setSQLXML(int parameterIndex, SQLXML xmlObject)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public
	void setTime(int parameterIndex, Time x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		// FIXME: support this...
		/*SQLRelayParameter	param=new SQLRelayParameter(driver);
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
			SQLRelayParameter.BindType.Time);
		parameters.put(parameterIndex,param);*/
		driver.debugEnd();
	}

	public
	void setTime(int parameterIndex, Time x,
					Calendar cal) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		// FIXME: support this...
		/*SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);*/
		driver.debugEnd();
	}

	public
	void setTimestamp(int parameterIndex, Timestamp x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		// FIXME: support this...
		/*SQLRelayParameter	param=new SQLRelayParameter(driver);
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
			SQLRelayParameter.BindType.Timestamp);
		parameters.put(parameterIndex,param);*/
		driver.debugEnd();
	}

	public
	void setTimestamp(int parameterIndex, Timestamp x,
					Calendar cal) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		// FIXME: support this...
		/*SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);*/
		driver.debugEnd();
	}

	public
	void setUnicodeStream(int parameterIndex, InputStream x,
					int length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayParameter	param=new SQLRelayParameter(driver);
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
		parameters.put(parameterIndex,param);
		driver.debugEnd();
	}

	public
	void setURL(int parameterIndex, URL x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public
	String asciiStreamToString(InputStream stream) {
		driver.debugFunction(this);
		String	s=streamToString(stream,"US-ASCII");
		driver.debugEnd();
		return s;
	}

	public
	String asciiStreamToString(InputStream stream, long length) {
		driver.debugFunction(this);
		String	s=streamToString(stream,length,"US-ASCII");
		driver.debugEnd();
		return s;
	}

	public
	String unicodeStreamToString(InputStream stream, long length) {
		driver.debugFunction(this);
		String	s=streamToString(stream,length,"UTF-8");
		driver.debugEnd();
		return s;
	}

	public
	String streamToString(InputStream stream, String encoding) {
		driver.debugFunction(this);
		String	s;
		try {
			s=readerToString(new BufferedReader(
						new InputStreamReader(
							stream,encoding)));
		} catch (Exception ex) {
			s=new String("");
		}
		driver.debugEnd();
		return s;
	}

	public
	String streamToString(InputStream stream, long length,
						String encoding) {
		driver.debugFunction(this);
		String	s;
		try {
			s=readerToString(new BufferedReader(
						new InputStreamReader(
							stream,encoding)),
							length);
		} catch (Exception ex) {
			s=new String("");
		}
		driver.debugEnd();
		return s;
	}

	public
	String readerToString(Reader reader) {
		driver.debugFunction(this);
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
		driver.debugEnd();
		return s;
	}

	public
	String readerToString(Reader reader, long length) {
		driver.debugFunction(this);
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
		driver.debugEnd();
		return s;
	}

	public
	String clobToString(Clob clob) {
		driver.debugFunction(this);
		String	s;
		try {
			s=asciiStreamToString(clob.getAsciiStream());
		} catch (Exception ex) {
			s=new String("");
		}
		driver.debugEnd();
		return s;
	}

	public
	String nClobToUnicodeString(NClob clob) {
		driver.debugFunction(this);
		String	s;
		try {
			s=readerToString(clob.getCharacterStream());
		} catch (Exception ex) {
			s=new String("");
		}
		driver.debugEnd();
		return s;
	}

	public
	byte[] binaryStreamToBytes(InputStream stream) {
		driver.debugFunction(this);
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
		driver.debugEnd();
		return b;
	}

	public
	byte[] binaryStreamToBytes(InputStream stream, long length) {
		driver.debugFunction(this);
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
		driver.debugEnd();
		return b;
	}

	public
	byte[] blobToBytes(Blob blob) {
		driver.debugFunction(this);
		byte[]	b;
		try {
			b=binaryStreamToBytes(blob.getBinaryStream());
		} catch (Exception ex) {
			b=new byte[]{0};
		} 
		driver.debugEnd();
		return b;
	}
}
