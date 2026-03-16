sqlrelay:start(),
sqlrelay:alloc("host", 9000, "/tmp/example.socket",
               "user", "password", 0, 1),

sqlrelay:sendQuery("select * from my_table"),

... do some stuff that takes a short time ...

sqlrelay:sendFileQuery("/usr/local/myprogram/sql", "myquery.sql"),
sqlrelay:endSession(),

... do some stuff that takes a long time ...

sqlrelay:sendQuery("select * from my_other_table"),
sqlrelay:endSession(),

... process the result set ...
