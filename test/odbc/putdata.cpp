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

SQLRETURN	erg;
SQLHENV		env;
SQLHDBC		dbc;
SQLHSTMT	stmt;

int main(int argc, char **argv) {

	SQLCHAR		*dsn;
	SQLCHAR		*user;
	SQLCHAR		*password;

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
	dsn=(SQLCHAR *)"sqlrodbc";
	user=(SQLCHAR *)"testuser";
	password=(SQLCHAR *)"testpassword";
	erg=SQLConnect(dbc,dsn,SQL_NTS,user,SQL_NTS,password,SQL_NTS);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// bind
	stdoutput.printf("BIND: \n");
	erg=SQLAllocHandle(SQL_HANDLE_STMT,dbc,&stmt);
	assertSuccessDbc(dbc,erg);

	SQLCHAR *placeholder1=(SQLCHAR *)"PARAM DATA1";
	SQLLEN	strlenorindptr1=-115;
	erg=SQLBindParameter(stmt,
				1,
				SQL_PARAM_INPUT,
				SQL_C_CHAR,
				SQL_LONGVARCHAR,
				0,
				0,
				(SQLPOINTER)placeholder1,
				0,
				&strlenorindptr1);
	assertSuccessStmt(stmt,erg);

	SQLCHAR	*placeholder2=(SQLCHAR *)"PARAM DATA2";
	SQLLEN	strlenorindptr2=-116;
	erg=SQLBindParameter(stmt,
				2,
				SQL_PARAM_INPUT,
				SQL_C_CHAR,
				SQL_LONGVARCHAR,
				0,
				0,
				(SQLPOINTER)placeholder2,
				0,
				&strlenorindptr2);
	assertSuccessStmt(stmt,erg);

	SQLCHAR	*placeholder3=(SQLCHAR *)"PARAM DATA3";
	SQLLEN	strlenorindptr3=-118;
	erg=SQLBindParameter(stmt,
				3,
				SQL_PARAM_INPUT,
				SQL_C_CHAR,
				SQL_LONGVARCHAR,
				0,
				0,
				(SQLPOINTER)placeholder3,
				0,
				&strlenorindptr3);
	assertSuccessStmt(stmt,erg);

	erg=SQLExecDirect(stmt,(SQLCHAR *)"select ?,?,?",SQL_NTS);
	assertTrueStmt(stmt,erg==SQL_NEED_DATA);

	SQLPOINTER	buffer=NULL;
	erg=SQLParamData(stmt,&buffer);
	assertTrueStmt(stmt,erg==SQL_NEED_DATA);
	assertEqualStmt(stmt,(const char *)buffer,"PARAM DATA1");

	SQLCHAR	*val=(SQLCHAR *)"param data 1 woohoooo";
	erg=SQLPutData(stmt,val,6);
	assertTrueStmt(stmt,erg==SQL_SUCCESS);
	erg=SQLPutData(stmt,val+6,6);
	assertTrueStmt(stmt,erg==SQL_SUCCESS);
	erg=SQLPutData(stmt,val+12,9);
	assertTrueStmt(stmt,erg==SQL_SUCCESS);

	erg=SQLParamData(stmt,&buffer);
	assertTrueStmt(stmt,erg==SQL_NEED_DATA);
	assertEqualStmt(stmt,(const char *)buffer,"PARAM DATA2");

	val=(SQLCHAR *)"param data 2 woohoooo";
	erg=SQLPutData(stmt,val,6);
	assertTrueStmt(stmt,erg==SQL_SUCCESS);
	erg=SQLPutData(stmt,val+6,6);
	assertTrueStmt(stmt,erg==SQL_SUCCESS);
	erg=SQLPutData(stmt,val+12,9);
	assertTrueStmt(stmt,erg==SQL_SUCCESS);

	erg=SQLParamData(stmt,&buffer);
	assertTrueStmt(stmt,erg==SQL_NEED_DATA);
	assertEqualStmt(stmt,(const char *)buffer,"PARAM DATA3");

	val=(SQLCHAR *)"param data 3 woohoooo";
	erg=SQLPutData(stmt,val,6);
	assertTrueStmt(stmt,erg==SQL_SUCCESS);
	erg=SQLPutData(stmt,val+6,6);
	assertTrueStmt(stmt,erg==SQL_SUCCESS);
	erg=SQLPutData(stmt,val+12,9);
	assertTrueStmt(stmt,erg==SQL_SUCCESS);

	erg=SQLParamData(stmt,&buffer);
	assertTrueStmt(stmt,erg==SQL_SUCCESS);

	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");

	reportTestStatus();

	return status;
}
