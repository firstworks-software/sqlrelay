sqlrelay:prepareQuery("call exampleproc(?,?,?,?,?,?)"),
sqlrelay:inputBindLong("1", 1),
sqlrelay:inputBindDouble("2", 1.1, 2, 1),
sqlrelay:inputBindString("3", "hello"),
$cur defineOutputBindInteger "4" 25
$cur defineOutputBindDouble "5" 25
sqlrelay:defineOutputBindString("6", 25),
sqlrelay:executeQuery(),
Out1 = sqlrelay:getOutputBindInteger("4"),
Out2 = sqlrelay:getOutputBindDouble("5"),
Out3 = sqlrelay:getOutputBindString("6"),
