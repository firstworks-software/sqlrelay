SQLExecDirect(stmt,(SQLCHAR *)"{call exampleproc}",SQL_NTS);

SQLCHAR result[256];
SQLLEN  resultlen;
SQLFetch(stmt);
SQLGetData(stmt,1,SQL_C_CHAR,result,sizeof(result),&resultlen);
