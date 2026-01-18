package com.firstworks.sql;

import java.sql.*;

import java.util.HashMap;

import com.firstworks.sqlrelay.*;

public class SQLRelayParameterMetaData implements ParameterMetaData {

	private HashMap<Integer,SQLRelayParameter>	parameters;
	private	SQLRelayDriver	driver;

	public SQLRelayParameterMetaData(SQLRelayDriver driver) {
		this.driver=driver;
		driver.debugFunction(this);
		parameters=null;
		driver.debugEnd();
	}

	public synchronized
	void setParameters(HashMap<Integer,SQLRelayParameter> parameters) {
		driver.debugFunction(this);
		this.parameters=parameters;
		driver.debugEnd();
	}

	public synchronized
	String getParameterClassName(int param) {
		driver.debugFunction(this);
		SQLRelayParameter	p=getParameter(param);
		String	name=(p!=null)?p.getClassName():null;
		driver.debugEnd();
		return name;
	}

	private SQLRelayParameter getParameter(int param) {
		driver.debugFunction(this);
		SQLRelayParameter	p=
			(parameters!=null)?parameters.get(param):null;
		driver.debugEnd();
		return p;
	}

	public synchronized
	int getParameterCount() {
		driver.debugFunction(this);
		int	p=(parameters!=null)?parameters.size():0;
		driver.debugEnd();
		return p;
	}

	public synchronized
	int getParameterMode(int param) {
		driver.debugFunction(this);
		SQLRelayParameter	p=getParameter(param);
		int	mode=(p!=null)?p.getMode():parameterModeUnknown;
		driver.debugEnd();
		return mode;
	}

	public synchronized
	int getParameterType(int param) {
		driver.debugFunction(this);
		SQLRelayParameter	p=getParameter(param);
		int	type=(p!=null)?p.getType():0;
		driver.debugEnd();
		return type;
	}

	public synchronized
	String getParameterTypeName(int param) {
		driver.debugFunction(this);
		SQLRelayParameter	p=getParameter(param);
		String	name=(p!=null)?p.getTypeName():null;
		driver.debugEnd();
		return name;
	}

	public synchronized
	int getPrecision(int param) {
		driver.debugFunction(this);
		SQLRelayParameter	p=getParameter(param);
		int	precision=(p!=null)?p.getPrecision():0;
		driver.debugEnd();
		return precision;
	}

	public synchronized
	int getScale(int param) {
		driver.debugFunction(this);
		SQLRelayParameter	p=getParameter(param);
		int	scale=(p!=null)?p.getScale():0;
		driver.debugEnd();
		return scale;
	}

	public synchronized
	int isNullable(int param) {
		driver.debugFunction(this);
		SQLRelayParameter	p=getParameter(param);
		int	in=(p!=null)?p.getIsNullable():parameterNullableUnknown;
		driver.debugEnd();
		return in;
	}

	public synchronized
	boolean isSigned(int param) {
		driver.debugFunction(this);
		SQLRelayParameter	p=getParameter(param);
		boolean	is=(p!=null)?p.getIsSigned():true;
		driver.debugEnd();
		return is;
	}

	public synchronized
	boolean isWrapperFor(Class<?> iface) throws SQLException {
		driver.debugFunction(this);
		driver.debugEnd();
		return false;
	}

	public synchronized
	<T> T unwrap(Class<T> iface) throws SQLException {
		driver.debugFunction(this);
		driver.debugEnd();
		return null;
	}
}
