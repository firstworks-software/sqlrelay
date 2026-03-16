sqlrelay:start(),
sqlrelay:alloc("host", 9000, "/tmp/example.socket",
               "user", "password", 0, 1),

sqlrelay:prepareQuery("select * from mytable $(whereclause)"),
sqlrelay:subString("whereclause",
    "where stringcol=:stringval "
    "and integercol>:integerval "
    "and floatcol>:floatval"),
sqlrelay:inputBindString("stringval", "true"),
sqlrelay:inputBindLong("integerval", 10),
sqlrelay:inputBindDouble("floatval", 1.1, 2, 1),
sqlrelay:executeQuery(),

... process the result set ...
