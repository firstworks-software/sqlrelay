sqlrelay:start(),
sqlrelay:alloc("host", 9000, "/tmp/example.socket",
               "user", "password", 0, 1),

sqlrelay:prepareQuery("select * from mytable where mycolumn>:value"),
sqlrelay:inputBindLong("value", 1),
sqlrelay:executeQuery(),

... process the result set ...

sqlrelay:clearBinds(),
sqlrelay:inputBindLong("value", 5),
sqlrelay:executeQuery(),

... process the result set ...

sqlrelay:clearBinds(),
sqlrelay:inputBindLong("value", 10),
sqlrelay:executeQuery(),

... process the result set ...
