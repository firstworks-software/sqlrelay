package com.firstworks.sql;

import java.sql.*;

import java.util.Calendar;

import com.firstworks.sqlrelay.*;

public class SQLRelayParameter {

	public enum BindType {
		Array,
		AsciiStream,
		AsciiStreamWithIntLength,
		AsciiStreamWithLongLength,
		BigDecimal,
		BinaryStream,
		BinaryStreamWithIntLength,
		BinaryStreamWithLongLength,
		Blob,
		BlobInputStream,
		BlobInputStreamWithLongLength,
		Boolean,
		Byte,
		Bytes,
		CharacterStream,
		CharacterStreamWithIntLength,
		CharacterStreamWithLongLength,
		Clob,
		ClobReader,
		ClobReaderWithLength,
		Date,
		DateWithCalendar,
		Double,
		Float,
		Int,
		Long,
		NCharStream,
		NCharStreamWithLength,
		NClob,
		NClobReader,
		NClobReaderWithLength,
		NString,
		Null,
		NullWithTypeName,
		Object,
		ObjectWithTargetType,
		ObjectWithTargetTypeAndScaleOrLength,
		Ref,
		RowId,
		Short,
		String,
		SQLXML,
		Time,
		TimeWithCalendar,
		Timestamp,
		TimestampWithCalendar,
		UnicodeStream,
		URL
	};

	private String	classname;
	private int	mode;
	private int	type;
	private String	typename;
	private int	precision;
	private int	scale;
	private int	nullable;
	private boolean	signed;

	private Object		object;
	private long		length;
	private boolean		binary;
	private boolean		lob;
	private boolean		ascii;
	private Calendar	cal;
	private	BindType	bindtype;

	private SQLRelayDriver	driver;

	public SQLRelayParameter(SQLRelayDriver driver) {
		this.driver=driver;
		driver.debugFunction(this);
		classname=null;
		mode=ParameterMetaData.parameterModeIn;
		type=0;
		typename=null;
		precision=0;
		scale=0;
		nullable=ParameterMetaData.parameterNullableUnknown;
		signed=true;
		object=null;
		length=0;
		binary=false;
		lob=false;
		ascii=false;
		cal=null;
		bindtype=BindType.Null;
		driver.debugEnd();
	}

	public synchronized
	String getClassName() {
		driver.debugFunction(this);
		driver.debugEnd();
		return classname;
	}

	public synchronized
	int getMode() {
		driver.debugFunction(this);
		driver.debugEnd();
		return mode;
	}

	public synchronized
	int getType() {
		driver.debugFunction(this);
		driver.debugEnd();
		return type;
	}

	public synchronized
	String getTypeName() {
		driver.debugFunction(this);
		driver.debugEnd();
		return typename;
	}

	public synchronized
	int getPrecision() {
		driver.debugFunction(this);
		driver.debugEnd();
		return precision;
	}

	public synchronized
	int getScale() {
		driver.debugFunction(this);
		driver.debugEnd();
		return scale;
	}

	public synchronized
	int getIsNullable() {
		driver.debugFunction(this);
		driver.debugEnd();
		return nullable;
	}

	public synchronized
	boolean getIsSigned() {
		driver.debugFunction(this);
		driver.debugEnd();
		return signed;
	}

	public synchronized
	Object getObject() {
		driver.debugFunction(this);
		driver.debugEnd();
		return object;
	}

	public synchronized
	long getLength() {
		driver.debugFunction(this);
		driver.debugEnd();
		return length;
	}

	public synchronized
	boolean getIsBinary() {
		driver.debugFunction(this);
		driver.debugEnd();
		return binary;
	}

	public synchronized
	boolean getIsLob() {
		driver.debugFunction(this);
		driver.debugEnd();
		return lob;
	}

	public synchronized
	boolean getIsAscii() {
		driver.debugFunction(this);
		driver.debugEnd();
		return ascii;
	}

	public synchronized
	Calendar getCalendar() {
		driver.debugFunction(this);
		driver.debugEnd();
		return cal;
	}

	public synchronized
	BindType getBindType() {
		driver.debugFunction(this);
		driver.debugEnd();
		return bindtype;
	}

	public synchronized
	void setClassName(String classname) {
		driver.debugFunction(this);
		this.classname=classname;
		driver.debugEnd();
	}

	public synchronized
	void setMode(int mode) {
		driver.debugFunction(this);
		this.mode=mode;
		driver.debugEnd();
	}

	public synchronized
	void setType(int type) {
		driver.debugFunction(this);
		this.type=type;
		driver.debugEnd();
	}

	public synchronized
	void setTypeName(String typename) {
		driver.debugFunction(this);
		this.typename=typename;
		driver.debugEnd();
	}

	public synchronized
	void setPrecision(int precision) {
		driver.debugFunction(this);
		this.precision=precision;
		driver.debugEnd();
	}

	public synchronized
	void setScale(int scale) {
		driver.debugFunction(this);
		this.scale=scale;
		driver.debugEnd();
	}

	public synchronized
	void setIsNullable(int nullable) {
		driver.debugFunction(this);
		this.nullable=nullable;
		driver.debugEnd();
	}

	public synchronized
	void setIsSigned(boolean signed) {
		driver.debugFunction(this);
		this.signed=signed;
		driver.debugEnd();
	}

	public synchronized
	void setObject(Object object) {
		driver.debugFunction(this);
		this.object=object;
		driver.debugEnd();
	}

	public synchronized
	void setLength(long length) {
		driver.debugFunction(this);
		this.length=length;
		driver.debugEnd();
	}

	public synchronized
	void setIsBinary(boolean binary) {
		driver.debugFunction(this);
		this.binary=binary;
		driver.debugEnd();
	}

	public synchronized
	void setIsLob(boolean lob) {
		driver.debugFunction(this);
		this.lob=lob;
		driver.debugEnd();
	}

	public synchronized
	void setIsAscii(boolean ascii) {
		driver.debugFunction(this);
		this.ascii=ascii;
		driver.debugEnd();
	}

	public synchronized
	void setCalendar(Calendar cal) {
		driver.debugFunction(this);
		this.cal=cal;
		driver.debugEnd();
	}

	public synchronized
	void setBindType(BindType bindtype) {
		driver.debugFunction(this);
		this.bindtype=bindtype;
		driver.debugEnd();
	}
}
