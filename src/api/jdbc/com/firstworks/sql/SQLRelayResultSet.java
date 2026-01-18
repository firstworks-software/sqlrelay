package com.firstworks.sql;

import java.sql.*;

import java.io.InputStream;
import java.io.Reader;
import java.io.ByteArrayInputStream;
import java.io.StringBufferInputStream;
import java.io.StringReader;
import java.io.InputStreamReader;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.util.Calendar;
import java.util.Map;
import java.net.URL;
import java.net.MalformedURLException;

import com.firstworks.sqlrelay.*;

public class SQLRelayResultSet implements ResultSet {

	private Statement	statement;
	private SQLRCursor	sqlrcur;
	private long		currentrow;
	private	boolean		beforefirst;
	private	boolean		islast;
	private	boolean		afterlast;
	private	int		fetchdirection;
	private boolean		wasnull;
	private SQLRelayDriver	driver;

	public SQLRelayResultSet(SQLRelayDriver driver) {
		this.driver=driver;
		driver.debugFunction(this);
		reset();
		driver.debugEnd();
	}

	private void reset() {
		driver.debugFunction(this);
		statement=null;
		sqlrcur=null;
		currentrow=0;
		beforefirst=true;
		islast=false;
		afterlast=false;
		fetchdirection=ResultSet.FETCH_FORWARD;
		wasnull=false;
		driver.debugEnd();
	}

	public synchronized
	void setStatement(Statement statement) {
		this.statement=statement;
	}

	public synchronized
	void setSQLRCursor(SQLRCursor sqlrcur) {
		this.sqlrcur=sqlrcur;
	}

	public synchronized
	SQLRCursor getSQLRCursor() {
		return sqlrcur;
	}

