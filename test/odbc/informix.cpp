// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/sys.h>
#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/stringbuffer.h>
#include <rudiments/environment.h>

#include "../../config.h"

#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>
#include <sqltypes.h>

#include "asserts.cpp"

static bool	issqlrelay=false;

#ifndef _WIN32
	// The informix ODBC driver's SQLGetDiagRecW, SQLGetDiagFieldW, and
	// SQLErrorW use 4-byte SQLWCHAR, but unixODBC uses a 2-byte SQLWCHAR,
	// so unixODBC crashes when attempting to cache the results of those
	// methods.  There's no obvious way to disable caching, or tell unixODBC
	// to use 4-bytes SQLWCHAR.  So, override dlsym() to hide these methods
	// and cause unixODBC to fall back to the ascii versions and just avoid
	// the issue altogether.

	// for RTLD_NEXT
	#ifndef _GNU_SOURCE
		#define _GNU_SOURCE
	#endif
	// for dlvsym()
	#include <dlfcn.h>
	extern "C" void *dlsym(void *handle, const char *name) {
		static void	*(*realdlsym)(void *, const char *)=NULL;
		if (!realdlsym) {
			realdlsym=(void *(*)(void *, const char *))
					dlvsym(RTLD_NEXT,"dlsym","GLIBC_2.2.5");
		}
		if (!issqlrelay &&
				(!charstring::compare(name,"SQLGetDiagRecW") ||
				!charstring::compare(name,"SQLGetDiagFieldW") ||
				!charstring::compare(name,"SQLErrorW"))) {
			return NULL;
		}
		return realdlsym(handle,name);
	}
