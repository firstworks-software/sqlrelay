SQLINTEGER      out1;
SQLDOUBLE       out2;
SQLCHAR         out3[21];
SQLLEN          out1len=0;
SQLLEN          out2len=0;
SQLLEN          out3len=0;

SQLPrepare(stmt,(SQLCHAR *)"{call exampleproc(?,?,?)}",SQL_NTS);
SQLBindParameter(stmt,1,SQL_PARAM_OUTPUT,
                SQL_C_SLONG,SQL_INTEGER,0,0,&out1,0,&out1len);
SQLBindParameter(stmt,2,SQL_PARAM_OUTPUT,
                SQL_C_DOUBLE,SQL_DOUBLE,0,0,&out2,0,&out2len);
SQLBindParameter(stmt,3,SQL_PARAM_OUTPUT,
                SQL_C_CHAR,SQL_VARCHAR,20,0,out3,sizeof(out3),&out3len);
SQLExecute(stmt);
// out1,out2,out3 now contain the results
