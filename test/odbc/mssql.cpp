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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "asserts.cpp"

#define USEDSN 1

SQLRETURN	erg;
SQLHENV		env;
SQLHDBC		dbc;
SQLHSTMT	stmt;

int main(int argc, char **argv) {

	// allocate environemnt handle
	printf("ENV HANDLE: \n");
#if (ODBCVER >= 0x3000)
	erg=SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&env);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);

#if defined(SQL_OV_ODBC3_80)
	erg=SQLSetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
				(SQLPOINTER)SQL_OV_ODBC3_80,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
#elif defined(SQL_OV_ODBC3)
	erg=SQLSetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
				(SQLPOINTER)SQL_OV_ODBC3,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
#else
	erg=SQLSetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
				(SQLPOINTER)SQL_OV_ODBC2,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
#endif

#else
	erg=SQLAllocEnv(&env);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
#endif
	printf("\n");

	// allocate connection handle
	printf("CONNECTION HANDLE: \n");
#if (ODBCVER >= 0x0300)
	erg=SQLAllocHandle(SQL_HANDLE_DBC,env,&dbc);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
#else
	erg=SQLAllocConnect(env,&dbc);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
#endif
	printf("\n");

	// connect
	printf("CONNECT: \n");
#ifdef USEDSN
	SQLCHAR	*dsn=(SQLCHAR *)"sqlrodbc";
	SQLCHAR	*user=(SQLCHAR *)"testuser";
	SQLCHAR	*password=(SQLCHAR *)"testpassword";
	erg=SQLConnect(dbc,dsn,SQL_NTS,user,SQL_NTS,password,SQL_NTS);
#else
	SQLCHAR		*incstring=(SQLCHAR *)"Driver={SQL Relay};Server=localhost;Port=8000;User=test;Password=test;LazyConnect=0;Debug=1;";
	SQLCHAR		outcstring[1024];
	SQLSMALLINT	outcstringlen;
	erg=SQLDriverConnect(dbc,NULL,
				incstring,SQL_NTS,
				outcstring,sizeof(outcstring),&outcstringlen,
				SQL_DRIVER_NOPROMPT);
