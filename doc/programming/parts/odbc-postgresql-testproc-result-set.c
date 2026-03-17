SQLExecDirect(stmt,
        (SQLCHAR *)"select * from examplefunc() "
                "as (col1 int, col2 float, col3 char(40))",
        SQL_NTS);

SQLCHAR col1[256];
SQLCHAR col2[256];
SQLCHAR col3[256];
SQLLEN  col1len;
SQLLEN  col2len;
SQLLEN  col3len;

while (SQLFetch(stmt)==SQL_SUCCESS) {
        SQLGetData(stmt,1,SQL_C_CHAR,col1,sizeof(col1),&col1len);
        SQLGetData(stmt,2,SQL_C_CHAR,col2,sizeof(col2),&col2len);
        SQLGetData(stmt,3,SQL_C_CHAR,col3,sizeof(col3),&col3len);
        ... process col1, col2, col3 ...
}