	public synchronized
	boolean absolute(int row) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("row: ",row);
		if (row<currentrow) {
			String	ex="FIXME: ResultSet type is Forward-Only";
			driver.debugPrintln("exception: ",ex);
			throw new SQLException(ex);
		} else if (row==0) {
			beforefirst=true;
			currentrow=0;
			islast=false;
			afterlast=false;
		} else if (row>0) {
			beforefirst=false;
			currentrow=row;
			// FIXME: we can evaulate the result set buffer size
			// to decide whether or not we need to call getField()
			sqlrcur.getField(currentrow-1,0);
			long	rowcount=sqlrcur.rowCount();
			if (sqlrcur.endOfResultSet()) {
				if (currentrow-1==rowcount-1) {
					islast=true;
					afterlast=false;
				} else if (currentrow-1>=rowcount) {
					islast=false;
					afterlast=true;
					driver.debugPrintln("after last");
					driver.debugEnd();
					return false;
				}
			}
		} else if (row<0) {
			// FIXME: implement this...
			// position relative to end of result set
			String	ex="FIXME: negative row not supported";
			driver.debugPrintln("exception: ",ex);
			throw new SQLException(ex);
		}
		driver.debugPrintln("success");
		driver.debugEnd();
		return true;
	}

	public synchronized
	void afterLast() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		afterlast=true;
		driver.debugEnd();
	}

	public synchronized
	void beforeFirst() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		beforefirst=true;
		driver.debugEnd();
	}

	public synchronized
	void cancelRowUpdates() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void clearWarnings() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugEnd();
	}

	public synchronized
	void close() throws SQLException {
		driver.debugFunction(this);
		if (sqlrcur!=null) {
			sqlrcur.closeResultSet();
		}
		reset();
		driver.debugEnd();
	}

	public synchronized
	void deleteRow() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	int findColumn(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		for (int i=0; i<sqlrcur.colCount(); i++) {
			if (sqlrcur.getColumnName(i).equals(columnlabel)) {
				driver.debugPrintln("column: ",(i+1));
				driver.debugEnd();
				return i+1;
			}
		}
		String	ex=("Column not found");
		driver.debugPrintln(ex);
		throw new SQLException(ex);
	}

	public synchronized
	boolean first() throws SQLException {
		driver.debugFunction(this);
		boolean	abs=absolute(1);
		driver.debugEnd();
		return abs;
	}

	public synchronized
	Array getArray(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	Array getArray(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public synchronized
	InputStream getAsciiStream(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		// FIXME: not sure this is correct, how do we ensure it's ascii?
		InputStream	is=new StringBufferInputStream(field);
		driver.debugEnd();
		return is;
	}

	public synchronized
	InputStream getAsciiStream(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		// FIXME: not sure this is correct, how do we ensure it's ascii?
		InputStream	is=new StringBufferInputStream(field);
		driver.debugEnd();
		return is;
	}

	public synchronized
	BigDecimal getBigDecimal(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		if (wasnull) {
			driver.debugEnd();
			return null;
		}
		BigDecimal	bd=new BigDecimal(field);
		driver.debugEnd();
		return bd;
	}

	public synchronized
	BigDecimal getBigDecimal(int columnindex, int scale)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		if (wasnull) {
			driver.debugEnd();
			return null;
		}
		BigDecimal	bd=new BigDecimal(
				new BigInteger(field.replace("\\.","")),
				scale);
		driver.debugEnd();
		return bd;
	}

	public synchronized
	BigDecimal getBigDecimal(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		if (wasnull) {
			driver.debugEnd();
			return null;
		}
		BigDecimal	bd=new BigDecimal(field);
		driver.debugEnd();
		return bd;
	}

	public synchronized
	BigDecimal getBigDecimal(String columnlabel, int scale)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		if (wasnull) {
			driver.debugEnd();
			return null;
		}
		BigDecimal	bd=new BigDecimal(
				new BigInteger(field.replace("\\.","")),
				scale);
		driver.debugEnd();
		return bd;
	}

	public synchronized
	InputStream getBinaryStream(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		byte[]	field=sqlrcur.getFieldAsByteArray(
					currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		InputStream	is=new ByteArrayInputStream(field);
		driver.debugEnd();
		return is;
	}

	public synchronized
	InputStream getBinaryStream(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		byte[]	field=sqlrcur.getFieldAsByteArray(
					currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		InputStream	is=new ByteArrayInputStream(field);
		driver.debugEnd();
		return is;
	}

	public synchronized
	Blob getBlob(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		// FIXME: we could theoretically support this, but currently
		// SQLRelayResultSetMetaData.getColumnType/getColumnClassName
		// don't return any lob types, so it's not currently necessary
		driver.debugEnd();
		return null;
	}

	public synchronized
	Blob getBlob(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		// FIXME: we could theoretically support this, but currently
		// SQLRelayResultSetMetaData.getColumnType/getColumnClassName
		// don't return any lob types, so it's not currently necessary
		driver.debugEnd();
		return null;
	}

	public synchronized
	boolean getBoolean(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		boolean	eq=field.equals("1");
		driver.debugEnd();
		return eq;
	}

	public synchronized
	boolean getBoolean(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		boolean	eq=field.equals("1");
		driver.debugEnd();
		return eq;
	}

	public synchronized
	byte getByte(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		long	field=sqlrcur.getFieldAsInteger(
					currentrow-1,columnindex-1);
		wasnull=(sqlrcur.getField(currentrow-1,columnindex-1)==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return (byte)field;
	}

	public synchronized
	byte getByte(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		long	field=sqlrcur.getFieldAsInteger(
					currentrow-1,columnlabel);
		wasnull=(sqlrcur.getField(currentrow-1,columnlabel)==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return (byte)field;
	}

	public synchronized
	byte[] getBytes(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		byte[]	field=sqlrcur.getFieldAsByteArray(
					currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return field;
	}

	public synchronized
	byte[] getBytes(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		byte[]	field=sqlrcur.getFieldAsByteArray(
					currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return field;
	}

	public synchronized
	Reader getCharacterStream(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return (wasnull)?null:(new StringReader(field));
	}

	public synchronized
	Reader getCharacterStream(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return (wasnull)?null:(new StringReader(field));
	}

	public synchronized
	Clob getClob(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		// FIXME: we could theoretically support this, but currently
		// SQLRelayResultSetMetaData.getColumnType/getColumnClassName
		// don't return any lob types, so it's not currently necessary
		driver.debugEnd();
		return null;
	}

	public synchronized
	Clob getClob(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		// FIXME: we could theoretically support this, but currently
		// SQLRelayResultSetMetaData.getColumnType/getColumnClassName
		// don't return any lob types, so it's not currently necessary
		driver.debugEnd();
		return null;
	}

	public synchronized
	int getConcurrency() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		int	concurrency=ResultSet.CONCUR_READ_ONLY;
		driver.debugPrintln("concurrency: ",concurrency);
		driver.debugEnd();
		return concurrency;
	}

	public synchronized
	String getCursorName() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		String	cursorname=null;
		driver.debugPrintln("cursor name: ",cursorname);
		driver.debugEnd();
		return cursorname;
	}

	public synchronized
	Date getDate(int columnindex) throws SQLException {
		driver.debugFunction(this);
		// FIXME: pass in some default calendar
		Date	dt=getDate(columnindex,null);
		driver.debugEnd();
		return dt;
	}

	public synchronized
	Date getDate(int columnindex, Calendar cal) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		// FIXME: use cal
		// FIXME: field isn't guaranteed to be in iso format
		Date	dt=null;
		if (!wasnull) {
			dt=Date.valueOf(field);
		}
		driver.debugEnd();
		return dt;
	}

	public synchronized
	Date getDate(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		// FIXME: pass in some default calendar
		Date	dt=getDate(columnlabel,null);
		driver.debugEnd();
		return dt;
	}

	public synchronized
	Date getDate(String columnlabel, Calendar cal) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		// FIXME: use cal
		// FIXME: field isn't guaranteed to be in iso format
		Date	dt=null;
		if (!wasnull) {
			dt=Date.valueOf(field);
		}
		driver.debugEnd();
		return dt;
	}

	public synchronized
	double getDouble(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		double	field=sqlrcur.getFieldAsDouble(
					currentrow-1,columnindex-1);
		wasnull=(sqlrcur.getField(currentrow-1,columnindex-1)==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return field;
	}

	public synchronized
	double getDouble(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		double	field=sqlrcur.getFieldAsDouble(
					currentrow-1,columnlabel);
		wasnull=(sqlrcur.getField(currentrow-1,columnlabel)==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return field;
	}

	public synchronized
	int getFetchDirection() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		int	direction=ResultSet.FETCH_FORWARD;
		driver.debugPrintln("direction: ",direction);
		driver.debugEnd();
		return direction;
	}

	public synchronized
	int getFetchSize() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		int	size=(int)sqlrcur.getResultSetBufferSize();
		driver.debugPrintln("size: ",size);
		driver.debugEnd();
		return size;
	}

	public synchronized
	float getFloat(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		float	field=(float)sqlrcur.getFieldAsDouble(
					currentrow-1,columnindex-1);
		wasnull=(sqlrcur.getField(currentrow-1,columnindex-1)==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return field;
	}

	public synchronized
	float getFloat(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		float	field=(float)sqlrcur.getFieldAsDouble(
					currentrow-1,columnlabel);
		wasnull=(sqlrcur.getField(currentrow-1,columnlabel)==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return field;
	}

	public synchronized
	int getHoldability() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		// FIXME: is this correct?
		int	holdability=ResultSet.CLOSE_CURSORS_AT_COMMIT;
		driver.debugPrintln("holdability: ",holdability);
		driver.debugEnd();
		return holdability;
	}

	public synchronized
	int getInt(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		int	field=(int)sqlrcur.getFieldAsInteger(
					currentrow-1,columnindex-1);
		wasnull=(sqlrcur.getField(currentrow-1,columnindex-1)==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return field;
	}

	public synchronized
	int getInt(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		int	field=(int)sqlrcur.getFieldAsInteger(
					currentrow-1,columnlabel);
		wasnull=(sqlrcur.getField(currentrow-1,columnlabel)==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return field;
	}

	public synchronized
	long getLong(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		long	field=(long)sqlrcur.getFieldAsInteger(
					currentrow-1,columnindex-1);
		wasnull=(sqlrcur.getField(currentrow-1,columnindex-1)==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return field;
	}

	public synchronized
	long getLong(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		long	field=(long)sqlrcur.getFieldAsInteger(
					currentrow-1,columnlabel);
		wasnull=(sqlrcur.getField(currentrow-1,columnlabel)==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return field;
	}

	public synchronized
	ResultSetMetaData getMetaData() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayResultSetMetaData	metadata=
				new SQLRelayResultSetMetaData(driver);
		metadata.setSQLRCursor(sqlrcur);
		driver.debugEnd();
		return metadata;
	}

	public synchronized
	Reader getNCharacterStream(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		byte[]	field=sqlrcur.getFieldAsByteArray(
						currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		Reader	r=null;
		if (!wasnull) {
			r=(new InputStreamReader(
					new ByteArrayInputStream(field)));
		}
		driver.debugEnd();
		return r;
	}

	public synchronized
	Reader getNCharacterStream(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		byte[]	field=sqlrcur.getFieldAsByteArray(
						currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		Reader	r=null;
		if (!wasnull) {
			r=(new InputStreamReader(
					new ByteArrayInputStream(field)));
		}
		driver.debugEnd();
		return r;
	}

	public synchronized
	NClob getNClob(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		// FIXME: we could theoretically support this, but currently
		// SQLRelayResultSetMetaData.getColumnType/getColumnClassName
		// don't return any lob types, so it's not currently necessary
		driver.debugEnd();
		return null;
	}

	public synchronized
	NClob getNClob(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		// FIXME: we could theoretically support this, but currently
		// SQLRelayResultSetMetaData.getColumnType/getColumnClassName
		// don't return any lob types, so it's not currently necessary
		driver.debugEnd();
		return null;
	}

	public synchronized
	String getNString(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		byte[]	field=sqlrcur.getFieldAsByteArray(
						currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		try {
			String	str=null;
			if (!wasnull) {
				str=new String(field,"UTF-8");
			}
			driver.debugEnd();
			return str;
		} catch (Exception ex) {
			throw new SQLException(ex.getMessage());
		}
	}

	public synchronized
	String getNString(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		byte[]	field=sqlrcur.getFieldAsByteArray(
						currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		try {
			String	str=null;
			if (!wasnull) {
				str=new String(field,"UTF-8");
			}
			driver.debugEnd();
			return str;
		} catch (Exception ex) {
			throw new SQLException(ex.getMessage());
		}
	}

	public synchronized
	Object getObject(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public synchronized
	<T> T getObject(int columnindex, Class<T> type) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		return null;
	}

	public synchronized
	Object getObject(int columnindex, Map<String,Class<?>> map)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public synchronized
	Object getObject(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public synchronized
	<T> T getObject(String columnlabel, Class<T> type) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public synchronized
	Object getObject(String columnlabel, Map<String,Class<?>> map)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public synchronized
	Ref getRef(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public synchronized
	Ref getRef(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public synchronized
	int getRow() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("current row: ",currentrow);
		driver.debugEnd();
		return (int)currentrow;
	}

	public synchronized
	RowId getRowId(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public synchronized
	RowId getRowId(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public synchronized
	short getShort(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		short	field=(short)sqlrcur.getFieldAsInteger(
						currentrow-1,columnindex-1);
		wasnull=(sqlrcur.getField(currentrow-1,columnindex-1)==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return field;
	}

	public synchronized
	short getShort(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		short	field=(short)sqlrcur.getFieldAsInteger(
						currentrow-1,columnlabel);
		wasnull=(sqlrcur.getField(currentrow-1,columnlabel)==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return field;
	}

	public synchronized
	SQLXML getSQLXML(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public synchronized
	SQLXML getSQLXML(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public synchronized
	Statement getStatement() throws SQLException {
		//driver.debugFunction(this);
		//driver.debugEnd();
		return statement;
	}

	public synchronized
	String getString(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return field;
	}

	public synchronized
	String getString(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return field;
	}

	public synchronized
	Time getTime(int columnindex) throws SQLException {
		driver.debugFunction(this);
		// FIXME: pass in some default calendar
		Time	t=getTime(columnindex,null);
		driver.debugEnd();
		return t;
	}

	public synchronized
	Time getTime(int columnindex, Calendar cal) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		// FIXME: use cal
		// FIXME: not guaranteed to be in iso format
		Time	t=null;
		if (!wasnull) {
			t=Time.valueOf(field);
		}
		driver.debugEnd();
		return t;
	}

	public synchronized
	Time getTime(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		// FIXME: pass in some default calendar
		Time	t=getTime(columnlabel,null);
		driver.debugEnd();
		return t;
	}

	public synchronized
	Time getTime(String columnlabel, Calendar cal) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		// FIXME: use cal
		// FIXME: not guaranteed to be in iso format
		Time	t=null;
		if (!wasnull) {
			t=Time.valueOf(field);
		}
		driver.debugEnd();
		return t;
	}

	public synchronized
	Timestamp getTimestamp(int columnindex) throws SQLException {
		driver.debugFunction(this);
		// FIXME: pass in some default calendar
		Timestamp	t=getTimestamp(columnindex,null);
		driver.debugEnd();
		return t;
	}

	public synchronized
	Timestamp getTimestamp(int columnindex, Calendar cal)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		// FIXME: use cal
		// FIXME: not guaranteed to be in iso format
		// FIXME: postgresql ends with timezone offset (eg. -04)
		Timestamp	t=null;
		if (!wasnull) {
			t=Timestamp.valueOf(field);
		}
		driver.debugEnd();
		return t;
	}

	public synchronized
	Timestamp getTimestamp(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		// FIXME: pass in some default calendar
		Timestamp	t=getTimestamp(columnlabel,null);
		driver.debugEnd();
		return t;
	}

	public synchronized
	Timestamp getTimestamp(String columnlabel, Calendar cal)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		// FIXME: use cal
		// FIXME: not guaranteed to be in iso format
		Timestamp	t=null;
		if (!wasnull) {
			t=Timestamp.valueOf(field);
		}
		driver.debugEnd();
		return t;
	}

	public synchronized
	int getType() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		int	type=ResultSet.TYPE_FORWARD_ONLY;
		driver.debugPrintln("type: ",type);
		driver.debugEnd();
		return type;
	}

	public synchronized
	InputStream getUnicodeStream(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		InputStream	is=new StringBufferInputStream(field);
		driver.debugEnd();
		return is;
	}

	public synchronized
	InputStream getUnicodeStream(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		InputStream	is=new StringBufferInputStream(field);
		driver.debugEnd();
		return is;
	}

	public synchronized
	URL getURL(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("column index: ",columnindex);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		if (wasnull) {
			driver.debugEnd();
			return null;
		}
		try {
			URL	u=new URL(field);
			driver.debugEnd();
			return u;
		} catch (MalformedURLException ex) {
			driver.debugEnd();
			return null;
		}
	}

	public synchronized
	URL getURL(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("column: ",columnlabel);
		driver.debugPrintln("field: ",field);
		driver.debugPrintln("was null: ",wasnull);
		if (wasnull) {
			driver.debugEnd();
			return null;
		}
		try {
			URL	u=new URL(field);
			driver.debugEnd();
			return u;
		} catch (MalformedURLException ex) {
			driver.debugEnd();
			return null;
		}
	}

	public synchronized
	SQLWarning getWarnings() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugEnd();
		return null;
	}

	public synchronized
	void insertRow() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	boolean isAfterLast() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("after last: ",afterlast);
		driver.debugEnd();
		return afterlast;
	}

	public synchronized
	boolean isBeforeFirst() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("before first: ",beforefirst);
		driver.debugEnd();
		return beforefirst;
	}

	public synchronized
	boolean isClosed() throws SQLException {
		driver.debugFunction(this);
		boolean	isclosed=(sqlrcur==null);
		driver.debugPrintln("is closed: ",isclosed);
		driver.debugEnd();
		return isclosed;
	}

	public synchronized
	boolean isFirst() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		boolean	isfirst=(currentrow==0);
		driver.debugPrintln("is first: ",isfirst);
		driver.debugEnd();
		return isfirst;
	}

	public synchronized
	boolean isLast() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("is last: ",islast);
		driver.debugEnd();
		return islast;
	}

	public synchronized
	boolean last() throws SQLException {
		driver.debugFunction(this);
		boolean	abs=absolute(-1);
		driver.debugEnd();
		return abs;
	}

	public synchronized
	void moveToCurrentRow() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		// since we don't support updating result sets, then we can't
		// be on the insert row, and we're always on the current row
		driver.debugEnd();
	}

	public synchronized
	void moveToInsertRow() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	boolean next() throws SQLException {
		driver.debugFunction(this);
		boolean	rel=relative(1);
		driver.debugEnd();
		return rel;
	}

	public synchronized
	boolean previous() throws SQLException {
		driver.debugFunction(this);
		boolean	rel=relative(-1);
		driver.debugEnd();
		return rel;
	}

	public synchronized
	void refreshRow() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	boolean relative(int rows) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("rows: ",rows);
		if (rows==0) {
			driver.debugEnd();
			return true;
		}
		int	newrow=(int)(currentrow+rows);
		driver.debugPrintln("newrow (before): ",newrow);
		if (newrow<1) {
			newrow=1;
		}
		driver.debugPrintln("newrow (after): ",newrow);
		boolean	abs=absolute(newrow);
		driver.debugEnd();
		return abs;
	}

	public synchronized
	boolean rowDeleted() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return false;
	}

	public synchronized
	boolean rowInserted() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return false;
	}

	public synchronized
	boolean rowUpdated() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return false;
	}

	public synchronized
	void setFetchDirection(int direction) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("direction: ",direction);
		fetchdirection=direction;
		driver.debugEnd();
	}

	public synchronized
	void setFetchSize(int rows) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("fetch size: ",rows);
		sqlrcur.setResultSetBufferSize(rows);
		driver.debugEnd();
	}

	public synchronized
	void updateArray(int columnindex, Array x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateArray(String columnlabel, Array x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateAsciiStream(int columnindex, InputStream x)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateAsciiStream(int columnindex, InputStream x,
					int length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateAsciiStream(int columnindex, InputStream x,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateAsciiStream(String columnlabel, InputStream x)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateAsciiStream(String columnlabel,InputStream x,
					int length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateAsciiStream(String columnlabel, InputStream x,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateBigDecimal(int columnindex, BigDecimal x)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateBigDecimal(String columnlabel, BigDecimal x)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateBinaryStream(int columnindex, InputStream x)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateBinaryStream(int columnindex, InputStream x,
					int length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateBinaryStream(int columnindex, InputStream x,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateBinaryStream(String columnlabel, InputStream x)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateBinaryStream(String columnlabel, InputStream x,
					int length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateBinaryStream(String columnlabel, InputStream x,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateBlob(int columnindex, Blob x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateBlob(int columnindex, InputStream inputStream)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateBlob(int columnindex, InputStream inputStream,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateBlob(String columnlabel, Blob x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateBlob(String columnlabel, InputStream inputStream)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateBlob(String columnlabel, InputStream inputStream,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateBoolean(int columnindex, boolean x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateBoolean(String columnlabel, boolean x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateByte(int columnindex, byte x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateByte(String columnlabel, byte x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateBytes(int columnindex, byte[] x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateBytes(String columnlabel, byte[] x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateCharacterStream(int columnindex, Reader x)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateCharacterStream(int columnindex, Reader x,
					int length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateCharacterStream(int columnindex, Reader x,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateCharacterStream(String columnlabel, Reader reader)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateCharacterStream(String columnlabel, Reader reader,
					int length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateCharacterStream(String columnlabel, Reader reader,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateClob(int columnindex, Clob x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateClob(int columnindex, Reader reader) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateClob(int columnindex, Reader reader,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateClob(String columnlabel, Clob x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateClob(String columnlabel, Reader reader) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateClob(String columnlabel, Reader reader,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateDate(int columnindex, Date x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateDate(String columnlabel, Date x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateDouble(int columnindex, double x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateDouble(String columnlabel, double x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateFloat(int columnindex, float x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateFloat(String columnlabel, float x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateInt(int columnindex, int x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateInt(String columnlabel, int x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateLong(int columnindex, long x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateLong(String columnlabel, long x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateNCharacterStream(int columnindex, Reader x)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateNCharacterStream(int columnindex, Reader x,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateNCharacterStream(String columnlabel, Reader reader)
							throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateNCharacterStream(String columnlabel, Reader reader,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateNClob(int columnindex, NClob nClob) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateNClob(int columnindex, Reader reader) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateNClob(int columnindex, Reader reader,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateNClob(String columnlabel, NClob nClob) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateNClob(String columnlabel, Reader reader)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateNClob(String columnlabel, Reader reader,
					long length) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateNString(int columnindex, String nString)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateNString(String columnlabel, String nString)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateNull(int columnindex) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateNull(String columnlabel) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateObject(int columnindex, Object x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateObject(int columnindex, Object x,
					int scaleOrLength) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateObject(String columnlabel, Object x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateObject(String columnlabel, Object x,
					int scaleOrLength) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateRef(int columnindex, Ref x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateRef(String columnlabel, Ref x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateRow() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateRowId(int columnindex, RowId x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateRowId(String columnlabel, RowId x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateShort(int columnindex, short x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateShort(String columnlabel, short x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateSQLXML(int columnindex, SQLXML xmlObject)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateSQLXML(String columnlabel, SQLXML xmlObject)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateString(int columnindex, String x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateString(String columnlabel, String x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateTime(int columnindex, Time x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateTime(String columnlabel, Time x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateTimestamp(int columnindex, Timestamp x) throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	void updateTimestamp(String columnlabel, Timestamp x)
						throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public synchronized
	boolean wasNull() throws SQLException {
		driver.debugFunction(this);
		throwExceptionIfClosed();
		driver.debugPrintln("was null: ",wasnull);
		driver.debugEnd();
		return wasnull;
	}

	public synchronized
	boolean isWrapperFor(Class<?> iface) throws SQLException {
		driver.debugFunction(this);
		driver.debugEnd();
		return (iface==SQLRCursor.class);
	}

	@SuppressWarnings({"unchecked"})
	public synchronized
	<T> T unwrap(Class<T> iface) throws SQLException {
		driver.debugFunction(this);
		driver.debugEnd();
		return (T)((iface==SQLRCursor.class)?sqlrcur:null);
	}

	private void throwExceptionIfClosed() throws SQLException {
		if (sqlrcur==null) {
			throw new SQLException("ResultSet is closed");
		}
	}

	private void throwInvalidColumn(int columnindex) throws SQLException {
		if (columnindex<1 || columnindex>sqlrcur.colCount()) {
			throw new SQLException("invalid column index");
		}
	}

	private void throwInvalidColumn(String columnlabel)
						throws SQLException {
		String[] cols=sqlrcur.getColumnNames();
		for (int i=0; i<cols.length; i++) {
			if (cols[i].equals(columnlabel)) {
				return;
			}
		}
		throw new SQLException("invalid column label");
	}

	private void throwErrorMessageException() throws SQLException {
		throw new SQLException(sqlrcur.errorMessage());
	}

	private void throwFeatureNotSupportedException() throws SQLException {
		throw new SQLFeatureNotSupportedException();
	}
}
