sqlrelay:start(),
sqlrelay:alloc("host", 9000, "/tmp/example.socket",
               "user", "password", 0, 1),

sqlrelay:sendQuery("select * from my_table"),
sqlrelay:endSession(),

ColCount = sqlrelay:colCount(),
print_column_info(0, ColCount).

print_column_info(I, ColCount) when I >= ColCount -> ok;
print_column_info(I, ColCount) ->
    io:format("Name:           ~s~n", [sqlrelay:getColumnName(I)]),
    io:format("Type:           ~s~n", [sqlrelay:getColumnTypeByIndex(I)]),
    io:format("Length:         ~p~n", [sqlrelay:getColumnLengthByIndex(I)]),
    io:format("Precision:      ~p~n", [sqlrelay:getColumnPrecisionByIndex(I)]),
    io:format("Scale:          ~p~n", [sqlrelay:getColumnScaleByIndex(I)]),
    io:format("Longest Field:  ~p~n", [sqlrelay:getLongestByIndex(I)]),
    io:format("Nullable:       ~p~n", [sqlrelay:getColumnIsNullableByIndex(I)]),
    io:format("Primary Key:    ~p~n", [sqlrelay:getColumnIsPrimaryKeyByIndex(I)]),
    io:format("Unique:         ~p~n", [sqlrelay:getColumnIsUniqueByIndex(I)]),
    io:format("Part Of Key:    ~p~n", [sqlrelay:getColumnIsPartOfKeyByIndex(I)]),
    io:format("Unsigned:       ~p~n", [sqlrelay:getColumnIsUnsignedByIndex(I)]),
    io:format("Zero Filled:    ~p~n", [sqlrelay:getColumnIsZeroFilledByIndex(I)]),
    io:format("Binary:         ~p~n", [sqlrelay:getColumnIsBinaryByIndex(I)]),
    io:format("Auto Increment: ~p~n", [sqlrelay:getColumnIsAutoIncrementByIndex(I)]),
    print_column_info(I + 1, ColCount).
