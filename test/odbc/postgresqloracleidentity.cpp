// Copyright (c) David Muse
// See the file COPYING for more information.

#include <rudiments/charstring.h>

#include "../../config.h"

#ifdef _WIN32
	#include <windows.h>
#endif
#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>
#include <sqltypes.h>

#include "asserts.cpp"

SQLRETURN	erg;
SQLHENV		env;
SQLHDBC		dbc;
SQLHSTMT	stmt;

// exercises #10179: an instance whose connection string overrides dbtype to
// "Oracle" while the real backend is postgresql must report integer/numeric
// result columns using oracle's NUMBER semantics, not postgresql's native
// types
int main(int, char **) {

	// environment handle
	stdoutput.printf("ENVIRONMENT HANDLE: \n");
	#if (ODBCVER >= 0x0300)
		erg=SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&env);
		assertSuccessEnv(env,erg);
		erg=SQLSetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
					(SQLPOINTER)SQL_OV_ODBC3,0);
		assertSuccessEnv(env,erg);
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
	SQLCHAR		incstring[]=
		"Driver={SQL Relay};"
		"Server=sqlrelay;Port=9044;"
		"Socket=/tmp/postgresqloracleidentity.socket;"
		"User=testuser;Password=testpassword;"
		"NullsAsNulls=yes;"
		"AutoCommit=yes;";
	SQLCHAR		outcstring[1024];
	SQLSMALLINT	outcstringlen;
	erg=SQLDriverConnect(dbc,NULL,
			incstring,SQL_NTS,
			outcstring,sizeof(outcstring),&outcstringlen,
			SQL_DRIVER_NOPROMPT);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");



	// statement handle
	stdoutput.printf("STATEMENT HANDLE: \n");
	erg=SQLAllocHandle(SQL_HANDLE_STMT,dbc,&stmt);
	assertSuccessDbc(dbc,erg);
	stdoutput.printf("\n");



	// create testtable
	stdoutput.printf("CREATE TESTTABLE: \n");
	SQLExecDirect(stmt,(SQLCHAR *)"drop table if exists testtable",SQL_NTS);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"create table testtable ("
		"	testint int, "
		"	testnum numeric(10,2))",
		SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// insert
	stdoutput.printf("INSERT: \n");
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"insert into testtable values (1,1.50)",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	stdoutput.printf("\n");



	// column info variables, reused for each select below
	SQLCHAR		colname[256];
	SQLSMALLINT	colnamelen;
	SQLSMALLINT	datatype;
	SQLULEN		colsize;
	SQLSMALLINT	decdigits;
	SQLSMALLINT	nullable;
	SQLCHAR		typenamebuf[64];
	SQLSMALLINT	typenamelen;
	SQLCHAR		databuf[64];
	SQLLEN		dataind;



	// select count(*): the ticket's actual regression - a postgresql
	// count(*) is native SQL_BIGINT, and impersonating oracle must remap
	// it to NUMBER(38) rather than leave it a binary bigint
	stdoutput.printf("SELECT COUNT(*): \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select count(*) as n from testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLDescribeCol(stmt,1,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)datatype,SQL_NUMERIC);
	assertEqualStmt(stmt,(int)colsize,38);
	assertEqualStmt(stmt,(int)decdigits,0);
	erg=SQLColAttribute(stmt,1,SQL_COLUMN_TYPE_NAME,
				typenamebuf,sizeof(typenamebuf),&typenamelen,
				NULL);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)typenamebuf,"NUMBER");
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	// SQL_C_DEFAULT must resolve to text now that the column is
	// reported as SQL_NUMERIC, not the 8-byte binary int it got as
	// SQL_BIGINT
	erg=SQLGetData(stmt,1,SQL_C_DEFAULT,
				databuf,sizeof(databuf),&dataind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)dataind,1);
	assertEqualStmt(stmt,(const char *)databuf,"1");
	stdoutput.printf("\n");



	// select an int4 literal
	stdoutput.printf("SELECT INT4 LITERAL: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select 42 as x from testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLDescribeCol(stmt,1,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)datatype,SQL_NUMERIC);
	assertEqualStmt(stmt,(int)colsize,38);
	assertEqualStmt(stmt,(int)decdigits,0);
	erg=SQLColAttribute(stmt,1,SQL_COLUMN_TYPE_NAME,
				typenamebuf,sizeof(typenamebuf),&typenamelen,
				NULL);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)typenamebuf,"NUMBER");
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetData(stmt,1,SQL_C_DEFAULT,
				databuf,sizeof(databuf),&dataind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)dataind,2);
	assertEqualStmt(stmt,(const char *)databuf,"42");
	stdoutput.printf("\n");



	// select an undeclared, unscaled numeric literal - documents the
	// #10165 side effect: colsize reports NUMBER(38) rather than the
	// unbounded 32768 hsql used to fall back to, and decdigits reports
	// 0 even though the real fractional text still reaches the client
	stdoutput.printf("SELECT UNDECLARED NUMERIC LITERAL: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select 42.5 as x from testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLDescribeCol(stmt,1,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)datatype,SQL_NUMERIC);
	assertEqualStmt(stmt,(int)colsize,38);
	assertEqualStmt(stmt,(int)decdigits,0);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetData(stmt,1,SQL_C_DEFAULT,
				databuf,sizeof(databuf),&dataind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)dataind,4);
	assertEqualStmt(stmt,(const char *)databuf,"42.5");
	stdoutput.printf("\n");



	// select a declared numeric(10,2) column - the "already has a
	// scale" branch: the source precision is kept, not forced to 38
	stdoutput.printf("SELECT DECLARED NUMERIC(10,2) COLUMN: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select testnum from testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLDescribeCol(stmt,1,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)datatype,SQL_NUMERIC);
	assertEqualStmt(stmt,(int)colsize,10);
	assertEqualStmt(stmt,(int)decdigits,2);
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetData(stmt,1,SQL_C_DEFAULT,
				databuf,sizeof(databuf),&dataind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)databuf,"1.50");
	stdoutput.printf("\n");



	// select a declared int column - the "no scale" branch: postgresql
	// reports no precision for a plain int, so it falls back to NUMBER(38)
	// rather than to the column size, which means something else entirely
	stdoutput.printf("SELECT DECLARED INT COLUMN: \n");
	SQLFreeStmt(stmt,SQL_CLOSE);
	erg=SQLExecDirect(stmt,(SQLCHAR *)
		"select testint from testtable",SQL_NTS);
	assertSuccessStmt(stmt,erg);
	erg=SQLDescribeCol(stmt,1,colname,sizeof(colname),&colnamelen,
				&datatype,&colsize,&decdigits,&nullable);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)datatype,SQL_NUMERIC);
	assertEqualStmt(stmt,(int)colsize,38);
	assertEqualStmt(stmt,(int)decdigits,0);
	erg=SQLColAttribute(stmt,1,SQL_COLUMN_TYPE_NAME,
				typenamebuf,sizeof(typenamebuf),&typenamelen,
				NULL);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(const char *)typenamebuf,"NUMBER");
	erg=SQLFetch(stmt);
	assertSuccessStmt(stmt,erg);
	erg=SQLGetData(stmt,1,SQL_C_DEFAULT,
				databuf,sizeof(databuf),&dataind);
	assertSuccessStmt(stmt,erg);
	assertEqualStmt(stmt,(int)dataind,1);
	assertEqualStmt(stmt,(const char *)databuf,"1");
	stdoutput.printf("\n");



	// cleanup and disconnect
	stdoutput.printf("CLEANUP AND DISCONNECT: \n");
	SQLExecDirect(stmt,(SQLCHAR *)"drop table if exists testtable",SQL_NTS);
	#if (ODBCVER >= 0x0300)
		erg=SQLFreeHandle(SQL_HANDLE_STMT,stmt);
		assertSuccessStmt(stmt,erg);
	#else
		erg=SQLFreeStmt(stmt,SQL_DROP);
		assertSuccessStmt(stmt,erg);
	#endif
	erg=SQLDisconnect(dbc);
	assertSuccessDbc(dbc,erg);
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
