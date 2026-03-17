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

        SQLINTEGER      value;
        SQLLEN          valuelen=0;

        SQLPrepare(stmt,
                (SQLCHAR *)"select * from mytable where mycolumn>?",
                SQL_NTS);
        SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
                        SQL_C_SLONG,SQL_INTEGER,
                        0,0,&value,0,&valuelen);

        value=1;
        SQLExecute(stmt);

        ... process the result set ...

        SQLFreeStmt(stmt,SQL_CLOSE);
        value=5;
        SQLExecute(stmt);

        ... process the result set ...

        SQLFreeStmt(stmt,SQL_CLOSE);
        value=10;
        SQLExecute(stmt);

        ... process the result set ...

        SQLFreeHandle(SQL_HANDLE_STMT,stmt);
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC,dbc);
        SQLFreeHandle(SQL_HANDLE_ENV,env);
}
