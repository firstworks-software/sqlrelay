sqlrelay:start(),
sqlrelay:alloc("host", 9000, "/tmp/example.socket",
               "user", "password", 0, 1),

sqlrelay:prepareQuery("begin  "
    ":result1:=addTwoIntegers(:integer1,:integer2);  "
    ":result2=addTwoFloats(:float1,:float2);  "
    ":result3=convertToString(:integer3); end;"),
sqlrelay:inputBindLong("integer1", 10),
sqlrelay:inputBindLong("integer2", 20),
sqlrelay:inputBindDouble("float1", 1.1, 2, 1),
sqlrelay:inputBindDouble("float2", 2.2, 2, 1),
sqlrelay:inputBindLong("integer3", 30),
sqlrelay:defineOutputBindInteger("result1"),
sqlrelay:defineOutputBindDouble("result2"),
sqlrelay:defineOutputBindString("result3", 100),
sqlrelay:executeQuery(),
Result1 = sqlrelay:getOutputBindInteger("result1"),
Result2 = sqlrelay:getOutputBindDouble("result2"),
Result3 = sqlrelay:getOutputBindString("result3"),
sqlrelay:endSession(),

... do something with the results ...
