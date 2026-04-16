// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/sys.h>
#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>

#include "../../config.h"

#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>
#include <sqltypes.h>

#include "asserts.cpp"

SQLRETURN	erg;
SQLHENV		env;
SQLHDBC		dbc;
SQLHSTMT	stmt;

int main(int argc, char **argv) {

	// hostname
	char    *hostname=sys::getHostName();
	char    *dot=(char *)charstring::findFirstOrEnd(hostname,'.');
	*dot='\0';


	// environment handle
	stdoutput.printf("ENVIRONMENT HANDLE: \n");
	#if (ODBCVER >= 0x3000)
		erg=SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&env);
		assertSuccessEnv(env,erg);
		#if defined(SQL_OV_ODBC3_80)
			erg=SQLSetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
						(SQLPOINTER)SQL_OV_ODBC3_80,0);
			assertSuccessEnv(env,erg);
		#elif defined(SQL_OV_ODBC3)
			erg=SQLSetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
						(SQLPOINTER)SQL_OV_ODBC3,0);
			assertSuccessEnv(env,erg);
		#else
			erg=SQLSetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
						(SQLPOINTER)SQL_OV_ODBC2,0);
			assertSuccessEnv(env,erg);
		#endif
	#else
		erg=SQLAllocEnv(&env);
		assertSuccessEnv(env,erg);
	#endif
	stdoutput.printf("\n");


	// environment attributes
	// FIXME:...


	// connection handle
	stdoutput.printf("CONNECTION HANDLE: \n");
	#if (ODBCVER >= 0x0300)
		erg=SQLAllocHandle(SQL_HANDLE_DBC,env,&dbc);
		assertSuccessEnv(env,erg);
	#else
		erg=SQLAllocConnect(env,&dbc);
		assertSuccessEnv(env,erg);
	#endif
	stdoutput.printf("\n");


	// connection attributes
	// FIXME:...


	// connect
	stdoutput.printf("CONNECT: \n");
	bool	issqlrelay=!(argc==2 && !charstring::compare(argv[1],"native"));
	SQLCHAR	*dsn;
	SQLCHAR	*user;
	SQLCHAR	*password;
	SQLCHAR	*incstring;
	if (issqlrelay) {
		incstring=(SQLCHAR *)
			"Driver={SQL Relay};"
			"Server=sqlrelay;Port=9000;"
			"Socket=/tmp/test.socket;"
			"User=testuser;Password=testpassword;"
			"NullsAsNulls=yes";
		SQLCHAR		outcstring[1024];
		SQLSMALLINT	outcstringlen;
		erg=SQLDriverConnect(dbc,NULL,
				incstring,SQL_NTS,
				outcstring,sizeof(outcstring),&outcstringlen,
				SQL_DRIVER_NOPROMPT);
	} else {
		dsn=(SQLCHAR *)"oracle";
		user=(SQLCHAR *)hostname;
		password=(SQLCHAR *)"testpassword";
		erg=SQLConnect(dbc,dsn,SQL_NTS,user,SQL_NTS,password,SQL_NTS);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// get info
	stdoutput.printf("GET INFO: \n");
	SQLUINTEGER	uintval;
	SQLCHAR		strval[2048];
	SQLSMALLINT	vallen;
	erg=SQLGetInfo(dbc,SQL_DBMS_NAME,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"oracle");
	} else {
		assertEqualDbc(dbc,(const char *)strval,"Oracle");
	}
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DEFAULT_TXN_ISOLATION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_TXN_READ_COMMITTED);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// isolation levels
	stdoutput.printf("ISOLATION LEVELS: \n");

	// you can set the isolation level, but to get it, you
	// have to have permisisons to read from sys.v_$session
	// and sys.v_$transaction, so we'll just set them here
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_READ_UNCOMMITTED,0);
	assertFailureDbc(dbc,erg);

	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_READ_COMMITTED,0);
	assertSuccessDbc(dbc,erg);

	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_REPEATABLE_READ,0);
	assertFailureDbc(dbc,erg);

	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_SERIALIZABLE,0);
	assertSuccessDbc(dbc,erg);

	// reset to default isolation level
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_READ_COMMITTED,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// statement handle
	stdoutput.printf("STATEMENT HANDLE: \n");
	erg=SQLAllocHandle(SQL_HANDLE_STMT,dbc,&stmt);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// statement attributes
	// FIXME:...


	// create testtable
	stdoutput.printf("CREATE TESTTABLE: \n");
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
		(SQLPOINTER)SQL_AUTOCOMMIT_OFF,0);
	assertSuccessDbc(dbc,erg);
	SQLExecDirect(stmt,
		(SQLCHAR *)"drop table testtable",
		SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable ("
		"	testnumber number, "
		"	testchar char(40), "
		"	testvarchar varchar2(40), "
		"	testdate date, "
		"	testlong long, "
		"	testclob clob, "
		"	testblob blob)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// insert
	stdoutput.printf("INSERT: \n");
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	'testchar1', "
		"	'testvarchar1', "
		"	'01-JAN-2001', "
		"	'testlong1', "
		"	'testclob1', "
		"	empty_blob())",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// affected rows
	stdoutput.printf("AFFECTED ROWS: \n");
	#ifdef SQLROWCOUNT_SQLLEN
	SQLLEN		affectedrows;
	#else
	SQLINTEGER	affectedrows;
	#endif
	erg=SQLRowCount(stmt,&affectedrows);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)affectedrows,1);
	stdoutput.printf("\n");


	// input bind by position
	stdoutput.printf("INPUT BIND BY POSITION: \n");
	erg=SQLFreeStmt(stmt,SQL_CLOSE);
	erg=SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	if (issqlrelay) {
		erg=SQLPrepare(stmt,(SQLCHAR *)
			"insert into "
			"	testtable "
			"values ( "
			"	:1, "
			"	:2, "
			"	:3, "
			"	:4, "
			"	:5, "
			"	:6, "
			"	:7)",
			SQL_NTS);
	} else {
		erg=SQLPrepare(stmt,(SQLCHAR *)
			"insert into "
			"	testtable "
			"values ( "
			"	?, "
			"	?, "
			"	?, "
			"	?, "
			"	?, "
			"	?, "
			"	?)",
			SQL_NTS);
	}
	assertSuccessStmt(stmt,erg);
	SQLSMALLINT	bindvarcount;
	erg=SQLNumParams(stmt,&bindvarcount);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)bindvarcount,7);

	// FIXME: why the different types?  can we bind DATE_STRUCT?
	SQLSMALLINT	datesqltype=(issqlrelay)?SQL_TYPE_DATE:SQL_CHAR;

	// OCI appears to convert data bound to clob columns to UCS2, even if
	// the clob column isn't an NCLOB.  It does the conversion in-place, in
	// the buffer that the value is stored in, so if the buffer contains
	// ascii data, then it needs to be at least 4 times as large as the
	// length of the data.  The buffer also needs to be writable, it can't
	// just be a string constant.  The value passed in to the bufferlength
	// parameter needs to be the size of the buffer, in bytes, not just the
	// length of the string in the buffer.  That's why clobval and cloblen
	// below are like they are, and why we're copying into clobval.

	SQLINTEGER	intval;
	SQLCHAR		*charval;
	SQLCHAR		*varcharval;
	SQLCHAR		*dateval;
	SQLCHAR		*longval;
	SQLCHAR		clobval[40];
	SQLCHAR		*blobval;
	SQLLEN		intlen=sizeof(SQLINTEGER);
	SQLLEN		charlen=SQL_NTS;
	SQLLEN		varcharlen=SQL_NTS;
	SQLLEN		datelen=SQL_NTS;
	SQLLEN		longlen=SQL_NTS;
	SQLLEN		cloblen=sizeof(clobval);
	SQLLEN		bloblen=9;

	intval=2;
	charval=(SQLCHAR *)"testchar2";
	varcharval=(SQLCHAR *)"testvarchar2";
	dateval=(SQLCHAR *)"01-JAN-2002";
	longval=(SQLCHAR *)"testlong2";
	charstring::copy((char *)clobval,"testclob2");
	blobval=(SQLCHAR *)"testblob2";
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				intlen,&intlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_CHAR,
				40,0,
				(SQLPOINTER)charval,
				0,&charlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_VARCHAR,
				40,0,
				(SQLPOINTER)varcharval,
				0,&varcharlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,4,SQL_PARAM_INPUT,
				SQL_C_CHAR,datesqltype,
				11,0,
				(SQLPOINTER)dateval,
				0,&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,5,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)longval,
				0,&longlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,6,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)clobval,
				cloblen,&cloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,7,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				0,0,
				(SQLPOINTER)blobval,
				bloblen,&bloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);

	intval=3;
	charval=(SQLCHAR *)"testchar3";
	varcharval=(SQLCHAR *)"testvarchar3";
	dateval=(SQLCHAR *)"01-JAN-2003";
	longval=(SQLCHAR *)"testlong3";
	charstring::copy((char *)clobval,"testclob3");
	blobval=(SQLCHAR *)"testblob3";
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				intlen,&intlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_CHAR,
				40,0,
				(SQLPOINTER)charval,
				0,&charlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_VARCHAR,
				40,0,
				(SQLPOINTER)varcharval,
				0,&varcharlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,4,SQL_PARAM_INPUT,
				SQL_C_CHAR,datesqltype,
				11,0,
				(SQLPOINTER)dateval,
				0,&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,5,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)longval,
				0,&longlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,6,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)clobval,
				cloblen,&cloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,7,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				0,0,
				(SQLPOINTER)blobval,
				bloblen,&bloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);

	intval=4;
	charval=(SQLCHAR *)"testchar4";
	varcharval=(SQLCHAR *)"testvarchar4";
	dateval=(SQLCHAR *)"01-JAN-2004";
	longval=(SQLCHAR *)"testlong4";
	charstring::copy((char *)clobval,"testclob4");
	blobval=(SQLCHAR *)"testblob4";
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				intlen,&intlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_CHAR,
				40,0,
				(SQLPOINTER)charval,
				0,&charlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_VARCHAR,
				40,0,
				(SQLPOINTER)varcharval,
				0,&varcharlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,4,SQL_PARAM_INPUT,
				SQL_C_CHAR,datesqltype,
				11,0,
				(SQLPOINTER)dateval,
				0,&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,5,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)longval,
				0,&longlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,6,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)clobval,
				cloblen,&cloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,7,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				0,0,
				(SQLPOINTER)blobval,
				bloblen,&bloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);

	intval=5;
	charval=(SQLCHAR *)"testchar5";
	varcharval=(SQLCHAR *)"testvarchar5";
	dateval=(SQLCHAR *)"01-JAN-2005";
	longval=(SQLCHAR *)"testlong5";
	charstring::copy((char *)clobval,"testclob5");
	blobval=(SQLCHAR *)"testblob5";
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				intlen,&intlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_CHAR,
				40,0,
				(SQLPOINTER)charval,
				0,&charlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_VARCHAR,
				40,0,
				(SQLPOINTER)varcharval,
				0,&varcharlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,4,SQL_PARAM_INPUT,
				SQL_C_CHAR,datesqltype,
				11,0,
				(SQLPOINTER)dateval,
				0,&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,5,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)longval,
				0,&longlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,6,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)clobval,
				cloblen,&cloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,7,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				0,0,
				(SQLPOINTER)blobval,
				bloblen,&bloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);

	intval=6;
	charval=(SQLCHAR *)"testchar6";
	varcharval=(SQLCHAR *)"testvarchar6";
	dateval=(SQLCHAR *)"01-JAN-2006";
	longval=(SQLCHAR *)"testlong6";
	charstring::copy((char *)clobval,"testclob6");
	blobval=(SQLCHAR *)"testblob6";
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				intlen,&intlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_CHAR,
				40,0,
				(SQLPOINTER)charval,
				0,&charlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_VARCHAR,
				40,0,
				(SQLPOINTER)varcharval,
				0,&varcharlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,4,SQL_PARAM_INPUT,
				SQL_C_CHAR,datesqltype,
				11,0,
				(SQLPOINTER)dateval,
				0,&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,5,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)longval,
				0,&longlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,6,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)clobval,
				cloblen,&cloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,7,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				0,0,
				(SQLPOINTER)blobval,
				bloblen,&bloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);

	intval=7;
	charval=(SQLCHAR *)"testchar7";
	varcharval=(SQLCHAR *)"testvarchar7";
	dateval=(SQLCHAR *)"01-JAN-2007";
	longval=(SQLCHAR *)"testlong7";
	charstring::copy((char *)clobval,"testclob7");
	blobval=(SQLCHAR *)"testblob7";
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				intlen,&intlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_CHAR,
				40,0,
				(SQLPOINTER)charval,
				0,&charlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_VARCHAR,
				40,0,
				(SQLPOINTER)varcharval,
				0,&varcharlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,4,SQL_PARAM_INPUT,
				SQL_C_CHAR,datesqltype,
				11,0,
				(SQLPOINTER)dateval,
				0,&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,5,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)longval,
				0,&longlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,6,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)clobval,
				cloblen,&cloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,7,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				0,0,
				(SQLPOINTER)blobval,
				bloblen,&bloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);

	intval=8;
	charval=(SQLCHAR *)"testchar8";
	varcharval=(SQLCHAR *)"testvarchar8";
	dateval=(SQLCHAR *)"01-JAN-2008";
	longval=(SQLCHAR *)"testlong8";
	charstring::copy((char *)clobval,"testclob8");
	blobval=(SQLCHAR *)"testblob8";
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				intlen,&intlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_CHAR,
				40,0,
				(SQLPOINTER)charval,
				0,&charlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_VARCHAR,
				40,0,
				(SQLPOINTER)varcharval,
				0,&varcharlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,4,SQL_PARAM_INPUT,
				SQL_C_CHAR,datesqltype,
				11,0,
				(SQLPOINTER)dateval,
				0,&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,5,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)longval,
				0,&longlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,6,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)clobval,
				cloblen,&cloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,7,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				0,0,
				(SQLPOINTER)blobval,
				bloblen,&bloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// select
	stdoutput.printf("SELECT: \n");
	erg=SQLFreeStmt(stmt,SQL_CLOSE);
	erg=SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// column count
	stdoutput.printf("COLUMN COUNT: \n");
	SQLSMALLINT	colcount;
	erg=SQLNumResultCols(stmt,&colcount);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)colcount,7);
	stdoutput.printf("\n");


	// column names
	stdoutput.printf("COLUMN NAMES: \n");
	SQLCHAR		colname[256];
	SQLSMALLINT	colnamelen;
	SQLSMALLINT	datatype;
	SQLULEN		colsize;
	SQLSMALLINT	decdigits;
	SQLSMALLINT	nullable;
	erg=SQLDescribeCol(stmt,1,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTNUMBER");
	erg=SQLDescribeCol(stmt,2,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTCHAR");
	erg=SQLDescribeCol(stmt,3,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTVARCHAR");
	erg=SQLDescribeCol(stmt,4,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTDATE");
	erg=SQLDescribeCol(stmt,5,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTLONG");
	erg=SQLDescribeCol(stmt,6,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTCLOB");
	erg=SQLDescribeCol(stmt,7,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTBLOB");
	stdoutput.printf("\n");


	// column types
	// FIXME:...


	// column length
	// FIXME:...


	// fetch rows
	stdoutput.printf("FETCH ROWS: \n");
	SQLINTEGER	numval;
	SQLCHAR		charfield[41];
	SQLCHAR		varcharfield[41];
	SQL_DATE_STRUCT	datefield;
	SQLCHAR		longfield[256];
	SQLCHAR		clobfield[256];
	SQLCHAR		blobfield[256];
	SQLLEN		numind;
	SQLLEN		charind;
	SQLLEN		varcharind;
	SQLLEN		dateind;
	SQLLEN		longind;
	SQLLEN		clobind;
	SQLLEN		blobind;

	erg=SQLBindCol(stmt,1,SQL_C_SLONG,
			&numval,sizeof(numval),&numind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,2,SQL_C_CHAR,
			charfield,sizeof(charfield),&charind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,3,SQL_C_CHAR,
			varcharfield,sizeof(varcharfield),&varcharind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,4,SQL_C_TYPE_DATE,
			&datefield,sizeof(datefield),&dateind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,5,SQL_C_CHAR,
			longfield,sizeof(longfield),&longind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,6,SQL_C_CHAR,
			clobfield,sizeof(clobfield),&clobind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,7,SQL_C_BINARY,
			blobfield,sizeof(blobfield),&blobind);
	assertSuccessStmt(stmt,erg);

	// row 1 (direct insert)
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)numind,(int)sizeof(SQLINTEGER));
	assertEqualStmt(stmt,(int)numval,1);
	assertEqualStmt(stmt,(int)charind,40);
	assertEqualStmt(stmt,(const char *)charfield,
			"testchar1                               ");
	assertEqualStmt(stmt,(int)varcharind,12);
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar1");
	assertEqualStmt(stmt,(int)dateind,(int)sizeof(SQL_DATE_STRUCT));
	assertEqualStmt(stmt,(int)datefield.year,2001);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	assertEqualStmt(stmt,(int)longind,9);
	assertEqualStmt(stmt,(const char *)longfield,"testlong1");
	assertEqualStmt(stmt,(int)clobind,9);
	assertEqualStmt(stmt,(const char *)clobfield,"testclob1");
	assertEqualStmt(stmt,(int)blobind,(int)SQL_NULL_DATA);

	// row 2
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)numind,(int)sizeof(SQLINTEGER));
	assertEqualStmt(stmt,(int)numval,2);
	assertEqualStmt(stmt,(int)charind,40);
	assertEqualStmt(stmt,(const char *)charfield,
			"testchar2                               ");
	assertEqualStmt(stmt,(int)varcharind,12);
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar2");
	assertEqualStmt(stmt,(int)dateind,(int)sizeof(SQL_DATE_STRUCT));
	assertEqualStmt(stmt,(int)datefield.year,2002);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	assertEqualStmt(stmt,(int)longind,9);
	assertEqualStmt(stmt,(const char *)longfield,"testlong2");
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)clobind,9);
	} else {
		assertEqualStmt(stmt,(int)clobind,(int)sizeof(clobval));
	}
	assertEqualStmt(stmt,
		(const char *)clobfield,"testclob2");
	assertEqualStmt(stmt,(int)blobind,9);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"testblob2",9));

	// row 3
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)numind,(int)sizeof(SQLINTEGER));
	assertEqualStmt(stmt,(int)numval,3);
	assertEqualStmt(stmt,(int)charind,40);
	assertEqualStmt(stmt,(const char *)charfield,
			"testchar3                               ");
	assertEqualStmt(stmt,(int)varcharind,12);
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar3");
	assertEqualStmt(stmt,(int)dateind,(int)sizeof(SQL_DATE_STRUCT));
	assertEqualStmt(stmt,(int)datefield.year,2003);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	assertEqualStmt(stmt,(int)longind,9);
	assertEqualStmt(stmt,(const char *)longfield,"testlong3");
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)clobind,9);
	} else {
		assertEqualStmt(stmt,(int)clobind,(int)sizeof(clobval));
	}
	assertEqualStmt(stmt,
		(const char *)clobfield,"testclob3");
	assertEqualStmt(stmt,(int)blobind,9);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"testblob3",9));

	// row 4
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)numind,(int)sizeof(SQLINTEGER));
	assertEqualStmt(stmt,(int)numval,4);
	assertEqualStmt(stmt,(int)charind,40);
	assertEqualStmt(stmt,(const char *)charfield,
			"testchar4                               ");
	assertEqualStmt(stmt,(int)varcharind,12);
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar4");
	assertEqualStmt(stmt,(int)dateind,(int)sizeof(SQL_DATE_STRUCT));
	assertEqualStmt(stmt,(int)datefield.year,2004);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	assertEqualStmt(stmt,(int)longind,9);
	assertEqualStmt(stmt,(const char *)longfield,"testlong4");
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)clobind,9);
	} else {
		assertEqualStmt(stmt,(int)clobind,(int)sizeof(clobval));
	}
	assertEqualStmt(stmt,
		(const char *)clobfield,"testclob4");
	assertEqualStmt(stmt,(int)blobind,9);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"testblob4",9));

	// row 5
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)numind,(int)sizeof(SQLINTEGER));
	assertEqualStmt(stmt,(int)numval,5);
	assertEqualStmt(stmt,(int)charind,40);
	assertEqualStmt(stmt,(const char *)charfield,
			"testchar5                               ");
	assertEqualStmt(stmt,(int)varcharind,12);
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar5");
	assertEqualStmt(stmt,(int)dateind,(int)sizeof(SQL_DATE_STRUCT));
	assertEqualStmt(stmt,(int)datefield.year,2005);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	assertEqualStmt(stmt,(int)longind,9);
	assertEqualStmt(stmt,(const char *)longfield,"testlong5");
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)clobind,9);
	} else {
		assertEqualStmt(stmt,(int)clobind,(int)sizeof(clobval));
	}
	assertEqualStmt(stmt,
		(const char *)clobfield,"testclob5");
	assertEqualStmt(stmt,(int)blobind,9);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"testblob5",9));

	// row 6
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)numind,(int)sizeof(SQLINTEGER));
	assertEqualStmt(stmt,(int)numval,6);
	assertEqualStmt(stmt,(int)charind,40);
	assertEqualStmt(stmt,(const char *)charfield,
			"testchar6                               ");
	assertEqualStmt(stmt,(int)varcharind,12);
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar6");
	assertEqualStmt(stmt,(int)dateind,(int)sizeof(SQL_DATE_STRUCT));
	assertEqualStmt(stmt,(int)datefield.year,2006);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	assertEqualStmt(stmt,(int)longind,9);
	assertEqualStmt(stmt,(const char *)longfield,"testlong6");
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)clobind,9);
	} else {
		assertEqualStmt(stmt,(int)clobind,(int)sizeof(clobval));
	}
	assertEqualStmt(stmt,
		(const char *)clobfield,"testclob6");
	assertEqualStmt(stmt,(int)blobind,9);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"testblob6",9));

	// row 7
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)numind,(int)sizeof(SQLINTEGER));
	assertEqualStmt(stmt,(int)numval,7);
	assertEqualStmt(stmt,(int)charind,40);
	assertEqualStmt(stmt,(const char *)charfield,
			"testchar7                               ");
	assertEqualStmt(stmt,(int)varcharind,12);
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar7");
	assertEqualStmt(stmt,(int)dateind,(int)sizeof(SQL_DATE_STRUCT));
	assertEqualStmt(stmt,(int)datefield.year,2007);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	assertEqualStmt(stmt,(int)longind,9);
	assertEqualStmt(stmt,(const char *)longfield,"testlong7");
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)clobind,9);
	} else {
		assertEqualStmt(stmt,(int)clobind,(int)sizeof(clobval));
	}
	assertEqualStmt(stmt,
		(const char *)clobfield,"testclob7");
	assertEqualStmt(stmt,(int)blobind,9);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"testblob7",9));

	// row 8
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)numind,(int)sizeof(SQLINTEGER));
	assertEqualStmt(stmt,(int)numval,8);
	assertEqualStmt(stmt,(int)charind,40);
	assertEqualStmt(stmt,(const char *)charfield,
			"testchar8                               ");
	assertEqualStmt(stmt,(int)varcharind,12);
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar8");
	assertEqualStmt(stmt,(int)dateind,(int)sizeof(SQL_DATE_STRUCT));
	assertEqualStmt(stmt,(int)datefield.year,2008);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	assertEqualStmt(stmt,(int)longind,9);
	assertEqualStmt(stmt,(const char *)longfield,"testlong8");
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)clobind,9);
	} else {
		assertEqualStmt(stmt,(int)clobind,(int)sizeof(clobval));
	}
	assertEqualStmt(stmt,
		(const char *)clobfield,"testclob8");
	assertEqualStmt(stmt,(int)blobind,9);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"testblob8",9));

	// no more rows
	erg=SQLFetch(stmt);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NO_DATA);
	stdoutput.printf("\n");


	// nested selects
	// FIXME:...


	// commit and rollback
	stdoutput.printf("COMMIT AND ROLLBACK: \n");
	erg=SQLFreeStmt(stmt,SQL_CLOSE);
	erg=SQLFreeStmt(stmt,SQL_UNBIND);
	SQLHDBC		dbc2;
	SQLHSTMT	stmt2;
	erg=SQLAllocHandle(SQL_HANDLE_DBC,env,&dbc2);
	assertSuccessEnv(env,erg);
	if (issqlrelay) {
		SQLCHAR		outcstring2[1024];
		SQLSMALLINT	outcstringlen2;
		erg=SQLDriverConnect(dbc2,NULL,
				incstring,SQL_NTS,
				outcstring2,
				sizeof(outcstring2),
				&outcstringlen2,
				SQL_DRIVER_NOPROMPT);
	} else {
		erg=SQLConnect(dbc2,
				dsn,SQL_NTS,
				user,SQL_NTS,
				password,SQL_NTS);
	}
	assertSuccessDbc(dbc2,erg);
	erg=SQLAllocHandle(SQL_HANDLE_STMT,dbc2,&stmt2);
	assertSuccessDbc(dbc2,erg);
	SQLINTEGER	rowcount;
	SQLLEN		rowcountind;
	erg=SQLExecDirect(stmt2,(SQLCHAR *)
		"select count(*) from testtable",
		SQL_NTS);
	assertSuccessStmt(stmt2,erg);
	erg=SQLBindCol(stmt2,1,SQL_C_SLONG,
		&rowcount,sizeof(rowcount),&rowcountind);
	erg=SQLFetch(stmt2);
	assertSuccessStmt(stmt2,erg);
	assertEqualStmt(stmt2,(int)rowcount,0);
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);
	erg=SQLFreeStmt(stmt2,SQL_CLOSE);
	erg=SQLFreeStmt(stmt2,SQL_UNBIND);
	erg=SQLExecDirect(stmt2,(SQLCHAR *)
		"select count(*) from testtable",
		SQL_NTS);
	assertSuccessStmt(stmt2,erg);
	erg=SQLBindCol(stmt2,1,SQL_C_SLONG,
		&rowcount,sizeof(rowcount),&rowcountind);
	erg=SQLFetch(stmt2);
	assertSuccessStmt(stmt2,erg);
	assertEqualStmt(stmt2,(int)rowcount,8);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into "
		"	testtable "
		"values ("
		"	10, "
		"	'testchar10', "
		"	'testvarchar10', "
		"	'01-JAN-2010', "
		"	'testlong10', "
		"	'testclob10', "
		"	NULL)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_ROLLBACK);
	assertSuccessDbc(dbc,erg);
	erg=SQLFreeStmt(stmt2,SQL_CLOSE);
	erg=SQLFreeStmt(stmt2,SQL_UNBIND);
	erg=SQLExecDirect(stmt2,(SQLCHAR *)
		"select count(*) from testtable",
		SQL_NTS);
	assertSuccessStmt(stmt2,erg);
	erg=SQLBindCol(stmt2,1,SQL_C_SLONG,
		&rowcount,sizeof(rowcount),&rowcountind);
	erg=SQLFetch(stmt2);
	assertSuccessStmt(stmt2,erg);
	assertEqualStmt(stmt2,(int)rowcount,8);
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
		(SQLPOINTER)SQL_AUTOCOMMIT_ON,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into "
		"	testtable "
		"values ("
		"	10, "
		"	'testchar10', "
		"	'testvarchar10', "
		"	'01-JAN-2010', "
		"	'testlong10', "
		"	'testclob10', "
		"	NULL)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLFreeStmt(stmt2,SQL_CLOSE);
	erg=SQLFreeStmt(stmt2,SQL_UNBIND);
	erg=SQLExecDirect(stmt2,(SQLCHAR *)
		"select count(*) from testtable",
		SQL_NTS);
	assertSuccessStmt(stmt2,erg);
	erg=SQLBindCol(stmt2,1,SQL_C_SLONG,
		&rowcount,sizeof(rowcount),&rowcountind);
	erg=SQLFetch(stmt2);
	assertSuccessStmt(stmt2,erg);
	assertEqualStmt(stmt2,(int)rowcount,9);
	SQLFreeHandle(SQL_HANDLE_STMT,stmt2);
	SQLDisconnect(dbc2);
	SQLFreeHandle(SQL_HANDLE_DBC,dbc2);
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
		(SQLPOINTER)SQL_AUTOCOMMIT_OFF,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"drop table testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// null values
	stdoutput.printf("NULL VALUES: \n");
	erg=SQLFreeStmt(stmt,SQL_CLOSE);
	erg=SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select NULL,1,NULL from dual",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		nullfield1[10];
	SQLLEN		nullind1;
	SQLINTEGER	intfield;
	SQLLEN		intind;
	SQLCHAR		nullfield2[10];
	SQLLEN		nullind2;
	erg=SQLBindCol(stmt,1,SQL_C_CHAR,
			nullfield1,sizeof(nullfield1),&nullind1);
	erg=SQLBindCol(stmt,2,SQL_C_SLONG,
			&intfield,sizeof(intfield),&intind);
	erg=SQLBindCol(stmt,3,SQL_C_CHAR,
			nullfield2,sizeof(nullfield2),&nullind2);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)nullind1,(int)SQL_NULL_DATA);
	assertEqualStmt(stmt,(int)intfield,1);
	assertEqualStmt(stmt,(int)nullind2,(int)SQL_NULL_DATA);
	stdoutput.printf("\n");


	// null and empty lobs
	// FIXME:...


	// long lobs
	// FIXME:...


	// output bind by position
	// FIXME:...


	// lob output bind
	// FIXME:...


	// long output bind
	// FIXME:...


	// negative input bind
	// FIXME:...


	// rebinding
	// FIXME:...


	// reexecute
	// FIXME:...


	// encoded binary data
	// FIXME:...


	// quotes
	// FIXME:...


	// catalog list
	// FIXME:...


	// schema list
	// FIXME:...


	// table type list
	// FIXME:...


	// table list
	// FIXME:...


	// type info list
	// FIXME:...


	// column list
	// FIXME:...


	// column list - auto_increment, primary key
	// FIXME:...


	// primary keys list
	// FIXME:...


	// key and index list
	// FIXME:...


	// procedure list
	// FIXME:...


	// procedure parameter list
	// FIXME:...


	// invalid queries
	stdoutput.printf("INVALID QUERIES: \n");
	erg=SQLFreeStmt(stmt,SQL_CLOSE);
	erg=SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber",
		SQL_NTS);
	assertEqualStmt(stmt,(int)erg,(int)SQL_ERROR);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testnumber",
		SQL_NTS);
	assertEqualStmt(stmt,(int)erg,(int)SQL_ERROR);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	2, "
		"	3, "
		"	4)",
		SQL_NTS);
	assertEqualStmt(stmt,(int)erg,(int)SQL_ERROR);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"create table testtable",SQL_NTS);
	assertEqualStmt(stmt,(int)erg,(int)SQL_ERROR);
	stdoutput.printf("\n");


	// cleanup and disconnect
	stdoutput.printf("CLEANUP AND DISCONNECT: \n");
	SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_ROLLBACK);
	#if (ODBCVER >= 0x0300)
		erg=SQLFreeHandle(SQL_HANDLE_STMT,stmt);
		assertSuccessStmt(stmt,erg);
	#else
		erg=SQLFreeStmt(stmt,SQL_DROP);
		assertSuccessStmt(stmt,erg);
	#endif
	erg=SQLDisconnect(dbc);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#if (ODBCVER >= 0x0300)
		erg=SQLFreeHandle(SQL_HANDLE_DBC,dbc);
		assertSuccessDbc(dbc,erg);
		erg=SQLFreeHandle(SQL_HANDLE_ENV,env);
		assertSuccessEnv(env,erg);
	#else
		erg=SQLFreeConnect(dbc);
		assertSuccessDbc(dbc,erg);
		erg=SQLFreeEnv(env);
		assertSuccessEnv(env,erg);
	#endif
	delete[] hostname;
	stdoutput.printf("\n");

	reportTestStatus();

	return status;
}
