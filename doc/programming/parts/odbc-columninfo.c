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

        for (int i=1; i<=cols; i++) {

                SQLCHAR         name[256];
                SQLSMALLINT     namelen;
                SQLSMALLINT     datatype;
                SQLULEN         columnsize;
                SQLSMALLINT     decimaldigits;
                SQLSMALLINT     nullable;

                SQLDescribeCol(stmt,i,
                                name,sizeof(name),&namelen,
                                &datatype,&columnsize,
                                &decimaldigits,&nullable);

                SQLLEN  displaysize;
                SQLColAttribute(stmt,i,
                                SQL_DESC_DISPLAY_SIZE,
                                NULL,0,NULL,
                                &displaysize);

                SQLLEN  autoincrement;
                SQLColAttribute(stmt,i,
                                SQL_DESC_AUTO_UNIQUE_VALUE,
                                NULL,0,NULL,
                                &autoincrement);

                SQLLEN  isunsigned;
                SQLColAttribute(stmt,i,
                                SQL_DESC_UNSIGNED,
                                NULL,0,NULL,
                                &isunsigned);

                printf("Name:           %s\n",name);
                printf("Type:           %d\n",datatype);
                printf("Column Size:    %ld\n",(long)columnsize);
                printf("Decimal Digits: %d\n",decimaldigits);
                printf("Nullable:       %d\n",nullable);
                printf("Display Size:   %ld\n",(long)displaysize);
                printf("Auto Increment: %ld\n",(long)autoincrement);
                printf("Unsigned:       %ld\n",(long)isunsigned);
                printf("\n");
        }

        SQLFreeHandle(SQL_HANDLE_STMT,stmt);
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC,dbc);
        SQLFreeHandle(SQL_HANDLE_ENV,env);
}
