// Copyright (c) David Muse
// See the file COPYING for more information.
package com.firstworks.sql;

import java.sql.*;

import java.util.Locale;

import com.firstworks.sqlrelay.*;

public class SQLRelayResultSetMetaData implements ResultSetMetaData {

	private	SQLRelayDriver		drv;
	private	SQLRelayResultSet	resultset;

	private	SQLRCursor		sqlrcur;


	public
	SQLRelayResultSetMetaData(SQLRelayDriver driver) {
		this.drv=driver;
		drv.debugFunction(this);
		sqlrcur=null;
		resultset=null;
		drv.debugEnd();
	}

	void setSQLRCursor(SQLRCursor sqlrcur) {
		this.sqlrcur=sqlrcur;
	}

	void setResultSet(SQLRelayResultSet resultset) {
		this.resultset=resultset;
	}

	SQLRCursor getSQLRCursor() {
		return sqlrcur;
	}

	public
	String getCatalogName(int column) {
		drv.debugFunction(this);
		String	catalogname="";
		drv.debugPrintln("column: "+column);
		drv.debugPrintln("catalog name: "+catalogname);
		drv.debugEnd();
		return catalogname;
	}

	// Uppercase with Locale.ROOT rather than the jvm's default
	// locale: in a turkish locale, "int4" uppercases to a dotted
	// capital I followed by NT4, which matches nothing.  Return ""
	// rather than null: getColumnType() returns null for an
	// out-of-range column, and java throws on a switch over a null
	// string.
	private
	String normalizeTypeName(String type) {
		return (type==null)?"":type.toUpperCase(Locale.ROOT);
	}

