SQLINTEGER      in1=1;
SQLDOUBLE       in2=1.1;
SQLCHAR         in3[]="hello";
SQLLEN          in1len=0;
SQLLEN          in2len=0;
SQLLEN          in3len=SQL_NTS;

SQLPrepare(stmt,
        (SQLCHAR *)"select * from exampleproc(?,?,?)",SQL_NTS);
SQLBindParameter(stmt,1,SQL_PARAM_INPUT,
                SQL_C_SLONG,SQL_INTEGER,0,0,&in1,0,&in1len);
SQLBindParameter(stmt,2,SQL_PARAM_INPUT,
                SQL_C_DOUBLE,SQL_DOUBLE,0,0,&in2,0,&in2len);
SQLBindParameter(stmt,3,SQL_PARAM_INPUT,
                SQL_C_CHAR,SQL_VARCHAR,20,0,in3,sizeof(in3),&in3len);
SQLExecute(stmt);

SQLCHAR result[256];
SQLLEN  resultlen;
SQLFetch(stmt);
SQLGetData(stmt,1,SQL_C_CHAR,result,sizeof(result),&resultlen);
