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
    RowList = sqlrelay:getRow(Row),
    print_row_fields(RowList, 0, ColCount),
    print_rows(Row + 1, RowCount, ColCount).

print_row_fields(_RowList, Col, ColCount) when Col >= ColCount -> ok;
print_row_fields(RowList, Col, ColCount) ->
    io:format("~s~n", [lists:nth(Col + 1, RowList)]),
    print_row_fields(RowList, Col + 1, ColCount).
