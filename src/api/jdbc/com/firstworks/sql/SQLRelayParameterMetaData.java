package com.firstworks.sql;

import java.sql.*;

import java.util.HashMap;

import com.firstworks.sqlrelay.*;

public class SQLRelayParameterMetaData extends SQLRelayDebug implements ParameterMetaData {

	private HashMap<Integer,SQLRelayParameter>	parameters;

	public SQLRelayParameterMetaData() {
		debugFunction();
		parameters=null;
		debugEnd();
	}

	public void	setParameters(
				HashMap<Integer,SQLRelayParameter> parameters) {
		debugFunction();
		this.parameters=parameters;
		debugEnd();
	}

	public String 	getParameterClassName(int param) {
		debugFunction();
		SQLRelayParameter	p=getParameter(param);
		String	name=(p!=null)?p.getClassName():null;
		debugEnd();
		return name;
	}

	private SQLRelayParameter getParameter(int param) {
		debugFunction();
		SQLRelayParameter	p=
			(parameters!=null)?parameters.get(param):null;
		debugEnd();
		return p;
	}

	public int 	getParameterCount() {
		debugFunction();
		int	p=(parameters!=null)?parameters.size():0;
		debugEnd();
		return p;
	}

	public int 	getParameterMode(int param) {
		debugFunction();
		SQLRelayParameter	p=getParameter(param);
		int	mode=(p!=null)?p.getMode():parameterModeUnknown;
		debugEnd();
		return mode;
	}

	public int 	getParameterType(int param) {
		debugFunction();
		SQLRelayParameter	p=getParameter(param);
		int	type=(p!=null)?p.getType():0;
		debugEnd();
		return type;
	}

	public String 	getParameterTypeName(int param) {
		debugFunction();
		SQLRelayParameter	p=getParameter(param);
		String	name=(p!=null)?p.getTypeName():null;
		debugEnd();
		return name;
	}

	public int 	getPrecision(int param) {
		debugFunction();
		SQLRelayParameter	p=getParameter(param);
		int	precision=(p!=null)?p.getPrecision():0;
		debugEnd();
		return precision;
	}

	public int 	getScale(int param) {
		debugFunction();
		SQLRelayParameter	p=getParameter(param);
		int	scale=(p!=null)?p.getScale():0;
		debugEnd();
		return scale;
	}

	public int 	isNullable(int param) {
		debugFunction();
		SQLRelayParameter	p=getParameter(param);
		int	in=(p!=null)?p.getIsNullable():parameterNullableUnknown;
		debugEnd();
		return in;
	}

	public boolean 	isSigned(int param) {
		debugFunction();
		SQLRelayParameter	p=getParameter(param);
		boolean	is=(p!=null)?p.getIsSigned():true;
		debugEnd();
		return is;
	}

	public boolean	isWrapperFor(Class<?> iface) throws SQLException {
		debugFunction();
		debugEnd();
		return false;
	}

	public <T> T	unwrap(Class<T> iface) throws SQLException {
		debugFunction();
		debugEnd();
		return null;
	}
}
