SQLINTEGER      in1=1;
SQLDOUBLE       in2=1.1;
SQLCHAR         in3[]="hello";
SQLINTEGER      out1;
SQLDOUBLE       out2;
SQLCHAR         out3[21];
SQLLEN          in1len=0;
SQLLEN          in2len=0;
SQLLEN          in3len=SQL_NTS;
SQLLEN          out1len=0;
SQLLEN          out2len=0;
SQLLEN          out3len=0;

SQLPrepare(stmt,(SQLCHAR *)"{call exampleproc(?,?,?,?,?,?)}",SQL_NTS);
SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
                SQL_C_SLONG,SQL_INTEGER,0,0,&in1,0,&in1len);
SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
                SQL_C_DOUBLE,SQL_DOUBLE,0,0,&in2,0,&in2len);
SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
                SQL_C_CHAR,SQL_VARCHAR,20,0,in3,sizeof(in3),&in3len);
SQLBindParameter(stmt,4,SQL_PARAM_OUTPUT,
                SQL_C_SLONG,SQL_INTEGER,0,0,&out1,0,&out1len);
SQLBindParameter(stmt,5,SQL_PARAM_OUTPUT,
                SQL_C_DOUBLE,SQL_DOUBLE,0,0,&out2,0,&out2len);
SQLBindParameter(stmt,6,SQL_PARAM_OUTPUT,
                SQL_C_CHAR,SQL_VARCHAR,20,0,out3,sizeof(out3),&out3len);
SQLExecute(stmt);
// out1,out2,out3 now contain the results
