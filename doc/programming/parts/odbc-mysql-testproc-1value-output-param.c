SQLINTEGER      out1;
SQLLEN          out1len=0;

SQLPrepare(stmt,(SQLCHAR *)"{call exampleproc(?)}",SQL_NTS);
SQLBindParameter(stmt,1,SQL_PARAM_OUTPUT,
                SQL_C_SLONG,SQL_INTEGER,0,0,&out1,0,&out1len);
SQLExecute(stmt);
// out1 now contains the result
