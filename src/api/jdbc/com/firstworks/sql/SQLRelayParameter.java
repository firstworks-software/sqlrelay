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

	public
	String getClassName() {
		driver.debugFunction(this);
		driver.debugPrintln("class name: "+classname);
		driver.debugEnd();
		return classname;
	}

	public
	int getMode() {
		driver.debugFunction(this);
		driver.debugPrintln("mode: "+mode);
		driver.debugEnd();
		return mode;
	}

	public
	int getType() {
		driver.debugFunction(this);
		driver.debugPrintln("type: "+type);
		driver.debugEnd();
		return type;
	}

	public
	String getTypeName() {
		driver.debugFunction(this);
		driver.debugPrintln("type name: "+typename);
		driver.debugEnd();
		return typename;
	}

	public
	int getPrecision() {
		driver.debugFunction(this);
		driver.debugPrintln("precision: "+precision);
		driver.debugEnd();
		return precision;
	}

	public
	int getScale() {
		driver.debugFunction(this);
		driver.debugPrintln("scale: "+scale);
		driver.debugEnd();
		return scale;
	}

	public
	int getIsNullable() {
		driver.debugFunction(this);
		driver.debugPrintln("nullable: "+nullable);
		driver.debugEnd();
		return nullable;
	}

	public
	boolean getIsSigned() {
		driver.debugFunction(this);
		driver.debugPrintln("signed: "+signed);
		driver.debugEnd();
		return signed;
	}

	public
	Object getObject() {
		driver.debugFunction(this);
		driver.debugPrintln("object: "+object);
		driver.debugEnd();
		return object;
	}

	public
	long getLength() {
		driver.debugFunction(this);
		driver.debugPrintln("length: "+length);
		driver.debugEnd();
		return length;
	}

	public
	boolean getIsBinary() {
		driver.debugFunction(this);
		driver.debugPrintln("binary: "+binary);
		driver.debugEnd();
		return binary;
	}

	public
	boolean getIsLob() {
		driver.debugFunction(this);
		driver.debugPrintln("lob: "+lob);
		driver.debugEnd();
		return lob;
	}

	public
	boolean getIsAscii() {
		driver.debugFunction(this);
		driver.debugPrintln("ascii: "+ascii);
		driver.debugEnd();
		return ascii;
	}

	public
	Calendar getCalendar() {
		driver.debugFunction(this);
		driver.debugPrintln("calendar: "+cal);
		driver.debugEnd();
		return cal;
	}

	public
	BindType getBindType() {
		driver.debugFunction(this);
		driver.debugPrintln("bind type: "+bindtype);
		driver.debugEnd();
		return bindtype;
	}

	public
	void setClassName(String classname) {
		driver.debugFunction(this);
		driver.debugPrintln("class name: "+classname);
		this.classname=classname;
		driver.debugEnd();
	}

	public
	void setMode(int mode) {
		driver.debugFunction(this);
		driver.debugPrintln("mode: "+mode);
		this.mode=mode;
		driver.debugEnd();
	}

	public
	void setType(int type) {
		driver.debugFunction(this);
		driver.debugPrintln("type: "+type);
		this.type=type;
		driver.debugEnd();
	}

	public
	void setTypeName(String typename) {
		driver.debugFunction(this);
		driver.debugPrintln("type name: "+typename);
		this.typename=typename;
		driver.debugEnd();
	}

	public
	void setPrecision(int precision) {
		driver.debugFunction(this);
		driver.debugPrintln("precision: "+precision);
		this.precision=precision;
		driver.debugEnd();
	}

	public
	void setScale(int scale) {
		driver.debugFunction(this);
		driver.debugPrintln("scale: "+scale);
		this.scale=scale;
		driver.debugEnd();
	}

	public
	void setIsNullable(int nullable) {
		driver.debugFunction(this);
		driver.debugPrintln("nullable: "+nullable);
		this.nullable=nullable;
		driver.debugEnd();
	}

	public
	void setIsSigned(boolean signed) {
		driver.debugFunction(this);
		driver.debugPrintln("signed: "+signed);
		this.signed=signed;
		driver.debugEnd();
	}

	public
	void setObject(Object object) {
		driver.debugFunction(this);
		driver.debugPrintln("object: "+object);
		this.object=object;
		driver.debugEnd();
	}

	public
	void setLength(long length) {
		driver.debugFunction(this);
		driver.debugPrintln("length: "+length);
		this.length=length;
		driver.debugEnd();
	}

	public
	void setIsBinary(boolean binary) {
		driver.debugFunction(this);
		driver.debugPrintln("binary: "+binary);
		this.binary=binary;
		driver.debugEnd();
	}

	public
	void setIsLob(boolean lob) {
		driver.debugFunction(this);
		driver.debugPrintln("lob: "+lob);
		this.lob=lob;
		driver.debugEnd();
	}

	public
	void setIsAscii(boolean ascii) {
		driver.debugFunction(this);
		driver.debugPrintln("ascii: "+ascii);
		this.ascii=ascii;
		driver.debugEnd();
	}

	public
	void setCalendar(Calendar cal) {
		driver.debugFunction(this);
		driver.debugPrintln("calendar: "+cal);
		this.cal=cal;
		driver.debugEnd();
	}

	public
	void setBindType(BindType bindtype) {
		driver.debugFunction(this);
		driver.debugPrintln("bind type: "+bindtype);
		this.bindtype=bindtype;
		driver.debugEnd();
	}
}