#endif
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	printf("\n");

	// get database type
	printf("SQLGETINFO: \n");
	SQLUSMALLINT	usmallintval;
	SQLUINTEGER	uintval;
	SQLULEN		ulenval;
	SQLCHAR		strval[2048];
	SQLSMALLINT	vallen;
	erg=SQLGetInfo(dbc,SQL_ACTIVE_CONNECTIONS,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_ACTIVE_STATEMENTS,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
#ifdef USEDSN
	erg=SQLGetInfo(dbc,SQL_DATA_SOURCE_NAME,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"sqlrodbc");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
#endif
	erg=SQLGetInfo(dbc,SQL_FETCH_DIRECTION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_FD_FETCH_NEXT);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SERVER_NAME,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"localhost");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SEARCH_PATTERN_ESCAPE,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	//assertEqualString((const char *)strval,"%");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	/*erg=SQLGetInfo(dbc,SQL_DATABASE_NAME,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"testdb");*/
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DBMS_NAME,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"odbc");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DBMS_VER,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	//assertEqualString((const char *)strval,"12.00.2000");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_ACCESSIBLE_TABLES,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"N");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_ACCESSIBLE_PROCEDURES,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"N");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CURSOR_COMMIT_BEHAVIOR,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,SQL_CB_CLOSE);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DATA_SOURCE_READ_ONLY,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"N");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DEFAULT_TXN_ISOLATION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_TXN_READ_COMMITTED);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_IDENTIFIER_CASE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,SQL_IC_MIXED);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_IDENTIFIER_QUOTE_CHAR,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"\"");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMN_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_CURSOR_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_OWNER_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,128);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_CATALOG_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_TABLE_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SCROLL_CONCURRENCY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_SCCO_READ_ONLY);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_TXN_CAPABLE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,SQL_TC_ALL);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_USER_NAME,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	//assertEqualString((const char *)strval,"dbo");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_TXN_ISOLATION_OPTION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_TXN_READ_UNCOMMITTED|
					SQL_TXN_READ_COMMITTED|
					SQL_TXN_REPEATABLE_READ|
					SQL_TXN_SERIALIZABLE);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_INTEGRITY,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"Y");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_GETDATA_EXTENSIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_GD_BLOCK);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_NULL_COLLATION,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,SQL_NC_LOW);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_ALTER_TABLE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0
				#if (ODBCVER >= 0x0200)
				|SQL_AT_ADD_COLUMN
				|SQL_AT_DROP_COLUMN
				#endif
				#if (ODBCVER >= 0x0300)
				|SQL_AT_ADD_COLUMN_SINGLE
				|SQL_AT_ADD_COLUMN_DEFAULT
				|SQL_AT_ADD_COLUMN_COLLATION
				|SQL_AT_SET_COLUMN_DEFAULT
				|SQL_AT_DROP_COLUMN_DEFAULT
				|SQL_AT_DROP_COLUMN_CASCADE
				|SQL_AT_DROP_COLUMN_RESTRICT
				|SQL_AT_ADD_TABLE_CONSTRAINT
				|SQL_AT_DROP_TABLE_CONSTRAINT_CASCADE
				|SQL_AT_DROP_TABLE_CONSTRAINT_RESTRICT
				|SQL_AT_CONSTRAINT_NAME_DEFINITION
				|SQL_AT_CONSTRAINT_INITIALLY_DEFERRED
				|SQL_AT_CONSTRAINT_INITIALLY_IMMEDIATE
				|SQL_AT_CONSTRAINT_DEFERRABLE
				|SQL_AT_CONSTRAINT_NON_DEFERRABLE
				#endif
				);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_ORDER_BY_COLUMNS_IN_SELECT,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"N");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SPECIAL_CHARACTERS,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"#$_");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_GROUP_BY,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_INDEX,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_ORDER_BY,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_SELECT,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_TABLE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_INDEX_SIZE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_ROW_SIZE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_STATEMENT_LEN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_TABLES_IN_SELECT,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_USER_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	#if (ODBCVER >= 0x0300)
	erg=SQLGetInfo(dbc,SQL_OJ_CAPABILITIES,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_OJ_LEFT|
					SQL_OJ_RIGHT|
					SQL_OJ_FULL|
					SQL_OJ_NESTED|
					SQL_OJ_NOT_ORDERED|
					SQL_OJ_INNER|
					SQL_OJ_ALL_COMPARISON_OPS);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_XOPEN_CLI_YEAR,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	// FIXME: driver should return 1996, but I get 1995???
	// maybe this is intercepted by the driver manager
	//assertEqualString((const char *)strval,"1996");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CURSOR_SENSITIVITY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_UNSPECIFIED);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DESCRIBE_PARAMETER,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"N");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CATALOG_NAME,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"Y");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_COLLATION_SEQ,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_IDENTIFIER_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	#endif
	assertEqualInt(usmallintval,128);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DRIVER_HDBC,
			(SQLPOINTER)&ulenval,
			(SQLSMALLINT)sizeof(ulenval),
			&vallen);
	// unsupported but returns success
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DRIVER_HENV,
			(SQLPOINTER)&ulenval,
			(SQLSMALLINT)sizeof(ulenval),
			&vallen);
	// unsupported but returns success
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DRIVER_HSTMT,
			(SQLPOINTER)&ulenval,
			(SQLSMALLINT)sizeof(ulenval),
			&vallen);
	// unsupported but returns success
	// FIXME: this should return success
	// maybe this is intercepted by the driver manager
	assertEqualInt((erg==SQL_ERROR)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DRIVER_NAME,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"libsqlrodbc.so");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DRIVER_VER,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	//assertEqualString((const char *)strval,conn->con->clientVersion());
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_ODBC_API_CONFORMANCE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,SQL_OAC_LEVEL2);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_ODBC_VER,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	//assertEqualString((const char *)strval,conn->con->clientVersion());
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_ROW_UPDATES,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"N");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	/*erg=SQLGetInfo(dbc,SQL_ODBC_SAG_CLI_CONFORMANCE,
			debugPrintf("  unsupported infotype: "
					"SQL_ODBC_SAG_CLI_CONFORMANCE\n");
	assertEqualInt((erg==SQL_ERROR)?1:0,1);*/
	erg=SQLGetInfo(dbc,SQL_ODBC_SQL_CONFORMANCE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,SQL_OSC_EXTENDED);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_PROCEDURES,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"Y");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONCAT_NULL_BEHAVIOR,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,SQL_CB_NON_NULL);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CURSOR_ROLLBACK_BEHAVIOR,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,SQL_CB_CLOSE);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_EXPRESSIONS_IN_ORDERBY,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"Y");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_PROCEDURE_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MULT_RESULT_SETS,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"N");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MULTIPLE_ACTIVE_TXN,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"N");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_OUTER_JOINS,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"Y");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_OWNER_TERM,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"schema");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_PROCEDURE_TERM,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"stored procedure");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_QUALIFIER_NAME_SEPARATOR,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,".");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_QUALIFIER_TERM,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"catalog");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SCROLL_OPTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_SO_FORWARD_ONLY);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_TABLE_TERM,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"table");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_FN_CVT_CAST|SQL_FN_CVT_CONVERT);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_NUMERIC_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_FN_NUM_ABS|
					SQL_FN_NUM_ACOS|
					SQL_FN_NUM_ASIN|
					SQL_FN_NUM_ATAN|
					SQL_FN_NUM_ATAN2|
					SQL_FN_NUM_CEILING|
					SQL_FN_NUM_COS|
					SQL_FN_NUM_COT|
					SQL_FN_NUM_DEGREES|
					SQL_FN_NUM_EXP|
					SQL_FN_NUM_FLOOR|
					SQL_FN_NUM_LOG|
					SQL_FN_NUM_LOG10|
					SQL_FN_NUM_MOD|
					SQL_FN_NUM_PI|
					SQL_FN_NUM_POWER|
					SQL_FN_NUM_RADIANS|
					SQL_FN_NUM_RAND|
					SQL_FN_NUM_ROUND|
					SQL_FN_NUM_SIGN|
					SQL_FN_NUM_SIN|
					SQL_FN_NUM_SQRT|
					SQL_FN_NUM_TAN|
					SQL_FN_NUM_TRUNCATE);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_STRING_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_FN_STR_CONCAT|
					SQL_FN_STR_INSERT|
					SQL_FN_STR_LEFT|
					SQL_FN_STR_LTRIM|
					SQL_FN_STR_LENGTH|
					SQL_FN_STR_LOCATE|
					SQL_FN_STR_LCASE|
					SQL_FN_STR_REPEAT|
					SQL_FN_STR_REPLACE|
					SQL_FN_STR_RIGHT|
					SQL_FN_STR_RTRIM|
					SQL_FN_STR_SUBSTRING|
					SQL_FN_STR_UCASE|
					SQL_FN_STR_ASCII|
					SQL_FN_STR_CHAR|
					SQL_FN_STR_DIFFERENCE|
					SQL_FN_STR_LOCATE_2|
					SQL_FN_STR_SOUNDEX|
					SQL_FN_STR_SPACE|
					SQL_FN_STR_BIT_LENGTH|
					SQL_FN_STR_CHAR_LENGTH|
					SQL_FN_STR_CHARACTER_LENGTH|
					SQL_FN_STR_OCTET_LENGTH|
					SQL_FN_STR_POSITION);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SYSTEM_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_FN_SYS_DBNAME|
					SQL_FN_SYS_IFNULL|
					SQL_FN_SYS_USERNAME);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_TIMEDATE_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_FN_TD_CURRENT_DATE|
					SQL_FN_TD_CURRENT_TIME|
					SQL_FN_TD_CURRENT_TIMESTAMP|
					SQL_FN_TD_CURDATE|
					SQL_FN_TD_CURTIME|
					SQL_FN_TD_DAYNAME|
					SQL_FN_TD_DAYOFMONTH|
					SQL_FN_TD_DAYOFWEEK|
					SQL_FN_TD_DAYOFYEAR|
					SQL_FN_TD_EXTRACT|
					SQL_FN_TD_HOUR|
					SQL_FN_TD_MINUTE|
					SQL_FN_TD_MONTH|
					SQL_FN_TD_MONTHNAME|
					SQL_FN_TD_NOW|
					SQL_FN_TD_QUARTER|
					SQL_FN_TD_SECOND|
					SQL_FN_TD_TIMESTAMPADD|
					SQL_FN_TD_TIMESTAMPDIFF|
					SQL_FN_TD_WEEK|
					SQL_FN_TD_YEAR);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_BIGINT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_BINARY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_BIT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_CHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_DATE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_DECIMAL,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_DOUBLE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_FLOAT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_INTEGER,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_LONGVARCHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_NUMERIC,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_REAL,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_SMALLINT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_TIME,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_TIMESTAMP,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_TINYINT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_VARBINARY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_VARCHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_LONGVARBINARY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CORRELATION_NAME,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,SQL_CN_ANY);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_NON_NULLABLE_COLUMNS,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,SQL_NNC_NON_NULL);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DRIVER_HLIB,
			(SQLPOINTER)&ulenval,
			(SQLSMALLINT)sizeof(ulenval),
			&vallen);
	// unsupported but returns success
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DRIVER_ODBC_VER,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	// FIXME: this returns 02.00 but I'd expect 03.00
	/*
	#if (ODBCVER >= 0x0380)
	// FIXME: not sure why we're doing this
	//assertEqualString((const char *)strval,"03.80");
	assertEqualString((const char *)strval,"03.00");
	#elif (ODBCVER >= 0x0300)
	assertEqualString((const char *)strval,"03.00");
	#else
	assertEqualString((const char *)strval,"02.00");
	#endif
	*/
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_LOCK_TYPES,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_LCK_NO_CHANGE|
					SQL_LCK_EXCLUSIVE|
					SQL_LCK_UNLOCK);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_POS_OPERATIONS,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,SQL_POS_POSITION);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_POSITIONED_STATEMENTS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_BOOKMARK_PERSISTENCE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_STATIC_SENSITIVITY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_FILE_USAGE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_FILE_NOT_SUPPORTED);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_COLUMN_ALIAS,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"Y");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_GROUP_BY,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,
				#if (ODBCVER >= 0x0300)
				SQL_GB_COLLATE
				#else
				SQL_GB_GROUP_BY_EQUALS_SELECT
				#endif
				);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_KEYWORDS,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,SQL_ODBC_KEYWORDS);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_OWNER_USAGE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_SU_DML_STATEMENTS|
					SQL_SU_PROCEDURE_INVOCATION|
					SQL_SU_TABLE_DEFINITION|
					SQL_SU_INDEX_DEFINITION|
					SQL_SU_PRIVILEGE_DEFINITION);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_QUALIFIER_USAGE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_SU_DML_STATEMENTS|
					SQL_SU_PROCEDURE_INVOCATION|
					SQL_SU_TABLE_DEFINITION|
					SQL_SU_INDEX_DEFINITION|
					SQL_SU_PRIVILEGE_DEFINITION);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_QUOTED_IDENTIFIER_CASE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,SQL_IC_SENSITIVE);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SUBQUERIES,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_SQ_CORRELATED_SUBQUERIES|
					SQL_SQ_COMPARISON|
					SQL_SQ_EXISTS|
					SQL_SQ_IN|
					SQL_SQ_QUANTIFIED);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_UNION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_U_UNION|SQL_U_UNION_ALL);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_ROW_SIZE_INCLUDES_LONG,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"N");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_CHAR_LITERAL_LEN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_TIMEDATE_ADD_INTERVALS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_TIMEDATE_DIFF_INTERVALS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_NEED_LONG_DATA_LEN,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"Y");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_BINARY_LITERAL_LEN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_LIKE_ESCAPE_CLAUSE,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualString((const char *)strval,"Y");
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_QUALIFIER_LOCATION,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,SQL_CL_START);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	#if (ODBCVER >= 0x0300)
	erg=SQLGetInfo(dbc,SQL_ACTIVE_ENVIRONMENTS,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualInt(usmallintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_ALTER_DOMAIN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_AD_ADD_DOMAIN_CONSTRAINT|
					SQL_AD_ADD_DOMAIN_DEFAULT|
					SQL_AD_CONSTRAINT_NAME_DEFINITION|
					SQL_AD_DROP_DOMAIN_CONSTRAINT|
					SQL_AD_DROP_DOMAIN_DEFAULT);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SQL_CONFORMANCE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_SC_SQL92_ENTRY);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DATETIME_LITERALS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_ASYNC_MODE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_AM_NONE);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_BATCH_ROW_COUNT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_BATCH_SUPPORT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_WCHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_INTERVAL_DAY_TIME,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_INTERVAL_YEAR_MONTH,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_WLONGVARCHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CONVERT_WVARCHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CREATE_ASSERTION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CREATE_CHARACTER_SET,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CREATE_COLLATION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CREATE_DOMAIN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CREATE_SCHEMA,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_CS_CREATE_SCHEMA|
					SQL_CS_AUTHORIZATION|
					SQL_CS_DEFAULT_CHARACTER_SET);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CREATE_TABLE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_CT_CREATE_TABLE|
					SQL_CT_TABLE_CONSTRAINT|
					SQL_CT_CONSTRAINT_NAME_DEFINITION|
					SQL_CT_COMMIT_DELETE|
					SQL_CT_GLOBAL_TEMPORARY|
					SQL_CT_COLUMN_CONSTRAINT|
					SQL_CT_COLUMN_DEFAULT|
					SQL_CT_COLUMN_COLLATION|
					SQL_CT_CONSTRAINT_INITIALLY_IMMEDIATE|
					SQL_CT_CONSTRAINT_NON_DEFERRABLE);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CREATE_TRANSLATION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_CREATE_VIEW,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_CV_CREATE_VIEW|
					SQL_CV_CHECK_OPTION|
					SQL_CV_CASCADED|
					SQL_CV_LOCAL);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DRIVER_HDESC,
			(SQLPOINTER)&ulenval,
			(SQLSMALLINT)sizeof(ulenval),
			&vallen);
	// unsupported but returns success
	// FIXME: this should return success
	// maybe this is intercepted by the driver manager
	assertEqualInt((erg==SQL_ERROR)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DROP_ASSERTION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DROP_CHARACTER_SET,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DROP_COLLATION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DROP_DOMAIN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DROP_SCHEMA,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_DS_DROP_SCHEMA|
					SQL_DS_CASCADE|
					SQL_DS_RESTRICT);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DROP_TABLE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_DT_DROP_TABLE|
					SQL_DT_CASCADE|
					SQL_DT_RESTRICT);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DROP_TRANSLATION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DROP_VIEW,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_DV_DROP_VIEW|
					SQL_DV_CASCADE|
					SQL_DV_RESTRICT);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DYNAMIC_CURSOR_ATTRIBUTES1,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_CA1_NEXT|SQL_CA1_POS_POSITION);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DYNAMIC_CURSOR_ATTRIBUTES2,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_CA2_READ_ONLY_CONCURRENCY);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_CA1_NEXT|SQL_CA1_POS_POSITION);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_CA2_READ_ONLY_CONCURRENCY);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_INDEX_KEYWORDS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_IK_ALL);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_INFO_SCHEMA_VIEWS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_KEYSET_CURSOR_ATTRIBUTES1,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_CA1_NEXT|SQL_CA1_POS_POSITION);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_KEYSET_CURSOR_ATTRIBUTES2,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_CA2_READ_ONLY_CONCURRENCY);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_MAX_ASYNC_CONCURRENT_STATEMENTS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_ODBC_INTERFACE_CONFORMANCE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_OIC_CORE);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_PARAM_ARRAY_ROW_COUNTS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,0);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_PARAM_ARRAY_SELECTS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_PAS_NO_SELECT);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SQL92_DATETIME_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_SDF_CURRENT_DATE|
					SQL_SDF_CURRENT_TIME|
					SQL_SDF_CURRENT_TIMESTAMP);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SQL92_FOREIGN_KEY_DELETE_RULE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_SFKD_CASCADE);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SQL92_FOREIGN_KEY_UPDATE_RULE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_SFKU_CASCADE);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SQL92_GRANT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_SG_DELETE_TABLE|
					SQL_SG_INSERT_COLUMN|
					SQL_SG_INSERT_TABLE|
					SQL_SG_REFERENCES_TABLE|
					SQL_SG_REFERENCES_COLUMN|
					SQL_SG_SELECT_TABLE|
					SQL_SG_UPDATE_COLUMN|
					SQL_SG_UPDATE_TABLE|
					SQL_SG_USAGE_ON_DOMAIN|
					SQL_SG_USAGE_ON_CHARACTER_SET|
					SQL_SG_USAGE_ON_COLLATION|
					SQL_SG_USAGE_ON_TRANSLATION|
					SQL_SG_WITH_GRANT_OPTION);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SQL92_NUMERIC_VALUE_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_SNVF_BIT_LENGTH|
					SQL_SNVF_CHAR_LENGTH|
					SQL_SNVF_CHARACTER_LENGTH|
					SQL_SNVF_EXTRACT|
					SQL_SNVF_OCTET_LENGTH|
					SQL_SNVF_POSITION);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SQL92_PREDICATES,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_SP_BETWEEN|
					SQL_SP_COMPARISON|
					SQL_SP_EXISTS|
					SQL_SP_IN|
					SQL_SP_ISNOTNULL|
					SQL_SP_ISNULL|
					SQL_SP_LIKE|
					SQL_SP_MATCH_FULL|
					SQL_SP_MATCH_PARTIAL|
					SQL_SP_MATCH_UNIQUE_FULL|
					SQL_SP_MATCH_UNIQUE_PARTIAL|
					SQL_SP_OVERLAPS|
					SQL_SP_QUANTIFIED_COMPARISON|
					SQL_SP_UNIQUE);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SQL92_RELATIONAL_JOIN_OPERATORS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_SRJO_CORRESPONDING_CLAUSE|
					SQL_SRJO_CROSS_JOIN|
					SQL_SRJO_EXCEPT_JOIN|
					SQL_SRJO_FULL_OUTER_JOIN|
					SQL_SRJO_INNER_JOIN|
					SQL_SRJO_INTERSECT_JOIN|
					SQL_SRJO_LEFT_OUTER_JOIN|
					SQL_SRJO_NATURAL_JOIN|
					SQL_SRJO_RIGHT_OUTER_JOIN|
					SQL_SRJO_UNION_JOIN);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SQL92_REVOKE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_SR_CASCADE|
					SQL_SR_DELETE_TABLE|
					SQL_SR_GRANT_OPTION_FOR|
					SQL_SR_INSERT_COLUMN|
					SQL_SR_INSERT_TABLE|
					SQL_SR_REFERENCES_COLUMN|
					SQL_SR_REFERENCES_TABLE|
					SQL_SR_RESTRICT|
					SQL_SR_SELECT_TABLE|
					SQL_SR_UPDATE_COLUMN|
					SQL_SR_UPDATE_TABLE|
					SQL_SR_USAGE_ON_DOMAIN|
					SQL_SR_USAGE_ON_CHARACTER_SET|
					SQL_SR_USAGE_ON_COLLATION|
					SQL_SR_USAGE_ON_TRANSLATION);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SQL92_ROW_VALUE_CONSTRUCTOR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_SRVC_VALUE_EXPRESSION|
					SQL_SRVC_NULL|
					SQL_SRVC_DEFAULT|
					SQL_SRVC_ROW_SUBQUERY);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SQL92_STRING_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_SSF_CONVERT|
					SQL_SSF_LOWER|
					SQL_SSF_UPPER|
					SQL_SSF_SUBSTRING|
					SQL_SSF_TRANSLATE|
					SQL_SSF_TRIM_BOTH|
					SQL_SSF_TRIM_LEADING|
					SQL_SSF_TRIM_TRAILING);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_SQL92_VALUE_EXPRESSIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_SVE_CASE|
					SQL_SVE_CAST|
					SQL_SVE_COALESCE|
					SQL_SVE_NULLIF);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_STANDARD_CLI_CONFORMANCE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_SCC_XOPEN_CLI_VERSION1);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_STATIC_CURSOR_ATTRIBUTES1,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_CA1_NEXT|SQL_CA1_POS_POSITION);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_STATIC_CURSOR_ATTRIBUTES2,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_CA2_READ_ONLY_CONCURRENCY);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_AGGREGATE_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_AF_ALL|
					SQL_AF_AVG|
					SQL_AF_COUNT|
					SQL_AF_DISTINCT|
					SQL_AF_MAX|
					SQL_AF_MIN|
					SQL_AF_SUM);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_DDL_INDEX,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_DI_CREATE_INDEX|SQL_DI_DROP_INDEX);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLGetInfo(dbc,SQL_INSERT_STATEMENT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_IS_INSERT_LITERALS|
					SQL_IS_INSERT_SEARCHED|
					SQL_IS_SELECT_INTO);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	#if (ODBCVER >= 0x0380)
	erg=SQLGetInfo(dbc,SQL_ASYNC_DBC_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualInt(uintval,SQL_ASYNC_DBC_NOT_CAPABLE);
	#endif
	#endif
	#ifdef SQL_DTC_TRANSITION_COST
	/*erg=SQLGetInfo(dbc,SQL_DTC_TRANSITION_COST,
			debugPrintf("  unsupported infotype: "
					"SQL_DTC_TRANSITION_COST\n");
	assertEqualInt((erg==SQL_ERROR)?1:0,1);*/
	#endif
	printf("\n");

	// drop existing table
	printf("DROP EXISTING TABLE\n");
	erg=SQLAllocHandle(SQL_HANDLE_STMT,dbc,&stmt);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	printf("\n");


	printf("CREATE TEMPTABLE: \n");
	erg=SQLExecDirect(stmt,(SQLCHAR *)"create table testtable (testint int, testsmallint smallint, testtinyint tinyint, testreal real, testfloat float, testdecimal decimal(4,1), testnumeric numeric(4,1), testmoney money, testsmallmoney smallmoney, testdatetime datetime, testsmalldatetime smalldatetime, testchar char(40), testvarchar varchar(40), testbit bit)",SQL_NTS);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	printf("\n");

	printf("BEGIN TRANSACTION: \n");
	SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
				(SQLPOINTER)SQL_AUTOCOMMIT_OFF,
				sizeof(SQLINTEGER));
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	printf("\n");

	printf("INSERT: \n");
	erg=SQLExecDirect(stmt,(SQLCHAR *)"insert into testtable values (1,1,1,1.1,1.1,1.1,1.1,1.00,1.00,'01-Jan-2001 01:00:00','01-Jan-2001 01:00:00','testchar1','testvarchar1',1)",SQL_NTS);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	printf("\n");

	printf("AFFECTED ROWS: \n");
	#ifdef SQLROWCOUNT_SQLLEN
	SQLLEN		affectedrows;
	#else
	SQLINTEGER	affectedrows;
	#endif
	erg=SQLRowCount(stmt,&affectedrows);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	assertEqualInt(affectedrows,1);
	printf("\n");

	printf("BIND BY POSITION: \n");
	erg=SQLPrepare(stmt,(SQLCHAR *)"insert into testtable values (?,?,?,?,?,?,?,?,?,?,?,?,?,?)",SQL_NTS);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	SQLSMALLINT	bindvarcount;
	erg=SQLNumParams(stmt,&bindvarcount);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	assertEqualInt(bindvarcount,14);

	SQLINTEGER	intval=2;
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				sizeof(SQLINTEGER),NULL);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
	SQLSMALLINT	smallintval=2;
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
				SQL_C_SSHORT,SQL_SMALLINT,
				0,0,
				(SQLPOINTER)&smallintval,
				sizeof(SQLSMALLINT),NULL);
	assertEqualInt((erg==SQL_SUCCESS || erg==SQL_SUCCESS_WITH_INFO)?1:0,1);
