sqlrelay:start(),
sqlrelay:alloc("host", 9000, "/tmp/example.socket",
               "user", "password", 0, 1),

%% column names will be forced to upper case
sqlrelay:upperCaseColumnNames(),
sqlrelay:sendQuery("select * from my_table"),
sqlrelay:endSession(),

ColCount = sqlrelay:colCount(),
print_names(0, ColCount),

%% column names will be forced to lower case
sqlrelay:lowerCaseColumnNames(),
sqlrelay:sendQuery("select * from my_table"),
sqlrelay:endSession(),

ColCount2 = sqlrelay:colCount(),
print_names(0, ColCount2),

%% column names will be the same as they are in the database
sqlrelay:mixedCaseColumnNames(),
sqlrelay:sendQuery("select * from my_table"),
sqlrelay:endSession(),

ColCount3 = sqlrelay:colCount(),
print_names(0, ColCount3).

print_names(I, ColCount) when I >= ColCount -> ok;
print_names(I, ColCount) ->
    io:format("Name: ~s~n", [sqlrelay:getColumnName(I)]),
    print_names(I + 1, ColCount).
