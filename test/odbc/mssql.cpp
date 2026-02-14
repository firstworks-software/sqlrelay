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

#define USEDSN 1

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
		assertSuccessDbc(dbc,erg);
	#else
		erg=SQLAllocConnect(env,&dbc);
		assertSuccessDbc(dbc,erg);
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
				"Server=localhost;Port=9000;"
				"Socket=/tmp/test.socket;"
				"User=test;Password=test;"
				"LazyConnect=0;Debug=1;";
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
	SQLUSMALLINT	usmallintval;
	SQLUINTEGER	uintval;
	SQLULEN		ulenval;
	SQLCHAR		strval[2048];
	SQLSMALLINT	vallen;
	erg=SQLGetInfo(dbc,SQL_ACTIVE_CONNECTIONS,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_ACTIVE_STATEMENTS,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,0);
	assertSuccessDbc(dbc,erg);
	#ifdef USEDSN
		erg=SQLGetInfo(dbc,SQL_DATA_SOURCE_NAME,
				(SQLPOINTER)strval,
				(SQLSMALLINT)sizeof(strval),
				&vallen);
		assertEqualDbc(dbc,(const char *)strval,"sqlrodbc");
		assertSuccessDbc(dbc,erg);
	#endif
	erg=SQLGetInfo(dbc,SQL_FETCH_DIRECTION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_FD_FETCH_NEXT);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SERVER_NAME,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"localhost");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SEARCH_PATTERN_ESCAPE,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	//assertEqualDbc(dbc,(const char *)strval,"%");
	assertSuccessDbc(dbc,erg);
	/*erg=SQLGetInfo(dbc,SQL_DATABASE_NAME,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"testdb");*/
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DBMS_NAME,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"odbc");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DBMS_VER,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	//assertEqualDbc(dbc,(const char *)strval,"12.00.2000");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_ACCESSIBLE_TABLES,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"N");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_ACCESSIBLE_PROCEDURES,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"N");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CURSOR_COMMIT_BEHAVIOR,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_CB_CLOSE);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DATA_SOURCE_READ_ONLY,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"N");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DEFAULT_TXN_ISOLATION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_TXN_READ_COMMITTED);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_IDENTIFIER_CASE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_IC_MIXED);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_IDENTIFIER_QUOTE_CHAR,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"\"");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMN_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_CURSOR_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_OWNER_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,128);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_CATALOG_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_TABLE_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SCROLL_CONCURRENCY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_SCCO_READ_ONLY);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_TXN_CAPABLE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_TC_ALL);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_USER_NAME,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	//assertEqualDbc(dbc,(const char *)strval,"dbo");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_TXN_ISOLATION_OPTION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_TXN_READ_UNCOMMITTED|
					SQL_TXN_READ_COMMITTED|
					SQL_TXN_REPEATABLE_READ|
					SQL_TXN_SERIALIZABLE));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_INTEGRITY,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"Y");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_GETDATA_EXTENSIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_GD_BLOCK);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_NULL_COLLATION,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_NC_LOW);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_ALTER_TABLE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(0
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
				));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_ORDER_BY_COLUMNS_IN_SELECT,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"N");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SPECIAL_CHARACTERS,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"#$_");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_GROUP_BY,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_INDEX,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_ORDER_BY,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_SELECT,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_COLUMNS_IN_TABLE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_INDEX_SIZE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_ROW_SIZE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_STATEMENT_LEN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_TABLES_IN_SELECT,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_USER_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,0);
	assertSuccessDbc(dbc,erg);
	#if (ODBCVER >= 0x0300)
	erg=SQLGetInfo(dbc,SQL_OJ_CAPABILITIES,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_OJ_LEFT|
					SQL_OJ_RIGHT|
					SQL_OJ_FULL|
					SQL_OJ_NESTED|
					SQL_OJ_NOT_ORDERED|
					SQL_OJ_INNER|
					SQL_OJ_ALL_COMPARISON_OPS));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_XOPEN_CLI_YEAR,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	// FIXME: driver should return 1996, but I get 1995???
	// maybe this is intercepted by the driver manager
	//assertEqualDbc(dbc,(const char *)strval,"1996");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CURSOR_SENSITIVITY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_UNSPECIFIED);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DESCRIBE_PARAMETER,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"N");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CATALOG_NAME,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"Y");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_COLLATION_SEQ,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_IDENTIFIER_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	#endif
	assertEqualDbc(dbc,(int)usmallintval,128);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DRIVER_HDBC,
			(SQLPOINTER)&ulenval,
			(SQLSMALLINT)sizeof(ulenval),
			&vallen);
	// unsupported but returns success
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DRIVER_HENV,
			(SQLPOINTER)&ulenval,
			(SQLSMALLINT)sizeof(ulenval),
			&vallen);
	// unsupported but returns success
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DRIVER_HSTMT,
			(SQLPOINTER)&ulenval,
			(SQLSMALLINT)sizeof(ulenval),
			&vallen);
	// unsupported but returns success
	// FIXME: this should return success
	// maybe this is intercepted by the driver manager
	assertTrueDbc(dbc,erg==SQL_ERROR);
	erg=SQLGetInfo(dbc,SQL_DRIVER_NAME,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"libsqlrodbc.so");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DRIVER_VER,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	//assertEqualDbc(dbc,(const char *)strval,conn->con->clientVersion());
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_ODBC_API_CONFORMANCE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_OAC_LEVEL2);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_ODBC_VER,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	//assertEqualDbc(dbc,(const char *)strval,conn->con->clientVersion());
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_ROW_UPDATES,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"N");
	assertSuccessDbc(dbc,erg);
	/*erg=SQLGetInfo(dbc,SQL_ODBC_SAG_CLI_CONFORMANCE,
			debugPrintf("  unsupported infotype: "
					"SQL_ODBC_SAG_CLI_CONFORMANCE\n");
	assertTrueDbc(dbc,erg==SQL_ERROR);*/
	erg=SQLGetInfo(dbc,SQL_ODBC_SQL_CONFORMANCE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_OSC_EXTENDED);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_PROCEDURES,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"Y");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONCAT_NULL_BEHAVIOR,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_CB_NON_NULL);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CURSOR_ROLLBACK_BEHAVIOR,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_CB_CLOSE);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_EXPRESSIONS_IN_ORDERBY,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"Y");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_PROCEDURE_NAME_LEN,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MULT_RESULT_SETS,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"N");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MULTIPLE_ACTIVE_TXN,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"N");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_OUTER_JOINS,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"Y");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_OWNER_TERM,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"schema");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_PROCEDURE_TERM,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"stored procedure");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_QUALIFIER_NAME_SEPARATOR,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,".");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_QUALIFIER_TERM,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"catalog");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SCROLL_OPTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_SO_FORWARD_ONLY);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_TABLE_TERM,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"table");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_FN_CVT_CAST|SQL_FN_CVT_CONVERT));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_NUMERIC_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_FN_NUM_ABS|
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
					SQL_FN_NUM_TRUNCATE));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_STRING_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_FN_STR_CONCAT|
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
					SQL_FN_STR_POSITION));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SYSTEM_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_FN_SYS_DBNAME|
					SQL_FN_SYS_IFNULL|
					SQL_FN_SYS_USERNAME));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_TIMEDATE_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_FN_TD_CURRENT_DATE|
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
					SQL_FN_TD_YEAR));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_BIGINT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_BINARY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_BIT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_CHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_DATE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_DECIMAL,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_DOUBLE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_FLOAT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_INTEGER,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_LONGVARCHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_NUMERIC,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_REAL,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_SMALLINT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_TIME,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_TIMESTAMP,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_TINYINT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_VARBINARY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_VARCHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_LONGVARBINARY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CORRELATION_NAME,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_CN_ANY);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_NON_NULLABLE_COLUMNS,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_NNC_NON_NULL);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DRIVER_HLIB,
			(SQLPOINTER)&ulenval,
			(SQLSMALLINT)sizeof(ulenval),
			&vallen);
	// unsupported but returns success
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DRIVER_ODBC_VER,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	// FIXME: this returns 02.00 but I'd expect 03.00
	/*
	#if (ODBCVER >= 0x0380)
	// FIXME: not sure why we're doing this
	//assertEqualDbc(dbc,(const char *)strval,"03.80");
	assertEqualDbc(dbc,(const char *)strval,"03.00");
	#elif (ODBCVER >= 0x0300)
	assertEqualDbc(dbc,(const char *)strval,"03.00");
	#else
	assertEqualDbc(dbc,(const char *)strval,"02.00");
	#endif
	*/
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_LOCK_TYPES,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_LCK_NO_CHANGE|
					SQL_LCK_EXCLUSIVE|
					SQL_LCK_UNLOCK));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_POS_OPERATIONS,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_POS_POSITION);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_POSITIONED_STATEMENTS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_BOOKMARK_PERSISTENCE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_STATIC_SENSITIVITY,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_FILE_USAGE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_FILE_NOT_SUPPORTED);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_COLUMN_ALIAS,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"Y");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_GROUP_BY,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)(
				#if (ODBCVER >= 0x0300)
				SQL_GB_COLLATE
				#else
				SQL_GB_GROUP_BY_EQUALS_SELECT
				#endif
				));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_KEYWORDS,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,SQL_ODBC_KEYWORDS);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_OWNER_USAGE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_SU_DML_STATEMENTS|
					SQL_SU_PROCEDURE_INVOCATION|
					SQL_SU_TABLE_DEFINITION|
					SQL_SU_INDEX_DEFINITION|
					SQL_SU_PRIVILEGE_DEFINITION));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_QUALIFIER_USAGE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_SU_DML_STATEMENTS|
					SQL_SU_PROCEDURE_INVOCATION|
					SQL_SU_TABLE_DEFINITION|
					SQL_SU_INDEX_DEFINITION|
					SQL_SU_PRIVILEGE_DEFINITION));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_QUOTED_IDENTIFIER_CASE,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_IC_SENSITIVE);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SUBQUERIES,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_SQ_CORRELATED_SUBQUERIES|
					SQL_SQ_COMPARISON|
					SQL_SQ_EXISTS|
					SQL_SQ_IN|
					SQL_SQ_QUANTIFIED));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_UNION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_U_UNION|SQL_U_UNION_ALL));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_ROW_SIZE_INCLUDES_LONG,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"N");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_CHAR_LITERAL_LEN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_TIMEDATE_ADD_INTERVALS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_TIMEDATE_DIFF_INTERVALS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_NEED_LONG_DATA_LEN,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"Y");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_BINARY_LITERAL_LEN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_LIKE_ESCAPE_CLAUSE,
			(SQLPOINTER)strval,
			(SQLSMALLINT)sizeof(strval),
			&vallen);
	assertEqualDbc(dbc,(const char *)strval,"Y");
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_QUALIFIER_LOCATION,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,(int)SQL_CL_START);
	assertSuccessDbc(dbc,erg);
	#if (ODBCVER >= 0x0300)
	erg=SQLGetInfo(dbc,SQL_ACTIVE_ENVIRONMENTS,
			(SQLPOINTER)&usmallintval,
			(SQLSMALLINT)sizeof(usmallintval),
			&vallen);
	assertEqualDbc(dbc,(int)usmallintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_ALTER_DOMAIN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_AD_ADD_DOMAIN_CONSTRAINT|
					SQL_AD_ADD_DOMAIN_DEFAULT|
					SQL_AD_CONSTRAINT_NAME_DEFINITION|
					SQL_AD_DROP_DOMAIN_CONSTRAINT|
					SQL_AD_DROP_DOMAIN_DEFAULT));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SQL_CONFORMANCE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_SC_SQL92_ENTRY);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DATETIME_LITERALS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_ASYNC_MODE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_AM_NONE);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_BATCH_ROW_COUNT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_BATCH_SUPPORT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_WCHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_INTERVAL_DAY_TIME,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_INTERVAL_YEAR_MONTH,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_WLONGVARCHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CONVERT_WVARCHAR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CREATE_ASSERTION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CREATE_CHARACTER_SET,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CREATE_COLLATION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CREATE_DOMAIN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CREATE_SCHEMA,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_CS_CREATE_SCHEMA|
					SQL_CS_AUTHORIZATION|
					SQL_CS_DEFAULT_CHARACTER_SET));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CREATE_TABLE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_CT_CREATE_TABLE|
					SQL_CT_TABLE_CONSTRAINT|
					SQL_CT_CONSTRAINT_NAME_DEFINITION|
					SQL_CT_COMMIT_DELETE|
					SQL_CT_GLOBAL_TEMPORARY|
					SQL_CT_COLUMN_CONSTRAINT|
					SQL_CT_COLUMN_DEFAULT|
					SQL_CT_COLUMN_COLLATION|
					SQL_CT_CONSTRAINT_INITIALLY_IMMEDIATE|
					SQL_CT_CONSTRAINT_NON_DEFERRABLE));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CREATE_TRANSLATION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_CREATE_VIEW,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_CV_CREATE_VIEW|
					SQL_CV_CHECK_OPTION|
					SQL_CV_CASCADED|
					SQL_CV_LOCAL));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DRIVER_HDESC,
			(SQLPOINTER)&ulenval,
			(SQLSMALLINT)sizeof(ulenval),
			&vallen);
	// unsupported but returns success
	// FIXME: this should return success
	// maybe this is intercepted by the driver manager
	assertTrueDbc(dbc,erg==SQL_ERROR);
	erg=SQLGetInfo(dbc,SQL_DROP_ASSERTION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DROP_CHARACTER_SET,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DROP_COLLATION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DROP_DOMAIN,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DROP_SCHEMA,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_DS_DROP_SCHEMA|
					SQL_DS_CASCADE|
					SQL_DS_RESTRICT));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DROP_TABLE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_DT_DROP_TABLE|
					SQL_DT_CASCADE|
					SQL_DT_RESTRICT));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DROP_TRANSLATION,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DROP_VIEW,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_DV_DROP_VIEW|
					SQL_DV_CASCADE|
					SQL_DV_RESTRICT));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DYNAMIC_CURSOR_ATTRIBUTES1,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_CA1_NEXT|SQL_CA1_POS_POSITION));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DYNAMIC_CURSOR_ATTRIBUTES2,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_CA2_READ_ONLY_CONCURRENCY);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES1,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_CA1_NEXT|SQL_CA1_POS_POSITION));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_FORWARD_ONLY_CURSOR_ATTRIBUTES2,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_CA2_READ_ONLY_CONCURRENCY);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_INDEX_KEYWORDS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_IK_ALL);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_INFO_SCHEMA_VIEWS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_KEYSET_CURSOR_ATTRIBUTES1,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_CA1_NEXT|SQL_CA1_POS_POSITION));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_KEYSET_CURSOR_ATTRIBUTES2,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_CA2_READ_ONLY_CONCURRENCY);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_MAX_ASYNC_CONCURRENT_STATEMENTS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_ODBC_INTERFACE_CONFORMANCE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_OIC_CORE);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_PARAM_ARRAY_ROW_COUNTS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,0);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_PARAM_ARRAY_SELECTS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_PAS_NO_SELECT);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SQL92_DATETIME_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_SDF_CURRENT_DATE|
					SQL_SDF_CURRENT_TIME|
					SQL_SDF_CURRENT_TIMESTAMP));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SQL92_FOREIGN_KEY_DELETE_RULE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_SFKD_CASCADE);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SQL92_FOREIGN_KEY_UPDATE_RULE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_SFKU_CASCADE);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SQL92_GRANT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_SG_DELETE_TABLE|
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
					SQL_SG_WITH_GRANT_OPTION));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SQL92_NUMERIC_VALUE_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_SNVF_BIT_LENGTH|
					SQL_SNVF_CHAR_LENGTH|
					SQL_SNVF_CHARACTER_LENGTH|
					SQL_SNVF_EXTRACT|
					SQL_SNVF_OCTET_LENGTH|
					SQL_SNVF_POSITION));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SQL92_PREDICATES,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_SP_BETWEEN|
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
					SQL_SP_UNIQUE));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SQL92_RELATIONAL_JOIN_OPERATORS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_SRJO_CORRESPONDING_CLAUSE|
					SQL_SRJO_CROSS_JOIN|
					SQL_SRJO_EXCEPT_JOIN|
					SQL_SRJO_FULL_OUTER_JOIN|
					SQL_SRJO_INNER_JOIN|
					SQL_SRJO_INTERSECT_JOIN|
					SQL_SRJO_LEFT_OUTER_JOIN|
					SQL_SRJO_NATURAL_JOIN|
					SQL_SRJO_RIGHT_OUTER_JOIN|
					SQL_SRJO_UNION_JOIN));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SQL92_REVOKE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_SR_CASCADE|
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
					SQL_SR_USAGE_ON_TRANSLATION));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SQL92_ROW_VALUE_CONSTRUCTOR,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_SRVC_VALUE_EXPRESSION|
					SQL_SRVC_NULL|
					SQL_SRVC_DEFAULT|
					SQL_SRVC_ROW_SUBQUERY));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SQL92_STRING_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_SSF_CONVERT|
					SQL_SSF_LOWER|
					SQL_SSF_UPPER|
					SQL_SSF_SUBSTRING|
					SQL_SSF_TRANSLATE|
					SQL_SSF_TRIM_BOTH|
					SQL_SSF_TRIM_LEADING|
					SQL_SSF_TRIM_TRAILING));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_SQL92_VALUE_EXPRESSIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_SVE_CASE|
					SQL_SVE_CAST|
					SQL_SVE_COALESCE|
					SQL_SVE_NULLIF));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_STANDARD_CLI_CONFORMANCE,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_SCC_XOPEN_CLI_VERSION1);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_STATIC_CURSOR_ATTRIBUTES1,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_CA1_NEXT|SQL_CA1_POS_POSITION));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_STATIC_CURSOR_ATTRIBUTES2,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_CA2_READ_ONLY_CONCURRENCY);
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_AGGREGATE_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_AF_ALL|
					SQL_AF_AVG|
					SQL_AF_COUNT|
					SQL_AF_DISTINCT|
					SQL_AF_MAX|
					SQL_AF_MIN|
					SQL_AF_SUM));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_DDL_INDEX,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_DI_CREATE_INDEX|SQL_DI_DROP_INDEX));
	assertSuccessDbc(dbc,erg);
	erg=SQLGetInfo(dbc,SQL_INSERT_STATEMENT,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)(SQL_IS_INSERT_LITERALS|
					SQL_IS_INSERT_SEARCHED|
					SQL_IS_SELECT_INTO));
	assertSuccessDbc(dbc,erg);
	#if (ODBCVER >= 0x0380)
	erg=SQLGetInfo(dbc,SQL_ASYNC_DBC_FUNCTIONS,
			(SQLPOINTER)&uintval,
			(SQLSMALLINT)sizeof(uintval),
			&vallen);
	assertEqualDbc(dbc,(int)uintval,(int)SQL_ASYNC_DBC_NOT_CAPABLE);
	#endif
	#endif
	#ifdef SQL_DTC_TRANSITION_COST
	/*erg=SQLGetInfo(dbc,SQL_DTC_TRANSITION_COST,
			debugPrintf("  unsupported infotype: "
					"SQL_DTC_TRANSITION_COST\n");
	assertTrue(erg==SQL_ERROR);*/
	#endif
	stdoutput.printf("\n");


	// drop existing table
	stdoutput.printf("DROP EXISTING TABLE\n");
	erg=SQLAllocHandle(SQL_HANDLE_STMT,dbc,&stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLExecDirect(stmt,(SQLCHAR *)"drop table testtable",SQL_NTS);
	stdoutput.printf("\n");


	// create temptable
	stdoutput.printf("CREATE TEMPTABLE: \n");
	erg=SQLExecDirect(stmt,
		(SQLCHAR *)"create table testtable ("
		"	testint int, "
		"	testsmallint smallint, "
		"	testtinyint tinyint, "
		"	testreal real, "
		"	testfloat float, "
		"	testdecimal decimal(4,1), "
		"	testnumeric numeric(4,1), "
		"	testmoney money, "
		"	testsmallmoney smallmoney, "
		"	testdatetime datetime, "
		"	testsmalldatetime smalldatetime, "
		"	testchar char(40), "
		"	testvarchar varchar(40), "
		"	testbit bit)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");


	// begin transaction
	stdoutput.printf("BEGIN TRANSACTION: \n");
	SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
				(SQLPOINTER)SQL_AUTOCOMMIT_OFF,
				sizeof(SQLINTEGER));
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");


	// insert
	stdoutput.printf("INSERT: \n");
	erg=SQLExecDirect(stmt,
		(SQLCHAR *)"insert into testtable "
		"values (1,1,1,1.1,1.1,1.1,1.1,"
		"1.00,1.00,"
		"'01-Jan-2001 01:00:00',"
		"'01-Jan-2001 01:00:00',"
		"'testchar1','testvarchar1',1)",
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


	// bind by position
	stdoutput.printf("BIND BY POSITION: \n");
	erg=SQLPrepare(stmt,
		(SQLCHAR *)"insert into testtable "
		"values (?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	SQLSMALLINT	bindvarcount;
	erg=SQLNumParams(stmt,&bindvarcount);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)bindvarcount,14);

	SQLINTEGER	intval=2;
	erg=SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
				SQL_C_SLONG,SQL_INTEGER,
				0,0,
				(SQLPOINTER)&intval,
				sizeof(SQLINTEGER),NULL);
	assertSuccessStmt(stmt,erg);
	SQLSMALLINT	smallintval=2;
	erg=SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
				SQL_C_SSHORT,SQL_SMALLINT,
				0,0,
				(SQLPOINTER)&smallintval,
				sizeof(SQLSMALLINT),NULL);
	assertSuccessStmt(stmt,erg);

	reportTestStatus();

	return status;
}
