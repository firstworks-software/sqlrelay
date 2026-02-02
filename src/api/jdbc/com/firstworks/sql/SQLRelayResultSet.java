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
import javax.sql.rowset.serial.SerialBlob;
import javax.sql.rowset.serial.SerialClob;
import java.nio.charset.StandardCharsets;

import com.firstworks.sqlrelay.*;

public class SQLRelayResultSet implements ResultSet {

	private Object			networklock;

	private SQLRelayDriver		drv;
	private SQLRelayConnection	conn;
	private SQLRelayStatement	stmt;

	private SQLRCursor		sqlrcur;

	private long		currentrow;
	private	boolean		beforefirst;
	private	boolean		islast;
	private	boolean		afterlast;
	private	int		fetchdirection;
	private boolean		wasnull;


	public
	SQLRelayResultSet(SQLRelayDriver driver) {
		this.drv=driver;
		drv.debugFunction(this);
		reset();
		drv.debugEnd();
	}

	private
	void reset() {
		drv.debugFunction(this);
		networklock=null;
		stmt=null;
		conn=null;
		sqlrcur=null;
		currentrow=0;
		beforefirst=true;
		islast=false;
		afterlast=false;
		fetchdirection=ResultSet.FETCH_FORWARD;
		wasnull=false;
		drv.debugEnd();
	}

	public
	void setNetworkLock(Object networklock) {
		this.networklock=networklock;
	}

	public
	void setStatement(SQLRelayStatement stmt) {
		this.stmt=stmt;
	}

	public
	void setConnection(SQLRelayConnection connection) {
		this.conn=connection;
	}

	public
	void setSQLRCursor(SQLRCursor sqlrcur) {
		this.sqlrcur=sqlrcur;
	}

	public
	SQLRCursor getSQLRCursor() {
		return sqlrcur;
	}

