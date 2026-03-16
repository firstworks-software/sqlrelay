sqlrelay:start(),
sqlrelay:alloc("host", 9000, "/tmp/example.socket",
               "user", "password", 0, 1),

sqlrelay:setResultSetBufferSize(2),

sqlrelay:sendQuery("select * from exampletable"),

ColCount = sqlrelay:colCount(),
print_rows(0, ColCount).

print_rows(Row, ColCount) ->
    print_cols(Row, 0, ColCount),
    io:format("~n", []),
    case sqlrelay:endOfResultSet() =:= 1 andalso
         Row + 1 >= sqlrelay:rowCount() of
        true -> ok;
        false -> print_rows(Row + 1, ColCount)
    end.

print_cols(_Row, Col, ColCount) when Col >= ColCount -> ok;
print_cols(Row, Col, ColCount) ->
    io:format("~s,", [sqlrelay:getFieldByIndex(Row, Col)]),
    print_cols(Row, Col + 1, ColCount).