#if 0
	sqlrcur_inputBindLong(cur,"3",2);
	sqlrcur_inputBindDouble(cur,"4",2.2,2,1);
	sqlrcur_inputBindDouble(cur,"5",2.2,2,1);
	sqlrcur_inputBindDouble(cur,"6",2.2,2,1);
	sqlrcur_inputBindDouble(cur,"7",2.2,2,1);
	sqlrcur_inputBindDouble(cur,"8",2.00,3,2);
	sqlrcur_inputBindDouble(cur,"9",2.00,3,2);
	sqlrcur_inputBindString(cur,"10","01-Jan-2002 02:00:00");
	sqlrcur_inputBindString(cur,"11","01-Jan-2002 02:00:00");
	sqlrcur_inputBindString(cur,"12","testchar2");
	sqlrcur_inputBindString(cur,"13","testvarchar2");
	sqlrcur_inputBindLong(cur,"14",1);
	assertEqualInt(sqlrcur_executeQuery(cur),1);
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",3);
	sqlrcur_inputBindLong(cur,"2",3);
	sqlrcur_inputBindLong(cur,"3",3);
	sqlrcur_inputBindDouble(cur,"4",3.3,2,1);
	sqlrcur_inputBindDouble(cur,"5",3.3,2,1);
	sqlrcur_inputBindDouble(cur,"6",3.3,2,1);
	sqlrcur_inputBindDouble(cur,"7",3.3,2,1);
	sqlrcur_inputBindDouble(cur,"8",3.00,3,2);
	sqlrcur_inputBindDouble(cur,"9",3.00,3,2);
	sqlrcur_inputBindString(cur,"10","01-Jan-2003 03:00:00");
	sqlrcur_inputBindString(cur,"11","01-Jan-2003 03:00:00");
	sqlrcur_inputBindString(cur,"12","testchar3");
	sqlrcur_inputBindString(cur,"13","testvarchar3");
	sqlrcur_inputBindLong(cur,"14",1);
	assertEqualInt(sqlrcur_executeQuery(cur),1);
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"1",4);
	sqlrcur_inputBindLong(cur,"2",4);
	sqlrcur_inputBindLong(cur,"3",4);
	sqlrcur_inputBindDouble(cur,"4",4.4,2,1);
	sqlrcur_inputBindDouble(cur,"5",4.4,2,1);
	sqlrcur_inputBindDouble(cur,"6",4.4,2,1);
	sqlrcur_inputBindDouble(cur,"7",4.4,2,1);
	sqlrcur_inputBindDouble(cur,"8",4.00,3,2);
	sqlrcur_inputBindDouble(cur,"9",4.00,3,2);
	sqlrcur_inputBindString(cur,"10","01-Jan-2004 04:00:00");
	sqlrcur_inputBindString(cur,"11","01-Jan-2004 04:00:00");
	sqlrcur_inputBindString(cur,"12","testchar4");
	sqlrcur_inputBindString(cur,"13","testvarchar4");
	sqlrcur_inputBindLong(cur,"14",1);
	assertEqualInt(sqlrcur_executeQuery(cur),1);
	printf("\n");

	printf("BIND BY NAME: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_prepareQuery(cur,"insert into testtable values (@var1,@var2,@var3,@var4,@var5,@var6,@var7,@var8,@var9,@var10,@var11,@var12,@var13,@var14)");
	sqlrcur_inputBindLong(cur,"var1",5);
	sqlrcur_inputBindLong(cur,"var2",5);
	sqlrcur_inputBindLong(cur,"var3",5);
	sqlrcur_inputBindDouble(cur,"var4",5.5,2,1);
	sqlrcur_inputBindDouble(cur,"var5",5.5,2,1);
	sqlrcur_inputBindDouble(cur,"var6",5.5,2,1);
	sqlrcur_inputBindDouble(cur,"var7",5.5,2,1);
	sqlrcur_inputBindDouble(cur,"var8",5.00,3,2);
	sqlrcur_inputBindDouble(cur,"var9",5.00,3,2);
	sqlrcur_inputBindString(cur,"var10","01-Jan-2005 05:00:00");
	sqlrcur_inputBindString(cur,"var11","01-Jan-2005 05:00:00");
	sqlrcur_inputBindString(cur,"var12","testchar5");
	sqlrcur_inputBindString(cur,"var13","testvarchar5");
	sqlrcur_inputBindLong(cur,"var14",1);
	assertEqualInt(sqlrcur_executeQuery(cur),1);
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",6);
	sqlrcur_inputBindLong(cur,"var2",6);
	sqlrcur_inputBindLong(cur,"var3",6);
	sqlrcur_inputBindDouble(cur,"var4",6.6,2,1);
	sqlrcur_inputBindDouble(cur,"var5",6.6,2,1);
	sqlrcur_inputBindDouble(cur,"var6",6.6,2,1);
	sqlrcur_inputBindDouble(cur,"var7",6.6,2,1);
	sqlrcur_inputBindDouble(cur,"var8",6.00,3,2);
	sqlrcur_inputBindDouble(cur,"var9",6.00,3,2);
	sqlrcur_inputBindString(cur,"var10","01-Jan-2006 06:00:00");
	sqlrcur_inputBindString(cur,"var11","01-Jan-2006 06:00:00");
	sqlrcur_inputBindString(cur,"var12","testchar6");
	sqlrcur_inputBindString(cur,"var13","testvarchar6");
	sqlrcur_inputBindLong(cur,"var14",1);
	assertEqualInt(sqlrcur_executeQuery(cur),1);
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",7);
	sqlrcur_inputBindLong(cur,"var2",7);
	sqlrcur_inputBindLong(cur,"var3",7);
	sqlrcur_inputBindDouble(cur,"var4",7.7,2,1);
	sqlrcur_inputBindDouble(cur,"var5",7.7,2,1);
	sqlrcur_inputBindDouble(cur,"var6",7.7,2,1);
	sqlrcur_inputBindDouble(cur,"var7",7.7,2,1);
	sqlrcur_inputBindDouble(cur,"var8",7.00,3,2);
	sqlrcur_inputBindDouble(cur,"var9",7.00,3,2);
	sqlrcur_inputBindString(cur,"var10","01-Jan-2007 07:00:00");
	sqlrcur_inputBindString(cur,"var11","01-Jan-2007 07:00:00");
	sqlrcur_inputBindString(cur,"var12","testchar7");
	sqlrcur_inputBindString(cur,"var13","testvarchar7");
	sqlrcur_inputBindLong(cur,"var14",1);
	assertEqualInt(sqlrcur_executeQuery(cur),1);
	printf("\n");

	printf("BIND BY NAME WITH VALIDATION: \n");
	sqlrcur_clearBinds(cur);
	sqlrcur_inputBindLong(cur,"var1",8);
	sqlrcur_inputBindLong(cur,"var2",8);
	sqlrcur_inputBindLong(cur,"var3",8);
	sqlrcur_inputBindDouble(cur,"var4",8.8,2,1);
	sqlrcur_inputBindDouble(cur,"var5",8.8,2,1);
	sqlrcur_inputBindDouble(cur,"var6",8.8,2,1);
	sqlrcur_inputBindDouble(cur,"var7",8.8,2,1);
	sqlrcur_inputBindDouble(cur,"var8",8.00,3,2);
	sqlrcur_inputBindDouble(cur,"var9",8.00,3,2);
	sqlrcur_inputBindString(cur,"var10","01-Jan-2008 08:00:00");
	sqlrcur_inputBindString(cur,"var11","01-Jan-2008 08:00:00");
	sqlrcur_inputBindString(cur,"var12","testchar8");
	sqlrcur_inputBindString(cur,"var13","testvarchar8");
	sqlrcur_inputBindLong(cur,"var14",1);
	sqlrcur_inputBindString(cur,"var15","junkvalue");
	sqlrcur_validateBinds(cur);
	assertEqualInt(sqlrcur_executeQuery(cur),1);
	printf("\n");

	printf("SELECT: \n");
	assertEqualInt(sqlrcur_sendQuery(cur,"select * from testtable order by testint"),1);
	printf("\n");

	printf("COLUMN COUNT: \n");
	assertEqualInt(sqlrcur_colCount(cur),14);
	printf("\n");

	printf("COLUMN NAMES: \n");
	assertEqualString(sqlrcur_getColumnName(cur,0),"testint");
	assertEqualString(sqlrcur_getColumnName(cur,1),"testsmallint");
	assertEqualString(sqlrcur_getColumnName(cur,2),"testtinyint");
	assertEqualString(sqlrcur_getColumnName(cur,3),"testreal");
	assertEqualString(sqlrcur_getColumnName(cur,4),"testfloat");
	assertEqualString(sqlrcur_getColumnName(cur,5),"testdecimal");
	assertEqualString(sqlrcur_getColumnName(cur,6),"testnumeric");
	assertEqualString(sqlrcur_getColumnName(cur,7),"testmoney");
	assertEqualString(sqlrcur_getColumnName(cur,8),"testsmallmoney");
	assertEqualString(sqlrcur_getColumnName(cur,9),"testdatetime");
	assertEqualString(sqlrcur_getColumnName(cur,10),"testsmalldatetime");
	assertEqualString(sqlrcur_getColumnName(cur,11),"testchar");
	assertEqualString(sqlrcur_getColumnName(cur,12),"testvarchar");
	assertEqualString(sqlrcur_getColumnName(cur,13),"testbit");
	cols=sqlrcur_getColumnNames(cur);
	assertEqualString(cols[0],"testint");
	assertEqualString(cols[1],"testsmallint");
	assertEqualString(cols[2],"testtinyint");
	assertEqualString(cols[3],"testreal");
	assertEqualString(cols[4],"testfloat");
	assertEqualString(cols[5],"testdecimal");
	assertEqualString(cols[6],"testnumeric");
	assertEqualString(cols[7],"testmoney");
	assertEqualString(cols[8],"testsmallmoney");
	assertEqualString(cols[9],"testdatetime");
	assertEqualString(cols[10],"testsmalldatetime");
	assertEqualString(cols[11],"testchar");
	assertEqualString(cols[12],"testvarchar");
	assertEqualString(cols[13],"testbit");
	printf("\n");

	printf("COLUMN TYPES: \n");
	assertEqualString(sqlrcur_getColumnTypeByIndex(cur,0),"INT");
	assertEqualString(sqlrcur_getColumnTypeByName(cur,"testint"),"INT");
	assertEqualString(sqlrcur_getColumnTypeByIndex(cur,1),"SMALLINT");
	assertEqualString(sqlrcur_getColumnTypeByName(cur,"testsmallint"),"SMALLINT");
	assertEqualString(sqlrcur_getColumnTypeByIndex(cur,2),"TINYINT");
	assertEqualString(sqlrcur_getColumnTypeByName(cur,"testtinyint"),"TINYINT");
	assertEqualString(sqlrcur_getColumnTypeByIndex(cur,3),"REAL");
	assertEqualString(sqlrcur_getColumnTypeByName(cur,"testreal"),"REAL");
	assertEqualString(sqlrcur_getColumnTypeByIndex(cur,4),"FLOAT");
	assertEqualString(sqlrcur_getColumnTypeByName(cur,"testfloat"),"FLOAT");
	assertEqualString(sqlrcur_getColumnTypeByIndex(cur,5),"DECIMAL");
	assertEqualString(sqlrcur_getColumnTypeByName(cur,"testdecimal"),"DECIMAL");
	assertEqualString(sqlrcur_getColumnTypeByIndex(cur,6),"NUMERIC");
	assertEqualString(sqlrcur_getColumnTypeByName(cur,"testnumeric"),"NUMERIC");
	assertEqualString(sqlrcur_getColumnTypeByIndex(cur,7),"MONEY");
	assertEqualString(sqlrcur_getColumnTypeByName(cur,"testmoney"),"MONEY");
	assertEqualString(sqlrcur_getColumnTypeByIndex(cur,8),"SMALLMONEY");
	assertEqualString(sqlrcur_getColumnTypeByName(cur,"testsmallmoney"),"SMALLMONEY");
	assertEqualString(sqlrcur_getColumnTypeByIndex(cur,9),"DATETIME");
	assertEqualString(sqlrcur_getColumnTypeByName(cur,"testdatetime"),"DATETIME");
	assertEqualString(sqlrcur_getColumnTypeByIndex(cur,10),"SMALLDATETIME");
	assertEqualString(sqlrcur_getColumnTypeByName(cur,"testsmalldatetime"),"SMALLDATETIME");
	assertEqualString(sqlrcur_getColumnTypeByIndex(cur,11),"CHAR");
	assertEqualString(sqlrcur_getColumnTypeByName(cur,"testchar"),"CHAR");
	assertEqualString(sqlrcur_getColumnTypeByIndex(cur,12),"CHAR");
	assertEqualString(sqlrcur_getColumnTypeByName(cur,"testvarchar"),"CHAR");
	assertEqualString(sqlrcur_getColumnTypeByIndex(cur,13),"BIT");
	assertEqualString(sqlrcur_getColumnTypeByName(cur,"testbit"),"BIT");
	printf("\n");

	printf("COLUMN LENGTH: \n");
	assertEqualInt(sqlrcur_getColumnLengthByIndex(cur,0),4);
	assertEqualInt(sqlrcur_getColumnLengthByName(cur,"testint"),4);
	assertEqualInt(sqlrcur_getColumnLengthByIndex(cur,1),2);
	assertEqualInt(sqlrcur_getColumnLengthByName(cur,"testsmallint"),2);
	assertEqualInt(sqlrcur_getColumnLengthByIndex(cur,2),1);
	assertEqualInt(sqlrcur_getColumnLengthByName(cur,"testtinyint"),1);
	assertEqualInt(sqlrcur_getColumnLengthByIndex(cur,3),4);
	assertEqualInt(sqlrcur_getColumnLengthByName(cur,"testreal"),4);
	assertEqualInt(sqlrcur_getColumnLengthByIndex(cur,4),8);
	assertEqualInt(sqlrcur_getColumnLengthByName(cur,"testfloat"),8);
	/* these seem to fluctuate with every freetds release */
	/*assertEqualInt(sqlrcur_getColumnLengthByIndex(cur,5),3);
	assertEqualInt(sqlrcur_getColumnLengthByName(cur,"testdecimal"),3);
	assertEqualInt(sqlrcur_getColumnLengthByIndex(cur,6),3);
	assertEqualInt(sqlrcur_getColumnLengthByName(cur,"testnumeric"),3);*/
	assertEqualInt(sqlrcur_getColumnLengthByIndex(cur,7),8);
	assertEqualInt(sqlrcur_getColumnLengthByName(cur,"testmoney"),8);
	assertEqualInt(sqlrcur_getColumnLengthByIndex(cur,8),4);
	assertEqualInt(sqlrcur_getColumnLengthByName(cur,"testsmallmoney"),4);
	assertEqualInt(sqlrcur_getColumnLengthByIndex(cur,9),8);
	assertEqualInt(sqlrcur_getColumnLengthByName(cur,"testdatetime"),8);
	assertEqualInt(sqlrcur_getColumnLengthByIndex(cur,10),4);
	assertEqualInt(sqlrcur_getColumnLengthByName(cur,"testsmalldatetime"),4);
	/* these seem to fluctuate too */
	/*assertEqualInt(sqlrcur_getColumnLengthByIndex(cur,11),40);
	assertEqualInt(sqlrcur_getColumnLengthByName(cur,"testchar"),40);
	assertEqualInt(sqlrcur_getColumnLengthByIndex(cur,12),40);
	assertEqualInt(sqlrcur_getColumnLengthByName(cur,"testvarchar"),40);*/
	assertEqualInt(sqlrcur_getColumnLengthByIndex(cur,13),1);
	assertEqualInt(sqlrcur_getColumnLengthByName(cur,"testbit"),1);
	printf("\n");

	printf("ROW COUNT: \n");
	assertEqualInt(sqlrcur_rowCount(cur),8);
	printf("\n");

	printf("TOTAL ROWS: \n");
	assertEqualInt(sqlrcur_totalRows(cur),0);
	printf("\n");

	printf("FIRST ROW INDEX: \n");
	assertEqualInt(sqlrcur_firstRowIndex(cur),0);
	printf("\n");

	printf("END OF RESULT SET: \n");
	assertEqualInt(sqlrcur_endOfResultSet(cur),1);
	printf("\n");

	printf("FIELDS BY INDEX: \n");
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,2),"1");
	/*assertEqualString(sqlrcur_getFieldByIndex(cur,0,3),"1.1");
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,4),"1.1");*/
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,5),"1.1");
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,6),"1.1");
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,7),"1.00");
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,8),"1.00");
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,9),"Jan  1 2001 01:00:00:000AM");
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,10),"Jan  1 2001 01:00:00:000AM");
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,11),"testchar1                               ");
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,12),"testvarchar1");
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,13),"1");
	printf("\n");
	assertEqualString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	assertEqualString(sqlrcur_getFieldByIndex(cur,7,1),"8");
	assertEqualString(sqlrcur_getFieldByIndex(cur,7,2),"8");
	/*assertEqualString(sqlrcur_getFieldByIndex(cur,7,3),"8.8");
	assertEqualString(sqlrcur_getFieldByIndex(cur,7,4),"8.8");
	assertEqualString(sqlrcur_getFieldByIndex(cur,7,5),"8.8");
	assertEqualString(sqlrcur_getFieldByIndex(cur,7,6),"8.8");*/
	assertEqualString(sqlrcur_getFieldByIndex(cur,7,7),"8.00");
	assertEqualString(sqlrcur_getFieldByIndex(cur,7,8),"8.00");
	assertEqualString(sqlrcur_getFieldByIndex(cur,7,9),"Jan  1 2008 08:00:00:000AM");
	assertEqualString(sqlrcur_getFieldByIndex(cur,7,10),"Jan  1 2008 08:00:00:000AM");
	assertEqualString(sqlrcur_getFieldByIndex(cur,7,11),"testchar8                               ");
	assertEqualString(sqlrcur_getFieldByIndex(cur,7,12),"testvarchar8");
	assertEqualString(sqlrcur_getFieldByIndex(cur,7,13),"1");
	printf("\n");

	printf("FIELD LENGTHS BY INDEX: \n");
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,0,0),1);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,0,1),1);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,0,2),1);
	/*assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,0,3),3);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,0,4),3);*/
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,0,5),3);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,0,6),3);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,0,7),4);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,0,8),4);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,0,9),26);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,0,10),26);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,0,11),40);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,0,12),12);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,0,13),1);
	printf("\n");
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,7,0),1);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,7,1),1);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,7,2),1);
	/*assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,7,3),3);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,7,4),17);*/
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,7,5),3);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,7,6),3);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,7,7),4);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,7,8),4);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,7,9),26);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,7,10),26);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,7,11),40);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,7,12),12);
	assertEqualInt(sqlrcur_getFieldLengthByIndex(cur,7,13),1);
	printf("\n");

	printf("FIELDS BY NAME: \n");
	assertEqualString(sqlrcur_getFieldByName(cur,0,"testint"),"1");
	assertEqualString(sqlrcur_getFieldByName(cur,0,"testsmallint"),"1");
	assertEqualString(sqlrcur_getFieldByName(cur,0,"testtinyint"),"1");
	/*assertEqualString(sqlrcur_getFieldByName(cur,0,"testreal"),"1.1");
	assertEqualString(sqlrcur_getFieldByName(cur,0,"testfloat"),"1.1");*/
	assertEqualString(sqlrcur_getFieldByName(cur,0,"testdecimal"),"1.1");
	assertEqualString(sqlrcur_getFieldByName(cur,0,"testnumeric"),"1.1");
	assertEqualString(sqlrcur_getFieldByName(cur,0,"testmoney"),"1.00");
	assertEqualString(sqlrcur_getFieldByName(cur,0,"testsmallmoney"),"1.00");
	assertEqualString(sqlrcur_getFieldByName(cur,0,"testdatetime"),"Jan  1 2001 01:00:00:000AM");
	assertEqualString(sqlrcur_getFieldByName(cur,0,"testsmalldatetime"),"Jan  1 2001 01:00:00:000AM");
	assertEqualString(sqlrcur_getFieldByName(cur,0,"testchar"),"testchar1                               ");
	assertEqualString(sqlrcur_getFieldByName(cur,0,"testvarchar"),"testvarchar1");
	assertEqualString(sqlrcur_getFieldByName(cur,0,"testbit"),"1");
	printf("\n");
	assertEqualString(sqlrcur_getFieldByName(cur,7,"testint"),"8");
	assertEqualString(sqlrcur_getFieldByName(cur,7,"testsmallint"),"8");
	assertEqualString(sqlrcur_getFieldByName(cur,7,"testtinyint"),"8");
	/*assertEqualString(sqlrcur_getFieldByName(cur,7,"testreal"),"8.8");
	assertEqualString(sqlrcur_getFieldByName(cur,7,"testfloat"),"8.8");
	assertEqualString(sqlrcur_getFieldByName(cur,7,"testdecimal"),"8.8");
	assertEqualString(sqlrcur_getFieldByName(cur,7,"testnumeric"),"8.8");*/
	assertEqualString(sqlrcur_getFieldByName(cur,7,"testmoney"),"8.00");
	assertEqualString(sqlrcur_getFieldByName(cur,7,"testsmallmoney"),"8.00");
	assertEqualString(sqlrcur_getFieldByName(cur,7,"testdatetime"),"Jan  1 2008 08:00:00:000AM");
	assertEqualString(sqlrcur_getFieldByName(cur,7,"testsmalldatetime"),"Jan  1 2008 08:00:00:000AM");
	assertEqualString(sqlrcur_getFieldByName(cur,7,"testchar"),"testchar8                               ");
	assertEqualString(sqlrcur_getFieldByName(cur,7,"testvarchar"),"testvarchar8");
	assertEqualString(sqlrcur_getFieldByName(cur,7,"testbit"),"1");
	printf("\n");

	printf("FIELD LENGTHS BY NAME: \n");
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,0,"testint"),1);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,0,"testsmallint"),1);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,0,"testtinyint"),1);
	/*assertEqualInt(sqlrcur_getFieldLengthByName(cur,0,"testreal"),3);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,0,"testfloat"),3);*/
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,0,"testdecimal"),3);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,0,"testnumeric"),3);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,0,"testmoney"),4);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,0,"testsmallmoney"),4);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,0,"testdatetime"),26);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,0,"testsmalldatetime"),26);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,0,"testchar"),40);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,0,"testvarchar"),12);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,0,"testbit"),1);
	printf("\n");
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,7,"testint"),1);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,7,"testsmallint"),1);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,7,"testtinyint"),1);
	/*assertEqualInt(sqlrcur_getFieldLengthByName(cur,7,"testreal"),3);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,7,"testfloat"),17);*/
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,7,"testdecimal"),3);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,7,"testnumeric"),3);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,7,"testmoney"),4);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,7,"testsmallmoney"),4);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,7,"testdatetime"),26);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,7,"testsmalldatetime"),26);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,7,"testchar"),40);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,7,"testvarchar"),12);
	assertEqualInt(sqlrcur_getFieldLengthByName(cur,7,"testbit"),1);
	printf("\n");

	printf("FIELDS BY ARRAY: \n");
	fields=sqlrcur_getRow(cur,0);
	assertEqualString(fields[0],"1");
	assertEqualString(fields[1],"1");
	assertEqualString(fields[2],"1");
	/*assertEqualString(fields[3],"1.1");
	assertEqualString(fields[4],"1.1");*/
	assertEqualString(fields[5],"1.1");
	assertEqualString(fields[6],"1.1");
	assertEqualString(fields[7],"1.00");
	assertEqualString(fields[8],"1.00");
	assertEqualString(fields[9],"Jan  1 2001 01:00:00:000AM");
	assertEqualString(fields[10],"Jan  1 2001 01:00:00:000AM");
	assertEqualString(fields[11],"testchar1                               ");
	assertEqualString(fields[12],"testvarchar1");
	assertEqualString(fields[13],"1");
	printf("\n");

	printf("FIELD LENGTHS BY ARRAY: \n");
	fieldlens=sqlrcur_getRowLengths(cur,0);
	assertEqualInt(fieldlens[0],1);
	assertEqualInt(fieldlens[1],1);
	assertEqualInt(fieldlens[2],1);
	/*assertEqualInt(fieldlens[3],3);
	assertEqualInt(fieldlens[4],3);*/
	assertEqualInt(fieldlens[5],3);
	assertEqualInt(fieldlens[6],3);
	assertEqualInt(fieldlens[7],4);
	assertEqualInt(fieldlens[8],4);
	assertEqualInt(fieldlens[9],26);
	assertEqualInt(fieldlens[10],26);
	assertEqualInt(fieldlens[11],40);
	assertEqualInt(fieldlens[12],12);
	assertEqualInt(fieldlens[13],1);
	printf("\n");