	public
	String getColumnClassName(int column) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("column: "+column);
		String	retval=null;
		String	ctype=normalizeTypeName(
					sqlrcur.getColumnType(column-1));
		drv.debugPrintln("ctype: "+ctype);
		switch (ctype) {
			case "UNKNOWN":
				retval=null;
				break;
			case "CHAR":
				retval="java.lang.String";
				break;
			case "INT":
				retval="java.lang.Integer";
				break;
			case "SMALLINT":
				retval="java.lang.Integer";
				break;
			case "TINYINT":
				retval="java.lang.Integer";
				break;
			case "MONEY":
				retval="java.lang.String";
				break;
			case "DATETIME":
				retval="java.sql.Timestamp";
				break;
			case "NUMERIC":
				retval="java.lang.String";
				break;
			case "DECIMAL":
				retval="java.lang.BigDecimal";
				break;
			case "SMALLDATETIME":
				retval="java.sql.Timestamp";
				break;
			case "SMALLMONEY":
				retval="java.lang.String";
				break;
			case "IMAGE":
				retval="java.lang.Byte";
				break;
			case "BINARY":
				retval="java.lang.Byte";
				break;
			case "BIT":
				retval="java.lang.Boolean";
				break;
			case "REAL":
				retval="java.lang.Double";
				break;
			case "FLOAT":
				retval="java.lang.Float";
				break;
			case "TEXT":
				retval="java.lang.String";
				break;
			case "VARCHAR":
				retval="java.lang.String";
				break;
			case "LVARCHAR":
				retval="java.lang.String";
				break;
			case "VARBINARY":
				retval="java.lang.Byte";
				break;
			case "LONGCHAR":
				retval="java.lang.String";
				break;
			case "LONGBINARY":
				retval="java.lang.Byte";
				break;
			case "LONG":
				retval="java.lang.String";
				break;
			case "ILLEGAL":
				retval="java.lang.String";
				break;
			case "SENSITIVITY":
				retval="java.lang.String";
				break;
			case "BOUNDARY":
				retval="java.lang.String";
				break;
			case "VOID":
				retval="java.lang.String";
				break;
			case "USHORT":
				retval="java.lang.Short";
				break;
	
			// added by lago
			case "UNDEFINED":
				retval=null;
				break;
			case "DOUBLE":
				retval="java.lang.Double";
				break;
			case "DATE":
				retval=(getDateToTimestamp())?
						"java.sql.Timestamp":
						"java.sql.Date";
				break;
			case "TIME":
				retval="java.sql.Time";
				break;
			case "TIMESTAMP":
				retval="java.sql.Timestamp";
				break;
	
			// added by msql
			case "UINT":
				retval="java.lang.Integer";
				break;
			case "LASTREAL":
				retval="java.lang.String";
				break;
	
			// added by mysql
			case "STRING":
				retval="java.lang.String";
				break;
			case "VARSTRING":
				retval="java.lang.String";
				break;
			case "LONGLONG":
				retval="java.lang.BigInteger";
				break;
			case "MEDIUMINT":
				retval="java.lang.Integer";
				break;
			case "YEAR":
				retval="java.lang.Short";
				break;
			case "NEWDATE":
				retval=(getDateToTimestamp())?
						"java.sql.Timestamp":
						"java.sql.Date";
				break;
			case "NULL":
				retval="java.lang.String";
				break;
			case "ENUM":
				retval="java.lang.String";
				break;
			case "SET":
				retval="java.lang.String";
				break;
			case "TINYBLOB":
			case "MEDIUMBLOB":
			case "LONGBLOB":
			case "BLOB":
				{
				boolean	binary=sqlrcur.
						getColumnIsBinary(column-1);
				drv.debugPrintln("is binary: "+binary);
				retval=(binary)?"java.lang.Byte":
						"java.lang.String";
				}
				break;

			// added by oracle
			case "VARCHAR2":
				retval="java.lang.String";
				break;
			case "NUMBER":
				retval="java.lang.String";
				break;
			case "ROWID":
				retval="java.lang.BigInteger";
				break;
			case "RAW":
				retval="java.lang.Byte";
				break;
			case "LONG_RAW":
				retval="java.lang.Byte";
				break;
			case "MLSLABEL":
				retval="java.lang.Byte";
				break;
			case "CLOB":
				retval="java.lang.String";
				break;
			case "BFILE":
				retval="java.lang.Byte";
				break;
	
			// added by odbc
			case "BIGINT":
				retval="java.lang.Long";
				break;
			case "INTEGER":
				retval="java.lang.Integer";
				break;
			case "LONGVARBINARY":
				retval="java.lang.Byte";
				break;
			case "LONGVARCHAR":
				retval="java.lang.String";
				break;
	
			// added by db2
			case "GRAPHIC":
				retval="java.lang.Byte";
				break;
			case "VARGRAPHIC":
				retval="java.lang.Byte";
				break;
			case "LONGVARGRAPHIC":
				retval="java.lang.Byte";
				break;
			case "DBCLOB":
				retval="java.lang.String";
				break;
			case "DATALINK":
				retval="java.lang.Byte";
				break;
			case "USER_DEFINED_TYPE":
				retval="java.lang.Byte";
				break;
			case "SHORT_DATATYPE":
				retval="java.lang.Short";
				break;
			case "TINY_DATATYPE":
				retval="java.lang.Short";
				break;
	
			// added by firebird
			case "D_FLOAT":
				retval="java.lang.Double";
				break;
			case "ARRAY":
				retval="java.lang.Byte";
				break;
			case "QUAD":
				retval="java.lang.BigInteger";
				break;
			case "INT64":
				retval="java.lang.BigInteger";
				break;
			case "DOUBLE PRECISION":
				retval="java.lang.Double";
				break;
	
			// added by postgresql
			case "BOOL":
				retval="java.lang.String";
				break;
			case "BYTEA":
				retval="java.lang.Byte";
				break;
			case "NAME":
				retval="java.lang.String";
				break;
			case "INT8":
				retval="java.lang.BigInteger";
				break;
			case "INT2":
				retval="java.lang.Short";
				break;
			case "INT2VECTOR":
				retval="java.lang.Byte";
				break;
			case "INT4":
				retval="java.lang.Integer";
				break;
			case "REGPROC":
				retval="java.lang.BigInteger";
				break;
			case "OID":
				retval="java.lang.BigInteger";
				break;
			case "TID":
				retval="java.lang.BigInteger";
				break;
			case "XID":
				retval="java.lang.BigInteger";
				break;
			case "CID":
				retval="java.lang.BigInteger";
				break;
			case "OIDVECTOR":
				retval="java.lang.Byte";
				break;
			case "SMGR":
				retval="java.lang.Byte";
				break;
			case "POINT":
				retval="java.lang.Byte";
				break;
			case "LSEG":
				retval="java.lang.Byte";
				break;
			case "PATH":
				retval="java.lang.Byte";
				break;
			case "BOX":
				retval="java.lang.Byte";
				break;
			case "POLYGON":
				retval="java.lang.Byte";
				break;
			case "LINE":
				retval="java.lang.Byte";
				break;
			case "LINE_ARRAY":
				retval="java.lang.Byte";
				break;
			case "FLOAT4":
				retval="java.lang.Float";
				break;
			case "FLOAT8":
				retval="java.lang.Double";
				break;
			case "ABSTIME":
				retval="java.lang.Integer";
				break;
			case "RELTIME":
				retval="java.lang.Integer";
				break;
			case "TINTERVAL":
				retval="java.lang.Byte";
				break;
			case "CIRCLE":
				retval="java.lang.Byte";
				break;
			case "CIRCLE_ARRAY":
				retval="java.lang.Byte";
				break;
			case "MONEY_ARRAY":
				retval="java.lang.Byte";
				break;
			case "MACADDR":
				retval="java.lang.Byte";
				break;
			case "INET":
				retval="java.lang.Byte";
				break;
			case "CIDR":
				retval="java.lang.Byte";
				break;
			case "BOOL_ARRAY":
				retval="java.lang.Byte";
				break;
			case "BYTEA_ARRAY":
				retval="java.lang.Byte";
				break;
			case "CHAR_ARRAY":
				retval="java.lang.Byte";
				break;
			case "NAME_ARRAY":
				retval="java.lang.Byte";
				break;
			case "INT2_ARRAY":
				retval="java.lang.Byte";
				break;
			case "INT2VECTOR_ARRAY":
				retval="java.lang.Byte";
				break;
			case "INT4_ARRAY":
				retval="java.lang.Byte";
				break;
			case "REGPROC_ARRAY":
				retval="java.lang.Byte";
				break;
			case "TEXT_ARRAY":
				retval="java.lang.Byte";
				break;
			case "OID_ARRAY":
				retval="java.lang.Byte";
				break;
			case "TID_ARRAY":
				retval="java.lang.Byte";
				break;
			case "XID_ARRAY":
				retval="java.lang.Byte";
				break;
			case "CID_ARRAY":
				retval="java.lang.Byte";
				break;
			case "OIDVECTOR_ARRAY":
				retval="java.lang.Byte";
				break;
			case "BPCHAR_ARRAY":
				retval="java.lang.Byte";
				break;
			case "VARCHAR_ARRAY":
				retval="java.lang.Byte";
				break;
			case "INT8_ARRAY":
				retval="java.lang.Byte";
				break;
			case "POINT_ARRAY":
				retval="java.lang.Byte";
				break;
			case "LSEG_ARRAY":
				retval="java.lang.Byte";
				break;
			case "PATH_ARRAY":
				retval="java.lang.Byte";
				break;
			case "BOX_ARRAY":
				retval="java.lang.Byte";
				break;
			case "FLOAT4_ARRAY":
				retval="java.lang.Byte";
				break;
			case "FLOAT8_ARRAY":
				retval="java.lang.Byte";
				break;
			case "ABSTIME_ARRAY":
				retval="java.lang.Byte";
				break;
			case "RELTIME_ARRAY":
				retval="java.lang.Byte";
				break;
			case "TINTERVAL_ARRAY":
				retval="java.lang.Byte";
				break;
			case "POLYGON_ARRAY":
				retval="java.lang.Byte";
				break;
			case "ACLITEM":
				retval="java.lang.Byte";
				break;
			case "ACLITEM_ARRAY":
				retval="java.lang.Byte";
				break;
			case "MACADDR_ARRAY":
				retval="java.lang.Byte";
				break;
			case "INET_ARRAY":
				retval="java.lang.Byte";
				break;
			case "CIDR_ARRAY":
				retval="java.lang.Byte";
				break;
			case "BPCHAR":
				retval="java.lang.String";
				break;
			case "TIMESTAMP_ARRAY":
				retval="java.lang.Byte";
				break;
			case "DATE_ARRAY":
				retval="java.lang.Byte";
				break;
			case "TIME_ARRAY":
				retval="java.lang.Byte";
				break;
			case "TIMESTAMPTZ":
				retval="java.sql.Timestamp";
				break;
			case "TIMESTAMPTZ_ARRAY":
				retval="java.lang.Byte";
				break;
			case "INTERVAL":
				retval="java.lang.Byte";
				break;
			case "INTERVAL_ARRAY":
				retval="java.lang.Byte";
				break;
			case "NUMERIC_ARRAY":
				retval="java.lang.Byte";
				break;
			case "TIMETZ":
				retval="java.sql.Time";
				break;
			case "TIMETZ_ARRAY":
				retval="java.lang.Byte";
				break;
			case "BIT_ARRAY":
				retval="java.lang.Byte";
				break;
			case "VARBIT":
				retval="java.lang.Byte";
				break;
			case "VARBIT_ARRAY":
				retval="java.lang.Byte";
				break;
			case "REFCURSOR":
				retval="java.lang.Byte";
				break;
			case "REFCURSOR_ARRAY":
				retval="java.lang.Byte";
				break;
			case "REGPROCEDURE":
				retval="java.lang.Byte";
				break;
			case "REGOPER":
				retval="java.lang.Byte";
				break;
			case "REGOPERATOR":
				retval="java.lang.Byte";
				break;
			case "REGCLASS":
				retval="java.lang.Byte";
				break;
			case "REGTYPE":
				retval="java.lang.Byte";
				break;
			case "REGPROCEDURE_ARRAY":
				retval="java.lang.Byte";
				break;
			case "REGOPER_ARRAY":
				retval="java.lang.Byte";
				break;
			case "REGOPERATOR_ARRAY":
				retval="java.lang.Byte";
				break;
			case "REGCLASS_ARRAY":
				retval="java.lang.Byte";
				break;
			case "REGTYPE_ARRAY":
				retval="java.lang.Byte";
				break;
			case "RECORD":
				retval="java.lang.Byte";
				break;
			case "CSTRING":
				retval="java.lang.String";
				break;
			case "ANY":
				retval="java.lang.Byte";
				break;
			case "ANYARRAY":
				retval="java.lang.Byte";
				break;
			case "TRIGGER":
				retval="java.lang.Byte";
				break;
			case "LANGUAGE_HANDLER":
				retval="java.lang.Byte";
				break;
			case "INTERNAL":
				retval="java.lang.Byte";
				break;
			case "OPAQUE":
				retval="java.lang.Byte";
				break;
			case "ANYELEMENT":
				retval="java.lang.Byte";
				break;
			case "PG_TYPE":
				retval="java.lang.Byte";
				break;
			case "PG_ATTRIBUTE":
				retval="java.lang.Byte";
				break;
			case "PG_PROC":
				retval="java.lang.Byte";
				break;
			case "PG_CLASS":
				retval="java.lang.Byte";
				break;

			// none added by sqlite

			// added by sqlserver
			case "UBIGINT":
				retval="java.lang.BigInteger";
				break;
			case "UNIQUEIDENTIFIER":
				retval="java.lang.Byte";
				break;

			// added by informix
			case "SMALLFLOAT":
				retval="java.lang.Float";
				break;
			case "BYTE":
				retval="java.lang.Byte";
				break;
			case "BOOLEAN":
				retval="java.lang.String";
				break;

			// also added by mysql
			case "TINYTEXT":
				retval="java.lang.String";
				break;
			case "MEDIUMTEXT":
				retval="java.lang.String";
				break;
			case "LONGTEXT":
				retval="java.lang.String";
				break;
			case "JSON":
				retval="java.lang.String";
				break;
			case "GEOMETRY":
				retval="java.lang.Byte";
				break;

			// also added by oracle
			case "SDO_GEOMETRY":
				retval="java.lang.Byte";
				break;

			// added by mssql
			case "NCHAR":
				retval="java.lang.String";
				break;
			case "NVARCHAR":
				retval="java.lang.String";
				break;
			case "NTEXT":
				retval="java.lang.String";
				break;
			case "XML":
				retval="java.lang.String";
				break;
			case "DATETIMEOFFSET":
				retval="java.sql.Timestamp";
				break;
		}
		drv.debugPrintln("class type: "+retval);
		drv.debugEnd();
		return retval;
	}

	public
	int getColumnCount() {
		drv.debugFunction(this);
		int	colcount=sqlrcur.colCount();
		drv.debugPrintln("colcount: "+colcount);
		drv.debugEnd();
		return colcount;
	}

	public
	int getColumnDisplaySize(int column) {
		drv.debugFunction(this);
		drv.debugPrintln("column: "+column);
		String	ctype=normalizeTypeName(
					sqlrcur.getColumnType(column-1));
		drv.debugPrintln("ctype: "+ctype);
		int	size=0;
		if (sqlrcur.isNumberType(ctype)) {
			// FIXME: not sure about this for ALL number types,
			// also this is just what oracle returns, and might
			// not be right for other dbs
			size=39;
		} else if (sqlrcur.isDateTimeType(ctype)) {
			// FIXME: probably need to discern between
			// timestamp, date, and time
			size=7;
		} else if (sqlrcur.isClobType(ctype) ||
				sqlrcur.isBlobType(ctype)) {
			// FIXME: this matches what oracle does,
			// but might not be right for other dbs
			if (ctype.equals("LONG")) {
				size=0;
			} else {
				size=4000;
			}
		} else {
			size=sqlrcur.getColumnLength(column-1);
		}
		drv.debugPrintln("size: "+size);
		drv.debugEnd();
		return size;
	}

	public
	String getColumnLabel(int column) {
		drv.debugFunction(this);
		String	label=sqlrcur.getColumnName(column-1);
		drv.debugPrintln("column: "+column);
		drv.debugPrintln("label: "+label);
		drv.debugEnd();
		return label;
	}

	public
	String getColumnName(int column) {
		drv.debugFunction(this);
		String	columnname=sqlrcur.getColumnName(column-1);
		drv.debugPrintln("column: "+column);
		drv.debugPrintln("column name: "+columnname);
		drv.debugEnd();
		return columnname;
	}

	public
	int getColumnType(int column) throws SQLException {
		drv.debugFunction(this);
		drv.debugPrintln("column: "+column);
		int	retval=0;
		String	ctype=normalizeTypeName(
					sqlrcur.getColumnType(column-1));
		drv.debugPrintln("ctype: "+ctype);
		switch (ctype) {
			case "UNKNOWN":
			case "UNDEFINED":
				retval=Types.OTHER;
				break;
			case "CHAR":
			case "MONEY":
			case "SMALLMONEY":
			case "TEXT":
			case "LONGCHAR":
			case "LONG":
			case "ILLEGAL":
			case "SENSITIVITY":
			case "BOUNDARY":
			case "VOID":
			case "STRING":
			case "NULL":
			case "ENUM":
			case "SET":
			case "BOOL":
			case "NAME":
			case "BPCHAR":
			case "CSTRING":
			case "BOOLEAN":
				retval=Types.CHAR;
				break;
			case "INT":
			case "UINT":
			case "MEDIUMINT":
			case "INTEGER":
			case "INT4":
			case "ABSTIME":
			case "RELTIME":
				retval=Types.INTEGER;
				break;
			case "SMALLINT":
			case "USHORT":
			case "YEAR":
			case "SHORT_DATATYPE":
			case "INT2":
				retval=Types.SMALLINT;
				break;
			case "TINYINT":
			case "TINY_DATATYPE":
				retval=Types.TINYINT;
				break;
			case "DATETIME":
			case "SMALLDATETIME":
			case "TIMESTAMP":
			case "TIMESTAMPTZ":
			case "DATETIMEOFFSET":
				retval=Types.TIMESTAMP;
				break;
			case "NUMERIC":
			case "NUMBER":
				retval=Types.NUMERIC;
				break;
			case "DECIMAL":
				retval=Types.DECIMAL;
				break;
			case "IMAGE":
			case "BINARY":
			case "LONGBINARY":
			case "MLSLABEL":
			case "GRAPHIC":
			case "DATALINK":
			case "USER_DEFINED_TYPE":
			case "ARRAY":
			case "BYTEA":
			case "INT2VECTOR":
			case "OIDVECTOR":
			case "SMGR":
			case "POINT":
			case "LSEG":
			case "PATH":
			case "BOX":
			case "POLYGON":
			case "LINE":
			case "LINE_ARRAY":
			case "TINTERVAL":
			case "CIRCLE":
			case "CIRCLE_ARRAY":
			case "MONEY_ARRAY":
			case "MACADDR":
			case "INET":
			case "CIDR":
			case "BOOL_ARRAY":
			case "BYTEA_ARRAY":
			case "CHAR_ARRAY":
			case "NAME_ARRAY":
			case "INT2_ARRAY":
			case "INT2VECTOR_ARRAY":
			case "INT4_ARRAY":
			case "REGPROC_ARRAY":
			case "TEXT_ARRAY":
			case "OID_ARRAY":
			case "TID_ARRAY":
			case "XID_ARRAY":
			case "CID_ARRAY":
			case "OIDVECTOR_ARRAY":
			case "BPCHAR_ARRAY":
			case "VARCHAR_ARRAY":
			case "INT8_ARRAY":
			case "POINT_ARRAY":
			case "LSEG_ARRAY":
			case "PATH_ARRAY":
			case "BOX_ARRAY":
			case "FLOAT4_ARRAY":
			case "FLOAT8_ARRAY":
			case "ABSTIME_ARRAY":
			case "RELTIME_ARRAY":
			case "TINTERVAL_ARRAY":
			case "POLYGON_ARRAY":
			case "ACLITEM":
			case "ACLITEM_ARRAY":
			case "MACADDR_ARRAY":
			case "INET_ARRAY":
			case "CIDR_ARRAY":
			case "TIMESTAMP_ARRAY":
			case "DATE_ARRAY":
			case "TIME_ARRAY":
			case "TIMESTAMPTZ_ARRAY":
			case "INTERVAL":
			case "INTERVAL_ARRAY":
			case "NUMERIC_ARRAY":
			case "TIMETZ_ARRAY":
			case "BIT_ARRAY":
			case "VARBIT":
			case "VARBIT_ARRAY":
			case "REFCURSOR":
			case "REFCURSOR_ARRAY":
			case "REGPROCEDURE":
			case "REGOPER":
			case "REGOPERATOR":
			case "REGCLASS":
			case "REGTYPE":
			case "REGPROCEDURE_ARRAY":
			case "REGOPER_ARRAY":
			case "REGOPERATOR_ARRAY":
			case "REGCLASS_ARRAY":
			case "REGTYPE_ARRAY":
			case "RECORD":
			case "ANY":
			case "ANYARRAY":
			case "TRIGGER":
			case "LANGUAGE_HANDLER":
			case "INTERNAL":
			case "OPAQUE":
			case "ANYELEMENT":
			case "PG_TYPE":
			case "PG_ATTRIBUTE":
			case "PG_PROC":
			case "PG_CLASS":
			case "UNIQUEIDENTIFIER":
			case "BYTE":
			case "GEOMETRY":
			case "SDO_GEOMETRY":
				retval=Types.BINARY;
				break;
			case "BIT":
				retval=Types.BIT;
				break;
			case "REAL":
			case "LASTREAL":
				retval=Types.REAL;
				break;
			case "FLOAT":
			case "FLOAT4":
			case "SMALLFLOAT":
				retval=Types.FLOAT;
				break;
			case "VARCHAR":
			case "VARSTRING":
			case "VARCHAR2":
			case "LVARCHAR":
				retval=Types.VARCHAR;
				break;
			case "VARBINARY":
			case "RAW":
			case "VARGRAPHIC":
				retval=Types.VARBINARY;
				break;
			case "DOUBLE":
			case "D_FLOAT":
			case "DOUBLE PRECISION":
			case "FLOAT8":
				retval=Types.DOUBLE;
				break;
			case "DATE":
				retval=(getDateToTimestamp())?
						Types.TIMESTAMP:
						Types.DATE;
				break;
			case "NEWDATE":
				retval=(getDateToTimestamp())?
						Types.TIMESTAMP:
						Types.DATE;
				break;
			case "TIME":
			case "TIMETZ":
				retval=Types.TIME;
				break;
			case "LONGLONG":
			case "ROWID":
			case "BIGINT":
			case "QUAD":
			case "INT64":
			case "INT8":
			case "REGPROC":
			case "OID":
			case "TID":
			case "XID":
			case "CID":
			case "UBIGINT":
				retval=Types.BIGINT;
				break;
			case "LONG_RAW":
			case "BFILE":
			case "LONGVARBINARY":
			case "LONGVARGRAPHIC":
				retval=Types.LONGVARBINARY;
				break;
			case "CLOB":
			case "LONGVARCHAR":
			case "DBCLOB":
			case "TINYTEXT":
			case "MEDIUMTEXT":
			case "LONGTEXT":
			case "JSON":
			case "XML":
				retval=Types.LONGVARCHAR;
				break;
			case "TINYBLOB":
			case "MEDIUMBLOB":
			case "LONGBLOB":
			case "BLOB":
				boolean	binary=sqlrcur.
						getColumnIsBinary(column-1);
				drv.debugPrintln("is binary: "+binary);
				retval=(binary)?Types.BINARY:Types.LONGVARCHAR;
				break;
			case "NCHAR":
				retval=Types.NCHAR;
				break;
			case "NVARCHAR":
				retval=Types.NVARCHAR;
				break;
			case "NTEXT":
				retval=Types.LONGNVARCHAR;
				break;
			default:
				break;
		}
		drv.debugPrintln("sql type: "+retval);
		drv.debugEnd();
		return retval;
	}

	private
	boolean getDateToTimestamp() throws SQLException {
		return resultset.getStatement().
					getConnection().
					getDateToTimestamp();
	}

	public
	String getColumnTypeName(int column) {
		drv.debugFunction(this);
		String	typename=sqlrcur.getColumnType(column-1);
		drv.debugPrintln("column: "+column);
		drv.debugPrintln("type name: "+typename);
		drv.debugEnd();
		return typename;
	}

	public
	int getPrecision(int column) {
		drv.debugFunction(this);
		drv.debugPrintln("column: "+column);
		String	ctype=normalizeTypeName(
					sqlrcur.getColumnType(column-1));
		drv.debugPrintln("ctype: "+ctype);
		int	precision=0;
		if (sqlrcur.isNumberType(ctype)) {
			precision=(int)sqlrcur.getColumnPrecision(column-1);
		} else if (sqlrcur.isDateTimeType(ctype)) {
			// FIXME: probably need to discern between
			// timestamp, date, and time
			precision=7;
		} else if (sqlrcur.isClobType(ctype) ||
				sqlrcur.isBlobType(ctype)) {
			// FIXME: this matches what oracle does,
			// but might not be right for other dbs
			if (ctype.equals("LONG")) {
				precision=2147483647;
			} else {
				precision=-1;
			}
		} else {
			precision=sqlrcur.getColumnLength(column-1);
		}
		drv.debugPrintln("precision: "+precision);
		drv.debugEnd();
		return precision;
	}

	public
	int getScale(int column) {
		drv.debugFunction(this);
		int	scale=(int)sqlrcur.getColumnScale(column-1);
		drv.debugPrintln("column: "+column);
		drv.debugPrintln("scale: "+scale);
		drv.debugEnd();
		return scale;
	}

	public
	String getSchemaName(int column) {
		drv.debugFunction(this);
		String	schemaname="";
		drv.debugPrintln("column: "+column);
		drv.debugPrintln("schema name: "+schemaname);
		drv.debugEnd();
		return schemaname;
	}

	public
	String getTableName(int column) {
		drv.debugFunction(this);
		String	tablename="";
		drv.debugPrintln("column: "+column);
		drv.debugPrintln("table name: "+tablename);
		// FIXME: this could be implemented if
		// getColumnTable was exposed
		drv.debugEnd();
		return tablename;
	}

	public
	boolean isAutoIncrement(int column) {
		drv.debugFunction(this);
		boolean	isautoinc=sqlrcur.getColumnIsAutoIncrement(column-1);
		drv.debugPrintln("column: "+column);
		drv.debugPrintln("is auto increment: "+isautoinc);
		drv.debugEnd();
		return isautoinc;
	}

	public
	boolean isCaseSensitive(int column) {
		drv.debugFunction(this);
		// FIXME: can db type tell us this?
		boolean	iscasesensitive=false;
		drv.debugPrintln("column: "+column);
		drv.debugPrintln("is case sensitive: "+iscasesensitive);
		drv.debugEnd();
		return iscasesensitive;
	}

	public
	boolean isCurrency(int column) {
		drv.debugFunction(this);
		String	ctype=normalizeTypeName(
					sqlrcur.getColumnType(column-1));
		boolean	iscurrency=ctype.equals("MONEY") ||
					ctype.equals("SMALLMONEY") ||
					ctype.equals("MONEY_ARRAY");
		drv.debugPrintln("column: "+column);
		drv.debugPrintln("ctype: "+ctype);
		drv.debugPrintln("is currency: "+iscurrency);
		drv.debugEnd();
		return iscurrency;
	}

	public
	boolean isDefinitelyWritable(int column) {
		drv.debugFunction(this);
		boolean	isdefinitelywriteable=false;
		drv.debugPrintln("column: "+column);
		drv.debugPrintln("is definitely writeable: "+
					isdefinitelywriteable);
		drv.debugEnd();
		return isdefinitelywriteable;
	}

	public
	int isNullable(int column) {
		drv.debugFunction(this);
		int	isnullable=(sqlrcur.getColumnIsNullable(column-1))?
						columnNullable:columnNoNulls;
		drv.debugPrintln("column: "+column);
		drv.debugPrintln("is nullable: "+isnullable);
		drv.debugEnd();
		return isnullable;
	}

	public
	boolean isReadOnly(int column) {
		drv.debugFunction(this);
		boolean	isreadonly=false;
		drv.debugPrintln("column: "+column);
		drv.debugPrintln("is read-only: "+isreadonly);
		drv.debugEnd();
		return isreadonly;
	}

	public
	boolean isSearchable(int column) {
		drv.debugFunction(this);
		boolean	issearchable=true;
		drv.debugPrintln("column: "+column);
		drv.debugPrintln("is searchable: "+issearchable);
		drv.debugEnd();
		return issearchable;
	}

	public
	boolean isSigned(int column) {
		drv.debugFunction(this);
		boolean	issigned=!sqlrcur.getColumnIsUnsigned(column-1);
		drv.debugPrintln("column: "+column);
		drv.debugPrintln("is signed: "+issigned);
		drv.debugEnd();
		return issigned;
	}

	public
	boolean isWritable(int column) {
		drv.debugFunction(this);
		boolean	iswriteable=true;
		drv.debugPrintln("column: "+column);
		drv.debugPrintln("is writeable: "+iswriteable);
		drv.debugEnd();
		return iswriteable;
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
