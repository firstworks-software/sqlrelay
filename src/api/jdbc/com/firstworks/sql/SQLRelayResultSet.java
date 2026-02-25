package com.firstworks.sql;

import java.sql.*;
import javax.sql.rowset.serial.SerialBlob;
import javax.sql.rowset.serial.SerialClob;

import java.io.InputStream;
import java.io.Reader;
import java.io.ByteArrayInputStream;
import java.io.StringBufferInputStream;
import java.io.StringReader;
import java.io.InputStreamReader;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Map;
import java.util.GregorianCalendar;
import java.net.URL;
import java.net.MalformedURLException;
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

	private	int		maxfieldsize;
	private	int		maxrows;


	public
	SQLRelayResultSet(SQLRelayDriver driver) {
		this.drv=driver;
		drv.debugFunction(this);
		reset();
		maxfieldsize=0;
		maxrows=0;
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

	void setNetworkLock(Object networklock) {
		this.networklock=networklock;
	}

	void setStatement(SQLRelayStatement stmt) {
		this.stmt=stmt;
	}

	void setConnection(SQLRelayConnection connection) {
		this.conn=connection;
	}

	void setSQLRCursor(SQLRCursor sqlrcur) {
		this.sqlrcur=sqlrcur;
	}

	SQLRCursor getSQLRCursor() {
		return sqlrcur;
	}

	void setMaxFieldSize(int maxfieldsize) {
		this.maxfieldsize=maxfieldsize;
	}

	void setMaxRows(int maxrows) {
		this.maxrows=maxrows;
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
			// FIXME: handle maxrows
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

	private
	boolean isCharBinaryCol(int columnindex) {
		return isCharBinaryType(
			sqlrcur.getColumnType(columnindex-1).toUpperCase());
	}

	private
	boolean isCharBinaryCol(String columnlabel) {
		return isCharBinaryType(
			sqlrcur.getColumnType(columnlabel).toUpperCase());
	}

	private
	boolean isCharBinaryType(String type) {
		switch (type) {
			case "CHAR":
			case "_CHAR":
			case "LONGCHAR":
			case "VARCHAR":
			case "_VARCHAR":
			case "VARCHAR2":
			case "LONGVARCHAR":
			case "BINARY":
			case "LONGBINARY":
			case "VARBINARY":
			case "LONGVARBINARY":
			case "STRING":
			case "VARSTRING":
			case "IMAGE":
			case "GRAPHIC":
			case "VARGRAPHIC":
			case "LONGVARGRAPHIC":
			case "NCHAR":
			case "NVARCHAR":
			case "LONGNVARCHAR":
				return true;
			default:
				return false;
		}
	}

	private
	String truncateField(boolean ischarbinary, String field)
							throws SQLException {
		if (maxfieldsize==0 || !ischarbinary || field==null) {
			return field;
		}
		return truncateField(field);
	}

	private
	String truncateField(String field) throws SQLException {
		// we need to truncate "field" to maxfieldsize bytes, not
		// characters, so we have to convert to byte[]s, truncate it,
		// and convert it back
		return new String(
			truncateField(field.getBytes(StandardCharsets.UTF_8)),
			StandardCharsets.UTF_8);
	}

	private
	char[] truncateField(boolean ischarbinary, char[] field)
							throws SQLException {
		if (maxfieldsize==0 || !ischarbinary || field==null) {
			return field;
		}
		return truncateField(field);
	}

	private
	char[] truncateField(char[] field) throws SQLException {
		// we need to truncate "field" to maxfieldsize bytes, not
		// characters, so we have to convert to byte[]s, truncate it,
		// and convert it back
		return (new String(
			truncateField(
				new String(field).getBytes(
					StandardCharsets.UTF_8)),
			StandardCharsets.UTF_8)).toCharArray();
	}

	private
	byte[] truncateField(boolean ischarbinary, byte[] field)
							throws SQLException {
		if (maxfieldsize==0 || !ischarbinary || field==null) {
			return field;
		}
		return truncateField(field);
	}

	private
	byte[] truncateField(byte[] field) throws SQLException {
		return Arrays.copyOf(field,maxfieldsize);
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
		field=truncateField(isCharBinaryCol(columnindex),field);
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
		field=truncateField(isCharBinaryCol(columnlabel),field);
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
		field=truncateField(isCharBinaryCol(columnindex),field);
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
		field=truncateField(isCharBinaryCol(columnlabel),field);
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
		Blob	b=null;
		if (!wasnull) {
			b=conn.createBlob();
			if (b!=null) {
				b.setBytes(1,field);
			}
		}
		drv.debugEnd();
		return b;
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
		Blob	b=null;
		if (!wasnull) {
			b=conn.createBlob();
			if (b!=null) {
				b.setBytes(1,field);
			}
		}
		drv.debugEnd();
		return b;
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
		field=truncateField(isCharBinaryCol(columnindex),field);
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
		field=truncateField(isCharBinaryCol(columnlabel),field);
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
		field=truncateField(isCharBinaryCol(columnindex),field);
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
		field=truncateField(isCharBinaryCol(columnlabel),field);
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
		byte[]	val=null;
		synchronized (networklock) {
			val=sqlrcur.getFieldAsByteArray(
					currentrow-1,columnindex-1);
		}
		char[]	field=null;
		if (val!=null) {
			field=(new String(val,StandardCharsets.UTF_8)).
							toCharArray();
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		Clob	c=null;
		if (!wasnull) {
			c=conn.createClob();
			if (c!=null) {
				c.setString(1,new String(field));
			}
		}
		drv.debugEnd();
		return c;
	}

	public
	Clob getClob(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		byte[]	val=null;
		synchronized (networklock) {
			val=sqlrcur.getFieldAsByteArray(
					currentrow-1,columnlabel);
		}
		char[]	field=null;
		if (val!=null) {
			field=(new String(val,StandardCharsets.UTF_8)).
							toCharArray();
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		Clob	c=null;
		if (!wasnull) {
			c=conn.createClob();
			if (c!=null) {
				c.setString(1,new String(field));
			}
		}
		drv.debugEnd();
		return c;
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
		String	cursorname=(stmt!=null)?stmt.getCursorName():null;
		drv.debugPrintln("cursor name: "+cursorname);
		drv.debugEnd();
		return cursorname;
	}

	public
	Date getDate(int columnindex) throws SQLException {
		drv.debugFunction(this);
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
		Date	dt=(wasnull)?null:
			createDate(currentrow-1,columnindex-1,cal);
		drv.debugEnd();
		return dt;
	}

	private
	Date createDate(long row, int col, Calendar cal) {
		drv.debugFunction(this);
		drv.debugPrintln("row: "+row);
		drv.debugPrintln("col: "+col);
		drv.debugPrintln("cal: "+cal);
		short	year;
		short	month;
		short	day;
		synchronized (networklock) {
			year=sqlrcur.getFieldAsDateYear(row,col);
			month=sqlrcur.getFieldAsDateMonth(row,col);
			day=sqlrcur.getFieldAsDateDay(row,col);
		}
		drv.debugPrintln("year: "+year);
		drv.debugPrintln("month: "+month);
		drv.debugPrintln("day: "+day);
		Date	dt=createDate(year,month,day,cal);
		drv.debugEnd();
		return dt;
	}

	private
	Date createDate(long row, String col, Calendar cal) {
		drv.debugFunction(this);
		drv.debugPrintln("row: "+row);
		drv.debugPrintln("col: "+col);
		drv.debugPrintln("cal: "+cal);
		short	year;
		short	month;
		short	day;
		synchronized (networklock) {
			year=sqlrcur.getFieldAsDateYear(row,col);
			month=sqlrcur.getFieldAsDateMonth(row,col);
			day=sqlrcur.getFieldAsDateDay(row,col);
		}
		drv.debugPrintln("year: "+year);
		drv.debugPrintln("month: "+month);
		drv.debugPrintln("day: "+day);
		Date	dt=createDate(year,month,day,cal);
		drv.debugEnd();
		return dt;
	}

	private
	Date createDate(short year, short month, short day, Calendar cal) {
		drv.debugFunction(this);
		year=(year>=0)?year:0;
		month=(month>=0)?month:0;
		day=(day>=0)?day:0;
		Date	dt=null;
		if (cal!=null) {
			cal.set(Calendar.YEAR,year);
			cal.set(Calendar.MONTH,month-1);
			cal.set(Calendar.DAY_OF_MONTH,day);
			cal.set(Calendar.HOUR_OF_DAY,0);
			cal.set(Calendar.MINUTE,0);
			cal.set(Calendar.SECOND,0);
			cal.set(Calendar.MILLISECOND,0);
			dt=new Date(cal.getTimeInMillis());
		} else {
			dt=new Date(year-1900,month-1,day);
		}
		drv.debugEnd();
		return dt;
	}

	public
	Date getDate(String columnlabel) throws SQLException {
		drv.debugFunction(this);
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
		Date	dt=(wasnull)?null:
			createDate(currentrow-1,columnlabel,cal);
		drv.debugEnd();
		return dt;
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
		// SQL Relay only supports HOLD_CURSORS_OVER_COMMIT
		int	holdability=ResultSet.HOLD_CURSORS_OVER_COMMIT;
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
		field=truncateField(isCharBinaryCol(columnindex),field);
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
		field=truncateField(isCharBinaryCol(columnlabel),field);
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
		byte[]	val=null;
		synchronized (networklock) {
			val=sqlrcur.getFieldAsByteArray(
					currentrow-1,columnindex-1);
		}
		char[]	field=null;
		if (val!=null) {
			field=(new String(val,StandardCharsets.UTF_8)).
							toCharArray();
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		NClob	nc=null;
		if (!wasnull) {
			nc=conn.createNClob();
			if (nc!=null) {
				nc.setString(1,new String(field));
			}
		}
		drv.debugEnd();
		return nc;
	}

	public
	NClob getNClob(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		byte[]	val=null;
		synchronized (networklock) {
			val=sqlrcur.getFieldAsByteArray(
					currentrow-1,columnlabel);
		}
		char[]	field=null;
		if (val!=null) {
			field=(new String(val,StandardCharsets.UTF_8)).
							toCharArray();
		}
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		NClob	nc=null;
		if (!wasnull) {
			nc=conn.createNClob();
			if (nc!=null) {
				nc.setString(1,new String(field));
			}
		}
		drv.debugEnd();
		return nc;
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
		field=truncateField(isCharBinaryCol(columnindex),field);
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
		field=truncateField(isCharBinaryCol(columnlabel),field);
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
		Object	o=getObject(columnindex,conn.getTypeMap());
		drv.debugEnd();
		return o;
	}

	public
	Object getObject(int columnindex, Map<String,Class<?>> map)
							throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("column index: "+columnindex);
		validateColumn(columnindex);
		conn.throwFeatureNotSupportedException();
		// FIXME: we could support this if the c++ api had
		// getColumnDatabase() and getColumnSchema(), and if
		// getColumnTable() was exposed.  We could them combine them,
		// and return getObject(columnindex,map.get("..."));
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
		// FIXME: create an instance of "type", call its readSQL()
		// method on getField(currentrow-1,columnindex) and return it,
		// or return a String if "type" is null
		return null;
	}

	public
	Object getObject(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		Object	o=getObject(columnlabel,conn.getTypeMap());
		drv.debugEnd();
		return o;
	}

	public
	Object getObject(String columnlabel, Map<String,Class<?>> map)
							throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		conn.throwFeatureNotSupportedException();
		// FIXME: we could support this if the c++ api had
		// getColumnDatabase() and getColumnSchema(), and if
		// getColumnTable() was exposed.  We could them combine them,
		// and return getObject(columnindex,map.get("..."));
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
		// FIXME: create an instance of "type", call its readSQL()
		// method on getField(currentrow-1,columnlabel) and return it,
		// or return a String if "type" is null
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
		// FIXME: we need an SQLRelayRef implementation to support this,
		// but basically we'd just:
		// SQLRelayRef	r=new SQLRelayRef();
		// r.setObject(getObject(columnindex));
		// return r;
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
		// FIXME: we need an SQLRelayRef implementation to support this,
		// but basically we'd just:
		// SQLRelayRef	r=new SQLRelayRef();
		// r.setObject(getObject(columnindex));
		// return r;
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
		// FIXME: we need an SQLRelayRowId implementation
		// to support this, which needs to be able to store a String
		// and return it as a String or byte array (I think)
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
		// FIXME: we need an SQLRelayRowId implementation
		// to support this, which needs to be able to store a String
		// and return it as a String or byte array (I think)
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
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnindex-1);
		}
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		SQLXML	xml=null;
		if (!wasnull) {
			xml=conn.createSQLXML();
			if (xml!=null) {
				xml.setString(field);
			}
		}
		drv.debugEnd();
		return xml;
	}

	public
	SQLXML getSQLXML(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		throwExceptionIfClosed();
		drv.debugPrintln("column label: "+columnlabel);
		validateColumn(columnlabel);
		String	field=null;
		synchronized (networklock) {
			field=sqlrcur.getField(currentrow-1,columnlabel);
		}
		wasnull=(field==null);
		drv.debugPrintln("column label: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		SQLXML	xml=null;
		if (!wasnull) {
			xml=conn.createSQLXML();
			if (xml!=null) {
				xml.setString(field);
			}
		}
		drv.debugEnd();
		return xml;
	}

	public
	SQLRelayStatement getStatement() throws SQLException {
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
		field=truncateField(isCharBinaryCol(columnindex),field);
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
		field=truncateField(isCharBinaryCol(columnlabel),field);
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
		Time	tm=(wasnull)?null:
			createTime(currentrow-1,columnindex-1,cal);
		drv.debugEnd();
		return tm;
	}

	private
	Time createTime(long row, int col, Calendar cal) {
		drv.debugFunction(this);
		drv.debugPrintln("row: "+row);
		drv.debugPrintln("col: "+col);
		drv.debugPrintln("cal: "+cal);
		short	hour;
		short	minute;
		short	second;
		synchronized (networklock) {
			hour=sqlrcur.getFieldAsDateHour(row,col);
			minute=sqlrcur.getFieldAsDateMinute(row,col);
			second=sqlrcur.getFieldAsDateSecond(row,col);
		}
		drv.debugPrintln("hour: "+hour);
		drv.debugPrintln("minute: "+minute);
		drv.debugPrintln("second: "+second);
		Time	tm=createTime(hour,minute,second,cal);
		drv.debugEnd();
		return tm;
	}

	private
	Time createTime(long row, String col, Calendar cal) {
		drv.debugFunction(this);
		drv.debugPrintln("row: "+row);
		drv.debugPrintln("col: "+col);
		drv.debugPrintln("cal: "+cal);
		short	hour;
		short	minute;
		short	second;
		synchronized (networklock) {
			hour=sqlrcur.getFieldAsDateHour(row,col);
			minute=sqlrcur.getFieldAsDateMinute(row,col);
			second=sqlrcur.getFieldAsDateSecond(row,col);
		}
		drv.debugPrintln("hour: "+hour);
		drv.debugPrintln("minute: "+minute);
		drv.debugPrintln("second: "+second);
		Time	tm=createTime(hour,minute,second,cal);
		drv.debugEnd();
		return tm;
	}

	private
	Time createTime(short hour, short minute, short second, Calendar cal) {
		drv.debugFunction(this);
		hour=(hour>=0)?hour:0;
		minute=(minute>=0)?minute:0;
		second=(second>=0)?second:0;
		Time	tm=null;
		if (cal!=null) {
			cal.set(Calendar.YEAR,1970);
			cal.set(Calendar.MONTH,Calendar.JANUARY);
			cal.set(Calendar.DAY_OF_MONTH,1);
			cal.set(Calendar.HOUR_OF_DAY,hour);
			cal.set(Calendar.MINUTE,minute);
			cal.set(Calendar.SECOND,second);
			cal.set(Calendar.MILLISECOND,0);
			tm=new Time(cal.getTimeInMillis());
		} else {
			tm=new Time(hour,minute,second);
		}
		drv.debugEnd();
		return tm;
	}

	public
	Time getTime(String columnlabel) throws SQLException {
		drv.debugFunction(this);
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
		Time	tm=(wasnull)?null:
			createTime(currentrow-1,columnlabel,cal);
		drv.debugEnd();
		return tm;
	}

	public
	Timestamp getTimestamp(int columnindex) throws SQLException {
		drv.debugFunction(this);
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
		Timestamp	ts=(wasnull)?null:
			createTimestamp(currentrow-1,columnindex-1,cal);
		drv.debugEnd();
		return ts;
	}

	private
	Timestamp createTimestamp(long row, int col, Calendar cal) {
		drv.debugFunction(this);
		drv.debugPrintln("row: "+row);
		drv.debugPrintln("col: "+col);
		drv.debugPrintln("cal: "+cal);
		short	year;
		short	month;
		short	day;
		short	hour;
		short	minute;
		short	second;
		int	microsecond;
		synchronized (networklock) {
			year=sqlrcur.getFieldAsDateYear(row,col);
			month=sqlrcur.getFieldAsDateMonth(row,col);
			day=sqlrcur.getFieldAsDateDay(row,col);
			hour=sqlrcur.getFieldAsDateHour(row,col);
			minute=sqlrcur.getFieldAsDateMinute(row,col);
			second=sqlrcur.getFieldAsDateSecond(row,col);
			microsecond=sqlrcur.getFieldAsDateMicrosecond(row,col);
		}
		drv.debugPrintln("year: "+year);
		drv.debugPrintln("month: "+month);
		drv.debugPrintln("day: "+day);
		drv.debugPrintln("hour: "+hour);
		drv.debugPrintln("minute: "+minute);
		drv.debugPrintln("second: "+second);
		drv.debugPrintln("microsecond: "+microsecond);
		Timestamp	ts=createTimestamp(year,month,day,
						hour,minute,second,
						microsecond,cal);
		drv.debugEnd();
		return ts;
	}

	private
	Timestamp createTimestamp(long row, String col, Calendar cal) {
		drv.debugFunction(this);
		drv.debugPrintln("row: "+row);
		drv.debugPrintln("col: "+col);
		drv.debugPrintln("cal: "+cal);
		short	year;
		short	month;
		short	day;
		short	hour;
		short	minute;
		short	second;
		int	microsecond;
		synchronized (networklock) {
			year=sqlrcur.getFieldAsDateYear(row,col);
			month=sqlrcur.getFieldAsDateMonth(row,col);
			day=sqlrcur.getFieldAsDateDay(row,col);
			hour=sqlrcur.getFieldAsDateHour(row,col);
			minute=sqlrcur.getFieldAsDateMinute(row,col);
			second=sqlrcur.getFieldAsDateSecond(row,col);
			microsecond=sqlrcur.getFieldAsDateMicrosecond(row,col);
		}
		drv.debugPrintln("year: "+year);
		drv.debugPrintln("month: "+month);
		drv.debugPrintln("day: "+day);
		drv.debugPrintln("hour: "+hour);
		drv.debugPrintln("minute: "+minute);
		drv.debugPrintln("second: "+second);
		drv.debugPrintln("microsecond: "+microsecond);
		Timestamp	ts=createTimestamp(year,month,day,
						hour,minute,second,
						microsecond,cal);
		drv.debugEnd();
		return ts;
	}

	public
	Timestamp getTimestamp(String columnlabel) throws SQLException {
		drv.debugFunction(this);
		Timestamp	t=getTimestamp(columnlabel,null);
		drv.debugEnd();
		return t;
	}

	private
	Timestamp createTimestamp(short year, short month, short day,
					short hour, short minute,
					short second, int microsecond,
					Calendar cal) {
		drv.debugFunction(this);
		year=(year>=0)?year:0;
		month=(month>=0)?month:0;
		day=(day>=0)?day:0;
		hour=(hour>=0)?hour:0;
		minute=(minute>=0)?minute:0;
		second=(second>=0)?second:0;
		microsecond=(microsecond>0)?microsecond:0;
		Timestamp	ts=null;
		if (cal!=null) {
			cal.set(Calendar.YEAR,year);
			cal.set(Calendar.MONTH,month-1);
			cal.set(Calendar.DAY_OF_MONTH,day);
			cal.set(Calendar.HOUR_OF_DAY,hour);
			cal.set(Calendar.MINUTE,minute);
			cal.set(Calendar.SECOND,second);
			cal.set(Calendar.MILLISECOND,0);
			ts=new Timestamp(cal.getTimeInMillis());
			ts.setNanos(microsecond*1000);
		} else {
			ts=new Timestamp(year-1900,month-1,day,
						hour,minute,second,
						microsecond*1000);
		}
		drv.debugEnd();
		return ts;
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
		Timestamp	ts=(wasnull)?null:
			createTimestamp(currentrow-1,columnlabel,cal);
		drv.debugEnd();
		return ts;
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
		field=truncateField(isCharBinaryCol(columnindex),field);
		wasnull=(field==null);
		drv.debugPrintln("column index: "+columnindex);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (wasnull)?null:new ByteArrayInputStream(
					field.getBytes(StandardCharsets.UTF_8));
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
		field=truncateField(isCharBinaryCol(columnlabel),field);
		wasnull=(field==null);
		drv.debugPrintln("column: "+columnlabel);
		drv.debugPrintln("field: "+field);
		drv.debugPrintln("was null: "+wasnull);
		drv.debugEnd();
		return (wasnull)?null:new ByteArrayInputStream(
					field.getBytes(StandardCharsets.UTF_8));
	}

	public
	URL getURL(int columnindex) throws SQLException {
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
}
