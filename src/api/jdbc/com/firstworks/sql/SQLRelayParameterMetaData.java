package com.firstworks.sql;

import java.sql.*;

import java.util.HashMap;

import com.firstworks.sqlrelay.*;

public class SQLRelayParameterMetaData implements ParameterMetaData {

	private HashMap<Integer,SQLRelayParameter>	parameters;
	private	SQLRelayDriver	driver;

	public SQLRelayParameterMetaData(SQLRelayDriver driver) {
		this.driver=driver;
		driver.debugFunction();
		parameters=null;
		driver.debugEnd();
	}

	public void	setParameters(
				HashMap<Integer,SQLRelayParameter> parameters) {
		driver.debugFunction();
		this.parameters=parameters;
		driver.debugEnd();
	}

	public String 	getParameterClassName(int param) {
		driver.debugFunction();
		SQLRelayParameter	p=getParameter(param);
		String	name=(p!=null)?p.getClassName():null;
		driver.debugEnd();
		return name;
	}

	private SQLRelayParameter getParameter(int param) {
		driver.debugFunction();
		SQLRelayParameter	p=
			(parameters!=null)?parameters.get(param):null;
		driver.debugEnd();
		return p;
	}

	public int 	getParameterCount() {
		driver.debugFunction();
		int	p=(parameters!=null)?parameters.size():0;
		driver.debugEnd();
		return p;
	}

	public int 	getParameterMode(int param) {
		driver.debugFunction();
		SQLRelayParameter	p=getParameter(param);
		int	mode=(p!=null)?p.getMode():parameterModeUnknown;
		driver.debugEnd();
		return mode;
	}

	public int 	getParameterType(int param) {
		driver.debugFunction();
		SQLRelayParameter	p=getParameter(param);
		int	type=(p!=null)?p.getType():0;
		driver.debugEnd();
		return type;
	}

	public String 	getParameterTypeName(int param) {
		driver.debugFunction();
		SQLRelayParameter	p=getParameter(param);
		String	name=(p!=null)?p.getTypeName():null;
		driver.debugEnd();
		return name;
	}

	public int 	getPrecision(int param) {
		driver.debugFunction();
		SQLRelayParameter	p=getParameter(param);
		int	precision=(p!=null)?p.getPrecision():0;
		driver.debugEnd();
		return precision;
	}

	public int 	getScale(int param) {
		driver.debugFunction();
		SQLRelayParameter	p=getParameter(param);
		int	scale=(p!=null)?p.getScale():0;
		driver.debugEnd();
		return scale;
	}

	public int 	isNullable(int param) {
		driver.debugFunction();
		SQLRelayParameter	p=getParameter(param);
		int	in=(p!=null)?p.getIsNullable():parameterNullableUnknown;
		driver.debugEnd();
		return in;
	}

	public boolean 	isSigned(int param) {
		driver.debugFunction();
		SQLRelayParameter	p=getParameter(param);
		boolean	is=(p!=null)?p.getIsSigned():true;
		driver.debugEnd();
		return is;
	}

	public boolean	isWrapperFor(Class<?> iface) throws SQLException {
		driver.debugFunction();
		driver.debugEnd();
		return false;
	}

	public <T> T	unwrap(Class<T> iface) throws SQLException {
		driver.debugFunction();
		driver.debugEnd();
		return null;
	}
}