#endif

#if 0
	printf("NULLS as Nulls: \n");
	sqlrcur_getNullsAsNulls(cur);
	assertEqualInt(sqlrcur_sendQuery(cur,"select NULL,1,NULL"),1);
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,0),NULL);
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,2),NULL);
	sqlrcur_getNullsAsEmptyStrings(cur);
	assertEqualInt(sqlrcur_sendQuery(cur,"select NULL,1,NULL"),1);
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,0),"");
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,1),"1");
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,2),"");
	sqlrcur_getNullsAsNulls(cur);
	printf("\n");

	printf("RESULT SET BUFFER SIZE: \n");
	assertEqualInt(sqlrcur_getResultSetBufferSize(cur),0);
	sqlrcur_setResultSetBufferSize(cur,2);
	assertEqualInt(sqlrcur_sendQuery(cur,"select * from testtable order by testint"),1);
	assertEqualInt(sqlrcur_getResultSetBufferSize(cur),2);
	printf("\n");
	assertEqualInt(sqlrcur_firstRowIndex(cur),0);
	assertEqualInt(sqlrcur_endOfResultSet(cur),0);
	assertEqualInt(sqlrcur_rowCount(cur),2);
	assertEqualString(sqlrcur_getFieldByIndex(cur,0,0),"1");
	assertEqualString(sqlrcur_getFieldByIndex(cur,1,0),"2");
	assertEqualString(sqlrcur_getFieldByIndex(cur,2,0),"3");
	printf("\n");
	assertEqualInt(sqlrcur_firstRowIndex(cur),2);
	assertEqualInt(sqlrcur_endOfResultSet(cur),0);
	assertEqualInt(sqlrcur_rowCount(cur),4);
	assertEqualString(sqlrcur_getFieldByIndex(cur,6,0),"7");
	assertEqualString(sqlrcur_getFieldByIndex(cur,7,0),"8");
	printf("\n");
	assertEqualInt(sqlrcur_firstRowIndex(cur),6);
	assertEqualInt(sqlrcur_endOfResultSet(cur),0);
	assertEqualInt(sqlrcur_rowCount(cur),8);
	assertEqualString(sqlrcur_getFieldByIndex(cur,8,0),NULL);
	printf("\n");
	assertEqualInt(sqlrcur_firstRowIndex(cur),8);
	assertEqualInt(sqlrcur_endOfResultSet(cur),1);
	assertEqualInt(sqlrcur_rowCount(cur),8);
	printf("\n");

	printf("DONT GET COLUMN INFO: \n");
	sqlrcur_dontGetColumnInfo(cur);
	assertEqualInt(sqlrcur_sendQuery(cur,"select * from testtable order by testint"),1);
	assertEqualString(sqlrcur_getColumnName(cur,0),NULL);
	assertEqualInt(sqlrcur_getColumnLengthByIndex(cur,0),0);
	assertEqualString(sqlrcur_getColumnTypeByIndex(cur,0),NULL);
	sqlrcur_getColumnInfo(cur);
	assertEqualInt(sqlrcur_sendQuery(cur,"select * from testtable order by testint"),1);
	assertEqualString(sqlrcur_getColumnName(cur,0),"testint");
	assertEqualInt(sqlrcur_getColumnLengthByIndex(cur,0),4);
	assertEqualString(sqlrcur_getColumnTypeByIndex(cur,0),"INT");
	printf("\n");

	// drop existing table
	sqlrcur_sendQuery(cur,"commit tran");
	sqlrcur_sendQuery(cur,"drop table testtable");

	// invalid queries...
	printf("INVALID QUERIES: \n");
	assertEqualInt(sqlrcur_sendQuery(cur,"select * from testtable order by testint"),0);
	assertEqualInt(sqlrcur_sendQuery(cur,"select * from testtable order by testint"),0);
	assertEqualInt(sqlrcur_sendQuery(cur,"select * from testtable order by testint"),0);
	assertEqualInt(sqlrcur_sendQuery(cur,"select * from testtable order by testint"),0);
	printf("\n");
	assertEqualInt(sqlrcur_sendQuery(cur,"insert into testtable values (1,2,3,4)"),0);
	assertEqualInt(sqlrcur_sendQuery(cur,"insert into testtable values (1,2,3,4)"),0);
	assertEqualInt(sqlrcur_sendQuery(cur,"insert into testtable values (1,2,3,4)"),0);
	assertEqualInt(sqlrcur_sendQuery(cur,"insert into testtable values (1,2,3,4)"),0);
	printf("\n");
	assertEqualInt(sqlrcur_sendQuery(cur,"create table testtable"),0);
	assertEqualInt(sqlrcur_sendQuery(cur,"create table testtable"),0);
	assertEqualInt(sqlrcur_sendQuery(cur,"create table testtable"),0);
	assertEqualInt(sqlrcur_sendQuery(cur,"create table testtable"),0);
	printf("\n");
#endif

	reportTestStatus();

	return status;
}
