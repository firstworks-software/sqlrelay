sqlrelay:start(),

... get the filename from the previous page ...

... get the page to display from the previous page ...

sqlrelay:alloc("host", 9000, "", "user", "password", 0, 1),

sqlrelay:openCachedResultSet(Filename),
sqlrelay:endSession(),

ColCount = sqlrelay:colCount(),
StartRow = PageToDisplay * 20,
EndRow = (PageToDisplay + 1) * 20,
print_rows(StartRow, EndRow, ColCount).

print_rows(Row, EndRow, _ColCount) when Row >= EndRow -> ok;
print_rows(Row, EndRow, ColCount) ->
    print_cols(Row, 0, ColCount),
    io:format("~n", []),
    print_rows(Row + 1, EndRow, ColCount).

print_cols(_Row, Col, ColCount) when Col >= ColCount -> ok;
print_cols(Row, Col, ColCount) ->
    io:format("~s,", [sqlrelay:getFieldByIndex(Row, Col)]),
    print_cols(Row, Col + 1, ColCount).
