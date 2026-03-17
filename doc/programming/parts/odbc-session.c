#include <sql.h>
#include <sqlext.h>

main() {

        SQLHENV env;
        SQLHDBC dbc;

        SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&env);
        SQLSetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
                                (SQLPOINTER)SQL_OV_ODBC3,0);
        SQLAllocHandle(SQL_HANDLE_DBC,env,&dbc);

        SQLConnect(dbc,(SQLCHAR *)"sqlrexample",SQL_NTS,
                        (SQLCHAR *)"user",SQL_NTS,
                        (SQLCHAR *)"password",SQL_NTS);

        ... execute some queries ...

        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC,dbc);
        SQLFreeHandle(SQL_HANDLE_ENV,env);
}
