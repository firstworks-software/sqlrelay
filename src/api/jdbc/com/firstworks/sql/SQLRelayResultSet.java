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
		driver.debugFunction();
		reset();
		driver.debugEnd();
	}

	private void reset() {
		driver.debugFunction();
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

	public void setStatement(Statement statement) {
		driver.debugFunction();
		this.statement=statement;
		driver.debugEnd();
	}

	public void setSQLRCursor(SQLRCursor sqlrcur) {
		driver.debugFunction();
		this.sqlrcur=sqlrcur;
		driver.debugEnd();
	}

	public boolean absolute(int row) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugPrintln("row: "+row);
		if (row<currentrow) {
			String	ex="FIXME: ResultSet "+
					"type is Forward-Only";
			driver.debugPrintln("exception: "+ex);
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
			driver.debugPrintln("exception: "+ex);
			throw new SQLException(ex);
		}
		driver.debugPrintln("success");
		driver.debugEnd();
		return true;
	}

	public void afterLast() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		afterlast=true;
		driver.debugEnd();
	}

	public void beforeFirst() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		beforefirst=true;
		driver.debugEnd();
	}

	public void cancelRowUpdates() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void clearWarnings() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugEnd();
	}

	public void close() throws SQLException {
		driver.debugFunction();
		if (sqlrcur!=null) {
			sqlrcur.closeResultSet();
		}
		reset();
		driver.debugEnd();
	}

	public void deleteRow() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public int findColumn(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		for (int i=0; i<sqlrcur.colCount(); i++) {
			if (sqlrcur.getColumnName(i).equals(columnlabel)) {
				driver.debugPrintln("column: "+(i+1));
				driver.debugEnd();
				return i+1;
			}
		}
		String	ex=("Column not found");
		driver.debugPrintln(ex);
		throw new SQLException(ex);
	}

	public boolean first() throws SQLException {
		driver.debugFunction();
		boolean	abs=absolute(1);
		driver.debugEnd();
		return abs;
	}

	public Array	getArray(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public Array	getArray(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return null;
	}

	public InputStream	getAsciiStream(int columnindex)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		// FIXME: not sure this is correct, how do we ensure it's ascii?
		InputStream	is=new StringBufferInputStream(field);
		driver.debugEnd();
		return is;
	}

	public InputStream	getAsciiStream(String columnlabel)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		// FIXME: not sure this is correct, how do we ensure it's ascii?
		InputStream	is=new StringBufferInputStream(field);
		driver.debugEnd();
		return is;
	}

	public BigDecimal	getBigDecimal(int columnindex)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		if (wasnull) {
			driver.debugEnd();
			return null;
		}
		BigDecimal	bd=new BigDecimal(field);
		driver.debugEnd();
		return bd;
	}

	public BigDecimal	getBigDecimal(int columnindex, int scale)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
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

	public BigDecimal	getBigDecimal(String columnlabel)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		if (wasnull) {
			driver.debugEnd();
			return null;
		}
		BigDecimal	bd=new BigDecimal(field);
		driver.debugEnd();
		return bd;
	}

	public BigDecimal	getBigDecimal(String columnlabel, int scale)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
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

	public InputStream	getBinaryStream(int columnindex)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		byte[]	field=sqlrcur.getFieldAsByteArray(
					currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		InputStream	is=new ByteArrayInputStream(field);
		driver.debugEnd();
		return is;
	}

	public InputStream	getBinaryStream(String columnlabel)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		byte[]	field=sqlrcur.getFieldAsByteArray(
					currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		InputStream	is=new ByteArrayInputStream(field);
		driver.debugEnd();
		return is;
	}

	public Blob	getBlob(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		// FIXME: we could theoretically support this, but currently
		// SQLRelayResultSetMetaData.getColumnType/getColumnClassName
		// don't return any lob types, so it's not currently necessary
		driver.debugEnd();
		return null;
	}

	public Blob	getBlob(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		// FIXME: we could theoretically support this, but currently
		// SQLRelayResultSetMetaData.getColumnType/getColumnClassName
		// don't return any lob types, so it's not currently necessary
		driver.debugEnd();
		return null;
	}

	public boolean	getBoolean(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		boolean	eq=field.equals("1");
		driver.debugEnd();
		return eq;
	}

	public boolean	getBoolean(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		boolean	eq=field.equals("1");
		driver.debugEnd();
		return eq;
	}

	public byte	getByte(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		long	field=sqlrcur.getFieldAsInteger(
					currentrow-1,columnindex-1);
		wasnull=(sqlrcur.getField(currentrow-1,columnindex-1)==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		driver.debugEnd();
		return (byte)field;
	}

	public byte	getByte(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		long	field=sqlrcur.getFieldAsInteger(
					currentrow-1,columnlabel);
		wasnull=(sqlrcur.getField(currentrow-1,columnlabel)==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		driver.debugEnd();
		return (byte)field;
	}

	public byte[]	getBytes(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		byte[]	field=sqlrcur.getFieldAsByteArray(
					currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		driver.debugEnd();
		return field;
	}

	public byte[]	getBytes(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		byte[]	field=sqlrcur.getFieldAsByteArray(
					currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		driver.debugEnd();
		return field;
	}

	public Reader	getCharacterStream(int columnindex)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		driver.debugEnd();
		return (wasnull)?null:(new StringReader(field));
	}

	public Reader	getCharacterStream(String columnlabel)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		driver.debugEnd();
		return (wasnull)?null:(new StringReader(field));
	}

	public Clob	getClob(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		// FIXME: we could theoretically support this, but currently
		// SQLRelayResultSetMetaData.getColumnType/getColumnClassName
		// don't return any lob types, so it's not currently necessary
		driver.debugEnd();
		return null;
	}

	public Clob	getClob(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		// FIXME: we could theoretically support this, but currently
		// SQLRelayResultSetMetaData.getColumnType/getColumnClassName
		// don't return any lob types, so it's not currently necessary
		driver.debugEnd();
		return null;
	}

	public int	getConcurrency() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		int	concurrency=ResultSet.CONCUR_READ_ONLY;
		driver.debugPrintln("concurrency: "+concurrency);
		driver.debugEnd();
		return concurrency;
	}

	public String	getCursorName() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		String	cursorname=null;
		driver.debugPrintln("cursor name: "+cursorname);
		driver.debugEnd();
		return cursorname;
	}

	public Date	getDate(int columnindex) throws SQLException {
		driver.debugFunction();
		// FIXME: pass in some default calendar
		Date	dt=getDate(columnindex,null);
		driver.debugEnd();
		return dt;
	}

	public Date	getDate(int columnindex, Calendar cal)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("wasnull: "+wasnull);
		// FIXME: use cal
		// FIXME: field isn't guaranteed to be in iso format
		Date	dt=null;
		if (!wasnull) {
			dt=Date.valueOf(field);
		}
		driver.debugEnd();
		return dt;
	}

	public Date	getDate(String columnlabel) throws SQLException {
		driver.debugFunction();
		// FIXME: pass in some default calendar
		Date	dt=getDate(columnlabel,null);
		driver.debugEnd();
		return dt;
	}

	public Date	getDate(String columnlabel, Calendar cal)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("wasnull: "+wasnull);
		// FIXME: use cal
		// FIXME: field isn't guaranteed to be in iso format
		Date	dt=null;
		if (!wasnull) {
			dt=Date.valueOf(field);
		}
		driver.debugEnd();
		return dt;
	}

	public double	getDouble(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		double	field=sqlrcur.getFieldAsDouble(
					currentrow-1,columnindex-1);
		wasnull=(sqlrcur.getField(currentrow-1,columnindex-1)==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		driver.debugEnd();
		return field;
	}

	public double	getDouble(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		double	field=sqlrcur.getFieldAsDouble(
					currentrow-1,columnlabel);
		wasnull=(sqlrcur.getField(currentrow-1,columnlabel)==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		driver.debugEnd();
		return field;
	}

	public int	getFetchDirection() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		int	direction=ResultSet.FETCH_FORWARD;
		driver.debugPrintln("direction: "+direction);
		driver.debugEnd();
		return direction;
	}

	public int	getFetchSize() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		int	size=(int)sqlrcur.getResultSetBufferSize();
		driver.debugPrintln("size: "+size);
		driver.debugEnd();
		return size;
	}

	public float	getFloat(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		float	field=(float)sqlrcur.getFieldAsDouble(
					currentrow-1,columnindex-1);
		wasnull=(sqlrcur.getField(currentrow-1,columnindex-1)==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		driver.debugEnd();
		return field;
	}

	public float	getFloat(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		float	field=(float)sqlrcur.getFieldAsDouble(
					currentrow-1,columnlabel);
		wasnull=(sqlrcur.getField(currentrow-1,columnlabel)==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		driver.debugEnd();
		return field;
	}

	public int	getHoldability() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		// FIXME: is this correct?
		int	holdability=ResultSet.CLOSE_CURSORS_AT_COMMIT;
		driver.debugPrintln("holdability: "+holdability);
		driver.debugEnd();
		return holdability;
	}

	public int	getInt(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		int	field=(int)sqlrcur.getFieldAsInteger(
					currentrow-1,columnindex-1);
		wasnull=(sqlrcur.getField(currentrow-1,columnindex-1)==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		driver.debugEnd();
		return field;
	}

	public int	getInt(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		int	field=(int)sqlrcur.getFieldAsInteger(
					currentrow-1,columnlabel);
		wasnull=(sqlrcur.getField(currentrow-1,columnlabel)==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		driver.debugEnd();
		return field;
	}

	public long	getLong(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		long	field=(long)sqlrcur.getFieldAsInteger(
					currentrow-1,columnindex-1);
		wasnull=(sqlrcur.getField(currentrow-1,columnindex-1)==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		driver.debugEnd();
		return field;
	}

	public long	getLong(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		long	field=(long)sqlrcur.getFieldAsInteger(
					currentrow-1,columnlabel);
		wasnull=(sqlrcur.getField(currentrow-1,columnlabel)==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		driver.debugEnd();
		return field;
	}

	public ResultSetMetaData	getMetaData() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		SQLRelayResultSetMetaData	metadata=
				new SQLRelayResultSetMetaData(driver);
		metadata.setSQLRCursor(sqlrcur);
		driver.debugEnd();
		return metadata;
	}

	public Reader	getNCharacterStream(int columnindex)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		byte[]	field=sqlrcur.getFieldAsByteArray(
						currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		Reader	r=null;
		if (!wasnull) {
			r=(new InputStreamReader(
					new ByteArrayInputStream(field)));
		}
		driver.debugEnd();
		return r;
	}

	public Reader	getNCharacterStream(String columnlabel)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		byte[]	field=sqlrcur.getFieldAsByteArray(
						currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
		Reader	r=null;
		if (!wasnull) {
			r=(new InputStreamReader(
					new ByteArrayInputStream(field)));
		}
		driver.debugEnd();
		return r;
	}

	public NClob	getNClob(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		// FIXME: we could theoretically support this, but currently
		// SQLRelayResultSetMetaData.getColumnType/getColumnClassName
		// don't return any lob types, so it's not currently necessary
		driver.debugEnd();
		return null;
	}

	public NClob	getNClob(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		// FIXME: we could theoretically support this, but currently
		// SQLRelayResultSetMetaData.getColumnType/getColumnClassName
		// don't return any lob types, so it's not currently necessary
		driver.debugEnd();
		return null;
	}

	public String	getNString(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		byte[]	field=sqlrcur.getFieldAsByteArray(
						currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
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

	public String	getNString(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		byte[]	field=sqlrcur.getFieldAsByteArray(
						currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("was null: "+wasnull);
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

	public Object	getObject(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public <T> T	getObject(int columnindex, Class<T> type)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		return null;
	}

	public Object	getObject(int columnindex, Map<String,Class<?>> map)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public Object	getObject(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public <T> T	getObject(String columnlabel, Class<T> type)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public Object	getObject(String columnlabel, Map<String,Class<?>> map)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public Ref	getRef(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public Ref	getRef(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public int	getRow() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugPrintln("current row: "+currentrow);
		driver.debugEnd();
		return (int)currentrow;
	}

	public RowId	getRowId(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public RowId	getRowId(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public short	getShort(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		short	field=(short)sqlrcur.getFieldAsInteger(
						currentrow-1,columnindex-1);
		wasnull=(sqlrcur.getField(currentrow-1,columnindex-1)==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("wasnull: "+wasnull);
		driver.debugEnd();
		return field;
	}

	public short	getShort(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		short	field=(short)sqlrcur.getFieldAsInteger(
						currentrow-1,columnlabel);
		wasnull=(sqlrcur.getField(currentrow-1,columnlabel)==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("wasnull: "+wasnull);
		driver.debugEnd();
		return field;
	}

	public SQLXML	getSQLXML(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public SQLXML	getSQLXML(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		driver.debugEnd();
		return null;
	}

	public Statement	getStatement() throws SQLException {
		driver.debugFunction();
		driver.debugEnd();
		return statement;
	}

	public String	getString(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("wasnull: "+wasnull);
		driver.debugEnd();
		return field;
	}

	public String	getString(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("wasnull: "+wasnull);
		driver.debugEnd();
		return field;
	}

	public Time	getTime(int columnindex) throws SQLException {
		driver.debugFunction();
		// FIXME: pass in some default calendar
		Time	t=getTime(columnindex,null);
		driver.debugEnd();
		return t;
	}

	public Time	getTime(int columnindex, Calendar cal)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("wasnull: "+wasnull);
		// FIXME: use cal
		// FIXME: not guaranteed to be in iso format
		Time	t=null;
		if (!wasnull) {
			t=Time.valueOf(field);
		}
		driver.debugEnd();
		return t;
	}

	public Time	getTime(String columnlabel) throws SQLException {
		driver.debugFunction();
		// FIXME: pass in some default calendar
		Time	t=getTime(columnlabel,null);
		driver.debugEnd();
		return t;
	}

	public Time	getTime(String columnlabel, Calendar cal)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("wasnull: "+wasnull);
		// FIXME: use cal
		// FIXME: not guaranteed to be in iso format
		Time	t=null;
		if (!wasnull) {
			t=Time.valueOf(field);
		}
		driver.debugEnd();
		return t;
	}

	public Timestamp	getTimestamp(int columnindex)
						throws SQLException {
		driver.debugFunction();
		// FIXME: pass in some default calendar
		Timestamp	t=getTimestamp(columnindex,null);
		driver.debugEnd();
		return t;
	}

	public Timestamp	getTimestamp(int columnindex, Calendar cal)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("wasnull: "+wasnull);
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

	public Timestamp	getTimestamp(String columnlabel)
							throws SQLException {
		driver.debugFunction();
		// FIXME: pass in some default calendar
		Timestamp	t=getTimestamp(columnlabel,null);
		driver.debugEnd();
		return t;
	}

	public Timestamp	getTimestamp(String columnlabel, Calendar cal)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("wasnull: "+wasnull);
		// FIXME: use cal
		// FIXME: not guaranteed to be in iso format
		Timestamp	t=null;
		if (!wasnull) {
			t=Timestamp.valueOf(field);
		}
		driver.debugEnd();
		return t;
	}

	public int	getType() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		int	type=ResultSet.TYPE_FORWARD_ONLY;
		driver.debugPrintln("type: "+type);
		driver.debugEnd();
		return type;
	}

	public InputStream	getUnicodeStream(int columnindex)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("wasnull: "+wasnull);
		InputStream	is=new StringBufferInputStream(field);
		driver.debugEnd();
		return is;
	}

	public InputStream	getUnicodeStream(String columnlabel)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("wasnull: "+wasnull);
		InputStream	is=new StringBufferInputStream(field);
		driver.debugEnd();
		return is;
	}

	public URL	getURL(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnindex);
		throwFeatureNotSupportedException();
		String	field=sqlrcur.getField(currentrow-1,columnindex-1);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("wasnull: "+wasnull);
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

	public URL	getURL(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwInvalidColumn(columnlabel);
		throwFeatureNotSupportedException();
		String	field=sqlrcur.getField(currentrow-1,columnlabel);
		wasnull=(field==null);
		driver.debugPrintln("field: "+field);
		driver.debugPrintln("wasnull: "+wasnull);
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

	public SQLWarning	getWarnings() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugEnd();
		return null;
	}

	public void	insertRow() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public boolean	isAfterLast() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugPrintln("after last: "+afterlast);
		driver.debugEnd();
		return afterlast;
	}

	public boolean	isBeforeFirst() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugPrintln("before first: "+beforefirst);
		driver.debugEnd();
		return beforefirst;
	}

	public boolean	isClosed() throws SQLException {
		driver.debugFunction();
		boolean	isclosed=(sqlrcur==null);
		driver.debugPrintln("is closed: "+isclosed);
		driver.debugEnd();
		return isclosed;
	}

	public boolean	isFirst() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		boolean	isfirst=(currentrow==0);
		driver.debugPrintln("is first: "+isfirst);
		driver.debugEnd();
		return isfirst;
	}

	public boolean	isLast() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugPrintln("is last: "+islast);
		driver.debugEnd();
		return islast;
	}

	public boolean	last() throws SQLException {
		driver.debugFunction();
		boolean	abs=absolute(-1);
		driver.debugEnd();
		return abs;
	}

	public void	moveToCurrentRow() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		// since we don't support updating result sets, then we can't
		// be on the insert row, and we're always on the current row
		driver.debugEnd();
	}

	public void	moveToInsertRow() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public boolean	next() throws SQLException {
		driver.debugFunction();
		boolean	rel=relative(1);
		driver.debugEnd();
		return rel;
	}

	public boolean	previous() throws SQLException {
		driver.debugFunction();
		boolean	rel=relative(-1);
		driver.debugEnd();
		return rel;
	}

	public void	refreshRow() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public boolean	relative(int rows) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugPrintln("rows: "+rows);
		if (rows==0) {
			driver.debugEnd();
			return true;
		}
		int	newrow=(int)(currentrow+rows);
		driver.debugPrintln("newrow (before): "+newrow);
		if (newrow<1) {
			newrow=1;
		}
		driver.debugPrintln("newrow (after): "+newrow);
		boolean	abs=absolute(newrow);
		driver.debugEnd();
		return abs;
	}

	public boolean	rowDeleted() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return false;
	}

	public boolean	rowInserted() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return false;
	}

	public boolean	rowUpdated() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
		return false;
	}

	public void setFetchDirection(int direction) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugPrintln("direction: "+direction);
		fetchdirection=direction;
		driver.debugEnd();
	}

	public void setFetchSize(int rows) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugPrintln("fetch size: "+rows);
		sqlrcur.setResultSetBufferSize(rows);
		driver.debugEnd();
	}

	public void	updateArray(int columnindex, Array x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateArray(String columnlabel, Array x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateAsciiStream(int columnindex,
						InputStream x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateAsciiStream(int columnindex,
						InputStream x,
						int length)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateAsciiStream(int columnindex,
						InputStream x,
						long length)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateAsciiStream(String columnlabel,
						InputStream x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateAsciiStream(String columnlabel,
						InputStream x,
						int length)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateAsciiStream(String columnlabel,
						InputStream x,
						long length)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateBigDecimal(int columnindex,
						BigDecimal x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateBigDecimal(String columnlabel,
						BigDecimal x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateBinaryStream(int columnindex,
						InputStream x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateBinaryStream(int columnindex,
						InputStream x,
						int length)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateBinaryStream(int columnindex,
						InputStream x,
						long length)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateBinaryStream(String columnlabel,
						InputStream x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateBinaryStream(String columnlabel,
						InputStream x,
						int length)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateBinaryStream(String columnlabel,
						InputStream x,
						long length)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateBlob(int columnindex, Blob x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateBlob(int columnindex,
					InputStream inputStream)
					throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateBlob(int columnindex,
					InputStream inputStream,
					long length)
					throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateBlob(String columnlabel,
					Blob x)
					throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateBlob(String columnlabel,
					InputStream inputStream)
					throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateBlob(String columnlabel,
					InputStream inputStream,
					long length)
					throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateBoolean(int columnindex,
						boolean x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateBoolean(String columnlabel,
						boolean x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateByte(int columnindex,
						byte x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateByte(String columnlabel,
						byte x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateBytes(int columnindex,
						byte[] x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateBytes(String columnlabel,
						byte[] x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateCharacterStream(int columnindex,
						Reader x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateCharacterStream(int columnindex,
							Reader x,
							int length)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateCharacterStream(int columnindex,
							Reader x,
							long length)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateCharacterStream(String columnlabel,
							Reader reader)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateCharacterStream(String columnlabel,
							Reader reader,
							int length)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateCharacterStream(String columnlabel,
							Reader reader,
							long length)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateClob(int columnindex,
					Clob x)
					throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateClob(int columnindex,
					Reader reader)
					throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateClob(int columnindex,
					Reader reader,
					long length)
					throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateClob(String columnlabel,
						Clob x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateClob(String columnlabel,
						Reader reader)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateClob(String columnlabel,
						Reader reader,
						long length)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateDate(int columnindex, Date x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateDate(String columnlabel, Date x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateDouble(int columnindex, double x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateDouble(String columnlabel, double x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateFloat(int columnindex, float x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateFloat(String columnlabel, float x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateInt(int columnindex, int x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateInt(String columnlabel, int x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateLong(int columnindex, long x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateLong(String columnlabel, long x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateNCharacterStream(int columnindex,
							Reader x)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateNCharacterStream(int columnindex,
							Reader x,
							long length)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateNCharacterStream(String columnlabel,
							Reader reader)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateNCharacterStream(String columnlabel,
							Reader reader,
							long length)
							throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateNClob(int columnindex,
						NClob nClob)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateNClob(int columnindex,
						Reader reader)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateNClob(int columnindex,
						Reader reader,
						long length)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateNClob(String columnlabel,
						NClob nClob)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateNClob(String columnlabel,
						Reader reader)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateNClob(String columnlabel,
						Reader reader,
						long length)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateNString(int columnindex,
						String nString)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateNString(String columnlabel,
						String nString)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateNull(int columnindex) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateNull(String columnlabel) throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateObject(int columnindex,
						Object x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateObject(int columnindex,
						Object x,
						int scaleOrLength)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateObject(String columnlabel,
						Object x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateObject(String columnlabel,
						Object x,
						int scaleOrLength)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateRef(int columnindex, Ref x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateRef(String columnlabel, Ref x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateRow() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateRowId(int columnindex, RowId x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateRowId(String columnlabel, RowId x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateShort(int columnindex, short x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateShort(String columnlabel, short x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateSQLXML(int columnindex,
						SQLXML xmlObject)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateSQLXML(String columnlabel,
						SQLXML xmlObject)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateString(int columnindex,
						String x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateString(String columnlabel,
						String x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateTime(int columnindex,
						Time x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateTime(String columnlabel,
						Time x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateTimestamp(int columnindex,
						Timestamp x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public void	updateTimestamp(String columnlabel,
						Timestamp x)
						throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		throwFeatureNotSupportedException();
		driver.debugEnd();
	}

	public boolean	wasNull() throws SQLException {
		driver.debugFunction();
		throwExceptionIfClosed();
		driver.debugEnd();
		return wasnull;
	}

	public boolean	isWrapperFor(Class<?> iface) throws SQLException {
		driver.debugFunction();
		driver.debugEnd();
		return (iface==SQLRCursor.class);
	}

	@SuppressWarnings({"unchecked"})
	public <T> T	unwrap(Class<T> iface) throws SQLException {
		driver.debugFunction();
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
