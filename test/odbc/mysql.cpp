// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/sys.h>
#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/stringbuffer.h>

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

	#define LARGE_BUFFER_LENGTH 8192
	#define LARGE_CHUNK_LENGTH (LARGE_BUFFER_LENGTH/4)
	SQLCHAR	largebuffer[LARGE_BUFFER_LENGTH+1];

	// hostname
	char    *hostname=sys::getHostName();
	char    *dot=(char *)charstring::findFirstOrEnd(hostname,'.');
	*dot='\0';

	// sqlrelay-vs-native flag
	bool	issqlrelay=!(argc==2 && !charstring::compare(argv[1],"native"));



	// environment handle
	stdoutput.printf("ENVIRONMENT HANDLE: \n");
	#if (ODBCVER >= 0x0300)
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



	// environment attributes (pre-connect, handled by driver manager)
	stdoutput.printf("ENVIRONMENT ATTRIBUTES (pre-connect): \n");
	SQLUINTEGER	envuintval;
	SQLINTEGER	envstrlen;
	SQLUINTEGER	initial;


	// SQL_ATTR_ODBC_VERSION
	stdoutput.printf("  SQL_ATTR_ODBC_VERSION\n");
	// save initial value
	erg=SQLGetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)&initial,0,&envstrlen);
	assertSuccessEnv(env,erg);
	// SQL_OV_ODBC2
	erg=SQLSetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)(uintptr_t)SQL_OV_ODBC2,0);
	assertSuccessEnv(env,erg);
	erg=SQLGetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)&envuintval,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)envuintval,(int)SQL_OV_ODBC2);
	// SQL_ATTR_ODBC_VERSION
	#if defined(SQL_OV_ODBC3)
	erg=SQLSetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)(uintptr_t)SQL_OV_ODBC3,0);
	assertSuccessEnv(env,erg);
	erg=SQLGetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)&envuintval,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)envuintval,(int)SQL_OV_ODBC3);
	#endif
	// restore initial value
	erg=SQLSetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)(uintptr_t)initial,0);
	assertSuccessEnv(env,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_OUTPUT_NTS
	stdoutput.printf("  SQL_ATTR_OUTPUT_NTS\n");
	// save initial value (default is SQL_TRUE)
	erg=SQLGetEnvAttr(env,SQL_ATTR_OUTPUT_NTS,
			(SQLPOINTER)&initial,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)initial,(int)SQL_TRUE);
	// SQL_TRUE
	erg=SQLSetEnvAttr(env,SQL_ATTR_OUTPUT_NTS,
			(SQLPOINTER)(uintptr_t)SQL_TRUE,0);
	assertSuccessEnv(env,erg);
	erg=SQLGetEnvAttr(env,SQL_ATTR_OUTPUT_NTS,
			(SQLPOINTER)&envuintval,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)envuintval,(int)SQL_TRUE);
	// SQL_FALSE (rejected, value unchanged)
	erg=SQLSetEnvAttr(env,SQL_ATTR_OUTPUT_NTS,
			(SQLPOINTER)(uintptr_t)SQL_FALSE,0);
	assertFailureEnv(env,erg);
	erg=SQLGetEnvAttr(env,SQL_ATTR_OUTPUT_NTS,
			(SQLPOINTER)&envuintval,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)envuintval,(int)SQL_TRUE);
	// restore initial value
	erg=SQLSetEnvAttr(env,SQL_ATTR_OUTPUT_NTS,
			(SQLPOINTER)(uintptr_t)initial,0);
	assertSuccessEnv(env,erg);
	stdoutput.printf("\n");


	#if (ODBCVER >= 0x0300)
	// SQL_ATTR_CONNECTION_POOLING
	stdoutput.printf("  SQL_ATTR_CONNECTION_POOLING\n");
	// save initial value (default is SQL_CP_OFF)
	erg=SQLGetEnvAttr(env,SQL_ATTR_CONNECTION_POOLING,
			(SQLPOINTER)&initial,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)initial,(int)SQL_CP_OFF);
	// SQL_CP_OFF
	erg=SQLSetEnvAttr(env,SQL_ATTR_CONNECTION_POOLING,
			(SQLPOINTER)(uintptr_t)SQL_CP_OFF,0);
	assertSuccessEnv(env,erg);
	erg=SQLGetEnvAttr(env,SQL_ATTR_CONNECTION_POOLING,
			(SQLPOINTER)&envuintval,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)envuintval,(int)SQL_CP_OFF);
	// SQL_CP_ONE_PER_DRIVER
	erg=SQLSetEnvAttr(env,SQL_ATTR_CONNECTION_POOLING,
			(SQLPOINTER)(uintptr_t)SQL_CP_ONE_PER_DRIVER,0);
	assertSuccessEnv(env,erg);
	erg=SQLGetEnvAttr(env,SQL_ATTR_CONNECTION_POOLING,
			(SQLPOINTER)&envuintval,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)envuintval,(int)SQL_CP_ONE_PER_DRIVER);
	// SQL_CP_ONE_PER_HENV
	erg=SQLSetEnvAttr(env,SQL_ATTR_CONNECTION_POOLING,
			(SQLPOINTER)(uintptr_t)SQL_CP_ONE_PER_HENV,0);
	assertSuccessEnv(env,erg);
	erg=SQLGetEnvAttr(env,SQL_ATTR_CONNECTION_POOLING,
			(SQLPOINTER)&envuintval,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)envuintval,(int)SQL_CP_ONE_PER_HENV);
	// restore initial value
	erg=SQLSetEnvAttr(env,SQL_ATTR_CONNECTION_POOLING,
			(SQLPOINTER)(uintptr_t)initial,0);
	assertSuccessEnv(env,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_ATTR_CP_MATCH
	stdoutput.printf("  SQL_ATTR_CP_MATCH\n");
	// save initial value (default is SQL_CP_MATCH_DEFAULT)
	erg=SQLGetEnvAttr(env,SQL_ATTR_CP_MATCH,
			(SQLPOINTER)&initial,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)initial,(int)SQL_CP_MATCH_DEFAULT);
	// SQL_CP_STRICT_MATCH
	erg=SQLSetEnvAttr(env,SQL_ATTR_CP_MATCH,
			(SQLPOINTER)(uintptr_t)SQL_CP_STRICT_MATCH,0);
	assertSuccessEnv(env,erg);
	erg=SQLGetEnvAttr(env,SQL_ATTR_CP_MATCH,
			(SQLPOINTER)&envuintval,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)envuintval,(int)SQL_CP_STRICT_MATCH);
	// SQL_CP_RELAXED_MATCH
	erg=SQLSetEnvAttr(env,SQL_ATTR_CP_MATCH,
			(SQLPOINTER)(uintptr_t)SQL_CP_RELAXED_MATCH,0);
	assertSuccessEnv(env,erg);
	erg=SQLGetEnvAttr(env,SQL_ATTR_CP_MATCH,
			(SQLPOINTER)&envuintval,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)envuintval,(int)SQL_CP_RELAXED_MATCH);
	// restore initial value
	erg=SQLSetEnvAttr(env,SQL_ATTR_CP_MATCH,
			(SQLPOINTER)(uintptr_t)initial,0);
	assertSuccessEnv(env,erg);
	stdoutput.printf("\n");
	#endif



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



	// connection attributes (pre-connect, handled by driver manager)
	stdoutput.printf("CONNECTION ATTRIBUTES (pre-connect): \n");
	SQLUINTEGER	dbcuintval;
	SQLINTEGER	dbcstrlen;
	SQLUINTEGER	dbcinitial;
	SQLCHAR		dbcstrval[2048];
	SQLCHAR		dbcstrinit[2048];
	SQLPOINTER	dbcptrval;
	SQLPOINTER	dbcptrinit;


	// SQL_ATTR_LOGIN_TIMEOUT
	stdoutput.printf("  SQL_ATTR_LOGIN_TIMEOUT\n");
	// save initial value
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_LOGIN_TIMEOUT,
			(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	// set to 30
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_LOGIN_TIMEOUT,
			(SQLPOINTER)(uintptr_t)30,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_LOGIN_TIMEOUT,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,30);
	// restore initial value
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_LOGIN_TIMEOUT,
			(SQLPOINTER)(uintptr_t)dbcinitial,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_ODBC_CURSORS
	stdoutput.printf("  SQL_ATTR_ODBC_CURSORS\n");
	// save initial value
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_ODBC_CURSORS,
			(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	// SQL_CUR_USE_ODBC
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_ODBC_CURSORS,
			(SQLPOINTER)(uintptr_t)SQL_CUR_USE_ODBC,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_ODBC_CURSORS,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_CUR_USE_ODBC);
	// SQL_CUR_USE_DRIVER
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_ODBC_CURSORS,
			(SQLPOINTER)(uintptr_t)SQL_CUR_USE_DRIVER,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_ODBC_CURSORS,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_CUR_USE_DRIVER);
	// SQL_CUR_USE_IF_NEEDED
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_ODBC_CURSORS,
			(SQLPOINTER)(uintptr_t)SQL_CUR_USE_IF_NEEDED,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_ODBC_CURSORS,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_CUR_USE_IF_NEEDED);
	// restore initial value
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_ODBC_CURSORS,
			(SQLPOINTER)(uintptr_t)dbcinitial,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_PACKET_SIZE
	// must be set before connect and can't be read until after; the
	// connect section reads it back
	stdoutput.printf("  SQL_ATTR_PACKET_SIZE\n");
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_PACKET_SIZE,
			(SQLPOINTER)(uintptr_t)2048,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_TRACE
	stdoutput.printf("  SQL_ATTR_TRACE\n");
	// save initial value
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TRACE,
			(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	// SQL_OPT_TRACE_ON
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TRACE,
			(SQLPOINTER)(uintptr_t)SQL_OPT_TRACE_ON,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TRACE,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_OPT_TRACE_ON);
	// SQL_OPT_TRACE_OFF
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TRACE,
			(SQLPOINTER)(uintptr_t)SQL_OPT_TRACE_OFF,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TRACE,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_OPT_TRACE_OFF);
	// restore initial value
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TRACE,
			(SQLPOINTER)(uintptr_t)dbcinitial,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");



	// connect
	stdoutput.printf("CONNECT: \n");
	stringbuffer	incstr;
	if (issqlrelay) {
		incstr.append(
			"Driver={SQL Relay};"
			"Server=sqlrelay;Port=9000;"
			"Socket=/tmp/test.socket;"
			"User=testuser;Password=testpassword;"
			"NullsAsNulls=yes;"
			// for ODBC spec compliance
			"AutoCommit=yes;");
	} else {
		incstr.append(
			"Driver={MariaDB};"
			"Server=mysql;"
			"User=testuser;Password=testpassword;Database=")->
			append(hostname)->append(";");
	}
	SQLCHAR		outcstring[1024];
	SQLSMALLINT	outcstringlen;
	erg=SQLDriverConnect(dbc,NULL,
			(SQLCHAR *)incstr.getString(),
			SQL_NTS,
			outcstring,
			sizeof(outcstring),
			&outcstringlen,
			SQL_DRIVER_NOPROMPT);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// the SQL Relay driver echoes the connect string back
		assertEqualDbc(dbc,(int)outcstringlen,
				(int)incstr.getStringLength());
		assertEqualDbc(dbc,(const char *)outcstring,
				incstr.getString());
	} else {
		// native drivers return a driver-specific completed
		// connect string; just verify that one came back
		assertTrueDbc(dbc,outcstringlen>0);
		assertTrueDbc(dbc,outcstring[0]!='\0');
	}
	// SQL_ATTR_PACKET_SIZE was set pre-connect and can be read now
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_PACKET_SIZE,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)dbcuintval,2048);
	} else {
		// the mariadb driver negotiates its own packet size
		assertEqualDbc(dbc,(int)dbcuintval,8192);
	}
	stdoutput.printf("\n");



	// environment attributes (post-connect, forwarded to driver)
	stdoutput.printf("ENVIRONMENT ATTRIBUTES (post-connect): \n");


	// SQL_ATTR_ODBC_VERSION
	// can't be set once a connection handle exists (HY010); value unchanged
	stdoutput.printf("  SQL_ATTR_ODBC_VERSION\n");
	// get initial value
	erg=SQLGetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)&initial,0,&envstrlen);
	assertSuccessEnv(env,erg);
	// SQL_OV_ODBC2 (rejected, value unchanged)
	erg=SQLSetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)(uintptr_t)SQL_OV_ODBC2,0);
	assertFailureEnv(env,erg);
	erg=SQLGetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)&envuintval,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)envuintval,(int)initial);
	#if defined(SQL_OV_ODBC3)
	// SQL_OV_ODBC3 (rejected, value unchanged)
	erg=SQLSetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)(uintptr_t)SQL_OV_ODBC3,0);
	assertFailureEnv(env,erg);
	erg=SQLGetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)&envuintval,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)envuintval,(int)initial);
	#endif
	stdoutput.printf("\n");


	// SQL_ATTR_OUTPUT_NTS
	stdoutput.printf("  SQL_ATTR_OUTPUT_NTS\n");
	// save initial value (default is SQL_TRUE)
	erg=SQLGetEnvAttr(env,SQL_ATTR_OUTPUT_NTS,
			(SQLPOINTER)&initial,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)initial,(int)SQL_TRUE);
	// SQL_TRUE
	erg=SQLSetEnvAttr(env,SQL_ATTR_OUTPUT_NTS,
			(SQLPOINTER)(uintptr_t)SQL_TRUE,0);
	assertSuccessEnv(env,erg);
	erg=SQLGetEnvAttr(env,SQL_ATTR_OUTPUT_NTS,
			(SQLPOINTER)&envuintval,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)envuintval,(int)SQL_TRUE);
	// SQL_FALSE (rejected, value unchanged)
	erg=SQLSetEnvAttr(env,SQL_ATTR_OUTPUT_NTS,
			(SQLPOINTER)(uintptr_t)SQL_FALSE,0);
	assertFailureEnv(env,erg);
	erg=SQLGetEnvAttr(env,SQL_ATTR_OUTPUT_NTS,
			(SQLPOINTER)&envuintval,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)envuintval,(int)SQL_TRUE);
	// restore initial value
	erg=SQLSetEnvAttr(env,SQL_ATTR_OUTPUT_NTS,
			(SQLPOINTER)(uintptr_t)initial,0);
	assertSuccessEnv(env,erg);
	stdoutput.printf("\n");


	#if (ODBCVER >= 0x0300)
	// SQL_ATTR_CONNECTION_POOLING
	stdoutput.printf("  SQL_ATTR_CONNECTION_POOLING\n");
	// save initial value (default is SQL_CP_OFF)
	erg=SQLGetEnvAttr(env,SQL_ATTR_CONNECTION_POOLING,
			(SQLPOINTER)&initial,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)initial,(int)SQL_CP_OFF);
	// SQL_CP_OFF
	erg=SQLSetEnvAttr(env,SQL_ATTR_CONNECTION_POOLING,
			(SQLPOINTER)(uintptr_t)SQL_CP_OFF,0);
	assertSuccessEnv(env,erg);
	erg=SQLGetEnvAttr(env,SQL_ATTR_CONNECTION_POOLING,
			(SQLPOINTER)&envuintval,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)envuintval,(int)SQL_CP_OFF);
	// SQL_CP_ONE_PER_DRIVER
	erg=SQLSetEnvAttr(env,SQL_ATTR_CONNECTION_POOLING,
			(SQLPOINTER)(uintptr_t)SQL_CP_ONE_PER_DRIVER,0);
	assertSuccessEnv(env,erg);
	erg=SQLGetEnvAttr(env,SQL_ATTR_CONNECTION_POOLING,
			(SQLPOINTER)&envuintval,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)envuintval,(int)SQL_CP_ONE_PER_DRIVER);
	// SQL_CP_ONE_PER_HENV
	erg=SQLSetEnvAttr(env,SQL_ATTR_CONNECTION_POOLING,
			(SQLPOINTER)(uintptr_t)SQL_CP_ONE_PER_HENV,0);
	assertSuccessEnv(env,erg);
	erg=SQLGetEnvAttr(env,SQL_ATTR_CONNECTION_POOLING,
			(SQLPOINTER)&envuintval,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)envuintval,(int)SQL_CP_ONE_PER_HENV);
	// restore initial value
	erg=SQLSetEnvAttr(env,SQL_ATTR_CONNECTION_POOLING,
			(SQLPOINTER)(uintptr_t)initial,0);
	assertSuccessEnv(env,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_ATTR_CP_MATCH
	stdoutput.printf("  SQL_ATTR_CP_MATCH\n");
	// save initial value (default is SQL_CP_MATCH_DEFAULT)
	erg=SQLGetEnvAttr(env,SQL_ATTR_CP_MATCH,
			(SQLPOINTER)&initial,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)initial,(int)SQL_CP_MATCH_DEFAULT);
	// SQL_CP_STRICT_MATCH
	erg=SQLSetEnvAttr(env,SQL_ATTR_CP_MATCH,
			(SQLPOINTER)(uintptr_t)SQL_CP_STRICT_MATCH,0);
	assertSuccessEnv(env,erg);
	erg=SQLGetEnvAttr(env,SQL_ATTR_CP_MATCH,
			(SQLPOINTER)&envuintval,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)envuintval,(int)SQL_CP_STRICT_MATCH);
	// SQL_CP_RELAXED_MATCH
	erg=SQLSetEnvAttr(env,SQL_ATTR_CP_MATCH,
			(SQLPOINTER)(uintptr_t)SQL_CP_RELAXED_MATCH,0);
	assertSuccessEnv(env,erg);
	erg=SQLGetEnvAttr(env,SQL_ATTR_CP_MATCH,
			(SQLPOINTER)&envuintval,0,&envstrlen);
	assertSuccessEnv(env,erg);
	assertEqualEnv(env,(int)envuintval,(int)SQL_CP_RELAXED_MATCH);
	// restore initial value
	erg=SQLSetEnvAttr(env,SQL_ATTR_CP_MATCH,
			(SQLPOINTER)(uintptr_t)initial,0);
	assertSuccessEnv(env,erg);
	stdoutput.printf("\n");
	#endif



	// connection attributes (post-connect, forwarded to driver)
	stdoutput.printf("CONNECTION ATTRIBUTES (post-connect): \n");


	// SQL_ATTR_AUTOCOMMIT
	stdoutput.printf("  SQL_ATTR_AUTOCOMMIT\n");
	// save initial value
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
			(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	// SQL_AUTOCOMMIT_OFF
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
			(SQLPOINTER)(uintptr_t)SQL_AUTOCOMMIT_OFF,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_AUTOCOMMIT_OFF);
	} else {
		// MariaDB always returns SQL_AUTOCOMMIT_ON regardless of set
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_AUTOCOMMIT_ON);
	}
	// SQL_AUTOCOMMIT_ON
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
			(SQLPOINTER)(uintptr_t)SQL_AUTOCOMMIT_ON,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_AUTOCOMMIT_ON);
	// restore initial value
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
			(SQLPOINTER)(uintptr_t)dbcinitial,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_ACCESS_MODE
	stdoutput.printf("  SQL_ATTR_ACCESS_MODE\n");
	// save initial value
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_ACCESS_MODE,
			(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	// SQL_MODE_READ_ONLY
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_ACCESS_MODE,
			(SQLPOINTER)(uintptr_t)SQL_MODE_READ_ONLY,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_ACCESS_MODE,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_MODE_READ_ONLY);
	} else {
		// MariaDB always returns SQL_MODE_READ_WRITE regardless of set
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_MODE_READ_WRITE);
	}
	// SQL_MODE_READ_WRITE
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_ACCESS_MODE,
			(SQLPOINTER)(uintptr_t)SQL_MODE_READ_WRITE,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_ACCESS_MODE,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_MODE_READ_WRITE);
	// restore initial value
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_ACCESS_MODE,
			(SQLPOINTER)(uintptr_t)dbcinitial,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	#if (ODBCVER >= 0x0300)
	// SQL_ATTR_CONNECTION_TIMEOUT
	stdoutput.printf("  SQL_ATTR_CONNECTION_TIMEOUT\n");
	// save initial value
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_CONNECTION_TIMEOUT,
			(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	// set to 30
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_CONNECTION_TIMEOUT,
			(SQLPOINTER)(uintptr_t)30,0);
	if (issqlrelay) {
		assertSuccessDbc(dbc,erg);
	} else {
		// MariaDB substitutes its own timeout, returns SQL_SUCCESS_WITH_INFO
		assertEqualDbc(dbc,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	}
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_CONNECTION_TIMEOUT,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)dbcuintval,30);
	} else {
		// MariaDB substituted; value never changed
		assertEqualDbc(dbc,(int)dbcuintval,(int)dbcinitial);
	}
	// restore initial value
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_CONNECTION_TIMEOUT,
			(SQLPOINTER)(uintptr_t)dbcinitial,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_ATTR_METADATA_ID
	// sqlrelay catalog functions always pattern-match: SQL_TRUE
	// substituted with SQL_FALSE, SQL_SUCCESS_WITH_INFO + SQLSTATE 01S02
	stdoutput.printf("  SQL_ATTR_METADATA_ID\n");
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)dbcinitial,(int)SQL_FALSE);
	} else {
		// MariaDB defaults to SQL_TRUE despite the spec default
		assertEqualDbc(dbc,(int)dbcinitial,(int)SQL_TRUE);
	}
	// SQL_TRUE
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)(uintptr_t)SQL_TRUE,0);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	} else {
		assertSuccessDbc(dbc,erg);
	}
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// get reflects the substituted value, not what the app set
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_FALSE);
	} else {
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_TRUE);
	}
	// SQL_FALSE
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)(uintptr_t)SQL_FALSE,0);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)erg,(int)SQL_SUCCESS);
	} else {
		assertSuccessDbc(dbc,erg);
	}
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_FALSE);
	} else {
		// MariaDB does not actually clear the flag.
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_TRUE);
	}
	// restore initial value
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)(uintptr_t)dbcinitial,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	// SQL_ATTR_TXN_ISOLATION
	stdoutput.printf("  SQL_ATTR_TXN_ISOLATION\n");
	// save initial value
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	// SQL_TXN_READ_UNCOMMITTED
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_READ_UNCOMMITTED,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_TXN_READ_UNCOMMITTED);
	// SQL_TXN_READ_COMMITTED
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_READ_COMMITTED,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_TXN_READ_COMMITTED);
	// SQL_TXN_REPEATABLE_READ
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_REPEATABLE_READ,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_TXN_REPEATABLE_READ);
	// SQL_TXN_SERIALIZABLE
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_SERIALIZABLE,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_TXN_SERIALIZABLE);
	// restore initial value
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)dbcinitial,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	#if (ODBCVER >= 0x0300)
	// SQL_ATTR_CURRENT_CATALOG
	stdoutput.printf("  SQL_ATTR_CURRENT_CATALOG\n");
	// save initial value
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_CURRENT_CATALOG,
			(SQLPOINTER)dbcstrinit,sizeof(dbcstrinit),&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	// the initial catalog is the database from the connect string
	// (named after the host)
	assertEqualDbc(dbc,(const char *)dbcstrinit,hostname);
	// set is a no-op for many drivers; round-trip to initial should succeed
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_CURRENT_CATALOG,
			(SQLPOINTER)dbcstrinit,SQL_NTS);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_CURRENT_CATALOG,
			(SQLPOINTER)dbcstrval,sizeof(dbcstrval),&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(const char *)dbcstrval,(const char *)dbcstrinit);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_ATTR_ASYNC_ENABLE
	stdoutput.printf("  SQL_ATTR_ASYNC_ENABLE\n");
	// save initial value
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_ASYNC_ENABLE,
			(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcinitial,(int)SQL_ASYNC_ENABLE_OFF);
	// SQL_ASYNC_ENABLE_OFF
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_ASYNC_ENABLE,
			(SQLPOINTER)(uintptr_t)SQL_ASYNC_ENABLE_OFF,0);
	assertEqualDbc(dbc,(int)erg,(int)SQL_SUCCESS);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_ASYNC_ENABLE,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_ASYNC_ENABLE_OFF);
	// SQL_ASYNC_ENABLE_ON (not supported by sqlrelay or mysql odbc)
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_ASYNC_ENABLE,
			(SQLPOINTER)(uintptr_t)SQL_ASYNC_ENABLE_ON,0);
	if (issqlrelay) {
		// substitutes SQL_ASYNC_ENABLE_OFF; SQL_SUCCESS_WITH_INFO,
		// SQLSTATE 01S02 ("Option value changed")
		assertEqualDbc(dbc,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	} else {
		// substitutes SQL_ASYNC_ENABLE_OFF; SQL_SUCCESS
		assertSuccessDbc(dbc,erg);
	}
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_ASYNC_ENABLE,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	// get reflects the substituted value, not what the app set
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_ASYNC_ENABLE_OFF);
	// restore initial value
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_ASYNC_ENABLE,
			(SQLPOINTER)(uintptr_t)dbcinitial,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_ATTR_CONNECTION_DEAD (read-only)
	stdoutput.printf("  SQL_ATTR_CONNECTION_DEAD\n");
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_CONNECTION_DEAD,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_CD_FALSE);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0351)
	// SQL_ATTR_AUTO_IPD (read-only)
	// neither driver auto-populates the IPD
	stdoutput.printf("  SQL_ATTR_AUTO_IPD\n");
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_AUTO_IPD,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_FALSE);
	stdoutput.printf("\n");
	#endif


	#if defined(SQL_ATTR_DISCONNECT_BEHAVIOR)
	// SQL_ATTR_DISCONNECT_BEHAVIOR
	// MariaDB rejects get and silently accepts every set.
	stdoutput.printf("  SQL_ATTR_DISCONNECT_BEHAVIOR\n");
	// save initial value
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_DISCONNECT_BEHAVIOR,
			(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
	if (issqlrelay) {
		assertSuccessDbc(dbc,erg);
	} else {
		assertFailureDbc(dbc,erg);
	}
	// SQL_DB_RETURN_TO_POOL
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_DISCONNECT_BEHAVIOR,
			(SQLPOINTER)(uintptr_t)SQL_DB_RETURN_TO_POOL,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_DISCONNECT_BEHAVIOR,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	if (issqlrelay) {
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_DB_RETURN_TO_POOL);
	} else {
		assertFailureDbc(dbc,erg);
	}
	// SQL_DB_DISCONNECT
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_DISCONNECT_BEHAVIOR,
			(SQLPOINTER)(uintptr_t)SQL_DB_DISCONNECT,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_DISCONNECT_BEHAVIOR,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	if (issqlrelay) {
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_DB_DISCONNECT);
	} else {
		assertFailureDbc(dbc,erg);
	}
	// restore initial value
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_DISCONNECT_BEHAVIOR,
			(SQLPOINTER)(uintptr_t)dbcinitial,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	// SQL_ATTR_TRACEFILE
	// driver manager rejects an empty restore value; don't restore
	stdoutput.printf("  SQL_ATTR_TRACEFILE\n");
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TRACEFILE,
			(SQLPOINTER)"/tmp/odbctrace.log",SQL_NTS);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TRACEFILE,
			(SQLPOINTER)dbcstrval,sizeof(dbcstrval),&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(const char *)dbcstrval,"/tmp/odbctrace.log");
	stdoutput.printf("\n");


	// SQL_ATTR_QUIET_MODE
	stdoutput.printf("  SQL_ATTR_QUIET_MODE\n");
	// save initial value (a window handle; NULL is valid)
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_QUIET_MODE,
			(SQLPOINTER)&dbcptrinit,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	// set to NULL (no parent window)
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_QUIET_MODE,
			(SQLPOINTER)NULL,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_QUIET_MODE,
			(SQLPOINTER)&dbcptrval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)(uintptr_t)dbcptrval,0);
	// restore initial value
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_QUIET_MODE,
			(SQLPOINTER)dbcptrinit,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	#if (ODBCVER >= 0x0300)
	// SQL_ATTR_TRANSLATE_LIB
	stdoutput.printf("  SQL_ATTR_TRANSLATE_LIB\n");
	// save initial value
	bytestring::zero(dbcstrinit,sizeof(dbcstrinit));
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TRANSLATE_LIB,
			(SQLPOINTER)dbcstrinit,
			sizeof(dbcstrinit),&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	// the initial translate lib is empty on both sides
	assertEqualDbc(dbc,(const char *)dbcstrinit,"");
	// round-trip to the initial value
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TRANSLATE_LIB,
			(SQLPOINTER)dbcstrinit,SQL_NTS);
	assertSuccessDbc(dbc,erg);
	bytestring::zero(dbcstrval,sizeof(dbcstrval));
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TRANSLATE_LIB,
			(SQLPOINTER)dbcstrval,
			sizeof(dbcstrval),&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(const char *)dbcstrval,
			(const char *)dbcstrinit);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_ATTR_TRANSLATE_OPTION
	stdoutput.printf("  SQL_ATTR_TRANSLATE_OPTION\n");
	// save initial value
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TRANSLATE_OPTION,
			(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	// set to 0
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TRANSLATE_OPTION,
			(SQLPOINTER)(uintptr_t)0,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TRANSLATE_OPTION,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,0);
	// restore initial value
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TRANSLATE_OPTION,
			(SQLPOINTER)(uintptr_t)dbcinitial,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if defined(SQL_ATTR_ANSI_APP)
	// SQL_ATTR_ANSI_APP
	// MariaDB returns HYC00 on get, silently accepts every set
	stdoutput.printf("  SQL_ATTR_ANSI_APP\n");
	// save initial value
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_ANSI_APP,
			(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
	if (issqlrelay) {
		assertSuccessDbc(dbc,erg);
	} else {
		assertFailureDbc(dbc,erg);
	}
	// SQL_AA_TRUE
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_ANSI_APP,
			(SQLPOINTER)(uintptr_t)SQL_AA_TRUE,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_ANSI_APP,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	if (issqlrelay) {
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_AA_TRUE);
	} else {
		assertFailureDbc(dbc,erg);
	}
	// SQL_AA_FALSE
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_ANSI_APP,
			(SQLPOINTER)(uintptr_t)SQL_AA_FALSE,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_ANSI_APP,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	if (issqlrelay) {
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_AA_FALSE);
	} else {
		assertFailureDbc(dbc,erg);
	}
	// restore initial value (safe on MariaDB though dbcinitial
	// was never populated - it accepts any value)
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_ANSI_APP,
			(SQLPOINTER)(uintptr_t)dbcinitial,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if defined(SQL_ATTR_RESET_CONNECTION)
	// SQL_ATTR_RESET_CONNECTION
	stdoutput.printf("  SQL_ATTR_RESET_CONNECTION\n");
	// SQL_RESET_CONNECTION_YES (write-only per spec; set
	// before reusing a pooled connection)
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_RESET_CONNECTION,
			(SQLPOINTER)(uintptr_t)
			SQL_RESET_CONNECTION_YES,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if defined(SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE)
	// SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE
	// MariaDB rejects get, silently accepts every set
	stdoutput.printf("  SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE\n");
	// save initial value
	erg=SQLGetConnectAttr(dbc,
			SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
			(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
	if (issqlrelay) {
		assertSuccessDbc(dbc,erg);
	} else {
		assertFailureDbc(dbc,erg);
	}
	// SQL_ASYNC_DBC_ENABLE_ON
	erg=SQLSetConnectAttr(dbc,
			SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
			(SQLPOINTER)(uintptr_t)SQL_ASYNC_DBC_ENABLE_ON,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,
			SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	if (issqlrelay) {
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,
				(int)SQL_ASYNC_DBC_ENABLE_ON);
	} else {
		assertFailureDbc(dbc,erg);
	}
	// SQL_ASYNC_DBC_ENABLE_OFF
	erg=SQLSetConnectAttr(dbc,
			SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
			(SQLPOINTER)(uintptr_t)SQL_ASYNC_DBC_ENABLE_OFF,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,
			SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	if (issqlrelay) {
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,
				(int)SQL_ASYNC_DBC_ENABLE_OFF);
	} else {
		assertFailureDbc(dbc,erg);
	}
	// restore initial value
	erg=SQLSetConnectAttr(dbc,
			SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
			(SQLPOINTER)(uintptr_t)dbcinitial,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if defined(SQL_ATTR_DRIVER_THREADING)
	// SQL_ATTR_DRIVER_THREADING
	// driver threading level; SQL Relay reports 1 (thread-safe per HDBC),
	// MariaDB rejects get and silently accepts every set
	stdoutput.printf("  SQL_ATTR_DRIVER_THREADING\n");
	// save initial value
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_DRIVER_THREADING,
			(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
	if (issqlrelay) {
		assertSuccessDbc(dbc,erg);
	} else {
		assertFailureDbc(dbc,erg);
	}
	// set to 1
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_DRIVER_THREADING,
			(SQLPOINTER)(uintptr_t)1,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_DRIVER_THREADING,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	if (issqlrelay) {
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,1);
	} else {
		assertFailureDbc(dbc,erg);
	}
	// restore initial value
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_DRIVER_THREADING,
			(SQLPOINTER)(uintptr_t)dbcinitial,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif



	// get info
	stdoutput.printf("GET INFO: \n");
	SQLUINTEGER	uintval;
	SQLUSMALLINT	usmallintval;
	SQLCHAR		strval[2048];
	SQLSMALLINT	vallen;
	SQLULEN		handleval;


	#if (ODBCVER >= 0x0300)
	// SQL_MAX_DRIVER_CONNECTIONS
	stdoutput.printf("  SQL_MAX_DRIVER_CONNECTIONS\n");
	erg=SQLGetInfo(dbc,SQL_MAX_DRIVER_CONNECTIONS,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,5);
	} else {
		assertEqualDbc(dbc,(int)usmallintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_MAX_CONCURRENT_ACTIVITIES
	stdoutput.printf("  SQL_MAX_CONCURRENT_ACTIVITIES\n");
	erg=SQLGetInfo(dbc,SQL_MAX_CONCURRENT_ACTIVITIES,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	// SQL_DATA_SOURCE_NAME
	stdoutput.printf("  SQL_DATA_SOURCE_NAME\n");
	erg=SQLGetInfo(dbc,SQL_DATA_SOURCE_NAME,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_FETCH_DIRECTION
	stdoutput.printf("  SQL_FETCH_DIRECTION\n");
	erg=SQLGetInfo(dbc,SQL_FETCH_DIRECTION,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	if (issqlrelay) {
		// sqlrelay only supports SQL_FD_FETCH_NEXT
		assertEqualDbc(dbc,(int)uintval,(int)SQL_FD_FETCH_NEXT);
		assertSuccessDbc(dbc,erg);
	} else {
		// MariaDB returns HY096 "Invalid information type".
		assertFailureDbc(dbc,erg);
	}
	stdoutput.printf("\n");


	// SQL_SERVER_NAME
	stdoutput.printf("  SQL_SERVER_NAME\n");
	erg=SQLGetInfo(dbc,SQL_SERVER_NAME,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"sqlrelay");
	} else {
		// MySQL ODBC reports the connected server name; verify non-empty
		assertTrueDbc(dbc,vallen>0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_SEARCH_PATTERN_ESCAPE
	stdoutput.printf("  SQL_SEARCH_PATTERN_ESCAPE\n");
	erg=SQLGetInfo(dbc,SQL_SEARCH_PATTERN_ESCAPE,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"\\");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_DBMS_NAME
	stdoutput.printf("  SQL_DBMS_NAME\n");
	erg=SQLGetInfo(dbc,SQL_DBMS_NAME,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"mysql");
	} else {
		assertEqualDbc(dbc,(const char *)strval,"MySQL");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_DBMS_VER
	stdoutput.printf("  SQL_DBMS_VER\n");
	erg=SQLGetInfo(dbc,SQL_DBMS_VER,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertIsVersionDbc(dbc,(const char *)strval);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ACCESSIBLE_TABLES
	stdoutput.printf("  SQL_ACCESSIBLE_TABLES\n");
	erg=SQLGetInfo(dbc,SQL_ACCESSIBLE_TABLES,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	// Both drivers report "N".
	assertEqualDbc(dbc,(const char *)strval,"N");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ACCESSIBLE_PROCEDURES
	stdoutput.printf("  SQL_ACCESSIBLE_PROCEDURES\n");
	erg=SQLGetInfo(dbc,SQL_ACCESSIBLE_PROCEDURES,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	// Both drivers report "N".
	assertEqualDbc(dbc,(const char *)strval,"N");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CURSOR_COMMIT_BEHAVIOR
	stdoutput.printf("  SQL_CURSOR_COMMIT_BEHAVIOR\n");
	erg=SQLGetInfo(dbc,SQL_CURSOR_COMMIT_BEHAVIOR,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,(int)SQL_CB_CLOSE);
	} else {
		// MariaDB reports SQL_CB_PRESERVE.
		assertEqualDbc(dbc,(int)usmallintval,(int)SQL_CB_PRESERVE);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_DATA_SOURCE_READ_ONLY
	stdoutput.printf("  SQL_DATA_SOURCE_READ_ONLY\n");
	erg=SQLGetInfo(dbc,SQL_DATA_SOURCE_READ_ONLY,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"N");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_DEFAULT_TXN_ISOLATION
	stdoutput.printf("  SQL_DEFAULT_TXN_ISOLATION\n");
	erg=SQLGetInfo(dbc,SQL_DEFAULT_TXN_ISOLATION,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,(int)SQL_TXN_READ_COMMITTED);
	} else {
		// MariaDB reports 0.
		assertEqualDbc(dbc,(int)uintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_IDENTIFIER_CASE
	stdoutput.printf("  SQL_IDENTIFIER_CASE\n");
	erg=SQLGetInfo(dbc,SQL_IDENTIFIER_CASE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,(int)SQL_IC_SENSITIVE);
	} else {
		// MySQL identifier case depends on the filesystem
		// (lower_case_table_names); SQL_IC_MIXED on linux
		// when lower_case_table_names=0
		assertEqualDbc(dbc,(int)usmallintval,(int)SQL_IC_MIXED);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_IDENTIFIER_QUOTE_CHAR
	stdoutput.printf("  SQL_IDENTIFIER_QUOTE_CHAR\n");
	erg=SQLGetInfo(dbc,SQL_IDENTIFIER_QUOTE_CHAR,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	// the backtick is the default; ANSI mode
	// would report the double-quote instead
	assertEqualDbc(dbc,(const char *)strval,"`");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_COLUMN_NAME_LEN
	stdoutput.printf("  SQL_MAX_COLUMN_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMN_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,64);
	} else {
		// MariaDB reports 255.
		assertEqualDbc(dbc,(int)usmallintval,255);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_CURSOR_NAME_LEN
	stdoutput.printf("  SQL_MAX_CURSOR_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_CURSOR_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,64);
	} else {
		// MariaDB reports 257.
		assertEqualDbc(dbc,(int)usmallintval,257);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_SCHEMA_NAME_LEN
	stdoutput.printf("  SQL_MAX_SCHEMA_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_SCHEMA_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,128);
	} else {
		// MariaDB reports 0; no schema namespace in MySQL.
		assertEqualDbc(dbc,(int)usmallintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_CATALOG_NAME_LEN
	stdoutput.printf("  SQL_MAX_CATALOG_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_CATALOG_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,32);
	} else {
		// MariaDB reports 256.
		assertEqualDbc(dbc,(int)usmallintval,256);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_TABLE_NAME_LEN
	stdoutput.printf("  SQL_MAX_TABLE_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_TABLE_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,64);
	} else {
		// MariaDB reports 256.
		assertEqualDbc(dbc,(int)usmallintval,256);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_SCROLL_CONCURRENCY
	stdoutput.printf("  SQL_SCROLL_CONCURRENCY\n");
	erg=SQLGetInfo(dbc,SQL_SCROLL_CONCURRENCY,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports SQL_SCCO_READ_ONLY|SQL_SCCO_OPT_VALUES.
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_SCCO_READ_ONLY|SQL_SCCO_OPT_VALUES));
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_TXN_CAPABLE
	stdoutput.printf("  SQL_TXN_CAPABLE\n");
	erg=SQLGetInfo(dbc,SQL_TXN_CAPABLE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_TC_DDL_COMMIT);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_USER_NAME
	stdoutput.printf("  SQL_USER_NAME\n");
	erg=SQLGetInfo(dbc,SQL_USER_NAME,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		// SQL Relay reports current_user() (account with
		// wildcard host)
		assertEqualDbc(dbc,(const char *)strval,"testuser@%");
	} else {
		// the native driver reports just the user name
		assertEqualDbc(dbc,(const char *)strval,"testuser");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_TXN_ISOLATION_OPTION
	stdoutput.printf("  SQL_TXN_ISOLATION_OPTION\n");
	erg=SQLGetInfo(dbc,SQL_TXN_ISOLATION_OPTION,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	// MySQL supports all four standard isolation levels
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_TXN_READ_UNCOMMITTED|SQL_TXN_READ_COMMITTED|
			SQL_TXN_REPEATABLE_READ|SQL_TXN_SERIALIZABLE));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_INTEGRITY
	stdoutput.printf("  SQL_INTEGRITY\n");
	erg=SQLGetInfo(dbc,SQL_INTEGRITY,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"N");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_GETDATA_EXTENSIONS
	stdoutput.printf("  SQL_GETDATA_EXTENSIONS\n");
	erg=SQLGetInfo(dbc,SQL_GETDATA_EXTENSIONS,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_GD_ANY_COLUMN|SQL_GD_ANY_ORDER|
				SQL_GD_BOUND|SQL_GD_BLOCK));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_NULL_COLLATION
	stdoutput.printf("  SQL_NULL_COLLATION\n");
	erg=SQLGetInfo(dbc,SQL_NULL_COLLATION,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_NC_LOW);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ALTER_TABLE
	stdoutput.printf("  SQL_ALTER_TABLE\n");
	erg=SQLGetInfo(dbc,SQL_ALTER_TABLE,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,1048547);
	} else {
		// MariaDB reports SQL_AT_ADD_COLUMN|SQL_AT_DROP_COLUMN.
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_AT_ADD_COLUMN|SQL_AT_DROP_COLUMN));
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ORDER_BY_COLUMNS_IN_SELECT
	stdoutput.printf("  SQL_ORDER_BY_COLUMNS_IN_SELECT\n");
	erg=SQLGetInfo(dbc,SQL_ORDER_BY_COLUMNS_IN_SELECT,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"Y");
	} else {
		assertEqualDbc(dbc,(const char *)strval,"N");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_SPECIAL_CHARACTERS
	stdoutput.printf("  SQL_SPECIAL_CHARACTERS\n");
	erg=SQLGetInfo(dbc,SQL_SPECIAL_CHARACTERS,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"$");
	} else {
		// MariaDB reports "\/.
		assertEqualDbc(dbc,(const char *)strval,"\"\\/");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_COLUMNS_IN_GROUP_BY
	stdoutput.printf("  SQL_MAX_COLUMNS_IN_GROUP_BY\n");
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_GROUP_BY,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,64);
	} else {
		assertEqualDbc(dbc,(int)usmallintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_COLUMNS_IN_INDEX
	stdoutput.printf("  SQL_MAX_COLUMNS_IN_INDEX\n");
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_INDEX,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,16);
	} else {
		// MariaDB reports 32.
		assertEqualDbc(dbc,(int)usmallintval,32);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_COLUMNS_IN_ORDER_BY
	stdoutput.printf("  SQL_MAX_COLUMNS_IN_ORDER_BY\n");
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_ORDER_BY,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,64);
	} else {
		assertEqualDbc(dbc,(int)usmallintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_COLUMNS_IN_SELECT
	stdoutput.printf("  SQL_MAX_COLUMNS_IN_SELECT\n");
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_SELECT,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,256);
	} else {
		// MariaDB reports 0.
		assertEqualDbc(dbc,(int)usmallintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_COLUMNS_IN_TABLE
	stdoutput.printf("  SQL_MAX_COLUMNS_IN_TABLE\n");
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_TABLE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,512);
	} else {
		// MariaDB reports 0.
		assertEqualDbc(dbc,(int)usmallintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_INDEX_SIZE
	stdoutput.printf("  SQL_MAX_INDEX_SIZE\n");
	erg=SQLGetInfo(dbc,SQL_MAX_INDEX_SIZE,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,256);
	} else {
		// MariaDB reports 3072 (InnoDB per-index byte limit).
		assertEqualDbc(dbc,(int)uintval,3072);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_ROW_SIZE
	stdoutput.printf("  SQL_MAX_ROW_SIZE\n");
	erg=SQLGetInfo(dbc,SQL_MAX_ROW_SIZE,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,2147483639);
	} else {
		assertEqualDbc(dbc,(int)uintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_STATEMENT_LEN
	stdoutput.printf("  SQL_MAX_STATEMENT_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_STATEMENT_LEN,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,65531);
	} else {
		// MariaDB reports 1073741824 (max_allowed_packet ceiling).
		assertEqualDbc(dbc,(int)uintval,1073741824);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_TABLES_IN_SELECT
	stdoutput.printf("  SQL_MAX_TABLES_IN_SELECT\n");
	erg=SQLGetInfo(dbc,SQL_MAX_TABLES_IN_SELECT,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,256);
	} else {
		// MariaDB reports 63 (MySQL's per-join cap).
		assertEqualDbc(dbc,(int)usmallintval,63);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_USER_NAME_LEN
	stdoutput.printf("  SQL_MAX_USER_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_USER_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,16);
	} else {
		// MariaDB reports 512.
		assertEqualDbc(dbc,(int)usmallintval,512);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	#if (ODBCVER >= 0x0300)
	// SQL_OJ_CAPABILITIES
	stdoutput.printf("  SQL_OJ_CAPABILITIES\n");
	erg=SQLGetInfo(dbc,SQL_OJ_CAPABILITIES,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,123);
	} else {
		// MariaDB reports 43.
		assertEqualDbc(dbc,(int)uintval,43);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_XOPEN_CLI_YEAR
	stdoutput.printf("  SQL_XOPEN_CLI_YEAR\n");
	erg=SQLGetInfo(dbc,SQL_XOPEN_CLI_YEAR,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"1995");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_CURSOR_SENSITIVITY
	stdoutput.printf("  SQL_CURSOR_SENSITIVITY\n");
	erg=SQLGetInfo(dbc,SQL_CURSOR_SENSITIVITY,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,(int)SQL_INSENSITIVE);
	} else {
		// MariaDB reports SQL_UNSPECIFIED.
		assertEqualDbc(dbc,(int)uintval,(int)SQL_UNSPECIFIED);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DESCRIBE_PARAMETER
	stdoutput.printf("  SQL_DESCRIBE_PARAMETER\n");
	erg=SQLGetInfo(dbc,SQL_DESCRIBE_PARAMETER,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	// Both drivers report "N".
	assertEqualDbc(dbc,(const char *)strval,"N");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_CATALOG_NAME
	stdoutput.printf("  SQL_CATALOG_NAME\n");
	erg=SQLGetInfo(dbc,SQL_CATALOG_NAME,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	// Both drivers report "Y".
	assertEqualDbc(dbc,(const char *)strval,"Y");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_COLLATION_SEQ
	stdoutput.printf("  SQL_COLLATION_SEQ\n");
	erg=SQLGetInfo(dbc,SQL_COLLATION_SEQ,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"");
	} else {
		// MariaDB reports the connection's actual collation.
		assertEqualDbc(dbc,(const char *)strval,
				"utf8mb4_general_ci");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_MAX_IDENTIFIER_LEN
	stdoutput.printf("  SQL_MAX_IDENTIFIER_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_IDENTIFIER_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,0);
	} else {
		// MariaDB reports 256.
		assertEqualDbc(dbc,(int)usmallintval,256);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	// SQL_DRIVER_HDBC
	// driver-manager-level; the underlying driver's handle, non-zero
	stdoutput.printf("  SQL_DRIVER_HDBC\n");
	handleval=0;
	erg=SQLGetInfo(dbc,SQL_DRIVER_HDBC,
			(SQLPOINTER)&handleval,
			(SQLSMALLINT)sizeof(handleval),&vallen);
	assertSuccessDbc(dbc,erg);
	assertTrueDbc(dbc,handleval!=0);
	stdoutput.printf("\n");


	// SQL_DRIVER_HENV
	// driver-manager-level; the underlying driver's handle, non-zero
	stdoutput.printf("  SQL_DRIVER_HENV\n");
	handleval=0;
	erg=SQLGetInfo(dbc,SQL_DRIVER_HENV,
			(SQLPOINTER)&handleval,
			(SQLSMALLINT)sizeof(handleval),&vallen);
	assertSuccessDbc(dbc,erg);
	assertTrueDbc(dbc,handleval!=0);
	stdoutput.printf("\n");


	// SQL_DRIVER_HSTMT
	// driver-manager-level; pass the dm statement handle in, the
	// underlying driver's handle (non-zero) comes back
	stdoutput.printf("  SQL_DRIVER_HSTMT\n");
	SQLHSTMT	hstmtval;
	erg=SQLAllocHandle(SQL_HANDLE_STMT,dbc,&hstmtval);
	assertSuccessDbc(dbc,erg);
	handleval=(SQLULEN)hstmtval;
	erg=SQLGetInfo(dbc,SQL_DRIVER_HSTMT,
			(SQLPOINTER)&handleval,
			(SQLSMALLINT)sizeof(handleval),&vallen);
	assertSuccessDbc(dbc,erg);
	assertTrueDbc(dbc,handleval!=0);
	erg=SQLFreeHandle(SQL_HANDLE_STMT,hstmtval);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_DRIVER_NAME
	stdoutput.printf("  SQL_DRIVER_NAME\n");
	erg=SQLGetInfo(dbc,SQL_DRIVER_NAME,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"libsqlrodbc.so");
	} else {
		// TODO: pin native filename (libmyodbc8w.so etc.);
		// for now require non-empty
		assertTrueDbc(dbc,vallen>0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_DRIVER_VER
	stdoutput.printf("  SQL_DRIVER_VER\n");
	erg=SQLGetInfo(dbc,SQL_DRIVER_VER,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertIsVersionDbc(dbc,(const char *)strval);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ODBC_API_CONFORMANCE
	stdoutput.printf("  SQL_ODBC_API_CONFORMANCE\n");
	erg=SQLGetInfo(dbc,SQL_ODBC_API_CONFORMANCE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,(int)SQL_OAC_LEVEL2);
	} else {
		assertEqualDbc(dbc,(int)usmallintval,(int)SQL_OAC_LEVEL1);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ODBC_VER
	stdoutput.printf("  SQL_ODBC_VER\n");
	erg=SQLGetInfo(dbc,SQL_ODBC_VER,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertIsVersionDbc(dbc,(const char *)strval);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ROW_UPDATES
	stdoutput.printf("  SQL_ROW_UPDATES\n");
	erg=SQLGetInfo(dbc,SQL_ROW_UPDATES,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	// Both drivers report "N".
	assertEqualDbc(dbc,(const char *)strval,"N");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ODBC_SAG_CLI_CONFORMANCE
	stdoutput.printf("  SQL_ODBC_SAG_CLI_CONFORMANCE\n");
	erg=SQLGetInfo(dbc,SQL_ODBC_SAG_CLI_CONFORMANCE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,
				(int)SQL_OSCC_NOT_COMPLIANT);
		assertSuccessDbc(dbc,erg);
	} else {
		// MariaDB returns HY096 "Invalid information type".
		assertFailureDbc(dbc,erg);
	}
	stdoutput.printf("\n");


	// SQL_ODBC_SQL_CONFORMANCE
	stdoutput.printf("  SQL_ODBC_SQL_CONFORMANCE\n");
	erg=SQLGetInfo(dbc,SQL_ODBC_SQL_CONFORMANCE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_OSC_CORE);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_PROCEDURES
	stdoutput.printf("  SQL_PROCEDURES\n");
	erg=SQLGetInfo(dbc,SQL_PROCEDURES,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"Y");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONCAT_NULL_BEHAVIOR
	stdoutput.printf("  SQL_CONCAT_NULL_BEHAVIOR\n");
	erg=SQLGetInfo(dbc,SQL_CONCAT_NULL_BEHAVIOR,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	// Both drivers report SQL_CB_NULL.
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_CB_NULL);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CURSOR_ROLLBACK_BEHAVIOR
	stdoutput.printf("  SQL_CURSOR_ROLLBACK_BEHAVIOR\n");
	erg=SQLGetInfo(dbc,SQL_CURSOR_ROLLBACK_BEHAVIOR,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,(int)SQL_CB_CLOSE);
	} else {
		assertEqualDbc(dbc,(int)usmallintval,(int)SQL_CB_PRESERVE);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_EXPRESSIONS_IN_ORDERBY
	stdoutput.printf("  SQL_EXPRESSIONS_IN_ORDERBY\n");
	erg=SQLGetInfo(dbc,SQL_EXPRESSIONS_IN_ORDERBY,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"Y");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_OWNER_NAME_LEN
	stdoutput.printf("  SQL_MAX_OWNER_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_OWNER_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,128);
	} else {
		// MariaDB reports 0.
		assertEqualDbc(dbc,(int)usmallintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_PROCEDURE_NAME_LEN
	stdoutput.printf("  SQL_MAX_PROCEDURE_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_PROCEDURE_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,0);
	} else {
		// MariaDB reports 256.
		assertEqualDbc(dbc,(int)usmallintval,256);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_QUALIFIER_NAME_LEN
	stdoutput.printf("  SQL_MAX_QUALIFIER_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_QUALIFIER_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,32);
	} else {
		// MariaDB reports 256.
		assertEqualDbc(dbc,(int)usmallintval,256);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MULT_RESULT_SETS
	stdoutput.printf("  SQL_MULT_RESULT_SETS\n");
	erg=SQLGetInfo(dbc,SQL_MULT_RESULT_SETS,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"Y");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MULTIPLE_ACTIVE_TXN
	stdoutput.printf("  SQL_MULTIPLE_ACTIVE_TXN\n");
	erg=SQLGetInfo(dbc,SQL_MULTIPLE_ACTIVE_TXN,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"Y");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_OUTER_JOINS
	stdoutput.printf("  SQL_OUTER_JOINS\n");
	erg=SQLGetInfo(dbc,SQL_OUTER_JOINS,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"Y");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_OWNER_TERM
	stdoutput.printf("  SQL_OWNER_TERM\n");
	erg=SQLGetInfo(dbc,SQL_OWNER_TERM,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	// Both drivers report "".
	assertEqualDbc(dbc,(const char *)strval,"");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_PROCEDURE_TERM
	stdoutput.printf("  SQL_PROCEDURE_TERM\n");
	erg=SQLGetInfo(dbc,SQL_PROCEDURE_TERM,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"PROCEDURE");
	} else {
		// MariaDB reports "stored procedure".
		assertEqualDbc(dbc,(const char *)strval,"stored procedure");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_QUALIFIER_NAME_SEPARATOR
	stdoutput.printf("  SQL_QUALIFIER_NAME_SEPARATOR\n");
	erg=SQLGetInfo(dbc,SQL_QUALIFIER_NAME_SEPARATOR,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	// Both drivers report ".".
	assertEqualDbc(dbc,(const char *)strval,".");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_QUALIFIER_TERM
	stdoutput.printf("  SQL_QUALIFIER_TERM\n");
	erg=SQLGetInfo(dbc,SQL_QUALIFIER_TERM,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	// Both drivers report "database".
	assertEqualDbc(dbc,(const char *)strval,"database");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_SCROLL_OPTIONS
	stdoutput.printf("  SQL_SCROLL_OPTIONS\n");
	erg=SQLGetInfo(dbc,SQL_SCROLL_OPTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// Both drivers report SQL_SO_FORWARD_ONLY|SQL_SO_STATIC.
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_SO_FORWARD_ONLY|SQL_SO_STATIC));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_TABLE_TERM
	stdoutput.printf("  SQL_TABLE_TERM\n");
	erg=SQLGetInfo(dbc,SQL_TABLE_TERM,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	// Both drivers report "table".
	assertEqualDbc(dbc,(const char *)strval,"table");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_FUNCTIONS
	// (uintval is poisoned before each get in the conversion run;
	// many expected values are identical, so a success-without-write
	// would otherwise false-pass)
	stdoutput.printf("  SQL_CONVERT_FUNCTIONS\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,(int)SQL_FN_CVT_CAST);
	} else {
		// MariaDB reports 0.
		assertEqualDbc(dbc,(int)uintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_NUMERIC_FUNCTIONS
	stdoutput.printf("  SQL_NUMERIC_FUNCTIONS\n");
	erg=SQLGetInfo(dbc,SQL_NUMERIC_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,16773119);
	} else {
		// MariaDB reports 16777215 (all SQL_FN_NUM_* flags).
		assertEqualDbc(dbc,(int)uintval,16777215);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_STRING_FUNCTIONS
	stdoutput.printf("  SQL_STRING_FUNCTIONS\n");
	erg=SQLGetInfo(dbc,SQL_STRING_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,16678911);
	} else {
		// MariaDB reports 14647295.
		assertEqualDbc(dbc,(int)uintval,14647295);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_SYSTEM_FUNCTIONS
	stdoutput.printf("  SQL_SYSTEM_FUNCTIONS\n");
	erg=SQLGetInfo(dbc,SQL_SYSTEM_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		// the mysql connection module doesn't advertise IFNULL
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_FN_SYS_USERNAME|SQL_FN_SYS_DBNAME));
	} else {
		// MariaDB reports USERNAME|DBNAME|IFNULL.
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_FN_SYS_USERNAME|SQL_FN_SYS_DBNAME|
				SQL_FN_SYS_IFNULL));
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_TIMEDATE_FUNCTIONS
	stdoutput.printf("  SQL_TIMEDATE_FUNCTIONS\n");
	erg=SQLGetInfo(dbc,SQL_TIMEDATE_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,1023999);
	} else {
		// MariaDB reports 2097151 (all SQL_FN_TD_* flags).
		assertEqualDbc(dbc,(int)uintval,2097151);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_BIGINT
	stdoutput.printf("  SQL_CONVERT_BIGINT\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_BIGINT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_BINARY
	stdoutput.printf("  SQL_CONVERT_BINARY\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_BINARY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_BIT
	stdoutput.printf("  SQL_CONVERT_BIT\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_BIT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_CHAR
	stdoutput.printf("  SQL_CONVERT_CHAR\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_CHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_DATE
	stdoutput.printf("  SQL_CONVERT_DATE\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_DATE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_DECIMAL
	stdoutput.printf("  SQL_CONVERT_DECIMAL\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_DECIMAL,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_DOUBLE
	stdoutput.printf("  SQL_CONVERT_DOUBLE\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_DOUBLE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_FLOAT
	stdoutput.printf("  SQL_CONVERT_FLOAT\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_FLOAT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_INTEGER
	stdoutput.printf("  SQL_CONVERT_INTEGER\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_INTEGER,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_LONGVARCHAR
	stdoutput.printf("  SQL_CONVERT_LONGVARCHAR\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_LONGVARCHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_NUMERIC
	stdoutput.printf("  SQL_CONVERT_NUMERIC\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_NUMERIC,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_REAL
	stdoutput.printf("  SQL_CONVERT_REAL\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_REAL,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_SMALLINT
	stdoutput.printf("  SQL_CONVERT_SMALLINT\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_SMALLINT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_TIME
	stdoutput.printf("  SQL_CONVERT_TIME\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_TIME,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_TIMESTAMP
	stdoutput.printf("  SQL_CONVERT_TIMESTAMP\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_TIMESTAMP,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_TINYINT
	stdoutput.printf("  SQL_CONVERT_TINYINT\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_TINYINT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_VARBINARY
	stdoutput.printf("  SQL_CONVERT_VARBINARY\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_VARBINARY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_VARCHAR
	stdoutput.printf("  SQL_CONVERT_VARCHAR\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_VARCHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_LONGVARBINARY
	stdoutput.printf("  SQL_CONVERT_LONGVARBINARY\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_LONGVARBINARY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_GUID
	stdoutput.printf("  SQL_CONVERT_GUID\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_GUID,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		// SQL Relay returns HYC00 "Optional feature not implemented"
		assertFailureDbc(dbc,erg);
	} else {
		// MariaDB returns 0.
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)uintval,0);
	}
	stdoutput.printf("\n");


	// SQL_CORRELATION_NAME
	stdoutput.printf("  SQL_CORRELATION_NAME\n");
	erg=SQLGetInfo(dbc,SQL_CORRELATION_NAME,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	// Both drivers report SQL_CN_DIFFERENT.
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_CN_DIFFERENT);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_NON_NULLABLE_COLUMNS
	stdoutput.printf("  SQL_NON_NULLABLE_COLUMNS\n");
	erg=SQLGetInfo(dbc,SQL_NON_NULLABLE_COLUMNS,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_NNC_NON_NULL);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_DRIVER_HLIB
	// driver-manager-level; the driver's shared-library handle, non-zero
	stdoutput.printf("  SQL_DRIVER_HLIB\n");
	handleval=0;
	erg=SQLGetInfo(dbc,SQL_DRIVER_HLIB,
			(SQLPOINTER)&handleval,
			(SQLSMALLINT)sizeof(handleval),&vallen);
	assertSuccessDbc(dbc,erg);
	assertTrueDbc(dbc,handleval!=0);
	stdoutput.printf("\n");


	// SQL_DRIVER_ODBC_VER
	stdoutput.printf("  SQL_DRIVER_ODBC_VER\n");
	erg=SQLGetInfo(dbc,SQL_DRIVER_ODBC_VER,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertIsVersionDbc(dbc,(const char *)strval);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_LOCK_TYPES
	stdoutput.printf("  SQL_LOCK_TYPES\n");
	erg=SQLGetInfo(dbc,SQL_LOCK_TYPES,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,(int)SQL_LCK_NO_CHANGE);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_POS_OPERATIONS
	stdoutput.printf("  SQL_POS_OPERATIONS\n");
	erg=SQLGetInfo(dbc,SQL_POS_OPERATIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,(int)SQL_POS_POSITION);
	} else {
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_POS_POSITION|SQL_POS_REFRESH|
				SQL_POS_UPDATE|SQL_POS_DELETE|
				SQL_POS_ADD));
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_POSITIONED_STATEMENTS
	stdoutput.printf("  SQL_POSITIONED_STATEMENTS\n");
	erg=SQLGetInfo(dbc,SQL_POSITIONED_STATEMENTS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
		assertSuccessDbc(dbc,erg);
	} else {
		// MariaDB returns HY096 "Invalid information type".
		assertFailureDbc(dbc,erg);
	}
	stdoutput.printf("\n");


	// SQL_BOOKMARK_PERSISTENCE
	stdoutput.printf("  SQL_BOOKMARK_PERSISTENCE\n");
	erg=SQLGetInfo(dbc,SQL_BOOKMARK_PERSISTENCE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// Both drivers report 0.
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_STATIC_SENSITIVITY
	stdoutput.printf("  SQL_STATIC_SENSITIVITY\n");
	erg=SQLGetInfo(dbc,SQL_STATIC_SENSITIVITY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports SQL_SS_DELETIONS|SQL_SS_UPDATES.
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_SS_DELETIONS|SQL_SS_UPDATES));
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_FILE_USAGE
	stdoutput.printf("  SQL_FILE_USAGE\n");
	erg=SQLGetInfo(dbc,SQL_FILE_USAGE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_FILE_NOT_SUPPORTED);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_COLUMN_ALIAS
	stdoutput.printf("  SQL_COLUMN_ALIAS\n");
	erg=SQLGetInfo(dbc,SQL_COLUMN_ALIAS,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"Y");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_GROUP_BY
	stdoutput.printf("  SQL_GROUP_BY\n");
	erg=SQLGetInfo(dbc,SQL_GROUP_BY,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		// sqlrelay returns SQL_GB_NO_RELATION|SQL_GB_COLLATE;
		// not a legal enum per spec (SQL_GROUP_BY is single enum,
		// not a bitmask)
		assertEqualDbc(dbc,(int)usmallintval,
			(int)(SQL_GB_NO_RELATION|SQL_GB_COLLATE));
	} else {
		// MariaDB reports SQL_GB_NO_RELATION.
		assertEqualDbc(dbc,(int)usmallintval,
			(int)SQL_GB_NO_RELATION);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_KEYWORDS
	stdoutput.printf("  SQL_KEYWORDS\n");
	erg=SQLGetInfo(dbc,SQL_KEYWORDS,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"ACCESSIBLE,ADD,ANALYZE,ASC,BEFORE,CASCADE,CHANGE,CONTINUE,DATABASE,DATABASES,DAY_HOUR,DAY_MICROSECOND,DAY_MINUTE,DAY_SECOND,DELAYED,DESC,DISTINCTROW,DIV,DUAL,ELSEIF,EMPTY,ENCLOSED,ESCAPED,EXIT,EXPLAIN,FIRST_VALUE,FLOAT4,FLOAT8,FORCE,FULLTEXT,GENERATED,GROUPS,HIGH_PRIORITY,HOUR_MICROSECOND,HOUR_MINUTE,HOUR_SECOND,IF,IGNORE,INDEX,INFILE,INT1,INT2,INT3,INT4,INT8,IO_AFTER_GTIDS,IO_BEFORE_GTIDS,ITERATE,JSON_TABLE,KEY,KEYS,KILL,LAG,LAST_VALUE,LEAD,LEAVE,LIMIT,LINEAR,LINES,LOAD,LOCK,LONG,LONGBLOB,LONGTEXT,LOOP,LOW_PRIORITY,MANUAL,MASTER_BIND,MASTER_SSL_VERIFY_SERVER_CERT,MAXVALUE,MEDIUMBLOB,MEDIUMINT,MEDIUMTEXT,MIDDLEINT,MINUTE_MICROSECOND,MINUTE_SECOND,NO_WRITE_TO_BINLOG,NTH_VALUE,NTILE,OPTIMIZE,OPTIMIZER_COSTS,OPTION,OPTIONALLY,OUTFILE,PARALLEL,PURGE,QUALIFY,READ,READ_WRITE,REGEXP,RENAME,REPEAT,REPLACE,REQUIRE,RESIGNAL,RESTRICT,RLIKE,SCHEMA,SCHEMAS,SECOND_MICROSECOND,SEPARATOR,SHOW,SIGNAL,SPATIAL,SQL_BIG_RESULT,SQL_CALC_FOUND_ROWS,SQL_SMALL_RESULT,SSL,STARTING,STORED,STRAIGHT_JOIN,TERMINATED,TINYBLOB,TINYINT,TINYTEXT,UNDO,UNLOCK,UNSIGNED,USAGE,USE,UTC_DATE,UTC_TIME,UTC_TIMESTAMP,VARBINARY,VARCHARACTER,VIRTUAL,WHILE,WRITE,XOR,YEAR_MONTH,ZEROFILL");
	} else {
		// MariaDB returns the server's reserved keywords list.
		assertEqualDbc(dbc,(const char *)strval,"ACCESSIBLE,ANALYZE,ASENSITIVE,BEFORE,BIGINT,BINARY,BLOB,CALL,CHANGE,CONDITION,DATABASE,DATABASES,DAY_HOUR,DAY_MICROSECOND,DAY_MINUTE,DAY_SECOND,DELAYED,DETERMINISTIC,DISTINCTROW,DIV,DUAL,EACH,ELSEIF,ENCLOSED,ESCAPED,EXIT,EXPLAIN,FLOAT4,FLOAT8,FORCE,FULLTEXT,HIGH_PRIORITY,HOUR_MICROSECOND,HOUR_MINUTE,HOUR_SECOND,IF,IGNORE,INFILE,INOUT,INT1,INT2,INT3,INT4,INT8,ITERATE,KEY,KEYS,KILL,LEAVE,LIMIT,LINEAR,LINES,LOAD,LOCALTIME,LOCALTIMESTAMP,LOCK,LONG,LONGBLOB,LONGTEXT,LOOP,LOW_PRIORITY,MEDIUMBLOB,MEDIUMINT,MEDIUMTEXT,MIDDLEINT,MINUTE_MICROSECOND,MINUTE_SECOND,MOD,MODIFIES,NO_WRITE_TO_BINLOG,OPTIMIZE,OPTIONALLY,OUT,OUTFILE,PURGE,RANGE,READS,READ_ONLY,READ_WRITE,REGEXP,RELEASE,RENAME,REPEAT,REPLACE,REQUIRE,RETURN,RLIKE,SCHEMAS,SECOND_MICROSECOND,SENSITIVE,SEPARATOR,SHOW,SPATIAL,SPECIFIC,SQLEXCEPTION,SQL_BIG_RESULT,SQL_CALC_FOUND_ROWS,SQL_SMALL_RESULT,SSL,STARTING,STRAIGHT_JOIN,TERMINATED,TINYBLOB,TINYINT,TINYTEXT,TRIGGER,UNDO,UNLOCK,UNSIGNED,USE,UTC_DATE,UTC_TIME,UTC_TIMESTAMP,VARBINARY,VARCHARACTER,WHILE,X509,XOR,YEAR_MONTH,ZEROFILL,GENERAL,IGNORE_SERVER_IDS,MASTER_HEARTBEAT_PERIOD,MAXVALUE,RESIGNAL,SIGNAL,SLOW");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_OWNER_USAGE
	stdoutput.printf("  SQL_OWNER_USAGE\n");
	erg=SQLGetInfo(dbc,SQL_OWNER_USAGE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// Both drivers report 0.
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_QUALIFIER_USAGE
	stdoutput.printf("  SQL_QUALIFIER_USAGE\n");
	erg=SQLGetInfo(dbc,SQL_QUALIFIER_USAGE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// Both drivers report all SQL_QU_* flags.
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_QU_DML_STATEMENTS|
			SQL_QU_PROCEDURE_INVOCATION|
			SQL_QU_TABLE_DEFINITION|
			SQL_QU_INDEX_DEFINITION|
			SQL_QU_PRIVILEGE_DEFINITION));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_QUOTED_IDENTIFIER_CASE
	stdoutput.printf("  SQL_QUOTED_IDENTIFIER_CASE\n");
	erg=SQLGetInfo(dbc,SQL_QUOTED_IDENTIFIER_CASE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_IC_SENSITIVE);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_SUBQUERIES
	stdoutput.printf("  SQL_SUBQUERIES\n");
	erg=SQLGetInfo(dbc,SQL_SUBQUERIES,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_SQ_COMPARISON|SQL_SQ_EXISTS|SQL_SQ_IN|
			SQL_SQ_QUANTIFIED|SQL_SQ_CORRELATED_SUBQUERIES));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_UNION
	stdoutput.printf("  SQL_UNION\n");
	erg=SQLGetInfo(dbc,SQL_UNION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_U_UNION|SQL_U_UNION_ALL));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_ROW_SIZE_INCLUDES_LONG
	stdoutput.printf("  SQL_MAX_ROW_SIZE_INCLUDES_LONG\n");
	erg=SQLGetInfo(dbc,SQL_MAX_ROW_SIZE_INCLUDES_LONG,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	// Both drivers report "Y".
	assertEqualDbc(dbc,(const char *)strval,"Y");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_CHAR_LITERAL_LEN
	stdoutput.printf("  SQL_MAX_CHAR_LITERAL_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_CHAR_LITERAL_LEN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,16777208);
	} else {
		assertEqualDbc(dbc,(int)uintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_TIMEDATE_ADD_INTERVALS
	stdoutput.printf("  SQL_TIMEDATE_ADD_INTERVALS\n");
	erg=SQLGetInfo(dbc,SQL_TIMEDATE_ADD_INTERVALS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_TIMEDATE_DIFF_INTERVALS
	stdoutput.printf("  SQL_TIMEDATE_DIFF_INTERVALS\n");
	erg=SQLGetInfo(dbc,SQL_TIMEDATE_DIFF_INTERVALS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_NEED_LONG_DATA_LEN
	stdoutput.printf("  SQL_NEED_LONG_DATA_LEN\n");
	erg=SQLGetInfo(dbc,SQL_NEED_LONG_DATA_LEN,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"N");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_BINARY_LITERAL_LEN
	stdoutput.printf("  SQL_MAX_BINARY_LITERAL_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_BINARY_LITERAL_LEN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,16777208);
	} else {
		assertEqualDbc(dbc,(int)uintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_LIKE_ESCAPE_CLAUSE
	stdoutput.printf("  SQL_LIKE_ESCAPE_CLAUSE\n");
	erg=SQLGetInfo(dbc,SQL_LIKE_ESCAPE_CLAUSE,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"Y");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_QUALIFIER_LOCATION
	stdoutput.printf("  SQL_QUALIFIER_LOCATION\n");
	erg=SQLGetInfo(dbc,SQL_QUALIFIER_LOCATION,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	// Both drivers report SQL_QL_START.
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_QL_START);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	#if (ODBCVER >= 0x0300)
	// SQL_ACTIVE_ENVIRONMENTS
	stdoutput.printf("  SQL_ACTIVE_ENVIRONMENTS\n");
	erg=SQLGetInfo(dbc,SQL_ACTIVE_ENVIRONMENTS,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	// Both drivers report 0.
	assertEqualDbc(dbc,(int)usmallintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_ALTER_DOMAIN
	stdoutput.printf("  SQL_ALTER_DOMAIN\n");
	erg=SQLGetInfo(dbc,SQL_ALTER_DOMAIN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL_CONFORMANCE
	stdoutput.printf("  SQL_SQL_CONFORMANCE\n");
	erg=SQLGetInfo(dbc,SQL_SQL_CONFORMANCE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,(int)SQL_SC_SQL92_ENTRY);
	} else {
		// MariaDB reports SQL_SC_SQL92_INTERMEDIATE.
		assertEqualDbc(dbc,(int)uintval,
			(int)SQL_SC_SQL92_INTERMEDIATE);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DATETIME_LITERALS
	stdoutput.printf("  SQL_DATETIME_LITERALS\n");
	erg=SQLGetInfo(dbc,SQL_DATETIME_LITERALS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// Both drivers report DATE|TIME|TIMESTAMP.
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_DL_SQL92_DATE|SQL_DL_SQL92_TIME|
			SQL_DL_SQL92_TIMESTAMP));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_ASYNC_MODE
	stdoutput.printf("  SQL_ASYNC_MODE\n");
	erg=SQLGetInfo(dbc,SQL_ASYNC_MODE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_BATCH_ROW_COUNT
	stdoutput.printf("  SQL_BATCH_ROW_COUNT\n");
	erg=SQLGetInfo(dbc,SQL_BATCH_ROW_COUNT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// Both drivers report SQL_BRC_EXPLICIT.
	assertEqualDbc(dbc,(int)uintval,(int)SQL_BRC_EXPLICIT);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_BATCH_SUPPORT
	stdoutput.printf("  SQL_BATCH_SUPPORT\n");
	erg=SQLGetInfo(dbc,SQL_BATCH_SUPPORT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// Both drivers report all four SQL_BS_* flags.
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_BS_SELECT_EXPLICIT|
			SQL_BS_ROW_COUNT_EXPLICIT|
			SQL_BS_SELECT_PROC|
			SQL_BS_ROW_COUNT_PROC));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_CONVERT_WCHAR
	stdoutput.printf("  SQL_CONVERT_WCHAR\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_WCHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_CONVERT_INTERVAL_DAY_TIME
	stdoutput.printf("  SQL_CONVERT_INTERVAL_DAY_TIME\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_INTERVAL_DAY_TIME,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_CONVERT_INTERVAL_YEAR_MONTH
	stdoutput.printf("  SQL_CONVERT_INTERVAL_YEAR_MONTH\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_INTERVAL_YEAR_MONTH,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_CONVERT_WLONGVARCHAR
	stdoutput.printf("  SQL_CONVERT_WLONGVARCHAR\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_WLONGVARCHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_CONVERT_WVARCHAR
	stdoutput.printf("  SQL_CONVERT_WVARCHAR\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_WVARCHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 0xE3F3FF.
		assertEqualDbc(dbc,(int)uintval,0xE3F3FF);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_CREATE_ASSERTION
	stdoutput.printf("  SQL_CREATE_ASSERTION\n");
	erg=SQLGetInfo(dbc,SQL_CREATE_ASSERTION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_CREATE_CHARACTER_SET
	stdoutput.printf("  SQL_CREATE_CHARACTER_SET\n");
	erg=SQLGetInfo(dbc,SQL_CREATE_CHARACTER_SET,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_CREATE_COLLATION
	stdoutput.printf("  SQL_CREATE_COLLATION\n");
	erg=SQLGetInfo(dbc,SQL_CREATE_COLLATION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_CREATE_DOMAIN
	stdoutput.printf("  SQL_CREATE_DOMAIN\n");
	erg=SQLGetInfo(dbc,SQL_CREATE_DOMAIN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_CREATE_SCHEMA
	stdoutput.printf("  SQL_CREATE_SCHEMA\n");
	erg=SQLGetInfo(dbc,SQL_CREATE_SCHEMA,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// Both drivers report 0.
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_CREATE_TABLE
	stdoutput.printf("  SQL_CREATE_TABLE\n");
	erg=SQLGetInfo(dbc,SQL_CREATE_TABLE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// Both drivers report CREATE_TABLE|COMMIT_DELETE|
	// LOCAL_TEMPORARY|COLUMN_DEFAULT|COLUMN_COLLATION.
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_CT_CREATE_TABLE|
			SQL_CT_COMMIT_DELETE|
			SQL_CT_LOCAL_TEMPORARY|
			SQL_CT_COLUMN_DEFAULT|
			SQL_CT_COLUMN_COLLATION));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_CREATE_TRANSLATION
	stdoutput.printf("  SQL_CREATE_TRANSLATION\n");
	erg=SQLGetInfo(dbc,SQL_CREATE_TRANSLATION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_CREATE_VIEW
	stdoutput.printf("  SQL_CREATE_VIEW\n");
	erg=SQLGetInfo(dbc,SQL_CREATE_VIEW,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// Both drivers report CREATE_VIEW|CHECK_OPTION|CASCADED.
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_CV_CREATE_VIEW|SQL_CV_CHECK_OPTION|
			SQL_CV_CASCADED));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DRIVER_HDESC
	// needs an app-allocated HDESC via InfoValuePtr (driver-manager
	// internal); unixODBC rejects this call with HY024
	stdoutput.printf("  SQL_DRIVER_HDESC\n");
	erg=SQLGetInfo(dbc,SQL_DRIVER_HDESC,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertFailureDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DROP_ASSERTION
	stdoutput.printf("  SQL_DROP_ASSERTION\n");
	erg=SQLGetInfo(dbc,SQL_DROP_ASSERTION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DROP_CHARACTER_SET
	stdoutput.printf("  SQL_DROP_CHARACTER_SET\n");
	erg=SQLGetInfo(dbc,SQL_DROP_CHARACTER_SET,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DROP_COLLATION
	stdoutput.printf("  SQL_DROP_COLLATION\n");
	erg=SQLGetInfo(dbc,SQL_DROP_COLLATION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DROP_DOMAIN
	stdoutput.printf("  SQL_DROP_DOMAIN\n");
	erg=SQLGetInfo(dbc,SQL_DROP_DOMAIN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DROP_SCHEMA
	stdoutput.printf("  SQL_DROP_SCHEMA\n");
	erg=SQLGetInfo(dbc,SQL_DROP_SCHEMA,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DROP_TABLE
	stdoutput.printf("  SQL_DROP_TABLE\n");
	erg=SQLGetInfo(dbc,SQL_DROP_TABLE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// Both drivers report DROP_TABLE|RESTRICT|CASCADE.
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_DT_DROP_TABLE|SQL_DT_RESTRICT|
			SQL_DT_CASCADE));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DROP_TRANSLATION
	stdoutput.printf("  SQL_DROP_TRANSLATION\n");
	erg=SQLGetInfo(dbc,SQL_DROP_TRANSLATION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DROP_VIEW
	stdoutput.printf("  SQL_DROP_VIEW\n");
	erg=SQLGetInfo(dbc,SQL_DROP_VIEW,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// Both drivers report DROP_VIEW|RESTRICT|CASCADE.
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_DV_DROP_VIEW|SQL_DV_RESTRICT|
			SQL_DV_CASCADE));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DYNAMIC_CURSOR_ATTRIBUTES1
	stdoutput.printf("  SQL_DYNAMIC_CURSOR_ATTRIBUTES1\n");
	erg=SQLGetInfo(dbc,SQL_DYNAMIC_CURSOR_ATTRIBUTES1,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// sqlrelay doesn't support dynamic cursors
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_CA1_NEXT|SQL_CA1_ABSOLUTE|
				SQL_CA1_RELATIVE|SQL_CA1_LOCK_NO_CHANGE|
				SQL_CA1_POS_POSITION|SQL_CA1_POS_UPDATE|
				SQL_CA1_POS_DELETE|SQL_CA1_POS_REFRESH|
				SQL_CA1_POSITIONED_UPDATE|
				SQL_CA1_POSITIONED_DELETE|
				SQL_CA1_BULK_ADD));
	}
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DYNAMIC_CURSOR_ATTRIBUTES2
	stdoutput.printf("  SQL_DYNAMIC_CURSOR_ATTRIBUTES2\n");
	erg=SQLGetInfo(dbc,SQL_DYNAMIC_CURSOR_ATTRIBUTES2,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// sqlrelay doesn't support dynamic cursors
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_CA2_SENSITIVITY_ADDITIONS|
				SQL_CA2_SENSITIVITY_DELETIONS|
				SQL_CA2_SENSITIVITY_UPDATES|
				SQL_CA2_MAX_ROWS_SELECT|
				SQL_CA2_MAX_ROWS_INSERT|
				SQL_CA2_MAX_ROWS_DELETE|
				SQL_CA2_MAX_ROWS_UPDATE|
				SQL_CA2_CRC_EXACT|
				SQL_CA2_SIMULATE_TRY_UNIQUE));
	}
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1
	stdoutput.printf("  SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1\n");
	erg=SQLGetInfo(dbc,SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 97863.
		assertEqualDbc(dbc,(int)uintval,97863);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2
	stdoutput.printf("  SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2\n");
	erg=SQLGetInfo(dbc,SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 6016.
		assertEqualDbc(dbc,(int)uintval,6016);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_INDEX_KEYWORDS
	stdoutput.printf("  SQL_INDEX_KEYWORDS\n");
	erg=SQLGetInfo(dbc,SQL_INDEX_KEYWORDS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// Both drivers report SQL_IK_ASC|SQL_IK_DESC.
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_IK_ASC|SQL_IK_DESC));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_INFO_SCHEMA_VIEWS
	stdoutput.printf("  SQL_INFO_SCHEMA_VIEWS\n");
	erg=SQLGetInfo(dbc,SQL_INFO_SCHEMA_VIEWS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports 4429930.
		assertEqualDbc(dbc,(int)uintval,4429930);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_KEYSET_CURSOR_ATTRIBUTES1
	stdoutput.printf("  SQL_KEYSET_CURSOR_ATTRIBUTES1\n");
	erg=SQLGetInfo(dbc,SQL_KEYSET_CURSOR_ATTRIBUTES1,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// Both drivers report 0.
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_KEYSET_CURSOR_ATTRIBUTES2
	stdoutput.printf("  SQL_KEYSET_CURSOR_ATTRIBUTES2\n");
	erg=SQLGetInfo(dbc,SQL_KEYSET_CURSOR_ATTRIBUTES2,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// Both drivers report 0.
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_MAX_ASYNC_CONCURRENT_STATEMENTS
	stdoutput.printf("  SQL_MAX_ASYNC_CONCURRENT_STATEMENTS\n");
	erg=SQLGetInfo(dbc,SQL_MAX_ASYNC_CONCURRENT_STATEMENTS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_ODBC_INTERFACE_CONFORMANCE
	stdoutput.printf("  SQL_ODBC_INTERFACE_CONFORMANCE\n");
	erg=SQLGetInfo(dbc,SQL_ODBC_INTERFACE_CONFORMANCE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_OIC_CORE);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_PARAM_ARRAY_ROW_COUNTS
	stdoutput.printf("  SQL_PARAM_ARRAY_ROW_COUNTS\n");
	erg=SQLGetInfo(dbc,SQL_PARAM_ARRAY_ROW_COUNTS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_PARC_NO_BATCH);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_PARAM_ARRAY_SELECTS
	stdoutput.printf("  SQL_PARAM_ARRAY_SELECTS\n");
	erg=SQLGetInfo(dbc,SQL_PARAM_ARRAY_SELECTS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,(int)SQL_PAS_NO_SELECT);
	} else {
		// MariaDB reports SQL_PAS_NO_BATCH.
		assertEqualDbc(dbc,(int)uintval,(int)SQL_PAS_NO_BATCH);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL92_DATETIME_FUNCTIONS
	stdoutput.printf("  SQL_SQL92_DATETIME_FUNCTIONS\n");
	erg=SQLGetInfo(dbc,SQL_SQL92_DATETIME_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_SDF_CURRENT_DATE|
			SQL_SDF_CURRENT_TIME|
			SQL_SDF_CURRENT_TIMESTAMP));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL92_FOREIGN_KEY_DELETE_RULE
	stdoutput.printf("  SQL_SQL92_FOREIGN_KEY_DELETE_RULE\n");
	erg=SQLGetInfo(dbc,SQL_SQL92_FOREIGN_KEY_DELETE_RULE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL92_FOREIGN_KEY_UPDATE_RULE
	stdoutput.printf("  SQL_SQL92_FOREIGN_KEY_UPDATE_RULE\n");
	erg=SQLGetInfo(dbc,SQL_SQL92_FOREIGN_KEY_UPDATE_RULE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL92_GRANT
	stdoutput.printf("  SQL_SQL92_GRANT\n");
	erg=SQLGetInfo(dbc,SQL_SQL92_GRANT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// both drivers report 8176
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_SG_WITH_GRANT_OPTION|SQL_SG_DELETE_TABLE|
			SQL_SG_INSERT_TABLE|SQL_SG_INSERT_COLUMN|
			SQL_SG_REFERENCES_TABLE|SQL_SG_REFERENCES_COLUMN|
			SQL_SG_SELECT_TABLE|SQL_SG_UPDATE_TABLE|
			SQL_SG_UPDATE_COLUMN));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL92_NUMERIC_VALUE_FUNCTIONS
	stdoutput.printf("  SQL_SQL92_NUMERIC_VALUE_FUNCTIONS\n");
	erg=SQLGetInfo(dbc,SQL_SQL92_NUMERIC_VALUE_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,55);
	} else {
		// MariaDB reports 63 (all SQL_SNVF_* flags).
		assertEqualDbc(dbc,(int)uintval,63);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL92_PREDICATES
	stdoutput.printf("  SQL_SQL92_PREDICATES\n");
	erg=SQLGetInfo(dbc,SQL_SQL92_PREDICATES,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// both drivers report 15879
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_SP_EXISTS|SQL_SP_ISNOTNULL|SQL_SP_ISNULL|
			SQL_SP_LIKE|SQL_SP_IN|SQL_SP_BETWEEN|
			SQL_SP_COMPARISON|
			SQL_SP_QUANTIFIED_COMPARISON));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL92_RELATIONAL_JOIN_OPERATORS
	stdoutput.printf("  SQL_SQL92_RELATIONAL_JOIN_OPERATORS\n");
	erg=SQLGetInfo(dbc,SQL_SQL92_RELATIONAL_JOIN_OPERATORS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// both drivers report 466
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_SRJO_CROSS_JOIN|SQL_SRJO_INNER_JOIN|
			SQL_SRJO_LEFT_OUTER_JOIN|
			SQL_SRJO_NATURAL_JOIN|
			SQL_SRJO_RIGHT_OUTER_JOIN));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL92_REVOKE
	stdoutput.printf("  SQL_SQL92_REVOKE\n");
	erg=SQLGetInfo(dbc,SQL_SQL92_REVOKE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// both drivers report 32640
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_SR_DELETE_TABLE|SQL_SR_INSERT_TABLE|
			SQL_SR_INSERT_COLUMN|SQL_SR_REFERENCES_TABLE|
			SQL_SR_REFERENCES_COLUMN|SQL_SR_SELECT_TABLE|
			SQL_SR_UPDATE_TABLE|SQL_SR_UPDATE_COLUMN));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL92_ROW_VALUE_CONSTRUCTOR
	stdoutput.printf("  SQL_SQL92_ROW_VALUE_CONSTRUCTOR\n");
	erg=SQLGetInfo(dbc,SQL_SQL92_ROW_VALUE_CONSTRUCTOR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// both drivers report the same value
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_SRVC_VALUE_EXPRESSION|SQL_SRVC_NULL|
			SQL_SRVC_DEFAULT|SQL_SRVC_ROW_SUBQUERY));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL92_STRING_FUNCTIONS
	stdoutput.printf("  SQL_SQL92_STRING_FUNCTIONS\n");
	erg=SQLGetInfo(dbc,SQL_SQL92_STRING_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_SSF_LOWER|SQL_SSF_UPPER|
				SQL_SSF_SUBSTRING|SQL_SSF_TRIM_BOTH));
	} else {
		// MariaDB reports 255 (all SQL_SSF_* flags).
		assertEqualDbc(dbc,(int)uintval,255);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL92_VALUE_EXPRESSIONS
	stdoutput.printf("  SQL_SQL92_VALUE_EXPRESSIONS\n");
	erg=SQLGetInfo(dbc,SQL_SQL92_VALUE_EXPRESSIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// Both drivers report CASE|CAST|COALESCE|NULLIF.
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_SVE_CASE|SQL_SVE_CAST|
			SQL_SVE_COALESCE|SQL_SVE_NULLIF));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_STANDARD_CLI_CONFORMANCE
	stdoutput.printf("  SQL_STANDARD_CLI_CONFORMANCE\n");
	erg=SQLGetInfo(dbc,SQL_STANDARD_CLI_CONFORMANCE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,
			(int)SQL_SCC_XOPEN_CLI_VERSION1);
	} else {
		// MariaDB reports SQL_SCC_ISO92_CLI.
		assertEqualDbc(dbc,(int)uintval,(int)SQL_SCC_ISO92_CLI);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_STATIC_CURSOR_ATTRIBUTES1
	stdoutput.printf("  SQL_STATIC_CURSOR_ATTRIBUTES1\n");
	erg=SQLGetInfo(dbc,SQL_STATIC_CURSOR_ATTRIBUTES1,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports NEXT|ABSOLUTE|RELATIVE.
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_CA1_NEXT|SQL_CA1_ABSOLUTE|
				SQL_CA1_RELATIVE));
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_STATIC_CURSOR_ATTRIBUTES2
	stdoutput.printf("  SQL_STATIC_CURSOR_ATTRIBUTES2\n");
	erg=SQLGetInfo(dbc,SQL_STATIC_CURSOR_ATTRIBUTES2,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// MariaDB reports SQL_CA2_MAX_ROWS_SELECT.
		assertEqualDbc(dbc,(int)uintval,
			(int)SQL_CA2_MAX_ROWS_SELECT);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_AGGREGATE_FUNCTIONS
	stdoutput.printf("  SQL_AGGREGATE_FUNCTIONS\n");
	erg=SQLGetInfo(dbc,SQL_AGGREGATE_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertSuccessDbc(dbc,erg);
	// both drivers support the standard aggregates
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_AF_ALL|SQL_AF_AVG|SQL_AF_COUNT|
			SQL_AF_DISTINCT|SQL_AF_MAX|SQL_AF_MIN|
			SQL_AF_SUM));
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DDL_INDEX
	stdoutput.printf("  SQL_DDL_INDEX\n");
	erg=SQLGetInfo(dbc,SQL_DDL_INDEX,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_DI_CREATE_INDEX|SQL_DI_DROP_INDEX));
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DM_VER
	stdoutput.printf("  SQL_DM_VER\n");
	erg=SQLGetInfo(dbc,SQL_DM_VER,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertIsVersionDbc(dbc,(const char *)strval);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_INSERT_STATEMENT
	stdoutput.printf("  SQL_INSERT_STATEMENT\n");
	erg=SQLGetInfo(dbc,SQL_INSERT_STATEMENT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_IS_INSERT_LITERALS|
				SQL_IS_INSERT_SEARCHED|
				SQL_IS_SELECT_INTO));
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0380)
	// SQL_ASYNC_DBC_FUNCTIONS
	stdoutput.printf("  SQL_ASYNC_DBC_FUNCTIONS\n");
	erg=SQLGetInfo(dbc,SQL_ASYNC_DBC_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertSuccessDbc(dbc,erg);
	// neither driver supports async connection operations
	assertEqualDbc(dbc,(int)uintval,(int)SQL_ASYNC_DBC_NOT_CAPABLE);
	stdoutput.printf("\n");
	#endif


	// SQL_DRIVER_AWARE_POOLING_SUPPORTED
	stdoutput.printf("  SQL_DRIVER_AWARE_POOLING_SUPPORTED\n");
	erg=SQLGetInfo(dbc,SQL_DRIVER_AWARE_POOLING_SUPPORTED,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertSuccessDbc(dbc,erg);
	// neither driver supports driver-aware pooling
	assertEqualDbc(dbc,(int)uintval,
			(int)SQL_DRIVER_AWARE_POOLING_NOT_CAPABLE);
	stdoutput.printf("\n");


	#if (ODBCVER >= 0x0380)
	// SQL_ASYNC_NOTIFICATION
	stdoutput.printf("  SQL_ASYNC_NOTIFICATION\n");
	erg=SQLGetInfo(dbc,SQL_ASYNC_NOTIFICATION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertSuccessDbc(dbc,erg);
	// neither driver supports async notification
	assertEqualDbc(dbc,(int)uintval,
			(int)SQL_ASYNC_NOTIFICATION_NOT_CAPABLE);
	stdoutput.printf("\n");
	#endif


	// SQL_DTC_TRANSITION_COST
	stdoutput.printf("  SQL_DTC_TRANSITION_COST\n");
	erg=SQLGetInfo(dbc,SQL_DTC_TRANSITION_COST,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		// sqlrelay accepts the infotype but writes no value
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)vallen,0);
	} else {
		// MariaDB doesn't implement this infotype
		assertFailureDbc(dbc,erg);
	}
	stdoutput.printf("\n");



	// get functions
	stdoutput.printf("GET FUNCTIONS: \n");
	SQLUSMALLINT	supported;


	// SQL_API_SQLALLOCCONNECT
	stdoutput.printf("  SQL_API_SQLALLOCCONNECT\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLALLOCCONNECT,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLALLOCENV
	stdoutput.printf("  SQL_API_SQLALLOCENV\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLALLOCENV,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLALLOCSTMT
	stdoutput.printf("  SQL_API_SQLALLOCSTMT\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLALLOCSTMT,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLBINDCOL
	stdoutput.printf("  SQL_API_SQLBINDCOL\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLBINDCOL,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLCANCEL
	stdoutput.printf("  SQL_API_SQLCANCEL\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLCANCEL,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLCOLATTRIBUTES
	stdoutput.printf("  SQL_API_SQLCOLATTRIBUTES\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLCOLATTRIBUTES,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLCONNECT
	stdoutput.printf("  SQL_API_SQLCONNECT\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLCONNECT,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLDESCRIBECOL
	stdoutput.printf("  SQL_API_SQLDESCRIBECOL\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLDESCRIBECOL,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLDISCONNECT
	stdoutput.printf("  SQL_API_SQLDISCONNECT\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLDISCONNECT,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLERROR
	stdoutput.printf("  SQL_API_SQLERROR\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLERROR,&supported);
	assertSuccessDbc(dbc,erg);
	// Both drivers report SQL_TRUE.
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLEXECDIRECT
	stdoutput.printf("  SQL_API_SQLEXECDIRECT\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLEXECDIRECT,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLEXECUTE
	stdoutput.printf("  SQL_API_SQLEXECUTE\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLEXECUTE,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLFETCH
	stdoutput.printf("  SQL_API_SQLFETCH\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLFETCH,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLFREECONNECT
	stdoutput.printf("  SQL_API_SQLFREECONNECT\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLFREECONNECT,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLFREEENV
	stdoutput.printf("  SQL_API_SQLFREEENV\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLFREEENV,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLFREESTMT
	stdoutput.printf("  SQL_API_SQLFREESTMT\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLFREESTMT,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLGETCURSORNAME
	stdoutput.printf("  SQL_API_SQLGETCURSORNAME\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLGETCURSORNAME,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLNUMRESULTCOLS
	stdoutput.printf("  SQL_API_SQLNUMRESULTCOLS\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLNUMRESULTCOLS,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLPREPARE
	stdoutput.printf("  SQL_API_SQLPREPARE\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLPREPARE,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLROWCOUNT
	stdoutput.printf("  SQL_API_SQLROWCOUNT\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLROWCOUNT,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLSETCURSORNAME
	stdoutput.printf("  SQL_API_SQLSETCURSORNAME\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLSETCURSORNAME,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLSETPARAM
	stdoutput.printf("  SQL_API_SQLSETPARAM\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLSETPARAM,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLTRANSACT
	stdoutput.printf("  SQL_API_SQLTRANSACT\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLTRANSACT,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLCOLUMNS
	stdoutput.printf("  SQL_API_SQLCOLUMNS\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLCOLUMNS,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLDRIVERCONNECT
	stdoutput.printf("  SQL_API_SQLDRIVERCONNECT\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLDRIVERCONNECT,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLGETCONNECTOPTION
	stdoutput.printf("  SQL_API_SQLGETCONNECTOPTION\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLGETCONNECTOPTION,&supported);
	assertSuccessDbc(dbc,erg);
	// Both drivers report SQL_TRUE.
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLGETDATA
	stdoutput.printf("  SQL_API_SQLGETDATA\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLGETDATA,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLGETFUNCTIONS
	stdoutput.printf("  SQL_API_SQLGETFUNCTIONS\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLGETFUNCTIONS,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLGETINFO
	stdoutput.printf("  SQL_API_SQLGETINFO\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLGETINFO,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLGETSTMTOPTION
	stdoutput.printf("  SQL_API_SQLGETSTMTOPTION\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLGETSTMTOPTION,&supported);
	assertSuccessDbc(dbc,erg);
	// Both drivers report SQL_TRUE.
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLGETTYPEINFO
	stdoutput.printf("  SQL_API_SQLGETTYPEINFO\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLGETTYPEINFO,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLPARAMDATA
	stdoutput.printf("  SQL_API_SQLPARAMDATA\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLPARAMDATA,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLPUTDATA
	stdoutput.printf("  SQL_API_SQLPUTDATA\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLPUTDATA,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLSETCONNECTOPTION
	stdoutput.printf("  SQL_API_SQLSETCONNECTOPTION\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLSETCONNECTOPTION,&supported);
	assertSuccessDbc(dbc,erg);
	// Both drivers report SQL_TRUE.
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLSETSTMTOPTION
	stdoutput.printf("  SQL_API_SQLSETSTMTOPTION\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLSETSTMTOPTION,&supported);
	assertSuccessDbc(dbc,erg);
	// Both drivers report SQL_TRUE.
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLSPECIALCOLUMNS
	stdoutput.printf("  SQL_API_SQLSPECIALCOLUMNS\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLSPECIALCOLUMNS,&supported);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// SQL Relay ODBC driver does not yet implement this.
		assertEqualDbc(dbc,(int)supported,(int)SQL_FALSE);
	} else {
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	}
	stdoutput.printf("\n");


	// SQL_API_SQLSTATISTICS
	stdoutput.printf("  SQL_API_SQLSTATISTICS\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLSTATISTICS,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLTABLES
	stdoutput.printf("  SQL_API_SQLTABLES\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLTABLES,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLBROWSECONNECT
	stdoutput.printf("  SQL_API_SQLBROWSECONNECT\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLBROWSECONNECT,&supported);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// SQL Relay ODBC driver does not yet implement this.
		assertEqualDbc(dbc,(int)supported,(int)SQL_FALSE);
	} else {
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	}
	stdoutput.printf("\n");


	// SQL_API_SQLCOLUMNPRIVILEGES
	stdoutput.printf("  SQL_API_SQLCOLUMNPRIVILEGES\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLCOLUMNPRIVILEGES,&supported);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// SQL Relay ODBC driver does not yet implement this.
		assertEqualDbc(dbc,(int)supported,(int)SQL_FALSE);
	} else {
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	}
	stdoutput.printf("\n");


	// SQL_API_SQLDATASOURCES
	stdoutput.printf("  SQL_API_SQLDATASOURCES\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLDATASOURCES,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLDESCRIBEPARAM
	stdoutput.printf("  SQL_API_SQLDESCRIBEPARAM\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLDESCRIBEPARAM,&supported);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// SQL Relay ODBC driver does not yet implement this.
		assertEqualDbc(dbc,(int)supported,(int)SQL_FALSE);
	} else {
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	}
	stdoutput.printf("\n");


	// SQL_API_SQLEXTENDEDFETCH
	stdoutput.printf("  SQL_API_SQLEXTENDEDFETCH\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLEXTENDEDFETCH,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLFOREIGNKEYS
	stdoutput.printf("  SQL_API_SQLFOREIGNKEYS\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLFOREIGNKEYS,&supported);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// SQL Relay ODBC driver does not yet implement this.
		assertEqualDbc(dbc,(int)supported,(int)SQL_FALSE);
	} else {
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	}
	stdoutput.printf("\n");


	// SQL_API_SQLMORERESULTS
	stdoutput.printf("  SQL_API_SQLMORERESULTS\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLMORERESULTS,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLNATIVESQL
	stdoutput.printf("  SQL_API_SQLNATIVESQL\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLNATIVESQL,&supported);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// SQL Relay ODBC driver does not yet implement this.
		assertEqualDbc(dbc,(int)supported,(int)SQL_FALSE);
	} else {
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	}
	stdoutput.printf("\n");


	// SQL_API_SQLNUMPARAMS
	stdoutput.printf("  SQL_API_SQLNUMPARAMS\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLNUMPARAMS,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLPARAMOPTIONS
	stdoutput.printf("  SQL_API_SQLPARAMOPTIONS\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLPARAMOPTIONS,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLPRIMARYKEYS
	stdoutput.printf("  SQL_API_SQLPRIMARYKEYS\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLPRIMARYKEYS,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLPROCEDURECOLUMNS
	stdoutput.printf("  SQL_API_SQLPROCEDURECOLUMNS\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLPROCEDURECOLUMNS,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLPROCEDURES
	stdoutput.printf("  SQL_API_SQLPROCEDURES\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLPROCEDURES,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLSETPOS
	stdoutput.printf("  SQL_API_SQLSETPOS\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLSETPOS,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLSETSCROLLOPTIONS
	stdoutput.printf("  SQL_API_SQLSETSCROLLOPTIONS\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLSETSCROLLOPTIONS,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLTABLEPRIVILEGES
	stdoutput.printf("  SQL_API_SQLTABLEPRIVILEGES\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLTABLEPRIVILEGES,&supported);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// SQL Relay ODBC driver does not yet implement this.
		assertEqualDbc(dbc,(int)supported,(int)SQL_FALSE);
	} else {
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	}
	stdoutput.printf("\n");


	// SQL_API_SQLDRIVERS
	stdoutput.printf("  SQL_API_SQLDRIVERS\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLDRIVERS,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLBINDPARAMETER
	stdoutput.printf("  SQL_API_SQLBINDPARAMETER\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLBINDPARAMETER,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	#if (ODBCVER >= 0x0300)
	// SQL_API_SQLALLOCHANDLE
	stdoutput.printf("  SQL_API_SQLALLOCHANDLE\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLALLOCHANDLE,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLBINDPARAM
	stdoutput.printf("  SQL_API_SQLBINDPARAM\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLBINDPARAM,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLCLOSECURSOR
	stdoutput.printf("  SQL_API_SQLCLOSECURSOR\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLCLOSECURSOR,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLCOLATTRIBUTE
	stdoutput.printf("  SQL_API_SQLCOLATTRIBUTE\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLCOLATTRIBUTE,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLCOPYDESC
	stdoutput.printf("  SQL_API_SQLCOPYDESC\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLCOPYDESC,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLENDTRAN
	stdoutput.printf("  SQL_API_SQLENDTRAN\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLENDTRAN,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLFETCHSCROLL
	stdoutput.printf("  SQL_API_SQLFETCHSCROLL\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLFETCHSCROLL,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLFREEHANDLE
	stdoutput.printf("  SQL_API_SQLFREEHANDLE\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLFREEHANDLE,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLGETCONNECTATTR
	stdoutput.printf("  SQL_API_SQLGETCONNECTATTR\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLGETCONNECTATTR,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLGETDESCFIELD
	stdoutput.printf("  SQL_API_SQLGETDESCFIELD\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLGETDESCFIELD,&supported);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// SQL Relay doesn't yet implement descriptor access
		assertEqualDbc(dbc,(int)supported,(int)SQL_FALSE);
	} else {
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	}
	stdoutput.printf("\n");


	// SQL_API_SQLGETDESCREC
	stdoutput.printf("  SQL_API_SQLGETDESCREC\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLGETDESCREC,&supported);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// SQL Relay doesn't yet implement descriptor access
		assertEqualDbc(dbc,(int)supported,(int)SQL_FALSE);
	} else {
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	}
	stdoutput.printf("\n");


	// SQL_API_SQLGETDIAGFIELD
	stdoutput.printf("  SQL_API_SQLGETDIAGFIELD\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLGETDIAGFIELD,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLGETDIAGREC
	stdoutput.printf("  SQL_API_SQLGETDIAGREC\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLGETDIAGREC,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLGETENVATTR
	stdoutput.printf("  SQL_API_SQLGETENVATTR\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLGETENVATTR,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLGETSTMTATTR
	stdoutput.printf("  SQL_API_SQLGETSTMTATTR\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLGETSTMTATTR,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLSETCONNECTATTR
	stdoutput.printf("  SQL_API_SQLSETCONNECTATTR\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLSETCONNECTATTR,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLSETDESCFIELD
	stdoutput.printf("  SQL_API_SQLSETDESCFIELD\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLSETDESCFIELD,&supported);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// SQL Relay doesn't yet implement descriptor access
		assertEqualDbc(dbc,(int)supported,(int)SQL_FALSE);
	} else {
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	}
	stdoutput.printf("\n");


	// SQL_API_SQLSETDESCREC
	stdoutput.printf("  SQL_API_SQLSETDESCREC\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLSETDESCREC,&supported);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// SQL Relay doesn't yet implement descriptor access
		assertEqualDbc(dbc,(int)supported,(int)SQL_FALSE);
	} else {
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	}
	stdoutput.printf("\n");


	// SQL_API_SQLSETENVATTR
	stdoutput.printf("  SQL_API_SQLSETENVATTR\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLSETENVATTR,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLSETSTMTATTR
	stdoutput.printf("  SQL_API_SQLSETSTMTATTR\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLSETSTMTATTR,&supported);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLBULKOPERATIONS
	stdoutput.printf("  SQL_API_SQLBULKOPERATIONS\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLBULKOPERATIONS,&supported);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// SQL Relay doesn't yet implement bulk operations
		assertEqualDbc(dbc,(int)supported,(int)SQL_FALSE);
	} else {
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	}
	stdoutput.printf("\n");
	#endif


	// SQL_API_ALL_FUNCTIONS (ODBC 2.x 100-element array form)
	stdoutput.printf("  SQL_API_ALL_FUNCTIONS\n");
	SQLUSMALLINT	allfuncs[100];
	for (SQLUSMALLINT i=0; i<100; i++) {
		allfuncs[i]=0xff;
	}
	erg=SQLGetFunctions(dbc,SQL_API_ALL_FUNCTIONS,allfuncs);
	assertSuccessDbc(dbc,erg);
	// spot-check: SQLAllocConnect=1, SQLConnect=7, SQLFetch=13,
	// SQLFreeStmt=16, SQLDisconnect=9
	assertEqualDbc(dbc,(int)allfuncs[SQL_API_SQLALLOCCONNECT],
							(int)SQL_TRUE);
	assertEqualDbc(dbc,(int)allfuncs[SQL_API_SQLCONNECT],(int)SQL_TRUE);
	assertEqualDbc(dbc,(int)allfuncs[SQL_API_SQLFETCH],(int)SQL_TRUE);
	assertEqualDbc(dbc,(int)allfuncs[SQL_API_SQLFREESTMT],(int)SQL_TRUE);
	assertEqualDbc(dbc,(int)allfuncs[SQL_API_SQLDISCONNECT],(int)SQL_TRUE);
	stdoutput.printf("\n");

	#if (ODBCVER >= 0x0300)
	// SQL_API_ODBC3_ALL_FUNCTIONS (ODBC 3.x bitmap form)
	stdoutput.printf("  SQL_API_ODBC3_ALL_FUNCTIONS\n");
	SQLUSMALLINT	funcs3[SQL_API_ODBC3_ALL_FUNCTIONS_SIZE];
	for (SQLUSMALLINT i=0; i<SQL_API_ODBC3_ALL_FUNCTIONS_SIZE; i++) {
		funcs3[i]=0;
	}
	erg=SQLGetFunctions(dbc,SQL_API_ODBC3_ALL_FUNCTIONS,funcs3);
	assertSuccessDbc(dbc,erg);
	// spot-check ODBC 3.x API IDs via SQL_FUNC_EXISTS bitmap
	assertEqualDbc(dbc,
		(int)SQL_FUNC_EXISTS(funcs3,SQL_API_SQLALLOCHANDLE),
		(int)SQL_TRUE);
	assertEqualDbc(dbc,
		(int)SQL_FUNC_EXISTS(funcs3,SQL_API_SQLFREEHANDLE),
		(int)SQL_TRUE);
	assertEqualDbc(dbc,
		(int)SQL_FUNC_EXISTS(funcs3,SQL_API_SQLFETCHSCROLL),
		(int)SQL_TRUE);
	assertEqualDbc(dbc,
		(int)SQL_FUNC_EXISTS(funcs3,SQL_API_SQLENDTRAN),
		(int)SQL_TRUE);
	// spot-check ODBC 2.x IDs are still in the ODBC3 bitmap
	assertEqualDbc(dbc,
		(int)SQL_FUNC_EXISTS(funcs3,SQL_API_SQLCONNECT),
		(int)SQL_TRUE);
	assertEqualDbc(dbc,
		(int)SQL_FUNC_EXISTS(funcs3,SQL_API_SQLFETCH),
		(int)SQL_TRUE);
	stdoutput.printf("\n");
	#endif



	// isolation levels
	stdoutput.printf("ISOLATION LEVELS: \n");

	// MySQL/InnoDB supports all four isolation levels; each set succeeds
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_READ_UNCOMMITTED,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_TXN_READ_UNCOMMITTED);

	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_READ_COMMITTED,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_TXN_READ_COMMITTED);

	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_REPEATABLE_READ,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_TXN_REPEATABLE_READ);

	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_SERIALIZABLE,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_TXN_SERIALIZABLE);

	// reset to MySQL's default isolation level (REPEATABLE READ)
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_REPEATABLE_READ,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_TXN_REPEATABLE_READ);
	stdoutput.printf("\n");



	// statement handle
	stdoutput.printf("STATEMENT HANDLE: \n");
	erg=SQLAllocHandle(SQL_HANDLE_STMT,dbc,&stmt);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");



	// statement attributes
	stdoutput.printf("STATEMENT ATTRIBUTES: \n");
	SQLULEN		stmtulenval;
	SQLULEN		stmtinitial;
	SQLINTEGER	stmtstrlen;
	SQLPOINTER	stmtptrval;
	SQLPOINTER	stmtptrinit;
	SQLUSMALLINT	stmtrowstatus[4]={0,0,0,0};
	SQLULEN		stmtrowsfetched=0;
	SQLULEN		stmtparamsprocessed=0;
	SQLULEN		stmtparambindoffset=0;
	SQLULEN		stmtrowbindoffset=0;
	SQLUSMALLINT	stmtparamstatus[4]={0,0,0,0};
	SQLUSMALLINT	stmtparamop[4]={0,0,0,0};
	SQLUSMALLINT	stmtrowop[4]={0,0,0,0};
	SQLLEN		stmtbookmark=0;


	#if (ODBCVER >= 0x0300)
	// SQL_ATTR_APP_ROW_DESC (descriptor handle, settable)
	stdoutput.printf("  SQL_ATTR_APP_ROW_DESC\n");
	// get initial (implicit app row descriptor)
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_APP_ROW_DESC,
			(SQLPOINTER)&stmtptrinit,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	// round-trip the same handle
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_APP_ROW_DESC,
			(SQLPOINTER)stmtptrinit,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_APP_ROW_DESC,
			(SQLPOINTER)&stmtptrval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)(stmtptrval==stmtptrinit),1);
	// SQL_NULL_DESC resets to the implicit descriptor
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_APP_ROW_DESC,
			(SQLPOINTER)SQL_NULL_DESC,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_APP_PARAM_DESC (descriptor handle, settable)
	stdoutput.printf("  SQL_ATTR_APP_PARAM_DESC\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_APP_PARAM_DESC,
			(SQLPOINTER)&stmtptrinit,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_APP_PARAM_DESC,
			(SQLPOINTER)stmtptrinit,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_APP_PARAM_DESC,
			(SQLPOINTER)&stmtptrval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)(stmtptrval==stmtptrinit),1);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_APP_PARAM_DESC,
		(SQLPOINTER)SQL_NULL_DESC,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_IMP_ROW_DESC (read-only)
	stdoutput.printf("  SQL_ATTR_IMP_ROW_DESC\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_IMP_ROW_DESC,
			(SQLPOINTER)&stmtptrval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	// setting should fail (HY017 automatically allocated descriptor)
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_IMP_ROW_DESC,
			(SQLPOINTER)stmtptrval,SQL_IS_POINTER);
	assertFailureStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_IMP_PARAM_DESC (read-only)
	stdoutput.printf("  SQL_ATTR_IMP_PARAM_DESC\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_IMP_PARAM_DESC,
			(SQLPOINTER)&stmtptrval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	// setting should fail (HY017 automatically allocated descriptor)
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_IMP_PARAM_DESC,
			(SQLPOINTER)stmtptrval,SQL_IS_POINTER);
	assertFailureStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_CURSOR_SCROLLABLE
	stdoutput.printf("  SQL_ATTR_CURSOR_SCROLLABLE\n");
	// save initial value
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_SCROLLABLE,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_NONSCROLLABLE);
	// SQL_NONSCROLLABLE
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_SCROLLABLE,
			(SQLPOINTER)(uintptr_t)SQL_NONSCROLLABLE,0);
	assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_SCROLLABLE,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_NONSCROLLABLE);
	// SQL_SCROLLABLE
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_SCROLLABLE,
			(SQLPOINTER)(uintptr_t)SQL_SCROLLABLE,0);
	if (issqlrelay) {
		// SQL Relay only supports SQL_NONSCROLLABLE; substitutes
		// it and returns SQL_SUCCESS_WITH_INFO, SQLSTATE 01S02
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_SCROLLABLE,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		// get reflects the substituted value, not what the app set
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_NONSCROLLABLE);
	} else {
		// MariaDB promotes to SQL_CURSOR_STATIC (3) for
		// SQL_SCROLLABLE and reports that, not SQL_SCROLLABLE (1)
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_CURSOR_STATIC);
	}
	// restore initial value
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_SCROLLABLE,
			(SQLPOINTER)(uintptr_t)stmtinitial,0);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_CURSOR_SENSITIVITY
	stdoutput.printf("  SQL_ATTR_CURSOR_SENSITIVITY\n");
	// save initial value
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_INSENSITIVE);
	} else {
		// MariaDB defaults to SQL_UNSPECIFIED.
		assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_UNSPECIFIED);
	}
	// SQL_UNSPECIFIED
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
			(SQLPOINTER)(uintptr_t)SQL_UNSPECIFIED,0);
	assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		// SQL Relay always reports SQL_INSENSITIVE.
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_INSENSITIVE);
	} else {
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_UNSPECIFIED);
	}
	// SQL_INSENSITIVE
	// SQL Relay only supports SQL_INSENSITIVE, MariaDB only
	// SQL_UNSPECIFIED; substitutes return SQL_SUCCESS_WITH_INFO +
	// SQLSTATE 01S02, a matching value returns SQL_SUCCESS
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
			(SQLPOINTER)(uintptr_t)SQL_INSENSITIVE,0);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS);
	} else {
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	}
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_INSENSITIVE);
	} else {
		// MariaDB substitutes back to SQL_UNSPECIFIED.
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_UNSPECIFIED);
	}
	// SQL_SENSITIVE
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
			(SQLPOINTER)(uintptr_t)SQL_SENSITIVE,0);
	// both substitute and return SQL_SUCCESS_WITH_INFO + 01S02
	assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		// get reflects the substituted value, not what the app set
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_INSENSITIVE);
	} else {
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_UNSPECIFIED);
	}
	// restore initial value
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
			(SQLPOINTER)(uintptr_t)stmtinitial,0);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");
	#endif


	// SQL_ATTR_CURSOR_TYPE
	// sqlrelay only supports SQL_CURSOR_FORWARD_ONLY; other values
	// substituted with SQL_SUCCESS_WITH_INFO + SQLSTATE 01S02
	// (assertSuccessStmt accepts both SUCCESS and SUCCESS_WITH_INFO)
	stdoutput.printf("  SQL_ATTR_CURSOR_TYPE\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_TYPE,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_CURSOR_FORWARD_ONLY);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_TYPE,
			(SQLPOINTER)(uintptr_t)SQL_CURSOR_STATIC,0);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_TYPE,
			(SQLPOINTER)(uintptr_t)SQL_CURSOR_DYNAMIC,0);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_TYPE,
			(SQLPOINTER)(uintptr_t)SQL_CURSOR_KEYSET_DRIVEN,0);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_TYPE,
			(SQLPOINTER)(uintptr_t)SQL_CURSOR_FORWARD_ONLY,0);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_TYPE,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_CURSOR_FORWARD_ONLY);
	stdoutput.printf("\n");


	// SQL_ATTR_CONCURRENCY
	// sqlrelay only supports SQL_CONCUR_READ_ONLY; other values are
	// substituted with SQL_SUCCESS_WITH_INFO + SQLSTATE 01S02
	stdoutput.printf("  SQL_ATTR_CONCURRENCY\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_CONCURRENCY,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_CONCUR_READ_ONLY);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_CONCURRENCY,
			(SQLPOINTER)(uintptr_t)SQL_CONCUR_LOCK,0);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_CONCURRENCY,
			(SQLPOINTER)(uintptr_t)SQL_CONCUR_ROWVER,0);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_CONCURRENCY,
			(SQLPOINTER)(uintptr_t)SQL_CONCUR_VALUES,0);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_CONCURRENCY,
			(SQLPOINTER)(uintptr_t)SQL_CONCUR_READ_ONLY,0);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_CONCURRENCY,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_CONCUR_READ_ONLY);
	stdoutput.printf("\n");


	// SQL_ATTR_QUERY_TIMEOUT
	stdoutput.printf("  SQL_ATTR_QUERY_TIMEOUT\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_QUERY_TIMEOUT,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtinitial,0);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_QUERY_TIMEOUT,
			(SQLPOINTER)(uintptr_t)30,0);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_QUERY_TIMEOUT,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)stmtulenval,30);
	} else {
		// MariaDB always returns 0 regardless of what was set.
		assertEqualStmt(stmt,(int)stmtulenval,0);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_QUERY_TIMEOUT,
			(SQLPOINTER)(uintptr_t)stmtinitial,0);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_MAX_ROWS
	stdoutput.printf("  SQL_ATTR_MAX_ROWS\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_MAX_ROWS,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtinitial,0);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_MAX_ROWS,
			(SQLPOINTER)(uintptr_t)100,0);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_MAX_ROWS,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,100);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_MAX_ROWS,
			(SQLPOINTER)(uintptr_t)stmtinitial,0);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_NOSCAN
	stdoutput.printf("  SQL_ATTR_NOSCAN\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_NOSCAN,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_NOSCAN_OFF);
	} else {
		// MariaDB defaults to SQL_NOSCAN_ON.
		assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_NOSCAN_ON);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_NOSCAN,
			(SQLPOINTER)(uintptr_t)SQL_NOSCAN_ON,0);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_NOSCAN,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_NOSCAN_ON);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_NOSCAN,
			(SQLPOINTER)(uintptr_t)SQL_NOSCAN_OFF,0);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_NOSCAN,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_NOSCAN_OFF);
	} else {
		// MariaDB does not actually flip the flag back to OFF.
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_NOSCAN_ON);
	}
	stdoutput.printf("\n");


	// SQL_ATTR_MAX_LENGTH
	stdoutput.printf("  SQL_ATTR_MAX_LENGTH\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_MAX_LENGTH,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtinitial,0);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_MAX_LENGTH,
			(SQLPOINTER)(uintptr_t)4096,0);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_MAX_LENGTH,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,4096);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_MAX_LENGTH,
			(SQLPOINTER)(uintptr_t)stmtinitial,0);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	#if (ODBCVER >= 0x0300)
	// SQL_ATTR_ASYNC_ENABLE
	stdoutput.printf("  SQL_ATTR_ASYNC_ENABLE\n");
	// save initial value
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ASYNC_ENABLE,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_ASYNC_ENABLE_OFF);
	// SQL_ASYNC_ENABLE_OFF
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ASYNC_ENABLE,
			(SQLPOINTER)(uintptr_t)SQL_ASYNC_ENABLE_OFF,0);
	assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ASYNC_ENABLE,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_ASYNC_ENABLE_OFF);
	// SQL_ASYNC_ENABLE_ON (not supported by sqlrelay or mysql odbc)
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ASYNC_ENABLE,
			(SQLPOINTER)(uintptr_t)SQL_ASYNC_ENABLE_ON,0);
	// Both drivers substitute with SQL_ASYNC_ENABLE_OFF and
	// return SQL_SUCCESS_WITH_INFO, SQLSTATE 01S02
	// ("Option value changed").
	assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ASYNC_ENABLE,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	// get reflects the substituted value, not what the app set
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_ASYNC_ENABLE_OFF);
	// restore initial value
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ASYNC_ENABLE,
			(SQLPOINTER)(uintptr_t)stmtinitial,0);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");
	#endif


	// SQL_ATTR_ROW_BIND_TYPE (SQL_BIND_BY_COLUMN == 0, or row length)
	stdoutput.printf("  SQL_ATTR_ROW_BIND_TYPE\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ROW_BIND_TYPE,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_BIND_BY_COLUMN);
	// row-wise binding: any non-zero row length is spec-valid
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ROW_BIND_TYPE,
			(SQLPOINTER)(uintptr_t)64,0);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ROW_BIND_TYPE,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,64);
	// back to column-wise
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ROW_BIND_TYPE,
			(SQLPOINTER)(uintptr_t)SQL_BIND_BY_COLUMN,0);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ROW_BIND_TYPE,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_BIND_BY_COLUMN);
	stdoutput.printf("\n");


	// SQL_ATTR_KEYSET_SIZE
	stdoutput.printf("  SQL_ATTR_KEYSET_SIZE\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_KEYSET_SIZE,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtinitial,0);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_KEYSET_SIZE,
			(SQLPOINTER)(uintptr_t)10,0);
	if (issqlrelay) {
		assertSuccessStmt(stmt,erg);
	} else {
		// MariaDB rejects nonzero values with HY024.
		assertFailureStmt(stmt,erg);
	}
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_KEYSET_SIZE,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)stmtulenval,10);
	} else {
		// the rejected set leaves the value unchanged
		assertEqualStmt(stmt,(int)stmtulenval,0);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_KEYSET_SIZE,
			(SQLPOINTER)(uintptr_t)stmtinitial,0);
	if (issqlrelay) {
		assertSuccessStmt(stmt,erg);
	} else {
		// MariaDB also rejects 0.
		assertFailureStmt(stmt,erg);
	}
	stdoutput.printf("\n");


	#if (ODBCVER >= 0x0300)
	// SQL_ATTR_ROW_ARRAY_SIZE
	stdoutput.printf("  SQL_ATTR_ROW_ARRAY_SIZE\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ROW_ARRAY_SIZE,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtinitial,1);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ROW_ARRAY_SIZE,
			(SQLPOINTER)(uintptr_t)10,0);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ROW_ARRAY_SIZE,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,10);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ROW_ARRAY_SIZE,
			(SQLPOINTER)(uintptr_t)stmtinitial,0);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");
	#endif


	// SQL_ATTR_SIMULATE_CURSOR
	stdoutput.printf("  SQL_ATTR_SIMULATE_CURSOR\n");
	// save initial value
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_SC_NON_UNIQUE);
	// SQL_SC_NON_UNIQUE
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
			(SQLPOINTER)(uintptr_t)SQL_SC_NON_UNIQUE,0);
	assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_SC_NON_UNIQUE);
	// SQL_SC_TRY_UNIQUE (sqlrelay has no positioned updates,
	// can't guarantee uniqueness)
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
			(SQLPOINTER)(uintptr_t)SQL_SC_TRY_UNIQUE,0);
	if (issqlrelay) {
		// substitutes SQL_SC_NON_UNIQUE; SQL_SUCCESS_WITH_INFO,
		// SQLSTATE 01S02 ("Option value changed")
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		// get reflects the substituted value, not what the app set
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_SC_NON_UNIQUE);
	} else {
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_SC_TRY_UNIQUE);
	}
	// SQL_SC_UNIQUE
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
			(SQLPOINTER)(uintptr_t)SQL_SC_UNIQUE,0);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_SC_NON_UNIQUE);
	} else {
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_SC_UNIQUE);
	}
	// restore initial value
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
			(SQLPOINTER)(uintptr_t)stmtinitial,0);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_RETRIEVE_DATA
	stdoutput.printf("  SQL_ATTR_RETRIEVE_DATA\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_RETRIEVE_DATA,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_RD_ON);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_RETRIEVE_DATA,
			(SQLPOINTER)(uintptr_t)SQL_RD_OFF,0);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_RETRIEVE_DATA,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_RD_OFF);
	} else {
		// MariaDB always returns SQL_RD_ON regardless of set
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_RD_ON);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_RETRIEVE_DATA,
			(SQLPOINTER)(uintptr_t)SQL_RD_ON,0);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_RETRIEVE_DATA,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_RD_ON);
	stdoutput.printf("\n");


	// SQL_ATTR_USE_BOOKMARKS
	// neither driver supports bookmarks; SQL_UB_VARIABLE is
	// substituted with SQL_UB_OFF
	stdoutput.printf("  SQL_ATTR_USE_BOOKMARKS\n");
	// save initial value
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_USE_BOOKMARKS,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_UB_OFF);
	// SQL_UB_OFF
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_USE_BOOKMARKS,
			(SQLPOINTER)(uintptr_t)SQL_UB_OFF,0);
	assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_USE_BOOKMARKS,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_UB_OFF);
	// SQL_UB_VARIABLE
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_USE_BOOKMARKS,
			(SQLPOINTER)(uintptr_t)SQL_UB_VARIABLE,0);
	if (issqlrelay) {
		// substitutes SQL_UB_OFF; SQL_SUCCESS_WITH_INFO,
		// SQLSTATE 01S02 ("Option value changed")
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	} else {
		// substitutes with SQL_UB_OFF, returns SQL_SUCCESS
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_USE_BOOKMARKS,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	// get reflects the substituted value, not what the app set
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_UB_OFF);
	// restore initial value
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_USE_BOOKMARKS,
			(SQLPOINTER)(uintptr_t)stmtinitial,0);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_ROW_NUMBER (read-only)
	stdoutput.printf("  SQL_ATTR_ROW_NUMBER\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ROW_NUMBER,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	// no cursor is open; the unixodbc driver manager itself
	// raises 24000 before the call reaches either driver
	assertFailureStmt(stmt,erg);
	// setting should fail (read-only)
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ROW_NUMBER,
			(SQLPOINTER)(uintptr_t)1,0);
	assertFailureStmt(stmt,erg);
	stdoutput.printf("\n");


	#if (ODBCVER >= 0x0300)
	// SQL_ATTR_METADATA_ID (inherits from connection, default SQL_FALSE)
	// sqlrelay catalog functions always pattern-match: SQL_TRUE
	// substituted with SQL_FALSE, SQL_SUCCESS_WITH_INFO + SQLSTATE 01S02
	stdoutput.printf("  SQL_ATTR_METADATA_ID\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_FALSE);
	} else {
		// MariaDB defaults to SQL_TRUE despite the spec default
		assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_TRUE);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)(uintptr_t)SQL_TRUE,0);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		// get reflects the substituted value, not what the app set
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_FALSE);
	} else {
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_TRUE);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)(uintptr_t)SQL_FALSE,0);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_FALSE);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)(uintptr_t)stmtinitial,0);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_ENABLE_AUTO_IPD
	// sqlrelay round-trips the value (auto-IPD not gated on it);
	// MariaDB accepts get but rejects every set with HY092
	stdoutput.printf("  SQL_ATTR_ENABLE_AUTO_IPD\n");
	// save initial value
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_TRUE);
	} else {
		// MariaDB defaults to SQL_FALSE.
		assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_FALSE);
	}
	// SQL_TRUE
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
			(SQLPOINTER)(uintptr_t)SQL_TRUE,0);
	if (issqlrelay) {
		assertSuccessStmt(stmt,erg);
	} else {
		assertFailureStmt(stmt,erg);
	}
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_TRUE);
	} else {
		// MariaDB's set was rejected, so the value never changed.
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_FALSE);
	}
	// SQL_FALSE
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
			(SQLPOINTER)(uintptr_t)SQL_FALSE,0);
	if (issqlrelay) {
		assertSuccessStmt(stmt,erg);
	} else {
		assertFailureStmt(stmt,erg);
	}
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_FALSE);
	// restore initial value
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
			(SQLPOINTER)(uintptr_t)stmtinitial,0);
	if (issqlrelay) {
		assertSuccessStmt(stmt,erg);
	} else {
		assertFailureStmt(stmt,erg);
	}
	stdoutput.printf("\n");


	// SQL_ATTR_PARAMSET_SIZE
	// sqlrelay has no parameter arrays; values other than 1
	// substituted with 1, SQL_SUCCESS_WITH_INFO + SQLSTATE 01S02
	stdoutput.printf("  SQL_ATTR_PARAMSET_SIZE\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_PARAMSET_SIZE,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtinitial,1);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_PARAMSET_SIZE,
			(SQLPOINTER)(uintptr_t)10,0);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_PARAMSET_SIZE,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		// get reflects the substituted value, not what the app set
		assertEqualStmt(stmt,(int)stmtulenval,1);
	} else {
		assertEqualStmt(stmt,(int)stmtulenval,10);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_PARAMSET_SIZE,
			(SQLPOINTER)(uintptr_t)stmtinitial,0);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_PARAM_BIND_TYPE (0 or row length)
	// sqlrelay has no parameter arrays; row-wise (non-zero) substituted
	// with SQL_PARAM_BIND_BY_COLUMN, SQL_SUCCESS_WITH_INFO + SQLSTATE 01S02
	stdoutput.printf("  SQL_ATTR_PARAM_BIND_TYPE\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_PARAM_BIND_TYPE,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtinitial,
				(int)SQL_PARAM_BIND_BY_COLUMN);
	// row-wise: spec allows any non-zero row length
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_PARAM_BIND_TYPE,
			(SQLPOINTER)(uintptr_t)32,0);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_PARAM_BIND_TYPE,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		// get reflects the substituted value, not what the app set
		assertEqualStmt(stmt,(int)stmtulenval,
					(int)SQL_PARAM_BIND_BY_COLUMN);
	} else {
		assertEqualStmt(stmt,(int)stmtulenval,32);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_PARAM_BIND_TYPE,
			(SQLPOINTER)(uintptr_t)SQL_PARAM_BIND_BY_COLUMN,0);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_PARAM_BIND_TYPE,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,
				(int)SQL_PARAM_BIND_BY_COLUMN);
	stdoutput.printf("\n");


	// SQL_ATTR_PARAM_BIND_OFFSET_PTR (pointer)
	stdoutput.printf("  SQL_ATTR_PARAM_BIND_OFFSET_PTR\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_PARAM_BIND_OFFSET_PTR,
			(SQLPOINTER)&stmtptrinit,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_PARAM_BIND_OFFSET_PTR,
			(SQLPOINTER)&stmtparambindoffset,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_PARAM_BIND_OFFSET_PTR,
			(SQLPOINTER)&stmtptrval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,
		(int)(stmtptrval==(SQLPOINTER)&stmtparambindoffset),1);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_PARAM_BIND_OFFSET_PTR,
			(SQLPOINTER)stmtptrinit,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_PARAM_OPERATION_PTR (pointer)
	stdoutput.printf("  SQL_ATTR_PARAM_OPERATION_PTR\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_PARAM_OPERATION_PTR,
			(SQLPOINTER)&stmtptrinit,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_PARAM_OPERATION_PTR,
			(SQLPOINTER)stmtparamop,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_PARAM_OPERATION_PTR,
			(SQLPOINTER)&stmtptrval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,
			(int)(stmtptrval==(SQLPOINTER)stmtparamop),1);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_PARAM_OPERATION_PTR,
			(SQLPOINTER)stmtptrinit,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_PARAM_STATUS_PTR (pointer)
	stdoutput.printf("  SQL_ATTR_PARAM_STATUS_PTR\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_PARAM_STATUS_PTR,
			(SQLPOINTER)&stmtptrinit,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_PARAM_STATUS_PTR,
			(SQLPOINTER)stmtparamstatus,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_PARAM_STATUS_PTR,
			(SQLPOINTER)&stmtptrval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,
		(int)(stmtptrval==(SQLPOINTER)stmtparamstatus),1);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_PARAM_STATUS_PTR,
			(SQLPOINTER)stmtptrinit,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_PARAMS_PROCESSED_PTR (pointer)
	stdoutput.printf("  SQL_ATTR_PARAMS_PROCESSED_PTR\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_PARAMS_PROCESSED_PTR,
			(SQLPOINTER)&stmtptrinit,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_PARAMS_PROCESSED_PTR,
			(SQLPOINTER)&stmtparamsprocessed,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_PARAMS_PROCESSED_PTR,
			(SQLPOINTER)&stmtptrval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		assertEqualStmt(stmt,
			(int)(stmtptrval==(SQLPOINTER)&stmtparamsprocessed),
			1);
	} else {
		// MariaDB discards the pointer.
		assertEqualStmt(stmt,
			(int)(stmtptrval==(SQLPOINTER)&stmtparamsprocessed),
			0);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_PARAMS_PROCESSED_PTR,
			(SQLPOINTER)stmtptrinit,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_ROW_BIND_OFFSET_PTR (pointer)
	stdoutput.printf("  SQL_ATTR_ROW_BIND_OFFSET_PTR\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ROW_BIND_OFFSET_PTR,
			(SQLPOINTER)&stmtptrinit,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ROW_BIND_OFFSET_PTR,
			(SQLPOINTER)&stmtrowbindoffset,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ROW_BIND_OFFSET_PTR,
			(SQLPOINTER)&stmtptrval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,
		(int)(stmtptrval==(SQLPOINTER)&stmtrowbindoffset),1);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ROW_BIND_OFFSET_PTR,
			(SQLPOINTER)stmtptrinit,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_ROW_OPERATION_PTR (pointer)
	stdoutput.printf("  SQL_ATTR_ROW_OPERATION_PTR\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ROW_OPERATION_PTR,
			(SQLPOINTER)&stmtptrinit,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ROW_OPERATION_PTR,
			(SQLPOINTER)stmtrowop,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ROW_OPERATION_PTR,
			(SQLPOINTER)&stmtptrval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,
			(int)(stmtptrval==(SQLPOINTER)stmtrowop),1);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ROW_OPERATION_PTR,
			(SQLPOINTER)stmtptrinit,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_ROW_STATUS_PTR (pointer)
	stdoutput.printf("  SQL_ATTR_ROW_STATUS_PTR\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ROW_STATUS_PTR,
			(SQLPOINTER)&stmtptrinit,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ROW_STATUS_PTR,
			(SQLPOINTER)stmtrowstatus,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ROW_STATUS_PTR,
			(SQLPOINTER)&stmtptrval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)(stmtptrval==(SQLPOINTER)stmtrowstatus),1);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ROW_STATUS_PTR,
			(SQLPOINTER)stmtptrinit,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_ROWS_FETCHED_PTR (pointer)
	stdoutput.printf("  SQL_ATTR_ROWS_FETCHED_PTR\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ROWS_FETCHED_PTR,
			(SQLPOINTER)&stmtptrinit,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ROWS_FETCHED_PTR,
			(SQLPOINTER)&stmtrowsfetched,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_ROWS_FETCHED_PTR,
			(SQLPOINTER)&stmtptrval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,
			(int)(stmtptrval==(SQLPOINTER)&stmtrowsfetched),1);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_ROWS_FETCHED_PTR,
			(SQLPOINTER)stmtptrinit,SQL_IS_POINTER);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// SQL_ATTR_FETCH_BOOKMARK_PTR (pointer)
	stdoutput.printf("  SQL_ATTR_FETCH_BOOKMARK_PTR\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_FETCH_BOOKMARK_PTR,
			(SQLPOINTER)&stmtptrinit,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		// SQL Relay has no bookmarks; no bookmark pointer in effect
		assertEqualStmt(stmt,(int)(stmtptrinit==NULL),1);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_FETCH_BOOKMARK_PTR,
			(SQLPOINTER)&stmtbookmark,SQL_IS_POINTER);
	// installing a non-NULL bookmark pointer is rejected
	// with HYC00 ("Optional feature not implemented")
	assertFailureStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_FETCH_BOOKMARK_PTR,
			(SQLPOINTER)&stmtptrval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		// The rejected set didn't take, so still NULL.
		assertEqualStmt(stmt,(int)(stmtptrval==NULL),1);
	} else {
		// MariaDB reports a non-NULL internal pointer.
		assertEqualStmt(stmt,(int)(stmtptrval!=NULL),1);
	}
	// restore the original (NULL on both; non-NULL rejected with
	// HYC00 by either, harmless here)
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_FETCH_BOOKMARK_PTR,
			(SQLPOINTER)stmtptrinit,SQL_IS_POINTER);
	if (issqlrelay) {
		// stmtptrinit was NULL; SQL Relay accepts NULL.
		assertSuccessStmt(stmt,erg);
	} else {
		// MariaDB rejects with HYC00 even though stmtptrinit is NULL
		assertFailureStmt(stmt,erg);
	}
	stdoutput.printf("\n");
	#endif


	// invalid attribute identifier
	// spec: get/set on an unrecognized attribute must return HY092
	stdoutput.printf("  invalid attribute\n");
	erg=SQLGetStmtAttr(stmt,99999,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	if (issqlrelay) {
		assertFailureStmt(stmt,erg);
	} else {
		// MariaDB returns SQL_SUCCESS instead of HY092.
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLSetStmtAttr(stmt,99999,
			(SQLPOINTER)(uintptr_t)0,0);
	assertFailureStmt(stmt,erg);
	stdoutput.printf("\n");



	// create testtable
	stdoutput.printf("CREATE TESTTABLE: \n");
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
		(SQLPOINTER)SQL_AUTOCOMMIT_OFF,0);
	assertSuccessDbc(dbc,erg);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable ("
		"	testtinyint tinyint, "
		"	testsmallint smallint, "
		"	testmediumint mediumint, "
		"	testint int, "
		"	testbigint bigint, "
		"	testfloat float, "
		"	testreal real, "
		"	testdecimal decimal(2,1), "
		"	testdate date, "
		"	testtime time, "
		"	testdatetime datetime, "
		"	testyear year, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testtext text, "
		"	testtinytext tinytext, "
		"	testmediumtext mediumtext, "
		"	testlongtext longtext, "
		"	testblob blob, "
		"	testtinyblob tinyblob, "
		"	testmediumblob mediumblob, "
		"	testlongblob longblob, "
		"	testtimestamp timestamp null)",
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
		"	1, "
		"	1, "
		"	1, "
		"	1, "
		"	1.5, "
		"	1.5, "
		"	1.5, "
		"	'2001-01-01', "
		"	'01:00:00', "
		"	'2001-01-01 01:00:00', "
		"	'2001', "
		"	'char1', "
		"	'varchar1', "
		"	'text1', "
		"	'tinytext1', "
		"	'mediumtext1', "
		"	'longtext1', "
		"	'blob1', "
		"	'tinyblob1', "
		"	'mediumblob1', "
		"	'longblob1', "
		"	NULL)",
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
	stdoutput.printf("INPUT BIND BY POSITION (prepare, bind, execute): \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLPrepare(stmt,(SQLCHAR *)
		"insert into "
		"	testtable "
		"values ( "
		"	?, ?, ?, ?, ?, "
		"	?, ?, ?, ?, ?, "
		"	?, ?, ?, ?, ?, "
		"	?, ?, ?, ?, ?, "
		"	?, ?, ?)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLSMALLINT	bindvarcount;
	erg=SQLNumParams(stmt,&bindvarcount);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)bindvarcount,23);

	SQLSCHAR		tinyintval;
	SQLSMALLINT		smallintval;
	SQLINTEGER		mediumintval;
	SQLINTEGER		intval;
	SQLBIGINT		bigintval;
	SQLREAL			floatval;
	SQLDOUBLE		realval;
	SQLCHAR			*decimalval;
	SQL_DATE_STRUCT		dateval;
	SQL_TIME_STRUCT		timeval;
	SQL_TIMESTAMP_STRUCT	datetimeval;
	SQLSMALLINT		yearval;
	SQLCHAR			*charval;
	SQLCHAR			*varcharval;
	SQLCHAR			*textval;
	SQLCHAR			*tinytextval;
	SQLCHAR			*mediumtextval;
	SQLCHAR			*longtextval;
	SQLCHAR			*blobval;
	SQLCHAR			*tinyblobval;
	SQLCHAR			*mediumblobval;
	SQLCHAR			*longblobval;

	SQLLEN		tinyintlen=sizeof(tinyintval);
	SQLLEN		smallintlen=sizeof(smallintval);
	SQLLEN		mediumintlen=sizeof(mediumintval);
	SQLLEN		intlen=sizeof(intval);
	SQLLEN		bigintlen=sizeof(bigintval);
	SQLLEN		floatlen=sizeof(floatval);
	SQLLEN		reallen=sizeof(realval);
	SQLLEN		decimallen=SQL_NTS;
	SQLLEN		datelen=sizeof(SQL_DATE_STRUCT);
	SQLLEN		timelen=sizeof(SQL_TIME_STRUCT);
	SQLLEN		datetimelen=sizeof(SQL_TIMESTAMP_STRUCT);
	SQLLEN		yearlen=sizeof(yearval);
	SQLLEN		charlen=SQL_NTS;
	SQLLEN		varcharlen=SQL_NTS;
	SQLLEN		textlen=SQL_NTS;
	SQLLEN		tinytextlen=SQL_NTS;
	SQLLEN		mediumtextlen=SQL_NTS;
	SQLLEN		longtextlen=SQL_NTS;
	SQLLEN		bloblen=5;
	SQLLEN		tinybloblen=9;
	SQLLEN		mediumbloblen=11;
	SQLLEN		longbloblen=9;
	SQLLEN		timestampnulllen=SQL_NULL_DATA;

	// row 2
	tinyintval=(SQLSCHAR)2;
	smallintval=(SQLSMALLINT)2;
	mediumintval=2;
	intval=2;
	bigintval=2;
	floatval=(SQLREAL)2.5;
	realval=(SQLDOUBLE)2.5;
	decimalval=(SQLCHAR *)"2.5";
	dateval.year=2002;
	dateval.month=1;
	dateval.day=1;
	timeval.hour=2;
	timeval.minute=0;
	timeval.second=0;
	datetimeval.year=2002;
	datetimeval.month=1;
	datetimeval.day=1;
	datetimeval.hour=2;
	datetimeval.minute=0;
	datetimeval.second=0;
	datetimeval.fraction=0;
	yearval=(SQLSMALLINT)2002;
	charval=(SQLCHAR *)"char2";
	varcharval=(SQLCHAR *)"varchar2";
	textval=(SQLCHAR *)"text2";
	tinytextval=(SQLCHAR *)"tinytext2";
	mediumtextval=(SQLCHAR *)"mediumtext2";
	longtextval=(SQLCHAR *)"longtext2";
	blobval=(SQLCHAR *)"blob2";
	tinyblobval=(SQLCHAR *)"tinyblob2";
	mediumblobval=(SQLCHAR *)"mediumblob2";
	longblobval=(SQLCHAR *)"longblob2";
	charlen=SQL_NTS;
	varcharlen=SQL_NTS;
	textlen=SQL_NTS;
	tinytextlen=SQL_NTS;
	mediumtextlen=SQL_NTS;
	longtextlen=SQL_NTS;
	decimallen=SQL_NTS;
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
			SQL_C_STINYINT,SQL_TINYINT,
			0,0,(SQLPOINTER)&tinyintval,
			sizeof(tinyintval),&tinyintlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
			SQL_C_SSHORT,SQL_SMALLINT,
			0,0,(SQLPOINTER)&smallintval,
			sizeof(smallintval),&smallintlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
			SQL_C_SLONG,SQL_INTEGER,
			0,0,(SQLPOINTER)&mediumintval,
			sizeof(mediumintval),&mediumintlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,4,SQL_PARAM_INPUT,
			SQL_C_SLONG,SQL_INTEGER,
			0,0,(SQLPOINTER)&intval,
			sizeof(intval),&intlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,5,SQL_PARAM_INPUT,
			SQL_C_SBIGINT,SQL_BIGINT,
			0,0,(SQLPOINTER)&bigintval,
			sizeof(bigintval),&bigintlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,6,SQL_PARAM_INPUT,
			SQL_C_FLOAT,SQL_REAL,
			0,0,(SQLPOINTER)&floatval,
			sizeof(floatval),&floatlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,7,SQL_PARAM_INPUT,
			SQL_C_DOUBLE,SQL_DOUBLE,
			0,0,(SQLPOINTER)&realval,
			sizeof(realval),&reallen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,8,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_DECIMAL,
			2,1,(SQLPOINTER)decimalval,
			0,&decimallen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,9,SQL_PARAM_INPUT,
			SQL_C_TYPE_DATE,SQL_TYPE_DATE,
			10,0,(SQLPOINTER)&dateval,
			sizeof(dateval),&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,10,SQL_PARAM_INPUT,
			SQL_C_TYPE_TIME,SQL_TYPE_TIME,
			8,0,(SQLPOINTER)&timeval,
			sizeof(timeval),&timelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,11,SQL_PARAM_INPUT,
			SQL_C_TYPE_TIMESTAMP,SQL_TYPE_TIMESTAMP,
			19,0,(SQLPOINTER)&datetimeval,
			sizeof(datetimeval),&datetimelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,12,SQL_PARAM_INPUT,
			SQL_C_SSHORT,SQL_SMALLINT,
			0,0,(SQLPOINTER)&yearval,
			sizeof(yearval),&yearlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,13,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_CHAR,
			40,0,(SQLPOINTER)charval,
			0,&charlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,14,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_VARCHAR,
			40,0,(SQLPOINTER)varcharval,
			0,&varcharlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,15,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_LONGVARCHAR,
			0,0,(SQLPOINTER)textval,
			0,&textlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,16,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_LONGVARCHAR,
			0,0,(SQLPOINTER)tinytextval,
			0,&tinytextlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,17,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_LONGVARCHAR,
			0,0,(SQLPOINTER)mediumtextval,
			0,&mediumtextlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,18,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_LONGVARCHAR,
			0,0,(SQLPOINTER)longtextval,
			0,&longtextlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,19,SQL_PARAM_INPUT,
			SQL_C_BINARY,SQL_LONGVARBINARY,
			0,0,(SQLPOINTER)blobval,
			0,&bloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,20,SQL_PARAM_INPUT,
			SQL_C_BINARY,SQL_LONGVARBINARY,
			0,0,(SQLPOINTER)tinyblobval,
			0,&tinybloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,21,SQL_PARAM_INPUT,
			SQL_C_BINARY,SQL_LONGVARBINARY,
			0,0,(SQLPOINTER)mediumblobval,
			0,&mediumbloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,22,SQL_PARAM_INPUT,
			SQL_C_BINARY,SQL_LONGVARBINARY,
			0,0,(SQLPOINTER)longblobval,
			0,&longbloblen);
	assertSuccessStmt(stmt,erg);
	// position 23 (testtimestamp) is bound as NULL
	erg=SQLBindParameter(stmt,23,SQL_PARAM_INPUT,
			SQL_C_TYPE_TIMESTAMP,SQL_TYPE_TIMESTAMP,
			19,0,(SQLPOINTER)&datetimeval,
			sizeof(datetimeval),&timestampnulllen);
	assertSuccessStmt(stmt,erg);

	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// input bind by position (bind, exec-direct)
	stdoutput.printf("INPUT BIND BY POSITION (bind, exec-direct): \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);

	// row 3
	tinyintval=(SQLSCHAR)3;
	smallintval=(SQLSMALLINT)3;
	mediumintval=3;
	intval=3;
	bigintval=3;
	floatval=(SQLREAL)3.5;
	realval=(SQLDOUBLE)3.5;
	decimalval=(SQLCHAR *)"3.5";
	dateval.year=2003;
	dateval.month=1;
	dateval.day=1;
	timeval.hour=3;
	timeval.minute=0;
	timeval.second=0;
	datetimeval.year=2003;
	datetimeval.month=1;
	datetimeval.day=1;
	datetimeval.hour=3;
	datetimeval.minute=0;
	datetimeval.second=0;
	datetimeval.fraction=0;
	yearval=(SQLSMALLINT)2003;
	charval=(SQLCHAR *)"char3";
	varcharval=(SQLCHAR *)"varchar3";
	textval=(SQLCHAR *)"text3";
	tinytextval=(SQLCHAR *)"tinytext3";
	mediumtextval=(SQLCHAR *)"mediumtext3";
	longtextval=(SQLCHAR *)"longtext3";
	blobval=(SQLCHAR *)"blob3";
	tinyblobval=(SQLCHAR *)"tinyblob3";
	mediumblobval=(SQLCHAR *)"mediumblob3";
	longblobval=(SQLCHAR *)"longblob3";
	charlen=SQL_NTS;
	varcharlen=SQL_NTS;
	textlen=SQL_NTS;
	tinytextlen=SQL_NTS;
	mediumtextlen=SQL_NTS;
	longtextlen=SQL_NTS;
	decimallen=SQL_NTS;
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
			SQL_C_STINYINT,SQL_TINYINT,
			0,0,(SQLPOINTER)&tinyintval,
			sizeof(tinyintval),&tinyintlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
			SQL_C_SSHORT,SQL_SMALLINT,
			0,0,(SQLPOINTER)&smallintval,
			sizeof(smallintval),&smallintlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
			SQL_C_SLONG,SQL_INTEGER,
			0,0,(SQLPOINTER)&mediumintval,
			sizeof(mediumintval),&mediumintlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,4,SQL_PARAM_INPUT,
			SQL_C_SLONG,SQL_INTEGER,
			0,0,(SQLPOINTER)&intval,
			sizeof(intval),&intlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,5,SQL_PARAM_INPUT,
			SQL_C_SBIGINT,SQL_BIGINT,
			0,0,(SQLPOINTER)&bigintval,
			sizeof(bigintval),&bigintlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,6,SQL_PARAM_INPUT,
			SQL_C_FLOAT,SQL_REAL,
			0,0,(SQLPOINTER)&floatval,
			sizeof(floatval),&floatlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,7,SQL_PARAM_INPUT,
			SQL_C_DOUBLE,SQL_DOUBLE,
			0,0,(SQLPOINTER)&realval,
			sizeof(realval),&reallen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,8,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_DECIMAL,
			2,1,(SQLPOINTER)decimalval,
			0,&decimallen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,9,SQL_PARAM_INPUT,
			SQL_C_TYPE_DATE,SQL_TYPE_DATE,
			10,0,(SQLPOINTER)&dateval,
			sizeof(dateval),&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,10,SQL_PARAM_INPUT,
			SQL_C_TYPE_TIME,SQL_TYPE_TIME,
			8,0,(SQLPOINTER)&timeval,
			sizeof(timeval),&timelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,11,SQL_PARAM_INPUT,
			SQL_C_TYPE_TIMESTAMP,SQL_TYPE_TIMESTAMP,
			19,0,(SQLPOINTER)&datetimeval,
			sizeof(datetimeval),&datetimelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,12,SQL_PARAM_INPUT,
			SQL_C_SSHORT,SQL_SMALLINT,
			0,0,(SQLPOINTER)&yearval,
			sizeof(yearval),&yearlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,13,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_CHAR,
			40,0,(SQLPOINTER)charval,
			0,&charlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,14,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_VARCHAR,
			40,0,(SQLPOINTER)varcharval,
			0,&varcharlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,15,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_LONGVARCHAR,
			0,0,(SQLPOINTER)textval,
			0,&textlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,16,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_LONGVARCHAR,
			0,0,(SQLPOINTER)tinytextval,
			0,&tinytextlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,17,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_LONGVARCHAR,
			0,0,(SQLPOINTER)mediumtextval,
			0,&mediumtextlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,18,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_LONGVARCHAR,
			0,0,(SQLPOINTER)longtextval,
			0,&longtextlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,19,SQL_PARAM_INPUT,
			SQL_C_BINARY,SQL_LONGVARBINARY,
			0,0,(SQLPOINTER)blobval,
			0,&bloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,20,SQL_PARAM_INPUT,
			SQL_C_BINARY,SQL_LONGVARBINARY,
			0,0,(SQLPOINTER)tinyblobval,
			0,&tinybloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,21,SQL_PARAM_INPUT,
			SQL_C_BINARY,SQL_LONGVARBINARY,
			0,0,(SQLPOINTER)mediumblobval,
			0,&mediumbloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,22,SQL_PARAM_INPUT,
			SQL_C_BINARY,SQL_LONGVARBINARY,
			0,0,(SQLPOINTER)longblobval,
			0,&longbloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,23,SQL_PARAM_INPUT,
			SQL_C_TYPE_TIMESTAMP,SQL_TYPE_TIMESTAMP,
			19,0,(SQLPOINTER)&datetimeval,
			sizeof(datetimeval),&timestampnulllen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into "
		"	testtable "
		"values ( "
		"	?, ?, ?, ?, ?, "
		"	?, ?, ?, ?, ?, "
		"	?, ?, ?, ?, ?, "
		"	?, ?, ?, ?, ?, "
		"	?, ?, ?)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// input bind by position (prepare, bind, execute, putdata)
	stdoutput.printf("INPUT BIND BY POSITION "
			"(prepare, bind, execute, putdata): \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLPrepare(stmt,(SQLCHAR *)
		"insert into "
		"	testtable "
		"values ( "
		"	?, ?, ?, ?, ?, "
		"	?, ?, ?, ?, ?, "
		"	?, ?, ?, ?, ?, "
		"	?, ?, ?, ?, ?, "
		"	?, ?, ?)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);

	// row 4
	tinyintval=(SQLSCHAR)4;
	smallintval=(SQLSMALLINT)4;
	mediumintval=4;
	intval=4;
	bigintval=4;
	floatval=(SQLREAL)4.5;
	realval=(SQLDOUBLE)4.5;
	decimalval=(SQLCHAR *)"4.5";
	dateval.year=2004;
	dateval.month=1;
	dateval.day=1;
	timeval.hour=4;
	timeval.minute=0;
	timeval.second=0;
	datetimeval.year=2004;
	datetimeval.month=1;
	datetimeval.day=1;
	datetimeval.hour=4;
	datetimeval.minute=0;
	datetimeval.second=0;
	datetimeval.fraction=0;
	yearval=(SQLSMALLINT)2004;
	charval=(SQLCHAR *)"char4";
	varcharval=(SQLCHAR *)"varchar4";
	textval=(SQLCHAR *)"text4";
	tinytextval=(SQLCHAR *)"tinytext4";
	mediumtextval=(SQLCHAR *)"mediumtext4";
	longtextval=(SQLCHAR *)"longtext4";
	blobval=(SQLCHAR *)"blob4";
	tinyblobval=(SQLCHAR *)"tinyblob4";
	mediumblobval=(SQLCHAR *)"mediumblob4";
	longblobval=(SQLCHAR *)"longblob4";
	charlen=SQL_NTS;
	varcharlen=SQL_NTS;
	textlen=SQL_NTS;
	tinytextlen=SQL_NTS;
	mediumtextlen=SQL_NTS;
	longtextlen=SQL_NTS;
	decimallen=SQL_NTS;
	// data-at-exec sentinel for the longtext column
	SQLLEN	longtextdaelen=SQL_LEN_DATA_AT_EXEC(9);
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
			SQL_C_STINYINT,SQL_TINYINT,
			0,0,(SQLPOINTER)&tinyintval,
			sizeof(tinyintval),&tinyintlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
			SQL_C_SSHORT,SQL_SMALLINT,
			0,0,(SQLPOINTER)&smallintval,
			sizeof(smallintval),&smallintlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
			SQL_C_SLONG,SQL_INTEGER,
			0,0,(SQLPOINTER)&mediumintval,
			sizeof(mediumintval),&mediumintlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,4,SQL_PARAM_INPUT,
			SQL_C_SLONG,SQL_INTEGER,
			0,0,(SQLPOINTER)&intval,
			sizeof(intval),&intlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,5,SQL_PARAM_INPUT,
			SQL_C_SBIGINT,SQL_BIGINT,
			0,0,(SQLPOINTER)&bigintval,
			sizeof(bigintval),&bigintlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,6,SQL_PARAM_INPUT,
			SQL_C_FLOAT,SQL_REAL,
			0,0,(SQLPOINTER)&floatval,
			sizeof(floatval),&floatlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,7,SQL_PARAM_INPUT,
			SQL_C_DOUBLE,SQL_DOUBLE,
			0,0,(SQLPOINTER)&realval,
			sizeof(realval),&reallen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,8,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_DECIMAL,
			2,1,(SQLPOINTER)decimalval,
			0,&decimallen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,9,SQL_PARAM_INPUT,
			SQL_C_TYPE_DATE,SQL_TYPE_DATE,
			10,0,(SQLPOINTER)&dateval,
			sizeof(dateval),&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,10,SQL_PARAM_INPUT,
			SQL_C_TYPE_TIME,SQL_TYPE_TIME,
			8,0,(SQLPOINTER)&timeval,
			sizeof(timeval),&timelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,11,SQL_PARAM_INPUT,
			SQL_C_TYPE_TIMESTAMP,SQL_TYPE_TIMESTAMP,
			19,0,(SQLPOINTER)&datetimeval,
			sizeof(datetimeval),&datetimelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,12,SQL_PARAM_INPUT,
			SQL_C_SSHORT,SQL_SMALLINT,
			0,0,(SQLPOINTER)&yearval,
			sizeof(yearval),&yearlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,13,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_CHAR,
			40,0,(SQLPOINTER)charval,
			0,&charlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,14,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_VARCHAR,
			40,0,(SQLPOINTER)varcharval,
			0,&varcharlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,15,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_LONGVARCHAR,
			0,0,(SQLPOINTER)textval,
			0,&textlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,16,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_LONGVARCHAR,
			0,0,(SQLPOINTER)tinytextval,
			0,&tinytextlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,17,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_LONGVARCHAR,
			0,0,(SQLPOINTER)mediumtextval,
			0,&mediumtextlen);
	assertSuccessStmt(stmt,erg);
	// longtextval is the data-at-exec token; real value via SQLPutData below
	erg=SQLBindParameter(stmt,18,SQL_PARAM_INPUT,
			SQL_C_CHAR,SQL_LONGVARCHAR,
			0,0,(SQLPOINTER)longtextval,
			0,&longtextdaelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,19,SQL_PARAM_INPUT,
			SQL_C_BINARY,SQL_LONGVARBINARY,
			0,0,(SQLPOINTER)blobval,
			0,&bloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,20,SQL_PARAM_INPUT,
			SQL_C_BINARY,SQL_LONGVARBINARY,
			0,0,(SQLPOINTER)tinyblobval,
			0,&tinybloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,21,SQL_PARAM_INPUT,
			SQL_C_BINARY,SQL_LONGVARBINARY,
			0,0,(SQLPOINTER)mediumblobval,
			0,&mediumbloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,22,SQL_PARAM_INPUT,
			SQL_C_BINARY,SQL_LONGVARBINARY,
			0,0,(SQLPOINTER)longblobval,
			0,&longbloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,23,SQL_PARAM_INPUT,
			SQL_C_TYPE_TIMESTAMP,SQL_TYPE_TIMESTAMP,
			19,0,(SQLPOINTER)&datetimeval,
			sizeof(datetimeval),&timestampnulllen);
	assertSuccessStmt(stmt,erg);
	// data-at-exec column triggers SQL_NEED_DATA
	erg=SQLExecute(stmt);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NEED_DATA);
	// drive the data-at-exec loop
	SQLPOINTER	paramdataptr=NULL;
	erg=SQLParamData(stmt,&paramdataptr);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NEED_DATA);
	assertEqualStmt(stmt,(const char *)paramdataptr,"longtext4");
	erg=SQLPutData(stmt,longtextval,9);
	assertSuccessStmt(stmt,erg);
	// final SQLParamData completes execution
	erg=SQLParamData(stmt,&paramdataptr);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// select
	stdoutput.printf("SELECT: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testint",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// column count
	stdoutput.printf("COLUMN COUNT: \n");
	SQLSMALLINT	colcount;
	erg=SQLNumResultCols(stmt,&colcount);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)colcount,23);
	stdoutput.printf("\n");



	// column info
	stdoutput.printf("COLUMN INFO: \n");
	SQLCHAR		colname[256];
	SQLSMALLINT	colnamelen;
	SQLSMALLINT	datatype;
	SQLULEN		colsize;
	SQLSMALLINT	decdigits;
	SQLSMALLINT	nullable;

	// col 1
	erg=SQLDescribeCol(stmt,1,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testtinyint");
	assertEqualStmt(stmt,(int)colnamelen,11);
	assertEqualStmt(stmt,(int)datatype,SQL_TINYINT);
	assertEqualStmt(stmt,(int)colsize,3);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 2
	erg=SQLDescribeCol(stmt,2,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testsmallint");
	assertEqualStmt(stmt,(int)colnamelen,12);
	assertEqualStmt(stmt,(int)datatype,SQL_SMALLINT);
	assertEqualStmt(stmt,(int)colsize,5);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 3
	erg=SQLDescribeCol(stmt,3,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testmediumint");
	assertEqualStmt(stmt,(int)colnamelen,13);
	assertEqualStmt(stmt,(int)datatype,SQL_INTEGER);
	assertEqualStmt(stmt,(int)colsize,10);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 4
	erg=SQLDescribeCol(stmt,4,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testint");
	assertEqualStmt(stmt,(int)colnamelen,7);
	assertEqualStmt(stmt,(int)datatype,SQL_INTEGER);
	assertEqualStmt(stmt,(int)colsize,10);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 5
	erg=SQLDescribeCol(stmt,5,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testbigint");
	assertEqualStmt(stmt,(int)colnamelen,10);
	assertEqualStmt(stmt,(int)datatype,SQL_BIGINT);
	if (issqlrelay) {
		// the precision of a signed bigint is 19 digits
		assertEqualStmt(stmt,(int)colsize,19);
	} else {
		// MariaDB reports 20 (the precision of an unsigned bigint)
		assertEqualStmt(stmt,(int)colsize,20);
	}
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 6
	erg=SQLDescribeCol(stmt,6,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testfloat");
	assertEqualStmt(stmt,(int)colnamelen,9);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)datatype,SQL_FLOAT);
		assertEqualStmt(stmt,(int)colsize,15);
		assertEqualStmt(stmt,(int)decdigits,31);
	} else {
		// MariaDB describes float as SQL_REAL/7/0.
		assertEqualStmt(stmt,(int)datatype,SQL_REAL);
		assertEqualStmt(stmt,(int)colsize,7);
		assertEqualStmt(stmt,(int)decdigits,0);
	}
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 7
	erg=SQLDescribeCol(stmt,7,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testreal");
	assertEqualStmt(stmt,(int)colnamelen,8);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)datatype,SQL_REAL);
		assertEqualStmt(stmt,(int)colsize,7);
		assertEqualStmt(stmt,(int)decdigits,31);
	} else {
		// MariaDB describes real as SQL_DOUBLE/15/0
		// (REAL is a synonym for DOUBLE PRECISION)
		assertEqualStmt(stmt,(int)datatype,SQL_DOUBLE);
		assertEqualStmt(stmt,(int)colsize,15);
		assertEqualStmt(stmt,(int)decdigits,0);
	}
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 8
	erg=SQLDescribeCol(stmt,8,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testdecimal");
	assertEqualStmt(stmt,(int)colnamelen,11);
	assertEqualStmt(stmt,(int)datatype,SQL_DECIMAL);
	// column size is the precision (2) of decimal(2,1)
	assertEqualStmt(stmt,(int)colsize,2);
	assertEqualStmt(stmt,(int)decdigits,1);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 9
	erg=SQLDescribeCol(stmt,9,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testdate");
	assertEqualStmt(stmt,(int)colnamelen,8);
	assertEqualStmt(stmt,(int)datatype,SQL_TYPE_DATE);
	assertEqualStmt(stmt,(int)colsize,10);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 10
	erg=SQLDescribeCol(stmt,10,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testtime");
	assertEqualStmt(stmt,(int)colnamelen,8);
	assertEqualStmt(stmt,(int)datatype,SQL_TYPE_TIME);
	assertEqualStmt(stmt,(int)colsize,8);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 11
	erg=SQLDescribeCol(stmt,11,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testdatetime");
	assertEqualStmt(stmt,(int)colnamelen,12);
	assertEqualStmt(stmt,(int)datatype,SQL_TYPE_TIMESTAMP);
	assertEqualStmt(stmt,(int)colsize,19);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 12
	erg=SQLDescribeCol(stmt,12,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testyear");
	assertEqualStmt(stmt,(int)colnamelen,8);
	assertEqualStmt(stmt,(int)datatype,SQL_SMALLINT);
	assertEqualStmt(stmt,(int)colsize,5);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 13
	erg=SQLDescribeCol(stmt,13,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testchar");
	assertEqualStmt(stmt,(int)colnamelen,8);
	assertEqualStmt(stmt,(int)datatype,SQL_CHAR);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)colsize,160);
	} else {
		// MariaDB reports char count (40), not utf8mb4 byte width (160)
		assertEqualStmt(stmt,(int)colsize,40);
	}
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 14
	erg=SQLDescribeCol(stmt,14,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testvarchar");
	assertEqualStmt(stmt,(int)colnamelen,11);
	assertEqualStmt(stmt,(int)datatype,SQL_VARCHAR);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)colsize,161);
	} else {
		// MariaDB reports char count (40), not utf8mb4 byte width (161)
		assertEqualStmt(stmt,(int)colsize,40);
	}
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 15
	erg=SQLDescribeCol(stmt,15,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testtext");
	assertEqualStmt(stmt,(int)colnamelen,8);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)datatype,-2);
		assertEqualStmt(stmt,(int)colsize,262140);
	} else {
		// MariaDB reports SQL_LONGVARCHAR/65535.
		assertEqualStmt(stmt,(int)datatype,-1);
		assertEqualStmt(stmt,(int)colsize,65535);
	}
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 16
	erg=SQLDescribeCol(stmt,16,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testtinytext");
	assertEqualStmt(stmt,(int)colnamelen,12);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)datatype,-2);
		assertEqualStmt(stmt,(int)colsize,1020);
	} else {
		// TINYTEXT: MariaDB reports SQL_LONGVARCHAR/255.
		assertEqualStmt(stmt,(int)datatype,-1);
		assertEqualStmt(stmt,(int)colsize,255);
	}
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 17
	erg=SQLDescribeCol(stmt,17,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testmediumtext");
	assertEqualStmt(stmt,(int)colnamelen,14);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)datatype,-2);
		assertEqualStmt(stmt,(int)colsize,67108860);
	} else {
		// MEDIUMTEXT: MariaDB reports SQL_LONGVARCHAR/16777215.
		assertEqualStmt(stmt,(int)datatype,-1);
		assertEqualStmt(stmt,(int)colsize,16777215);
	}
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 18
	erg=SQLDescribeCol(stmt,18,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testlongtext");
	assertEqualStmt(stmt,(int)colnamelen,12);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)datatype,-2);
	} else {
		// LONGTEXT: MariaDB reports SQL_LONGVARCHAR (-1).
		assertEqualStmt(stmt,(int)datatype,-1);
	}
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 19
	erg=SQLDescribeCol(stmt,19,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testblob");
	assertEqualStmt(stmt,(int)colnamelen,8);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)datatype,-2);
	} else {
		// BLOB: MariaDB reports SQL_LONGVARBINARY (-4), not
		// SQL_VARBINARY (-2)
		assertEqualStmt(stmt,(int)datatype,-4);
	}
	assertEqualStmt(stmt,(int)colsize,65535);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 20
	erg=SQLDescribeCol(stmt,20,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testtinyblob");
	assertEqualStmt(stmt,(int)colnamelen,12);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)datatype,-2);
	} else {
		// TINYBLOB: MariaDB reports SQL_LONGVARBINARY (-4).
		assertEqualStmt(stmt,(int)datatype,-4);
	}
	assertEqualStmt(stmt,(int)colsize,255);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 21
	erg=SQLDescribeCol(stmt,21,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testmediumblob");
	assertEqualStmt(stmt,(int)colnamelen,14);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)datatype,-2);
	} else {
		// MEDIUMBLOB: MariaDB reports SQL_LONGVARBINARY (-4).
		assertEqualStmt(stmt,(int)datatype,-4);
	}
	assertEqualStmt(stmt,(int)colsize,16777215);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 22
	erg=SQLDescribeCol(stmt,22,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testlongblob");
	assertEqualStmt(stmt,(int)colnamelen,12);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)datatype,-2);
	} else {
		// LONGBLOB: MariaDB reports SQL_LONGVARBINARY (-4).
		assertEqualStmt(stmt,(int)datatype,-4);
	}
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 23
	erg=SQLDescribeCol(stmt,23,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testtimestamp");
	assertEqualStmt(stmt,(int)colnamelen,13);
	assertEqualStmt(stmt,(int)datatype,SQL_TYPE_TIMESTAMP);
	assertEqualStmt(stmt,(int)colsize,19);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);
	stdoutput.printf("\n");



	// fetch rows (SQLBindCol)
	stdoutput.printf("FETCH ROWS (SQLBindCol): \n");
	SQLSCHAR		tinyintfield;
	SQLSMALLINT		smallintfield;
	SQLINTEGER		mediumintfield;
	SQLINTEGER		intfield;
	SQLBIGINT		bigintfield;
	SQLREAL			floatfield;
	SQLDOUBLE		realfield;
	SQLCHAR			decimalfield[16];
	SQL_DATE_STRUCT		datefield;
	SQL_TIME_STRUCT		timefield;
	SQL_TIMESTAMP_STRUCT	datetimefield;
	SQLSMALLINT		yearfield;
	SQLCHAR			charfield[64];
	SQLCHAR			varcharfield[64];
	SQLCHAR			textfield[256];
	SQLCHAR			tinytextfield[256];
	SQLCHAR			mediumtextfield[256];
	SQLCHAR			longtextfield[256];
	SQLCHAR			blobfield[256];
	SQLCHAR			tinyblobfield[256];
	SQLCHAR			mediumblobfield[256];
	SQLCHAR			longblobfield[256];
	SQL_TIMESTAMP_STRUCT	timestampfield;
	SQLLEN			tinyintind;
	SQLLEN			smallintind;
	SQLLEN			mediumintind;
	SQLLEN			intind;
	SQLLEN			bigintind;
	SQLLEN			floatind;
	SQLLEN			realind;
	SQLLEN			decimalind;
	SQLLEN			dateind;
	SQLLEN			timeind;
	SQLLEN			datetimeind;
	SQLLEN			yearind;
	SQLLEN			charind;
	SQLLEN			varcharind;
	SQLLEN			textind;
	SQLLEN			tinytextind;
	SQLLEN			mediumtextind;
	SQLLEN			longtextind;
	SQLLEN			blobind;
	SQLLEN			tinyblobind;
	SQLLEN			mediumblobind;
	SQLLEN			longblobind;
	SQLLEN			timestampind;

	erg=SQLBindCol(stmt,1,SQL_C_STINYINT,
			&tinyintfield,sizeof(tinyintfield),&tinyintind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,2,SQL_C_SSHORT,
			&smallintfield,sizeof(smallintfield),&smallintind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,3,SQL_C_SLONG,
			&mediumintfield,sizeof(mediumintfield),&mediumintind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,4,SQL_C_SLONG,
			&intfield,sizeof(intfield),&intind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,5,SQL_C_SBIGINT,
			&bigintfield,sizeof(bigintfield),&bigintind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,6,SQL_C_FLOAT,
			&floatfield,sizeof(floatfield),&floatind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,7,SQL_C_DOUBLE,
			&realfield,sizeof(realfield),&realind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,8,SQL_C_CHAR,
			decimalfield,sizeof(decimalfield),&decimalind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,9,SQL_C_TYPE_DATE,
			&datefield,sizeof(datefield),&dateind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,10,SQL_C_TYPE_TIME,
			&timefield,sizeof(timefield),&timeind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,11,SQL_C_TYPE_TIMESTAMP,
			&datetimefield,sizeof(datetimefield),&datetimeind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,12,SQL_C_SSHORT,
			&yearfield,sizeof(yearfield),&yearind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,13,SQL_C_CHAR,
			charfield,sizeof(charfield),&charind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,14,SQL_C_CHAR,
			varcharfield,sizeof(varcharfield),&varcharind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,15,SQL_C_CHAR,
			textfield,sizeof(textfield),&textind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,16,SQL_C_CHAR,
			tinytextfield,sizeof(tinytextfield),&tinytextind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,17,SQL_C_CHAR,
			mediumtextfield,sizeof(mediumtextfield),&mediumtextind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,18,SQL_C_CHAR,
			longtextfield,sizeof(longtextfield),&longtextind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,19,SQL_C_BINARY,
			blobfield,sizeof(blobfield),&blobind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,20,SQL_C_BINARY,
			tinyblobfield,sizeof(tinyblobfield),&tinyblobind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,21,SQL_C_BINARY,
			mediumblobfield,sizeof(mediumblobfield),&mediumblobind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,22,SQL_C_BINARY,
			longblobfield,sizeof(longblobfield),&longblobind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,23,SQL_C_TYPE_TIMESTAMP,
			&timestampfield,sizeof(timestampfield),&timestampind);
	assertSuccessStmt(stmt,erg);

	// row 1
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)tinyintfield,1);
	assertEqualStmt(stmt,(int)smallintfield,1);
	assertEqualStmt(stmt,(int)mediumintfield,1);
	assertEqualStmt(stmt,(int)intfield,1);
	assertEqualStmt(stmt,(int)bigintfield,1);
	assertTrueStmt(stmt,floatfield==1.5);
	assertTrueStmt(stmt,realfield==1.5);
	assertEqualStmt(stmt,(const char *)decimalfield,"1.5");
	assertEqualStmt(stmt,(int)yearfield,2001);
	assertEqualStmt(stmt,(int)datefield.year,2001);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	assertEqualStmt(stmt,(int)timefield.hour,1);
	assertEqualStmt(stmt,(int)timefield.minute,0);
	assertEqualStmt(stmt,(int)timefield.second,0);
	assertEqualStmt(stmt,(int)datetimefield.year,2001);
	assertEqualStmt(stmt,(int)datetimefield.month,1);
	assertEqualStmt(stmt,(int)datetimefield.day,1);
	assertEqualStmt(stmt,(int)datetimefield.hour,1);
	assertEqualStmt(stmt,(int)datetimefield.minute,0);
	assertEqualStmt(stmt,(int)datetimefield.second,0);
	assertEqualStmt(stmt,(const char *)charfield,"char1");
	assertEqualStmt(stmt,(const char *)varcharfield,"varchar1");
	assertEqualStmt(stmt,(const char *)textfield,"text1");
	assertEqualStmt(stmt,(const char *)tinytextfield,"tinytext1");
	assertEqualStmt(stmt,(const char *)mediumtextfield,"mediumtext1");
	assertEqualStmt(stmt,(const char *)longtextfield,"longtext1");
	assertEqualStmt(stmt,(int)blobind,5);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"blob1",5));
	assertEqualStmt(stmt,(int)tinyblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(tinyblobfield,"tinyblob1",9));
	assertEqualStmt(stmt,(int)mediumblobind,11);
	assertTrueStmt(stmt,!bytestring::compare(mediumblobfield,"mediumblob1",11));
	assertEqualStmt(stmt,(int)longblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(longblobfield,"longblob1",9));
	assertEqualStmt(stmt,(int)timestampind,(int)SQL_NULL_DATA);

	// row 2
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)tinyintfield,2);
	assertEqualStmt(stmt,(int)smallintfield,2);
	assertEqualStmt(stmt,(int)mediumintfield,2);
	assertEqualStmt(stmt,(int)intfield,2);
	assertEqualStmt(stmt,(int)bigintfield,2);
	assertTrueStmt(stmt,floatfield==2.5);
	assertTrueStmt(stmt,realfield==2.5);
	assertEqualStmt(stmt,(const char *)decimalfield,"2.5");
	assertEqualStmt(stmt,(int)yearfield,2002);
	assertEqualStmt(stmt,(int)datefield.year,2002);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	assertEqualStmt(stmt,(int)timefield.hour,2);
	assertEqualStmt(stmt,(int)timefield.minute,0);
	assertEqualStmt(stmt,(int)timefield.second,0);
	assertEqualStmt(stmt,(int)datetimefield.year,2002);
	assertEqualStmt(stmt,(int)datetimefield.month,1);
	assertEqualStmt(stmt,(int)datetimefield.day,1);
	assertEqualStmt(stmt,(int)datetimefield.hour,2);
	assertEqualStmt(stmt,(int)datetimefield.minute,0);
	assertEqualStmt(stmt,(int)datetimefield.second,0);
	assertEqualStmt(stmt,(const char *)charfield,"char2");
	assertEqualStmt(stmt,(const char *)varcharfield,"varchar2");
	assertEqualStmt(stmt,(const char *)textfield,"text2");
	assertEqualStmt(stmt,(const char *)tinytextfield,"tinytext2");
	assertEqualStmt(stmt,(const char *)mediumtextfield,"mediumtext2");
	assertEqualStmt(stmt,(const char *)longtextfield,"longtext2");
	assertEqualStmt(stmt,(int)blobind,5);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"blob2",5));
	assertEqualStmt(stmt,(int)tinyblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(tinyblobfield,"tinyblob2",9));
	assertEqualStmt(stmt,(int)mediumblobind,11);
	assertTrueStmt(stmt,!bytestring::compare(mediumblobfield,"mediumblob2",11));
	assertEqualStmt(stmt,(int)longblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(longblobfield,"longblob2",9));
	assertEqualStmt(stmt,(int)timestampind,(int)SQL_NULL_DATA);

	// row 3
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)tinyintfield,3);
	assertEqualStmt(stmt,(int)smallintfield,3);
	assertEqualStmt(stmt,(int)mediumintfield,3);
	assertEqualStmt(stmt,(int)intfield,3);
	assertEqualStmt(stmt,(int)bigintfield,3);
	assertTrueStmt(stmt,floatfield==3.5);
	assertTrueStmt(stmt,realfield==3.5);
	assertEqualStmt(stmt,(const char *)decimalfield,"3.5");
	assertEqualStmt(stmt,(int)yearfield,2003);
	assertEqualStmt(stmt,(int)datefield.year,2003);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	assertEqualStmt(stmt,(int)timefield.hour,3);
	assertEqualStmt(stmt,(int)timefield.minute,0);
	assertEqualStmt(stmt,(int)timefield.second,0);
	assertEqualStmt(stmt,(int)datetimefield.year,2003);
	assertEqualStmt(stmt,(int)datetimefield.month,1);
	assertEqualStmt(stmt,(int)datetimefield.day,1);
	assertEqualStmt(stmt,(int)datetimefield.hour,3);
	assertEqualStmt(stmt,(int)datetimefield.minute,0);
	assertEqualStmt(stmt,(int)datetimefield.second,0);
	assertEqualStmt(stmt,(const char *)charfield,"char3");
	assertEqualStmt(stmt,(const char *)varcharfield,"varchar3");
	assertEqualStmt(stmt,(const char *)textfield,"text3");
	assertEqualStmt(stmt,(const char *)tinytextfield,"tinytext3");
	assertEqualStmt(stmt,(const char *)mediumtextfield,"mediumtext3");
	assertEqualStmt(stmt,(const char *)longtextfield,"longtext3");
	assertEqualStmt(stmt,(int)blobind,5);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"blob3",5));
	assertEqualStmt(stmt,(int)tinyblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(tinyblobfield,"tinyblob3",9));
	assertEqualStmt(stmt,(int)mediumblobind,11);
	assertTrueStmt(stmt,!bytestring::compare(mediumblobfield,"mediumblob3",11));
	assertEqualStmt(stmt,(int)longblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(longblobfield,"longblob3",9));
	assertEqualStmt(stmt,(int)timestampind,(int)SQL_NULL_DATA);

	// row 4
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)tinyintfield,4);
	assertEqualStmt(stmt,(int)smallintfield,4);
	assertEqualStmt(stmt,(int)mediumintfield,4);
	assertEqualStmt(stmt,(int)intfield,4);
	assertEqualStmt(stmt,(int)bigintfield,4);
	assertTrueStmt(stmt,floatfield==4.5);
	assertTrueStmt(stmt,realfield==4.5);
	assertEqualStmt(stmt,(const char *)decimalfield,"4.5");
	assertEqualStmt(stmt,(int)yearfield,2004);
	assertEqualStmt(stmt,(int)datefield.year,2004);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	assertEqualStmt(stmt,(int)timefield.hour,4);
	assertEqualStmt(stmt,(int)timefield.minute,0);
	assertEqualStmt(stmt,(int)timefield.second,0);
	assertEqualStmt(stmt,(int)datetimefield.year,2004);
	assertEqualStmt(stmt,(int)datetimefield.month,1);
	assertEqualStmt(stmt,(int)datetimefield.day,1);
	assertEqualStmt(stmt,(int)datetimefield.hour,4);
	assertEqualStmt(stmt,(int)datetimefield.minute,0);
	assertEqualStmt(stmt,(int)datetimefield.second,0);
	assertEqualStmt(stmt,(const char *)charfield,"char4");
	assertEqualStmt(stmt,(const char *)varcharfield,"varchar4");
	assertEqualStmt(stmt,(const char *)textfield,"text4");
	assertEqualStmt(stmt,(const char *)tinytextfield,"tinytext4");
	assertEqualStmt(stmt,(const char *)mediumtextfield,"mediumtext4");
	assertEqualStmt(stmt,(const char *)longtextfield,"longtext4");
	assertEqualStmt(stmt,(int)blobind,5);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"blob4",5));
	assertEqualStmt(stmt,(int)tinyblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(tinyblobfield,"tinyblob4",9));
	assertEqualStmt(stmt,(int)mediumblobind,11);
	assertTrueStmt(stmt,!bytestring::compare(mediumblobfield,"mediumblob4",11));
	assertEqualStmt(stmt,(int)longblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(longblobfield,"longblob4",9));
	assertEqualStmt(stmt,(int)timestampind,(int)SQL_NULL_DATA);

	// no more rows
	erg=SQLFetch(stmt);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NO_DATA);
	stdoutput.printf("\n");



	// select
	stdoutput.printf("SELECT: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testint",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// fetch rows (SQLGetData)
	stdoutput.printf("FETCH ROWS (SQLGetData): \n");

	// row 1
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetData(stmt,1,SQL_C_STINYINT,
			&tinyintfield,sizeof(tinyintfield),&tinyintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)tinyintfield,1);
	erg=SQLGetData(stmt,2,SQL_C_SSHORT,
			&smallintfield,sizeof(smallintfield),&smallintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)smallintfield,1);
	erg=SQLGetData(stmt,3,SQL_C_SLONG,
			&mediumintfield,sizeof(mediumintfield),&mediumintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)mediumintfield,1);
	erg=SQLGetData(stmt,4,SQL_C_SLONG,
			&intfield,sizeof(intfield),&intind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)intfield,1);
	erg=SQLGetData(stmt,5,SQL_C_SBIGINT,
			&bigintfield,sizeof(bigintfield),&bigintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)bigintfield,1);
	erg=SQLGetData(stmt,6,SQL_C_FLOAT,
			&floatfield,sizeof(floatfield),&floatind);
	assertSuccessStmt(stmt,erg);
	assertTrueStmt(stmt,floatfield==1.5);
	erg=SQLGetData(stmt,7,SQL_C_DOUBLE,
			&realfield,sizeof(realfield),&realind);
	assertSuccessStmt(stmt,erg);
	assertTrueStmt(stmt,realfield==1.5);
	erg=SQLGetData(stmt,8,SQL_C_CHAR,
			decimalfield,sizeof(decimalfield),&decimalind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)decimalfield,"1.5");
	erg=SQLGetData(stmt,9,SQL_C_TYPE_DATE,
			&datefield,sizeof(datefield),&dateind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)datefield.year,2001);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	erg=SQLGetData(stmt,10,SQL_C_TYPE_TIME,
			&timefield,sizeof(timefield),&timeind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)timefield.hour,1);
	assertEqualStmt(stmt,(int)timefield.minute,0);
	assertEqualStmt(stmt,(int)timefield.second,0);
	erg=SQLGetData(stmt,11,SQL_C_TYPE_TIMESTAMP,
			&datetimefield,sizeof(datetimefield),&datetimeind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)datetimefield.year,2001);
	assertEqualStmt(stmt,(int)datetimefield.month,1);
	assertEqualStmt(stmt,(int)datetimefield.day,1);
	assertEqualStmt(stmt,(int)datetimefield.hour,1);
	erg=SQLGetData(stmt,12,SQL_C_SSHORT,
			&yearfield,sizeof(yearfield),&yearind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)yearfield,2001);
	erg=SQLGetData(stmt,13,SQL_C_CHAR,
			charfield,sizeof(charfield),&charind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)charfield,"char1");
	erg=SQLGetData(stmt,14,SQL_C_CHAR,
			varcharfield,sizeof(varcharfield),&varcharind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)varcharfield,"varchar1");
	erg=SQLGetData(stmt,15,SQL_C_CHAR,
			textfield,sizeof(textfield),&textind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)textfield,"text1");
	erg=SQLGetData(stmt,16,SQL_C_CHAR,
			tinytextfield,sizeof(tinytextfield),&tinytextind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)tinytextfield,"tinytext1");
	erg=SQLGetData(stmt,17,SQL_C_CHAR,
			mediumtextfield,sizeof(mediumtextfield),&mediumtextind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)mediumtextfield,"mediumtext1");
	erg=SQLGetData(stmt,18,SQL_C_CHAR,
			longtextfield,sizeof(longtextfield),&longtextind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)longtextfield,"longtext1");
	erg=SQLGetData(stmt,19,SQL_C_BINARY,
			blobfield,sizeof(blobfield),&blobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)blobind,5);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"blob1",5));
	erg=SQLGetData(stmt,20,SQL_C_BINARY,
			tinyblobfield,sizeof(tinyblobfield),&tinyblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)tinyblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(tinyblobfield,"tinyblob1",9));
	erg=SQLGetData(stmt,21,SQL_C_BINARY,
			mediumblobfield,sizeof(mediumblobfield),&mediumblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)mediumblobind,11);
	assertTrueStmt(stmt,!bytestring::compare(mediumblobfield,"mediumblob1",11));
	erg=SQLGetData(stmt,22,SQL_C_BINARY,
			longblobfield,sizeof(longblobfield),&longblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)longblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(longblobfield,"longblob1",9));
	erg=SQLGetData(stmt,23,SQL_C_TYPE_TIMESTAMP,
			&timestampfield,sizeof(timestampfield),&timestampind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)timestampind,(int)SQL_NULL_DATA);

	// row 2
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetData(stmt,1,SQL_C_STINYINT,
			&tinyintfield,sizeof(tinyintfield),&tinyintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)tinyintfield,2);
	erg=SQLGetData(stmt,2,SQL_C_SSHORT,
			&smallintfield,sizeof(smallintfield),&smallintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)smallintfield,2);
	erg=SQLGetData(stmt,3,SQL_C_SLONG,
			&mediumintfield,sizeof(mediumintfield),&mediumintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)mediumintfield,2);
	erg=SQLGetData(stmt,4,SQL_C_SLONG,
			&intfield,sizeof(intfield),&intind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)intfield,2);
	erg=SQLGetData(stmt,5,SQL_C_SBIGINT,
			&bigintfield,sizeof(bigintfield),&bigintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)bigintfield,2);
	erg=SQLGetData(stmt,6,SQL_C_FLOAT,
			&floatfield,sizeof(floatfield),&floatind);
	assertSuccessStmt(stmt,erg);
	assertTrueStmt(stmt,floatfield==2.5);
	erg=SQLGetData(stmt,7,SQL_C_DOUBLE,
			&realfield,sizeof(realfield),&realind);
	assertSuccessStmt(stmt,erg);
	assertTrueStmt(stmt,realfield==2.5);
	erg=SQLGetData(stmt,8,SQL_C_CHAR,
			decimalfield,sizeof(decimalfield),&decimalind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)decimalfield,"2.5");
	erg=SQLGetData(stmt,9,SQL_C_TYPE_DATE,
			&datefield,sizeof(datefield),&dateind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)datefield.year,2002);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	erg=SQLGetData(stmt,10,SQL_C_TYPE_TIME,
			&timefield,sizeof(timefield),&timeind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)timefield.hour,2);
	assertEqualStmt(stmt,(int)timefield.minute,0);
	assertEqualStmt(stmt,(int)timefield.second,0);
	erg=SQLGetData(stmt,11,SQL_C_TYPE_TIMESTAMP,
			&datetimefield,sizeof(datetimefield),&datetimeind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)datetimefield.year,2002);
	assertEqualStmt(stmt,(int)datetimefield.month,1);
	assertEqualStmt(stmt,(int)datetimefield.day,1);
	assertEqualStmt(stmt,(int)datetimefield.hour,2);
	erg=SQLGetData(stmt,12,SQL_C_SSHORT,
			&yearfield,sizeof(yearfield),&yearind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)yearfield,2002);
	erg=SQLGetData(stmt,13,SQL_C_CHAR,
			charfield,sizeof(charfield),&charind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)charfield,"char2");
	erg=SQLGetData(stmt,14,SQL_C_CHAR,
			varcharfield,sizeof(varcharfield),&varcharind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)varcharfield,"varchar2");
	erg=SQLGetData(stmt,15,SQL_C_CHAR,
			textfield,sizeof(textfield),&textind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)textfield,"text2");
	erg=SQLGetData(stmt,16,SQL_C_CHAR,
			tinytextfield,sizeof(tinytextfield),&tinytextind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)tinytextfield,"tinytext2");
	erg=SQLGetData(stmt,17,SQL_C_CHAR,
			mediumtextfield,sizeof(mediumtextfield),&mediumtextind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)mediumtextfield,"mediumtext2");
	erg=SQLGetData(stmt,18,SQL_C_CHAR,
			longtextfield,sizeof(longtextfield),&longtextind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)longtextfield,"longtext2");
	erg=SQLGetData(stmt,19,SQL_C_BINARY,
			blobfield,sizeof(blobfield),&blobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)blobind,5);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"blob2",5));
	erg=SQLGetData(stmt,20,SQL_C_BINARY,
			tinyblobfield,sizeof(tinyblobfield),&tinyblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)tinyblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(tinyblobfield,"tinyblob2",9));
	erg=SQLGetData(stmt,21,SQL_C_BINARY,
			mediumblobfield,sizeof(mediumblobfield),&mediumblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)mediumblobind,11);
	assertTrueStmt(stmt,!bytestring::compare(mediumblobfield,"mediumblob2",11));
	erg=SQLGetData(stmt,22,SQL_C_BINARY,
			longblobfield,sizeof(longblobfield),&longblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)longblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(longblobfield,"longblob2",9));
	erg=SQLGetData(stmt,23,SQL_C_TYPE_TIMESTAMP,
			&timestampfield,sizeof(timestampfield),&timestampind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)timestampind,(int)SQL_NULL_DATA);

	// row 3
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetData(stmt,1,SQL_C_STINYINT,
			&tinyintfield,sizeof(tinyintfield),&tinyintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)tinyintfield,3);
	erg=SQLGetData(stmt,2,SQL_C_SSHORT,
			&smallintfield,sizeof(smallintfield),&smallintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)smallintfield,3);
	erg=SQLGetData(stmt,3,SQL_C_SLONG,
			&mediumintfield,sizeof(mediumintfield),&mediumintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)mediumintfield,3);
	erg=SQLGetData(stmt,4,SQL_C_SLONG,
			&intfield,sizeof(intfield),&intind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)intfield,3);
	erg=SQLGetData(stmt,5,SQL_C_SBIGINT,
			&bigintfield,sizeof(bigintfield),&bigintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)bigintfield,3);
	erg=SQLGetData(stmt,6,SQL_C_FLOAT,
			&floatfield,sizeof(floatfield),&floatind);
	assertSuccessStmt(stmt,erg);
	assertTrueStmt(stmt,floatfield==3.5);
	erg=SQLGetData(stmt,7,SQL_C_DOUBLE,
			&realfield,sizeof(realfield),&realind);
	assertSuccessStmt(stmt,erg);
	assertTrueStmt(stmt,realfield==3.5);
	erg=SQLGetData(stmt,8,SQL_C_CHAR,
			decimalfield,sizeof(decimalfield),&decimalind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)decimalfield,"3.5");
	erg=SQLGetData(stmt,9,SQL_C_TYPE_DATE,
			&datefield,sizeof(datefield),&dateind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)datefield.year,2003);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	erg=SQLGetData(stmt,10,SQL_C_TYPE_TIME,
			&timefield,sizeof(timefield),&timeind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)timefield.hour,3);
	assertEqualStmt(stmt,(int)timefield.minute,0);
	assertEqualStmt(stmt,(int)timefield.second,0);
	erg=SQLGetData(stmt,11,SQL_C_TYPE_TIMESTAMP,
			&datetimefield,sizeof(datetimefield),&datetimeind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)datetimefield.year,2003);
	assertEqualStmt(stmt,(int)datetimefield.month,1);
	assertEqualStmt(stmt,(int)datetimefield.day,1);
	assertEqualStmt(stmt,(int)datetimefield.hour,3);
	erg=SQLGetData(stmt,12,SQL_C_SSHORT,
			&yearfield,sizeof(yearfield),&yearind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)yearfield,2003);
	erg=SQLGetData(stmt,13,SQL_C_CHAR,
			charfield,sizeof(charfield),&charind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)charfield,"char3");
	erg=SQLGetData(stmt,14,SQL_C_CHAR,
			varcharfield,sizeof(varcharfield),&varcharind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)varcharfield,"varchar3");
	erg=SQLGetData(stmt,15,SQL_C_CHAR,
			textfield,sizeof(textfield),&textind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)textfield,"text3");
	erg=SQLGetData(stmt,16,SQL_C_CHAR,
			tinytextfield,sizeof(tinytextfield),&tinytextind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)tinytextfield,"tinytext3");
	erg=SQLGetData(stmt,17,SQL_C_CHAR,
			mediumtextfield,sizeof(mediumtextfield),&mediumtextind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)mediumtextfield,"mediumtext3");
	erg=SQLGetData(stmt,18,SQL_C_CHAR,
			longtextfield,sizeof(longtextfield),&longtextind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)longtextfield,"longtext3");
	erg=SQLGetData(stmt,19,SQL_C_BINARY,
			blobfield,sizeof(blobfield),&blobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)blobind,5);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"blob3",5));
	erg=SQLGetData(stmt,20,SQL_C_BINARY,
			tinyblobfield,sizeof(tinyblobfield),&tinyblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)tinyblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(tinyblobfield,"tinyblob3",9));
	erg=SQLGetData(stmt,21,SQL_C_BINARY,
			mediumblobfield,sizeof(mediumblobfield),&mediumblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)mediumblobind,11);
	assertTrueStmt(stmt,!bytestring::compare(mediumblobfield,"mediumblob3",11));
	erg=SQLGetData(stmt,22,SQL_C_BINARY,
			longblobfield,sizeof(longblobfield),&longblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)longblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(longblobfield,"longblob3",9));
	erg=SQLGetData(stmt,23,SQL_C_TYPE_TIMESTAMP,
			&timestampfield,sizeof(timestampfield),&timestampind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)timestampind,(int)SQL_NULL_DATA);

	// row 4
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetData(stmt,1,SQL_C_STINYINT,
			&tinyintfield,sizeof(tinyintfield),&tinyintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)tinyintfield,4);
	erg=SQLGetData(stmt,2,SQL_C_SSHORT,
			&smallintfield,sizeof(smallintfield),&smallintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)smallintfield,4);
	erg=SQLGetData(stmt,3,SQL_C_SLONG,
			&mediumintfield,sizeof(mediumintfield),&mediumintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)mediumintfield,4);
	erg=SQLGetData(stmt,4,SQL_C_SLONG,
			&intfield,sizeof(intfield),&intind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)intfield,4);
	erg=SQLGetData(stmt,5,SQL_C_SBIGINT,
			&bigintfield,sizeof(bigintfield),&bigintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)bigintfield,4);
	erg=SQLGetData(stmt,6,SQL_C_FLOAT,
			&floatfield,sizeof(floatfield),&floatind);
	assertSuccessStmt(stmt,erg);
	assertTrueStmt(stmt,floatfield==4.5);
	erg=SQLGetData(stmt,7,SQL_C_DOUBLE,
			&realfield,sizeof(realfield),&realind);
	assertSuccessStmt(stmt,erg);
	assertTrueStmt(stmt,realfield==4.5);
	erg=SQLGetData(stmt,8,SQL_C_CHAR,
			decimalfield,sizeof(decimalfield),&decimalind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)decimalfield,"4.5");
	erg=SQLGetData(stmt,9,SQL_C_TYPE_DATE,
			&datefield,sizeof(datefield),&dateind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)datefield.year,2004);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	erg=SQLGetData(stmt,10,SQL_C_TYPE_TIME,
			&timefield,sizeof(timefield),&timeind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)timefield.hour,4);
	assertEqualStmt(stmt,(int)timefield.minute,0);
	assertEqualStmt(stmt,(int)timefield.second,0);
	erg=SQLGetData(stmt,11,SQL_C_TYPE_TIMESTAMP,
			&datetimefield,sizeof(datetimefield),&datetimeind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)datetimefield.year,2004);
	assertEqualStmt(stmt,(int)datetimefield.month,1);
	assertEqualStmt(stmt,(int)datetimefield.day,1);
	assertEqualStmt(stmt,(int)datetimefield.hour,4);
	erg=SQLGetData(stmt,12,SQL_C_SSHORT,
			&yearfield,sizeof(yearfield),&yearind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)yearfield,2004);
	erg=SQLGetData(stmt,13,SQL_C_CHAR,
			charfield,sizeof(charfield),&charind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)charfield,"char4");
	erg=SQLGetData(stmt,14,SQL_C_CHAR,
			varcharfield,sizeof(varcharfield),&varcharind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)varcharfield,"varchar4");
	erg=SQLGetData(stmt,15,SQL_C_CHAR,
			textfield,sizeof(textfield),&textind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)textfield,"text4");
	erg=SQLGetData(stmt,16,SQL_C_CHAR,
			tinytextfield,sizeof(tinytextfield),&tinytextind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)tinytextfield,"tinytext4");
	erg=SQLGetData(stmt,17,SQL_C_CHAR,
			mediumtextfield,sizeof(mediumtextfield),&mediumtextind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)mediumtextfield,"mediumtext4");
	erg=SQLGetData(stmt,18,SQL_C_CHAR,
			longtextfield,sizeof(longtextfield),&longtextind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)longtextfield,"longtext4");
	erg=SQLGetData(stmt,19,SQL_C_BINARY,
			blobfield,sizeof(blobfield),&blobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)blobind,5);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"blob4",5));
	erg=SQLGetData(stmt,20,SQL_C_BINARY,
			tinyblobfield,sizeof(tinyblobfield),&tinyblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)tinyblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(tinyblobfield,"tinyblob4",9));
	erg=SQLGetData(stmt,21,SQL_C_BINARY,
			mediumblobfield,sizeof(mediumblobfield),&mediumblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)mediumblobind,11);
	assertTrueStmt(stmt,!bytestring::compare(mediumblobfield,"mediumblob4",11));
	erg=SQLGetData(stmt,22,SQL_C_BINARY,
			longblobfield,sizeof(longblobfield),&longblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)longblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(longblobfield,"longblob4",9));
	erg=SQLGetData(stmt,23,SQL_C_TYPE_TIMESTAMP,
			&timestampfield,sizeof(timestampfield),&timestampind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)timestampind,(int)SQL_NULL_DATA);

	// no more rows
	erg=SQLFetch(stmt);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NO_DATA);
	stdoutput.printf("\n");



	// nested selects
	// MySQL allows only one active result set per connection, so
	// nested selects can't be done natively; SQL Relay can't either
	// (even with buffer size 0) because lazy fetching leaves the outer
	// query's result set open while the inner query runs - skip the test



	// commit and rollback
	stdoutput.printf("COMMIT AND ROLLBACK: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);

	// open second connection and statement
	SQLHDBC		dbc2;
	SQLHSTMT	stmt2;
	erg=SQLAllocHandle(SQL_HANDLE_DBC,env,&dbc2);
	assertSuccessEnv(env,erg);
	SQLCHAR		outcstring2[1024];
	SQLSMALLINT	outcstringlen2;
	erg=SQLDriverConnect(dbc2,NULL,
			(SQLCHAR *)incstr.getString(),
			SQL_NTS,
			outcstring2,
			sizeof(outcstring2),
			&outcstringlen2,
			SQL_DRIVER_NOPROMPT);
	assertSuccessDbc(dbc2,erg);
	erg=SQLAllocHandle(SQL_HANDLE_STMT,dbc2,&stmt2);
	assertSuccessDbc(dbc2,erg);

	// autocommit-off on dbc2 so we can release its repeatable-read
	// snapshot (SQL_COMMIT) before each read; default isolation would
	// otherwise hide data dbc committed after dbc2's first read
	erg=SQLSetConnectAttr(dbc2,SQL_ATTR_AUTOCOMMIT,
		(SQLPOINTER)SQL_AUTOCOMMIT_OFF,0);
	assertSuccessDbc(dbc2,erg);

	// get row count (should be 0, dbc hasn't committed)
	SQLINTEGER	rowcount;
	SQLLEN		rowcountind;
	erg=SQLExecDirect(stmt2,(SQLCHAR *)
		"select count(*) from testtable",
		SQL_NTS);
	assertSuccessStmt(stmt2,erg);
	erg=SQLBindCol(stmt2,1,SQL_C_SLONG,
		&rowcount,sizeof(rowcount),&rowcountind);
	assertSuccessStmt(stmt2,erg);
	rowcount=-1;
	erg=SQLFetch(stmt2);
	assertSuccessStmt(stmt2,erg);
	assertEqualStmt(stmt2,(int)rowcount,0);

	// commit on dbc
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);

	// release dbc2's snapshot to start a new one
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc2,SQL_COMMIT);
	assertSuccessDbc(dbc2,erg);

	// get row count (should be 4)
	SQLFreeStmt(stmt2,SQL_CLOSE);
	SQLFreeStmt(stmt2,SQL_UNBIND);
	erg=SQLExecDirect(stmt2,(SQLCHAR *)
		"select count(*) from testtable",
		SQL_NTS);
	assertSuccessStmt(stmt2,erg);
	erg=SQLBindCol(stmt2,1,SQL_C_SLONG,
		&rowcount,sizeof(rowcount),&rowcountind);
	assertSuccessStmt(stmt2,erg);
	rowcount=-1;
	erg=SQLFetch(stmt2);
	assertSuccessStmt(stmt2,erg);
	assertEqualStmt(stmt2,(int)rowcount,4);

	// insert another row on dbc
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into "
		"	testtable "
		"values ("
		"	5, 5, 5, 5, 5, "
		"	5.5, 5.5, 5.5, "
		"	'2005-05-05', "
		"	'05:00:00', "
		"	'2005-05-05 05:00:00', "
		"	'2005', "
		"	'char5', 'varchar5', "
		"	'text5', 'tinytext5', "
		"	'mediumtext5', 'longtext5', "
		"	'blob5', 'tinyblob5', "
		"	'mediumblob5', 'longblob5', "
		"	NULL)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);

	// rollback on dbc
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_ROLLBACK);
	assertSuccessDbc(dbc,erg);

	// release dbc2's snapshot to start a new one
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc2,SQL_COMMIT);
	assertSuccessDbc(dbc2,erg);

	// get row count (should still be 4)
	SQLFreeStmt(stmt2,SQL_CLOSE);
	SQLFreeStmt(stmt2,SQL_UNBIND);
	erg=SQLExecDirect(stmt2,(SQLCHAR *)
		"select count(*) from testtable",
		SQL_NTS);
	assertSuccessStmt(stmt2,erg);
	erg=SQLBindCol(stmt2,1,SQL_C_SLONG,
		&rowcount,sizeof(rowcount),&rowcountind);
	assertSuccessStmt(stmt2,erg);
	rowcount=-1;
	erg=SQLFetch(stmt2);
	assertSuccessStmt(stmt2,erg);
	assertEqualStmt(stmt2,(int)rowcount,4);

	// switch dbc to autocommit ON; the next insert is auto-committed
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
		(SQLPOINTER)SQL_AUTOCOMMIT_ON,0);
	assertSuccessDbc(dbc,erg);

	// insert another row on dbc
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into "
		"	testtable "
		"values ("
		"	5, 5, 5, 5, 5, "
		"	5.5, 5.5, 5.5, "
		"	'2005-05-05', "
		"	'05:00:00', "
		"	'2005-05-05 05:00:00', "
		"	'2005', "
		"	'char5', 'varchar5', "
		"	'text5', 'tinytext5', "
		"	'mediumtext5', 'longtext5', "
		"	'blob5', 'tinyblob5', "
		"	'mediumblob5', 'longblob5', "
		"	NULL)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);

	// release dbc2's snapshot to start a new one
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc2,SQL_COMMIT);
	assertSuccessDbc(dbc2,erg);

	// get row count (should be 5)
	SQLFreeStmt(stmt2,SQL_CLOSE);
	SQLFreeStmt(stmt2,SQL_UNBIND);
	erg=SQLExecDirect(stmt2,(SQLCHAR *)
		"select count(*) from testtable",
		SQL_NTS);
	assertSuccessStmt(stmt2,erg);
	erg=SQLBindCol(stmt2,1,SQL_C_SLONG,
		&rowcount,sizeof(rowcount),&rowcountind);
	assertSuccessStmt(stmt2,erg);
	rowcount=-1;
	erg=SQLFetch(stmt2);
	assertSuccessStmt(stmt2,erg);
	assertEqualStmt(stmt2,(int)rowcount,5);

	// clean up and disconnect
	SQLFreeHandle(SQL_HANDLE_STMT,stmt2);
	SQLDisconnect(dbc2);
	SQLFreeHandle(SQL_HANDLE_DBC,dbc2);
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
		(SQLPOINTER)SQL_AUTOCOMMIT_OFF,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// column info - not null
	// (after the commit-and-rollback section; mysql DDL implicitly
	// commits, which would break that section's uncommitted-row counts)
	stdoutput.printf("COLUMN INFO - not null: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)
			"drop table if exists testtable2",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable2 ("
		"	col1 int not null, "
		"	col2 int)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into testtable2 values (1,1)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select * from testtable2",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLDescribeCol(stmt,1,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)nullable,SQL_NO_NULLS);
	erg=SQLDescribeCol(stmt,2,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
			"drop table testtable2",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// null values
	stdoutput.printf("NULL VALUES: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select NULL,1,NULL",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		nullfield1[10];
	SQLLEN		nullind1;
	SQLINTEGER	nvintfield;
	SQLLEN		nvintind;
	SQLCHAR		nullfield2[10];
	SQLLEN		nullind2;
	erg=SQLBindCol(stmt,1,SQL_C_CHAR,
			nullfield1,sizeof(nullfield1),&nullind1);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,2,SQL_C_SLONG,
			&nvintfield,sizeof(nvintfield),&nvintind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,3,SQL_C_CHAR,
			nullfield2,sizeof(nullfield2),&nullind2);
	assertSuccessStmt(stmt,erg);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)nullind1,(int)SQL_NULL_DATA);
	assertEqualStmt(stmt,(int)nvintfield,1);
	assertEqualStmt(stmt,(int)nullind2,(int)SQL_NULL_DATA);
	stdoutput.printf("\n");



	// null and empty lobs
	stdoutput.printf("NULL AND EMPTY LOBS: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable ("
		"	testtext1 text, "
		"	testtext2 text, "
		"	testblob1 blob, "
		"	testblob2 blob)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLPrepare(stmt,(SQLCHAR *)
		"insert into testtable values (?,?,?,?)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		emptylob[1]={0};
	SQLLEN		emptyloblen=0;
	SQLLEN		nullloblen=SQL_NULL_DATA;
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)emptylob,
				0,&emptyloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)emptylob,
				0,&nullloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				0,0,
				(SQLPOINTER)emptylob,
				0,&emptyloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,4,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				0,0,
				(SQLPOINTER)emptylob,
				0,&nullloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select * from testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		lobclob1[1024];
	SQLLEN		lobclob1ind;
	SQLCHAR		lobclob2[1024];
	SQLLEN		lobclob2ind;
	SQLCHAR		lobblob1[1024];
	SQLLEN		lobblob1ind;
	SQLCHAR		lobblob2[1024];
	SQLLEN		lobblob2ind;
	erg=SQLBindCol(stmt,1,SQL_C_CHAR,
			lobclob1,sizeof(lobclob1),&lobclob1ind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,2,SQL_C_CHAR,
			lobclob2,sizeof(lobclob2),&lobclob2ind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,3,SQL_C_BINARY,
			lobblob1,sizeof(lobblob1),&lobblob1ind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,4,SQL_C_BINARY,
			lobblob2,sizeof(lobblob2),&lobblob2ind);
	assertSuccessStmt(stmt,erg);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)lobclob1ind,0);
	assertEqualStmt(stmt,(int)lobclob2ind,(int)SQL_NULL_DATA);
	assertEqualStmt(stmt,(int)lobblob1ind,0);
	assertEqualStmt(stmt,(int)lobblob2ind,(int)SQL_NULL_DATA);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// long lobs (prepare, bind, execute)
	stdoutput.printf("LONG LOBS (prepare, bind, execute): \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable ("
		"	testtext longtext, "
		"	testblob longblob)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	erg=SQLPrepare(stmt,(SQLCHAR *)
		"insert into testtable values (?,?)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLLEN	largetextlen=LARGE_BUFFER_LENGTH;
	SQLLEN	largebloblen=LARGE_BUFFER_LENGTH;
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				LARGE_BUFFER_LENGTH,0,
				(SQLPOINTER)largebuffer,
				0,&largetextlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				LARGE_BUFFER_LENGTH,0,
				(SQLPOINTER)largebuffer,
				0,&largebloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"select * from testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR	largetextout[LARGE_BUFFER_LENGTH+1];
	SQLLEN	largetextind;
	SQLCHAR	largeblobout[LARGE_BUFFER_LENGTH+1];
	SQLLEN	largeblobind;
	erg=SQLBindCol(stmt,1,SQL_C_CHAR,
			largetextout,sizeof(largetextout),&largetextind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,2,SQL_C_BINARY,
			largeblobout,sizeof(largeblobout),&largeblobind);
	assertSuccessStmt(stmt,erg);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)largetextind,LARGE_BUFFER_LENGTH);
	assertTrueStmt(stmt,
		!bytestring::compare(largetextout,largebuffer,
						LARGE_BUFFER_LENGTH));
	assertEqualStmt(stmt,(int)largeblobind,LARGE_BUFFER_LENGTH);
	assertTrueStmt(stmt,
		!bytestring::compare(largeblobout,largebuffer,
						LARGE_BUFFER_LENGTH));
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// long lobs (prepare, bind, execute, putdata)
	stdoutput.printf("LONG LOBS (prepare, bind, execute, putdata): \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable ("
		"	testtext longtext, "
		"	testblob longblob)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='D';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	erg=SQLPrepare(stmt,(SQLCHAR *)
		"insert into testtable values (?,?)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLLEN	largetextdaelen=SQL_LEN_DATA_AT_EXEC(LARGE_BUFFER_LENGTH);
	SQLLEN	largeblobdaelen=SQL_LEN_DATA_AT_EXEC(LARGE_BUFFER_LENGTH);
	const char	*texttoken="largetext";
	const char	*blobtoken="largeblob";
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				LARGE_BUFFER_LENGTH,0,
				(SQLPOINTER)texttoken,
				0,&largetextdaelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				LARGE_BUFFER_LENGTH,0,
				(SQLPOINTER)blobtoken,
				0,&largeblobdaelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NEED_DATA);
	SQLPOINTER	largetoken=NULL;
	erg=SQLParamData(stmt,&largetoken);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NEED_DATA);
	assertEqualStmt(stmt,(const char *)largetoken,texttoken);
	for (int i=0; i<4; i++) {
		erg=SQLPutData(stmt,
				largebuffer+i*LARGE_CHUNK_LENGTH,
				LARGE_CHUNK_LENGTH);
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLParamData(stmt,&largetoken);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NEED_DATA);
	assertEqualStmt(stmt,(const char *)largetoken,blobtoken);
	for (int i=0; i<4; i++) {
		erg=SQLPutData(stmt,
				largebuffer+i*LARGE_CHUNK_LENGTH,
				LARGE_CHUNK_LENGTH);
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLParamData(stmt,&largetoken);
	assertSuccessStmt(stmt,erg);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"select * from testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	// re-init output buffers
	bytestring::zero(largetextout,sizeof(largetextout));
	largetextind=-1;
	bytestring::zero(largeblobout,sizeof(largeblobout));
	largeblobind=-1;
	erg=SQLBindCol(stmt,1,SQL_C_CHAR,
			largetextout,sizeof(largetextout),&largetextind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,2,SQL_C_BINARY,
			largeblobout,sizeof(largeblobout),&largeblobind);
	assertSuccessStmt(stmt,erg);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)largetextind,LARGE_BUFFER_LENGTH);
	assertTrueStmt(stmt,
		!bytestring::compare(largetextout,largebuffer,
					LARGE_BUFFER_LENGTH));
	assertEqualStmt(stmt,(int)largeblobind,LARGE_BUFFER_LENGTH);
	assertTrueStmt(stmt,
		!bytestring::compare(largeblobout,largebuffer,
					LARGE_BUFFER_LENGTH));
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// output bind by position
	// mysql doesn't support output binds


	// lob output bind
	// mysql doesn't support output binds


	// long output bind
	// mysql doesn't support output binds



	// negative input bind
	stdoutput.printf("NEGATIVE INPUT BIND: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable (testval int)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLPrepare(stmt,(SQLCHAR *)
		"insert into testtable values (?)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLINTEGER	negval=-1;
	SQLLEN		negvallen=sizeof(negval);
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&negval,
				negvallen,&negvallen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select testval from testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLINTEGER	negoutval=0;
	SQLLEN		negoutind=0;
	erg=SQLBindCol(stmt,1,SQL_C_SLONG,
			&negoutval,sizeof(negoutval),&negoutind);
	assertSuccessStmt(stmt,erg);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)negoutval,-1);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// bind before prepare
	stdoutput.printf("BIND BEFORE PREPARE: \n");

	// bind, prepare, execute
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLINTEGER	bbpinval=42;
	SQLLEN		bbpinind=0;
	SQLCHAR		bbpoutval[16]={0};
	SQLLEN		bbpoutind=0;
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&bbpinval,
				bbpinind,&bbpinind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,1,SQL_C_CHAR,
			bbpoutval,sizeof(bbpoutval),&bbpoutind);
	assertSuccessStmt(stmt,erg);
	erg=SQLPrepare(stmt,(SQLCHAR *)"select cast(? as char)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)bbpoutval,"42");

	// bind, exec-direct
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	bbpinval=99;
	bytestring::zero(bbpoutval,sizeof(bbpoutval));
	bbpoutind=0;
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&bbpinval,
				bbpinind,&bbpinind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,1,SQL_C_CHAR,
			bbpoutval,sizeof(bbpoutval),&bbpoutind);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"select cast(? as char)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)bbpoutval,"99");
	stdoutput.printf("\n");



	// rebinding
	stdoutput.printf("REBINDING: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLPrepare(stmt,(SQLCHAR *)"select ?",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLINTEGER	rebindin=1;
	SQLLEN		rebindinlen=sizeof(rebindin);
	SQLINTEGER	rebindout=0;
	SQLLEN		rebindoutind=0;
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&rebindin,
				rebindinlen,&rebindinlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,1,SQL_C_SLONG,
			&rebindout,sizeof(rebindout),&rebindoutind);
	assertSuccessStmt(stmt,erg);
	for (int rb=1; rb<=3; rb++) {
		rebindin=rb;
		erg=SQLExecute(stmt);
		assertSuccessStmt(stmt,erg);
		erg=SQLFetch(stmt);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)rebindout,rb);
		SQLCloseCursor(stmt);
	}
	stdoutput.printf("\n");



	// reexecute
	stdoutput.printf("REEXECUTE: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLPrepare(stmt,(SQLCHAR *)"select 1",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLINTEGER	reexval=0;
	SQLLEN		reexind=0;
	erg=SQLBindCol(stmt,1,SQL_C_SLONG,
			&reexval,sizeof(reexval),&reexind);
	assertSuccessStmt(stmt,erg);
	reexval=0;
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)reexval,1);
	SQLCloseCursor(stmt);
	reexval=0;
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)reexval,1);
	SQLCloseCursor(stmt);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLPrepare(stmt,(SQLCHAR *)"select ?",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLINTEGER	reexbind=1;
	SQLLEN		reexbindlen=sizeof(reexbind);
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&reexbind,
				reexbindlen,&reexbindlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,1,SQL_C_SLONG,
			&reexval,sizeof(reexval),&reexind);
	assertSuccessStmt(stmt,erg);
	reexval=0;
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)reexval,1);
	SQLCloseCursor(stmt);
	reexval=0;
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)reexval,1);
	SQLCloseCursor(stmt);
	reexbind=2;
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)reexval,2);
	stdoutput.printf("\n");



	// bind survives SQLFreeStmt(SQL_CLOSE)
	stdoutput.printf("BIND SURVIVES SQL_CLOSE: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable (testval int)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLPrepare(stmt,(SQLCHAR *)
		"insert into testtable values (?)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLINTEGER	bscval=100;
	SQLLEN		bscind=0;
	// bind once
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&bscval,
				0,&bscind);
	assertSuccessStmt(stmt,erg);
	// execute with bscval=100
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	// SQL_CLOSE only - no SQL_UNBIND, no SQL_RESET_PARAMS
	erg=SQLFreeStmt(stmt,SQL_CLOSE);
	assertSuccessStmt(stmt,erg);
	// change the host variable; binding should still point at it
	bscval=200;
	// execute again WITHOUT rebinding
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	// read back both rows in ascending order
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select testval from testtable order by testval",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLINTEGER	bscread=0;
	SQLLEN		bscreadind=0;
	erg=SQLBindCol(stmt,1,SQL_C_SLONG,
			(SQLPOINTER)&bscread,0,&bscreadind);
	assertSuccessStmt(stmt,erg);
	// row 1: 100
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)bscread,100);
	// row 2: 200 if the binding survived SQL_CLOSE
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)bscread,200);
	// no row 3
	erg=SQLFetch(stmt);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NO_DATA);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// leftover bind ignored
	stdoutput.printf("LEFTOVER BIND IGNORED: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable (col1 int, col2 int)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	// bind two parameters
	SQLINTEGER	lbi1=11;
	SQLINTEGER	lbi2=22;
	SQLLEN		lbi1ind=0;
	SQLLEN		lbi2ind=0;
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,(SQLPOINTER)&lbi1,0,&lbi1ind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,(SQLPOINTER)&lbi2,0,&lbi2ind);
	assertSuccessStmt(stmt,erg);
	// execute a statement with two markers
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into testtable values (?,?)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	// SQL_CLOSE only - leftover binds stay on the statement handle
	erg=SQLFreeStmt(stmt,SQL_CLOSE);
	assertSuccessStmt(stmt,erg);
	// parameterless statement without clearing binds; leftover
	// bindings must not be sent to the backend
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into testtable values (33,44)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLFreeStmt(stmt,SQL_CLOSE);
	// fewer-but-nonzero: binds 1 and 2 stashed but only one marker;
	// only bind 1 should apply
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into testtable values (?,99)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	// read back: rows should be (11,22), (11,99), (33,44)
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select col1, col2 from testtable order by col1, col2",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLINTEGER	lbr1=0;
	SQLINTEGER	lbr2=0;
	SQLLEN		lbr1ind=0;
	SQLLEN		lbr2ind=0;
	erg=SQLBindCol(stmt,1,SQL_C_SLONG,(SQLPOINTER)&lbr1,0,&lbr1ind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,2,SQL_C_SLONG,(SQLPOINTER)&lbr2,0,&lbr2ind);
	assertSuccessStmt(stmt,erg);
	// row 1: 11,22
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)lbr1,11);
	assertEqualStmt(stmt,(int)lbr2,22);
	// row 2: 11,99
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)lbr1,11);
	assertEqualStmt(stmt,(int)lbr2,99);
	// row 3: 33,44
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)lbr1,33);
	assertEqualStmt(stmt,(int)lbr2,44);
	// no row 4
	erg=SQLFetch(stmt);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NO_DATA);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// encoded binary data
	stdoutput.printf("ENCODED BINARY DATA: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable (col1 blob)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	unsigned char	encbuf[256];
	for (int i=0; i<256; i++) {
		encbuf[i]=(unsigned char)i;
	}
	erg=SQLPrepare(stmt,(SQLCHAR *)
		"insert into testtable values (?)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLLEN	enclen=sizeof(encbuf);
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				sizeof(encbuf),0,
				(SQLPOINTER)encbuf,
				0,&enclen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select col1 from testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	unsigned char	encout[sizeof(encbuf)]={0};
	SQLLEN		encoutind=0;
	erg=SQLBindCol(stmt,1,SQL_C_BINARY,
			encout,sizeof(encout),&encoutind);
	assertSuccessStmt(stmt,erg);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)encoutind,(int)sizeof(encbuf));
	assertTrueStmt(stmt,
		!bytestring::compare(encout,encbuf,sizeof(encbuf)));
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// quotes
	stdoutput.printf("QUOTES: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable (col1 varchar(4))",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into testtable values ('''''')",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select col1 from testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR	quotesout[8]={0};
	SQLLEN	quotesind=0;
	erg=SQLBindCol(stmt,1,SQL_C_CHAR,
			quotesout,sizeof(quotesout),&quotesind);
	assertSuccessStmt(stmt,erg);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)quotesind,2);
	assertEqualStmt(stmt,(const char *)quotesout,"''");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// catalog list
	stdoutput.printf("CATALOG LIST: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLTables(stmt,
			(SQLCHAR *)SQL_ALL_CATALOGS,SQL_NTS,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)"",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		catname[1024];
	SQLLEN		catnameind;
	erg=SQLBindCol(stmt,1,SQL_C_CHAR,
			catname,sizeof(catname),&catnameind);
	assertSuccessStmt(stmt,erg);
	int		catrows=0;
	bool		catfound=false;
	bool		catdeffound=false;
	for (;;) {
		erg=SQLFetch(stmt);
		if (erg==SQL_NO_DATA) {
			break;
		}
		assertSuccessStmt(stmt,erg);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			break;
		}
		if (!charstring::compareIgnoringCase(
					(const char *)catname,hostname)) {
			catfound=true;
		}
		if (!charstring::compare((const char *)catname,"def")) {
			catdeffound=true;
		}
		catrows++;
	}
	// Through sqlrelay the list follows information_schema's model -
	// the lone catalog is "def" and the databases are schemas.  The
	// native driver treats databases as catalogs instead; the connected
	// database, named after the host, appears in the list.
	if (issqlrelay) {
		assertEqualStmt(stmt,catrows,1);
		assertTrueStmt(stmt,catdeffound);
	} else {
		assertTrueStmt(stmt,catfound);
	}
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	stdoutput.printf("\n");



	// schema list
	stdoutput.printf("SCHEMA LIST: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLTables(stmt,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)SQL_ALL_SCHEMAS,SQL_NTS,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)"",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		schname[1024];
	SQLLEN		schnameind;
	erg=SQLBindCol(stmt,2,SQL_C_CHAR,
			schname,sizeof(schname),&schnameind);
	assertSuccessStmt(stmt,erg);
	int		schrows=0;
	bool		schfound=false;
	for (;;) {
		erg=SQLFetch(stmt);
		if (erg==SQL_NO_DATA) {
			break;
		}
		assertSuccessStmt(stmt,erg);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			break;
		}
		if (!charstring::compareIgnoringCase(
					(const char *)schname,hostname)) {
			schfound=true;
		}
		schrows++;
	}
	if (issqlrelay) {
		// mysql databases are schemas through sqlrelay; the connected
		// database, named after the host, should appear in the list
		assertTrueStmt(stmt,schfound);
	} else {
		// the native driver treats databases as catalogs and
		// has no schemas; the list is empty
		assertEqualStmt(stmt,schrows,0);
	}
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	stdoutput.printf("\n");



	// table type list
	// MySQL ODBC reports TABLE, VIEW, SYSTEM TABLE
	stdoutput.printf("TABLE TYPE LIST: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLTables(stmt,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)SQL_ALL_TABLE_TYPES,SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		tabletype[64];
	SQLLEN		tabletypeind;
	erg=SQLBindCol(stmt,4,SQL_C_CHAR,
			tabletype,sizeof(tabletype),&tabletypeind);
	assertSuccessStmt(stmt,erg);
	bool		foundtable=false;
	bool		foundview=false;
	for (;;) {
		erg=SQLFetch(stmt);
		if (erg==SQL_NO_DATA) {
			break;
		}
		assertSuccessStmt(stmt,erg);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			break;
		}
		if (!charstring::compare((const char *)tabletype,"TABLE")) {
			foundtable=true;
		} else if (!charstring::compare(
					(const char *)tabletype,"VIEW")) {
			foundview=true;
		}
	}
	assertTrueStmt(stmt,foundtable);
	assertTrueStmt(stmt,foundview);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	stdoutput.printf("\n");



	// table list
	stdoutput.printf("TABLE LIST: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable1",SQL_NTS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable2",SQL_NTS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable3",SQL_NTS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable4",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable1 (col1 int, col2 int)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable2 (col1 int, col2 int)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable3 (col1 int, col2 int)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable4 (col1 int, col2 int)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	// commit so the catalog query sees the new tables
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);
	// force metadata_id off so the table-name pattern is honored;
	// MariaDB embeds catalog/schema as a non-quoted identifier when true
	SQLSetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)(uintptr_t)SQL_FALSE,0);
	// MariaDB returns no rows for empty-string catalog/schema; pass NULL.
	// SQL Relay matches across the connected database with empty strings.
	// FIXME: which is correct? (other list tests do the same)
	SQLCHAR		*tlcatalog=(SQLCHAR *)(issqlrelay?"":NULL);
	SQLSMALLINT	tlcataloglen=(issqlrelay?SQL_NTS:0);
	SQLCHAR		*tlschema=(SQLCHAR *)(issqlrelay?"":NULL);
	SQLSMALLINT	tlschemalen=(issqlrelay?SQL_NTS:0);
	erg=SQLTables(stmt,
			tlcatalog,tlcataloglen,
			tlschema,tlschemalen,
			(SQLCHAR *)"testtable%",SQL_NTS,
			(SQLCHAR *)"TABLE",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		tblname[256];
	SQLLEN		tblnameind;
	erg=SQLBindCol(stmt,3,SQL_C_CHAR,
			tblname,sizeof(tblname),&tblnameind);
	assertSuccessStmt(stmt,erg);
	int		tblcounter=0;
	for (;;) {
		erg=SQLFetch(stmt);
		if (erg==SQL_NO_DATA) {
			break;
		}
		assertSuccessStmt(stmt,erg);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			break;
		}
		if (!charstring::compare((const char *)tblname,"testtable1") ||
			!charstring::compare(
					(const char *)tblname,"testtable2") ||
			!charstring::compare(
					(const char *)tblname,"testtable3") ||
			!charstring::compare(
					(const char *)tblname,"testtable4")) {
			tblcounter++;
		}
	}
	assertEqualStmt(stmt,tblcounter,4);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	for (int t=1; t<=4; t++) {
		char dropbuf[64];
		charstring::printf(dropbuf,sizeof(dropbuf),
					"drop table testtable%d",t);
		erg=SQLExecDirect(stmt,(SQLCHAR *)dropbuf,SQL_NTS);
		assertSuccessStmt(stmt,erg);
	}
	stdoutput.printf("\n");



	// type info list
	// walk the result set for INT, VARCHAR, CHAR, DATE, TEXT, BLOB
	stdoutput.printf("TYPE INFO LIST: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLGetTypeInfo(stmt,SQL_ALL_TYPES);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		typname[64];
	SQLLEN		typnameind;
	erg=SQLBindCol(stmt,1,SQL_C_CHAR,
			typname,sizeof(typname),&typnameind);
	assertSuccessStmt(stmt,erg);
	bool		foundint=false;
	bool		foundvarchar=false;
	bool		foundchar=false;
	bool		founddate=false;
	bool		foundtext=false;
	bool		foundblob=false;
	for (;;) {
		erg=SQLFetch(stmt);
		if (erg==SQL_NO_DATA) {
			break;
		}
		assertSuccessStmt(stmt,erg);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			break;
		}
		if (!charstring::compareIgnoringCase(
					(const char *)typname,"INT")) {
			foundint=true;
		} else if (!charstring::compareIgnoringCase(
					(const char *)typname,"VARCHAR")) {
			foundvarchar=true;
		} else if (!charstring::compareIgnoringCase(
					(const char *)typname,"CHAR")) {
			foundchar=true;
		} else if (!charstring::compareIgnoringCase(
					(const char *)typname,"DATE")) {
			founddate=true;
		} else if (!charstring::compareIgnoringCase(
					(const char *)typname,"TEXT")) {
			foundtext=true;
		} else if (!charstring::compareIgnoringCase(
					(const char *)typname,"BLOB")) {
			foundblob=true;
		}
	}
	assertTrueStmt(stmt,foundint);
	assertTrueStmt(stmt,foundvarchar);
	assertTrueStmt(stmt,foundchar);
	assertTrueStmt(stmt,founddate);
	assertTrueStmt(stmt,foundtext);
	assertTrueStmt(stmt,foundblob);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	stdoutput.printf("\n");



	// column list
	stdoutput.printf("COLUMN LIST: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable ("
		"	testtinyint tinyint, "
		"	testsmallint smallint, "
		"	testmediumint mediumint, "
		"	testint int, "
		"	testbigint bigint, "
		"	testfloat float, "
		"	testreal real, "
		"	testdecimal decimal(2,1), "
		"	testdate date, "
		"	testtime time, "
		"	testdatetime datetime, "
		"	testyear year, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testtext text, "
		"	testtinytext tinytext, "
		"	testmediumtext mediumtext, "
		"	testlongtext longtext, "
		"	testblob blob, "
		"	testtinyblob tinyblob, "
		"	testmediumblob mediumblob, "
		"	testlongblob longblob, "
		"	testtimestamp timestamp null)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);
	// MariaDB raises "Unknown column" for a non-NULL catalog; pass NULL
	// native, empty string for SQL Relay. Also force metadata_id off so
	// the table name is a pattern.
	SQLCHAR		*catalogfilter=(SQLCHAR *)(issqlrelay?"":NULL);
	SQLSMALLINT	catalogfilterlen=(issqlrelay?SQL_NTS:0);
	SQLSetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)(uintptr_t)SQL_FALSE,0);
	erg=SQLColumns(stmt,
			catalogfilter,catalogfilterlen,
			NULL,0,
			(SQLCHAR *)"testtable",SQL_NTS,
			NULL,0);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		clcolname[64];
	SQLLEN		clcolnameind;
	erg=SQLBindCol(stmt,4,SQL_C_CHAR,
			clcolname,sizeof(clcolname),&clcolnameind);
	assertSuccessStmt(stmt,erg);
	const char	*clexpcolnames[]={
		"testtinyint","testsmallint","testmediumint","testint",
		"testbigint","testfloat","testreal","testdecimal",
		"testdate","testtime","testdatetime","testyear",
		"testchar","testvarchar","testtext","testtinytext",
		"testmediumtext","testlongtext","testblob","testtinyblob",
		"testmediumblob","testlongblob","testtimestamp"};
	for (int c=0; c<23; c++) {
		erg=SQLFetch(stmt);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(const char *)clcolname,clexpcolnames[c]);
	}
	erg=SQLFetch(stmt);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NO_DATA);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// column list - auto_increment, primary key
	stdoutput.printf("COLUMN LIST - auto_increment, primary key: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);
	erg=SQLPrimaryKeys(stmt,
			catalogfilter,catalogfilterlen,
			NULL,0,
			(SQLCHAR *)"testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		pkcolname[64];
	SQLLEN		pkcolnameind;
	erg=SQLBindCol(stmt,4,SQL_C_CHAR,
			pkcolname,sizeof(pkcolname),&pkcolnameind);
	assertSuccessStmt(stmt,erg);
	bool		foundcol1=false;
	bool		foundcol2=false;
	for (;;) {
		erg=SQLFetch(stmt);
		if (erg==SQL_NO_DATA) {
			break;
		}
		assertSuccessStmt(stmt,erg);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			break;
		}
		if (!charstring::compare((const char *)pkcolname,"col1")) {
			foundcol1=true;
		} else if (!charstring::compare(
					(const char *)pkcolname,"col2")) {
			foundcol2=true;
		}
	}
	assertTrueStmt(stmt,foundcol1);
	assertFalseStmt(stmt,foundcol2);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// primary keys list
	stdoutput.printf("PRIMARY KEYS LIST: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);
	erg=SQLPrimaryKeys(stmt,
			catalogfilter,catalogfilterlen,
			NULL,0,
			(SQLCHAR *)"testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		pkname[64];
	SQLLEN		pknameind;
	SQLSMALLINT	pkseq;
	SQLLEN		pkseqind;
	SQLCHAR		pktable[64];
	SQLLEN		pktableind;
	SQLCHAR		pkcol[64];
	SQLLEN		pkcolind;
	erg=SQLBindCol(stmt,3,SQL_C_CHAR,
			pktable,sizeof(pktable),&pktableind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,4,SQL_C_CHAR,
			pkcol,sizeof(pkcol),&pkcolind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,5,SQL_C_SHORT,&pkseq,sizeof(pkseq),&pkseqind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,6,SQL_C_CHAR,
			pkname,sizeof(pkname),&pknameind);
	assertSuccessStmt(stmt,erg);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)pktable,"testtable");
	assertEqualStmt(stmt,(const char *)pkcol,"col1");
	assertEqualStmt(stmt,(int)pkseq,1);
	assertEqualStmt(stmt,(int)pknameind,7);
	assertEqualStmt(stmt,(const char *)pkname,"PRIMARY");
	erg=SQLFetch(stmt);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NO_DATA);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// key and index list
	stdoutput.printf("KEY AND INDEX LIST: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable ("
		"	col1 int primary key, "
		"	col2 int)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);
	erg=SQLStatistics(stmt,
			catalogfilter,catalogfilterlen,
			NULL,0,
			(SQLCHAR *)"testtable",SQL_NTS,
			SQL_INDEX_ALL,SQL_QUICK);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		idxtable[64];
	SQLLEN		idxtableind;
	SQLSMALLINT	idxnonunique;
	SQLLEN		idxnonuniqueind;
	SQLSMALLINT	idxseq;
	SQLLEN		idxseqind;
	SQLCHAR		idxcol[64];
	SQLLEN		idxcolind;
	SQLSMALLINT	idxtype;
	SQLLEN		idxtypeind;
	erg=SQLBindCol(stmt,3,SQL_C_CHAR,
			idxtable,sizeof(idxtable),&idxtableind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,4,SQL_C_SHORT,
			&idxnonunique,sizeof(idxnonunique),&idxnonuniqueind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,7,SQL_C_SHORT,
			&idxtype,sizeof(idxtype),&idxtypeind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,8,SQL_C_SHORT,
			&idxseq,sizeof(idxseq),&idxseqind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,9,SQL_C_CHAR,
			idxcol,sizeof(idxcol),&idxcolind);
	assertSuccessStmt(stmt,erg);
	bool	foundidxcol1=false;
	for (;;) {
		erg=SQLFetch(stmt);
		if (erg==SQL_NO_DATA) {
			break;
		}
		assertSuccessStmt(stmt,erg);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			break;
		}
		if (idxtype==SQL_TABLE_STAT) {
			continue;
		}
		assertEqualStmt(stmt,(const char *)idxtable,"testtable");
		assertEqualStmt(stmt,(int)idxnonunique,0);
		assertEqualStmt(stmt,(int)idxseq,1);
		if (!charstring::compare((const char *)idxcol,"col1")) {
			foundidxcol1=true;
		}
	}
	assertTrueStmt(stmt,foundidxcol1);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// procedure list
	stdoutput.printf("PROCEDURE LIST: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)
			"drop procedure if exists testproc1",SQL_NTS);
	SQLExecDirect(stmt,(SQLCHAR *)
			"drop procedure if exists testproc2",SQL_NTS);
	SQLExecDirect(stmt,(SQLCHAR *)
			"drop procedure if exists testproc3",SQL_NTS);
	SQLExecDirect(stmt,(SQLCHAR *)
			"drop procedure if exists testproc4",SQL_NTS);
	// 'create procedure ... begin end'; no `or replace`, no trailing
	// `;` (some clients reject it)
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create procedure testproc1 ("
		"	in in1 int, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) "
		"begin end",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create procedure testproc2 ("
		"	in in1 int, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) "
		"begin end",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create procedure testproc3 ("
		"	in in1 int, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) "
		"begin end",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create procedure testproc4 ("
		"	in in1 int, "
		"	in in2 char(20), "
		"	in in3 varchar(20), "
		"	in in4 date) "
		"begin end",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);
	// force metadata_id off so the procedure name pattern is honored
	SQLSetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)(uintptr_t)SQL_FALSE,0);
	erg=SQLProcedures(stmt,
			catalogfilter,catalogfilterlen,
			(SQLCHAR *)(issqlrelay?"":NULL),
			(issqlrelay?SQL_NTS:0),
			(SQLCHAR *)"testproc%",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		procname[64];
	SQLLEN		procnameind;
	erg=SQLBindCol(stmt,3,SQL_C_CHAR,
			procname,sizeof(procname),&procnameind);
	assertSuccessStmt(stmt,erg);
	int		proccounter=0;
	for (;;) {
		erg=SQLFetch(stmt);
		if (erg==SQL_NO_DATA) {
			break;
		}
		assertSuccessStmt(stmt,erg);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			break;
		}
		if (!charstring::compare(
			(const char *)procname,"testproc1") ||
			!charstring::compare(
				(const char *)procname,"testproc2") ||
			!charstring::compare(
				(const char *)procname,"testproc3") ||
			!charstring::compare(
				(const char *)procname,"testproc4")) {
			proccounter++;
		}
	}
	assertEqualStmt(stmt,proccounter,4);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	stdoutput.printf("\n");



	// procedure parameter list
	// types reported uppercase INT/CHAR/VARCHAR/DATE,
	// parameter mode 1 (SQL_PARAM_INPUT)
	stdoutput.printf("PROCEDURE PARAMETER LIST: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLSetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)(uintptr_t)SQL_FALSE,0);
	erg=SQLProcedureColumns(stmt,
			catalogfilter,catalogfilterlen,
			(SQLCHAR *)(issqlrelay?"":NULL),
			(issqlrelay?SQL_NTS:0),
			(SQLCHAR *)"testproc1",SQL_NTS,
			(SQLCHAR *)(issqlrelay?"":NULL),
			(issqlrelay?SQL_NTS:0));
	assertSuccessStmt(stmt,erg);
	SQLCHAR		ppname[64];
	SQLLEN		ppnameind;
	SQLSMALLINT	ppmode;
	SQLLEN		ppmodeind;
	SQLCHAR		pptypename[64];
	SQLLEN		pptypenameind;
	SQLSMALLINT	ppordinal;
	SQLLEN		ppordinalind;
	erg=SQLBindCol(stmt,4,SQL_C_CHAR,
			ppname,sizeof(ppname),&ppnameind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,5,SQL_C_SHORT,
			&ppmode,sizeof(ppmode),&ppmodeind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,7,SQL_C_CHAR,
			pptypename,sizeof(pptypename),&pptypenameind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,18,SQL_C_SHORT,
			&ppordinal,sizeof(ppordinal),&ppordinalind);
	assertSuccessStmt(stmt,erg);
	// SQL Relay reports type names upper case; MariaDB lower.
	// in1
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)ppname,"in1");
	assertEqualStmt(stmt,(int)ppmode,(int)SQL_PARAM_INPUT);
	if (issqlrelay) {
		assertEqualStmt(stmt,(const char *)pptypename,"INT");
	} else {
		assertEqualStmt(stmt,(const char *)pptypename,"int");
	}
	assertEqualStmt(stmt,(int)ppordinal,1);
	// in2
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)ppname,"in2");
	assertEqualStmt(stmt,(int)ppmode,(int)SQL_PARAM_INPUT);
	if (issqlrelay) {
		assertEqualStmt(stmt,(const char *)pptypename,"CHAR");
	} else {
		assertEqualStmt(stmt,(const char *)pptypename,"char");
	}
	assertEqualStmt(stmt,(int)ppordinal,2);
	// in3
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)ppname,"in3");
	assertEqualStmt(stmt,(int)ppmode,(int)SQL_PARAM_INPUT);
	if (issqlrelay) {
		assertEqualStmt(stmt,(const char *)pptypename,"VARCHAR");
	} else {
		assertEqualStmt(stmt,(const char *)pptypename,"varchar");
	}
	assertEqualStmt(stmt,(int)ppordinal,3);
	// in4
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)ppname,"in4");
	assertEqualStmt(stmt,(int)ppmode,(int)SQL_PARAM_INPUT);
	if (issqlrelay) {
		assertEqualStmt(stmt,(const char *)pptypename,"DATE");
	} else {
		assertEqualStmt(stmt,(const char *)pptypename,"date");
	}
	assertEqualStmt(stmt,(int)ppordinal,4);
	// no more rows
	erg=SQLFetch(stmt);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NO_DATA);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc1",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc2",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc3",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc4",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// invalid queries
	// (testtable was dropped above; references to it should fail)
	stdoutput.printf("INVALID QUERIES: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testint",
		SQL_NTS);
	assertEqualStmt(stmt,(int)erg,(int)SQL_ERROR);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testint",
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
