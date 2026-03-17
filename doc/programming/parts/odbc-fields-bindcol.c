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

        SQLCHAR col1[256];
        SQLCHAR col2[256];
        SQLCHAR col3[256];
        SQLLEN  col1len;
        SQLLEN  col2len;
        SQLLEN  col3len;

        SQLBindCol(stmt,1,SQL_C_CHAR,col1,sizeof(col1),&col1len);
        SQLBindCol(stmt,2,SQL_C_CHAR,col2,sizeof(col2),&col2len);
        SQLBindCol(stmt,3,SQL_C_CHAR,col3,sizeof(col3),&col3len);

        while (SQLFetch(stmt)==SQL_SUCCESS) {
                printf("%s,%s,%s\n",col1,col2,col3);
        }

        SQLFreeHandle(SQL_HANDLE_STMT,stmt);
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC,dbc);
        SQLFreeHandle(SQL_HANDLE_ENV,env);
}
