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

	static const char * const db2osprefixes[]={
					"centos","co",
					"debian","db",
					"fedora","fc",
					"freebsd","fb",
					"haiku","hk",
					"openbsd","ob",
					"netbsd","nb",
					"opensuse","os",
					"solaris","sl",
					"syllable","sb",
					"ubuntu","u",
					NULL};
	for (int pi=0; db2osprefixes[pi]; pi=pi+2) {
		size_t	prefixlen=charstring::getLength(db2osprefixes[pi]);
		if (charstring::getLength(hostname)>=prefixlen &&
			!charstring::compare(hostname,
						db2osprefixes[pi],prefixlen)) {
			stringbuffer	shortname;
			shortname.append(db2osprefixes[pi+1])->
					append(hostname+prefixlen);
			charstring::copy(hostname,shortname.getString());
			break;
		}
	}

	static const char * const db2archsuffixes[]={
					"alpha","a",
					"mipsel","m32",
					"mips64el","m64",
					"hppa","hp",
					"i386","x86",
					"sparc","s32",
					"sparc64","s64",
					NULL};
	size_t	hostnamelen=charstring::getLength(hostname);
	for (int si=0; db2archsuffixes[si]; si=si+2) {
		size_t	suffixlen=charstring::getLength(db2archsuffixes[si]);
		if (hostnamelen>=suffixlen &&
			!charstring::compare(
					hostname+hostnamelen-suffixlen,
					db2archsuffixes[si])) {
			charstring::copy(hostname+hostnamelen-suffixlen,
						db2archsuffixes[si+1]);
			break;
		}
	}

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
	// must be set pre-connect, can't be read until post-connect; set only
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
			"User=db2inst1;Password=testpassword;"
			"NullsAsNulls=yes;"
			// for ODBC spec compliance
			"AutoCommit=yes;");
	} else {
		incstr.append(
			"Driver={DB2};"
			"Database=")->append(hostname)->append(";"
			"Uid=db2inst1;Pwd=testpassword;");
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
	stdoutput.printf("\n");



	// environment attributes (post-connect, forwarded to driver)
	stdoutput.printf("ENVIRONMENT ATTRIBUTES (post-connect): \n");


	// SQL_ATTR_ODBC_VERSION
	// can't be set once a connection handle exists (HY010); value sticks
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
	// some drivers don't store a READ_ONLY->READ_WRITE switch;
	// round-trip one direction and don't verify the restore
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
	// restore initial value
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
		// native db2 driver doesn't implement
		// SQL_ATTR_CONNECTION_TIMEOUT; get and set raise HYC00
		// "Driver not capable"
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
	// sqlrelay catalog functions always pattern-match, so SQL_TRUE is
	// substituted with SQL_FALSE (SQL_SUCCESS_WITH_INFO, SQLSTATE 01S02)
	stdoutput.printf("  SQL_ATTR_METADATA_ID\n");
	if (issqlrelay) {
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_METADATA_ID,
				(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcinitial,(int)SQL_FALSE);
		// SQL_TRUE
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_METADATA_ID,
				(SQLPOINTER)(uintptr_t)SQL_TRUE,0);
		assertEqualDbc(dbc,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_METADATA_ID,
				(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		// get reflects the substituted value, not what the app set
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_FALSE);
		// SQL_FALSE
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_METADATA_ID,
				(SQLPOINTER)(uintptr_t)SQL_FALSE,0);
		assertEqualDbc(dbc,(int)erg,(int)SQL_SUCCESS);
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_METADATA_ID,
				(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_FALSE);
		// restore initial value
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_METADATA_ID,
				(SQLPOINTER)(uintptr_t)dbcinitial,0);
		assertSuccessDbc(dbc,erg);
	} else {
		// native db2 driver doesn't implement connection-level
		// SQL_ATTR_METADATA_ID; get and set raise HYC00 "Driver not
		// capable"
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_METADATA_ID,
				(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
		assertFailureDbc(dbc,erg);
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_METADATA_ID,
				(SQLPOINTER)(uintptr_t)SQL_TRUE,0);
		assertFailureDbc(dbc,erg);
	}
	stdoutput.printf("\n");
	#endif


	// SQL_ATTR_TXN_ISOLATION
	// just confirm the default round-trips; ISOLATION LEVELS section
	// below exercises the individual db2 levels
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
	// often a no-op, but round-trip to initial value always succeeds
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_CURRENT_CATALOG,
			(SQLPOINTER)dbcstrinit,SQL_NTS);
	if (issqlrelay) {
		assertSuccessDbc(dbc,erg);
	} else {
		// native db2 driver doesn't allow setting the current catalog;
		// set raises HYC00 "Driver not capable"
		assertFailureDbc(dbc,erg);
	}
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
		// SQL_ASYNC_ENABLE_ON: unsupported, substituted with
		// SQL_ASYNC_ENABLE_OFF (SQLSTATE 01S02 "Option value changed")
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
		// native db2 driver supports connection-level async enable;
		// get returns the current mode and set-off succeeds
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)(uintptr_t)SQL_ASYNC_ENABLE_OFF,0);
		assertSuccessDbc(dbc,erg);
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
	if (issqlrelay) {
		// sqlrelay doesn't auto-populate the IPD
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_AUTO_IPD,
				(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_FALSE);
	} else {
		// the native db2 driver auto-populates the IPD
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_AUTO_IPD,
				(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
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
		// native db2 driver may not implement it; get raises HYT00
		// "Timeout expired", set raises HY003 "Invalid application
		// buffer type"
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
	if (issqlrelay) {
		// save initial value
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_TRANSLATE_LIB,
				(SQLPOINTER)dbcstrinit,
				sizeof(dbcstrinit),&dbcstrlen);
		assertSuccessDbc(dbc,erg);
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
	} else {
		// native db2 driver may not implement it; get returns
		// SQL_NO_DATA (100), buffer left unchanged
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_TRANSLATE_LIB,
				(SQLPOINTER)dbcstrinit,
				sizeof(dbcstrinit),&dbcstrlen);
		assertFailureDbc(dbc,erg);
	}
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_ATTR_TRANSLATE_OPTION
	stdoutput.printf("  SQL_ATTR_TRANSLATE_OPTION\n");
	if (issqlrelay) {
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
	} else {
		// native db2 driver doesn't implement
		// SQL_ATTR_TRANSLATE_OPTION; get raises HY092 "Option type out
		// of range" and set raises HYC00 "Driver not capable"
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_TRANSLATE_OPTION,
				(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
		assertFailureDbc(dbc,erg);
		erg=SQLSetConnectAttr(dbc,SQL_ATTR_TRANSLATE_OPTION,
				(SQLPOINTER)(uintptr_t)0,0);
		assertFailureDbc(dbc,erg);
	}
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
		// native db2 driver supports reading SQL_ATTR_ANSI_APP
		erg=SQLGetConnectAttr(dbc,SQL_ATTR_ANSI_APP,
				(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
	}
	stdoutput.printf("\n");
	#endif


	#if defined(SQL_ATTR_RESET_CONNECTION)
	// SQL_ATTR_RESET_CONNECTION
	stdoutput.printf("  SQL_ATTR_RESET_CONNECTION\n");
	// SQL_RESET_CONNECTION_YES (write-only; driver manager sets this
	// before reusing a pooled connection)
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_RESET_CONNECTION,
			(SQLPOINTER)(uintptr_t)
			SQL_RESET_CONNECTION_YES,0);
	assertSuccessDbc(dbc,erg);
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
		// native db2 driver supports connection-level async dbc
		// functions enable; get and set succeed
		erg=SQLGetConnectAttr(dbc,
				SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
				(SQLPOINTER)&dbcinitial,0,&dbcstrlen);
		assertSuccessDbc(dbc,erg);
		erg=SQLSetConnectAttr(dbc,
				SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
				(SQLPOINTER)(uintptr_t)
				SQL_ASYNC_DBC_ENABLE_ON,0);
		assertSuccessDbc(dbc,erg);
		// restore the off state so later connection functions stay
		// synchronous
		erg=SQLSetConnectAttr(dbc,
				SQL_ATTR_ASYNC_DBC_FUNCTIONS_ENABLE,
				(SQLPOINTER)(uintptr_t)
				SQL_ASYNC_DBC_ENABLE_OFF,0);
		assertSuccessDbc(dbc,erg);
	}
	stdoutput.printf("\n");
	#endif


	#if defined(SQL_ATTR_DRIVER_THREADING)
	// SQL_ATTR_DRIVER_THREADING
	// driver-reported threading level; SQL Relay reports 1 (thread-safe
	// per HDBC)
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
		// native db2 driver may not implement it; get and set raise
		// HYT00 "Timeout expired"
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
	} else {
		// native db2 reports 255
		assertEqualDbc(dbc,(int)uintval,255);
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
		// native driver returns the database/server name; verify non-empty
		assertTrueDbc(dbc,vallen>0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_SEARCH_PATTERN_ESCAPE
	stdoutput.printf("  SQL_SEARCH_PATTERN_ESCAPE\n");
	erg=SQLGetInfo(dbc,SQL_SEARCH_PATTERN_ESCAPE,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		// db2 uses backslash as the search pattern escape character
		assertEqualDbc(dbc,(const char *)strval,"\\");
	} else {
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_DBMS_NAME
	stdoutput.printf("  SQL_DBMS_NAME\n");
	erg=SQLGetInfo(dbc,SQL_DBMS_NAME,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"db2");
	} else {
		// native driver returns a platform-specific name
		// such as "DB2/LINUXX8664"
		assertTrueDbc(dbc,charstring::contains(
					(const char *)strval,"DB2"));
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_DBMS_VER
	stdoutput.printf("  SQL_DBMS_VER\n");
	erg=SQLGetInfo(dbc,SQL_DBMS_VER,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	// version string varies; just verify non-empty
	assertTrueDbc(dbc,vallen>0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ACCESSIBLE_TABLES
	stdoutput.printf("  SQL_ACCESSIBLE_TABLES\n");
	erg=SQLGetInfo(dbc,SQL_ACCESSIBLE_TABLES,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"N");
	} else {
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ACCESSIBLE_PROCEDURES
	stdoutput.printf("  SQL_ACCESSIBLE_PROCEDURES\n");
	erg=SQLGetInfo(dbc,SQL_ACCESSIBLE_PROCEDURES,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"N");
	} else {
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CURSOR_COMMIT_BEHAVIOR
	stdoutput.printf("  SQL_CURSOR_COMMIT_BEHAVIOR\n");
	erg=SQLGetInfo(dbc,SQL_CURSOR_COMMIT_BEHAVIOR,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	// db2 holds cursors open across a commit
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_CB_PRESERVE);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_DATA_SOURCE_READ_ONLY
	stdoutput.printf("  SQL_DATA_SOURCE_READ_ONLY\n");
	erg=SQLGetInfo(dbc,SQL_DATA_SOURCE_READ_ONLY,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"N");
	} else {
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
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
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_IC_UPPER);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_IDENTIFIER_QUOTE_CHAR
	stdoutput.printf("  SQL_IDENTIFIER_QUOTE_CHAR\n");
	erg=SQLGetInfo(dbc,SQL_IDENTIFIER_QUOTE_CHAR,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"\"");
	} else {
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
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
	assertEqualDbc(dbc,(int)usmallintval,128);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_SCHEMA_NAME_LEN
	stdoutput.printf("  SQL_MAX_SCHEMA_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_SCHEMA_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,128);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_CATALOG_NAME_LEN
	stdoutput.printf("  SQL_MAX_CATALOG_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_CATALOG_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,8);
	} else {
		// native db2 reports 0
		assertEqualDbc(dbc,(int)usmallintval,0);
	}
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
		// db2 reports no scroll concurrencies
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 11
		assertEqualDbc(dbc,(int)uintval,11);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_TXN_CAPABLE
	stdoutput.printf("  SQL_TXN_CAPABLE\n");
	erg=SQLGetInfo(dbc,SQL_TXN_CAPABLE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	// db2 supports both DDL and DML in transactions
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_TC_ALL);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_USER_NAME
	stdoutput.printf("  SQL_USER_NAME\n");
	erg=SQLGetInfo(dbc,SQL_USER_NAME,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertTrueDbc(dbc,!charstring::compareIgnoringCase(
					(const char *)strval,"db2inst1"));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_TXN_ISOLATION_OPTION
	stdoutput.printf("  SQL_TXN_ISOLATION_OPTION\n");
	erg=SQLGetInfo(dbc,SQL_TXN_ISOLATION_OPTION,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	// db2 supports all four isolation levels (UR, CS, RS, RR)
	assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_TXN_READ_UNCOMMITTED|
				SQL_TXN_READ_COMMITTED|
				SQL_TXN_REPEATABLE_READ|
				SQL_TXN_SERIALIZABLE));
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
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_GETDATA_EXTENSIONS
	stdoutput.printf("  SQL_GETDATA_EXTENSIONS\n");
	erg=SQLGetInfo(dbc,SQL_GETDATA_EXTENSIONS,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,
				(int)(SQL_GD_ANY_COLUMN|SQL_GD_ANY_ORDER|
					SQL_GD_BOUND|SQL_GD_BLOCK));
	} else {
		// native db2 reports 7
		assertEqualDbc(dbc,(int)uintval,7);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_NULL_COLLATION
	stdoutput.printf("  SQL_NULL_COLLATION\n");
	erg=SQLGetInfo(dbc,SQL_NULL_COLLATION,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,(int)SQL_NC_HIGH);
	} else {
		// native db2 reports 0
		assertEqualDbc(dbc,(int)usmallintval,0);
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
		// ADD_COLUMN, though most are orthogonal to ADD COLUMN; it
		// omits the DROP_COLUMN bits, which the db2 backend doesn't
		// report
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_AT_ADD_COLUMN|
				SQL_AT_ADD_COLUMN_SINGLE|
				SQL_AT_ADD_COLUMN_DEFAULT|
				SQL_AT_ADD_COLUMN_COLLATION|
				SQL_AT_SET_COLUMN_DEFAULT|
				SQL_AT_ADD_TABLE_CONSTRAINT|
				SQL_AT_CONSTRAINT_NAME_DEFINITION|
				SQL_AT_CONSTRAINT_INITIALLY_DEFERRED|
				SQL_AT_CONSTRAINT_INITIALLY_IMMEDIATE|
				SQL_AT_CONSTRAINT_DEFERRABLE|
				SQL_AT_CONSTRAINT_NON_DEFERRABLE));
	} else {
		// native db2 reports 61545
		assertEqualDbc(dbc,(int)uintval,61545);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ORDER_BY_COLUMNS_IN_SELECT
	stdoutput.printf("  SQL_ORDER_BY_COLUMNS_IN_SELECT\n");
	erg=SQLGetInfo(dbc,SQL_ORDER_BY_COLUMNS_IN_SELECT,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"N");
	} else {
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_SPECIAL_CHARACTERS
	stdoutput.printf("  SQL_SPECIAL_CHARACTERS\n");
	erg=SQLGetInfo(dbc,SQL_SPECIAL_CHARACTERS,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"@#");
	} else {
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_COLUMNS_IN_GROUP_BY
	stdoutput.printf("  SQL_MAX_COLUMNS_IN_GROUP_BY\n");
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_GROUP_BY,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,1012);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_COLUMNS_IN_INDEX
	stdoutput.printf("  SQL_MAX_COLUMNS_IN_INDEX\n");
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_INDEX,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,16);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_COLUMNS_IN_ORDER_BY
	stdoutput.printf("  SQL_MAX_COLUMNS_IN_ORDER_BY\n");
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_ORDER_BY,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,1012);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_COLUMNS_IN_SELECT
	stdoutput.printf("  SQL_MAX_COLUMNS_IN_SELECT\n");
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_SELECT,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,1012);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_COLUMNS_IN_TABLE
	stdoutput.printf("  SQL_MAX_COLUMNS_IN_TABLE\n");
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_TABLE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,1012);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_INDEX_SIZE
	stdoutput.printf("  SQL_MAX_INDEX_SIZE\n");
	erg=SQLGetInfo(dbc,SQL_MAX_INDEX_SIZE,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,1024);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_ROW_SIZE
	stdoutput.printf("  SQL_MAX_ROW_SIZE\n");
	erg=SQLGetInfo(dbc,SQL_MAX_ROW_SIZE,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,32677);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_STATEMENT_LEN
	stdoutput.printf("  SQL_MAX_STATEMENT_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_STATEMENT_LEN,
			(SQLPOINTER)&uintval,(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,2097152);
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,30);
	} else {
		// native db2 reports 8
		assertEqualDbc(dbc,(int)usmallintval,8);
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
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_OJ_LEFT|SQL_OJ_RIGHT|SQL_OJ_FULL|
				SQL_OJ_NESTED|SQL_OJ_NOT_ORDERED|
				SQL_OJ_INNER|SQL_OJ_ALL_COMPARISON_OPS));
	} else {
		// native db2 reports 127
		assertEqualDbc(dbc,(int)uintval,127);
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
		// native db2 reports 0
		assertEqualDbc(dbc,(int)uintval,0);
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"N");
	} else {
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"Y");
	} else {
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
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
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
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
	// requires a valid SQLHSTMT input; skip the call, just reference the
	// macro to ensure it's defined
	stdoutput.printf("  SQL_DRIVER_HSTMT\n");
	(void)SQL_DRIVER_HSTMT;
	stdoutput.printf("\n");


	// SQL_DRIVER_NAME
	stdoutput.printf("  SQL_DRIVER_NAME\n");
	erg=SQLGetInfo(dbc,SQL_DRIVER_NAME,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"libsqlrodbc.so");
	} else {
		// native driver file name varies by platform; verify non-empty
		assertTrueDbc(dbc,vallen>0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_DRIVER_VER
	stdoutput.printf("  SQL_DRIVER_VER\n");
	erg=SQLGetInfo(dbc,SQL_DRIVER_VER,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"2.1.2");
	} else {
		// native driver version varies; verify non-empty
		assertTrueDbc(dbc,vallen>0);
	}
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
		// native db2 reports 2
		assertEqualDbc(dbc,(int)usmallintval,2);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_ODBC_VER
	stdoutput.printf("  SQL_ODBC_VER\n");
	erg=SQLGetInfo(dbc,SQL_ODBC_VER,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"03.52");
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
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
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
		// native db2 reports 2
		assertEqualDbc(dbc,(int)usmallintval,2);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_PROCEDURES
	stdoutput.printf("  SQL_PROCEDURES\n");
	erg=SQLGetInfo(dbc,SQL_PROCEDURES,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"Y");
	} else {
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
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
		// native db2 reports 0
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
		// native db2 reports 1
		assertEqualDbc(dbc,(int)usmallintval,1);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_EXPRESSIONS_IN_ORDERBY
	stdoutput.printf("  SQL_EXPRESSIONS_IN_ORDERBY\n");
	erg=SQLGetInfo(dbc,SQL_EXPRESSIONS_IN_ORDERBY,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"Y");
	} else {
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_OWNER_NAME_LEN
	stdoutput.printf("  SQL_MAX_OWNER_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_OWNER_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,128);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_PROCEDURE_NAME_LEN
	stdoutput.printf("  SQL_MAX_PROCEDURE_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_PROCEDURE_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	assertEqualDbc(dbc,(int)usmallintval,128);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_QUALIFIER_NAME_LEN
	stdoutput.printf("  SQL_MAX_QUALIFIER_NAME_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_QUALIFIER_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,8);
	} else {
		// native db2 reports 0
		assertEqualDbc(dbc,(int)usmallintval,0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MULT_RESULT_SETS
	stdoutput.printf("  SQL_MULT_RESULT_SETS\n");
	erg=SQLGetInfo(dbc,SQL_MULT_RESULT_SETS,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"Y");
	} else {
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
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
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_OUTER_JOINS
	stdoutput.printf("  SQL_OUTER_JOINS\n");
	erg=SQLGetInfo(dbc,SQL_OUTER_JOINS,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"Y");
	} else {
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_OWNER_TERM
	stdoutput.printf("  SQL_OWNER_TERM\n");
	erg=SQLGetInfo(dbc,SQL_OWNER_TERM,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"schema");
	} else {
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_PROCEDURE_TERM
	stdoutput.printf("  SQL_PROCEDURE_TERM\n");
	erg=SQLGetInfo(dbc,SQL_PROCEDURE_TERM,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"stored procedure");
	} else {
		// the native db2 ODBC driver's term for a procedure
		assertTrueDbc(dbc,vallen>0);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_QUALIFIER_NAME_SEPARATOR
	stdoutput.printf("  SQL_QUALIFIER_NAME_SEPARATOR\n");
	erg=SQLGetInfo(dbc,SQL_QUALIFIER_NAME_SEPARATOR,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,".");
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
		// the native db2 driver reports no catalog term (db2 does not
		// use catalog qualifiers)
		assertEqualDbc(dbc,(const char *)strval,"");
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
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_SO_FORWARD_ONLY|SQL_SO_KEYSET_DRIVEN|
				SQL_SO_STATIC));
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
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_FUNCTIONS
	stdoutput.printf("  SQL_CONVERT_FUNCTIONS\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,(int)SQL_FN_CVT_CAST);
	} else {
		// native db2 reports 3
		assertEqualDbc(dbc,(int)uintval,3);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_NUMERIC_FUNCTIONS
	stdoutput.printf("  SQL_NUMERIC_FUNCTIONS\n");
	erg=SQLGetInfo(dbc,SQL_NUMERIC_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		// db2 numeric scalar functions (note: no PI)
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_FN_NUM_ABS|SQL_FN_NUM_ACOS|
				SQL_FN_NUM_ASIN|SQL_FN_NUM_ATAN|
				SQL_FN_NUM_ATAN2|SQL_FN_NUM_CEILING|
				SQL_FN_NUM_COS|SQL_FN_NUM_COT|
				SQL_FN_NUM_DEGREES|SQL_FN_NUM_EXP|
				SQL_FN_NUM_FLOOR|SQL_FN_NUM_LOG|
				SQL_FN_NUM_LOG10|SQL_FN_NUM_MOD|
				SQL_FN_NUM_POWER|SQL_FN_NUM_RADIANS|
				SQL_FN_NUM_RAND|SQL_FN_NUM_ROUND|
				SQL_FN_NUM_SIGN|SQL_FN_NUM_SIN|
				SQL_FN_NUM_SQRT|SQL_FN_NUM_TAN|
				SQL_FN_NUM_TRUNCATE));
	} else {
		// native db2 reports 16777215
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
		// db2 string scalar functions
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_FN_STR_ASCII|SQL_FN_STR_CHAR|
				SQL_FN_STR_CONCAT|SQL_FN_STR_DIFFERENCE|
				SQL_FN_STR_INSERT|SQL_FN_STR_LCASE|
				SQL_FN_STR_LEFT|SQL_FN_STR_LENGTH|
				SQL_FN_STR_LOCATE|SQL_FN_STR_LTRIM|
				SQL_FN_STR_REPEAT|SQL_FN_STR_REPLACE|
				SQL_FN_STR_RIGHT|SQL_FN_STR_RTRIM|
				SQL_FN_STR_SOUNDEX|SQL_FN_STR_SPACE|
				SQL_FN_STR_SUBSTRING|SQL_FN_STR_UCASE));
	} else {
		// native db2 reports 524287
		assertEqualDbc(dbc,(int)uintval,524287);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_SYSTEM_FUNCTIONS
	stdoutput.printf("  SQL_SYSTEM_FUNCTIONS\n");
	erg=SQLGetInfo(dbc,SQL_SYSTEM_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		// db2 reports no ODBC system scalar functions
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 7
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
		// db2 timedate scalar functions
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_FN_TD_NOW|SQL_FN_TD_CURDATE|
				SQL_FN_TD_DAYOFWEEK|SQL_FN_TD_DAYOFYEAR|
				SQL_FN_TD_MONTH|SQL_FN_TD_QUARTER|
				SQL_FN_TD_WEEK|SQL_FN_TD_YEAR|
				SQL_FN_TD_CURTIME|SQL_FN_TD_HOUR|
				SQL_FN_TD_MINUTE|SQL_FN_TD_SECOND|
				SQL_FN_TD_TIMESTAMPDIFF|SQL_FN_TD_DAYNAME|
				SQL_FN_TD_MONTHNAME));
	} else {
		// native db2 reports 131071
		assertEqualDbc(dbc,(int)uintval,131071);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_BIGINT
	stdoutput.printf("  SQL_CONVERT_BIGINT\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_BIGINT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_BINARY
	stdoutput.printf("  SQL_CONVERT_BINARY\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_BINARY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_BIT
	stdoutput.printf("  SQL_CONVERT_BIT\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_BIT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_CHAR
	stdoutput.printf("  SQL_CONVERT_CHAR\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_CHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 129
		assertEqualDbc(dbc,(int)uintval,129);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_DATE
	stdoutput.printf("  SQL_CONVERT_DATE\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_DATE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 1
		assertEqualDbc(dbc,(int)uintval,1);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_DECIMAL
	stdoutput.printf("  SQL_CONVERT_DECIMAL\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_DECIMAL,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 129
		assertEqualDbc(dbc,(int)uintval,129);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_DOUBLE
	stdoutput.printf("  SQL_CONVERT_DOUBLE\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_DOUBLE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 129
		assertEqualDbc(dbc,(int)uintval,129);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_FLOAT
	stdoutput.printf("  SQL_CONVERT_FLOAT\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_FLOAT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 129
		assertEqualDbc(dbc,(int)uintval,129);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_INTEGER
	stdoutput.printf("  SQL_CONVERT_INTEGER\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_INTEGER,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 129
		assertEqualDbc(dbc,(int)uintval,129);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_LONGVARCHAR
	stdoutput.printf("  SQL_CONVERT_LONGVARCHAR\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_LONGVARCHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_NUMERIC
	stdoutput.printf("  SQL_CONVERT_NUMERIC\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_NUMERIC,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 129
		assertEqualDbc(dbc,(int)uintval,129);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_REAL
	stdoutput.printf("  SQL_CONVERT_REAL\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_REAL,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_SMALLINT
	stdoutput.printf("  SQL_CONVERT_SMALLINT\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_SMALLINT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 129
		assertEqualDbc(dbc,(int)uintval,129);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_TIME
	stdoutput.printf("  SQL_CONVERT_TIME\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_TIME,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 1
		assertEqualDbc(dbc,(int)uintval,1);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_TIMESTAMP
	stdoutput.printf("  SQL_CONVERT_TIMESTAMP\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_TIMESTAMP,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 1
		assertEqualDbc(dbc,(int)uintval,1);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_TINYINT
	stdoutput.printf("  SQL_CONVERT_TINYINT\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_TINYINT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_VARBINARY
	stdoutput.printf("  SQL_CONVERT_VARBINARY\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_VARBINARY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_VARCHAR
	stdoutput.printf("  SQL_CONVERT_VARCHAR\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_VARCHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 128
		assertEqualDbc(dbc,(int)uintval,128);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_LONGVARBINARY
	stdoutput.printf("  SQL_CONVERT_LONGVARBINARY\n");
	erg=SQLGetInfo(dbc,SQL_CONVERT_LONGVARBINARY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_CONVERT_GUID
	// neither driver supports this infotype: the native db2 driver
	// returns HY096 "Information type out of range", SQL Relay returns
	// HYC00 "Optional field not implemented"
	stdoutput.printf("  SQL_CONVERT_GUID\n");
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
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_CN_ANY);
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
	assertEqualDbc(dbc,(const char *)strval,"03.80");
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_LOCK_TYPES
	stdoutput.printf("  SQL_LOCK_TYPES\n");
	erg=SQLGetInfo(dbc,SQL_LOCK_TYPES,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	// db2 reports no lock types
	assertEqualDbc(dbc,(int)uintval,0);
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
		// db2 supports positioned delete and update
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_PS_POSITIONED_DELETE|
				SQL_PS_POSITIONED_UPDATE));
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
		// native db2 reports 90
		assertEqualDbc(dbc,(int)uintval,90);
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
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_GROUP_BY
	stdoutput.printf("  SQL_GROUP_BY\n");
	erg=SQLGetInfo(dbc,SQL_GROUP_BY,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		// sqlrelay returns SQL_GB_NO_RELATION|SQL_GB_COLLATE, an
		// illegal value per spec (SQL_GROUP_BY is a single enum, not
		// a bitmask)
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
	// keyword list varies by driver/version; verify non-empty
	assertTrueDbc(dbc,vallen>0);
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
	assertEqualDbc(dbc,(int)uintval,0);
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"N");
	} else {
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_CHAR_LITERAL_LEN
	stdoutput.printf("  SQL_MAX_CHAR_LITERAL_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_CHAR_LITERAL_LEN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,32672);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_TIMEDATE_ADD_INTERVALS
	stdoutput.printf("  SQL_TIMEDATE_ADD_INTERVALS\n");
	erg=SQLGetInfo(dbc,SQL_TIMEDATE_ADD_INTERVALS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 511
		assertEqualDbc(dbc,(int)uintval,511);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_TIMEDATE_DIFF_INTERVALS
	stdoutput.printf("  SQL_TIMEDATE_DIFF_INTERVALS\n");
	erg=SQLGetInfo(dbc,SQL_TIMEDATE_DIFF_INTERVALS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 511
		assertEqualDbc(dbc,(int)uintval,511);
	}
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
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_MAX_BINARY_LITERAL_LEN
	stdoutput.printf("  SQL_MAX_BINARY_LITERAL_LEN\n");
	erg=SQLGetInfo(dbc,SQL_MAX_BINARY_LITERAL_LEN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertEqualDbc(dbc,(int)uintval,4000);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_LIKE_ESCAPE_CLAUSE
	stdoutput.printf("  SQL_LIKE_ESCAPE_CLAUSE\n");
	erg=SQLGetInfo(dbc,SQL_LIKE_ESCAPE_CLAUSE,
			(SQLPOINTER)strval,(SQLSMALLINT)sizeof(strval),
			&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(const char *)strval,"Y");
	} else {
		// native db2 driver bug: SQLGetInfo for this string info type
		// returns the SQL_DRIVER_ODBC_VER value ("03.80") instead of
		// the real value; pin the bug so a fix gets noticed
		assertEqualDbc(dbc,(const char *)strval,"03.80");
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// SQL_QUALIFIER_LOCATION
	stdoutput.printf("  SQL_QUALIFIER_LOCATION\n");
	erg=SQLGetInfo(dbc,SQL_QUALIFIER_LOCATION,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)usmallintval,(int)SQL_QL_START);
	} else {
		// native db2 reports 0
		assertEqualDbc(dbc,(int)usmallintval,0);
	}
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
		assertEqualDbc(dbc,(int)usmallintval,1);
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
	// db2 reports no datetime literal support through sqlrelay
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_ASYNC_MODE
	stdoutput.printf("  SQL_ASYNC_MODE\n");
	erg=SQLGetInfo(dbc,SQL_ASYNC_MODE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 2
		assertEqualDbc(dbc,(int)uintval,2);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_BATCH_ROW_COUNT
	stdoutput.printf("  SQL_BATCH_ROW_COUNT\n");
	erg=SQLGetInfo(dbc,SQL_BATCH_ROW_COUNT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 4
		assertEqualDbc(dbc,(int)uintval,4);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_BATCH_SUPPORT
	stdoutput.printf("  SQL_BATCH_SUPPORT\n");
	erg=SQLGetInfo(dbc,SQL_BATCH_SUPPORT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 7
		assertEqualDbc(dbc,(int)uintval,7);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_CONVERT_WCHAR
	stdoutput.printf("  SQL_CONVERT_WCHAR\n");
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
	if (issqlrelay) {
		// the db2 backend module reports no create-schema clauses
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 3
		assertEqualDbc(dbc,(int)uintval,3);
	}
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
		// the db2 backend module reports no create-table clauses
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 9729
		assertEqualDbc(dbc,(int)uintval,9729);
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
		// the db2 backend module reports no create-view clauses
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 15
		assertEqualDbc(dbc,(int)uintval,15);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DRIVER_HDESC
	// driver-manager-internal infotype needing an app-allocated HDESC in
	// InfoValuePtr; unixODBC rejects this call with HY024 "Invalid
	// attribute value"
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
		// the db2 backend module reports no drop-schema clauses
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 3
		assertEqualDbc(dbc,(int)uintval,3);
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
		// the db2 backend module reports no drop-table clauses
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,(int)SQL_DT_DROP_TABLE);
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
		// the db2 backend module reports no drop-view clauses
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		assertEqualDbc(dbc,(int)uintval,(int)SQL_DV_DROP_VIEW);
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
	assertSuccessDbc(dbc,erg);
	// neither driver reports dynamic-cursor support
	assertEqualDbc(dbc,(int)uintval,0);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_DYNAMIC_CURSOR_ATTRIBUTES2
	stdoutput.printf("  SQL_DYNAMIC_CURSOR_ATTRIBUTES2\n");
	erg=SQLGetInfo(dbc,SQL_DYNAMIC_CURSOR_ATTRIBUTES2,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	assertSuccessDbc(dbc,erg);
	// neither driver reports dynamic-cursor support
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
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_CA1_NEXT|SQL_CA1_POSITIONED_UPDATE|
				SQL_CA1_POSITIONED_DELETE|
				SQL_CA1_SELECT_FOR_UPDATE));
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
		// native db2 reports 2179
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
		// the db2 backend module reports no index keywords
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 3
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
	assertEqualDbc(dbc,(int)uintval,0);
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
		// native db2 reports 990799
		assertEqualDbc(dbc,(int)uintval,990799);
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
		// native db2 reports 16395
		assertEqualDbc(dbc,(int)uintval,16395);
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
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 1
		assertEqualDbc(dbc,(int)uintval,1);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0300)
	// SQL_ODBC_INTERFACE_CONFORMANCE
	stdoutput.printf("  SQL_ODBC_INTERFACE_CONFORMANCE\n");
	erg=SQLGetInfo(dbc,SQL_ODBC_INTERFACE_CONFORMANCE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),&vallen);
	if (issqlrelay) {
		assertEqualDbc(dbc,(int)uintval,(int)SQL_OIC_CORE);
	} else {
		// native db2 reports 2
		assertEqualDbc(dbc,(int)uintval,2);
	}
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
		// native db2 reports 1
		assertEqualDbc(dbc,(int)uintval,1);
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
	if (issqlrelay) {
		// db2's time_date_functions list has no
		// CURRENT_DATE/TIME/TIMESTAMP
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 7
		assertEqualDbc(dbc,(int)uintval,7);
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
		// the db2 backend module reports no foreign-key delete rules
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 11
		assertEqualDbc(dbc,(int)uintval,11);
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
		// the db2 backend module reports no foreign-key update rules
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 2
		assertEqualDbc(dbc,(int)uintval,2);
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
		// the db2 backend module reports no grant clauses
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 8048
		assertEqualDbc(dbc,(int)uintval,8048);
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
	// db2's function lists have no CHAR_LENGTH/EXTRACT/etc.
	assertEqualDbc(dbc,(int)uintval,0);
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
		// the db2 backend module reports no predicates
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 15879
		assertEqualDbc(dbc,(int)uintval,15879);
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
		// the db2 backend module reports no relational join operators
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 380
		assertEqualDbc(dbc,(int)uintval,380);
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
		// the db2 backend module reports no revoke clauses
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 13696
		assertEqualDbc(dbc,(int)uintval,13696);
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
		// the db2 backend module reports no row value constructors
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 11
		assertEqualDbc(dbc,(int)uintval,11);
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
		assertEqualDbc(dbc,(int)uintval,
			(int)(SQL_SSF_LOWER|SQL_SSF_UPPER|
				SQL_SSF_SUBSTRING));
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
		// the db2 backend module reports no value expressions
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 15
		assertEqualDbc(dbc,(int)uintval,15);
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
		// native db2 reports 1
		assertEqualDbc(dbc,(int)uintval,1);
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
		// native db2 reports 15
		assertEqualDbc(dbc,(int)uintval,15);
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
		// native db2 reports 131
		assertEqualDbc(dbc,(int)uintval,131);
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
		// the native driver reports only SQL_AF_ALL
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
		// the db2 backend module reports no ddl index operations
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 3 (create/drop index)
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
	assertEqualDbc(dbc,(const char *)strval,"03.52.0002.0003");
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
		// the db2 backend module reports no insert statement options
		assertEqualDbc(dbc,(int)uintval,0);
	} else {
		// native db2 reports 7 (insert searched/select/bulk)
		assertEqualDbc(dbc,(int)uintval,7);
	}
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");
	#endif


	#if (ODBCVER >= 0x0380)
	// SQL_ASYNC_DBC_FUNCTIONS
	// the native db2 driver doesn't implement this ODBC 3.8 infotype
	// (HY096 "Information type out of range")
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
	// the native db2 driver doesn't implement this infotype (HY096
	// "Information type out of range")
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
	// the native db2 driver doesn't implement this ODBC 3.8 infotype
	// (HY096 "Information type out of range")
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
	// the native db2 driver doesn't implement this infotype (HY096
	// "Information type out of range")
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
	// the native db2 driver still implements this ODBC 2.x call
	// (replaced by SQLGetDiagRec)
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
	// the native db2 driver still implements this ODBC 2.x call
	// (replaced by SQLGetConnectAttr)
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
	// the native db2 driver still implements this ODBC 2.x call
	// (replaced by SQLGetStmtAttr)
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
	// the native db2 driver still implements this ODBC 2.x call
	// (replaced by SQLSetConnectAttr)
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLSETSTMTOPTION
	stdoutput.printf("  SQL_API_SQLSETSTMTOPTION\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLSETSTMTOPTION,&supported);
	assertSuccessDbc(dbc,erg);
	// the native db2 driver still implements this ODBC 2.x call
	// (replaced by SQLSetStmtAttr)
	assertEqualDbc(dbc,(int)supported,(int)SQL_TRUE);
	stdoutput.printf("\n");


	// SQL_API_SQLSPECIALCOLUMNS
	stdoutput.printf("  SQL_API_SQLSPECIALCOLUMNS\n");
	supported=0xff;
	erg=SQLGetFunctions(dbc,SQL_API_SQLSPECIALCOLUMNS,&supported);
	assertSuccessDbc(dbc,erg);
	if (issqlrelay) {
		// SQL Relay doesn't yet implement this
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
		// SQL Relay doesn't yet implement this
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
		// SQL Relay doesn't yet implement this
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
		// SQL Relay doesn't yet implement this
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
		// SQL Relay doesn't yet implement this
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
		// SQL Relay doesn't yet implement this
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
		// SQL Relay doesn't yet implement this
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
		// SQL Relay doesn't yet implement descriptor field/record access
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
		// SQL Relay doesn't yet implement descriptor field/record access
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
		// SQL Relay doesn't yet implement descriptor field/record access
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
		// SQL Relay doesn't yet implement descriptor field/record access
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

	// db2 supports all four isolation levels (UR, CS, RS, RR)
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

	// reset to default isolation level
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)(uintptr_t)SQL_TXN_READ_COMMITTED,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetConnectAttr(dbc,SQL_ATTR_TXN_ISOLATION,
			(SQLPOINTER)&dbcuintval,0,&dbcstrlen);
	assertSuccessDbc(dbc,erg);
	assertEqualDbc(dbc,(int)dbcuintval,(int)SQL_TXN_READ_COMMITTED);
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
	// hangs when run directly against the native driver
	if (issqlrelay) {
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
	}
	stdoutput.printf("\n");


	// SQL_ATTR_APP_PARAM_DESC (descriptor handle, settable)
	stdoutput.printf("  SQL_ATTR_APP_PARAM_DESC\n");
	// hangs when run directly against the native driver
	if (issqlrelay) {
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
	}
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
	if (issqlrelay) {
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_SCROLLABLE,
				(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_NONSCROLLABLE);
		// SQL Relay only supports SQL_NONSCROLLABLE; expect
		// SQL_SUCCESS_WITH_INFO, SQLSTATE 01S02 "Option value changed"
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
	} else {
		// native db2 driver supports scrollable cursors
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_SCROLLABLE,
				(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_NONSCROLLABLE);
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_SCROLLABLE,
				(SQLPOINTER)(uintptr_t)SQL_SCROLLABLE,0);
		assertSuccessStmt(stmt,erg);
		// restore non-scrollable so the cursor type stays forward-only
		// for the SQL_ATTR_CURSOR_TYPE test below
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_SCROLLABLE,
				(SQLPOINTER)(uintptr_t)SQL_NONSCROLLABLE,0);
		assertSuccessStmt(stmt,erg);
	}
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
		// SQL_INSENSITIVE (SQLSTATE 01S02 "Option value changed")
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
		// native db2 driver supports cursor sensitivity; round-trip the
		// initial value so the cursor type is left unchanged for the
		// SQL_ATTR_CURSOR_TYPE test below (setting SQL_INSENSITIVE would
		// turn the cursor static)
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
				(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_UNSPECIFIED);
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_SENSITIVITY,
				(SQLPOINTER)(uintptr_t)stmtinitial,0);
		assertSuccessStmt(stmt,erg);
	}
	stdoutput.printf("\n");
	#endif


	// SQL_ATTR_CURSOR_TYPE
	// sqlrelay only supports SQL_CURSOR_FORWARD_ONLY; other values are
	// substituted (SQL_SUCCESS_WITH_INFO, SQLSTATE 01S02 "Option value
	// changed"); assertSuccessStmt accepts SUCCESS or SUCCESS_WITH_INFO
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
	// sqlrelay only supports SQL_CONCUR_READ_ONLY; other values
	// substituted (SQL_SUCCESS_WITH_INFO, SQLSTATE 01S02)
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
		// SQL_ASYNC_ENABLE_OFF (SQLSTATE 01S02 "Option value changed")
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
		// the db2 native driver supports statement-level async; get
		// returns the current mode (off) and set actually turns it on
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtinitial,
					(int)SQL_ASYNC_ENABLE_OFF);
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)(uintptr_t)SQL_ASYNC_ENABLE_ON,0);
		assertSuccessStmt(stmt,erg);
		// the set actually enabled async, so subsequent SQLExecDirect
		// calls would return SQL_STILL_EXECUTING; restore synchronous
		// mode so the rest of the test runs normally
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_ASYNC_ENABLE,
				(SQLPOINTER)(uintptr_t)SQL_ASYNC_ENABLE_OFF,0);
		assertSuccessStmt(stmt,erg);
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
	// sqlrelay round-trips the value; the native db2 driver ignores
	// it for non-keyset cursors, leaving it at 0
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
		// sqlrelay lacks positioned updates, so it can't guarantee
		// uniqueness; only SQL_SC_NON_UNIQUE is supported, others
		// substituted (SQL_SUCCESS_WITH_INFO, 01S02)
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
		// native db2 driver supports simulated cursors,
		// defaulting to SQL_SC_UNIQUE
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
				(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_SC_UNIQUE);
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
				(SQLPOINTER)(uintptr_t)SQL_SC_UNIQUE,0);
		assertSuccessStmt(stmt,erg);
		// restore the default so later positioned operations are
		// unaffected
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_SIMULATE_CURSOR,
				(SQLPOINTER)(uintptr_t)SQL_SC_NON_UNIQUE,0);
		assertSuccessStmt(stmt,erg);
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
	// sqlrelay lacks bookmarks; SQL_UB_VARIABLE substituted with
	// SQL_UB_OFF (SQL_SUCCESS_WITH_INFO, SQLSTATE 01S02)
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
	// sqlrelay catalog functions always pattern-match, so SQL_TRUE is
	// substituted with SQL_FALSE (SQL_SUCCESS_WITH_INFO, SQLSTATE 01S02)
	stdoutput.printf("  SQL_ATTR_METADATA_ID\n");
	if (issqlrelay) {
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
				(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_FALSE);
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
				(SQLPOINTER)(uintptr_t)SQL_TRUE,0);
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS_WITH_INFO);
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
				(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		// get reflects the substituted value, not what the app set
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_FALSE);
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
				(SQLPOINTER)(uintptr_t)SQL_FALSE,0);
		assertEqualStmt(stmt,(int)erg,(int)SQL_SUCCESS);
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
				(SQLPOINTER)&stmtulenval,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtulenval,(int)SQL_FALSE);
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
				(SQLPOINTER)(uintptr_t)stmtinitial,0);
		assertSuccessStmt(stmt,erg);
	} else {
		// native db2 driver doesn't implement statement-level
		// SQL_ATTR_METADATA_ID; get and set raise HYC00 "Driver not
		// capable"
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
				(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
		assertFailureStmt(stmt,erg);
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_METADATA_ID,
				(SQLPOINTER)(uintptr_t)SQL_TRUE,0);
		assertFailureStmt(stmt,erg);
	}
	stdoutput.printf("\n");


	// SQL_ATTR_ENABLE_AUTO_IPD
	// stub: sqlrelay round-trips the value but auto-IPD isn't gated on it
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
		// native db2 driver supports SQL_ATTR_ENABLE_AUTO_IPD,
		// defaulting to SQL_FALSE
		erg=SQLGetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
				(SQLPOINTER)&stmtinitial,0,&stmtstrlen);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(int)stmtinitial,(int)SQL_FALSE);
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
				(SQLPOINTER)(uintptr_t)SQL_FALSE,0);
		assertSuccessStmt(stmt,erg);
		// restore the initial value
		erg=SQLSetStmtAttr(stmt,SQL_ATTR_ENABLE_AUTO_IPD,
				(SQLPOINTER)(uintptr_t)stmtinitial,0);
		assertSuccessStmt(stmt,erg);
	}
	stdoutput.printf("\n");


	// SQL_ATTR_PARAMSET_SIZE
	// sqlrelay lacks parameter arrays; values != 1 substituted with 1
	// (SQL_SUCCESS_WITH_INFO, SQLSTATE 01S02)
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
	// sqlrelay lacks parameter arrays; row-wise (non-zero) substituted
	// with SQL_PARAM_BIND_BY_COLUMN (SQL_SUCCESS_WITH_INFO, SQLSTATE 01S02)
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
	if (issqlrelay) {
		assertEqualStmt(stmt,
			(int)(stmtptrval==(SQLPOINTER)stmtparamstatus),1);
	} else {
		// native db2 driver accepts the set but its get does not
		// round-trip the param status pointer; it reports an
		// internal, non-null pointer instead
		assertEqualStmt(stmt,(int)(stmtptrval!=NULL &&
			stmtptrval!=(SQLPOINTER)stmtparamstatus),1);
	}
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
		// SQL Relay lacks bookmarks, so no bookmark pointer is ever set
		assertEqualStmt(stmt,(int)(stmtptrinit==NULL),1);
	}
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_FETCH_BOOKMARK_PTR,
			(SQLPOINTER)&stmtbookmark,SQL_IS_POINTER);
	if (issqlrelay) {
		// non-NULL bookmark pointer rejected with SQLSTATE HYC00
		// "Optional feature not implemented"
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
	// unrecognized attribute must return SQLSTATE HY092
	// "Invalid attribute/option identifier"
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
	// db2 DDL is transactional; create with autocommit on so it stays
	// committed (inserts below run autocommit off, staying uncommitted
	// for the COMMIT AND ROLLBACK test)
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable ("
		"	testsmallint smallint, "
		"	testint integer, "
		"	testbigint bigint, "
		"	testdecimal decimal(10,2), "
		"	testreal real, "
		"	testdouble double, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testdate date, "
		"	testtime time, "
		"	testtimestamp timestamp, "
		"	testclob clob, "
		"	testblob blob)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// insert
	stdoutput.printf("INSERT: \n");
	erg=SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
		(SQLPOINTER)SQL_AUTOCOMMIT_OFF,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into "
		"	testtable "
		"values ("
		"	1, "
		"	1, "
		"	1, "
		"	1.5, "
		"	1.5, "
		"	1.5, "
		"	'testchar1', "
		"	'testvarchar1', "
		"	'2001-01-01', "
		"	'01:00:00', "
		"	NULL, "
		"	'testclob1', "
		"	blob('testblob1'))",
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
	// db2 uses ? placeholders, so the same query text works through
	// both sql relay and the native driver
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
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	NULL, "
		"	?, "
		"	?)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLSMALLINT	bindvarcount;
	erg=SQLNumParams(stmt,&bindvarcount);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)bindvarcount,12);

	SQLSMALLINT	smallintval;
	SQLINTEGER	intval;
	SQLBIGINT	bigintval;
	SQLCHAR		*decimalval;
	SQLREAL		realval;
	SQLDOUBLE	doubleval;
	SQLCHAR		*charval;
	SQLCHAR		*varcharval;
	SQL_DATE_STRUCT	dateval;
	SQL_TIME_STRUCT	timeval;
	SQLCHAR		clobval[40];
	SQLCHAR		*blobval;
	SQLLEN		smallintlen=sizeof(SQLSMALLINT);
	SQLLEN		intlen=sizeof(SQLINTEGER);
	SQLLEN		bigintlen=sizeof(SQLBIGINT);
	SQLLEN		decimallen=SQL_NTS;
	SQLLEN		reallen=sizeof(SQLREAL);
	SQLLEN		doublelen=sizeof(SQLDOUBLE);
	SQLLEN		charlen=SQL_NTS;
	SQLLEN		varcharlen=SQL_NTS;
	SQLLEN		datelen=sizeof(SQL_DATE_STRUCT);
	SQLLEN		timelen=sizeof(SQL_TIME_STRUCT);
	SQLLEN		cloblen=sizeof(clobval);
	SQLLEN		clobstrlen=SQL_NTS;
	SQLLEN		bloblen=9;

	// row 2
	smallintval=2;
	intval=2;
	bigintval=2;
	decimalval=(SQLCHAR *)"2.5";
	realval=2.5;
	doubleval=2.5;
	charval=(SQLCHAR *)"testchar2";
	varcharval=(SQLCHAR *)"testvarchar2";
	dateval.year=2002;
	dateval.month=1;
	dateval.day=1;
	timeval.hour=2;
	timeval.minute=0;
	timeval.second=0;
	charstring::copy((char *)clobval,"testclob2");
	blobval=(SQLCHAR *)"testblob2";
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SSHORT,SQL_SMALLINT,
				0,0,
				(SQLPOINTER)&smallintval,
				smallintlen,&smallintlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				intlen,&intlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
				SQL_C_SBIGINT,SQL_BIGINT,
				0,0,
				(SQLPOINTER)&bigintval,
				bigintlen,&bigintlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,4,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_DECIMAL,
				10,2,
				(SQLPOINTER)decimalval,
				0,&decimallen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,5,SQL_PARAM_INPUT,
				SQL_C_FLOAT,SQL_REAL,
				0,0,
				(SQLPOINTER)&realval,
				reallen,&reallen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,6,SQL_PARAM_INPUT,
				SQL_C_DOUBLE,SQL_DOUBLE,
				0,0,
				(SQLPOINTER)&doubleval,
				doublelen,&doublelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,7,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_CHAR,
				40,0,
				(SQLPOINTER)charval,
				0,&charlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,8,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_VARCHAR,
				40,0,
				(SQLPOINTER)varcharval,
				0,&varcharlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,9,SQL_PARAM_INPUT,
				SQL_C_TYPE_DATE,SQL_TYPE_DATE,
				10,0,
				(SQLPOINTER)&dateval,
				sizeof(dateval),&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,10,SQL_PARAM_INPUT,
				SQL_C_TYPE_TIME,SQL_TYPE_TIME,
				8,0,
				(SQLPOINTER)&timeval,
				sizeof(timeval),&timelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,11,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)clobval,
				cloblen,&clobstrlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,12,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				0,0,
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
	smallintval=3;
	intval=3;
	bigintval=3;
	decimalval=(SQLCHAR *)"3.5";
	realval=3.5;
	doubleval=3.5;
	charval=(SQLCHAR *)"testchar3";
	varcharval=(SQLCHAR *)"testvarchar3";
	dateval.year=2003;
	dateval.month=1;
	dateval.day=1;
	timeval.hour=3;
	timeval.minute=0;
	timeval.second=0;
	charstring::copy((char *)clobval,"testclob3");
	blobval=(SQLCHAR *)"testblob3";
	charlen=SQL_NTS;
	varcharlen=SQL_NTS;
	datelen=sizeof(SQL_DATE_STRUCT);
	timelen=sizeof(SQL_TIME_STRUCT);
	cloblen=sizeof(clobval);
	clobstrlen=SQL_NTS;
	bloblen=9;
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SSHORT,SQL_SMALLINT,
				0,0,
				(SQLPOINTER)&smallintval,
				smallintlen,&smallintlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				intlen,&intlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
				SQL_C_SBIGINT,SQL_BIGINT,
				0,0,
				(SQLPOINTER)&bigintval,
				bigintlen,&bigintlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,4,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_DECIMAL,
				10,2,
				(SQLPOINTER)decimalval,
				0,&decimallen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,5,SQL_PARAM_INPUT,
				SQL_C_FLOAT,SQL_REAL,
				0,0,
				(SQLPOINTER)&realval,
				reallen,&reallen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,6,SQL_PARAM_INPUT,
				SQL_C_DOUBLE,SQL_DOUBLE,
				0,0,
				(SQLPOINTER)&doubleval,
				doublelen,&doublelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,7,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_CHAR,
				40,0,
				(SQLPOINTER)charval,
				0,&charlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,8,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_VARCHAR,
				40,0,
				(SQLPOINTER)varcharval,
				0,&varcharlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,9,SQL_PARAM_INPUT,
				SQL_C_TYPE_DATE,SQL_TYPE_DATE,
				10,0,
				(SQLPOINTER)&dateval,
				sizeof(dateval),&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,10,SQL_PARAM_INPUT,
				SQL_C_TYPE_TIME,SQL_TYPE_TIME,
				8,0,
				(SQLPOINTER)&timeval,
				sizeof(timeval),&timelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,11,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)clobval,
				cloblen,&clobstrlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,12,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				0,0,
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
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	NULL, "
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
		"	?, "
		"	?, "
		"	?, "
		"	?, "
		"	NULL, "
		"	?, "
		"	?)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);

	// row 4
	smallintval=4;
	intval=4;
	bigintval=4;
	decimalval=(SQLCHAR *)"4.5";
	realval=4.5;
	doubleval=4.5;
	charval=(SQLCHAR *)"testchar4";
	varcharval=(SQLCHAR *)"testvarchar4";
	dateval.year=2004;
	dateval.month=1;
	dateval.day=1;
	timeval.hour=4;
	timeval.minute=0;
	timeval.second=0;
	charstring::copy((char *)clobval,"testclob4");
	blobval=(SQLCHAR *)"testblob4";
	charlen=SQL_NTS;
	varcharlen=SQL_NTS;
	datelen=sizeof(SQL_DATE_STRUCT);
	timelen=sizeof(SQL_TIME_STRUCT);
	// data-at-exec sentinel for the CLOB column
	SQLLEN	clobdataatexeclen=SQL_LEN_DATA_AT_EXEC(9);
	bloblen=9;
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SSHORT,SQL_SMALLINT,
				0,0,
				(SQLPOINTER)&smallintval,
				smallintlen,&smallintlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				intlen,&intlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
				SQL_C_SBIGINT,SQL_BIGINT,
				0,0,
				(SQLPOINTER)&bigintval,
				bigintlen,&bigintlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,4,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_DECIMAL,
				10,2,
				(SQLPOINTER)decimalval,
				0,&decimallen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,5,SQL_PARAM_INPUT,
				SQL_C_FLOAT,SQL_REAL,
				0,0,
				(SQLPOINTER)&realval,
				reallen,&reallen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,6,SQL_PARAM_INPUT,
				SQL_C_DOUBLE,SQL_DOUBLE,
				0,0,
				(SQLPOINTER)&doubleval,
				doublelen,&doublelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,7,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_CHAR,
				40,0,
				(SQLPOINTER)charval,
				0,&charlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,8,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_VARCHAR,
				40,0,
				(SQLPOINTER)varcharval,
				0,&varcharlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,9,SQL_PARAM_INPUT,
				SQL_C_TYPE_DATE,SQL_TYPE_DATE,
				10,0,
				(SQLPOINTER)&dateval,
				sizeof(dateval),&datelen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,10,SQL_PARAM_INPUT,
				SQL_C_TYPE_TIME,SQL_TYPE_TIME,
				8,0,
				(SQLPOINTER)&timeval,
				sizeof(timeval),&timelen);
	assertSuccessStmt(stmt,erg);
	// clobval is the app token returned by SQLParamData; the real value
	// is supplied via SQLPutData below
	erg=SQLBindParameter(stmt,11,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
				0,0,
				(SQLPOINTER)clobval,
				0,&clobdataatexeclen);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,12,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				0,0,
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
		"	testsmallint",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// column count
	stdoutput.printf("COLUMN COUNT: \n");
	SQLSMALLINT	colcount;
	erg=SQLNumResultCols(stmt,&colcount);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)colcount,13);
	stdoutput.printf("\n");



	// column info
	stdoutput.printf("COLUMN INFO: \n");
	SQLCHAR		colname[256];
	SQLSMALLINT	colnamelen;
	SQLSMALLINT	datatype;
	SQLULEN		colsize;
	SQLSMALLINT	decdigits;
	SQLSMALLINT	nullable;

	// col 1: testsmallint
	erg=SQLDescribeCol(stmt,1,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTSMALLINT");
	assertEqualStmt(stmt,(int)colnamelen,12);
	assertEqualStmt(stmt,(int)datatype,SQL_SMALLINT);
	assertEqualStmt(stmt,(int)colsize,5);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 2: testint
	erg=SQLDescribeCol(stmt,2,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTINT");
	assertEqualStmt(stmt,(int)colnamelen,7);
	assertEqualStmt(stmt,(int)datatype,SQL_INTEGER);
	assertEqualStmt(stmt,(int)colsize,10);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 3: testbigint
	erg=SQLDescribeCol(stmt,3,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTBIGINT");
	assertEqualStmt(stmt,(int)colnamelen,10);
	assertEqualStmt(stmt,(int)datatype,SQL_BIGINT);
	// column size is the precision: 19 digits for a signed bigint
	assertEqualStmt(stmt,(int)colsize,19);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 4: testdecimal
	erg=SQLDescribeCol(stmt,4,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTDECIMAL");
	assertEqualStmt(stmt,(int)colnamelen,11);
	assertEqualStmt(stmt,(int)datatype,SQL_DECIMAL);
	// column size is the precision (10) of decimal(10,2)
	assertEqualStmt(stmt,(int)colsize,10);
	assertEqualStmt(stmt,(int)decdigits,2);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 5: testreal
	erg=SQLDescribeCol(stmt,5,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTREAL");
	assertEqualStmt(stmt,(int)colnamelen,8);
	assertEqualStmt(stmt,(int)datatype,SQL_REAL);
	assertEqualStmt(stmt,(int)colsize,7);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 6: testdouble
	erg=SQLDescribeCol(stmt,6,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTDOUBLE");
	assertEqualStmt(stmt,(int)colnamelen,10);
	assertEqualStmt(stmt,(int)datatype,SQL_DOUBLE);
	assertEqualStmt(stmt,(int)colsize,15);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 7: testchar
	erg=SQLDescribeCol(stmt,7,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTCHAR");
	assertEqualStmt(stmt,(int)colnamelen,8);
	assertEqualStmt(stmt,(int)datatype,SQL_CHAR);
	assertEqualStmt(stmt,(int)colsize,40);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 8: testvarchar
	erg=SQLDescribeCol(stmt,8,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTVARCHAR");
	assertEqualStmt(stmt,(int)colnamelen,11);
	assertEqualStmt(stmt,(int)datatype,SQL_VARCHAR);
	assertEqualStmt(stmt,(int)colsize,40);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 9: testdate
	erg=SQLDescribeCol(stmt,9,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTDATE");
	assertEqualStmt(stmt,(int)colnamelen,8);
	#if (ODBCVER >= 0x0300)
	assertEqualStmt(stmt,(int)datatype,SQL_TYPE_DATE);
	#else
	assertEqualStmt(stmt,(int)datatype,SQL_DATE);
	#endif
	assertEqualStmt(stmt,(int)colsize,10);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 10: testtime
	erg=SQLDescribeCol(stmt,10,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTTIME");
	assertEqualStmt(stmt,(int)colnamelen,8);
	#if (ODBCVER >= 0x0300)
	assertEqualStmt(stmt,(int)datatype,SQL_TYPE_TIME);
	#else
	assertEqualStmt(stmt,(int)datatype,SQL_TIME);
	#endif
	assertEqualStmt(stmt,(int)colsize,8);
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 11: testtimestamp
	erg=SQLDescribeCol(stmt,11,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTTIMESTAMP");
	assertEqualStmt(stmt,(int)colnamelen,13);
	#if (ODBCVER >= 0x0300)
	assertEqualStmt(stmt,(int)datatype,SQL_TYPE_TIMESTAMP);
	#else
	assertEqualStmt(stmt,(int)datatype,SQL_TIMESTAMP);
	#endif
	assertEqualStmt(stmt,(int)colsize,26);
	assertEqualStmt(stmt,(int)decdigits,6);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 12: testclob
	erg=SQLDescribeCol(stmt,12,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTCLOB");
	assertEqualStmt(stmt,(int)colnamelen,8);
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)datatype,SQL_LONGVARCHAR);
	} else {
		// native db2 reports its driver-specific SQL_CLOB (-99)
		assertEqualStmt(stmt,(int)datatype,-99);
	}
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);

	// col 13: testblob
	erg=SQLDescribeCol(stmt,13,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)colname,"TESTBLOB");
	assertEqualStmt(stmt,(int)colnamelen,8);
	if (issqlrelay) {
		// SQL Relay reports db2 BLOB as SQL_BINARY (same as other
		// backends)
		assertEqualStmt(stmt,(int)datatype,SQL_BINARY);
	} else {
		// native db2 reports its driver-specific SQL_BLOB (-98)
		assertEqualStmt(stmt,(int)datatype,-98);
	}
	assertEqualStmt(stmt,(int)decdigits,0);
	assertEqualStmt(stmt,(int)nullable,SQL_NULLABLE);
	stdoutput.printf("\n");



	// fetch rows (SQLBindCol)
	stdoutput.printf("FETCH ROWS (SQLBindCol): \n");
	SQLSMALLINT	smallintfield;
	SQLINTEGER	intfield;
	SQLBIGINT	bigintfield;
	SQLDOUBLE	decimalfield;
	SQLREAL		realfield;
	SQLDOUBLE	doublefield;
	SQLCHAR		charfield[41];
	SQLCHAR		varcharfield[41];
	SQL_DATE_STRUCT	datefield;
	SQL_TIME_STRUCT	timefield;
	SQL_TIMESTAMP_STRUCT	timestampfield;
	SQLCHAR		clobfield[256];
	SQLCHAR		blobfield[256];
	SQLLEN		smallintind;
	SQLLEN		intind;
	SQLLEN		bigintind;
	SQLLEN		decimalind;
	SQLLEN		realind;
	SQLLEN		doubleind;
	SQLLEN		charind;
	SQLLEN		varcharind;
	SQLLEN		dateind;
	SQLLEN		timeind;
	SQLLEN		timestampind;
	SQLLEN		clobind;
	SQLLEN		blobind;

	erg=SQLBindCol(stmt,1,SQL_C_SSHORT,
			&smallintfield,sizeof(smallintfield),&smallintind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,2,SQL_C_SLONG,
			&intfield,sizeof(intfield),&intind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,3,SQL_C_SBIGINT,
			&bigintfield,sizeof(bigintfield),&bigintind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,4,SQL_C_DOUBLE,
			&decimalfield,sizeof(decimalfield),&decimalind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,5,SQL_C_FLOAT,
			&realfield,sizeof(realfield),&realind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,6,SQL_C_DOUBLE,
			&doublefield,sizeof(doublefield),&doubleind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,7,SQL_C_CHAR,
			charfield,sizeof(charfield),&charind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,8,SQL_C_CHAR,
			varcharfield,sizeof(varcharfield),&varcharind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,9,SQL_C_TYPE_DATE,
			&datefield,sizeof(datefield),&dateind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,10,SQL_C_TYPE_TIME,
			&timefield,sizeof(timefield),&timeind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,11,SQL_C_TYPE_TIMESTAMP,
			&timestampfield,sizeof(timestampfield),&timestampind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,12,SQL_C_CHAR,
			clobfield,sizeof(clobfield),&clobind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindCol(stmt,13,SQL_C_BINARY,
			blobfield,sizeof(blobfield),&blobind);
	assertSuccessStmt(stmt,erg);

	// row 1 (direct insert)
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)smallintfield,1);
	assertEqualStmt(stmt,(int)intfield,1);
	assertEqualStmt(stmt,(int)bigintfield,1);
	assertTrueStmt(stmt,decimalfield==1.5);
	assertTrueStmt(stmt,realfield==1.5);
	assertTrueStmt(stmt,doublefield==1.5);
	assertEqualStmt(stmt,(int)charind,40);
	assertEqualStmt(stmt,(const char *)charfield,
			"testchar1                               ");
	assertEqualStmt(stmt,(int)varcharind,12);
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar1");
	assertEqualStmt(stmt,(int)dateind,(int)sizeof(SQL_DATE_STRUCT));
	assertEqualStmt(stmt,(int)datefield.year,2001);
	assertEqualStmt(stmt,(int)datefield.month,1);
	assertEqualStmt(stmt,(int)datefield.day,1);
	assertEqualStmt(stmt,(int)timefield.hour,1);
	assertEqualStmt(stmt,(int)timefield.minute,0);
	assertEqualStmt(stmt,(int)timefield.second,0);
	assertEqualStmt(stmt,(int)timestampind,(int)SQL_NULL_DATA);
	assertEqualStmt(stmt,(int)clobind,9);
	assertEqualStmt(stmt,(const char *)clobfield,"testclob1");
	assertEqualStmt(stmt,(int)blobind,9);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"testblob1",9));

	// row 2
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)smallintfield,2);
	assertEqualStmt(stmt,(int)intfield,2);
	assertEqualStmt(stmt,(int)bigintfield,2);
	assertTrueStmt(stmt,decimalfield==2.5);
	assertTrueStmt(stmt,realfield==2.5);
	assertTrueStmt(stmt,doublefield==2.5);
	assertEqualStmt(stmt,(int)charind,40);
	assertEqualStmt(stmt,(const char *)charfield,
			"testchar2                               ");
	assertEqualStmt(stmt,(int)varcharind,12);
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar2");
	assertEqualStmt(stmt,(int)datefield.year,2002);
	assertEqualStmt(stmt,(int)timefield.hour,2);
	assertEqualStmt(stmt,(int)timestampind,(int)SQL_NULL_DATA);
	assertEqualStmt(stmt,(int)clobind,9);
	assertEqualStmt(stmt,(const char *)clobfield,"testclob2");
	assertEqualStmt(stmt,(int)blobind,9);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"testblob2",9));

	// row 3
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)smallintfield,3);
	assertEqualStmt(stmt,(int)intfield,3);
	assertEqualStmt(stmt,(int)bigintfield,3);
	assertTrueStmt(stmt,decimalfield==3.5);
	assertTrueStmt(stmt,realfield==3.5);
	assertTrueStmt(stmt,doublefield==3.5);
	assertEqualStmt(stmt,(const char *)charfield,
			"testchar3                               ");
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar3");
	assertEqualStmt(stmt,(int)datefield.year,2003);
	assertEqualStmt(stmt,(int)timefield.hour,3);
	assertEqualStmt(stmt,(int)timestampind,(int)SQL_NULL_DATA);
	assertEqualStmt(stmt,(const char *)clobfield,"testclob3");
	assertEqualStmt(stmt,(int)blobind,9);
	assertTrueStmt(stmt,!bytestring::compare(blobfield,"testblob3",9));

	// row 4
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)smallintfield,4);
	assertEqualStmt(stmt,(int)intfield,4);
	assertEqualStmt(stmt,(int)bigintfield,4);
	assertTrueStmt(stmt,decimalfield==4.5);
	assertTrueStmt(stmt,realfield==4.5);
	assertTrueStmt(stmt,doublefield==4.5);
	assertEqualStmt(stmt,(const char *)charfield,
			"testchar4                               ");
	assertEqualStmt(stmt,(const char *)varcharfield,"testvarchar4");
	assertEqualStmt(stmt,(int)datefield.year,2004);
	assertEqualStmt(stmt,(int)timefield.hour,4);
	assertEqualStmt(stmt,(int)timestampind,(int)SQL_NULL_DATA);
	// row 4's CLOB came via SQLPutData with length 9, so it's genuinely
	// 9 bytes (no buffer-padding artifact)
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
		"	testsmallint",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// fetch rows (SQLGetData)
	stdoutput.printf("FETCH ROWS (SQLGetData): \n");
	SQLSMALLINT	gsmallintval;
	SQLINTEGER	gintval;
	SQLBIGINT	gbigintval;
	SQLDOUBLE	gdecimalfield;
	SQLREAL		grealfield;
	SQLDOUBLE	gdoublefield;
	SQLCHAR		gcharfield[41];
	SQLCHAR		gvarcharfield[41];
	SQL_DATE_STRUCT	gdatefield;
	SQL_TIME_STRUCT	gtimefield;
	SQL_TIMESTAMP_STRUCT	gtimestampfield;
	SQLCHAR		gclobfield[256];
	SQLCHAR		gblobfield[256];
	SQLLEN		gsmallintind;
	SQLLEN		gintind;
	SQLLEN		gbigintind;
	SQLLEN		gdecimalind;
	SQLLEN		grealind;
	SQLLEN		gdoubleind;
	SQLLEN		gcharind;
	SQLLEN		gvarcharind;
	SQLLEN		gdateind;
	SQLLEN		gtimeind;
	SQLLEN		gtimestampind;
	SQLLEN		gclobind;
	SQLLEN		gblobind;

	// row 1
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetData(stmt,1,SQL_C_SSHORT,
			&gsmallintval,sizeof(gsmallintval),&gsmallintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gsmallintval,1);
	erg=SQLGetData(stmt,2,SQL_C_SLONG,
			&gintval,sizeof(gintval),&gintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gintval,1);
	erg=SQLGetData(stmt,3,SQL_C_SBIGINT,
			&gbigintval,sizeof(gbigintval),&gbigintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gbigintval,1);
	erg=SQLGetData(stmt,4,SQL_C_DOUBLE,
			&gdecimalfield,sizeof(gdecimalfield),&gdecimalind);
	assertSuccessStmt(stmt,erg);
	assertTrueStmt(stmt,gdecimalfield==1.5);
	erg=SQLGetData(stmt,5,SQL_C_FLOAT,
			&grealfield,sizeof(grealfield),&grealind);
	assertSuccessStmt(stmt,erg);
	assertTrueStmt(stmt,grealfield==1.5);
	erg=SQLGetData(stmt,6,SQL_C_DOUBLE,
			&gdoublefield,sizeof(gdoublefield),&gdoubleind);
	assertSuccessStmt(stmt,erg);
	assertTrueStmt(stmt,gdoublefield==1.5);
	erg=SQLGetData(stmt,7,SQL_C_CHAR,
			gcharfield,sizeof(gcharfield),&gcharind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gcharind,40);
	assertEqualStmt(stmt,(const char *)gcharfield,
			"testchar1                               ");
	erg=SQLGetData(stmt,8,SQL_C_CHAR,
			gvarcharfield,sizeof(gvarcharfield),&gvarcharind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gvarcharind,12);
	assertEqualStmt(stmt,(const char *)gvarcharfield,"testvarchar1");
	erg=SQLGetData(stmt,9,SQL_C_TYPE_DATE,
			&gdatefield,sizeof(gdatefield),&gdateind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gdateind,(int)sizeof(SQL_DATE_STRUCT));
	assertEqualStmt(stmt,(int)gdatefield.year,2001);
	assertEqualStmt(stmt,(int)gdatefield.month,1);
	assertEqualStmt(stmt,(int)gdatefield.day,1);
	erg=SQLGetData(stmt,10,SQL_C_TYPE_TIME,
			&gtimefield,sizeof(gtimefield),&gtimeind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gtimefield.hour,1);
	erg=SQLGetData(stmt,11,SQL_C_TYPE_TIMESTAMP,
			&gtimestampfield,sizeof(gtimestampfield),&gtimestampind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gtimestampind,(int)SQL_NULL_DATA);
	erg=SQLGetData(stmt,12,SQL_C_CHAR,
			gclobfield,sizeof(gclobfield),&gclobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gclobind,9);
	assertEqualStmt(stmt,(const char *)gclobfield,"testclob1");
	erg=SQLGetData(stmt,13,SQL_C_BINARY,
			gblobfield,sizeof(gblobfield),&gblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(gblobfield,"testblob1",9));

	// row 2
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetData(stmt,1,SQL_C_SSHORT,
			&gsmallintval,sizeof(gsmallintval),&gsmallintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gsmallintval,2);
	erg=SQLGetData(stmt,7,SQL_C_CHAR,
			gcharfield,sizeof(gcharfield),&gcharind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)gcharfield,
			"testchar2                               ");
	erg=SQLGetData(stmt,12,SQL_C_CHAR,
			gclobfield,sizeof(gclobfield),&gclobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gclobind,9);
	assertEqualStmt(stmt,(const char *)gclobfield,"testclob2");
	erg=SQLGetData(stmt,13,SQL_C_BINARY,
			gblobfield,sizeof(gblobfield),&gblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(gblobfield,"testblob2",9));

	// row 3
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetData(stmt,1,SQL_C_SSHORT,
			&gsmallintval,sizeof(gsmallintval),&gsmallintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gsmallintval,3);
	erg=SQLGetData(stmt,7,SQL_C_CHAR,
			gcharfield,sizeof(gcharfield),&gcharind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)gcharfield,
			"testchar3                               ");
	erg=SQLGetData(stmt,12,SQL_C_CHAR,
			gclobfield,sizeof(gclobfield),&gclobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gclobind,9);
	assertEqualStmt(stmt,(const char *)gclobfield,"testclob3");
	erg=SQLGetData(stmt,13,SQL_C_BINARY,
			gblobfield,sizeof(gblobfield),&gblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(gblobfield,"testblob3",9));

	// row 4
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetData(stmt,1,SQL_C_SSHORT,
			&gsmallintval,sizeof(gsmallintval),&gsmallintind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gsmallintval,4);
	erg=SQLGetData(stmt,7,SQL_C_CHAR,
			gcharfield,sizeof(gcharfield),&gcharind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)gcharfield,
			"testchar4                               ");
	erg=SQLGetData(stmt,12,SQL_C_CHAR,
			gclobfield,sizeof(gclobfield),&gclobind);
	assertSuccessStmt(stmt,erg);
	// row 4 was inserted via SQLPutData with length 9, so the CLOB is
	// genuinely 9 bytes
	assertEqualStmt(stmt,(int)gclobind,9);
	assertEqualStmt(stmt,(const char *)gclobfield,"testclob4");
	erg=SQLGetData(stmt,13,SQL_C_BINARY,
			gblobfield,sizeof(gblobfield),&gblobind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)gblobind,9);
	assertTrueStmt(stmt,!bytestring::compare(gblobfield,"testblob4",9));

	// no more rows
	erg=SQLFetch(stmt);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NO_DATA);
	stdoutput.printf("\n");



	// nested selects
	stdoutput.printf("NESTED SELECTS: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	// save initial SQL_ROWSET_SIZE to restore later; unixODBC rejects a
	// set to 0 with HY024
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
		// close the previous nested cursor before re-executing
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
		"	10, "
		"	10, "
		"	10, "
		"	10.10, "
		"	10.10, "
		"	10.10, "
		"	'testchar10', "
		"	'testvarchar10', "
		"	'2010-01-01', "
		"	'10:00:00', "
		"	NULL, "
		"	'testclob10', "
		"	NULL)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);

	// rollback on dbc
	erg=SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_ROLLBACK);
	assertSuccessDbc(dbc,erg);

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
		"	10, "
		"	10, "
		"	10, "
		"	10.10, "
		"	10.10, "
		"	10.10, "
		"	'testchar10', "
		"	'testvarchar10', "
		"	'2010-01-01', "
		"	'10:00:00', "
		"	NULL, "
		"	'testclob10', "
		"	NULL)",
		SQL_NTS);
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



	// null values
	stdoutput.printf("NULL VALUES: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select NULL,1,NULL from sysibm.sysdummy1",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		nullfield1[10];
	SQLLEN		nullind1;
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
		"	testclob1 clob, "
		"	testclob2 clob, "
		"	testblob1 blob, "
		"	testblob2 blob)",
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
	// db2 preserves empty-vs-NULL for CLOB and BLOB: zero-length input
	// round-trips as empty (indicator 0), NULL input round-trips as NULL
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
		"	testclob clob, "
		"	testblob blob)",
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
		"	testclob clob, "
		"	testblob blob)",
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
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	// db2 does output binds through stored procedure parameters
	SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create procedure testproc("
		"	out out1 int, "
		"	out out2 varchar(20), "
		"	out out3 double, "
		"	out out4 varchar(20), "
		"	out out5 varchar(20)) "
		"language sql "
		"begin "
		"	set out1 = 1; "
		"	set out2 = 'hello'; "
		"	set out3 = 2.5; "
		"	set out4 = '2001-02-03'; "
		"	set out5 = null; "
		"end",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLPrepare(stmt,(SQLCHAR *)"call testproc(?,?,?,?,?)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLNumParams(stmt,&bindvarcount);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)bindvarcount,5);
	SQLINTEGER	obnumvar=0;
	SQLLEN		obnumvarind=0;
	SQLCHAR		obstringvar[11]={0};
	SQLLEN		obstringvarind=10;
	SQLDOUBLE	obfloatvar=0.0;
	SQLLEN		obfloatvarind=0;
	SQLCHAR		obdatevar[16]={0};
	SQLLEN		obdatevarind=sizeof(obdatevar);
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
				(SQLPOINTER)obdatevar,
				sizeof(obdatevar),&obdatevarind);
	assertSuccessStmt(stmt,erg);
	erg=SQLBindParameter(stmt,5,SQL_PARAM_OUTPUT,
				SQL_C_CHAR,SQL_VARCHAR,
				20,0,
				(SQLPOINTER)obnullvar,
				sizeof(obnullvar),&obnullvarind);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)obnumvar,1);
	assertEqualStmt(stmt,(const char *)obstringvar,"hello");
	assertTrueStmt(stmt,obfloatvar==2.5);
	assertEqualStmt(stmt,(const char *)obdatevar,"2001-02-03");
	assertEqualStmt(stmt,(int)obnullvarind,(int)SQL_NULL_DATA);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// lob output bind
	stdoutput.printf("LOB OUTPUT BIND: \n");
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
		"insert into testtable values ('hello',?)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLCHAR		lobinval[6]="hello";
	SQLLEN		lobinlen=5;
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_BINARY,SQL_LONGVARBINARY,
				0,0,
				(SQLPOINTER)lobinval,
				0,&lobinlen);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecute(stmt);
	assertSuccessStmt(stmt,erg);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create procedure testproc("
		"	out out1 clob, "
		"	out out2 blob) "
		"language sql "
		"begin "
		"	select testclob into out1 from testtable; "
		"	select testblob into out2 from testtable; "
		"end",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLPrepare(stmt,(SQLCHAR *)"call testproc(?,?)",SQL_NTS);
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
	stdoutput.printf("\n");



	// long output bind
	stdoutput.printf("LONG OUTPUT BIND: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	for (int i=0; i<LARGE_BUFFER_LENGTH; i++) {
		largebuffer[i]='C';
	}
	largebuffer[LARGE_BUFFER_LENGTH]='\0';
	// db2 echoes a long clob through an in/out procedure parameter
	SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create procedure testproc("
		"	in in1 clob, "
		"	out out1 clob) "
		"language sql "
		"begin "
		"	set out1 = in1; "
		"end",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLPrepare(stmt,(SQLCHAR *)"call testproc(?,?)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLLEN	longinind=LARGE_BUFFER_LENGTH;
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_CHAR,SQL_LONGVARCHAR,
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
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop procedure testproc",SQL_NTS);
	assertSuccessStmt(stmt,erg);
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
	erg=SQLPrepare(stmt,(SQLCHAR *)
		"select cast(? as varchar(16)) from sysibm.sysdummy1",
		SQL_NTS);
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
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select cast(? as varchar(16)) from sysibm.sysdummy1",
		SQL_NTS);
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
	erg=SQLPrepare(stmt,(SQLCHAR *)
		"select cast(? as integer) from sysibm.sysdummy1",SQL_NTS);
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
	erg=SQLPrepare(stmt,(SQLCHAR *)
		"select 1 from sysibm.sysdummy1",SQL_NTS);
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
		"select cast(? as integer) from sysibm.sysdummy1",SQL_NTS);
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
	// parameterless statement without clearing binds; leftover
	// bindings must not reach the backend
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into testtable values (33,44)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLFreeStmt(stmt,SQL_CLOSE);
	// fewer-but-nonzero: binds 1 and 2 stashed but this statement has
	// one marker, so only bind 1 applies
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
	// the catalog functions below return forward-only result sets; the
	// native db2 driver rejects them with HYC00 "Driver not capable" if
	// the statement still has a scrollable/non-forward-only cursor type
	// left over from the cursor-attribute tests above, so reset it
	erg=SQLSetStmtAttr(stmt,SQL_ATTR_CURSOR_TYPE,
			(SQLPOINTER)(uintptr_t)SQL_CURSOR_FORWARD_ONLY,0);
	assertSuccessStmt(stmt,erg);
	erg=SQLTables(stmt,
			(SQLCHAR *)SQL_ALL_CATALOGS,SQL_NTS,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)"",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	int		catrows=0;
	for (;;) {
		erg=SQLFetch(stmt);
		if (erg==SQL_NO_DATA) {
			break;
		}
		assertSuccessStmt(stmt,erg);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			break;
		}
		catrows++;
	}
	// db2 has no way to list catalogs; both sqlrelay and the native
	// driver return an empty list
	assertEqualStmt(stmt,catrows,0);
	SQLFreeStmt(stmt,SQL_CLOSE);
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
	// find the connected user's schema in the result set
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
					(const char *)schname,"db2inst1")) {
			schfound=true;
		}
	}
	assertTrueStmt(stmt,schfound);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
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
	for (;;) {
		erg=SQLFetch(stmt);
		if (erg==SQL_NO_DATA) {
			break;
		}
		assertSuccessStmt(stmt,erg);
		if (erg!=SQL_SUCCESS && erg!=SQL_SUCCESS_WITH_INFO) {
			break;
		}
		// db2's table type list has TABLE, ALIAS, NICKNAME, SYSTEM
		// TABLE, HIERARCHY TABLE, MATERIALIZED QUERY TABLE and
		// INOPERATIVE VIEW, but no plain VIEW type
		if (!charstring::compare(
					(const char *)tabletype,"TABLE")) {
			foundtable=true;
		}
	}
	assertTrueStmt(stmt,foundtable);
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
		"	col1 integer, "
		"	col2 integer)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable2 ("
		"	col1 integer, "
		"	col2 integer)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable3 ("
		"	col1 integer, "
		"	col2 integer)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable4 ("
		"	col1 integer, "
		"	col2 integer)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	// SQL Relay treats an empty schema as "any schema"; the native db2
	// driver (per spec) treats it as "the empty schema name" and finds
	// nothing, so it needs the connected user's schema
	const char	*tblschema=(issqlrelay)?"":"DB2INST1";
	erg=SQLTables(stmt,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)tblschema,SQL_NTS,
			(SQLCHAR *)"TESTTABLE%",SQL_NTS,
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
		if (!charstring::compare((const char *)tblname,"TESTTABLE1") ||
			!charstring::compare(
					(const char *)tblname,"TESTTABLE2") ||
			!charstring::compare(
					(const char *)tblname,"TESTTABLE3") ||
			!charstring::compare(
					(const char *)tblname,"TESTTABLE4")) {
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
	// walk through the list and find INTEGER, CHAR, VARCHAR, DATE
	SQLCHAR		typname[64];
	SQLLEN		typnameind;
	erg=SQLBindCol(stmt,1,SQL_C_CHAR,
			typname,sizeof(typname),&typnameind);
	assertSuccessStmt(stmt,erg);
	bool		foundinteger=false;
	bool		foundchar=false;
	bool		foundvarchar=false;
	bool		founddate=false;
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
					(const char *)typname,"INTEGER")) {
			foundinteger=true;
		} else if (!charstring::compareIgnoringCase(
					(const char *)typname,"CHAR")) {
			foundchar=true;
		} else if (!charstring::compareIgnoringCase(
					(const char *)typname,"VARCHAR")) {
			foundvarchar=true;
		} else if (!charstring::compareIgnoringCase(
					(const char *)typname,"DATE")) {
			founddate=true;
		}
	}
	assertTrueStmt(stmt,foundinteger);
	assertTrueStmt(stmt,foundchar);
	assertTrueStmt(stmt,foundvarchar);
	assertTrueStmt(stmt,founddate);
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
		"	testsmallint smallint, "
		"	testint integer, "
		"	testbigint bigint, "
		"	testdecimal decimal(10,2), "
		"	testreal real, "
		"	testdouble double, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testdate date, "
		"	testtime time, "
		"	testtimestamp timestamp, "
		"	testclob clob, "
		"	testblob blob)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	// native db2 driver needs an explicit schema; SQL Relay fills in the
	// connected schema when left empty
	char	schemafilter[64]={0};
	if (!issqlrelay) {
		charstring::copy(schemafilter,"DB2INST1");
	}
	erg=SQLColumns(stmt,
			NULL,0,
			(SQLCHAR *)schemafilter,SQL_NTS,
			(SQLCHAR *)"TESTTABLE",SQL_NTS,
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
	const char	*expcols[]={"TESTSMALLINT","TESTINT","TESTBIGINT",
				"TESTDECIMAL","TESTREAL","TESTDOUBLE",
				"TESTCHAR","TESTVARCHAR","TESTDATE",
				"TESTTIME","TESTTIMESTAMP","TESTCLOB",
				"TESTBLOB"};
	for (int c=0; c<13; c++) {
		erg=SQLFetch(stmt);
		assertSuccessStmt(stmt,erg);
		assertEqualStmt(stmt,(const char *)clcolname,expcols[c]);
	}
	erg=SQLFetch(stmt);
	assertEqualStmt(stmt,(int)erg,(int)SQL_NO_DATA);
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// column list - auto_increment, primary key
	// exercises primary-key reporting via SQLPrimaryKeys
	stdoutput.printf("COLUMN LIST - auto_increment, primary key: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	SQLFreeStmt(stmt,SQL_UNBIND);
	SQLFreeStmt(stmt,SQL_RESET_PARAMS);
	SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable ("
		"	col1 int not null primary key, "
		"	col2 int)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLPrimaryKeys(stmt,
			NULL,0,
			(SQLCHAR *)schemafilter,SQL_NTS,
			(SQLCHAR *)"TESTTABLE",SQL_NTS);
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
		if (!charstring::compare((const char *)pkcolname,"COL1")) {
			foundcol1=true;
		} else if (!charstring::compare(
					(const char *)pkcolname,"COL2")) {
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
		"	col1 int not null, "
		"	col2 int, "
		"	constraint testtable_pk primary key (col1))",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLPrimaryKeys(stmt,
			NULL,0,
			(SQLCHAR *)schemafilter,SQL_NTS,
			(SQLCHAR *)"TESTTABLE",SQL_NTS);
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
	assertEqualStmt(stmt,(const char *)pktable,"TESTTABLE");
	assertEqualStmt(stmt,(const char *)pkcol,"COL1");
	assertEqualStmt(stmt,(int)pkseq,1);
	assertEqualStmt(stmt,(int)pknameind,12);
	assertEqualStmt(stmt,(const char *)pkname,"TESTTABLE_PK");
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
		"	col1 int not null primary key, "
		"	col2 int)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLStatistics(stmt,
			NULL,0,
			(SQLCHAR *)schemafilter,SQL_NTS,
			(SQLCHAR *)"TESTTABLE",SQL_NTS,
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
	// SQLStatistics may interleave a SQL_TABLE_STAT row (type=0, no
	// column) with the index rows; look for the COL1 index row
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
		assertEqualStmt(stmt,(const char *)idxtable,"TESTTABLE");
		assertEqualStmt(stmt,(int)idxnonunique,0);
		assertEqualStmt(stmt,(int)idxseq,1);
		if (!charstring::compare((const char *)idxcol,"COL1")) {
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
		"	in1 integer, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"language sql begin end",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create procedure testproc2 ("
		"	in1 integer, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"language sql begin end",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create procedure testproc3 ("
		"	in1 integer, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"language sql begin end",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create procedure testproc4 ("
		"	in1 integer, "
		"	in2 char(20), "
		"	in3 varchar(20), "
		"	in4 date) "
		"language sql begin end",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLProcedures(stmt,
			(SQLCHAR *)"",SQL_NTS,
			(SQLCHAR *)schemafilter,SQL_NTS,
			(SQLCHAR *)"TESTPROC%",SQL_NTS);
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
			(const char *)procname,"TESTPROC1") ||
			!charstring::compare(
				(const char *)procname,"TESTPROC2") ||
			!charstring::compare(
				(const char *)procname,"TESTPROC3") ||
			!charstring::compare(
				(const char *)procname,"TESTPROC4")) {
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
			(SQLCHAR *)"TESTPROC1",SQL_NTS,
			(SQLCHAR *)"",SQL_NTS);
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
	// native db2 driver may not populate ORDINAL_POSITION; SQL Relay
	// returns 1..N per spec
	// IN1
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)ppname,"IN1");
	assertEqualStmt(stmt,(int)ppmode,(int)SQL_PARAM_INPUT);
	assertEqualStmt(stmt,(const char *)pptypename,"INTEGER");
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)ppordinal,1);
	}
	// IN2
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)ppname,"IN2");
	assertEqualStmt(stmt,(int)ppmode,(int)SQL_PARAM_INPUT);
	if (issqlrelay) {
		assertEqualStmt(stmt,(const char *)pptypename,"CHARACTER");
	} else {
		// native db2 reports the type name as CHAR
		assertEqualStmt(stmt,(const char *)pptypename,"CHAR");
	}
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)ppordinal,2);
	}
	// IN3
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)ppname,"IN3");
	assertEqualStmt(stmt,(int)ppmode,(int)SQL_PARAM_INPUT);
	assertEqualStmt(stmt,(const char *)pptypename,"VARCHAR");
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)ppordinal,3);
	}
	// IN4
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)ppname,"IN4");
	assertEqualStmt(stmt,(int)ppmode,(int)SQL_PARAM_INPUT);
	assertEqualStmt(stmt,(const char *)pptypename,"DATE");
	if (issqlrelay) {
		assertEqualStmt(stmt,(int)ppordinal,4);
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
		"	testsmallint",
		SQL_NTS);
	assertEqualStmt(stmt,(int)erg,(int)SQL_ERROR);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select "
		"	* "
		"from "
		"	testtable "
		"order by "
		"	testsmallint",
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
