sqlrelay:prepareQuery("execute procedure exampleproc ?, ?, ?"),
sqlrelay:inputBindLong("1", 1),
sqlrelay:inputBindDouble("2", 1.1, 2, 1),
sqlrelay:inputBindString("3", "hello"),
$cur defineOutputBindInteger "1" 20
$cur defineOutputBindDouble "2" 20
sqlrelay:defineOutputBindString("3", 20),
sqlrelay:executeQuery(),
Out1 = sqlrelay:getOutputBindInteger("1"),
Out2 = sqlrelay:getOutputBindDouble("2"),
Out3 = sqlrelay:getOutputBindString("3"),
