package com.firstworks.sql;

import java.sql.*;

import java.util.Calendar;

import com.firstworks.sqlrelay.*;

public class SQLRelayParameter extends SQLRelayDebug {

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

	public SQLRelayParameter() {
		debugFunction();
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
		debugEnd();
	}

	public String 	getClassName() {
		debugFunction();
		debugEnd();
		return classname;
	}

	public int 	getMode() {
		debugFunction();
		debugEnd();
		return mode;
	}

	public int 	getType() {
		debugFunction();
		debugEnd();
		return type;
	}

	public String 	getTypeName() {
		debugFunction();
		debugEnd();
		return typename;
	}

	public int 	getPrecision() {
		debugFunction();
		debugEnd();
		return precision;
	}

	public int 	getScale() {
		debugFunction();
		debugEnd();
		return scale;
	}

	public int 	getIsNullable() {
		debugFunction();
		debugEnd();
		return nullable;
	}

	public boolean 	getIsSigned() {
		debugFunction();
		debugEnd();
		return signed;
	}

	public Object	getObject() {
		debugFunction();
		debugEnd();
		return object;
	}

	public long	getLength() {
		debugFunction();
		debugEnd();
		return length;
	}

	public boolean	getIsBinary() {
		debugFunction();
		debugEnd();
		return binary;
	}

	public boolean	getIsLob() {
		debugFunction();
		debugEnd();
		return lob;
	}

	public boolean	getIsAscii() {
		debugFunction();
		debugEnd();
		return ascii;
	}

	public Calendar getCalendar() {
		debugFunction();
		debugEnd();
		return cal;
	}

	public BindType getBindType() {
		debugFunction();
		debugEnd();
		return bindtype;
	}

	public void 	setClassName(String classname) {
		debugFunction();
		this.classname=classname;
		debugEnd();
	}

	public void 	setMode(int mode) {
		debugFunction();
		this.mode=mode;
		debugEnd();
	}

	public void 	setType(int type) {
		debugFunction();
		this.type=type;
		debugEnd();
	}

	public void 	setTypeName(String typename) {
		debugFunction();
		this.typename=typename;
		debugEnd();
	}

	public void 	setPrecision(int precision) {
		debugFunction();
		this.precision=precision;
		debugEnd();
	}

	public void 	setScale(int scale) {
		debugFunction();
		this.scale=scale;
		debugEnd();
	}

	public void 	setIsNullable(int nullable) {
		debugFunction();
		this.nullable=nullable;
		debugEnd();
	}

	public void 	setIsSigned(boolean signed) {
		debugFunction();
		this.signed=signed;
		debugEnd();
	}

	public void	setObject(Object object) {
		debugFunction();
		this.object=object;
		debugEnd();
	}

	public void	setLength(long length) {
		debugFunction();
		this.length=length;
		debugEnd();
	}

	public void	setIsBinary(boolean binary) {
		debugFunction();
		this.binary=binary;
		debugEnd();
	}

	public void	setIsLob(boolean lob) {
		debugFunction();
		this.lob=lob;
		debugEnd();
	}

	public void	setIsAscii(boolean ascii) {
		debugFunction();
		this.ascii=ascii;
		debugEnd();
	}

	public void	setCalendar(Calendar cal) {
		debugFunction();
		this.cal=cal;
		debugEnd();
	}

	public void	setBindType(BindType bindtype) {
		debugFunction();
		this.bindtype=bindtype;
		debugEnd();
	}
}
