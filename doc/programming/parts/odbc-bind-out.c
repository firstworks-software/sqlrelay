#include <sql.h>
#include <sqlext.h>

main() {

        SQLHENV env;
        SQLHDBC dbc;
        SQLHSTMT stmt;

        SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&env);
        SQLSetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
                                (SQLPOINTER)SQL_OV_ODBC3,0);
        SQLAllocHandle(SQL_HANDLE_DBC,env,&dbc);
        SQLConnect(dbc,(SQLCHAR *)"sqlrexample",SQL_NTS,
                        (SQLCHAR *)"user",SQL_NTS,
                        (SQLCHAR *)"password",SQL_NTS);
        SQLAllocHandle(SQL_HANDLE_STMT,dbc,&stmt);

        SQLINTEGER      integer1=10;
        SQLINTEGER      integer2=20;
        SQLDOUBLE       float1=1.1;
        SQLDOUBLE       float2=2.2;
        SQLINTEGER      integer3=30;
        SQLINTEGER      result1;
        SQLDOUBLE       result2;
        SQLCHAR         result3[101];
        SQLLEN          integer1len=0;
        SQLLEN          integer2len=0;
        SQLLEN          float1len=0;
        SQLLEN          float2len=0;
        SQLLEN          integer3len=0;
        SQLLEN          result1len=0;
        SQLLEN          result2len=0;
        SQLLEN          result3len=0;

        SQLPrepare(stmt,
                (SQLCHAR *)"{call addAndConvert(?,?,?,?,?,?,?,?)}",
                SQL_NTS);
        SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
                        SQL_C_SLONG,SQL_INTEGER,
                        0,0,&integer1,0,&integer1len);
        SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
                        SQL_C_SLONG,SQL_INTEGER,
                        0,0,&integer2,0,&integer2len);
        SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
                        SQL_C_DOUBLE,SQL_DOUBLE,
                        0,0,&float1,0,&float1len);
        SQLBindParameter(stmt,4,SQL_PARAM_INPUT,
                        SQL_C_DOUBLE,SQL_DOUBLE,
                        0,0,&float2,0,&float2len);
        SQLBindParameter(stmt,5,SQL_PARAM_INPUT,
                        SQL_C_SLONG,SQL_INTEGER,
                        0,0,&integer3,0,&integer3len);
        SQLBindParameter(stmt,6,SQL_PARAM_OUTPUT,
                        SQL_C_SLONG,SQL_INTEGER,
                        0,0,&result1,0,&result1len);
        SQLBindParameter(stmt,7,SQL_PARAM_OUTPUT,
                        SQL_C_DOUBLE,SQL_DOUBLE,
                        0,0,&result2,0,&result2len);
        SQLBindParameter(stmt,8,SQL_PARAM_OUTPUT,
                        SQL_C_CHAR,SQL_VARCHAR,
                        100,0,result3,sizeof(result3),
                        &result3len);
        SQLExecute(stmt);

        ... do something with result1, result2, result3 ...

        SQLFreeHandle(SQL_HANDLE_STMT,stmt);
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC,dbc);
        SQLFreeHandle(SQL_HANDLE_ENV,env);
}
