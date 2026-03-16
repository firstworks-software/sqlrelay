sqlrelay:start(),
sqlrelay:alloc("host", 9000, "", "user", "password", 0, 1),

... generate a unique file name ...

sqlrelay:cacheToFile(Filename),
sqlrelay:setCacheTtl(600),
sqlrelay:sendQuery("select * from my_table"),
sqlrelay:endSession(),
sqlrelay:cacheOff(),

... pass the filename to the next page ...
