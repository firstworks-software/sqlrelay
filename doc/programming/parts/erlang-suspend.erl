sqlrelay:start(),
sqlrelay:alloc("host", 9000, "", "user", "password", 0, 1),

sqlrelay:sendQuery("insert into my_table values (1,2,3)"),
sqlrelay:suspendResultSet(),
sqlrelay:suspendSession(),
Rs = sqlrelay:getResultSetId(),
Port = sqlrelay:getConnectionPort(),
Socket = sqlrelay:getConnectionSocket(),

... pass the Rs, Port and Socket to the next page ...
