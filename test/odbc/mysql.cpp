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
	assertEqualDbc(dbc,(const char *)strval,"mysql");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DEFAULT_TXN_ISOLATION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	// FIXME: #7935
	//assertEqualDbc(dbc,(int)uintval,(int)SQL_TXN_REPEATABLE_READ);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// isolation levels
	stdoutput.printf("ISOLATION LEVELS: \n");
	SQLUINTEGER	isolevel;
	SQLINTEGER	isolevellen;

	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_READ_UNCOMMITTED,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
				(SQLPOINTER)&isolevel,
				sizeof(isolevel),&isolevellen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)isolevel,(int)SQL_TXN_READ_UNCOMMITTED);

	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_READ_COMMITTED,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
				(SQLPOINTER)&isolevel,
				sizeof(isolevel),&isolevellen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)isolevel,(int)SQL_TXN_READ_COMMITTED);

	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_REPEATABLE_READ,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
				(SQLPOINTER)&isolevel,
				sizeof(isolevel),&isolevellen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)isolevel,(int)SQL_TXN_REPEATABLE_READ);

	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_SERIALIZABLE,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
				(SQLPOINTER)&isolevel,
				sizeof(isolevel),&isolevellen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)isolevel,(int)SQL_TXN_SERIALIZABLE);

	// reset to the default isolation level
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_REPEATABLE_READ,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// cleanup and disconnect
	stdoutput.printf("CLEANUP AND DISCONNECT: \n");
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