	public
	boolean absolute(int row) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("row: "+row);
		if (row<currentrow) {
			conn.throwException(
				"FIXME: ResultSet type is Forward-Only");
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
			synchronized (networklock) {
				sqlrcur.getField(currentrow-1,0);
			}
			long	rowcount=sqlrcur.rowCount();
			if (sqlrcur.endOfResultSet()) {
				if (currentrow-1==rowcount-1) {
					islast=true;
					afterlast=false;
				} else if (currentrow-1>=rowcount) {
					islast=false;
					afterlast=true;
					drv.debugPrintln("after last");
					drv.debugEnd();
					return false;
				}
			}
		} else if (row<0) {
			// FIXME: implement this...
			// position relative to end of result set
			conn.throwException(
				"FIXME: negative row not supported");
		}
		drv.debugPrintln("success");
		drv.debugEnd();
		return true;
	}

	public
	void afterLast() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		afterlast=true;
		drv.debugEnd();
	}

	public
	void beforeFirst() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		beforefirst=true;
		drv.debugEnd();
	}

	public
	void cancelRowUpdates() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void clearWarnings() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugEnd();
	}

	public
	void close() throws SQLException {
		drv.debugFunction(this);
		if (sqlrcur!=null) {
			synchronized (networklock) {
				sqlrcur.closeResultSet();
			}
		}
		reset();
		drv.debugEnd();
	}

	public
	void deleteRow() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	int findColumn(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		for (int i=0; i<sqlrcur.colCount(); i++) {
			if (sqlrcur.getColumnName(i).equals(columnlabel)) {
				drv.debugPrintln("column: "+(i+1));
				drv.debugEnd();
				return i+1;
			}
		}
		conn.throwException("Column not found");
		return 0;
	}

	public
	boolean first() throws SQLException {
		drv.debugFunction(this);
		boolean	abs=absolute(1);
		drv.debugEnd();
		return abs;
	}

	public
	Array getArray(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	Array getArray(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return null;
	}

	public
	InputStream getAsciiStream(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnindex-1);
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (wasnull)?null:new StringBufferInputStream(field);
	}

	public
	InputStream getAsciiStream(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnlabel);
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (wasnull)?null:new StringBufferInputStream(field);
	}

	public
	BigDecimal getBigDecimal(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnindex-1);
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (wasnull)?null:new BigDecimal(field);
	}

	public
	BigDecimal getBigDecimal(int columnindex, int scale)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnindex-1);
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (wasnull)?null:new BigDecimal(
					new BigInteger(field.replace("\\.","")),
					scale);
	}

	public
	BigDecimal getBigDecimal(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnlabel);
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (wasnull)?null:new BigDecimal(field);
	}

	public
	BigDecimal getBigDecimal(String columnlabel, int scale)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnlabel);
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (wasnull)?null:new BigDecimal(
					new BigInteger(field.replace("\\.","")),
					scale);
	}

	public
	InputStream getBinaryStream(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		byte[]	field=null;
		synchronized (networklock) {
			field=sqlrcur.getFieldAsByteArray(
					currentrow-1,columnindex-1);
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (wasnull)?null:new ByteArrayInputStream(field);
	}

	public
	InputStream getBinaryStream(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		byte[]	field=null;
		synchronized (networklock) {
			field=sqlrcur.getFieldAsByteArray(
					currentrow-1,columnlabel);
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (wasnull)?null:new ByteArrayInputStream(field);
	}

	public
	Blob getBlob(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		byte[]	field=null;
		synchronized (networklock) {
			field=sqlrcur.getFieldAsByteArray(
					currentrow-1,columnindex-1);
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (wasnull)?null:new SerialBlob(field);
	}

	public
	Blob getBlob(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		byte[]	field=null;
		synchronized (networklock) {
			field=sqlrcur.getFieldAsByteArray(
					currentrow-1,columnlabel);
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (wasnull)?null:new SerialBlob(field);
	}

	public
	boolean getBoolean(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnindex-1);
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		boolean	eq=field.equals("1");
		drv.debugEnd();
		return eq;
	}

	public
	boolean getBoolean(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnlabel);
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		boolean	eq=field.equals("1");
		drv.debugEnd();
		return eq;
	}

	public
	byte getByte(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		long	field=0;
		synchronized (networklock) {
			field=sqlrcur.getFieldAsInteger(
					currentrow-1,columnindex-1);
			wasnull=(sqlrcur.getField(
					currentrow-1,columnindex-1)==null);
		}
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (byte)field;
	}

	public
	byte getByte(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		long	field=0;
		synchronized (networklock) {
			field=sqlrcur.getFieldAsInteger(
					currentrow-1,columnlabel);
			wasnull=(sqlrcur.getField(
					currentrow-1,columnlabel)==null);
		}
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (byte)field;
	}

	public
	byte[] getBytes(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		byte[]	field=null;
		synchronized (networklock) {
			field=sqlrcur.getFieldAsByteArray(
					currentrow-1,columnindex-1);
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return field;
	}

	public
	byte[] getBytes(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		byte[]	field=null;
		synchronized (networklock) {
			field=sqlrcur.getFieldAsByteArray(
					currentrow-1,columnlabel);
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return field;
	}

	public
	Reader getCharacterStream(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnindex-1);
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (wasnull)?null:(new StringReader(field));
	}

	public
	Reader getCharacterStream(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnlabel);
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (wasnull)?null:(new StringReader(field));
	}

	public
	Clob getClob(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		char[]	field=null;
		synchronized (networklock) {
			byte[]	val=sqlrcur.getFieldAsByteArray(
						currentrow-1,columnindex-1);
			if (val!=null) {
				field=(new String(val,StandardCharsets.UTF_8)).
								toCharArray();
			}
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (wasnull)?null:new SerialClob(field);
	}

	public
	Clob getClob(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		char[]	field=null;
		synchronized (networklock) {
			byte[]	val=sqlrcur.getFieldAsByteArray(
						currentrow-1,columnlabel);
			if (val!=null) {
				field=(new String(val,StandardCharsets.UTF_8)).
								toCharArray();
			}
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (wasnull)?null:new SerialClob(field);
	}

	public
	int getConcurrency() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		int	concurrency=ResultSet.CONCUR_READ_ONLY;
		drv.debugPrintln("concurrency: "+concurrency);
		drv.debugEnd();
		return concurrency;
	}

	public
	String getCursorName() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		String	cursorname=null;
		drv.debugPrintln("cursor name: "+cursorname);
		drv.debugEnd();
		return cursorname;
	}

	public
	Date getDate(int columnindex) throws SQLException {
		drv.debugFunction(this);
		// FIXME: pass in some default calendar
		Date	dt=getDate(columnindex,null);
		drv.debugEnd();
		return dt;
	}

	public
	Date getDate(int columnindex, Calendar cal) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnindex-1);
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		// FIXME: use cal
		// FIXME: field isn't guaranteed to be in iso format
		drv.debugEnd();
		return (wasnull)?null:Date.valueOf(field);
	}

	public
	Date getDate(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		// FIXME: pass in some default calendar
		Date	dt=getDate(columnlabel,null);
		drv.debugEnd();
		return dt;
	}

	public
	Date getDate(String columnlabel, Calendar cal) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnlabel);
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		// FIXME: use cal
		// FIXME: field isn't guaranteed to be in iso format
		drv.debugEnd();
		return (wasnull)?null:Date.valueOf(field);
	}

	public
	double getDouble(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		double	field=0.0;
		synchronized (networklock) {
			field=sqlrcur.getFieldAsDouble(
					currentrow-1,columnindex-1);
			wasnull=(sqlrcur.getField(
					currentrow-1,columnindex-1)==null);
		}
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return field;
	}

	public
	double getDouble(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		double	field=0.0;
		synchronized (networklock) {
			field=sqlrcur.getFieldAsDouble(
					currentrow-1,columnlabel);
			wasnull=(sqlrcur.getField(
					currentrow-1,columnlabel)==null);
		}
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return field;
	}

	public
	int getFetchDirection() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		int	direction=ResultSet.FETCH_FORWARD;
		drv.debugPrintln("direction: "+direction);
		drv.debugEnd();
		return direction;
	}

	public
	int getFetchSize() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		int	size=(int)sqlrcur.getResultSetBufferSize();
		drv.debugPrintln("size: "+size);
		drv.debugEnd();
		return size;
	}

	public
	float getFloat(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		float	field=0.0f;
		synchronized (networklock) {
			field=(float)sqlrcur.getFieldAsDouble(
					currentrow-1,columnindex-1);
			wasnull=(sqlrcur.getField(
					currentrow-1,columnindex-1)==null);
		}
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return field;
	}

	public
	float getFloat(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		float	field=0.0f;
		synchronized (networklock) {
			field=(float)sqlrcur.getFieldAsDouble(
					currentrow-1,columnlabel);
			wasnull=(sqlrcur.getField(
					currentrow-1,columnlabel)==null);
		}
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return field;
	}

	public
	int getHoldability() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		// FIXME: is this correct?
		int	holdability=ResultSet.CLOSE_CURSORS_AT_COMMIT;
		drv.debugPrintln("holdability: "+holdability);
		drv.debugEnd();
		return holdability;
	}

	public
	int getInt(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		int	field=0;
		synchronized (networklock) {
			field=(int)sqlrcur.getFieldAsInteger(
					currentrow-1,columnindex-1);
			wasnull=(sqlrcur.getField(
					currentrow-1,columnindex-1)==null);
		}
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return field;
	}

	public
	int getInt(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		int	field=0;
		synchronized (networklock) {
			field=(int)sqlrcur.getFieldAsInteger(
					currentrow-1,columnlabel);
			wasnull=(sqlrcur.getField(
					currentrow-1,columnlabel)==null);
		}
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return field;
	}

	public
	long getLong(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		long	field=0;
		synchronized (networklock) {
			field=sqlrcur.getFieldAsInteger(
					currentrow-1,columnindex-1);
			wasnull=(sqlrcur.getField(
					currentrow-1,columnindex-1)==null);
		}
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return field;
	}

	public
	long getLong(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		long	field=0;
		synchronized (networklock) {
			field=sqlrcur.getFieldAsInteger(
					currentrow-1,columnlabel);
			wasnull=(sqlrcur.getField(
					currentrow-1,columnlabel)==null);
		}
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return field;
	}

	public
	ResultSetMetaData getMetaData() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		SQLRelayResultSetMetaData	metadata=
				new SQLRelayResultSetMetaData(drv);
		metadata.setSQLRCursor(sqlrcur);
		metadata.setResultSet(this);
		drv.debugEnd();
		return metadata;
	}

	public
	Reader getNCharacterStream(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		byte[]	field=null;
		synchronized (networklock) {
			field=sqlrcur.getFieldAsByteArray(
						currentrow-1,columnindex-1);
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (wasnull)?null:new InputStreamReader(
					new ByteArrayInputStream(field));
	}

	public
	Reader getNCharacterStream(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		byte[]	field=null;
		synchronized (networklock) {
			field=sqlrcur.getFieldAsByteArray(
						currentrow-1,columnlabel);
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (wasnull)?null:new InputStreamReader(
					new ByteArrayInputStream(field));
	}

	public
	NClob getNClob(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		conn.throwFeatureNotSupportedException();
		// FIXME: we could theoretically support this, but currently
		// SQLRelayResultSetMetaData.getColumnType/getColumnClassName
		// don't return any lob types, so it's not currently necessary
		drv.debugEnd();
		return null;
	}

	public
	NClob getNClob(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		conn.throwFeatureNotSupportedException();
		// FIXME: we could theoretically support this, but currently
		// SQLRelayResultSetMetaData.getColumnType/getColumnClassName
		// don't return any lob types, so it's not currently necessary
		drv.debugEnd();
		return null;
	}

	public
	String getNString(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		byte[]	field=null;
		synchronized (networklock) {
			field=sqlrcur.getFieldAsByteArray(
						currentrow-1,columnindex-1);
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		String	str=null;
		try {
			if (!wasnull) {
				str=new String(field,"UTF-8");
			}
		} catch (Exception ex) {
			conn.throwException(ex.getMessage());
		}
		drv.debugEnd();
		return str;
	}

	public
	String getNString(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		byte[]	field=null;
		synchronized (networklock) {
			field=sqlrcur.getFieldAsByteArray(
						currentrow-1,columnlabel);
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		String	str=null;
		try {
			if (!wasnull) {
				str=new String(field,"UTF-8");
			}
		} catch (Exception ex) {
			conn.throwException(ex.getMessage());
		}
		drv.debugEnd();
		return str;
	}

	public
	Object getObject(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		conn.throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		drv.debugEnd();
		return null;
	}

	public
	<T> T getObject(int columnindex, Class<T> type) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		conn.throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		return null;
	}

	public
	Object getObject(int columnindex, Map<String,Class<?>> map)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		conn.throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		drv.debugEnd();
		return null;
	}

	public
	Object getObject(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		conn.throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		drv.debugEnd();
		return null;
	}

	public
	<T> T getObject(String columnlabel, Class<T> type) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		conn.throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		drv.debugEnd();
		return null;
	}

	public
	Object getObject(String columnlabel, Map<String,Class<?>> map)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		conn.throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		drv.debugEnd();
		return null;
	}

	public
	Ref getRef(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		conn.throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		drv.debugEnd();
		return null;
	}

	public
	Ref getRef(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		conn.throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		drv.debugEnd();
		return null;
	}

	public
	int getRow() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("current row: "+currentrow);
		drv.debugEnd();
		return (int)currentrow;
	}

	public
	RowId getRowId(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		conn.throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		drv.debugEnd();
		return null;
	}

	public
	RowId getRowId(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		conn.throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		drv.debugEnd();
		return null;
	}

	public
	short getShort(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		short	field=0;
		synchronized (networklock) {
			field=(short)sqlrcur.getFieldAsInteger(
					currentrow-1,columnindex-1);
			wasnull=(sqlrcur.getField(
					currentrow-1,columnindex-1)==null);
		}
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return field;
	}

	public
	short getShort(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		short	field=0;
		synchronized (networklock) {
			field=(short)sqlrcur.getFieldAsInteger(
					currentrow-1,columnlabel);
			wasnull=(sqlrcur.getField(
					currentrow-1,columnlabel)==null);
		}
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return field;
	}

	public
	SQLXML getSQLXML(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		conn.throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		drv.debugEnd();
		return null;
	}

	public
	SQLXML getSQLXML(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		conn.throwFeatureNotSupportedException();
		// FIXME: we might be able to support this somehow...
		drv.debugEnd();
		return null;
	}

	public
	SQLRelayStatement getStatement() throws SQLException {
		//drv.debugFunction(this);
		//drv.debugEnd();
		return stmt;
	}

	public
	String getString(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnindex-1);
		}
		wasnull=(field==null);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return field;
	}

	public
	String getString(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnlabel);
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return field;
	}

	public
	Time getTime(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		// FIXME: pass in some default calendar
		Time	t=getTime(columnindex,null);
		drv.debugEnd();
		return t;
	}

	public
	Time getTime(int columnindex, Calendar cal) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnindex-1);
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		// FIXME: use cal
		// FIXME: not guaranteed to be in iso format
		drv.debugEnd();
		return (wasnull)?null:Time.valueOf(field);
	}

	public
	Time getTime(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		// FIXME: pass in some default calendar
		Time	t=getTime(columnlabel,null);
		drv.debugEnd();
		return t;
	}

	public
	Time getTime(String columnlabel, Calendar cal) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnlabel);
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		// FIXME: use cal
		// FIXME: Time.valueOf() expects hh:mm:ss format
		drv.debugEnd();
		return (wasnull)?null:Time.valueOf(field);
	}

	public
	Timestamp getTimestamp(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		// FIXME: pass in some default calendar
		Timestamp	t=getTimestamp(columnindex,null);
		drv.debugEnd();
		return t;
	}

	public
	Timestamp getTimestamp(int columnindex, Calendar cal)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnindex-1);
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		// FIXME: use cal
		// FIXME: Timestamp.valueOf() expects iso format
		// FIXME: postgresql ends with timezone offset (eg. -04)
		drv.debugEnd();
		return (wasnull)?null:Timestamp.valueOf(field);
	}

	public
	Timestamp getTimestamp(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		// FIXME: pass in some default calendar
		Timestamp	t=getTimestamp(columnlabel,null);
		drv.debugEnd();
		return t;
	}

	public
	Timestamp getTimestamp(String columnlabel, Calendar cal)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnlabel);
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		// FIXME: use cal
		// FIXME: Timestamp.valueOf() expects iso format
		// FIXME: postgresql ends with timezone offset (eg. -04)
		drv.debugEnd();
		return (wasnull)?null:Timestamp.valueOf(field);
	}

	public
	int getType() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		int	type=ResultSet.TYPE_FORWARD_ONLY;
		drv.debugPrintln("type: "+type);
		drv.debugEnd();
		return type;
	}

	public
	InputStream getUnicodeStream(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnindex-1);
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		// FIXME: I think StringBufferInputStream only produces ASCII
		return (wasnull)?null:new StringBufferInputStream(field);
	}

	public
	InputStream getUnicodeStream(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnlabel);
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		// FIXME: I think StringBufferInputStream only produces ASCII
		return (wasnull)?null:new StringBufferInputStream(field);
	}

	public
	URL getURL(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		conn.throwFeatureNotSupportedException();
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnindex-1);
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		URL	u=null;
		if (!wasnull) {
			try {
				u=new URL(field);
			} catch (MalformedURLException ex) {
				u=null;
			}
		}
		drv.debugEnd();
		return u;
	}

	public
	URL getURL(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		conn.throwFeatureNotSupportedException();
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnlabel);
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		URL	u=null;
		if (!wasnull) {
			try {
				u=new URL(field);
			} catch (MalformedURLException ex) {
				u=null;
			}
		}
		drv.debugEnd();
		return u;
	}

	public
	SQLWarning getWarnings() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugEnd();
		return null;
	}

	public
	void insertRow() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	boolean isAfterLast() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("after last: "+afterlast);
		drv.debugEnd();
		return afterlast;
	}

	public
	boolean isBeforeFirst() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("before first: "+beforefirst);
		drv.debugEnd();
		return beforefirst;
	}

	public
	boolean isClosed() throws SQLException {
		drv.debugFunction(this);
		boolean	isclosed=(sqlrcur==null);
		drv.debugPrintln("is closed: "+isclosed);
		drv.debugEnd();
		return isclosed;
	}

	public
	boolean isFirst() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		boolean	isfirst=(currentrow==0);
		drv.debugPrintln("is first: "+isfirst);
		drv.debugEnd();
		return isfirst;
	}

	public
	boolean isLast() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("is last: "+islast);
		drv.debugEnd();
		return islast;
	}

	public
	boolean last() throws SQLException {
		drv.debugFunction(this);
		boolean	abs=absolute(-1);
		drv.debugEnd();
		return abs;
	}

	public
	void moveToCurrentRow() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		// since we don't support updating result sets, then we can't
		// be on the insert row, and we're always on the current row
		drv.debugEnd();
	}

	public
	void moveToInsertRow() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	boolean next() throws SQLException {
		drv.debugFunction(this);
		boolean	rel=relative(1);
		drv.debugEnd();
		return rel;
	}

	public
	boolean previous() throws SQLException {
		drv.debugFunction(this);
		boolean	rel=relative(-1);
		drv.debugEnd();
		return rel;
	}

	public
	void refreshRow() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	boolean relative(int rows) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("rows: "+rows);
		if (rows==0) {
			drv.debugEnd();
			return true;
		}
		int	newrow=(int)(currentrow+rows);
		drv.debugPrintln("newrow (before): "+newrow);
		if (newrow<1) {
			newrow=1;
		}
		drv.debugPrintln("newrow (after): "+newrow);
		boolean	abs=absolute(newrow);
		drv.debugEnd();
		return abs;
	}

	public
	boolean rowDeleted() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return false;
	}

	public
	boolean rowInserted() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return false;
	}

	public
	boolean rowUpdated() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
		return false;
	}

	public
	void setFetchDirection(int direction) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("direction: "+direction);
		fetchdirection=direction;
		drv.debugEnd();
	}

	public
	void setFetchSize(int rows) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("fetch size: "+rows);
		sqlrcur.setResultSetBufferSize(rows);
		drv.debugEnd();
	}

	public
	void updateArray(int columnindex, Array x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateArray(String columnlabel, Array x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateAsciiStream(int columnindex, InputStream x)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateAsciiStream(int columnindex, InputStream x,
					int length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateAsciiStream(int columnindex, InputStream x,
					long length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateAsciiStream(String columnlabel, InputStream x)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateAsciiStream(String columnlabel,InputStream x,
					int length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateAsciiStream(String columnlabel, InputStream x,
					long length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateBigDecimal(int columnindex, BigDecimal x)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateBigDecimal(String columnlabel, BigDecimal x)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateBinaryStream(int columnindex, InputStream x)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateBinaryStream(int columnindex, InputStream x,
					int length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateBinaryStream(int columnindex, InputStream x,
					long length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateBinaryStream(String columnlabel, InputStream x)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateBinaryStream(String columnlabel, InputStream x,
					int length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateBinaryStream(String columnlabel, InputStream x,
					long length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateBlob(int columnindex, Blob x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateBlob(int columnindex, InputStream inputStream)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateBlob(int columnindex, InputStream inputStream,
					long length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateBlob(String columnlabel, Blob x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateBlob(String columnlabel, InputStream inputStream)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateBlob(String columnlabel, InputStream inputStream,
					long length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateBoolean(int columnindex, boolean x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateBoolean(String columnlabel, boolean x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateByte(int columnindex, byte x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateByte(String columnlabel, byte x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateBytes(int columnindex, byte[] x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateBytes(String columnlabel, byte[] x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateCharacterStream(int columnindex, Reader x)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateCharacterStream(int columnindex, Reader x,
					int length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateCharacterStream(int columnindex, Reader x,
					long length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateCharacterStream(String columnlabel, Reader reader)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateCharacterStream(String columnlabel, Reader reader,
					int length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateCharacterStream(String columnlabel, Reader reader,
					long length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateClob(int columnindex, Clob x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateClob(int columnindex, Reader reader) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateClob(int columnindex, Reader reader,
					long length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateClob(String columnlabel, Clob x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateClob(String columnlabel, Reader reader) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateClob(String columnlabel, Reader reader,
					long length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateDate(int columnindex, Date x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateDate(String columnlabel, Date x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateDouble(int columnindex, double x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateDouble(String columnlabel, double x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateFloat(int columnindex, float x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateFloat(String columnlabel, float x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateInt(int columnindex, int x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateInt(String columnlabel, int x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateLong(int columnindex, long x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateLong(String columnlabel, long x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateNCharacterStream(int columnindex, Reader x)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateNCharacterStream(int columnindex, Reader x,
					long length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateNCharacterStream(String columnlabel, Reader reader)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateNCharacterStream(String columnlabel, Reader reader,
					long length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateNClob(int columnindex, NClob nClob) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateNClob(int columnindex, Reader reader) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateNClob(int columnindex, Reader reader,
					long length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateNClob(String columnlabel, NClob nClob) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateNClob(String columnlabel, Reader reader)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateNClob(String columnlabel, Reader reader,
					long length) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateNString(int columnindex, String nString)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateNString(String columnlabel, String nString)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateNull(int columnindex) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateNull(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateObject(int columnindex, Object x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateObject(int columnindex, Object x,
					int scaleOrLength) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateObject(String columnlabel, Object x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateObject(String columnlabel, Object x,
					int scaleOrLength) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateRef(int columnindex, Ref x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateRef(String columnlabel, Ref x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateRow() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateRowId(int columnindex, RowId x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateRowId(String columnlabel, RowId x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateShort(int columnindex, short x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateShort(String columnlabel, short x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateSQLXML(int columnindex, SQLXML xmlObject)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateSQLXML(String columnlabel, SQLXML xmlObject)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateString(int columnindex, String x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateString(String columnlabel, String x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateTime(int columnindex, Time x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateTime(String columnlabel, Time x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateTimestamp(int columnindex, Timestamp x) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column index: "+columnindex);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	void updateTimestamp(String columnlabel, Timestamp x)
						throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		conn.throwFeatureNotSupportedException();
		drv.debugEnd();
	}

	public
	boolean wasNull() throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return wasnull;
	}

	public
	boolean isWrapperFor(Class<?> iface) throws SQLException {
		drv.debugFunction(this);
		drv.debugEnd();
		return (iface==SQLRCursor.class);
	}

	@SuppressWarnings({"unchecked"})
	public
	<T> T unwrap(Class<T> iface) throws SQLException {
		drv.debugFunction(this);
		drv.debugEnd();
		return (T)((iface==SQLRCursor.class)?sqlrcur:null);
	}

	private
	void validateColumn(int columnindex) throws SQLException {
		if (columnindex<1 || columnindex>sqlrcur.colCount()) {
			conn.throwException("invalid column index");
		}
	}

	private
	void validateColumn(String columnlabel) throws SQLException {
		String[] cols=sqlrcur.getColumnNames();
		for (int i=0; i<cols.length; i++) {
			if (cols[i].equals(columnlabel)) {
				return;
			}
		}
		conn.throwException("invalid column label");
	}

	private
	void throwExceptionIfClosed() throws SQLException {
		if (sqlrcur==null) {
			conn.throwException("ResultSet is closed");
		}
	}
}
