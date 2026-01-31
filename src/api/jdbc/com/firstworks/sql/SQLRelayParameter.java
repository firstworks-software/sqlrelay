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

	private SQLRelayDriver	drv;

	private String		classname;
	private int		mode;
	private int		type;
	private String		typename;
	private int		precision;
	private int		scale;
	private int		nullable;
	private boolean		signed;
	private Object		object;
	private long		length;
	private boolean		binary;
	private boolean		lob;
	private boolean		ascii;
	private Calendar	cal;
	private	BindType	bindtype;


	public SQLRelayParameter(SQLRelayDriver driver) {
		this.drv=driver;
		drv.debugFunction(this);
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
		drv.debugEnd();
	}

	public
	String getClassName() {
		drv.debugFunction(this);
		drv.debugPrintln("class name: "+classname);
		drv.debugEnd();
		return classname;
	}

	public
	int getMode() {
		drv.debugFunction(this);
		drv.debugPrintln("mode: "+mode);
		drv.debugEnd();
		return mode;
	}

	public
	int getType() {
		drv.debugFunction(this);
		drv.debugPrintln("type: "+type);
		drv.debugEnd();
		return type;
	}

	public
	String getTypeName() {
		drv.debugFunction(this);
		drv.debugPrintln("type name: "+typename);
		drv.debugEnd();
		return typename;
	}

	public
	int getPrecision() {
		drv.debugFunction(this);
		drv.debugPrintln("precision: "+precision);
		drv.debugEnd();
		return precision;
	}

	public
	int getScale() {
		drv.debugFunction(this);
		drv.debugPrintln("scale: "+scale);
		drv.debugEnd();
		return scale;
	}

	public
	int getIsNullable() {
		drv.debugFunction(this);
		drv.debugPrintln("nullable: "+nullable);
		drv.debugEnd();
		return nullable;
	}

	public
	boolean getIsSigned() {
		drv.debugFunction(this);
		drv.debugPrintln("signed: "+signed);
		drv.debugEnd();
		return signed;
	}

	public
	Object getObject() {
		drv.debugFunction(this);
		drv.debugPrintln("object: "+object);
		drv.debugEnd();
		return object;
	}

	public
	long getLength() {
		drv.debugFunction(this);
		drv.debugPrintln("length: "+length);
		drv.debugEnd();
		return length;
	}

	public
	boolean getIsBinary() {
		drv.debugFunction(this);
		drv.debugPrintln("binary: "+binary);
		drv.debugEnd();
		return binary;
	}

	public
	boolean getIsLob() {
		drv.debugFunction(this);
		drv.debugPrintln("lob: "+lob);
		drv.debugEnd();
		return lob;
	}

	public
	boolean getIsAscii() {
		drv.debugFunction(this);
		drv.debugPrintln("ascii: "+ascii);
		drv.debugEnd();
		return ascii;
	}

	public
	Calendar getCalendar() {
		drv.debugFunction(this);
		drv.debugPrintln("calendar: "+cal);
		drv.debugEnd();
		return cal;
	}

	public
	BindType getBindType() {
		drv.debugFunction(this);
		drv.debugPrintln("bind type: "+bindtype);
		drv.debugEnd();
		return bindtype;
	}

	public
	void setClassName(String classname) {
		drv.debugFunction(this);
		drv.debugPrintln("class name: "+classname);
		this.classname=classname;
		drv.debugEnd();
	}

	public
	void setMode(int mode) {
		drv.debugFunction(this);
		drv.debugPrintln("mode: "+mode);
		this.mode=mode;
		drv.debugEnd();
	}

	public
	void setType(int type) {
		drv.debugFunction(this);
		drv.debugPrintln("type: "+type);
		this.type=type;
		drv.debugEnd();
	}

	public
	void setTypeName(String typename) {
		drv.debugFunction(this);
		drv.debugPrintln("type name: "+typename);
		this.typename=typename;
		drv.debugEnd();
	}

	public
	void setPrecision(int precision) {
		drv.debugFunction(this);
		drv.debugPrintln("precision: "+precision);
		this.precision=precision;
		drv.debugEnd();
	}

	public
	void setScale(int scale) {
		drv.debugFunction(this);
		drv.debugPrintln("scale: "+scale);
		this.scale=scale;
		drv.debugEnd();
	}

	public
	void setIsNullable(int nullable) {
		drv.debugFunction(this);
		drv.debugPrintln("nullable: "+nullable);
		this.nullable=nullable;
		drv.debugEnd();
	}

	public
	void setIsSigned(boolean signed) {
		drv.debugFunction(this);
		drv.debugPrintln("signed: "+signed);
		this.signed=signed;
		drv.debugEnd();
	}

	public
	void setObject(Object object) {
		drv.debugFunction(this);
		drv.debugPrintln("object: "+object);
		this.object=object;
		drv.debugEnd();
	}

	public
	void setLength(long length) {
		drv.debugFunction(this);
		drv.debugPrintln("length: "+length);
		this.length=length;
		drv.debugEnd();
	}

	public
	void setIsBinary(boolean binary) {
		drv.debugFunction(this);
		drv.debugPrintln("binary: "+binary);
		this.binary=binary;
		drv.debugEnd();
	}

	public
	void setIsLob(boolean lob) {
		drv.debugFunction(this);
		drv.debugPrintln("lob: "+lob);
		this.lob=lob;
		drv.debugEnd();
	}

	public
	void setIsAscii(boolean ascii) {
		drv.debugFunction(this);
		drv.debugPrintln("ascii: "+ascii);
		this.ascii=ascii;
		drv.debugEnd();
	}

	public
	void setCalendar(Calendar cal) {
		drv.debugFunction(this);
		drv.debugPrintln("calendar: "+cal);
		this.cal=cal;
		drv.debugEnd();
	}

	public
	void setBindType(BindType bindtype) {
		drv.debugFunction(this);
		drv.debugPrintln("bind type: "+bindtype);
		this.bindtype=bindtype;
		drv.debugEnd();
	}
}
