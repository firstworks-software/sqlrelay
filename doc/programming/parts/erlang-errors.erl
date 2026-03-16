sqlrelay:start(),
sqlrelay:alloc("host", 9000, "/tmp/example.socket",
               "user", "password", 0, 1),

case sqlrelay:sendQuery("select * from my_table") of
    {ok, _} ->
        ok;
    {error, _} ->
        io:format("~s~n", [sqlrelay:errorMessage()])
end.
