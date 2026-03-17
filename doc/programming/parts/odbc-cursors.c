#include <sql.h>
#include <sqlext.h>

main() {

        SQLHENV env;
        SQLHDBC dbc;
        SQLHSTMT stmt1;
        SQLHSTMT stmt2;

        SQLAllocHandle(SQL_HANDLE_ENV,SQL_NULL_HANDLE,&env);
        SQLSetEnvAttr(env,SQL_ATTR_ODBC_VERSION,
                                (SQLPOINTER)SQL_OV_ODBC3,0);
        SQLAllocHandle(SQL_HANDLE_DBC,env,&dbc);
        SQLConnect(dbc,(SQLCHAR *)"sqlrexample",SQL_NTS,
                        (SQLCHAR *)"user",SQL_NTS,
                        (SQLCHAR *)"password",SQL_NTS);
        SQLAllocHandle(SQL_HANDLE_STMT,dbc,&stmt1);
        SQLAllocHandle(SQL_HANDLE_STMT,dbc,&stmt2);

        SQLExecDirect(stmt1,
                (SQLCHAR *)"select * from my_huge_table",SQL_NTS);

        SQLCHAR col1[256];
        SQLCHAR col2[256];
        SQLCHAR col3[256];
        SQLLEN  col1len;
        SQLLEN  col2len;
        SQLLEN  col3len;
        SQLLEN  p1len=SQL_NTS;
        SQLLEN  p2len=SQL_NTS;
        SQLLEN  p3len=SQL_NTS;

        SQLBindCol(stmt1,1,SQL_C_CHAR,
                        col1,sizeof(col1),&col1len);
        SQLBindCol(stmt1,2,SQL_C_CHAR,
                        col2,sizeof(col2),&col2len);
        SQLBindCol(stmt1,3,SQL_C_CHAR,
                        col3,sizeof(col3),&col3len);

        SQLPrepare(stmt2,
                (SQLCHAR *)"insert into my_other_table "
                        "values (?,?,?)",
                SQL_NTS);
        SQLBindParameter(stmt2,1,SQL_PARAM_INPUT,
                        SQL_C_CHAR,SQL_VARCHAR,
                        255,0,col1,sizeof(col1),&p1len);
        SQLBindParameter(stmt2,2,SQL_PARAM_INPUT,
                        SQL_C_CHAR,SQL_VARCHAR,
                        255,0,col2,sizeof(col2),&p2len);
        SQLBindParameter(stmt2,3,SQL_PARAM_INPUT,
                        SQL_C_CHAR,SQL_VARCHAR,
                        255,0,col3,sizeof(col3),&p3len);

        while (SQLFetch(stmt1)==SQL_SUCCESS) {
                SQLExecute(stmt2);
        }

        SQLFreeHandle(SQL_HANDLE_STMT,stmt2);
        SQLFreeHandle(SQL_HANDLE_STMT,stmt1);
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC,dbc);
        SQLFreeHandle(SQL_HANDLE_ENV,env);
}
