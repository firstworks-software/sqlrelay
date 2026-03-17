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

        // turn off autocommit
        SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
                                (SQLPOINTER)SQL_AUTOCOMMIT_OFF,0);

        SQLAllocHandle(SQL_HANDLE_STMT,dbc,&stmt);

        SQLExecDirect(stmt,
                (SQLCHAR *)"insert into my_table values (1,2,3)",
                SQL_NTS);

        SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_COMMIT);

        SQLFreeStmt(stmt,SQL_CLOSE);
        SQLExecDirect(stmt,
                (SQLCHAR *)"insert into my_table values (4,5,6)",
                SQL_NTS);

        SQLEndTran(SQL_HANDLE_DBC,dbc,SQL_ROLLBACK);

        // turn autocommit back on
        SQLSetConnectAttr(dbc,SQL_ATTR_AUTOCOMMIT,
                                (SQLPOINTER)SQL_AUTOCOMMIT_ON,0);

        SQLFreeHandle(SQL_HANDLE_STMT,stmt);
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC,dbc);
        SQLFreeHandle(SQL_HANDLE_ENV,env);
}
