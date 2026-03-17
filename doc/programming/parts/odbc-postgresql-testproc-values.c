SQLINTEGER      in1=1;
SQLDOUBLE       in2=1.1;
SQLCHAR         in3[]="hello";
SQLLEN          in1len=0;
SQLLEN          in2len=0;
SQLLEN          in3len=SQL_NTS;

SQLPrepare(stmt,
        (SQLCHAR *)"select * from examplefunc(?,?,?) "
                "as (col1 int, col2 float, col3 char(20))",
        SQL_NTS);
SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
                SQL_C_SLONG,SQL_INTEGER,0,0,&in1,0,&in1len);
SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
                SQL_C_DOUBLE,SQL_DOUBLE,0,0,&in2,0,&in2len);
SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
                SQL_C_CHAR,SQL_VARCHAR,20,0,in3,sizeof(in3),&in3len);
SQLExecute(stmt);

SQLCHAR out1[256];
SQLCHAR out2[256];
SQLCHAR out3[256];
SQLLEN  out1len;
SQLLEN  out2len;
SQLLEN  out3len;
SQLFetch(stmt);
SQLGetData(stmt,1,SQL_C_CHAR,out1,sizeof(out1),&out1len);
SQLGetData(stmt,2,SQL_C_CHAR,out2,sizeof(out2),&out2len);
SQLGetData(stmt,3,SQL_C_CHAR,out3,sizeof(out3),&out3len);
