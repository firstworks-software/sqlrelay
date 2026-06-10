%% Copyright (c) David Muse
%% See the file COPYING for more information.

-module(sqlite).
-export([main/0]).

-import(asserts, [pass/0, fail/2,
                  getStatus/0, reportTestStatus/0,
                  assertEqualsString/2, assertEqualsStringLen/3,
                  assertEqualsInt/2, assertEqualsDouble/2,
                  assertTrue/1, assertFalse/1,
                  assertInResultSet/2,
                  waitForPort/1, largeBuffer/1, shortHostname/0]).

%% Iterate through an isolation-level list.
setIsolationLevels([]) ->
    ok;
setIsolationLevels([Il | Rest]) ->
    assertTrue(sqlrelay:setIsolationLevel(Il)),
    assertEqualsString(sqlrelay:getIsolationLevel(), Il),
    io:format("~n"),
    setIsolationLevels(Rest).

main() ->
    sqlrelay:start(),
    waitForPort(50),
    {ok, _} = sqlrelay:alloc("sqlrelay", 9000, "/tmp/test.socket",
                             "testuser", "testpassword", 0, 1),

    _Hostname = shortHostname(),

    %% IDENTIFY
    io:format("IDENTIFY: ~n"),
    assertEqualsString(sqlrelay:identify(), "sqlite"),
    io:format("~n"),

    %% DB VERSION
    io:format("DB VERSION: ~n"),
    {ok, DbVersion} = sqlrelay:dbVersion(),
    IsSqlite3 = case DbVersion of
                    undefined -> false;
                    null      -> false;
                    ""        -> false;
                    "unknown" -> false;
                    _ ->
                        Major = case string:chr(DbVersion, $.) of
                                    0 -> DbVersion;
                                    N -> string:substr(DbVersion, 1, N - 1)
                                end,
                        case (catch list_to_integer(Major)) of
                            V when is_integer(V), V >= 3 -> true;
                            _ -> false
                        end
                end,
    io:format("~n"),

    %% PING
    io:format("PING: ~n"),
    assertTrue(sqlrelay:ping()),
    io:format("~n"),

    %% TRANSACTION STATE
    io:format("TRANSACTION STATE: ~n"),
    assertEqualsString(sqlrelay:getDefaultTransactionModel(), "explicit"),
    assertEqualsString(sqlrelay:getTransactionModel(), "explicit"),
    assertFalse(sqlrelay:getInTransaction()),
    assertTrue(sqlrelay:getAutoCommit()),
    io:format("~n"),

    %% BIND FORMAT
    io:format("BIND FORMAT: ~n"),
    assertEqualsString(sqlrelay:bindFormat(), ":*"),
    io:format("~n"),

    %% NEXTVAL FORMAT
    io:format("NEXTVAL FORMAT: ~n"),
    assertEqualsString(sqlrelay:nextvalFormat(), ""),
    io:format("~n"),

    %% ISOLATION LEVELS
    io:format("ISOLATION LEVELS: ~n"),
    IsolationLevels = ["0", "1"],
    setIsolationLevels(IsolationLevels),
    %% reset to the default isolation level
    assertTrue(sqlrelay:setIsolationLevel(hd(IsolationLevels))),
    io:format("~n"),

    %% CREATE TESTTABLE
    io:format("CREATE TESTTABLE: ~n"),
    sqlrelay:beginTransaction(),
    sqlrelay:sendQuery("drop table if exists testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	testint int, "
        "	testfloat float, "
        "	testchar char(40), "
        "	testvarchar varchar(40), "
        "	testclob clob, "
        "	testblob blob)")),
    sqlrelay:commit(),
    io:format("~n"),

    %% INSERT
    io:format("INSERT: ~n"),
    assertTrue(sqlrelay:beginTransaction()),
    assertTrue(sqlrelay:sendQuery(
        "insert into "
        "	testtable "
        "values ("
        "	1, "
        "	1.5, "
        "	'testchar1', "
        "	'testvarchar1', "
        "	'testclob1', "
        "	'testblob1')")),
    assertTrue(sqlrelay:sendQuery(
        "insert into "
        "	testtable "
        "values ("
        "	2, "
        "	2.5, "
        "	'testchar2', "
        "	'testvarchar2', "
        "	'testclob2', "
        "	'testblob2')")),
    assertTrue(sqlrelay:sendQuery(
        "insert into "
        "	testtable "
        "values ("
        "	3, "
        "	3.5, "
        "	'testchar3', "
        "	'testvarchar3', "
        "	'testclob3', "
        "	'testblob3')")),
    assertTrue(sqlrelay:sendQuery(
        "insert into "
        "	testtable "
        "values ("
        "	4, "
        "	4.5, "
        "	'testchar4', "
        "	'testvarchar4', "
        "	'testclob4', "
        "	'testblob4')")),
    io:format("~n"),

    %% AFFECTED ROWS
    io:format("AFFECTED ROWS: ~n"),
    assertEqualsInt(sqlrelay:affectedRows(), 1),
    io:format("~n"),

    %% INPUT BIND BY POSITION
    %% sqlite doesn't support bind by position

    %% ARRAY OF INPUT BINDS BY POSITION
    %% sqlite doesn't support bind by position

    %% INPUT BIND BY POSITION WITH VALIDATION
    %% sqlite doesn't support bind by position

    %% INPUT BIND BY NAME
    io:format("INPUT BIND BY NAME: ~n"),
    sqlrelay:prepareQuery(
        "insert into "
        "	testtable "
        "values ("
        "	:var1, "
        "	:var2, "
        "	:var3, "
        "	:var4, "
        "	:var5, "
        "	:var6)"),
    assertEqualsInt(sqlrelay:countBindVariables(), 6),
    sqlrelay:inputBindLong("var1", 5),
    sqlrelay:inputBindDouble("var2", 5.5, 4, 1),
    sqlrelay:inputBindString("var3", "testchar5"),
    sqlrelay:inputBindString("var4", "testvarchar5"),
    sqlrelay:inputBindClob("var5", "testclob5", 9),
    sqlrelay:inputBindBlob("var6", "testblob5", 9),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("var1", 6),
    sqlrelay:inputBindDouble("var2", 6.5, 4, 1),
    sqlrelay:inputBindString("var3", "testchar6"),
    sqlrelay:inputBindString("var4", "testvarchar6"),
    sqlrelay:inputBindClob("var5", "testclob6", 9),
    sqlrelay:inputBindBlob("var6", "testblob6", 9),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("var1", 7),
    sqlrelay:inputBindDouble("var2", 7.5, 4, 1),
    sqlrelay:inputBindString("var3", "testchar7"),
    sqlrelay:inputBindString("var4", "testvarchar7"),
    sqlrelay:inputBindClob("var5", "testclob7", 9),
    sqlrelay:inputBindBlob("var6", "testblob7", 9),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% ARRAY OF INPUT BINDS BY NAME
    %% sqlite doesn't support implicit conversion of string binds to
    %% other data types, so arrays of binds don't generally work.

    %% INPUT BIND BY NAME WITH VALIDATION
    io:format("INPUT BIND BY NAME WITH VALIDATION: ~n"),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("var1", 8),
    sqlrelay:inputBindDouble("var2", 8.5, 4, 1),
    sqlrelay:inputBindString("var3", "testchar8"),
    sqlrelay:inputBindString("var4", "testvarchar8"),
    sqlrelay:inputBindClob("var5", "testclob8", 9),
    sqlrelay:inputBindBlob("var6", "testblob8", 9),
    sqlrelay:validateBinds(),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% SELECT
    io:format("SELECT: ~n"),
    assertTrue(sqlrelay:sendQuery("select * from testtable order by testint")),
    io:format("~n"),

    %% COLUMN COUNT
    io:format("COLUMN COUNT: ~n"),
    assertEqualsInt(sqlrelay:colCount(), 6),
    io:format("~n"),

    %% COLUMN NAMES
    io:format("COLUMN NAMES: ~n"),
    assertEqualsString(sqlrelay:getColumnName(0), "testint"),
    assertEqualsString(sqlrelay:getColumnName(1), "testfloat"),
    assertEqualsString(sqlrelay:getColumnName(2), "testchar"),
    assertEqualsString(sqlrelay:getColumnName(3), "testvarchar"),
    {ok, Cols1} = sqlrelay:getColumnNames(),
    assertEqualsString(lists:nth(1, Cols1), "testint"),
    assertEqualsString(lists:nth(2, Cols1), "testfloat"),
    assertEqualsString(lists:nth(3, Cols1), "testchar"),
    assertEqualsString(lists:nth(4, Cols1), "testvarchar"),
    io:format("~n"),

    %% COLUMN TYPES
    io:format("COLUMN TYPES: ~n"),
    case IsSqlite3 of
        true ->
            assertEqualsString(sqlrelay:getColumnTypeByIndex(0), "INTEGER"),
            assertEqualsString(sqlrelay:getColumnTypeByName("testint"), "INTEGER"),
            assertEqualsString(sqlrelay:getColumnTypeByIndex(1), "FLOAT"),
            assertEqualsString(sqlrelay:getColumnTypeByName("testfloat"), "FLOAT"),
            assertEqualsString(sqlrelay:getColumnTypeByIndex(2), "STRING"),
            assertEqualsString(sqlrelay:getColumnTypeByName("testchar"), "STRING"),
            assertEqualsString(sqlrelay:getColumnTypeByIndex(3), "STRING"),
            assertEqualsString(sqlrelay:getColumnTypeByName("testvarchar"), "STRING"),
            assertEqualsString(sqlrelay:getColumnTypeByIndex(4), "STRING"),
            assertEqualsString(sqlrelay:getColumnTypeByName("testclob"), "STRING"),
            assertEqualsString(sqlrelay:getColumnTypeByIndex(5), "STRING"),
            assertEqualsString(sqlrelay:getColumnTypeByName("testblob"), "STRING");
        false ->
            assertEqualsString(sqlrelay:getColumnTypeByIndex(0), "UNKNOWN"),
            assertEqualsString(sqlrelay:getColumnTypeByName("testint"), "UNKNOWN"),
            assertEqualsString(sqlrelay:getColumnTypeByIndex(1), "UNKNOWN"),
            assertEqualsString(sqlrelay:getColumnTypeByName("testfloat"), "UNKNOWN"),
            assertEqualsString(sqlrelay:getColumnTypeByIndex(2), "UNKNOWN"),
            assertEqualsString(sqlrelay:getColumnTypeByName("testchar"), "UNKNOWN"),
            assertEqualsString(sqlrelay:getColumnTypeByIndex(3), "UNKNOWN"),
            assertEqualsString(sqlrelay:getColumnTypeByName("testvarchar"), "UNKNOWN"),
            assertEqualsString(sqlrelay:getColumnTypeByIndex(4), "UNKNOWN"),
            assertEqualsString(sqlrelay:getColumnTypeByName("testclob"), "UNKNOWN"),
            assertEqualsString(sqlrelay:getColumnTypeByIndex(5), "UNKNOWN"),
            assertEqualsString(sqlrelay:getColumnTypeByName("testblob"), "UNKNOWN")
    end,
    io:format("~n"),

    %% COLUMN LENGTH
    io:format("COLUMN LENGTH: ~n"),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testint"), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(1), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testfloat"), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(2), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testchar"), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(3), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testvarchar"), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(4), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testclob"), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(5), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testblob"), 0),
    io:format("~n"),

    %% LONGEST COLUMN
    io:format("LONGEST COLUMN: ~n"),
    assertEqualsInt(sqlrelay:getLongestByIndex(0), 1),
    assertEqualsInt(sqlrelay:getLongestByName("testint"), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(1), 3),
    assertEqualsInt(sqlrelay:getLongestByName("testfloat"), 3),
    assertEqualsInt(sqlrelay:getLongestByIndex(2), 9),
    assertEqualsInt(sqlrelay:getLongestByName("testchar"), 9),
    assertEqualsInt(sqlrelay:getLongestByIndex(3), 12),
    assertEqualsInt(sqlrelay:getLongestByName("testvarchar"), 12),
    assertEqualsInt(sqlrelay:getLongestByIndex(4), 9),
    assertEqualsInt(sqlrelay:getLongestByName("testclob"), 9),
    assertEqualsInt(sqlrelay:getLongestByIndex(5), 9),
    assertEqualsInt(sqlrelay:getLongestByName("testblob"), 9),
    io:format("~n"),

    %% ROW COUNT
    io:format("ROW COUNT: ~n"),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    io:format("~n"),

    %% TOTAL ROWS
    io:format("TOTAL ROWS: ~n"),
    ExpectedTotalRows = case IsSqlite3 of true -> 0; false -> 8 end,
    assertEqualsInt(sqlrelay:totalRows(), ExpectedTotalRows),
    io:format("~n"),

    %% FIRST ROW INDEX
    io:format("FIRST ROW INDEX: ~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 0),
    io:format("~n"),

    %% END OF RESULT SET
    io:format("END OF RESULT SET: ~n"),
    assertTrue(sqlrelay:endOfResultSet()),
    io:format("~n"),

    %% FIELDS BY INDEX
    io:format("FIELDS BY INDEX: ~n"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "1.5"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "testchar1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 3), "testvarchar1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 4), "testclob1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 5), "testblob1"),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8.5"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 2), "testchar8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 3), "testvarchar8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 4), "testclob8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 5), "testblob8"),
    io:format("~n"),

    %% FIELD LENGTHS BY INDEX
    io:format("FIELD LENGTHS BY INDEX: ~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 1), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 2), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 3), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 4), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 5), 9),
    io:format("~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 0), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 1), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 2), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 3), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 4), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 5), 9),
    io:format("~n"),

    %% FIELDS BY NAME
    io:format("FIELDS BY NAME: ~n"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testint"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testfloat"), "1.5"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testchar"), "testchar1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testvarchar"), "testvarchar1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testclob"), "testclob1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testblob"), "testblob1"),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testint"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testfloat"), "8.5"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testchar"), "testchar8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testvarchar"), "testvarchar8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testclob"), "testclob8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testblob"), "testblob8"),
    io:format("~n"),

    %% FIELD LENGTHS BY NAME
    io:format("FIELD LENGTHS BY NAME: ~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testfloat"), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testchar"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testvarchar"), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testclob"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testblob"), 9),
    io:format("~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testfloat"), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testchar"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testvarchar"), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testclob"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testblob"), 9),
    io:format("~n"),

    %% FIELDS BY ARRAY
    io:format("FIELDS BY ARRAY: ~n"),
    {ok, Row0} = sqlrelay:getRow(0),
    assertEqualsString(lists:nth(1, Row0), "1"),
    assertEqualsString(lists:nth(2, Row0), "1.5"),
    assertEqualsString(lists:nth(3, Row0), "testchar1"),
    assertEqualsString(lists:nth(4, Row0), "testvarchar1"),
    assertEqualsString(lists:nth(5, Row0), "testclob1"),
    assertEqualsString(lists:nth(6, Row0), "testblob1"),
    io:format("~n"),

    %% FIELD LENGTHS BY ARRAY
    io:format("FIELD LENGTHS BY ARRAY: ~n"),
    {ok, Rowlens0} = sqlrelay:getRowLengths(0),
    assertEqualsInt(lists:nth(1, Rowlens0), 1),
    assertEqualsInt(lists:nth(2, Rowlens0), 3),
    assertEqualsInt(lists:nth(3, Rowlens0), 9),
    assertEqualsInt(lists:nth(4, Rowlens0), 12),
    assertEqualsInt(lists:nth(5, Rowlens0), 9),
    assertEqualsInt(lists:nth(6, Rowlens0), 9),
    io:format("~n"),

    %% RESULT SET BUFFER SIZE
    io:format("RESULT SET BUFFER SIZE: ~n"),
    assertEqualsInt(sqlrelay:getResultSetBufferSize(), 0),
    sqlrelay:setResultSetBufferSize(2),
    assertTrue(sqlrelay:sendQuery("select * from testtable order by testint")),
    assertEqualsInt(sqlrelay:getResultSetBufferSize(), 2),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 0),
    assertFalse(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 2),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(1, 0), "2"),
    assertEqualsString(sqlrelay:getFieldByIndex(2, 0), "3"),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 2),
    assertFalse(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 4),
    assertEqualsString(sqlrelay:getFieldByIndex(6, 0), "7"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 6),
    assertFalse(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    assertEqualsString(sqlrelay:getFieldByIndex(8, 0), null),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 8),
    assertTrue(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    sqlrelay:setResultSetBufferSize(0),
    io:format("~n"),

    %% DONT GET COLUMN INFO
    io:format("DONT GET COLUMN INFO: ~n"),
    sqlrelay:dontGetColumnInfo(),
    assertTrue(sqlrelay:sendQuery("select * from testtable order by testint")),
    assertEqualsString(sqlrelay:getColumnName(0), null),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 0),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(0), null),
    sqlrelay:getColumnInfo(),
    assertTrue(sqlrelay:sendQuery("select * from testtable order by testint")),
    assertEqualsString(sqlrelay:getColumnName(0), "testint"),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 0),
    ExpectedColType = case IsSqlite3 of true -> "INTEGER"; false -> "UNKNOWN" end,
    assertEqualsString(sqlrelay:getColumnTypeByIndex(0), ExpectedColType),
    io:format("~n"),

    %% SUSPENDED SESSION
    io:format("SUSPENDED SESSION: ~n"),
    assertTrue(sqlrelay:sendQuery("select * from testtable order by testint")),
    sqlrelay:suspendResultSet(),
    assertTrue(sqlrelay:suspendSession()),
    {ok, Port1} = sqlrelay:getConnectionPort(),
    {ok, Socket1} = sqlrelay:getConnectionSocket(),
    assertTrue(sqlrelay:resumeSession(Port1, Socket1)),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(1, 0), "2"),
    assertEqualsString(sqlrelay:getFieldByIndex(2, 0), "3"),
    assertEqualsString(sqlrelay:getFieldByIndex(3, 0), "4"),
    assertEqualsString(sqlrelay:getFieldByIndex(4, 0), "5"),
    assertEqualsString(sqlrelay:getFieldByIndex(5, 0), "6"),
    assertEqualsString(sqlrelay:getFieldByIndex(6, 0), "7"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    io:format("~n"),

    %% SUSPENDED RESULT SET
    io:format("SUSPENDED RESULT SET: ~n"),
    sqlrelay:setResultSetBufferSize(2),
    assertTrue(sqlrelay:sendQuery("select * from testtable order by testint")),
    assertEqualsString(sqlrelay:getFieldByIndex(2, 0), "3"),
    {ok, Id1} = sqlrelay:getResultSetId(),
    sqlrelay:suspendResultSet(),
    assertTrue(sqlrelay:suspendSession()),
    {ok, Port2} = sqlrelay:getConnectionPort(),
    {ok, Socket2} = sqlrelay:getConnectionSocket(),
    assertTrue(sqlrelay:resumeSession(Port2, Socket2)),
    assertTrue(sqlrelay:resumeResultSet(Id1)),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 4),
    assertFalse(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 6),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 6),
    assertFalse(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    assertEqualsString(sqlrelay:getFieldByIndex(8, 0), null),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 8),
    assertTrue(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    sqlrelay:setResultSetBufferSize(0),
    io:format("~n"),

    %% CACHED RESULT SET
    io:format("CACHED RESULT SET: ~n"),
    sqlrelay:cacheToFile("cachefile1"),
    sqlrelay:setCacheTtl(200),
    assertTrue(sqlrelay:sendQuery("select * from testtable order by testint")),
    {ok, Filename1} = sqlrelay:getCacheFileName(),
    assertEqualsString(Filename1, "cachefile1"),
    sqlrelay:cacheOff(),
    assertTrue(sqlrelay:openCachedResultSet(Filename1)),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    io:format("~n"),

    %% COLUMN COUNT FOR CACHED RESULT SET
    io:format("COLUMN COUNT FOR CACHED RESULT SET: ~n"),
    assertEqualsInt(sqlrelay:colCount(), 6),
    io:format("~n"),

    %% COLUMN NAMES FOR CACHED RESULT SET
    io:format("COLUMN NAMES FOR CACHED RESULT SET: ~n"),
    assertEqualsString(sqlrelay:getColumnName(0), "testint"),
    assertEqualsString(sqlrelay:getColumnName(1), "testfloat"),
    assertEqualsString(sqlrelay:getColumnName(2), "testchar"),
    assertEqualsString(sqlrelay:getColumnName(3), "testvarchar"),
    assertEqualsString(sqlrelay:getColumnName(4), "testclob"),
    assertEqualsString(sqlrelay:getColumnName(5), "testblob"),
    {ok, Cols2} = sqlrelay:getColumnNames(),
    assertEqualsString(lists:nth(1, Cols2), "testint"),
    assertEqualsString(lists:nth(2, Cols2), "testfloat"),
    assertEqualsString(lists:nth(3, Cols2), "testchar"),
    assertEqualsString(lists:nth(4, Cols2), "testvarchar"),
    assertEqualsString(lists:nth(5, Cols2), "testclob"),
    assertEqualsString(lists:nth(6, Cols2), "testblob"),
    io:format("~n"),

    %% CACHED RESULT SET WITH RESULT SET BUFFER SIZE
    io:format("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ~n"),
    sqlrelay:setResultSetBufferSize(2),
    sqlrelay:cacheToFile("cachefile1"),
    sqlrelay:setCacheTtl(200),
    assertTrue(sqlrelay:sendQuery("select * from testtable order by testint")),
    {ok, Filename2} = sqlrelay:getCacheFileName(),
    assertEqualsString(Filename2, "cachefile1"),
    sqlrelay:cacheOff(),
    assertTrue(sqlrelay:openCachedResultSet(Filename2)),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(8, 0), null),
    sqlrelay:setResultSetBufferSize(0),
    io:format("~n"),

    %% FROM ONE CACHE FILE TO ANOTHER
    io:format("FROM ONE CACHE FILE TO ANOTHER: ~n"),
    sqlrelay:cacheToFile("cachefile2"),
    assertTrue(sqlrelay:openCachedResultSet("cachefile1")),
    sqlrelay:cacheOff(),
    assertTrue(sqlrelay:openCachedResultSet("cachefile2")),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(8, 0), null),
    io:format("~n"),

    %% FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET BUFFER SIZE
    io:format("FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET BUFFER SIZE: ~n"),
    sqlrelay:setResultSetBufferSize(2),
    sqlrelay:cacheToFile("cachefile2"),
    assertTrue(sqlrelay:openCachedResultSet("cachefile1")),
    sqlrelay:cacheOff(),
    assertTrue(sqlrelay:openCachedResultSet("cachefile2")),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(8, 0), null),
    sqlrelay:setResultSetBufferSize(0),
    io:format("~n"),

    %% CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE
    io:format("CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE: ~n"),
    sqlrelay:setResultSetBufferSize(2),
    sqlrelay:cacheToFile("cachefile1"),
    sqlrelay:setCacheTtl(200),
    assertTrue(sqlrelay:sendQuery("select * from testtable order by testint")),
    assertEqualsString(sqlrelay:getFieldByIndex(2, 0), "3"),
    {ok, Filename3} = sqlrelay:getCacheFileName(),
    assertEqualsString(Filename3, "cachefile1"),
    {ok, Id2} = sqlrelay:getResultSetId(),
    sqlrelay:suspendResultSet(),
    assertTrue(sqlrelay:suspendSession()),
    {ok, Port3} = sqlrelay:getConnectionPort(),
    {ok, Socket3} = sqlrelay:getConnectionSocket(),
    io:format("~n"),
    assertTrue(sqlrelay:resumeSession(Port3, Socket3)),
    assertTrue(sqlrelay:resumeCachedResultSet(Id2, Filename3)),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 4),
    assertFalse(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 6),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 6),
    assertFalse(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    assertEqualsString(sqlrelay:getFieldByIndex(8, 0), null),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 8),
    assertTrue(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    sqlrelay:cacheOff(),
    io:format("~n"),
    assertTrue(sqlrelay:openCachedResultSet(Filename3)),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(8, 0), null),
    sqlrelay:setResultSetBufferSize(0),
    io:format("~n"),

    %% FINISHED SUSPENDED SESSION
    io:format("FINISHED SUSPENDED SESSION: ~n"),
    assertTrue(sqlrelay:sendQuery("select * from testtable")),
    assertEqualsString(sqlrelay:getFieldByIndex(4, 0), "5"),
    assertEqualsString(sqlrelay:getFieldByIndex(5, 0), "6"),
    assertEqualsString(sqlrelay:getFieldByIndex(6, 0), "7"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    {ok, Id3} = sqlrelay:getResultSetId(),
    sqlrelay:suspendResultSet(),
    assertTrue(sqlrelay:suspendSession()),
    {ok, Port4} = sqlrelay:getConnectionPort(),
    {ok, Socket4} = sqlrelay:getConnectionSocket(),
    assertTrue(sqlrelay:resumeSession(Port4, Socket4)),
    assertTrue(sqlrelay:resumeResultSet(Id3)),
    assertEqualsString(sqlrelay:getFieldByIndex(4, 0), null),
    assertEqualsString(sqlrelay:getFieldByIndex(5, 0), null),
    assertEqualsString(sqlrelay:getFieldByIndex(6, 0), null),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), null),
    io:format("~n"),

    %% NESTED SELECTS
    %% SKIPPED: the C++ version opens a second cursor (secondcur) inside
    %% the loop and runs a parallel query against the same connection.
    %% The Erlang binding only supports one cursor per process (see
    %% sqlrelay.erl: alloc/cursorFree use a single cursor slot in the
    %% process dictionary), so two concurrent cursors cannot be held.
    %% We still run the outer query to exercise setResultSetBufferSize.
    io:format("NESTED SELECTS: ~n"),
    sqlrelay:setResultSetBufferSize(1),
    assertTrue(sqlrelay:sendQuery("select * from testtable")),
    %% Inner loop with secondcur is omitted.
    sqlrelay:setResultSetBufferSize(0),
    io:format("~n"),

    %% RESET TRANSACTION STATE
    io:format("RESET TRANSACTION STATE: ~n"),
    assertTrue(sqlrelay:commit()),
    assertEqualsString(sqlrelay:getTransactionModel(), "explicit"),
    assertTrue(sqlrelay:getAutoCommit()),
    io:format("~n"),

    %% TRANSACTION BEHAVIOR - implicit/explicit/explicit-deferred/explicit-error/none
    %% SKIPPED: these blocks require a second concurrent connection
    %% (secondcon) and cursor (secondcur) to verify cross-connection
    %% isolation. The Erlang binding only supports one connection per
    %% process (see sqlrelay.erl: alloc/connectionFree use a single
    %% connection slot in the process dictionary), so a second
    %% connection cannot be instantiated here.
    io:format("TRANSACTION BEHAVIOR - implicit/explicit/explicit-deferred/explicit-error/none: ~n"),
    io:format("(skipped - requires second concurrent connection)~n"),
    %% Drop the leftover testtable so subsequent sections start clean.
    assertTrue(sqlrelay:commit()),
    assertTrue(sqlrelay:sendQuery("drop table if exists testtable")),
    io:format("~n"),

    %% RESET TRANSACTION BEHAVIOR
    io:format("RESET TRANSACTION BEHAVIOR: ~n"),
    {ok, DefaultModel} = sqlrelay:getDefaultTransactionModel(),
    assertTrue(sqlrelay:setTransactionModel(DefaultModel)),
    assertEqualsString(sqlrelay:getTransactionModel(), "explicit"),
    assertTrue(sqlrelay:getAutoCommit()),
    io:format("~n"),

    %% INDIVIDUAL SUBSTITUTIONS
    io:format("INDIVIDUAL SUBSTITUTIONS: ~n"),
    sqlrelay:sendQuery("drop table if exists testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 int, "
        "	col2 char, "
        "	col3 float)")),
    sqlrelay:prepareQuery(
        "insert into "
        "	testtable "
        "values ("
        "	$(var1), "
        "	'$(var2)', "
        "	$(var3))"),
    sqlrelay:subLong("var1", 1),
    sqlrelay:subString("var2", "hello"),
    sqlrelay:subDouble("var3", 10.5556, 6, 4),
    assertTrue(sqlrelay:executeQuery()),
    assertTrue(sqlrelay:sendQuery("select * from testtable")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "hello"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "10.5556"),
    assertTrue(sqlrelay:sendQuery("delete from testtable")),
    io:format("~n"),

    %% ARRAY SUBSTITUTIONS (done individually; no array-subst API)
    io:format("ARRAY SUBSTITUTIONS: ~n"),
    sqlrelay:prepareQuery(
        "insert into "
        "	testtable "
        "values ("
        "	'$(var1)', "
        "	'$(var2)', "
        "	'$(var3)')"),
    sqlrelay:subString("var1", "hi"),
    sqlrelay:subString("var2", "hello"),
    sqlrelay:subString("var3", "bye"),
    assertTrue(sqlrelay:executeQuery()),
    assertTrue(sqlrelay:sendQuery("select * from testtable")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "hi"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "hello"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "bye"),
    assertTrue(sqlrelay:sendQuery("delete from testtable")),
    io:format("~n"),
    sqlrelay:prepareQuery(
        "insert into "
        "	testtable "
        "values ("
        "	$(var1), "
        "	'$(var2)', "
        "	$(var3))"),
    sqlrelay:subLong("var1", 1),
    sqlrelay:subLong("var2", 2),
    sqlrelay:subLong("var3", 3),
    assertTrue(sqlrelay:executeQuery()),
    assertTrue(sqlrelay:sendQuery("select * from testtable")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "2"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "3.0"),
    assertTrue(sqlrelay:sendQuery("delete from testtable")),
    io:format("~n"),
    sqlrelay:prepareQuery(
        "insert into "
        "	testtable "
        "values ("
        "	$(var1), "
        "	'$(var2)', "
        "	$(var3))"),
    sqlrelay:subDouble("var1", 10.55, 4, 2),
    sqlrelay:subDouble("var2", 10.556, 5, 3),
    sqlrelay:subDouble("var3", 10.5556, 6, 4),
    assertTrue(sqlrelay:executeQuery()),
    assertTrue(sqlrelay:sendQuery("select * from testtable")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "10.55"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "10.556"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "10.5556"),
    assertTrue(sqlrelay:sendQuery("delete from testtable")),
    io:format("~n"),

    %% NULLS AS NULLS
    io:format("NULLS AS NULLS: ~n"),
    sqlrelay:getNullsAsNulls(),
    assertTrue(sqlrelay:sendQuery(
        "insert into "
        "	testtable "
        "values ("
        "	1, "
        "	NULL, "
        "	NULL)")),
    assertTrue(sqlrelay:sendQuery("select * from testtable")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), null),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), null),
    sqlrelay:getNullsAsEmptyStrings(),
    assertTrue(sqlrelay:sendQuery("select * from testtable")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), ""),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), ""),
    assertTrue(sqlrelay:sendQuery("drop table if exists testtable")),
    io:format("~n"),

    %% NULL AND EMPTY LOBS
    %% the Erlang inputBindClob/inputBindBlob functions require a list
    %% Value, so a true NULL cannot be bound; all four columns are bound
    %% as empty strings and round-trip as empty strings (the true-NULL
    %% LOB case is exercised by the c++ test)
    io:format("NULL AND EMPTY LOBS: ~n"),
    sqlrelay:getNullsAsNulls(),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	testclob1 clob, "
        "	testclob2 clob, "
        "	testblob1 blob, "
        "	testblob2 blob)")),
    sqlrelay:prepareQuery(
        "insert into "
        "	testtable "
        "values ("
        "	:var1, "
        "	:var2, "
        "	:var3, "
        "	:var4)"),
    sqlrelay:inputBindClob("var1", "", 0),
    sqlrelay:inputBindClob("var2", "", 0),
    sqlrelay:inputBindBlob("var3", "", 0),
    sqlrelay:inputBindBlob("var4", "", 0),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("select * from testtable"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), ""),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), ""),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), ""),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 3), ""),
    sqlrelay:getNullsAsEmptyStrings(),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% LONG LOBS
    io:format("LONG LOBS: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    sqlrelay:sendQuery(
        "create table testtable ("
        "	testclob clob, "
        "	testblob blob)"),
    sqlrelay:prepareQuery("insert into testtable values (:clobval,:blobval)"),
    LargeBufferLength = 8192,
    LargeBuf = largeBuffer(LargeBufferLength),
    sqlrelay:inputBindClob("clobval", LargeBuf, LargeBufferLength),
    sqlrelay:inputBindBlob("blobval", LargeBuf, LargeBufferLength),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("select * from testtable"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testclob"),
                    LargeBufferLength),
    assertEqualsString(sqlrelay:getFieldByName(0, "testclob"), LargeBuf),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testblob"),
                    LargeBufferLength),
    assertEqualsStringLen(sqlrelay:getFieldByName(0, "testblob"),
                          LargeBuf, LargeBufferLength),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% OUTPUT BIND BY POSITION
    %% sqlite doesn't support output binds

    %% OUTPUT BIND BY NAME
    %% sqlite doesn't support output binds

    %% OUTPUT BIND BY NAME WITH VALIDATION
    %% sqlite doesn't support output binds

    %% LOB OUTPUT BIND
    %% sqlite doesn't support output binds

    %% LONG OUTPUT BIND
    %% sqlite doesn't support output binds

    %% NEGATIVE INPUT BIND
    io:format("NEGATIVE INPUT BIND: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    sqlrelay:sendQuery("create table testtable (testval int)"),
    sqlrelay:prepareQuery("insert into testtable values (:testval)"),
    sqlrelay:inputBindLong("testval", -1),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("select testval from testtable"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testval"), "-1"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% BIND VALIDATION
    io:format("BIND VALIDATION: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 varchar(20), "
        "	col2 varchar(20), "
        "	col3 varchar(20))"),
    sqlrelay:prepareQuery(
        "insert into "
        "	testtable "
        "values ("
        "	$(var1), "
        "	$(var2), "
        "	$(var3))"),
    sqlrelay:inputBindString("var1", "1"),
    sqlrelay:inputBindString("var2", "2"),
    sqlrelay:inputBindString("var3", "3"),
    sqlrelay:subString("var1", ":var1"),
    assertTrue(sqlrelay:validBind("var1")),
    assertFalse(sqlrelay:validBind("var2")),
    assertFalse(sqlrelay:validBind("var3")),
    assertFalse(sqlrelay:validBind("var4")),
    io:format("~n"),
    sqlrelay:subString("var2", ":var2"),
    assertTrue(sqlrelay:validBind("var1")),
    assertTrue(sqlrelay:validBind("var2")),
    assertFalse(sqlrelay:validBind("var3")),
    assertFalse(sqlrelay:validBind("var4")),
    io:format("~n"),
    sqlrelay:subString("var3", ":var3"),
    assertTrue(sqlrelay:validBind("var1")),
    assertTrue(sqlrelay:validBind("var2")),
    assertTrue(sqlrelay:validBind("var3")),
    assertFalse(sqlrelay:validBind("var4")),
    assertTrue(sqlrelay:executeQuery()),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% REBINDING
    io:format("REBINDING: ~n"),
    sqlrelay:prepareQuery("select :val"),
    sqlrelay:inputBindLong("val", 1),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    sqlrelay:inputBindLong("val", 2),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "2"),
    sqlrelay:inputBindLong("val", 3),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "3"),
    io:format("~n"),

    %% REEXECUTE
    io:format("REEXECUTE: ~n"),
    sqlrelay:prepareQuery("select 1"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:rowCount(), 1),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    io:format("~n"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:rowCount(), 1),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    io:format("~n"),
    sqlrelay:prepareQuery("select :var"),
    sqlrelay:inputBindLong("var", 1),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:rowCount(), 1),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    io:format("~n"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:rowCount(), 1),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    io:format("~n"),
    sqlrelay:inputBindLong("var", 2),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:rowCount(), 1),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "2"),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING NO VALUE
    %% sqlite doesn't support stored procedures

    %% STORED PROCEDURE RETURNING SINGLE VALUE
    %% sqlite doesn't support stored procedures

    %% STORED PROCEDURE RETURNING MULTIPLE VALUES
    %% sqlite doesn't support stored procedures

    %% STORED PROCEDURE RETURNING RESULT SET
    %% sqlite doesn't support stored procedures

    %% TEMPORARY TABLES
    io:format("TEMPORARY TABLES: ~n"),
    sqlrelay:sendQuery("drop table if exists temptable\n"),
    sqlrelay:sendQuery("create temporary table temptable (col1 int)"),
    assertTrue(sqlrelay:sendQuery("insert into temptable values (1)")),
    assertTrue(sqlrelay:sendQuery("select count(*) from temptable")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    sqlrelay:endSession(),
    io:format("~n"),
    assertFalse(sqlrelay:sendQuery("select count(*) from temptable")),
    assertTrue(sqlrelay:sendQuery("drop table if exists temptable\n")),
    io:format("~n"),

    %% ENCODED BINARY DATA
    io:format("ENCODED BINARY DATA: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery("create table testtable (col1 blob)")),
    Buffer = lists:seq(0, 255),
    HexStr = lists:flatten([io_lib:format("~2.16.0b", [B]) || B <- Buffer]),
    QueryStr = "insert into testtable values (X'" ++ HexStr ++ "')",
    assertTrue(sqlrelay:sendQuery(QueryStr)),
    assertTrue(sqlrelay:sendQuery("select col1 from testtable")),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 256),
    {ok, RawBytes} = sqlrelay:getFieldByIndex(0, 0),
    %% bytestring-compare equivalent: compare raw bytes to 0..255
    case RawBytes =:= Buffer of
        true  -> pass();
        false -> fail(RawBytes, Buffer)
    end,
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% QUOTES
    io:format("QUOTES: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery("create table testtable (col1 varchar(4))")),
    assertTrue(sqlrelay:sendQuery("insert into testtable values ('''''')")),
    assertTrue(sqlrelay:sendQuery("select col1 from testtable")),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 2),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "''"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% LAST INSERT ID
    io:format("LAST INSERT ID: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable "
        "	(col1 integer primary key "
        "	autoincrement, "
        "	col2 int)")),
    assertTrue(sqlrelay:sendQuery(
        "insert into testtable values (null,1)")),
    assertEqualsInt(sqlrelay:getLastInsertId(), 1),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% DATABASE IS SCHEMA
    io:format("DATABASE IS SCHEMA: ~n"),
    assertFalse(sqlrelay:getDatabaseIsSchema()),
    io:format("~n"),

    %% CATALOG LIST
    io:format("CATALOG LIST: ~n"),
    assertTrue(sqlrelay:getCatalogList("")),
    assertEqualsString(sqlrelay:getColumnName(0), "Database"),
    io:format("~n"),

    %% SCHEMA LIST
    io:format("SCHEMA LIST: ~n"),
    assertTrue(sqlrelay:getSchemaList("")),
    assertEqualsString(sqlrelay:getColumnName(0), "Database"),
    io:format("~n"),

    %% TABLE TYPE LIST
    io:format("TABLE TYPE LIST: ~n"),
    assertTrue(sqlrelay:getTableTypeList()),
    assertEqualsString(sqlrelay:getColumnName(0), "table_type"),
    assertInResultSet("table_type", "TABLE"),
    io:format("~n"),

    %% TABLE LIST
    io:format("TABLE LIST: ~n"),
    sqlrelay:sendQuery("drop table if exists testtable1"),
    sqlrelay:sendQuery("drop table if exists testtable2"),
    sqlrelay:sendQuery("drop table if exists testtable3"),
    sqlrelay:sendQuery("drop table if exists testtable4"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable1 ("
        "	col1 int, "
        "	col2 int)")),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable2 ("
        "	col1 int, "
        "	col2 int)")),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable3 ("
        "	col1 int, "
        "	col2 int)")),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable4 ("
        "	col1 int, "
        "	col2 int)")),
    assertTrue(sqlrelay:getTableList("")),
    assertInResultSet("Tables_in_xxx", "testtable1"),
    assertInResultSet("Tables_in_xxx", "testtable2"),
    assertInResultSet("Tables_in_xxx", "testtable3"),
    assertInResultSet("Tables_in_xxx", "testtable4"),
    assertTrue(sqlrelay:sendQuery("drop table if exists testtable1")),
    assertTrue(sqlrelay:sendQuery("drop table if exists testtable2")),
    assertTrue(sqlrelay:sendQuery("drop table if exists testtable3")),
    assertTrue(sqlrelay:sendQuery("drop table if exists testtable4")),
    io:format("~n"),

    %% TYPE INFO LIST
    io:format("TYPE INFO LIST: ~n"),
    assertTrue(sqlrelay:getTypeInfoList("integer")),
    assertEqualsString(sqlrelay:getColumnName(0), "type_name"),
    assertEqualsString(sqlrelay:getColumnName(1), "data_type"),
    assertEqualsString(sqlrelay:getColumnName(2), "precision"),
    assertEqualsString(sqlrelay:getColumnName(3), "literal_prefix"),
    assertEqualsString(sqlrelay:getColumnName(4), "literal_suffix"),
    assertEqualsString(sqlrelay:getColumnName(5), "create_params"),
    assertEqualsString(sqlrelay:getColumnName(6), "nullable"),
    assertEqualsString(sqlrelay:getColumnName(7), "case_sensitive"),
    assertEqualsString(sqlrelay:getColumnName(8), "searchable"),
    assertEqualsString(sqlrelay:getColumnName(9), "unsigned_attribute"),
    assertEqualsString(sqlrelay:getColumnName(10), "fixed_prec_scale"),
    assertEqualsString(sqlrelay:getColumnName(11), "auto_increment"),
    assertEqualsString(sqlrelay:getColumnName(12), "local_type_name"),
    assertEqualsString(sqlrelay:getColumnName(13), "minumum_scale"),
    assertEqualsString(sqlrelay:getColumnName(14), "maxiumm_scale"),
    assertEqualsString(sqlrelay:getColumnName(15), "sql_data_type"),
    assertEqualsString(sqlrelay:getColumnName(16), "sql_datetime_sub"),
    assertEqualsString(sqlrelay:getColumnName(17), "num_prec_radix"),
    assertEqualsString(sqlrelay:getColumnName(18), "interval_precision"),
    assertEqualsString(sqlrelay:getFieldByName(0, "type_name"), "INTEGER"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "4"),
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "19"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"), "INTEGER"),
    assertTrue(sqlrelay:getTypeInfoList("char")),
    assertEqualsString(sqlrelay:getFieldByName(0, "type_name"), "CHAR"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "2147483647"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"), "CHAR"),
    assertTrue(sqlrelay:getTypeInfoList("varchar")),
    assertEqualsString(sqlrelay:getFieldByName(0, "type_name"), "VARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "12"),
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "2147483647"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"), "VARCHAR"),
    assertTrue(sqlrelay:getTypeInfoList("date")),
    assertEqualsString(sqlrelay:getFieldByName(0, "type_name"), "DATE"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "91"),
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "10"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"), "DATE"),
    io:format("~n"),

    %% COLUMN LIST
    io:format("COLUMN LIST: ~n"),
    sqlrelay:sendQuery("drop table if exists testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	testint int, "
        "	testfloat float, "
        "	testchar char(40), "
        "	testvarchar varchar(40), "
        "	testclob clob, "
        "	testblob blob)")),
    assertTrue(sqlrelay:getColumnList("testtable", "")),
    assertEqualsString(sqlrelay:getColumnName(0), "column_name"),
    assertEqualsString(sqlrelay:getColumnName(1), "data_type"),
    assertEqualsString(sqlrelay:getColumnName(2), "character_maximum_length"),
    assertEqualsString(sqlrelay:getColumnName(3), "numeric_precision"),
    assertEqualsString(sqlrelay:getColumnName(4), "numeric_scale"),
    assertEqualsString(sqlrelay:getColumnName(5), "is_nullable"),
    assertEqualsString(sqlrelay:getColumnName(6), "column_key"),
    assertEqualsString(sqlrelay:getColumnName(7), "column_default"),
    assertEqualsString(sqlrelay:getColumnName(8), "extra"),
    assertEqualsString(sqlrelay:getFieldByName(0, "column_name"), "testint"),
    assertEqualsString(sqlrelay:getFieldByName(1, "column_name"), "testfloat"),
    assertEqualsString(sqlrelay:getFieldByName(2, "column_name"), "testchar"),
    assertEqualsString(sqlrelay:getFieldByName(3, "column_name"), "testvarchar"),
    assertEqualsString(sqlrelay:getFieldByName(4, "column_name"), "testclob"),
    assertEqualsString(sqlrelay:getFieldByName(5, "column_name"), "testblob"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "INT"),
    assertEqualsString(sqlrelay:getFieldByName(1, "data_type"), "FLOAT"),
    assertEqualsString(sqlrelay:getFieldByName(2, "data_type"), "CHAR"),
    assertEqualsString(sqlrelay:getFieldByName(3, "data_type"), "VARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(4, "data_type"), "CLOB"),
    assertEqualsString(sqlrelay:getFieldByName(5, "data_type"), "BLOB"),
    assertTrue(sqlrelay:sendQuery("drop table if exists testtable")),
    io:format("~n"),

    %% COLUMN LIST - auto_increment, primary key
    io:format("COLUMN LIST - auto_increment, primary key: ~n"),
    sqlrelay:sendQuery("drop table if exists testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 integer primary key autoincrement, "
        "	col2 int)")),
    assertTrue(sqlrelay:getColumnList("testtable", "")),
    {ok, Ex0} = sqlrelay:getFieldByName(0, "extra"),
    assertEqualsString(Ex0, "auto_increment"),
    {ok, Ck0} = sqlrelay:getFieldByName(0, "column_key"),
    assertEqualsString(Ck0, "PRI"),
    {ok, Ex1} = sqlrelay:getFieldByName(1, "extra"),
    assertEqualsString(Ex1, ""),
    {ok, Ck1} = sqlrelay:getFieldByName(1, "column_key"),
    assertEqualsString(Ck1, ""),
    io:format("~n"),
    assertTrue(sqlrelay:sendQuery("drop table if exists testtable")),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 int primary key, "
        "	col2 int)")),
    assertTrue(sqlrelay:getColumnList("testtable", "")),
    {ok, Ex0b} = sqlrelay:getFieldByName(0, "extra"),
    assertEqualsString(Ex0b, ""),
    {ok, Ck0b} = sqlrelay:getFieldByName(0, "column_key"),
    assertEqualsString(Ck0b, "PRI"),
    assertTrue(sqlrelay:sendQuery("drop table if exists testtable")),
    io:format("~n"),

    %% PRIMARY KEYS LIST
    io:format("PRIMARY KEYS LIST: ~n"),
    sqlrelay:sendQuery("drop table if exists testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 int primary key, "
        "	col2 int)")),
    assertTrue(sqlrelay:getPrimaryKeysList("testtable", "")),
    assertEqualsString(sqlrelay:getColumnName(0), "table"),
    assertEqualsString(sqlrelay:getColumnName(1), "non_unique"),
    assertEqualsString(sqlrelay:getColumnName(2), "key_name"),
    assertEqualsString(sqlrelay:getColumnName(3), "seq_in_index"),
    assertEqualsString(sqlrelay:getColumnName(4), "column_name"),
    assertEqualsString(sqlrelay:getColumnName(5), "collation"),
    assertEqualsString(sqlrelay:getColumnName(6), "cardinality"),
    assertEqualsString(sqlrelay:getColumnName(7), "sub_part"),
    assertEqualsString(sqlrelay:getColumnName(8), "packed"),
    assertEqualsString(sqlrelay:getColumnName(9), "null"),
    assertEqualsString(sqlrelay:getColumnName(10), "index_type"),
    assertEqualsString(sqlrelay:getColumnName(11), "comment"),
    assertEqualsString(sqlrelay:getColumnName(12), "index_comment"),
    assertEqualsInt(sqlrelay:rowCount(), 1),
    assertEqualsString(sqlrelay:getFieldByName(0, "table"), "testtable"),
    assertEqualsString(sqlrelay:getFieldByName(0, "seq_in_index"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "column_name"), "col1"),
    assertTrue(sqlrelay:sendQuery("drop table if exists testtable")),
    io:format("~n"),

    %% KEY AND INDEX LIST
    io:format("KEY AND INDEX LIST: ~n"),
    sqlrelay:sendQuery("drop table if exists testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 int primary key, "
        "	col2 int)")),
    assertTrue(sqlrelay:getKeyAndIndexList("testtable", "")),
    assertEqualsString(sqlrelay:getColumnName(0), "table"),
    assertEqualsString(sqlrelay:getColumnName(1), "non_unique"),
    assertEqualsString(sqlrelay:getColumnName(2), "key_name"),
    assertEqualsString(sqlrelay:getColumnName(3), "seq_in_index"),
    assertEqualsString(sqlrelay:getColumnName(4), "column_name"),
    assertEqualsString(sqlrelay:getColumnName(5), "collation"),
    assertEqualsString(sqlrelay:getColumnName(6), "cardinality"),
    assertEqualsString(sqlrelay:getColumnName(7), "sub_part"),
    assertEqualsString(sqlrelay:getColumnName(8), "packed"),
    assertEqualsString(sqlrelay:getColumnName(9), "null"),
    assertEqualsString(sqlrelay:getColumnName(10), "index_type"),
    assertEqualsString(sqlrelay:getColumnName(11), "comment"),
    assertEqualsString(sqlrelay:getColumnName(12), "index_comment"),
    assertEqualsInt(sqlrelay:rowCount(), 1),
    assertEqualsString(sqlrelay:getFieldByName(0, "table"), "testtable"),
    assertEqualsString(sqlrelay:getFieldByName(0, "non_unique"), "0"),
    assertEqualsString(sqlrelay:getFieldByName(0, "seq_in_index"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "column_name"), "col1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "collation"), "A"),
    assertEqualsString(sqlrelay:getFieldByName(0, "index_type"), "3"),
    {ok, KeyName} = sqlrelay:getFieldByName(0, "key_name"),
    assertEqualsString(KeyName, "sqlite_autoindex_testtable_1"),
    assertTrue(sqlrelay:sendQuery("drop table if exists testtable")),
    io:format("~n"),

    %% PROCEDURE LIST
    io:format("PROCEDURE LIST: ~n"),
    assertTrue(sqlrelay:getProcedureList("")),
    assertEqualsInt(sqlrelay:rowCount(), 0),
    io:format("~n"),

    %% PROCEDURE PARAMETER LIST
    io:format("PROCEDURE PARAMETER LIST: ~n"),
    assertTrue(sqlrelay:getProcedureParameterList("testproc1", "")),
    assertEqualsString(sqlrelay:getColumnName(0), "parameter_name"),
    assertEqualsString(sqlrelay:getColumnName(1), "parameter_mode"),
    assertEqualsString(sqlrelay:getColumnName(2), "data_type"),
    assertEqualsString(sqlrelay:getColumnName(3), "character_maximum_length"),
    assertEqualsString(sqlrelay:getColumnName(4), "ordinal_position"),
    assertEqualsInt(sqlrelay:rowCount(), 0),
    io:format("~n"),

    %% INVALID QUERIES
    io:format("INVALID QUERIES: ~n"),
    assertFalse(sqlrelay:sendQuery("select * from testtable")),
    assertFalse(sqlrelay:sendQuery("select * from testtable")),
    assertFalse(sqlrelay:sendQuery("select * from testtable")),
    assertFalse(sqlrelay:sendQuery("select * from testtable")),
    io:format("~n"),
    assertFalse(sqlrelay:sendQuery("insert into testtable values (1,2,3,4)")),
    assertFalse(sqlrelay:sendQuery("insert into testtable values (1,2,3,4)")),
    assertFalse(sqlrelay:sendQuery("insert into testtable values (1,2,3,4)")),
    assertFalse(sqlrelay:sendQuery("insert into testtable values (1,2,3,4)")),
    io:format("~n"),
    assertFalse(sqlrelay:sendQuery("create table testtable")),
    assertFalse(sqlrelay:sendQuery("create table testtable")),
    assertFalse(sqlrelay:sendQuery("create table testtable")),
    assertFalse(sqlrelay:sendQuery("create table testtable")),
    io:format("~n"),

    reportTestStatus(),

    sqlrelay:cursorFree(),
    sqlrelay:connectionFree(),
    sqlrelay:stop(),
    init:stop(getStatus()).
