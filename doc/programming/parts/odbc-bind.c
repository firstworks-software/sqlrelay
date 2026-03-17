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

        SQLCHAR         stringval[]="true";
        SQLINTEGER      integerval=10;
        SQLDOUBLE       floatval=1.1;
        SQLLEN          stringvallen=SQL_NTS;
        SQLLEN          integervallen=0;
        SQLLEN          floatvallen=0;

        SQLPrepare(stmt,
                (SQLCHAR *)"select * from mytable "
                        "where stringcol=? "
                        "and integercol>? "
                        "and floatcol>?",
                SQL_NTS);
        SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
                        SQL_C_CHAR,SQL_VARCHAR,
                        4,0,stringval,sizeof(stringval),
                        &stringvallen);
        SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
                        SQL_C_SLONG,SQL_INTEGER,
                        0,0,&integerval,0,
                        &integervallen);
        SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
                        SQL_C_DOUBLE,SQL_DOUBLE,
                        0,0,&floatval,0,
                        &floatvallen);
        SQLExecute(stmt);

        ... process the result set ...

        SQLFreeHandle(SQL_HANDLE_STMT,stmt);
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC,dbc);
        SQLFreeHandle(SQL_HANDLE_ENV,env);
}
