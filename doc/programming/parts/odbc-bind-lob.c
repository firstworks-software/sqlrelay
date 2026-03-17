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
                (SQLCHAR *)"create table images "
                        "(image blob, description clob)",
                SQL_NTS);
        SQLFreeStmt(stmt,SQL_CLOSE);

        unsigned char   imagedata[40000];
        SQLLEN          imagelength;

        ... read an image from a file into imagedata and the length of the
                file into imagelength ...

        SQLCHAR         description[40000];
        SQLLEN          desclength;

        ... read a description from a file into description and the length of
                the file into desclength ...

        SQLPrepare(stmt,
                (SQLCHAR *)"insert into images values (?,?)",
                SQL_NTS);
        SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
                        SQL_C_BINARY,SQL_LONGVARBINARY,
                        imagelength,0,imagedata,imagelength,
                        &imagelength);
        SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
                        SQL_C_CHAR,SQL_LONGVARCHAR,
                        desclength,0,description,desclength,
                        &desclength);
        SQLExecute(stmt);

        SQLFreeHandle(SQL_HANDLE_STMT,stmt);
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC,dbc);
        SQLFreeHandle(SQL_HANDLE_ENV,env);
}
