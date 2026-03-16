sqlrelay:start(),
sqlrelay:alloc("host", 9000, "/tmp/example.socket",
               "user", "password", 0, 1),

%% Note: The Erlang API currently supports a single cursor.
%% To use multiple cursors, multiple processes would be needed.

sqlrelay:setResultSetBufferSize(10),
sqlrelay:sendQuery("select * from my_huge_table"),

... iterate through the result set and process rows ...
