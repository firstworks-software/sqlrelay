#include <sql.h>
#include <sqlext.h>
#include <stdio.h>

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

        SQLExecDirect(stmt,
                (SQLCHAR *)"select * from my_table",SQL_NTS);

        SQLSMALLINT     cols;
        SQLNumResultCols(stmt,&cols);

        while (SQLFetch(stmt)==SQL_SUCCESS) {
                SQLCHAR field[256];
                SQLLEN  fieldlen;
                for (int col=1; col<=cols; col++) {
                        SQLGetData(stmt,col,SQL_C_CHAR,
                                        field,sizeof(field),
                                        &fieldlen);
                        printf("%s,",field);
                }
                printf("\n");
        }

        SQLFreeHandle(SQL_HANDLE_STMT,stmt);
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC,dbc);
        SQLFreeHandle(SQL_HANDLE_ENV,env);
}
