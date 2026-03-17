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

        if (SQLExecDirect(stmt,
                (SQLCHAR *)"select * from my_nonexistant_table",
                SQL_NTS)!=SQL_SUCCESS) {

                SQLCHAR         state[6];
                SQLINTEGER      nativeerror;
                SQLCHAR         message[SQL_MAX_MESSAGE_LENGTH];
                SQLSMALLINT     messagelen;

                SQLGetDiagRec(SQL_HANDLE_STMT,stmt,1,
                                state,&nativeerror,
                                message,sizeof(message),
                                &messagelen);

                printf("Error %s: %s\n",state,message);
        }

        SQLFreeHandle(SQL_HANDLE_STMT,stmt);
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC,dbc);
        SQLFreeHandle(SQL_HANDLE_ENV,env);
}
