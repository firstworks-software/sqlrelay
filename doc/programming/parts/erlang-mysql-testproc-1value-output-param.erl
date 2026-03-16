sqlrelay:sendQuery("set @out1=0"),
sqlrelay:sendQuery("call exampleproc()"),
sqlrelay:sendQuery("select @out1"),
Result = sqlrelay:getFieldByIndex(0, 0),
