sqlrelay:start(),
sqlrelay:alloc("host", 9000, "/tmp/example.socket",
               "user", "password", 0, 1),

sqlrelay:sendQuery("select * from my_table"),
sqlrelay:endSession(),

RowCount = sqlrelay:rowCount(),
ColCount = sqlrelay:colCount(),
print_rows(0, RowCount, ColCount).

print_rows(Row, RowCount, _ColCount) when Row >= RowCount -> ok;
print_rows(Row, RowCount, ColCount) ->
    print_cols(Row, 0, ColCount),
    print_rows(Row + 1, RowCount, ColCount).

print_cols(_Row, Col, ColCount) when Col >= ColCount -> ok;
print_cols(Row, Col, ColCount) ->
    io:format("~s~n", [sqlrelay:getFieldByIndex(Row, Col)]),
    print_cols(Row, Col + 1, ColCount).
