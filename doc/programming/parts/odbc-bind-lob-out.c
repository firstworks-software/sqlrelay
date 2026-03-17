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

        SQLExecDirect(stmt,
                (SQLCHAR *)"select image, description from images",
                SQL_NTS);

        unsigned char   image[40000];
        SQLLEN          imagelength;
        SQLCHAR         desc[40000];
        SQLLEN          desclength;

        while (SQLFetch(stmt)==SQL_SUCCESS) {
                SQLGetData(stmt,1,SQL_C_BINARY,
                                image,sizeof(image),
                                &imagelength);
                SQLGetData(stmt,2,SQL_C_CHAR,
                                desc,sizeof(desc),
                                &desclength);

                ... do something with image and desc ...
        }

        SQLFreeHandle(SQL_HANDLE_STMT,stmt);
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC,dbc);
        SQLFreeHandle(SQL_HANDLE_ENV,env);
}
