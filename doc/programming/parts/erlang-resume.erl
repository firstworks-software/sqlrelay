sqlrelay:start(),

... get Rs, Port and Socket from previous page ...

sqlrelay:alloc("host", 9000, "", "user", "password", 0, 1),

sqlrelay:resumeSession(Port, Socket),
sqlrelay:resumeResultSet(Rs),
sqlrelay:sendQuery("commit"),
sqlrelay:endSession(),
