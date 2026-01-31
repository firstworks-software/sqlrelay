// Copyright (c) David Muse
// See the file COPYING for more information.

#include "../../config.h"

// windows needs this and it doesn't appear to hurt on other platforms
#include <rudiments/private/winsock.h>

#include <rudiments/private/inttypes.h>

#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>
#include <sqltypes.h>

#include "asserts.cpp"

//#define	USEDSN	1

SQLRETURN	erg;
SQLHENV		env;
SQLHDBC		dbc;
SQLHSTMT	stmt;

int main(int argc, char **argv) {

	// env handle
	stdoutput.printf("ENV HANDLE: \n");
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


	// connect
	stdoutput.printf("CONNECT: \n");
	#ifdef USEDSN
		SQLCHAR	*dsn=(SQLCHAR *)"sqlrodbc";
		SQLCHAR	*user=(SQLCHAR *)"testuser";
		SQLCHAR	*password=(SQLCHAR *)"testpassword";
		erg=SQLConnect(dbc,dsn,SQL_NTS,user,SQL_NTS,password,SQL_NTS);
	#else
		SQLCHAR		*incstring=(SQLCHAR *)
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
	#endif
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// sqlgetinfo
	stdoutput.printf("SQLGETINFO: \n");
	SQLUINTEGER	uintval;
	SQLCHAR		strval[2048];
	SQLSMALLINT	vallen;
	erg=SQLGetInfo(dbc,SQL_DBMS_NAME,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"oracle");
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

	// oracle requires the isolation level to
	// be the first query of the transaction
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);

	// you can set the isolation level, but to get it, you
	// have to have permisisons to read from sys.v_$session
	// and sys.v_$transaction, so we'll just set them here
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_READ_UNCOMMITTED,0);
	assertFailureDbc(dbc,erg);

	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_READ_COMMITTED,0);
	assertSuccessDbc(dbc,erg);

	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_REPEATABLE_READ,0);
	assertFailureDbc(dbc,erg);

	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_SERIALIZABLE,0);
	assertSuccessDbc(dbc,erg);

	// reset to default isolation level
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_READ_COMMITTED,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// statement handle
	stdoutput.printf("STATEMENT HANDLE: \n");
	erg=SQLAllocHandle(SQL_HANDLE_STMT,dbc,&stmt);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");

	// drop existing table
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);


	// create temptable
	stdoutput.printf("CREATE TEMPTABLE: \n");
	erg=SQLExecDirect(stmt,(SQLCHAR *)"create table testtable (testnumber number, testchar char(40), testvarchar varchar2(40), testdate date)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// begin transaction
	stdoutput.printf("BEGIN TRANSACTION: \n");
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
				(SQLPOINTER)SQL_AUTOCOMMIT_OFF,
				sizeof(SQLINTEGER));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// insert
	stdoutput.printf("INSERT: \n");
	erg=SQLExecDirect(stmt,(SQLCHAR *)"insert into testtable values (1,'testchar1','testvarchar1','01-JAN-2001')",SQL_NTS);
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


	// bind by position
	stdoutput.printf("BIND BY POSITION: \n");
	erg=SQLFreeStmt(stmt,SQL_CLOSE);
	erg=SQLPrepare(stmt,(SQLCHAR *)"insert into testtable values (:1,:2,:3,:4)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLSMALLINT	bindvarcount;
	erg=SQLNumParams(stmt,&bindvarcount);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)bindvarcount,4);

	SQLINTEGER	intval=2;
	SQLCHAR		*charval;
	SQLCHAR		*varcharval;
	SQLCHAR		*dateval;
	SQLLEN		charlen=SQL_NTS;
	SQLLEN		varcharlen=SQL_NTS;
	SQLLEN		datelen=SQL_NTS;

	charval=(SQLCHAR *)"testchar2";
	varcharval=(SQLCHAR *)"testvarchar2";
	dateval=(SQLCHAR *)"01-JAN-2002";
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				sizeof(SQLINTEGER),NULL);
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
				SQL_C_CHAR,SQL_TYPE_DATE,
				11,0,
				(SQLPOINTER)dateval,
				0,&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);

	intval=3;
	charval=(SQLCHAR *)"testchar3";
	varcharval=(SQLCHAR *)"testvarchar3";
	dateval=(SQLCHAR *)"01-JAN-2003";
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				sizeof(SQLINTEGER),NULL);
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
				SQL_C_CHAR,SQL_TYPE_DATE,
				11,0,
				(SQLPOINTER)dateval,
				0,&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);

	intval=4;
	charval=(SQLCHAR *)"testchar4";
	varcharval=(SQLCHAR *)"testvarchar4";
	dateval=(SQLCHAR *)"01-JAN-2004";
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				sizeof(SQLINTEGER),NULL);
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
				SQL_C_CHAR,SQL_TYPE_DATE,
				11,0,
				(SQLPOINTER)dateval,
				0,&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);

	intval=5;
	charval=(SQLCHAR *)"testchar5";
	varcharval=(SQLCHAR *)"testvarchar5";
	dateval=(SQLCHAR *)"01-JAN-2005";
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				sizeof(SQLINTEGER),NULL);
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
				SQL_C_CHAR,SQL_TYPE_DATE,
				11,0,
				(SQLPOINTER)dateval,
				0,&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);

	intval=6;
	charval=(SQLCHAR *)"testchar6";
	varcharval=(SQLCHAR *)"testvarchar6";
	dateval=(SQLCHAR *)"01-JAN-2006";
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				sizeof(SQLINTEGER),NULL);
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
				SQL_C_CHAR,SQL_TYPE_DATE,
				11,0,
				(SQLPOINTER)dateval,
				0,&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);

	intval=7;
	charval=(SQLCHAR *)"testchar7";
	varcharval=(SQLCHAR *)"testvarchar7";
	dateval=(SQLCHAR *)"01-JAN-2007";
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				sizeof(SQLINTEGER),NULL);
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
				SQL_C_CHAR,SQL_TYPE_DATE,
				11,0,
				(SQLPOINTER)dateval,
				0,&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);

	intval=8;
	charval=(SQLCHAR *)"testchar8";
	varcharval=(SQLCHAR *)"testvarchar8";
	dateval=(SQLCHAR *)"01-JAN-2008";
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				sizeof(SQLINTEGER),NULL);
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
				SQL_C_CHAR,SQL_TYPE_DATE,
				11,0,
				(SQLPOINTER)dateval,
				0,&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// commit
	stdoutput.printf("COMMIT: \n");
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// select
	stdoutput.printf("SELECT: \n");
	erg=SQLFreeStmt(stmt,SQL_CLOSE);
	erg=SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"select * from testtable order by testnumber",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// column count
	stdoutput.printf("COLUMN COUNT: \n");
	SQLSMALLINT	colcount;
	erg=SQLNumResultCols(stmt,&colcount);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)colcount,4);
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
	stdoutput.printf("\n");


	// fetch rows
	stdoutput.printf("FETCH ROWS: \n");
	SQLINTEGER	numval;
	SQLCHAR		charfield[41];
	SQLCHAR		varcharfield[41];
	SQLCHAR		datefield[12];
	SQLLEN		numind;
	SQLLEN		charind;
	SQLLEN		varcharind;
	SQLLEN		dateind;

	erg=SQLBindCol(stmt,1,SQL_C_SLONG,&numval,sizeof(numval),&numind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,2,SQL_C_CHAR,charfield,sizeof(charfield),&charind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,3,SQL_C_CHAR,varcharfield,sizeof(varcharfield),&varcharind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,4,SQL_C_CHAR,datefield,sizeof(datefield),&dateind);
	assertSuccessStmt(stmt,erg);

	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)numval,1);
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar1");
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)numval,2);
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar2");
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)numval,3);
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar3");
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)numval,4);
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar4");
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)numval,5);
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar5");
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)numval,6);
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar6");
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)numval,7);
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar7");
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)numval,8);
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar8");
	erg=SQLFetch(stmt);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NO_DATA);
	stdoutput.printf("\n");


	// rollback
	stdoutput.printf("ROLLBACK: \n");
	erg=SQLFreeStmt(stmt,SQL_CLOSE);
	erg=SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"insert into testtable values (9,'testchar9','testvarchar9','01-JAN-2009')",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_ROLLBACK);
	assertSuccessDbc(dbc,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"select count(*) from testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLINTEGER	rowcount;
	SQLLEN		rowcountind;
	erg=SQLBindCol(stmt,1,SQL_C_SLONG,&rowcount,sizeof(rowcount),&rowcountind);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)rowcount,8);
	stdoutput.printf("\n");


	// autocommit on
	stdoutput.printf("AUTOCOMMIT ON: \n");
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
				(SQLPOINTER)SQL_AUTOCOMMIT_ON,
				sizeof(SQLINTEGER));
	assertSuccessDbc(dbc,erg);
	erg=SQLFreeStmt(stmt,SQL_CLOSE);
	erg=SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"insert into testtable values (9,'testchar9','testvarchar9','01-JAN-2009')",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"select count(*) from testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,1,SQL_C_SLONG,&rowcount,sizeof(rowcount),&rowcountind);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)rowcount,9);
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
				(SQLPOINTER)SQL_AUTOCOMMIT_OFF,
				sizeof(SQLINTEGER));
	stdoutput.printf("\n");


	// null values
	stdoutput.printf("NULL VALUES: \n");
	erg=SQLFreeStmt(stmt,SQL_CLOSE);
	erg=SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"select NULL,1,NULL from dual",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		nullfield1[10];
	SQLLEN		nullind1;
	SQLINTEGER	intfield;
	SQLLEN		intind;
	SQLCHAR		nullfield2[10];
	SQLLEN		nullind2;
	erg=SQLBindCol(stmt,1,SQL_C_CHAR,nullfield1,sizeof(nullfield1),&nullind1);
	erg=SQLBindCol(stmt,2,SQL_C_SLONG,&intfield,sizeof(intfield),&intind);
	erg=SQLBindCol(stmt,3,SQL_C_CHAR,nullfield2,sizeof(nullfield2),&nullind2);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)nullind1,(int)SQL_NULL_DATA);
	assertEqualStmt(stmt,(int)intfield,1);
	assertEqualStmt(stmt,(int)nullind2,(int)SQL_NULL_DATA);
	stdoutput.printf("\n");


	// drop table
	stdoutput.printf("DROP TABLE: \n");
	erg=SQLFreeStmt(stmt,SQL_CLOSE);
	erg=SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// invalid queries
	stdoutput.printf("INVALID QUERIES: \n");
	erg=SQLExecDirect(stmt,(SQLCHAR *)"select * from testtable order by testnumber",SQL_NTS);
	assertEqualStmt(stmt,(int)erg,(int)SQL_ERROR);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"select * from testtable order by testnumber",SQL_NTS);
	assertEqualStmt(stmt,(int)erg,(int)SQL_ERROR);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"insert into testtable values (1,2,3,4)",SQL_NTS);
	assertEqualStmt(stmt,(int)erg,(int)SQL_ERROR);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"create table testtable",SQL_NTS);
	assertEqualStmt(stmt,(int)erg,(int)SQL_ERROR);
	stdoutput.printf("\n");


	// cleanup and disconnect
	stdoutput.printf("CLEANUP AND DISCONNECT: \n");
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
	stdoutput.printf("\n");

	reportTestStatus();

	return status;
}
