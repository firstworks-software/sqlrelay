// Copyright (c) David Muse
// See the file COPYING for more information.
package com.firstworks.sql;

import java.sql.*;

import java.util.HashMap;

import com.firstworks.sqlrelay.*;

public class SQLRelayParameterMetaData implements ParameterMetaData {

	private	SQLRelayDriver		drv;
	private	SQLRelayConnection	conn;

	private HashMap<String,SQLRelayParameter>	inparams;
	private HashMap<String,SQLRelayParameter>	outparams;


	public
	SQLRelayParameterMetaData(SQLRelayDriver driver) {
		this.drv=driver;
		drv.debugFunction(this);
		conn=null;
		inparams=null;
		outparams=null;
		drv.debugEnd();
	}

	void setConn(SQLRelayConnection conn) {
		drv.debugFunction(this);
		this.conn=conn;
		drv.debugEnd();
	}

	void setInParams(HashMap<String,SQLRelayParameter> inparams) {
		drv.debugFunction(this);
		this.inparams=inparams;
		drv.debugPrintln("input parameter count: "+inparams.size());
		drv.debugEnd();
	}

	void setOutParams(HashMap<String,SQLRelayParameter> outparams) {
		drv.debugFunction(this);
		this.outparams=outparams;
		drv.debugPrintln("output parameter count: "+
							outparams.size());
		drv.debugEnd();
	}

	public
	String getParameterClassName(int param) {
		drv.debugFunction(this);
		SQLRelayParameter	p=getParameter(param);
		String	name=(p!=null)?p.getClassName():null;
		drv.debugPrintln("param: "+param);
		drv.debugPrintln("class name: "+name);
		drv.debugEnd();
		return name;
	}

	private
	SQLRelayParameter getParameter(int param) {
		drv.debugFunction(this);
		String	key=String.valueOf(param);
		SQLRelayParameter	p=null;
		if (inparams!=null) {
			p=inparams.get(key);
		}
		if (p==null && outparams!=null) {
			p=outparams.get(key);
		}
		drv.debugPrintln("param: "+param);
		drv.debugEnd();
		return p;
	}

	// jdbc's ParameterMetaData only promises to throw on a database
	// access error, and every reference driver default here is at
	// least honest ("unknown", not a plausible-looking answer) -
	// except isSigned(), which has no "unknown" value to fall back on
	// and would otherwise affirmatively lie about a parameter that
	// does not exist.  So only isSigned() validates.
	private
	void validateParameter(SQLRelayParameter p) throws SQLException {
		if (p==null) {
			conn.throwException("invalid parameter index");
		}
	}

	public
	int getParameterCount() {
		drv.debugFunction(this);
		int	count=0;
		if (inparams!=null) {
			count+=inparams.size();
		}
		if (outparams!=null) {
			count+=outparams.size();
		}
		drv.debugPrintln("parameter count: "+count);
		drv.debugEnd();
		return count;
	}

	public
	int getParameterMode(int param) {
		drv.debugFunction(this);
		SQLRelayParameter	p=getParameter(param);
		int	mode=(p!=null)?p.getMode():parameterModeUnknown;
		drv.debugPrintln("param: "+param);
		drv.debugPrintln("mode: "+mode);
		drv.debugEnd();
		return mode;
	}

	public
	int getParameterType(int param) {
		drv.debugFunction(this);
		SQLRelayParameter	p=getParameter(param);
		int	type=(p!=null)?p.getType():0;
		drv.debugPrintln("param: "+param);
		drv.debugPrintln("type: "+type);
		drv.debugEnd();
		return type;
	}

	public
	String getParameterTypeName(int param) {
		drv.debugFunction(this);
		SQLRelayParameter	p=getParameter(param);
		String	name=(p!=null)?p.getTypeName():null;
		drv.debugPrintln("param: "+param);
		drv.debugPrintln("type name: "+name);
		drv.debugEnd();
		return name;
	}

	public
	int getPrecision(int param) {
		drv.debugFunction(this);
		SQLRelayParameter	p=getParameter(param);
		int	precision=(p!=null)?p.getPrecision():0;
		drv.debugPrintln("param: "+param);
		drv.debugPrintln("precision: "+precision);
		drv.debugEnd();
		return precision;
	}

	public
	int getScale(int param) {
		drv.debugFunction(this);
		SQLRelayParameter	p=getParameter(param);
		int	scale=(p!=null)?p.getScale():0;
		drv.debugPrintln("param: "+param);
		drv.debugPrintln("scale: "+scale);
		drv.debugEnd();
		return scale;
	}

	public
	int isNullable(int param) {
		drv.debugFunction(this);
		SQLRelayParameter	p=getParameter(param);
		int	in=(p!=null)?p.getIsNullable():parameterNullableUnknown;
		drv.debugPrintln("param: "+param);
		switch (in) {
			case ParameterMetaData.parameterNoNulls:
				drv.debugPrintln("is nullable: no");
				break;
			case ParameterMetaData.parameterNullable:
				drv.debugPrintln("is nullable: yes");
				break;
			case ParameterMetaData.parameterNullableUnknown:
				drv.debugPrintln("is nullable: unknown");
				break;
		}
		drv.debugEnd();
		return in;
	}

	public
	boolean isSigned(int param) throws SQLException {
		drv.debugFunction(this);
		SQLRelayParameter	p=getParameter(param);
		validateParameter(p);
		boolean	is=p.getIsSigned();
		drv.debugPrintln("param: "+param);
		drv.debugPrintln("is signed: "+((is)?"yes":"no"));
		drv.debugEnd();
		return is;
	}

	public
	boolean isWrapperFor(Class<?> iface) throws SQLException {
		drv.debugFunction(this);
		drv.debugEnd();
		return false;
	}

	public
	<T> T unwrap(Class<T> iface) throws SQLException {
		drv.debugFunction(this);
		drv.debugEnd();
		return null;
	}
}