#endif

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
	issqlrelay=!(argc==2 && !charstring::compare(argv[1],"native"));

	// informix odbc needs these environment variables to be set
	if (!issqlrelay) {
		environment::setValue("INFORMIXDIR","/opt/informix");
		environment::setValue("INFORMIXSERVER","ol_informix1210");
	}



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
			"Driver={Informix};"
			"Server=ol_informix1210;"
			"Database=fedora40x64;"
			"UID=testuser;PWD=testpassword;");
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
		// the native informix driver returns an empty completed
		// connect string
		assertEqualDbc(dbc,(int)outcstringlen,0);
		assertEqualDbc(dbc,(const char *)outcstring,"");
	}
	// SQL_ATTR_PACKET_SIZE was set pre-connect and can be read now
	// (pass a null string-length; the native informix driver
	// scribbles an 8-byte length through it on this call)
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_PACKET_SIZE,
			(SQLPOINTER)&dbcuintval,0,NULL);
	if (issqlrelay) {
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,2048);
	} else {
		// the native informix driver can't read it back
		assertFailureDbc(dbc,erg);
	}
	stdoutput.printf("\n");



	// environment attributes (post-connect, forwarded to driver)
	stdoutput.printf("ENVIRONMENT ATTRIBUTES (post-connect): \n");


	// SQL_ATTR_ODBC_VERSION
	// can't be set once a connection handle exists (HY010)
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
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_AUTOCOMMIT_OFF);
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
	// the native informix driver doesn't store a READ_ONLY->READ_WRITE
	// switch; round-trip one direction and don't verify the restore
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
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_MODE_READ_ONLY);
	// restore initial value (driver may ignore the change; don't verify)
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_ACCESS_MODE,
			(SQLPOINTER)(uintptr_t)dbcinitial,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	#if (ODBCVER >= 0x0300)
	// SQL_ATTR_CONNECTION_TIMEOUT
	stdoutput.printf("  SQL_ATTR_CONNECTION_TIMEOUT\n");
	if (issqlrelay) {
		// save initial value
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_CONNECTION_TIMEOUT,
				(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		// set to 30
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_CONNECTION_TIMEOUT,
				(SQLPOINTER)(uintptr_t)30,0);
		assertSuccessDbc(dbc,erg);
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_CONNECTION_TIMEOUT,
				(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,30);
		// restore initial value
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_CONNECTION_TIMEOUT,
				(SQLPOINTER)(uintptr_t)dbcinitial,0);
		assertSuccessDbc(dbc,erg);
	} else {
		// the native informix driver doesn't implement this
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_CONNECTION_TIMEOUT,
				(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
		assertFailureDbc(dbc,erg);
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_CONNECTION_TIMEOUT,
				(SQLPOINTER)(uintptr_t)30,0);
		assertFailureDbc(dbc,erg);
	}
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_ATTR_METADATA_ID
	// sqlrelay always pattern-matches, so SQL_TRUE is substituted with
	// SQL_FALSE and returns SQL_SUCCESS_WITH_INFO + SQLSTATE 01S02
	stdoutput.printf("  SQL_ATTR_METADATA_ID\n");
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	// spec default is SQL_FALSE on both sides
	assertEqualDbc(dbc,(int)dbcinitial,(int)SQL_FALSE);
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
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_FALSE);
	// restore initial value
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)(uintptr_t)dbcinitial,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	// SQL_ATTR_TXN_ISOLATION
	// skip get here; it needs sys.v_$session/sys.v_$transaction read
	// access the test user lacks (ISOLATION LEVELS section covers levels)
	stdoutput.printf("  SQL_ATTR_TXN_ISOLATION\n");
	// SQL_TXN_READ_COMMITTED (set only; restore via SET below)
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_READ_COMMITTED,0);
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
	// set is a no-op for many drivers, but round-tripping the
	// initial value should always succeed
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
	if (issqlrelay) {
		// save initial value
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcinitial,(int)SQL_ASYNC_ENABLE_OFF);
		// SQL_ASYNC_ENABLE_OFF: matches the actual mode
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)(uintptr_t)SQL_ASYNC_ENABLE_OFF,0);
		assertEqualDbc(dbc,(int)erg,(int)SQL_SUCCESS);
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_ASYNC_ENABLE_OFF);
		// SQL_ASYNC_ENABLE_ON: unsupported, substituted with OFF;
		// SQLSTATE 01S02
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)(uintptr_t)SQL_ASYNC_ENABLE_ON,0);
		assertEqualDbc(dbc,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		// get reflects the substituted value, not what the app set
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_ASYNC_ENABLE_OFF);
		// restore initial value
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)(uintptr_t)dbcinitial,0);
		assertSuccessDbc(dbc,erg);
	} else {
		// the native informix driver doesn't implement this
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
		assertFailureDbc(dbc,erg);
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)(uintptr_t)SQL_ASYNC_ENABLE_OFF,0);
		assertFailureDbc(dbc,erg);
	}
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
	stdoutput.printf("  SQL_ATTR_AUTO_IPD\n");
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_AUTO_IPD,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// sqlrelay doesn't auto-populate the IPD
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_FALSE);
	} else {
		// the native informix driver auto-populates the IPD
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_TRUE);
	}
	stdoutput.printf("\n");
	#endif


	#if defined(SQL_ATTR_DISCONNECT_BEHAVIOR)
	// SQL_ATTR_DISCONNECT_BEHAVIOR
	stdoutput.printf("  SQL_ATTR_DISCONNECT_BEHAVIOR\n");
	if (issqlrelay) {
		// save initial value
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_DISCONNECT_BEHAVIOR,
				(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		// SQL_DB_RETURN_TO_POOL
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_DISCONNECT_BEHAVIOR,
				(SQLPOINTER)(uintptr_t)SQL_DB_RETURN_TO_POOL,0);
		assertSuccessDbc(dbc,erg);
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_DISCONNECT_BEHAVIOR,
				(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_DB_RETURN_TO_POOL);
		// SQL_DB_DISCONNECT
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_DISCONNECT_BEHAVIOR,
				(SQLPOINTER)(uintptr_t)SQL_DB_DISCONNECT,0);
		assertSuccessDbc(dbc,erg);
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_DISCONNECT_BEHAVIOR,
				(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_DB_DISCONNECT);
		// restore initial value
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_DISCONNECT_BEHAVIOR,
				(SQLPOINTER)(uintptr_t)dbcinitial,0);
		assertSuccessDbc(dbc,erg);
	} else {
		// the native informix driver doesn't implement this
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_DISCONNECT_BEHAVIOR,
				(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
		assertFailureDbc(dbc,erg);
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_DISCONNECT_BEHAVIOR,
				(SQLPOINTER)(uintptr_t)SQL_DB_RETURN_TO_POOL,0);
		assertFailureDbc(dbc,erg);
	}
	stdoutput.printf("\n");
	#endif


	// SQL_ATTR_TRACEFILE
	// driver manager rejects an empty restore value, so don't restore
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
	// both sqlrelay and the informix native driver implement this
	// save initial value
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TRANSLATE_LIB,
			(SQLPOINTER)dbcstrinit,
			sizeof(dbcstrinit),&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// the SQL Relay driver's initial translate lib is empty
		assertEqualDbc(dbc,(const char *)dbcstrinit,"");
	} else {
		// the native driver defaults to its gls conversion library
		assertEqualDbc(dbc,(const char *)dbcstrinit,
				"/opt/informix/lib//esql/igo4a304.so");
	}
	// round-trip to the initial value
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TRANSLATE_LIB,
			(SQLPOINTER)dbcstrinit,SQL_NTS);
	assertSuccessDbc(dbc,erg);
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
	stdoutput.printf("  SQL_ATTR_ANSI_APP\n");
	if (issqlrelay) {
		// save initial value
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_ANSI_APP,
				(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		// SQL_AA_TRUE
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_ANSI_APP,
				(SQLPOINTER)(uintptr_t)SQL_AA_TRUE,0);
		assertSuccessDbc(dbc,erg);
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_ANSI_APP,
				(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_AA_TRUE);
		// SQL_AA_FALSE
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_ANSI_APP,
				(SQLPOINTER)(uintptr_t)SQL_AA_FALSE,0);
		assertSuccessDbc(dbc,erg);
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_ANSI_APP,
				(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_AA_FALSE);
		// restore initial value
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_ANSI_APP,
				(SQLPOINTER)(uintptr_t)dbcinitial,0);
		assertSuccessDbc(dbc,erg);
	} else {
		// the native informix driver doesn't implement this; get fails
		// (the driver manager intercepts set, so set appears to succeed)
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_ANSI_APP,
				(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
		assertFailureDbc(dbc,erg);
	}
	stdoutput.printf("\n");
	#endif


	#if defined(SQL_ATTR_RESET_CONNECTION)
	// SQL_ATTR_RESET_CONNECTION
	stdoutput.printf("  SQL_ATTR_RESET_CONNECTION\n");
	if (issqlrelay) {
		// SQL_RESET_CONNECTION_YES: write-only per spec; set by the
		// driver manager before reusing a pooled connection
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_RESET_CONNECTION,
				(SQLPOINTER)(uintptr_t)
				SQL_RESET_CONNECTION_YES,0);
		assertSuccessDbc(dbc,erg);
	} else {
		// the native informix driver doesn't implement this
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_RESET_CONNECTION,
				(SQLPOINTER)(uintptr_t)
				SQL_RESET_CONNECTION_YES,0);
		assertFailureDbc(dbc,erg);
	}
	stdoutput.printf("\n");
	#endif


	#if defined(SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE)
	// SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE
	stdoutput.printf("  SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE\n");
	if (issqlrelay) {
		// save initial value
		erg=SQLGetConnectAttr(dbc,
				SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
				(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		// SQL_ASYNC_DBC_ENABLE_ON
		erg=SQLSetConnectAttr(dbc,
				SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
				(SQLPOINTER)(uintptr_t)
				SQL_ASYNC_DBC_ENABLE_ON,0);
		assertSuccessDbc(dbc,erg);
		erg=SQLGetConnectAttr(dbc,
				SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
				(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,
				(int)SQL_ASYNC_DBC_ENABLE_ON);
		// SQL_ASYNC_DBC_ENABLE_OFF
		erg=SQLSetConnectAttr(dbc,
				SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
				(SQLPOINTER)(uintptr_t)
				SQL_ASYNC_DBC_ENABLE_OFF,0);
		assertSuccessDbc(dbc,erg);
		erg=SQLGetConnectAttr(dbc,
				SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
				(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,
				(int)SQL_ASYNC_DBC_ENABLE_OFF);
		// restore initial value
		erg=SQLSetConnectAttr(dbc,
				SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
				(SQLPOINTER)(uintptr_t)dbcinitial,0);
		assertSuccessDbc(dbc,erg);
	} else {
		// the native informix driver doesn't implement this
		erg=SQLGetConnectAttr(dbc,
				SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
				(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
		assertFailureDbc(dbc,erg);
		erg=SQLSetConnectAttr(dbc,
				SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
				(SQLPOINTER)(uintptr_t)
				SQL_ASYNC_DBC_ENABLE_ON,0);
		assertFailureDbc(dbc,erg);
	}
	stdoutput.printf("\n");
	#endif


	#if defined(SQL_ATTR_DRIVER_THREADING)
	// SQL_ATTR_DRIVER_THREADING
	// driver-reported threading level; sqlrelay reports 1 (per-HDBC)
	stdoutput.printf("  SQL_ATTR_DRIVER_THREADING\n");
	if (issqlrelay) {
		// save initial value
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_DRIVER_THREADING,
				(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		// set to 1
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_DRIVER_THREADING,
				(SQLPOINTER)(uintptr_t)1,0);
		assertSuccessDbc(dbc,erg);
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_DRIVER_THREADING,
				(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,1);
		// restore initial value
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_DRIVER_THREADING,
				(SQLPOINTER)(uintptr_t)dbcinitial,0);
		assertSuccessDbc(dbc,erg);
	} else {
		// the native informix driver doesn't implement this
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_DRIVER_THREADING,
				(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
		assertFailureDbc(dbc,erg);
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_DRIVER_THREADING,
				(SQLPOINTER)(uintptr_t)1,0);
		assertFailureDbc(dbc,erg);
	}
	stdoutput.printf("\n");
	#endif



	// get info
	stdoutput.printf("GET INFO: \n");
	SQLUINTEGER	uintval;
	SQLUSMALLINT	usmallintval;
	// the informix native driver returns a ~2.7KB keyword list for
	// SQL_KEYWORDS and overflows the buffer regardless of the length
	// passed to SQLGetInfo, so this needs to be larger than 2048
	SQLCHAR		strval[4096];
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
	} else {
		assertEqualDbc(dbc,(int)uintval,1);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_SERVER_NAME
	stdoutput.printf("  SQL_SERVER_NAME\n");
	erg=SQLGetInfo(dbc,SQL_SERVER_NAME,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"sqlrelay");
	} else {
		// the Server= connect-string parameter
		assertEqualDbc(dbc,(const char *)strval,"ol_informix1210");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_SEARCH_PATTERN_ESCAPE
	stdoutput.printf("  SQL_SEARCH_PATTERN_ESCAPE\n");
	erg=SQLGetInfo(dbc,SQL_SEARCH_PATTERN_ESCAPE,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"\\");
	} else {
		// the native informix driver returns "\\"
		assertEqualDbc(dbc,(const char *)strval,"\\");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_DBMS_NAME
	stdoutput.printf("  SQL_DBMS_NAME\n");
	erg=SQLGetInfo(dbc,SQL_DBMS_NAME,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"informix");
	} else {
		assertEqualDbc(dbc,(const char *)strval,"Informix");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_DBMS_VER
	stdoutput.printf("  SQL_DBMS_VER\n");
	erg=SQLGetInfo(dbc,SQL_DBMS_VER,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertContainsVersionDbc(dbc,(const char *)strval);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ACCESSIBLE_TABLES
	stdoutput.printf("  SQL_ACCESSIBLE_TABLES\n");
	erg=SQLGetInfo(dbc,SQL_ACCESSIBLE_TABLES,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"Y");
	} else {
		// the native informix driver returns "Y"
		assertEqualDbc(dbc,(const char *)strval,"Y");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ACCESSIBLE_PROCEDURES
	stdoutput.printf("  SQL_ACCESSIBLE_PROCEDURES\n");
	erg=SQLGetInfo(dbc,SQL_ACCESSIBLE_PROCEDURES,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"Y");
	} else {
		// the native informix driver returns "Y"
		assertEqualDbc(dbc,(const char *)strval,"Y");
	}
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
		assertEqualDbc(dbc,(int)usmallintval,1);
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
	assertEqualDbc(dbc,(int)uintval,(int)SQL_TXN_READ_COMMITTED);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_IDENTIFIER_CASE
	stdoutput.printf("  SQL_IDENTIFIER_CASE\n");
	erg=SQLGetInfo(dbc,SQL_IDENTIFIER_CASE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,2);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_IDENTIFIER_QUOTE_CHAR
	stdoutput.printf("  SQL_IDENTIFIER_QUOTE_CHAR\n");
	erg=SQLGetInfo(dbc,SQL_IDENTIFIER_QUOTE_CHAR,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval," ");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_COLUMN_NAME_LEN
	stdoutput.printf("  SQL_MAX_COLUMN_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMN_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,128);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_CURSOR_NAME_LEN
	stdoutput.printf("  SQL_MAX_CURSOR_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_CURSOR_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,128);
	} else {
		assertEqualDbc(dbc,(int)usmallintval,128);
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
		assertEqualDbc(dbc,(int)usmallintval,32);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_CATALOG_NAME_LEN
	stdoutput.printf("  SQL_MAX_CATALOG_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_CATALOG_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,128);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_TABLE_NAME_LEN
	stdoutput.printf("  SQL_MAX_TABLE_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_TABLE_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,128);
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
		assertEqualDbc(dbc,(int)uintval,3);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_TXN_CAPABLE
	stdoutput.printf("  SQL_TXN_CAPABLE\n");
	erg=SQLGetInfo(dbc,SQL_TXN_CAPABLE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,2);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_USER_NAME
	stdoutput.printf("  SQL_USER_NAME\n");
	erg=SQLGetInfo(dbc,SQL_USER_NAME,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"testuser");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_TXN_ISOLATION_OPTION
	stdoutput.printf("  SQL_TXN_ISOLATION_OPTION\n");
	erg=SQLGetInfo(dbc,SQL_TXN_ISOLATION_OPTION,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,
				(int)(SQL_TXN_READ_UNCOMMITTED|SQL_TXN_READ_COMMITTED|
					SQL_TXN_REPEATABLE_READ|SQL_TXN_SERIALIZABLE));
	} else {
		assertEqualDbc(dbc,(int)uintval,11);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_INTEGRITY
	stdoutput.printf("  SQL_INTEGRITY\n");
	erg=SQLGetInfo(dbc,SQL_INTEGRITY,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"Y");
	} else {
		// the native informix driver returns "N"
		assertEqualDbc(dbc,(const char *)strval,"N");
	}
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,4);
	} else {
		// the native informix driver returns SQL_NC_LOW
		assertEqualDbc(dbc,(int)usmallintval,(int)SQL_NC_LOW);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ALTER_TABLE
	stdoutput.printf("  SQL_ALTER_TABLE\n");
	erg=SQLGetInfo(dbc,SQL_ALTER_TABLE,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	if (issqlrelay) {
		// sqlrelay bundles these bits whenever the backend reports
		// ADD_COLUMN, and omits DROP_COLUMN bits because the informix
		// backend module doesn't report it
		assertEqualDbc(dbc,(int)uintval,1048547);
	} else {
		assertEqualDbc(dbc,(int)uintval,11);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ORDER_BY_COLUMNS_IN_SELECT
	stdoutput.printf("  SQL_ORDER_BY_COLUMNS_IN_SELECT\n");
	erg=SQLGetInfo(dbc,SQL_ORDER_BY_COLUMNS_IN_SELECT,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"N");
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
		assertEqualDbc(dbc,(const char *)strval,"");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_COLUMNS_IN_GROUP_BY
	stdoutput.printf("  SQL_MAX_COLUMNS_IN_GROUP_BY\n");
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_GROUP_BY,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,32767);
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
		assertEqualDbc(dbc,(int)usmallintval,16);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_COLUMNS_IN_ORDER_BY
	stdoutput.printf("  SQL_MAX_COLUMNS_IN_ORDER_BY\n");
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_ORDER_BY,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,32767);
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
		assertEqualDbc(dbc,(int)usmallintval,32767);
	} else {
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
		assertEqualDbc(dbc,(int)usmallintval,32767);
	} else {
		assertEqualDbc(dbc,(int)usmallintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_INDEX_SIZE
	stdoutput.printf("  SQL_MAX_INDEX_SIZE\n");
	erg=SQLGetInfo(dbc,SQL_MAX_INDEX_SIZE,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,255);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_ROW_SIZE
	stdoutput.printf("  SQL_MAX_ROW_SIZE\n");
	erg=SQLGetInfo(dbc,SQL_MAX_ROW_SIZE,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,32767);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_STATEMENT_LEN
	stdoutput.printf("  SQL_MAX_STATEMENT_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_STATEMENT_LEN,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,2147483647);
	} else {
		// the native informix driver returns 0
		assertEqualDbc(dbc,(int)uintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_TABLES_IN_SELECT
	stdoutput.printf("  SQL_MAX_TABLES_IN_SELECT\n");
	erg=SQLGetInfo(dbc,SQL_MAX_TABLES_IN_SELECT,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_USER_NAME_LEN
	stdoutput.printf("  SQL_MAX_USER_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_USER_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,32);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	#if (ODBCVER >= 0x0300)
	// SQL_OJ_CAPABILITIES
	stdoutput.printf("  SQL_OJ_CAPABILITIES\n");
	erg=SQLGetInfo(dbc,SQL_OJ_CAPABILITIES,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_OJ_LEFT|SQL_OJ_RIGHT|SQL_OJ_FULL|
				SQL_OJ_NESTED|SQL_OJ_NOT_ORDERED|
				SQL_OJ_INNER|SQL_OJ_ALL_COMPARISON_OPS));
	} else {
		assertEqualDbc(dbc,(int)uintval,107);
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
	assertEqualDbc(dbc,(int)uintval,(int)SQL_INSENSITIVE);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DESCRIBE_PARAMETER
	stdoutput.printf("  SQL_DESCRIBE_PARAMETER\n");
	erg=SQLGetInfo(dbc,SQL_DESCRIBE_PARAMETER,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"N");
	} else {
		assertEqualDbc(dbc,(const char *)strval,"Y");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_CATALOG_NAME
	stdoutput.printf("  SQL_CATALOG_NAME\n");
	erg=SQLGetInfo(dbc,SQL_CATALOG_NAME,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
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
	assertEqualDbc(dbc,(const char *)strval,"");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_MAX_IDENTIFIER_LEN
	stdoutput.printf("  SQL_MAX_IDENTIFIER_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_IDENTIFIER_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,128);
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
		assertEqualDbc(dbc,(const char *)strval,"iclis09b.so");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_DRIVER_VER
	stdoutput.printf("  SQL_DRIVER_VER\n");
	erg=SQLGetInfo(dbc,SQL_DRIVER_VER,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertContainsVersionDbc(dbc,(const char *)strval);
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
	assertContainsVersionDbc(dbc,(const char *)strval);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ROW_UPDATES
	stdoutput.printf("  SQL_ROW_UPDATES\n");
	erg=SQLGetInfo(dbc,SQL_ROW_UPDATES,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"N");
	} else {
		assertEqualDbc(dbc,(const char *)strval,"N");
	}
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
	} else {
		assertEqualDbc(dbc,(int)usmallintval,(int)SQL_OSCC_COMPLIANT);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ODBC_SQL_CONFORMANCE
	stdoutput.printf("  SQL_ODBC_SQL_CONFORMANCE\n");
	erg=SQLGetInfo(dbc,SQL_ODBC_SQL_CONFORMANCE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,(int)SQL_OSC_EXTENDED);
	} else {
		assertEqualDbc(dbc,(int)usmallintval,(int)SQL_OSC_CORE);
	}
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,(int)SQL_CB_NULL);
	} else {
		assertEqualDbc(dbc,(int)usmallintval,0);
	}
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
		assertEqualDbc(dbc,(int)usmallintval,1);
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
		assertEqualDbc(dbc,(int)usmallintval,32);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_PROCEDURE_NAME_LEN
	stdoutput.printf("  SQL_MAX_PROCEDURE_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_PROCEDURE_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,128);
	} else {
		assertEqualDbc(dbc,(int)usmallintval,128);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_QUALIFIER_NAME_LEN
	stdoutput.printf("  SQL_MAX_QUALIFIER_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_QUALIFIER_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,128);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MULT_RESULT_SETS
	stdoutput.printf("  SQL_MULT_RESULT_SETS\n");
	erg=SQLGetInfo(dbc,SQL_MULT_RESULT_SETS,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"N");
	} else {
		assertEqualDbc(dbc,(const char *)strval,"N");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MULTIPLE_ACTIVE_TXN
	stdoutput.printf("  SQL_MULTIPLE_ACTIVE_TXN\n");
	erg=SQLGetInfo(dbc,SQL_MULTIPLE_ACTIVE_TXN,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"Y");
	} else {
		assertEqualDbc(dbc,(const char *)strval,"N");
	}
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"user");
	} else {
		assertEqualDbc(dbc,(const char *)strval,"Owner");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_PROCEDURE_TERM
	stdoutput.printf("  SQL_PROCEDURE_TERM\n");
	erg=SQLGetInfo(dbc,SQL_PROCEDURE_TERM,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"function");
	} else {
		assertEqualDbc(dbc,(const char *)strval,"Procedure");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_QUALIFIER_NAME_SEPARATOR
	stdoutput.printf("  SQL_QUALIFIER_NAME_SEPARATOR\n");
	erg=SQLGetInfo(dbc,SQL_QUALIFIER_NAME_SEPARATOR,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,":");
	} else {
		assertEqualDbc(dbc,(const char *)strval,":");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_QUALIFIER_TERM
	stdoutput.printf("  SQL_QUALIFIER_TERM\n");
	erg=SQLGetInfo(dbc,SQL_QUALIFIER_TERM,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"database");
	} else {
		assertEqualDbc(dbc,(const char *)strval,"Database");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_SCROLL_OPTIONS
	stdoutput.printf("  SQL_SCROLL_OPTIONS\n");
	erg=SQLGetInfo(dbc,SQL_SCROLL_OPTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_SO_FORWARD_ONLY|SQL_SO_STATIC));
	} else {
		assertEqualDbc(dbc,(int)uintval,1);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_TABLE_TERM
	stdoutput.printf("  SQL_TABLE_TERM\n");
	erg=SQLGetInfo(dbc,SQL_TABLE_TERM,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"table");
	} else {
		assertEqualDbc(dbc,(const char *)strval,"Table");
	}
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
		assertEqualDbc(dbc,(int)uintval,4778335);
	} else {
		assertEqualDbc(dbc,(int)uintval,14216543);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_STRING_FUNCTIONS
	stdoutput.printf("  SQL_STRING_FUNCTIONS\n");
	erg=SQLGetInfo(dbc,SQL_STRING_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,16);
	} else {
		assertEqualDbc(dbc,(int)uintval,8029);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_SYSTEM_FUNCTIONS
	stdoutput.printf("  SQL_SYSTEM_FUNCTIONS\n");
	erg=SQLGetInfo(dbc,SQL_SYSTEM_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,7);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_TIMEDATE_FUNCTIONS
	stdoutput.printf("  SQL_TIMEDATE_FUNCTIONS\n");
	erg=SQLGetInfo(dbc,SQL_TIMEDATE_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,288);
	} else {
		assertEqualDbc(dbc,(int)uintval,99183);
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
		assertEqualDbc(dbc,(int)uintval,0);
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
		assertEqualDbc(dbc,(int)uintval,0);
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
		assertEqualDbc(dbc,(int)uintval,0);
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
		assertEqualDbc(dbc,(int)uintval,0);
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
		assertEqualDbc(dbc,(int)uintval,0);
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
		assertEqualDbc(dbc,(int)uintval,0);
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
		assertEqualDbc(dbc,(int)uintval,0);
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
		assertEqualDbc(dbc,(int)uintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_LONGVARCHAR
	stdoutput.printf("  SQL_CONVERT_LONGVARCHAR\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_LONGVARCHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
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
		assertEqualDbc(dbc,(int)uintval,0);
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
		assertEqualDbc(dbc,(int)uintval,0);
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
		assertEqualDbc(dbc,(int)uintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_TIME
	stdoutput.printf("  SQL_CONVERT_TIME\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_TIME,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
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
		assertEqualDbc(dbc,(int)uintval,0);
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
		assertEqualDbc(dbc,(int)uintval,0);
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
		assertEqualDbc(dbc,(int)uintval,0);
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
	// neither the native informix driver nor sqlrelay supports it
	stdoutput.printf("  SQL_CONVERT_GUID\n");
	uintval=0xffffffff;
	erg=SQLGetInfo(dbc,SQL_CONVERT_GUID,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertFailureDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CORRELATION_NAME
	stdoutput.printf("  SQL_CORRELATION_NAME\n");
	erg=SQLGetInfo(dbc,SQL_CORRELATION_NAME,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,0);
	} else {
		assertEqualDbc(dbc,(int)usmallintval,(int)SQL_CN_ANY);
	}
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
	assertContainsVersionDbc(dbc,(const char *)strval);
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
		assertEqualDbc(dbc,(int)uintval,30);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_POSITIONED_STATEMENTS
	stdoutput.printf("  SQL_POSITIONED_STATEMENTS\n");
	erg=SQLGetInfo(dbc,SQL_POSITIONED_STATEMENTS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,3);
	} else {
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_PS_POSITIONED_DELETE|
				SQL_PS_POSITIONED_UPDATE|
				SQL_PS_SELECT_FOR_UPDATE));
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_BOOKMARK_PERSISTENCE
	stdoutput.printf("  SQL_BOOKMARK_PERSISTENCE\n");
	erg=SQLGetInfo(dbc,SQL_BOOKMARK_PERSISTENCE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,64);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_STATIC_SENSITIVITY
	stdoutput.printf("  SQL_STATIC_SENSITIVITY\n");
	erg=SQLGetInfo(dbc,SQL_STATIC_SENSITIVITY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"Y");
	} else {
		assertEqualDbc(dbc,(const char *)strval,"N");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_GROUP_BY
	stdoutput.printf("  SQL_GROUP_BY\n");
	erg=SQLGetInfo(dbc,SQL_GROUP_BY,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		// sqlrelay returns SQL_GB_NO_RELATION|SQL_GB_COLLATE, not a
		// legal enum (spec says SQL_GROUP_BY is a single enum, not a mask)
		assertEqualDbc(dbc,(int)usmallintval,
			(int)(SQL_GB_NO_RELATION|SQL_GB_COLLATE));
	} else {
		assertEqualDbc(dbc,(int)usmallintval,
			(int)SQL_GB_GROUP_BY_CONTAINS_SELECT);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_KEYWORDS
	stdoutput.printf("  SQL_KEYWORDS\n");
	erg=SQLGetInfo(dbc,SQL_KEYWORDS,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"after,ansi,append,attach,audit,before,bitmap,boolean,buffered,byte,cache,call,cluster,clustersize,codeset,database,datafiles,dataskip,datetime,dba,dbdate,dbmoney,debug,define,delimiter,deluxe,detach,dirty,distributions,document,each,elif,exclusive,exit,explain,express,expression,extend,extent,file,fillfactor,foreach,format,fraction,fragment,gk,hash,high,hold,hybrid,if,index,init,labeleq,labelge,labelgt,labelle,labellt,let,listing,lock,log,low,matches,maxerrors,medium,mode,modify,money,mounting,new,nvarchar,off,old,operational,optical,optimization,page,pdqpriority,pload,private,raise,range,raw,recordend,recover,referencing,rejectfile,release,remainder,rename,reserve,resolution,resource,resume,return,returning,returns,ridlist,robin,rollforward,round,row,rowids,sameas,samples,schedule,scratch,serial,share,skall,skinhibit,skshow,smallfloat,stability,standard,start,static,statistics,stdev,step,sync,synonym,system,temp,text,timeout,trace,trigger,units,unlock,variance,wait,while,xload,xunload");
	} else {
		assertEqualDbc(dbc,(const char *)strval,"access,access_method,account,address,after,aggregate,alignment,all_mutables,all_rows,ansi,append,array,attach,attributes,audit,authentication,authid,authorized,auto,avoid_execute,based,before,buckets,buffered,byte,cache,call,cannothash,cardinality,class,client,cluster,clustersize,coarse,codeset,collection,committed,commutator,component,components,compressed,connect_by_root,constructor,context,copy,costfunc,crcols,cycle,database,datafiles,dataskip,datetime,db2,dba,dbdate,dbmoney,dbpassword,dbsecadm,debug,decode,decrypt_binary,decrypt_char,define,delay,delimiter,deluxe,detach,dirty,disable,disabled,discard,distributebinary,distributesreferences,distributions,document,donotdistribute,duplicate,each,elif,enable,enabled,encrypt_aes,encrypt_tdes,encryption,environment,exclusive,executeanywhere,exemption,exit,explain,explicit,express,expression,extend,extent,file,fillfactor,filtering,first_rows,first_value,force,foreach,format,fraction,fragment,fragments,gb,general,gethint,gib,gk,handlesnulls,hash,high,hint,hold,home,hybrid,idslbacrules,ignore,immutable,implicit,increment,indexes,informix,instead,int8,internal,internallength,iscanonical,item,iterator,kb,keep,kib,label,labeleq,labelge,labelglb,labelgt,labelle,labellt,labellub,labeltostring,lag,last_value,lateral,limit,listing,lock,locks,matched,matches,maxerrors,maxlen,maxvalue,mb,median,medium,memory_resident,merge,mib,middle,minus,minvalue,mode,moderate,modify,money,mounting,move,mutable,negator,new,nlscase,nocache,nocycle,nomaxvalue,nomigrate,nominvalue,non_resident,noorder,normal,notemplatearg,novalidate,numrows,nvarchar,nvl,off,old,online,opaque,opclass,operational,optical,optimization,override,page,parallelizable,parameter,partition,partitions,passedbyvalue,password,pdqpriority,percall_cost,percent_rank,pload,policy,private,properties,raise,range,ratio_to_report,ratiotoreport,raw,recordend,recover,referencing,rejectfile,release,remainder,rename,repeatable,replace,replication,resolution,resource,respect,restart,resume,retain,return,returning,returns,reuse,robin,role,roles,rollforward,rolling,root,round,routine,rowids,rule,sameas,samples,sampling,savepoint,schedule,scratch,seclabel_by_comp,seclabel_by_name,seclabel_to_char,secondary,secured,security,selconst,selfunc,sequence,serial,serial8,serializable,serveruuid,setsessionauth,share,siblings,skall,skinhibit,skip,skshow,smallfloat,specific,stability,stack,standard,start,statchange,statistics,statlevel,stdev,step,stop,storage,store,strategies,stringtolabel,style,support,sync,synonym,table_space,table_type,tb,text,tib,timeout,trace,transition,tree,trigger,triggers,truncate,trusted,uid,uncommitted,under,units,unlock,upon,variable,variance,variant,violations,wait,while,without,xadatasource,xload,xunload");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_OWNER_USAGE
	stdoutput.printf("  SQL_OWNER_USAGE\n");
	erg=SQLGetInfo(dbc,SQL_OWNER_USAGE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,
		(int)(SQL_OU_DML_STATEMENTS|SQL_OU_PROCEDURE_INVOCATION|
			SQL_OU_TABLE_DEFINITION|SQL_OU_INDEX_DEFINITION|
			SQL_OU_PRIVILEGE_DEFINITION));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_QUALIFIER_USAGE
	stdoutput.printf("  SQL_QUALIFIER_USAGE\n");
	erg=SQLGetInfo(dbc,SQL_QUALIFIER_USAGE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,3);
	} else {
		assertEqualDbc(dbc,(int)uintval,1);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_QUOTED_IDENTIFIER_CASE
	stdoutput.printf("  SQL_QUOTED_IDENTIFIER_CASE\n");
	erg=SQLGetInfo(dbc,SQL_QUOTED_IDENTIFIER_CASE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,2);
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"N");
	} else {
		assertEqualDbc(dbc,(const char *)strval,"N");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_CHAR_LITERAL_LEN
	stdoutput.printf("  SQL_MAX_CHAR_LITERAL_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_CHAR_LITERAL_LEN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,256);
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"N");
	} else {
		assertEqualDbc(dbc,(const char *)strval,"Y");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_BINARY_LITERAL_LEN
	stdoutput.printf("  SQL_MAX_BINARY_LITERAL_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_BINARY_LITERAL_LEN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
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
	assertEqualDbc(dbc,(int)usmallintval,1);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	#if (ODBCVER >= 0x0300)
	// SQL_ACTIVE_ENVIRONMENTS
	stdoutput.printf("  SQL_ACTIVE_ENVIRONMENTS\n");
	erg=SQLGetInfo(dbc,SQL_ACTIVE_ENVIRONMENTS,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,0);
	} else {
		assertEqualDbc(dbc,(int)usmallintval,0);
	}
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
	assertEqualDbc(dbc,(int)uintval,(int)SQL_SC_SQL92_ENTRY);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DATETIME_LITERALS
	stdoutput.printf("  SQL_DATETIME_LITERALS\n");
	erg=SQLGetInfo(dbc,SQL_DATETIME_LITERALS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,0);
	}
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
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_BATCH_SUPPORT
	stdoutput.printf("  SQL_BATCH_SUPPORT\n");
	erg=SQLGetInfo(dbc,SQL_BATCH_SUPPORT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
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
	assertEqualDbc(dbc,(int)uintval,0);
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
	assertEqualDbc(dbc,(int)uintval,0);
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
	assertEqualDbc(dbc,(int)uintval,0);
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,1);
	}
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_CV_CREATE_VIEW|SQL_CV_CHECK_OPTION));
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DRIVER_HDESC
	// driver-manager-internal; needs an app-allocated HDESC via
	// InfoValuePtr, so unixODBC rejects this call with HY024
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DROP_TABLE
	stdoutput.printf("  SQL_DROP_TABLE\n");
	erg=SQLGetInfo(dbc,SQL_DROP_TABLE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,7);
	}
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,7);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DYNAMIC_CURSOR_ATTRIBUTES1
	stdoutput.printf("  SQL_DYNAMIC_CURSOR_ATTRIBUTES1\n");
	erg=SQLGetInfo(dbc,SQL_DYNAMIC_CURSOR_ATTRIBUTES1,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// neither driver supports dynamic-cursor attributes
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)uintval,0);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DYNAMIC_CURSOR_ATTRIBUTES2
	stdoutput.printf("  SQL_DYNAMIC_CURSOR_ATTRIBUTES2\n");
	erg=SQLGetInfo(dbc,SQL_DYNAMIC_CURSOR_ATTRIBUTES2,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// neither driver supports dynamic-cursor attributes
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)uintval,0);
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
		assertEqualDbc(dbc,(int)uintval,73217);
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
		assertEqualDbc(dbc,(int)uintval,2179);
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,3);
	}
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
		assertEqualDbc(dbc,(int)uintval,0);
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_KEYSET_CURSOR_ATTRIBUTES2
	stdoutput.printf("  SQL_KEYSET_CURSOR_ATTRIBUTES2\n");
	erg=SQLGetInfo(dbc,SQL_KEYSET_CURSOR_ATTRIBUTES2,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,0);
	}
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
	assertEqualDbc(dbc,(int)uintval,(int)SQL_PAS_NO_SELECT);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL92_DATETIME_FUNCTIONS
	stdoutput.printf("  SQL_SQL92_DATETIME_FUNCTIONS\n");
	erg=SQLGetInfo(dbc,SQL_SQL92_DATETIME_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL92_FOREIGN_KEY_DELETE_RULE
	stdoutput.printf("  SQL_SQL92_FOREIGN_KEY_DELETE_RULE\n");
	erg=SQLGetInfo(dbc,SQL_SQL92_FOREIGN_KEY_DELETE_RULE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL92_FOREIGN_KEY_UPDATE_RULE
	stdoutput.printf("  SQL_SQL92_FOREIGN_KEY_UPDATE_RULE\n");
	erg=SQLGetInfo(dbc,SQL_SQL92_FOREIGN_KEY_UPDATE_RULE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL92_GRANT
	stdoutput.printf("  SQL_SQL92_GRANT\n");
	erg=SQLGetInfo(dbc,SQL_SQL92_GRANT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,0);
	}
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
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,0);
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL92_RELATIONAL_JOIN_OPERATORS
	stdoutput.printf("  SQL_SQL92_RELATIONAL_JOIN_OPERATORS\n");
	erg=SQLGetInfo(dbc,SQL_SQL92_RELATIONAL_JOIN_OPERATORS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL92_REVOKE
	stdoutput.printf("  SQL_SQL92_REVOKE\n");
	erg=SQLGetInfo(dbc,SQL_SQL92_REVOKE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,96);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_SQL92_ROW_VALUE_CONSTRUCTOR
	stdoutput.printf("  SQL_SQL92_ROW_VALUE_CONSTRUCTOR\n");
	erg=SQLGetInfo(dbc,SQL_SQL92_ROW_VALUE_CONSTRUCTOR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,0);
	}
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
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,0);
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,0);
	}
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
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_SCC_XOPEN_CLI_VERSION1|
				SQL_SCC_ISO92_CLI));
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
		assertEqualDbc(dbc,(int)uintval,7681);
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
		assertEqualDbc(dbc,(int)uintval,10371);
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_AF_ALL|SQL_AF_AVG|SQL_AF_COUNT|
				SQL_AF_DISTINCT|SQL_AF_MAX|SQL_AF_MIN|
				SQL_AF_SUM));
	} else {
		// the native informix driver reports only SQL_AF_ALL
		assertEqualDbc(dbc,(int)uintval,(int)SQL_AF_ALL);
	}
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
		assertEqualDbc(dbc,(int)uintval,3);
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
	assertContainsVersionDbc(dbc,(const char *)strval);
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
		assertEqualDbc(dbc,(int)uintval,7);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0380)
	// SQL_ASYNC_DBC_FUNCTIONS
	// the native informix driver doesn't implement this ODBC 3.8 infotype
	stdoutput.printf("  SQL_ASYNC_DBC_FUNCTIONS\n");
	erg=SQLGetInfo(dbc,SQL_ASYNC_DBC_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertSuccessDbc(dbc,erg);
		// sqlrelay doesn't support async connection operations
		assertEqualDbc(dbc,(int)uintval,
				(int)SQL_ASYNC_DBC_NOT_CAPABLE);
	} else {
		assertFailureDbc(dbc,erg);
	}
	stdoutput.printf("\n");
	#endif


	// SQL_DRIVER_AWARE_POOLING_SUPPORTED
	// the native informix driver doesn't implement this infotype
	stdoutput.printf("  SQL_DRIVER_AWARE_POOLING_SUPPORTED\n");
	erg=SQLGetInfo(dbc,SQL_DRIVER_AWARE_POOLING_SUPPORTED,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertSuccessDbc(dbc,erg);
		// sqlrelay doesn't support driver-aware pooling
		assertEqualDbc(dbc,(int)uintval,
				(int)SQL_DRIVER_AWARE_POOLING_NOT_CAPABLE);
	} else {
		assertFailureDbc(dbc,erg);
	}
	stdoutput.printf("\n");


	#if (ODBCVER >= 0x0380)
	// SQL_ASYNC_NOTIFICATION
	// the native informix driver doesn't implement this ODBC 3.8 infotype
	stdoutput.printf("  SQL_ASYNC_NOTIFICATION\n");
	erg=SQLGetInfo(dbc,SQL_ASYNC_NOTIFICATION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertSuccessDbc(dbc,erg);
		// sqlrelay doesn't support async notification
		assertEqualDbc(dbc,(int)uintval,
				(int)SQL_ASYNC_NOTIFICATION_NOT_CAPABLE);
	} else {
		assertFailureDbc(dbc,erg);
	}
	stdoutput.printf("\n");
	#endif


	// SQL_DTC_TRANSITION_COST
	// the native informix driver doesn't implement this infotype
	stdoutput.printf("  SQL_DTC_TRANSITION_COST\n");
	erg=SQLGetInfo(dbc,SQL_DTC_TRANSITION_COST,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		// sqlrelay accepts the infotype but writes no value
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)vallen,0);
	} else {
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	} else {
		// the informix native driver still implements this ODBC 2.x call
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	}
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	} else {
		// the informix native driver still implements this ODBC 2.x call
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	}
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	} else {
		// the informix native driver still implements this ODBC 2.x call
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	}
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	} else {
		// the informix native driver still implements this ODBC 2.x call
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	}
	stdoutput.printf("\n");


	// SQL_API_SQLSETSTMTOPTION
	stdoutput.printf("  SQL_API_SQLSETSTMTOPTION\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLSETSTMTOPTION,&supported);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	} else {
		// the informix native driver still implements this ODBC 2.x call
		assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	}
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
		// the informix native driver does not implement SQLBrowseConnect
		assertEqualDbc(dbc,(int)supported,(int)SQL_FALSE);
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
		// sqlrelay doesn't yet implement descriptor field/record access
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
		// sqlrelay doesn't yet implement descriptor field/record access
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
		// sqlrelay doesn't yet implement descriptor field/record access
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
		// sqlrelay doesn't yet implement descriptor field/record access
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
		// sqlrelay doesn't yet implement bulk operations
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

	// informix supports all four isolation levels: dirty read
	// (read uncommitted), committed read, cursor stability (repeatable
	// read), and repeatable read (serializable)
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_READ_UNCOMMITTED,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	if (issqlrelay) {
		// the module's isolation-get query needs select access
		// to syssqlcurses, which the test user lacks
		assertFailureDbc(dbc,erg);
	} else {
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_TXN_READ_UNCOMMITTED);
	}

	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_READ_COMMITTED,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	if (issqlrelay) {
		// the module's isolation-get query needs select access
		// to syssqlcurses, which the test user lacks
		assertFailureDbc(dbc,erg);
	} else {
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_TXN_READ_COMMITTED);
	}

	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_REPEATABLE_READ,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	if (issqlrelay) {
		// the module's isolation-get query needs select access
		// to syssqlcurses, which the test user lacks
		assertFailureDbc(dbc,erg);
	} else {
		// informix's repeatable read is ansi serializable, and the
		// native driver reports it as such
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_TXN_SERIALIZABLE);
	}

	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_SERIALIZABLE,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	if (issqlrelay) {
		// the module's isolation-get query needs select access
		// to syssqlcurses, which the test user lacks
		assertFailureDbc(dbc,erg);
	} else {
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_TXN_SERIALIZABLE);
	}

	// reset to default isolation level
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_READ_COMMITTED,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	if (issqlrelay) {
		// the module's isolation-get query needs select access
		// to syssqlcurses, which the test user lacks
		assertFailureDbc(dbc,erg);
	} else {
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_TXN_READ_COMMITTED);
	}
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
	// both sqlrelay and the informix native driver support only
	// SQL_NONSCROLLABLE
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_SCROLLABLE,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_NONSCROLLABLE);
	// SQL_SCROLLABLE unsupported; substituted with SQL_NONSCROLLABLE,
	// expect SQL_SUCCESS_WITH_INFO + SQLSTATE 01S02
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_SCROLLABLE,
			(SQLPOINTER)(uintptr_t)SQL_SCROLLABLE,0);
	assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_SCROLLABLE,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	// get reflects the substituted value, not what the app set
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_NONSCROLLABLE);
	// SQL_NONSCROLLABLE: accepted as-is
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_SCROLLABLE,
			(SQLPOINTER)(uintptr_t)SQL_NONSCROLLABLE,0);
	assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_SCROLLABLE,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_NONSCROLLABLE);
	stdoutput.printf("\n");


	// SQL_ATTR_CURSOR_SENSITIVITY
	stdoutput.printf("  SQL_ATTR_CURSOR_SENSITIVITY\n");
	if (issqlrelay) {
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
				(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_INSENSITIVE);
		// SQL_INSENSITIVE: matches the actual cursor mode
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
				(SQLPOINTER)(uintptr_t)SQL_INSENSITIVE,0);
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS);
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
				(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_INSENSITIVE);
		// SQL_SENSITIVE: unsupported, substituted with
		// SQL_INSENSITIVE; SQLSTATE 01S02 ("Option value changed")
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
				(SQLPOINTER)(uintptr_t)SQL_SENSITIVE,0);
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
				(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		// get reflects the substituted value, not what the app set
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_INSENSITIVE);
		// SQL_UNSPECIFIED: spec lets the driver pick (insensitive)
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
				(SQLPOINTER)(uintptr_t)SQL_UNSPECIFIED,0);
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS);
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
				(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_INSENSITIVE);
		// restore
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
				(SQLPOINTER)(uintptr_t)stmtinitial,0);
		assertSuccessStmt(stmt,erg);
	} else {
		// the native informix driver doesn't implement this
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
				(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
		assertFailureStmt(stmt,erg);
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
				(SQLPOINTER)(uintptr_t)SQL_INSENSITIVE,0);
		assertFailureStmt(stmt,erg);
	}
	stdoutput.printf("\n");
	#endif


	// SQL_ATTR_CURSOR_TYPE
	// sqlrelay only supports SQL_CURSOR_FORWARD_ONLY; other values are
	// substituted, returning SQL_SUCCESS_WITH_INFO + SQLSTATE 01S02
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
	assertEqualStmt(stmt,(int)stmtulenval,30);
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
	assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_NOSCAN_OFF);
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
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_NOSCAN_OFF);
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
	if (issqlrelay) {
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtinitial,
					(int)SQL_ASYNC_ENABLE_OFF);
		// SQL_ASYNC_ENABLE_ON: unsupported, substituted with
		// SQL_ASYNC_ENABLE_OFF; SQLSTATE 01S02 ("Option value changed")
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)(uintptr_t)SQL_ASYNC_ENABLE_ON,0);
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		// get reflects the substituted value, not what the app set
		assertEqualStmt(stmt,(int)stmtulenval,
					(int)SQL_ASYNC_ENABLE_OFF);
		// SQL_ASYNC_ENABLE_OFF: matches the actual mode
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)(uintptr_t)SQL_ASYNC_ENABLE_OFF,0);
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS);
	} else {
		// the informix native driver is not capable of this statement
		// attribute; get and set both fail with SQLSTATE S1C00
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
		assertFailureStmt(stmt,erg);
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)(uintptr_t)SQL_ASYNC_ENABLE_ON,0);
		assertFailureStmt(stmt,erg);
	}
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
	// driver may ignore for non-keyset cursors; the native informix driver
	// leaves it at 0, sqlrelay round-trips the value
	stdoutput.printf("  SQL_ATTR_KEYSET_SIZE\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_KEYSET_SIZE,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtinitial,0);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_KEYSET_SIZE,
			(SQLPOINTER)(uintptr_t)10,0);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_KEYSET_SIZE,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)stmtulenval,10);
	} else {
		assertEqualStmt(stmt,(int)stmtulenval,0);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_KEYSET_SIZE,
			(SQLPOINTER)(uintptr_t)stmtinitial,0);
	assertSuccessStmt(stmt,erg);
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
	if (issqlrelay) {
		// sqlrelay lacks positioned updates, so only SQL_SC_NON_UNIQUE
		// is supported; others are substituted with 01S02
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
				(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_SC_NON_UNIQUE);
		// SQL_SC_NON_UNIQUE: matches the actual mode
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
				(SQLPOINTER)(uintptr_t)SQL_SC_NON_UNIQUE,0);
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS);
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
				(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_SC_NON_UNIQUE);
		// SQL_SC_TRY_UNIQUE: substituted
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
				(SQLPOINTER)(uintptr_t)SQL_SC_TRY_UNIQUE,0);
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
				(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		// get reflects the substituted value, not what the app set
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_SC_NON_UNIQUE);
		// SQL_SC_UNIQUE: substituted
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
				(SQLPOINTER)(uintptr_t)SQL_SC_UNIQUE,0);
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
				(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_SC_NON_UNIQUE);
	} else {
		// the native informix driver doesn't implement this
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
				(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
		assertFailureStmt(stmt,erg);
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
				(SQLPOINTER)(uintptr_t)SQL_SC_UNIQUE,0);
		assertFailureStmt(stmt,erg);
	}
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
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_RD_OFF);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_RETRIEVE_DATA,
			(SQLPOINTER)(uintptr_t)SQL_RD_ON,0);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_RETRIEVE_DATA,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_RD_ON);
	stdoutput.printf("\n");


	// SQL_ATTR_USE_BOOKMARKS
	// sqlrelay lacks bookmarks; SQL_UB_VARIABLE is substituted with
	// SQL_UB_OFF, returning SQL_SUCCESS_WITH_INFO + SQLSTATE 01S02
	stdoutput.printf("  SQL_ATTR_USE_BOOKMARKS\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_USE_BOOKMARKS,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_UB_OFF);
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_USE_BOOKMARKS,
			(SQLPOINTER)(uintptr_t)SQL_UB_VARIABLE,0);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_USE_BOOKMARKS,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		// get reflects the substituted value, not what the app set
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_UB_OFF);
	} else {
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_UB_VARIABLE);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_USE_BOOKMARKS,
			(SQLPOINTER)(uintptr_t)SQL_UB_OFF,0);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_USE_BOOKMARKS,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_UB_OFF);
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
	// sqlrelay always pattern-matches, so SQL_TRUE is substituted with
	// SQL_FALSE and returns SQL_SUCCESS_WITH_INFO + SQLSTATE 01S02
	stdoutput.printf("  SQL_ATTR_METADATA_ID\n");
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	// spec default is SQL_FALSE on both sides
	assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_FALSE);
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
	// stub: sqlrelay round-trips the value; auto-IPD isn't gated on it
	stdoutput.printf("  SQL_ATTR_ENABLE_AUTO_IPD\n");
	if (issqlrelay) {
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
				(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_TRUE);
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
				(SQLPOINTER)(uintptr_t)SQL_TRUE,0);
		assertSuccessStmt(stmt,erg);
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
				(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_TRUE);
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
				(SQLPOINTER)(uintptr_t)SQL_FALSE,0);
		assertSuccessStmt(stmt,erg);
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
				(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_FALSE);
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
				(SQLPOINTER)(uintptr_t)stmtinitial,0);
		assertSuccessStmt(stmt,erg);
	} else {
		// informix implements this; the initial value is SQL_FALSE
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
				(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_FALSE);
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
				(SQLPOINTER)(uintptr_t)SQL_TRUE,0);
		assertSuccessStmt(stmt,erg);
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
				(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_TRUE);
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
				(SQLPOINTER)(uintptr_t)SQL_FALSE,0);
		assertSuccessStmt(stmt,erg);
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
				(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_FALSE);
	}
	stdoutput.printf("\n");


	// SQL_ATTR_PARAMSET_SIZE
	// sqlrelay lacks parameter arrays; values other than 1 are
	// substituted with 1, returning SQL_SUCCESS_WITH_INFO + 01S02
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
	// sqlrelay lacks parameter arrays; row-wise (non-zero) is substituted
	// with SQL_PARAM_BIND_BY_COLUMN, returning SQL_SUCCESS_WITH_INFO + 01S02
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
	assertEqualStmt(stmt,
		(int)(stmtptrval==(SQLPOINTER)&stmtparamsprocessed),1);
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
		// sqlrelay lacks bookmarks, so no bookmark pointer is in effect
		assertEqualStmt(stmt,(int)(stmtptrinit==NULL),1);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_FETCH_BOOKMARK_PTR,
			(SQLPOINTER)&stmtbookmark,SQL_IS_POINTER);
	if (issqlrelay) {
		// non-NULL bookmark pointer rejected with SQLSTATE HYC00
		assertFailureStmt(stmt,erg);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	erg=SQLGetStmtAttr(stmt,SQL_ATTR_FETCH_BOOKMARK_PTR,
			(SQLPOINTER)&stmtptrval,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
		// the rejected set didn't take, so still NULL
		assertEqualStmt(stmt,(int)(stmtptrval==NULL),1);
	} else {
		assertEqualStmt(stmt,
			(int)(stmtptrval==(SQLPOINTER)&stmtbookmark),1);
	}
	// restore: NULL is accepted, non-NULL is not
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_FETCH_BOOKMARK_PTR,
			(SQLPOINTER)stmtptrinit,SQL_IS_POINTER);
	if (issqlrelay && stmtptrinit!=NULL) {
		assertFailureStmt(stmt,erg);
	} else {
		assertSuccessStmt(stmt,erg);
	}
	stdoutput.printf("\n");
	#endif


	// invalid attribute identifier
	// spec: get/set of an unrecognized attribute must return HY092
	stdoutput.printf("  invalid attribute\n");
	erg=SQLGetStmtAttr(stmt,99999,
			(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
	assertFailureStmt(stmt,erg);
	erg=SQLSetStmtAttr(stmt,99999,
			(SQLPOINTER)(uintptr_t)0,0);
	assertFailureStmt(stmt,erg);
	stdoutput.printf("\n");



	// create testtable
	stdoutput.printf("CREATE TESTTABLE: \n");
	// informix has no MVCC; run the data sections in autocommit-on mode so
	// each statement commits immediately and the second connection's
	// count(*) in the COMMIT AND ROLLBACK section doesn't block on row
	// locks held by an in-flight transaction (that section manages its own
	// transaction and a dirty-read second connection)
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
		(SQLPOINTER)SQL_AUTOCOMMIT_ON,0);
	assertSuccessDbc(dbc,erg);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable ("
		"	testint integer, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testdatetime datetime year to second, "
		"	testtext text, "
		"	testclob text, "
		"	testblob byte)",
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
		"	'2001-01-01 01:00:00', "
		"	'testtext1', "
		"	'testclob1', "
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
	// informix uses positional ? parameter markers in both modes
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
	assertSuccessStmt(stmt,erg);
	SQLSMALLINT	bindvarcount;
	erg=SQLNumParams(stmt,&bindvarcount);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)bindvarcount,7);

	SQLINTEGER	intval;
	SQLCHAR		*charval;
	SQLCHAR		*varcharval;
	SQL_TIMESTAMP_STRUCT	dateval;
	SQLCHAR		*longval;
	SQLCHAR		clobval[40];
	SQLCHAR		*blobval;
	SQLLEN		intlen=sizeof(SQLINTEGER);
	SQLLEN		charlen=SQL_NTS;
	SQLLEN		varcharlen=SQL_NTS;
	SQLLEN		datelen=sizeof(SQL_TIMESTAMP_STRUCT);
	SQLLEN		longlen=SQL_NTS;
	SQLLEN		cloblen=sizeof(clobval);
	SQLLEN		clobstrlen=SQL_NTS;
	SQLLEN		bloblen=9;
	// The informix native driver stores an empty value unless ColumnSize is
	// non-zero for the SQL_LONGVARCHAR/SQL_LONGVARBINARY (text/byte) binds
	// below; sqlrelay accepts 0.
	SQLULEN		lobcolsize=(issqlrelay)?0:4096;

	// row 2
	intval=2;
	charval=(SQLCHAR *)"testchar2";
	varcharval=(SQLCHAR *)"testvarchar2";
	dateval.year=2002;
	dateval.month=1;
	dateval.day=1;
	dateval.hour=2;
	dateval.minute=3;
	dateval.second=4;
	dateval.fraction=0;
	longval=(SQLCHAR *)"testtext2";
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
				SQL_C_TYPE_TIMESTAMP,SQL_TYPE_TIMESTAMP,
				19,0,
				(SQLPOINTER)&dateval,
				sizeof(dateval),&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,5,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				lobcolsize,0,
				(SQLPOINTER)longval,
				0,&longlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,6,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				lobcolsize,0,
				(SQLPOINTER)clobval,
				cloblen,&clobstrlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,7,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				lobcolsize,0,
				(SQLPOINTER)blobval,
				0,&bloblen);
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
	intval=3;
	charval=(SQLCHAR *)"testchar3";
	varcharval=(SQLCHAR *)"testvarchar3";
	dateval.year=2003;
	dateval.month=1;
	dateval.day=1;
	dateval.hour=3;
	dateval.minute=4;
	dateval.second=5;
	dateval.fraction=0;
	longval=(SQLCHAR *)"testtext3";
	charstring::copy((char *)clobval,"testclob3");
	blobval=(SQLCHAR *)"testblob3";
	charlen=SQL_NTS;
	varcharlen=SQL_NTS;
	datelen=sizeof(SQL_TIMESTAMP_STRUCT);
	longlen=SQL_NTS;
	cloblen=sizeof(clobval);
	clobstrlen=SQL_NTS;
	bloblen=9;
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
				SQL_C_TYPE_TIMESTAMP,SQL_TYPE_TIMESTAMP,
				19,0,
				(SQLPOINTER)&dateval,
				sizeof(dateval),&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,5,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				lobcolsize,0,
				(SQLPOINTER)longval,
				0,&longlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,6,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				lobcolsize,0,
				(SQLPOINTER)clobval,
				cloblen,&clobstrlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,7,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				lobcolsize,0,
				(SQLPOINTER)blobval,
				0,&bloblen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
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
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	?)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);

	// row 4
	intval=4;
	charval=(SQLCHAR *)"testchar4";
	varcharval=(SQLCHAR *)"testvarchar4";
	dateval.year=2004;
	dateval.month=1;
	dateval.day=1;
	dateval.hour=4;
	dateval.minute=5;
	dateval.second=6;
	dateval.fraction=0;
	longval=(SQLCHAR *)"testtext4";
	charstring::copy((char *)clobval,"testclob4");
	blobval=(SQLCHAR *)"testblob4";
	charlen=SQL_NTS;
	varcharlen=SQL_NTS;
	datelen=sizeof(SQL_TIMESTAMP_STRUCT);
	longlen=SQL_NTS;
	// data-at-exec sentinel for the CLOB column
	SQLLEN	clobdataatexeclen=SQL_LEN_DATA_AT_EXEC(9);
	bloblen=9;
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
				SQL_C_TYPE_TIMESTAMP,SQL_TYPE_TIMESTAMP,
				19,0,
				(SQLPOINTER)&dateval,
				sizeof(dateval),&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,5,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				lobcolsize,0,
				(SQLPOINTER)longval,
				0,&longlen);
	assertSuccessStmt(stmt,erg);
	// clobval is the SQLParamData token; real value comes via SQLPutData
	erg=SQLBindParameter(stmt,6,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				lobcolsize,0,
				(SQLPOINTER)clobval,
				0,&clobdataatexeclen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,7,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				lobcolsize,0,
				(SQLPOINTER)blobval,
				0,&bloblen);
	assertSuccessStmt(stmt,erg);
	// data-at-exec column triggers SQL_NEED_DATA
	erg=SQLExecute(stmt);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NEED_DATA);
	// drive the data-at-exec loop
	SQLPOINTER	paramdataptr=NULL;
	erg=SQLParamData(stmt,&paramdataptr);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NEED_DATA);
	assertEqualStmt(stmt,(const char *)paramdataptr,"testclob4");
	erg=SQLPutData(stmt,clobval,9);
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
	assertEqualStmt(stmt,(int)colcount,7);
	stdoutput.printf("\n");



	// column info
	stdoutput.printf("COLUMN INFO: \n");
	SQLCHAR		colname[256];
	SQLSMALLINT	colnamelen;
	SQLSMALLINT	datatype;
	SQLULEN		colsize;
	SQLSMALLINT	decdigits;
	SQLSMALLINT	nullable;

	// informix folds unquoted identifiers to lowercase

	// col 1
	erg=SQLDescribeCol(stmt,1,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testint");
	assertEqualStmt(stmt,(int)colnamelen,7);
	assertEqualStmt(stmt,(int)datatype,SQL_INTEGER);
	assertEqualStmt(stmt,(int)colsize,10);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 2
	erg=SQLDescribeCol(stmt,2,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testchar");
	assertEqualStmt(stmt,(int)colnamelen,8);
	assertEqualStmt(stmt,(int)datatype,SQL_CHAR);
	assertEqualStmt(stmt,(int)colsize,40);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 3
	erg=SQLDescribeCol(stmt,3,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testvarchar");
	assertEqualStmt(stmt,(int)colnamelen,11);
	assertEqualStmt(stmt,(int)datatype,SQL_VARCHAR);
	assertEqualStmt(stmt,(int)colsize,40);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 4
	erg=SQLDescribeCol(stmt,4,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testdatetime");
	assertEqualStmt(stmt,(int)colnamelen,12);
	#if (ODBCVER >= 0x0300)
	assertEqualStmt(stmt,(int)datatype,SQL_TYPE_TIMESTAMP);
	#else
	assertEqualStmt(stmt,(int)datatype,SQL_TIMESTAMP);
	#endif
	assertEqualStmt(stmt,(int)colsize,19);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 5
	erg=SQLDescribeCol(stmt,5,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testtext");
	assertEqualStmt(stmt,(int)colnamelen,8);
	assertEqualStmt(stmt,(int)datatype,SQL_LONGVARCHAR);
	assertEqualStmt(stmt,(int)colsize,2147483647);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 6
	erg=SQLDescribeCol(stmt,6,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testclob");
	assertEqualStmt(stmt,(int)colnamelen,8);
	assertEqualStmt(stmt,(int)datatype,SQL_LONGVARCHAR);
	assertEqualStmt(stmt,(int)colsize,2147483647);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 7
	erg=SQLDescribeCol(stmt,7,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"testblob");
	assertEqualStmt(stmt,(int)colnamelen,8);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)datatype,SQL_BINARY);
	} else {
		assertEqualStmt(stmt,(int)datatype,SQL_LONGVARBINARY);
	}
	assertEqualStmt(stmt,(int)colsize,2147483647);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);
	stdoutput.printf("\n");



	// fetch rows (SQLBindCol)
	stdoutput.printf("FETCH ROWS (SQLBindCol): \n");
	SQLINTEGER	numval;
	SQLCHAR		charfield[41];
	SQLCHAR		varcharfield[41];
	SQL_TIMESTAMP_STRUCT	datefield;
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
	erg=SQLBindCol(stmt,4,SQL_C_TYPE_TIMESTAMP,
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
	assertEqualStmt(stmt,(int)dateind,(int)sizeof(SQL_TIMESTAMP_STRUCT));
	assertEqualStmt(stmt,(int)datefield.year,2001);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	assertEqualStmt(stmt,(int)datefield.hour,1);
	assertEqualStmt(stmt,(int)datefield.minute,0);
	assertEqualStmt(stmt,(int)datefield.second,0);
	assertEqualStmt(stmt,(int)datefield.fraction,0);
	assertEqualStmt(stmt,(int)longind,9);
	assertEqualStmt(stmt,(const char *)longfield,"testtext1");
	assertEqualStmt(stmt,(int)clobind,9);
	assertEqualStmt(stmt,(const char *)clobfield,"testclob1");
	// row 1's blob was inserted NULL
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
	assertEqualStmt(stmt,(int)dateind,(int)sizeof(SQL_TIMESTAMP_STRUCT));
	assertEqualStmt(stmt,(int)datefield.year,2002);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	assertEqualStmt(stmt,(int)datefield.hour,2);
	assertEqualStmt(stmt,(int)datefield.minute,3);
	assertEqualStmt(stmt,(int)datefield.second,4);
	assertEqualStmt(stmt,(int)datefield.fraction,0);
	assertEqualStmt(stmt,(int)longind,9);
	assertEqualStmt(stmt,(const char *)longfield,"testtext2");
	assertEqualStmt(stmt,(int)clobind,9);
	assertEqualStmt(stmt,(const char *)clobfield,"testclob2");
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
	assertEqualStmt(stmt,(int)dateind,(int)sizeof(SQL_TIMESTAMP_STRUCT));
	assertEqualStmt(stmt,(int)datefield.year,2003);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	assertEqualStmt(stmt,(int)datefield.hour,3);
	assertEqualStmt(stmt,(int)datefield.minute,4);
	assertEqualStmt(stmt,(int)datefield.second,5);
	assertEqualStmt(stmt,(int)datefield.fraction,0);
	assertEqualStmt(stmt,(int)longind,9);
	assertEqualStmt(stmt,(const char *)longfield,"testtext3");
	assertEqualStmt(stmt,(int)clobind,9);
	assertEqualStmt(stmt,(const char *)clobfield,"testclob3");
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
	assertEqualStmt(stmt,(int)dateind,(int)sizeof(SQL_TIMESTAMP_STRUCT));
	assertEqualStmt(stmt,(int)datefield.year,2004);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	assertEqualStmt(stmt,(int)datefield.hour,4);
	assertEqualStmt(stmt,(int)datefield.minute,5);
	assertEqualStmt(stmt,(int)datefield.second,6);
	assertEqualStmt(stmt,(int)datefield.fraction,0);
	assertEqualStmt(stmt,(int)longind,9);
	assertEqualStmt(stmt,(const char *)longfield,"testtext4");
	// row 4 was SQLPutData'd with length 9, so the CLOB is genuinely
	// 9 bytes in both modes (no buffer-padding artifact)
	assertEqualStmt(stmt,(int)clobind,9);
	assertEqualStmt(stmt,(const char *)clobfield,"testclob4");
	assertEqualStmt(stmt,(int)blobind,9);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"testblob4",9));

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
	SQLINTEGER	gnumval;
	SQLCHAR		gcharfield[41];
	SQLCHAR		gvarcharfield[41];
	SQL_TIMESTAMP_STRUCT	gdatefield;
	SQLCHAR		glongfield[256];
	SQLCHAR		gclobfield[256];
	SQLCHAR		gblobfield[256];
	SQLLEN		gnumind;
	SQLLEN		gcharind;
	SQLLEN		gvarcharind;
	SQLLEN		gdateind;
	SQLLEN		glongind;
	SQLLEN		gclobind;
	SQLLEN		gblobind;

	// row 1
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetData(stmt,1,SQL_C_SLONG,
			&gnumval,sizeof(gnumval),&gnumind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gnumind,(int)sizeof(SQLINTEGER));
	assertEqualStmt(stmt,(int)gnumval,1);
	// SQL Relay returns SQL_NO_DATA on a repeat SQLGetData() of a
	// fixed-length column; native drivers vary on this.
	if (issqlrelay) {
		erg=SQLGetData(stmt,1,SQL_C_SLONG,
				&gnumval,sizeof(gnumval),&gnumind);
		assertEqualStmt(stmt,(int)erg,(int)SQL_NO_DATA);
	}
	erg=SQLGetData(stmt,2,SQL_C_CHAR,
			gcharfield,sizeof(gcharfield),&gcharind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gcharind,40);
	assertEqualStmt(stmt,(const char *)gcharfield,
			"testchar1                               ");
	erg=SQLGetData(stmt,3,SQL_C_CHAR,
			gvarcharfield,sizeof(gvarcharfield),&gvarcharind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gvarcharind,12);
	assertEqualStmt(stmt,(const char *)gvarcharfield,"testvarchar1");
	erg=SQLGetData(stmt,4,SQL_C_TYPE_TIMESTAMP,
			&gdatefield,sizeof(gdatefield),&gdateind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gdateind,(int)sizeof(SQL_TIMESTAMP_STRUCT));
	assertEqualStmt(stmt,(int)gdatefield.year,2001);
	assertEqualStmt(stmt,(int)gdatefield.month,1);
	assertEqualStmt(stmt,(int)gdatefield.day,1);
	assertEqualStmt(stmt,(int)gdatefield.hour,1);
	assertEqualStmt(stmt,(int)gdatefield.minute,0);
	assertEqualStmt(stmt,(int)gdatefield.second,0);
	assertEqualStmt(stmt,(int)gdatefield.fraction,0);
	erg=SQLGetData(stmt,5,SQL_C_CHAR,
			glongfield,sizeof(glongfield),&glongind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)glongind,9);
	assertEqualStmt(stmt,(const char *)glongfield,"testtext1");
	erg=SQLGetData(stmt,6,SQL_C_CHAR,
			gclobfield,sizeof(gclobfield),&gclobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gclobind,9);
	assertEqualStmt(stmt,(const char *)gclobfield,"testclob1");
	erg=SQLGetData(stmt,7,SQL_C_BINARY,
			gblobfield,sizeof(gblobfield),&gblobind);
	assertSuccessStmt(stmt,erg);
	// row 1's blob was inserted NULL
	assertEqualStmt(stmt,(int)gblobind,(int)SQL_NULL_DATA);
	// SQL Relay returns SQL_NO_DATA on a repeat SQLGetData() of a NULL
	// column; native drivers vary on this.
	if (issqlrelay) {
		erg=SQLGetData(stmt,7,SQL_C_BINARY,
				gblobfield,sizeof(gblobfield),&gblobind);
		assertEqualStmt(stmt,(int)erg,(int)SQL_NO_DATA);
	}

	// row 2
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetData(stmt,1,SQL_C_SLONG,
			&gnumval,sizeof(gnumval),&gnumind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gnumind,(int)sizeof(SQLINTEGER));
	assertEqualStmt(stmt,(int)gnumval,2);
	erg=SQLGetData(stmt,2,SQL_C_CHAR,
			gcharfield,sizeof(gcharfield),&gcharind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gcharind,40);
	assertEqualStmt(stmt,(const char *)gcharfield,
			"testchar2                               ");
	erg=SQLGetData(stmt,3,SQL_C_CHAR,
			gvarcharfield,sizeof(gvarcharfield),&gvarcharind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gvarcharind,12);
	assertEqualStmt(stmt,(const char *)gvarcharfield,"testvarchar2");
	erg=SQLGetData(stmt,4,SQL_C_TYPE_TIMESTAMP,
			&gdatefield,sizeof(gdatefield),&gdateind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gdateind,(int)sizeof(SQL_TIMESTAMP_STRUCT));
	assertEqualStmt(stmt,(int)gdatefield.year,2002);
	assertEqualStmt(stmt,(int)gdatefield.month,1);
	assertEqualStmt(stmt,(int)gdatefield.day,1);
	assertEqualStmt(stmt,(int)gdatefield.hour,2);
	assertEqualStmt(stmt,(int)gdatefield.minute,3);
	assertEqualStmt(stmt,(int)gdatefield.second,4);
	assertEqualStmt(stmt,(int)gdatefield.fraction,0);
	erg=SQLGetData(stmt,5,SQL_C_CHAR,
			glongfield,sizeof(glongfield),&glongind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)glongind,9);
	assertEqualStmt(stmt,(const char *)glongfield,"testtext2");
	erg=SQLGetData(stmt,6,SQL_C_CHAR,
			gclobfield,sizeof(gclobfield),&gclobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gclobind,9);
	assertEqualStmt(stmt,(const char *)gclobfield,"testclob2");
	erg=SQLGetData(stmt,7,SQL_C_BINARY,
			gblobfield,sizeof(gblobfield),&gblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(gblobfield,"testblob2",9));

	// row 3
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetData(stmt,1,SQL_C_SLONG,
			&gnumval,sizeof(gnumval),&gnumind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gnumind,(int)sizeof(SQLINTEGER));
	assertEqualStmt(stmt,(int)gnumval,3);
	erg=SQLGetData(stmt,2,SQL_C_CHAR,
			gcharfield,sizeof(gcharfield),&gcharind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gcharind,40);
	assertEqualStmt(stmt,(const char *)gcharfield,
			"testchar3                               ");
	erg=SQLGetData(stmt,3,SQL_C_CHAR,
			gvarcharfield,sizeof(gvarcharfield),&gvarcharind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gvarcharind,12);
	assertEqualStmt(stmt,(const char *)gvarcharfield,"testvarchar3");
	erg=SQLGetData(stmt,4,SQL_C_TYPE_TIMESTAMP,
			&gdatefield,sizeof(gdatefield),&gdateind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gdateind,(int)sizeof(SQL_TIMESTAMP_STRUCT));
	assertEqualStmt(stmt,(int)gdatefield.year,2003);
	assertEqualStmt(stmt,(int)gdatefield.month,1);
	assertEqualStmt(stmt,(int)gdatefield.day,1);
	assertEqualStmt(stmt,(int)gdatefield.hour,3);
	assertEqualStmt(stmt,(int)gdatefield.minute,4);
	assertEqualStmt(stmt,(int)gdatefield.second,5);
	assertEqualStmt(stmt,(int)gdatefield.fraction,0);
	erg=SQLGetData(stmt,5,SQL_C_CHAR,
			glongfield,sizeof(glongfield),&glongind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)glongind,9);
	assertEqualStmt(stmt,(const char *)glongfield,"testtext3");
	erg=SQLGetData(stmt,6,SQL_C_CHAR,
			gclobfield,sizeof(gclobfield),&gclobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gclobind,9);
	assertEqualStmt(stmt,(const char *)gclobfield,"testclob3");
	erg=SQLGetData(stmt,7,SQL_C_BINARY,
			gblobfield,sizeof(gblobfield),&gblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(gblobfield,"testblob3",9));

	// row 4
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetData(stmt,1,SQL_C_SLONG,
			&gnumval,sizeof(gnumval),&gnumind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gnumind,(int)sizeof(SQLINTEGER));
	assertEqualStmt(stmt,(int)gnumval,4);
	erg=SQLGetData(stmt,2,SQL_C_CHAR,
			gcharfield,sizeof(gcharfield),&gcharind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gcharind,40);
	assertEqualStmt(stmt,(const char *)gcharfield,
			"testchar4                               ");
	erg=SQLGetData(stmt,3,SQL_C_CHAR,
			gvarcharfield,sizeof(gvarcharfield),&gvarcharind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gvarcharind,12);
	assertEqualStmt(stmt,(const char *)gvarcharfield,"testvarchar4");
	erg=SQLGetData(stmt,4,SQL_C_TYPE_TIMESTAMP,
			&gdatefield,sizeof(gdatefield),&gdateind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gdateind,(int)sizeof(SQL_TIMESTAMP_STRUCT));
	assertEqualStmt(stmt,(int)gdatefield.year,2004);
	assertEqualStmt(stmt,(int)gdatefield.month,1);
	assertEqualStmt(stmt,(int)gdatefield.day,1);
	assertEqualStmt(stmt,(int)gdatefield.hour,4);
	assertEqualStmt(stmt,(int)gdatefield.minute,5);
	assertEqualStmt(stmt,(int)gdatefield.second,6);
	assertEqualStmt(stmt,(int)gdatefield.fraction,0);
	erg=SQLGetData(stmt,5,SQL_C_CHAR,
			glongfield,sizeof(glongfield),&glongind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)glongind,9);
	assertEqualStmt(stmt,(const char *)glongfield,"testtext4");
	erg=SQLGetData(stmt,6,SQL_C_CHAR,
			gclobfield,sizeof(gclobfield),&gclobind);
	assertSuccessStmt(stmt,erg);
	// row 4 was SQLPutData'd with length 9, so the CLOB is genuinely
	// 9 bytes in both modes
	assertEqualStmt(stmt,(int)gclobind,9);
	assertEqualStmt(stmt,(const char *)gclobfield,"testclob4");
	erg=SQLGetData(stmt,7,SQL_C_BINARY,
			gblobfield,sizeof(gblobfield),&gblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(gblobfield,"testblob4",9));

	// no more rows
	erg=SQLFetch(stmt);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NO_DATA);
	stdoutput.printf("\n");



	// column info - not null
	stdoutput.printf("COLUMN INFO - not null: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable2",SQL_NTS);
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



	// nested selects
	stdoutput.printf("NESTED SELECTS: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	// save initial SQL_ROWSET_SIZE to restore later; unixODBC's driver
	// manager rejects a set to 0 with HY024
	erg=SQLGetStmtAttr(stmt,SQL_ROWSET_SIZE,
			(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLSetStmtAttr(stmt,SQL_ROWSET_SIZE,
			(SQLPOINTER)(uintptr_t)1,0);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
			"select * from testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLHSTMT	nestedstmt;
	erg=SQLAllocHandle(SQL_HANDLE_STMT,dbc,&nestedstmt);
	assertSuccessDbc(dbc,erg);
	erg=SQLSetStmtAttr(nestedstmt,SQL_ROWSET_SIZE,
			(SQLPOINTER)(uintptr_t)1,0);
	assertSuccessStmt(nestedstmt,erg);
	int	nestedrows=0;
	for (;;) {
		erg=SQLFetch(stmt);
		if (erg==SQL_NO_DATA) {
			break;
		}
		assertSuccessStmt(stmt,erg);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			break;
		}
		// close the previous nested cursor before re-executing on the
		// same statement handle
		if (nestedrows>0) {
			erg=SQLFreeStmt(nestedstmt,SQL_CLOSE);
			assertSuccessStmt(nestedstmt,erg);
		}
		erg=SQLExecDirect(nestedstmt,(SQLCHAR *)
				"select * from testtable",SQL_NTS);
		assertSuccessStmt(nestedstmt,erg);
		nestedrows++;
	}
	erg=SQLFreeHandle(SQL_HANDLE_STMT,nestedstmt);
	assertSuccessStmt(nestedstmt,erg);
	assertEqualStmt(stmt,nestedrows,4);
	// restore the initial rowset size
	if (stmtinitial>0) {
		erg=SQLSetStmtAttr(stmt,SQL_ROWSET_SIZE,
				(SQLPOINTER)(uintptr_t)stmtinitial,0);
		assertSuccessStmt(stmt,erg);
	}
	stdoutput.printf("\n");



	// commit and rollback
	// informix has no MVCC; the second connection uses dirty-read isolation
	// so its count(*) sees the first connection's uncommitted rows instead
	// of blocking on the in-flight transaction's row locks - this verifies
	// dirty-read semantics and that commit/rollback persist/discard rows
	stdoutput.printf("COMMIT AND ROLLBACK: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);

	// reset testtable to a known-empty state (autocommit is on so the
	// create commits)
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable (testint integer)",SQL_NTS);
	assertSuccessStmt(stmt,erg);

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

	// dirty-read isolation lets the second connection's count(*) see
	// uncommitted rows without blocking on the writer's row locks
	erg=SQLExecDirect(stmt2,(SQLCHAR *)
		"set isolation to dirty read",SQL_NTS);
	assertSuccessStmt(stmt2,erg);

	// switch dbc to manual commit and insert four uncommitted rows
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
		(SQLPOINTER)SQL_AUTOCOMMIT_OFF,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into testtable values (1)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into testtable values (2)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into testtable values (3)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into testtable values (4)",SQL_NTS);
	assertSuccessStmt(stmt,erg);

	// get row count (dirty read sees the 4 uncommitted rows)
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
	assertEqualStmt(stmt2,(int)rowcount,4);

	// commit on dbc
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);
	assertSuccessDbc(dbc,erg);

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

	// insert another row on dbc, then roll it back
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into testtable values (5)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_ROLLBACK);
	assertSuccessDbc(dbc,erg);

	// get row count (should still be 4, the rolled-back row is gone)
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
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into testtable values (5)",SQL_NTS);
	assertSuccessStmt(stmt,erg);

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

	// clean up and disconnect; leave dbc in autocommit-on mode
	SQLFreeHandle(SQL_HANDLE_STMT,stmt2);
	SQLDisconnect(dbc2);
	SQLFreeHandle(SQL_HANDLE_DBC,dbc2);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// null values
	stdoutput.printf("NULL VALUES: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	// informix needs an explicit type for a NULL literal and a from clause
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select NULL::int,1,NULL::int from sysmaster:sysdual",
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
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,2,SQL_C_SLONG,
			&intfield,sizeof(intfield),&intind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,3,SQL_C_CHAR,
			nullfield2,sizeof(nullfield2),&nullind2);
	assertSuccessStmt(stmt,erg);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)nullind1,(int)SQL_NULL_DATA);
	assertEqualStmt(stmt,(int)intfield,1);
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
		"	testclob1 text, "
		"	testclob2 text, "
		"	testblob1 byte, "
		"	testblob2 byte)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLPrepare(stmt,(SQLCHAR *)
		"insert into testtable values (?,?,?,?)",
		SQL_NTS);
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
	// a genuine NULL round-trips as SQL_NULL_DATA in both modes; an empty
	// lob round-trips with length 1 through sqlrelay (which stores a
	// zero-length text/byte as a single byte) but with length 0 through the
	// native driver
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)lobclob1ind,1);
		assertEqualStmt(stmt,(int)lobblob1ind,1);
	} else {
		assertEqualStmt(stmt,(int)lobclob1ind,0);
		assertEqualStmt(stmt,(int)lobblob1ind,0);
	}
	assertEqualStmt(stmt,(int)lobclob2ind,(int)SQL_NULL_DATA);
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
		"	testclob text, "
		"	testblob byte)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	erg=SQLPrepare(stmt,(SQLCHAR *)
		"insert into testtable values (?,?)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLLEN	largecloblen=LARGE_BUFFER_LENGTH;
	SQLLEN	largebloblen=LARGE_BUFFER_LENGTH;
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				LARGE_BUFFER_LENGTH,0,
				(SQLPOINTER)largebuffer,
				LARGE_BUFFER_LENGTH,&largecloblen);
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
	SQLCHAR	largeclobout[LARGE_BUFFER_LENGTH+1];
	SQLLEN	largeclobind;
	SQLCHAR	largeblobout[LARGE_BUFFER_LENGTH+1];
	SQLLEN	largeblobind;
	erg=SQLBindCol(stmt,1,SQL_C_CHAR,
			largeclobout,sizeof(largeclobout),&largeclobind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,2,SQL_C_BINARY,
			largeblobout,sizeof(largeblobout),&largeblobind);
	assertSuccessStmt(stmt,erg);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)largeclobind,LARGE_BUFFER_LENGTH);
	assertTrueStmt(stmt,
		!bytestring::compare(largeclobout,largebuffer,
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
		"	testclob text, "
		"	testblob byte)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='D';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	erg=SQLPrepare(stmt,(SQLCHAR *)
		"insert into testtable values (?,?)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLLEN	largeclobdaelen=SQL_LEN_DATA_AT_EXEC(LARGE_BUFFER_LENGTH);
	SQLLEN	largeblobdaelen=SQL_LEN_DATA_AT_EXEC(LARGE_BUFFER_LENGTH);
	// the application tokens we expect SQLParamData to hand back
	const char	*clobtoken="largeclob";
	const char	*blobtoken="largeblob";
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				LARGE_BUFFER_LENGTH,0,
				(SQLPOINTER)clobtoken,
				0,&largeclobdaelen);
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
	// CLOB: SQLParamData hands back the CLOB token, then 4 chunks
	erg=SQLParamData(stmt,&largetoken);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NEED_DATA);
	assertEqualStmt(stmt,(const char *)largetoken,clobtoken);
	for (int i=0; i<4; i++) {
		erg=SQLPutData(stmt,
				largebuffer+i*LARGE_CHUNK_LENGTH,
				LARGE_CHUNK_LENGTH);
		assertSuccessStmt(stmt,erg);
	}
	// BLOB: next SQLParamData hands back the BLOB token, then 4 chunks
	erg=SQLParamData(stmt,&largetoken);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NEED_DATA);
	assertEqualStmt(stmt,(const char *)largetoken,blobtoken);
	for (int i=0; i<4; i++) {
		erg=SQLPutData(stmt,
				largebuffer+i*LARGE_CHUNK_LENGTH,
				LARGE_CHUNK_LENGTH);
		assertSuccessStmt(stmt,erg);
	}
	// final SQLParamData completes execution
	erg=SQLParamData(stmt,&largetoken);
	assertSuccessStmt(stmt,erg);
	// fetch the row back and verify it matches the source buffer
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"select * from testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	// re-init output buffers
	bytestring::zero(largeclobout,sizeof(largeclobout));
	largeclobind=-1;
	bytestring::zero(largeblobout,sizeof(largeblobout));
	largeblobind=-1;
	erg=SQLBindCol(stmt,1,SQL_C_CHAR,
			largeclobout,sizeof(largeclobout),&largeclobind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,2,SQL_C_BINARY,
			largeblobout,sizeof(largeblobout),&largeblobind);
	assertSuccessStmt(stmt,erg);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)largeclobind,LARGE_BUFFER_LENGTH);
	assertTrueStmt(stmt,
		!bytestring::compare(largeclobout,largebuffer,
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
	stdoutput.printf("OUTPUT BIND BY POSITION: \n");
	// A prior data-at-exec SQLPutData (the LONG LOBS sections above) leaves
	// the native informix driver unable to populate output-parameter binds
	// for the rest of the session; a fresh statement handle clears that
	// state (SQLFreeStmt/SQLCancel/rollback do not).  The output-bind tests
	// that follow all reuse this handle, and nothing below uses SQLPutData.
	if (!issqlrelay) {
		SQLFreeHandle(SQL_HANDLE_STMT,stmt);
		erg=SQLAllocHandle(SQL_HANDLE_STMT,dbc,&stmt);
		assertSuccessDbc(dbc,erg);
	}
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create procedure testproc("
		"	out out1 int, "
		"	out out2 varchar(20), "
		"	out out3 float, "
		"	out out4 varchar(20)) "
		"let out1 = 1; "
		"let out2 = 'hello'; "
		"let out3 = 2.5; "
		"let out4 = null; "
		"end procedure;",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLPrepare(stmt,(SQLCHAR *)
		"{call testproc(?,?,?,?)}",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLNumParams(stmt,&bindvarcount);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)bindvarcount,4);
	SQLINTEGER	obnumvar=0;
	SQLLEN		obnumvarind=0;
	SQLCHAR		obstringvar[11]={0};
	SQLLEN		obstringvarind=10;
	SQLDOUBLE	obfloatvar=0.0;
	SQLLEN		obfloatvarind=0;
	SQLCHAR		obnullvar[11]={0};
	SQLLEN		obnullvarind=10;
	erg=SQLBindParameter(stmt,1,SQL_PARAM_OUTPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&obnumvar,
				sizeof(obnumvar),&obnumvarind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_OUTPUT,
				SQL_C_CHAR,SQL_VARCHAR,
				20,0,
				(SQLPOINTER)obstringvar,
				sizeof(obstringvar),&obstringvarind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,3,SQL_PARAM_OUTPUT,
				SQL_C_DOUBLE,SQL_DOUBLE,
				0,0,
				(SQLPOINTER)&obfloatvar,
				sizeof(obfloatvar),&obfloatvarind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,4,SQL_PARAM_OUTPUT,
				SQL_C_CHAR,SQL_VARCHAR,
				20,0,
				(SQLPOINTER)obnullvar,
				sizeof(obnullvar),&obnullvarind);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)obnumvar,1);
	assertEqualStmt(stmt,(int)obnumvarind,(int)sizeof(SQLINTEGER));
	assertEqualStmt(stmt,(const char *)obstringvar,"hello");
	assertEqualStmt(stmt,(int)obstringvarind,5);
	assertTrueStmt(stmt,obfloatvar==2.5);
	assertEqualStmt(stmt,(int)obfloatvarind,(int)sizeof(SQLDOUBLE));
	assertEqualStmt(stmt,(int)obnullvarind,(int)SQL_NULL_DATA);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// lob output bind
	// informix can't pass a simple large object (text/byte) as an spl
	// procedure parameter, so the smart large objects (clob/blob) are used
	stdoutput.printf("LOB OUTPUT BIND: \n");
	// The native informix odbc driver cannot bind informix smart large
	// objects (clob/blob) through standard odbc: SQL_C_CHAR/SQL_C_BINARY
	// binds of clob/blob columns fail with "07006 Restricted data type
	// attribute violation" (and segfault the driver outright when ColumnSize
	// and BufferLength are 0).  Smart lobs are the only lob type that can be
	// an spl out parameter (text/byte cannot), so there is no native
	// equivalent of this output-bind test; the LONG OUTPUT BIND section below
	// exercises output binds for the native driver via lvarchar.
	if (issqlrelay) {
		SQLFreeStmt(stmt,SQL_CLOSE);
		SQLFreeStmt(stmt,SQL_UNBIND);
		SQLFreeStmt(stmt,SQL_RESET_PARAMS);
		SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
		erg=SQLExecDirect(stmt,(SQLCHAR *)
			"create table testtable ("
			"	testclob clob, "
			"	testblob blob)",
			SQL_NTS);
		assertSuccessStmt(stmt,erg);
		erg=SQLPrepare(stmt,(SQLCHAR *)
			"insert into testtable values (?,?)",
			SQL_NTS);
		assertSuccessStmt(stmt,erg);
		SQLCHAR		lobclobin[6]="hello";
		SQLLEN		lobclobinlen=5;
		SQLCHAR		lobblobin[6]="hello";
		SQLLEN		lobblobinlen=5;
		erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
					SQL_C_CHAR,SQL_LONGVARCHAR,
					0,0,
					(SQLPOINTER)lobclobin,
					0,&lobclobinlen);
		assertSuccessStmt(stmt,erg);
		erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
					SQL_C_BINARY,SQL_LONGVARBINARY,
					0,0,
					(SQLPOINTER)lobblobin,
					0,&lobblobinlen);
		assertSuccessStmt(stmt,erg);
		erg=SQLExecute(stmt);
		assertSuccessStmt(stmt,erg);
		SQLFreeStmt(stmt,SQL_CLOSE);
		SQLFreeStmt(stmt,SQL_UNBIND);
		SQLFreeStmt(stmt,SQL_RESET_PARAMS);
		// informix returns the lobs through out parameters of an spl procedure
		// that selects them into the bind variables
		SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc",SQL_NTS);
		erg=SQLExecDirect(stmt,(SQLCHAR *)
			"create procedure testproc("
			"	out out1 clob, "
			"	out out2 blob) "
			"select testclob, testblob "
			"	into out1, out2 "
			"	from testtable; "
			"end procedure;",
			SQL_NTS);
		assertSuccessStmt(stmt,erg);
		erg=SQLPrepare(stmt,(SQLCHAR *)
			"{call testproc(?,?)}",SQL_NTS);
		assertSuccessStmt(stmt,erg);
		SQLCHAR		lobclobout[1024]={0};
		SQLLEN		lobclobout_ind=sizeof(lobclobout);
		SQLCHAR		lobblobout[1024]={0};
		SQLLEN		lobblobout_ind=sizeof(lobblobout);
		erg=SQLBindParameter(stmt,1,SQL_PARAM_OUTPUT,
					SQL_C_CHAR,SQL_LONGVARCHAR,
					sizeof(lobclobout),0,
					(SQLPOINTER)lobclobout,
					sizeof(lobclobout),&lobclobout_ind);
		assertSuccessStmt(stmt,erg);
		erg=SQLBindParameter(stmt,2,SQL_PARAM_OUTPUT,
					SQL_C_BINARY,SQL_LONGVARBINARY,
					sizeof(lobblobout),0,
					(SQLPOINTER)lobblobout,
					sizeof(lobblobout),&lobblobout_ind);
		assertSuccessStmt(stmt,erg);
		erg=SQLExecute(stmt);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)lobclobout_ind,5);
		assertTrueStmt(stmt,
			!bytestring::compare(lobclobout,"hello",5));
		assertEqualStmt(stmt,(int)lobblobout_ind,5);
		assertTrueStmt(stmt,
			!bytestring::compare(lobblobout,"hello",5));
		SQLFreeStmt(stmt,SQL_CLOSE);
		SQLFreeStmt(stmt,SQL_UNBIND);
		SQLFreeStmt(stmt,SQL_RESET_PARAMS);
		SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc",SQL_NTS);
		erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
		assertSuccessStmt(stmt,erg);
	}
	stdoutput.printf("\n");



	// long output bind
	stdoutput.printf("LONG OUTPUT BIND: \n");
	// The native informix odbc driver mishandles large lvarchar output
	// binds: SQLExecute of an spl "out lvarchar(8192)" parameter returns
	// "22001 String data right truncation" with an exact-size buffer and an
	// "08S01 Communication link failure" (or a protocol-level hang) with a
	// larger one.  lvarchar is the only non-lob type that can be an spl out
	// parameter, so there is no native equivalent of this large-output-bind
	// test.
	if (issqlrelay) {
		SQLFreeStmt(stmt,SQL_CLOSE);
		SQLFreeStmt(stmt,SQL_UNBIND);
		SQLFreeStmt(stmt,SQL_RESET_PARAMS);
		for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
			largebuffer[i]='C';
		}
		largebuffer[LARGE_BUFFER_LENGTH]='\0';
		// informix returns the long value through an lvarchar out parameter of
		// an spl procedure that copies its input to its output; lvarchar is a
		// non-lob long varchar, so (unlike text/byte) it can be an spl
		// parameter, and the input is bound as SQL_VARCHAR (not SQL_LONGVARCHAR,
		// which the driver would route to the lob bind path) to match it
		SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc",SQL_NTS);
		char	longoutproc[256];
		charstring::printf(longoutproc,sizeof(longoutproc),
				"create procedure testproc("
				"	in1 lvarchar(%d), "
				"	out out1 lvarchar(%d)) "
				"let out1 = in1; "
				"end procedure;",
				LARGE_BUFFER_LENGTH,LARGE_BUFFER_LENGTH);
		erg=SQLExecDirect(stmt,(SQLCHAR *)longoutproc,SQL_NTS);
		assertSuccessStmt(stmt,erg);
		erg=SQLPrepare(stmt,(SQLCHAR *)"{call testproc(?,?)}",SQL_NTS);
		assertSuccessStmt(stmt,erg);
		SQLLEN	longinind=LARGE_BUFFER_LENGTH;
		erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
					SQL_C_CHAR,SQL_VARCHAR,
					LARGE_BUFFER_LENGTH,0,
					(SQLPOINTER)largebuffer,
					LARGE_BUFFER_LENGTH,&longinind);
		assertSuccessStmt(stmt,erg);
		SQLCHAR	longoutval[LARGE_BUFFER_LENGTH+1]={0};
		SQLLEN	longoutind=sizeof(longoutval);
		erg=SQLBindParameter(stmt,2,SQL_PARAM_OUTPUT,
					SQL_C_CHAR,SQL_LONGVARCHAR,
					LARGE_BUFFER_LENGTH,0,
					(SQLPOINTER)longoutval,
					sizeof(longoutval),&longoutind);
		assertSuccessStmt(stmt,erg);
		erg=SQLExecute(stmt);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)longoutind,LARGE_BUFFER_LENGTH);
		assertTrueStmt(stmt,
			!bytestring::compare(longoutval,largebuffer,
						LARGE_BUFFER_LENGTH));
		SQLFreeStmt(stmt,SQL_CLOSE);
		SQLFreeStmt(stmt,SQL_UNBIND);
		SQLFreeStmt(stmt,SQL_RESET_PARAMS);
		SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc",SQL_NTS);
	}
	stdoutput.printf("\n");



	// negative input bind
	stdoutput.printf("NEGATIVE INPUT BIND: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable (testval integer)",SQL_NTS);
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
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	// an spl procedure that converts its int input to a char output, called
	// via the ODBC {call} escape with params bound before prepare
	SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create procedure testproc("
		"	in1 int, "
		"	out out1 varchar(20)) "
		"let out1 = in1; "
		"end procedure;",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);

	// bind, prepare, execute
	SQLINTEGER	bbpinval=42;
	SQLLEN		bbpinind=0;
	SQLCHAR		bbpoutval[16]={0};
	SQLLEN		bbpoutind=sizeof(bbpoutval);
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&bbpinval,
				bbpinind,&bbpinind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_OUTPUT,
				SQL_C_CHAR,SQL_VARCHAR,
				sizeof(bbpoutval),0,
				(SQLPOINTER)bbpoutval,
				sizeof(bbpoutval),&bbpoutind);
	assertSuccessStmt(stmt,erg);
	erg=SQLPrepare(stmt,(SQLCHAR *)"{call testproc(?,?)}",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)bbpoutval,"42");

	// bind, exec-direct
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	bbpinval=99;
	bytestring::zero(bbpoutval,sizeof(bbpoutval));
	bbpoutind=sizeof(bbpoutval);
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&bbpinval,
				bbpinind,&bbpinind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_OUTPUT,
				SQL_C_CHAR,SQL_VARCHAR,
				sizeof(bbpoutval),0,
				(SQLPOINTER)bbpoutval,
				sizeof(bbpoutval),&bbpoutind);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"{call testproc(?,?)}",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)bbpoutval,"99");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// rebinding
	stdoutput.printf("REBINDING: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	// an spl procedure that copies its input to its output, re-executed with
	// a rebound input each iteration; the input parameter is declared first
	// so the input-then-output bind order matches the procedure signature
	SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create procedure testproc("
		"	in1 int, "
		"	out out1 int) "
		"let out1 = in1; "
		"end procedure;",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLPrepare(stmt,(SQLCHAR *)"{call testproc(?,?)}",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLINTEGER	rebindout=0;
	SQLLEN		rebindoutind=0;
	SQLINTEGER	rebindin=1;
	SQLLEN		rebindinlen=sizeof(rebindin);
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&rebindin,
				rebindinlen,&rebindinlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_OUTPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&rebindout,
				sizeof(rebindout),&rebindoutind);
	assertSuccessStmt(stmt,erg);
	for (int rb=1; rb<=3; rb++) {
		rebindin=rb;
		erg=SQLExecute(stmt);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)rebindout,rb);
	}
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// reexecute
	stdoutput.printf("REEXECUTE: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLPrepare(stmt,(SQLCHAR *)
		"select 1 from sysmaster:sysdual",SQL_NTS);
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
	erg=SQLPrepare(stmt,(SQLCHAR *)
		"select ?::int from sysmaster:sysdual",SQL_NTS);
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
		"create table testtable (testval integer)",SQL_NTS);
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
		"create table testtable (col1 integer, col2 integer)",SQL_NTS);
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
	// execute a parameterless statement WITHOUT clearing the binds -
	// the leftover bindings must not be sent to the backend
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into testtable values (33,44)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLFreeStmt(stmt,SQL_CLOSE);
	// fewer-but-nonzero path: binds 1 and 2 are still stashed, but
	// this statement has only one marker, so only bind 1 should apply
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
		"create table testtable (col1 byte)",SQL_NTS);
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
	// both sqlrelay and the informix native driver implement
	// SQL_ALL_CATALOGS
	assertSuccessStmt(stmt,erg);
	// informix databases are catalogs; the connected database
	// (named after this host) should appear in the list
	SQLCHAR		catname[1024];
	SQLLEN		catnameind;
	erg=SQLBindCol(stmt,1,SQL_C_CHAR,
			catname,sizeof(catname),&catnameind);
	assertSuccessStmt(stmt,erg);
	bool		catfound=false;
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
	}
	assertTrueStmt(stmt,catfound);
	SQLFreeStmt(stmt,SQL_CLOSE);
	stdoutput.printf("\n");



	// schema list
	stdoutput.printf("SCHEMA LIST: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	// informix reports object owners as schemas, so create a table to ensure
	// the connected user owns at least one object and thus appears
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable (testint integer)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
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
					(const char *)schname,"testuser")) {
			schfound=true;
		}
	}
	assertTrueStmt(stmt,schfound);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// table type list
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
		if (!charstring::compare(
					(const char *)tabletype,"TABLE")) {
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
		"create table testtable1 ("
		"	testint integer, "
		"	testchar char(40))",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable2 ("
		"	testint integer, "
		"	testchar char(40))",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable3 ("
		"	testint integer, "
		"	testchar char(40))",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable4 ("
		"	testint integer, "
		"	testchar char(40))",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	// informix folds unquoted identifiers to lowercase
	erg=SQLTables(stmt,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)"testtable%",SQL_NTS,
			(SQLCHAR *)"TABLE",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		tblname[256];
	SQLLEN		tblnameind;
	erg=SQLBindCol(stmt,3,SQL_C_CHAR,
			tblname,sizeof(tblname),&tblnameind);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		tbltype[256];
	SQLLEN		tbltypeind;
	erg=SQLBindCol(stmt,4,SQL_C_CHAR,
			tbltype,sizeof(tbltype),&tbltypeind);
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
			// filtered on TABLE, so each match must report that type
			assertEqualStmt(stmt,(const char *)tbltype,"TABLE");
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
	stdoutput.printf("TYPE INFO LIST: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLGetTypeInfo(stmt,SQL_ALL_TYPES);
	assertSuccessStmt(stmt,erg);
	// walk through the list and find integer, char, varchar, datetime
	SQLCHAR		typname[64];
	SQLLEN		typnameind;
	erg=SQLBindCol(stmt,1,SQL_C_CHAR,
			typname,sizeof(typname),&typnameind);
	assertSuccessStmt(stmt,erg);
	SQLSMALLINT	typdatatype;
	SQLLEN		typdatatypeind;
	erg=SQLBindCol(stmt,2,SQL_C_SHORT,&typdatatype,
			sizeof(typdatatype),&typdatatypeind);
	assertSuccessStmt(stmt,erg);
	bool		foundinteger=false;
	bool		foundchar=false;
	bool		foundvarchar=false;
	bool		founddatetime=false;
	for (;;) {
		erg=SQLFetch(stmt);
		if (erg==SQL_NO_DATA) {
			break;
		}
		assertSuccessStmt(stmt,erg);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			break;
		}
		// the native driver lists DATETIME under several qualified
		// names (with different time/timestamp codes), so only the
		// clean 1:1 names get a type-code check (on first match)
		if (!charstring::compareIgnoringCase(
					(const char *)typname,"integer")) {
			if (!foundinteger) {
				assertEqualStmt(stmt,(int)typdatatype,
							SQL_INTEGER);
			}
			foundinteger=true;
		} else if (!charstring::compareIgnoringCase(
					(const char *)typname,"char")) {
			if (!foundchar) {
				assertEqualStmt(stmt,(int)typdatatype,SQL_CHAR);
			}
			foundchar=true;
		} else if (!charstring::compareIgnoringCase(
					(const char *)typname,"varchar")) {
			if (!foundvarchar) {
				assertEqualStmt(stmt,(int)typdatatype,
							SQL_VARCHAR);
			}
			foundvarchar=true;
		} else if (!charstring::compareIgnoringCase(
					(const char *)typname,"datetime",8)) {
			// sqlrelay reports a plain "datetime" type; the native
			// driver reports qualified names like
			// "datetime year to fraction(5)", so match the prefix
			founddatetime=true;
		}
	}
	assertTrueStmt(stmt,foundinteger);
	assertTrueStmt(stmt,foundchar);
	assertTrueStmt(stmt,foundvarchar);
	assertTrueStmt(stmt,founddatetime);
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
		"	testint integer, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testdatetime datetime year to second, "
		"	testtext text, "
		"	testclob text, "
		"	testblob byte)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	// informix's catalog functions are scoped to the table owner; tables
	// this user creates are owned by the connected user
	char	schemafilter[64]={0};
	if (!issqlrelay) {
		charstring::copy(schemafilter,"testuser");
	}
	erg=SQLColumns(stmt,
			NULL,0,
			(SQLCHAR *)schemafilter,SQL_NTS,
			(SQLCHAR *)"testtable",SQL_NTS,
			NULL,0);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		clcolname[64];
	SQLLEN		clcolnameind;
	SQLCHAR		clcoltype[64];
	SQLLEN		clcoltypeind;
	erg=SQLBindCol(stmt,4,SQL_C_CHAR,
			clcolname,sizeof(clcolname),&clcolnameind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,6,SQL_C_CHAR,
			clcoltype,sizeof(clcoltype),&clcoltypeind);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		clcat[64];
	SQLLEN		clcatind;
	erg=SQLBindCol(stmt,1,SQL_C_CHAR,
			clcat,sizeof(clcat),&clcatind);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		clschem[64];
	SQLLEN		clschemind;
	erg=SQLBindCol(stmt,2,SQL_C_CHAR,
			clschem,sizeof(clschem),&clschemind);
	assertSuccessStmt(stmt,erg);
	const char	*expcols[]={"testint","testchar","testvarchar",
				"testdatetime","testtext","testclob","testblob"};
	// the native driver spells out the datetime precision
	const char	*expcoltypessqlr[]={"INTEGER","CHAR","VARCHAR",
				"DATETIME","TEXT","TEXT","BYTE"};
	const char	*expcoltypesnative[]={"INTEGER","CHAR","VARCHAR",
				"DATETIME YEAR TO SECOND","TEXT","TEXT","BYTE"};
	const char	**expcoltypes=(issqlrelay)?
				expcoltypessqlr:expcoltypesnative;
	for (int c=0; c<7; c++) {
		erg=SQLFetch(stmt);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(const char *)clcolname,expcols[c]);
		assertEqualStmt(stmt,(const char *)clcoltype,expcoltypes[c]);
		// #7971 - informix returns the current catalog (the database)
		assertTrueStmt(stmt,clcatind!=SQL_NULL_DATA && clcat[0]!='\0');
		assertTrueStmt(stmt,clschemind!=SQL_NULL_DATA &&
					clschem[0]!='\0');
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
		"	col1 integer primary key, "
		"	col2 integer)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLPrimaryKeys(stmt,
			NULL,0,
			(SQLCHAR *)schemafilter,SQL_NTS,
			(SQLCHAR *)"testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		pkcolname[64];
	SQLLEN		pkcolnameind;
	erg=SQLBindCol(stmt,4,SQL_C_CHAR,
			pkcolname,sizeof(pkcolname),&pkcolnameind);
	assertSuccessStmt(stmt,erg);
	SQLSMALLINT	pkkeyseq;
	SQLLEN		pkkeyseqind;
	erg=SQLBindCol(stmt,5,SQL_C_SHORT,&pkkeyseq,
			sizeof(pkkeyseq),&pkkeyseqind);
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
			// single-column key, so col1 is at sequence 1
			assertEqualStmt(stmt,(int)pkkeyseq,1);
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
		"	col1 integer, "
		"	col2 integer, "
		"	primary key (col1) constraint testtable_pk)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLPrimaryKeys(stmt,
			NULL,0,
			(SQLCHAR *)schemafilter,SQL_NTS,
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
	assertEqualStmt(stmt,(int)pknameind,12);
	assertEqualStmt(stmt,(const char *)pkname,"testtable_pk");
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
		"	col1 integer primary key, "
		"	col2 integer)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLStatistics(stmt,
			NULL,0,
			(SQLCHAR *)schemafilter,SQL_NTS,
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
	// SQLStatistics may return a SQL_TABLE_STAT row (type=0, no
	// column) interleaved with the index rows; walk the result set
	// and look for the COL1 index row.
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
	SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc1",SQL_NTS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc2",SQL_NTS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc3",SQL_NTS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc4",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create procedure testproc1 ("
		"	in1 int, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 datetime year to second) "
		"end procedure;",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create procedure testproc2 ("
		"	in1 int, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 datetime year to second) "
		"end procedure;",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create procedure testproc3 ("
		"	in1 int, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 datetime year to second) "
		"end procedure;",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create procedure testproc4 ("
		"	in1 int, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 datetime year to second) "
		"end procedure;",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLProcedures(stmt,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)schemafilter,SQL_NTS,
			(SQLCHAR *)"testproc%",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		procname[64];
	SQLLEN		procnameind;
	erg=SQLBindCol(stmt,3,SQL_C_CHAR,
			procname,sizeof(procname),&procnameind);
	assertSuccessStmt(stmt,erg);
	SQLSMALLINT	proctype;
	SQLLEN		proctypeind;
	erg=SQLBindCol(stmt,8,SQL_C_SHORT,&proctype,
			sizeof(proctype),&proctypeind);
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
			// created as procedures, not functions
			assertEqualStmt(stmt,(int)proctype,SQL_PT_PROCEDURE);
			proccounter++;
		}
	}
	assertEqualStmt(stmt,proccounter,4);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	stdoutput.printf("\n");



	// procedure parameter list
	stdoutput.printf("PROCEDURE PARAMETER LIST: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLProcedureColumns(stmt,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)schemafilter,SQL_NTS,
			(SQLCHAR *)"testproc1",SQL_NTS,
			(SQLCHAR *)"",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	if (issqlrelay) {
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
		// in1
		erg=SQLFetch(stmt);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(const char *)ppname,"in1");
		assertEqualStmt(stmt,(int)ppmode,(int)SQL_PARAM_INPUT);
		assertEqualStmt(stmt,(const char *)pptypename,"integer");
		assertEqualStmt(stmt,(int)ppordinal,1);
		// in2
		erg=SQLFetch(stmt);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(const char *)ppname,"in2");
		assertEqualStmt(stmt,(int)ppmode,(int)SQL_PARAM_INPUT);
		assertEqualStmt(stmt,(const char *)pptypename,"char");
		assertEqualStmt(stmt,(int)ppordinal,2);
		// in3
		erg=SQLFetch(stmt);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(const char *)ppname,"in3");
		assertEqualStmt(stmt,(int)ppmode,(int)SQL_PARAM_INPUT);
		assertEqualStmt(stmt,(const char *)pptypename,"varchar");
		assertEqualStmt(stmt,(int)ppordinal,3);
		// in4
		erg=SQLFetch(stmt);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(const char *)ppname,"in4");
		assertEqualStmt(stmt,(int)ppmode,(int)SQL_PARAM_INPUT);
		assertEqualStmt(stmt,(const char *)pptypename,"datetime");
		assertEqualStmt(stmt,(int)ppordinal,4);
	} else {
		// The native informix driver's SQLProcedureColumns returns no
		// rows for SPL procedures even though the parameter metadata
		// exists in sysproccolumns (a driver bug), so the first fetch
		// returns SQL_NO_DATA.
		erg=SQLFetch(stmt);
		assertEqualStmt(stmt,(int)erg,(int)SQL_NO_DATA);
	}
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
