%% Copyright (c) David Muse
%% See the file COPYING for more information.

-module(postgresql).
-export([main/0]).

-import(asserts, [pass/0, fail/2,
                  getStatus/0, reportTestStatus/0,
                  assertEqualsString/2, assertEqualsStringLen/3,
                  assertEqualsInt/2, assertEqualsDouble/2,
                  assertTrue/1, assertFalse/1,
                  waitForPort/1, largeBuffer/1, shortHostname/0]).

%% Iterate through an isolation-level list, setting each inside its
%% own transaction (postgresql requires the SET to be the first query
%% of the transaction) and verifying that getIsolationLevel agrees.
setIsolationLevels([]) ->
    ok;
setIsolationLevels([Il | Rest]) ->
    sqlrelay:beginTransaction(),
    assertTrue(sqlrelay:setIsolationLevel(Il)),
    assertEqualsString(sqlrelay:getIsolationLevel(), Il),
    sqlrelay:commit(),
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
    assertEqualsString(sqlrelay:identify(), "postgresql"),
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
    assertEqualsString(sqlrelay:bindFormat(), "$1"),
    io:format("~n"),

    %% NEXTVAL FORMAT
    io:format("NEXTVAL FORMAT: ~n"),
    assertEqualsString(sqlrelay:nextvalFormat(), "nextval('%s')"),
    io:format("~n"),

    %% ISOLATION LEVELS
    io:format("ISOLATION LEVELS: ~n"),
    IsolationLevels = ["read committed", "read uncommitted",
                       "repeatable read", "serializable"],
    setIsolationLevels(IsolationLevels),
    %% reset to the default isolation level
    sqlrelay:beginTransaction(),
    assertTrue(sqlrelay:setIsolationLevel(hd(IsolationLevels))),
    sqlrelay:commit(),
    io:format("~n"),

    %% CREATE TESTTABLE
    io:format("CREATE TESTTABLE: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	testint int, "
        "	testfloat float, "
        "	testreal real, "
        "	testsmallint smallint, "
        "	testchar char(40), "
        "	testvarchar varchar(40), "
        "	testdate date, "
        "	testtime time, "
        "	testtimestamp timestamp, "
        "	testtext text, "
        "	testbytea bytea)")),
    io:format("~n"),

    %% INSERT
    io:format("INSERT: ~n"),
    assertTrue(sqlrelay:beginTransaction()),
    assertTrue(sqlrelay:sendQuery(
        "insert into "
        "	testtable "
        "values ("
        "	1, "
        "	1.1, "
        "	1.1, "
        "	1, "
        "	'testchar1', "
        "	'testvarchar1', "
        "	'01/01/2001', "
        "	'01:00:00', "
        "	NULL, "
        "	'testtext1', "
        "	'testbytea1')")),
    assertTrue(sqlrelay:sendQuery(
        "insert into "
        "	testtable "
        "values ("
        "	2, "
        "	2.2, "
        "	2.2, "
        "	2, "
        "	'testchar2', "
        "	'testvarchar2', "
        "	'01/01/2002', "
        "	'02:00:00', "
        "	NULL, "
        "	'testtext2', "
        "	'testbytea2')")),
    assertTrue(sqlrelay:sendQuery(
        "insert into "
        "	testtable "
        "values ("
        "	3, "
        "	3.3, "
        "	3.3, "
        "	3, "
        "	'testchar3', "
        "	'testvarchar3', "
        "	'01/01/2003', "
        "	'03:00:00', "
        "	NULL, "
        "	'testtext3', "
        "	'testbytea3')")),
    assertTrue(sqlrelay:sendQuery(
        "insert into "
        "	testtable "
        "values ("
        "	4, "
        "	4.4, "
        "	4.4, "
        "	4, "
        "	'testchar4', "
        "	'testvarchar4', "
        "	'01/01/2004', "
        "	'04:00:00', "
        "	NULL, "
        "	'testtext4', "
        "	'testbytea4')")),
    io:format("~n"),

    %% AFFECTED ROWS
    io:format("AFFECTED ROWS: ~n"),
    assertEqualsInt(sqlrelay:affectedRows(), 1),
    io:format("~n"),

    %% INPUT BIND BY POSITION
    io:format("INPUT BIND BY POSITION: ~n"),
    sqlrelay:prepareQuery(
        "insert into "
        "	testtable "
        "values ("
        "	$1, "
        "	$2, "
        "	$3, "
        "	$4, "
        "	$5, "
        "	$6, "
        "	$7, "
        "	$8, "
        "	NULL, "
        "	$9, "
        "	$10)"),
    assertEqualsInt(sqlrelay:countBindVariables(), 10),
    sqlrelay:inputBindLong("1", 5),
    sqlrelay:inputBindDouble("2", 5.5, 4, 2),
    sqlrelay:inputBindDouble("3", 5.5, 4, 2),
    sqlrelay:inputBindLong("4", 5),
    sqlrelay:inputBindString("5", "testchar5"),
    sqlrelay:inputBindString("6", "testvarchar5"),
    sqlrelay:inputBindString("7", "01/01/2005"),
    sqlrelay:inputBindString("8", "05:00:00"),
    sqlrelay:inputBindClob("9", "testtext5", 9),
    sqlrelay:inputBindBlob("10", "testbytea5", 10),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 6),
    sqlrelay:inputBindDouble("2", 6.6, 4, 2),
    sqlrelay:inputBindDouble("3", 6.6, 4, 2),
    sqlrelay:inputBindLong("4", 6),
    sqlrelay:inputBindString("5", "testchar6"),
    sqlrelay:inputBindString("6", "testvarchar6"),
    sqlrelay:inputBindString("7", "01/01/2006"),
    sqlrelay:inputBindString("8", "06:00:00"),
    sqlrelay:inputBindClob("9", "testtext6", 9),
    sqlrelay:inputBindBlob("10", "testbytea6", 10),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 7),
    sqlrelay:inputBindDouble("2", 7.7, 4, 2),
    sqlrelay:inputBindDouble("3", 7.7, 4, 2),
    sqlrelay:inputBindLong("4", 7),
    sqlrelay:inputBindString("5", "testchar7"),
    sqlrelay:inputBindString("6", "testvarchar7"),
    sqlrelay:inputBindString("7", "01/01/2007"),
    sqlrelay:inputBindString("8", "07:00:00"),
    sqlrelay:inputBindClob("9", "testtext7", 9),
    sqlrelay:inputBindBlob("10", "testbytea8", 10),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% ARRAY OF INPUT BINDS BY POSITION
    %% postgresql doesn't support implicit conversion of string binds to
    %% other data types, so arrays of binds don't generally work.

    %% INPUT BIND BY NAME
    %% postgresql doesn't support bind by name

    %% INPUT BIND BY POSITION WITH VALIDATION
    io:format("BIND BY POSITION WITH VALIDATION: ~n"),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 8),
    sqlrelay:inputBindDouble("2", 8.8, 4, 2),
    sqlrelay:inputBindDouble("3", 8.8, 4, 2),
    sqlrelay:inputBindLong("4", 8),
    sqlrelay:inputBindString("5", "testchar8"),
    sqlrelay:inputBindString("6", "testvarchar8"),
    sqlrelay:inputBindString("7", "01/01/2008"),
    sqlrelay:inputBindString("8", "08:00:00"),
    sqlrelay:inputBindClob("9", "testtext8", 9),
    sqlrelay:inputBindClob("10", "testbytea8", 10),
    sqlrelay:validateBinds(),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% ARRAY OF INPUT BINDS BY NAME
    %% postgresql doesn't support bind by name

    %% INPUT BIND BY NAME WITH VALIDATION
    %% postgresql doesn't support bind by name

    %% SELECT
    io:format("SELECT: ~n"),
    assertTrue(sqlrelay:sendQuery("select * from testtable order by testint")),
    io:format("~n"),

    %% COLUMN COUNT
    io:format("COLUMN COUNT: ~n"),
    assertEqualsInt(sqlrelay:colCount(), 11),
    io:format("~n"),

    %% COLUMN NAMES
    io:format("COLUMN NAMES: ~n"),
    assertEqualsString(sqlrelay:getColumnName(0), "testint"),
    assertEqualsString(sqlrelay:getColumnName(1), "testfloat"),
    assertEqualsString(sqlrelay:getColumnName(2), "testreal"),
    assertEqualsString(sqlrelay:getColumnName(3), "testsmallint"),
    assertEqualsString(sqlrelay:getColumnName(4), "testchar"),
    assertEqualsString(sqlrelay:getColumnName(5), "testvarchar"),
    assertEqualsString(sqlrelay:getColumnName(6), "testdate"),
    assertEqualsString(sqlrelay:getColumnName(7), "testtime"),
    assertEqualsString(sqlrelay:getColumnName(8), "testtimestamp"),
    assertEqualsString(sqlrelay:getColumnName(9), "testtext"),
    assertEqualsString(sqlrelay:getColumnName(10), "testbytea"),
    {ok, Cols1} = sqlrelay:getColumnNames(),
    assertEqualsString(lists:nth(1, Cols1), "testint"),
    assertEqualsString(lists:nth(2, Cols1), "testfloat"),
    assertEqualsString(lists:nth(3, Cols1), "testreal"),
    assertEqualsString(lists:nth(4, Cols1), "testsmallint"),
    assertEqualsString(lists:nth(5, Cols1), "testchar"),
    assertEqualsString(lists:nth(6, Cols1), "testvarchar"),
    assertEqualsString(lists:nth(7, Cols1), "testdate"),
    assertEqualsString(lists:nth(8, Cols1), "testtime"),
    assertEqualsString(lists:nth(9, Cols1), "testtimestamp"),
    assertEqualsString(lists:nth(10, Cols1), "testtext"),
    assertEqualsString(lists:nth(11, Cols1), "testbytea"),
    io:format("~n"),

    %% COLUMN TYPES
    io:format("COLUMN TYPES: ~n"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(0), "int4"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testint"), "int4"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(1), "float8"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testfloat"), "float8"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(2), "float4"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testreal"), "float4"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(3), "int2"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testsmallint"), "int2"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(4), "bpchar"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testchar"), "bpchar"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(5), "varchar"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testvarchar"), "varchar"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(6), "date"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testdate"), "date"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(7), "time"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testtime"), "time"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(8), "timestamp"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testtimestamp"),
                       "timestamp"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(9), "text"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testtext"), "text"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(10), "bytea"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testbytea"), "bytea"),
    io:format("~n"),

    %% COLUMN LENGTH
    io:format("COLUMN LENGTH: ~n"),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testint"), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(1), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testfloat"), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(2), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testreal"), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(3), 2),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testsmallint"), 2),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(4), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testchar"), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(5), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testvarchar"), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(6), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testdate"), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(7), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testtime"), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(8), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testtimestamp"), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(9), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testtext"), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(10), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testbytea"), 0),
    io:format("~n"),

    %% LONGEST COLUMN
    io:format("LONGEST COLUMN: ~n"),
    assertEqualsInt(sqlrelay:getLongestByIndex(0), 1),
    assertEqualsInt(sqlrelay:getLongestByName("testint"), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(1), 3),
    assertEqualsInt(sqlrelay:getLongestByName("testfloat"), 3),
    assertEqualsInt(sqlrelay:getLongestByIndex(2), 3),
    assertEqualsInt(sqlrelay:getLongestByName("testreal"), 3),
    assertEqualsInt(sqlrelay:getLongestByIndex(3), 1),
    assertEqualsInt(sqlrelay:getLongestByName("testsmallint"), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(4), 40),
    assertEqualsInt(sqlrelay:getLongestByName("testchar"), 40),
    assertEqualsInt(sqlrelay:getLongestByIndex(5), 12),
    assertEqualsInt(sqlrelay:getLongestByName("testvarchar"), 12),
    assertEqualsInt(sqlrelay:getLongestByIndex(6), 10),
    assertEqualsInt(sqlrelay:getLongestByName("testdate"), 10),
    assertEqualsInt(sqlrelay:getLongestByIndex(7), 8),
    assertEqualsInt(sqlrelay:getLongestByName("testtime"), 8),
    assertEqualsInt(sqlrelay:getLongestByIndex(9), 9),
    assertEqualsInt(sqlrelay:getLongestByName("testtext"), 9),
    assertEqualsInt(sqlrelay:getLongestByIndex(10), 10),
    assertEqualsInt(sqlrelay:getLongestByName("testbytea"), 10),
    io:format("~n"),

    %% ROW COUNT
    io:format("ROW COUNT: ~n"),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    io:format("~n"),

    %% TOTAL ROWS
    io:format("TOTAL ROWS: ~n"),
    assertEqualsInt(sqlrelay:totalRows(), 8),
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
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "1.1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "1.1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 3), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 4),
                       "testchar1                               "),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 5), "testvarchar1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 6), "2001-01-01"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 7), "01:00:00"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 9), "testtext1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 10), "testbytea1"),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8.8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 2), "8.8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 3), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 4),
                       "testchar8                               "),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 5), "testvarchar8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 6), "2008-01-01"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 7), "08:00:00"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 9), "testtext8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 10), "testbytea8"),
    io:format("~n"),

    %% FIELD LENGTHS BY INDEX
    io:format("FIELD LENGTHS BY INDEX: ~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 1), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 2), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 3), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 4), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 5), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 6), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 7), 8),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 9), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 10), 10),
    io:format("~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 0), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 1), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 2), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 3), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 4), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 5), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 6), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 7), 8),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 9), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 10), 10),
    io:format("~n"),

    %% FIELDS BY NAME
    io:format("FIELDS BY NAME: ~n"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testint"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testfloat"), "1.1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testreal"), "1.1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testsmallint"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testchar"),
                       "testchar1                               "),
    assertEqualsString(sqlrelay:getFieldByName(0, "testvarchar"),
                       "testvarchar1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testdate"), "2001-01-01"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testtime"), "01:00:00"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testtext"), "testtext1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testbytea"), "testbytea1"),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testint"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testfloat"), "8.8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testreal"), "8.8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testsmallint"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testchar"),
                       "testchar8                               "),
    assertEqualsString(sqlrelay:getFieldByName(7, "testvarchar"),
                       "testvarchar8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testdate"), "2008-01-01"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testtime"), "08:00:00"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testtext"), "testtext8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testbytea"), "testbytea8"),
    io:format("~n"),

    %% FIELD LENGTHS BY NAME
    io:format("FIELD LENGTHS BY NAME: ~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testfloat"), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testreal"), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testsmallint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testchar"), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testvarchar"), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testdate"), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testtime"), 8),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testtext"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testbytea"), 10),
    io:format("~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testfloat"), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testreal"), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testsmallint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testchar"), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testvarchar"), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testdate"), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testtime"), 8),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testtext"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testbytea"), 10),
    io:format("~n"),

    %% FIELDS BY ARRAY
    io:format("FIELDS BY ARRAY: ~n"),
    {ok, Row0} = sqlrelay:getRow(0),
    assertEqualsString(lists:nth(1, Row0), "1"),
    assertEqualsString(lists:nth(2, Row0), "1.1"),
    assertEqualsString(lists:nth(3, Row0), "1.1"),
    assertEqualsString(lists:nth(4, Row0), "1"),
    assertEqualsString(lists:nth(5, Row0),
                       "testchar1                               "),
    assertEqualsString(lists:nth(6, Row0), "testvarchar1"),
    assertEqualsString(lists:nth(7, Row0), "2001-01-01"),
    assertEqualsString(lists:nth(8, Row0), "01:00:00"),
    assertEqualsString(lists:nth(10, Row0), "testtext1"),
    assertEqualsString(lists:nth(11, Row0), "testbytea1"),
    io:format("~n"),

    %% FIELD LENGTHS BY ARRAY
    io:format("FIELD LENGTHS BY ARRAY: ~n"),
    {ok, Rowlens0} = sqlrelay:getRowLengths(0),
    assertEqualsInt(lists:nth(1, Rowlens0), 1),
    assertEqualsInt(lists:nth(2, Rowlens0), 3),
    assertEqualsInt(lists:nth(3, Rowlens0), 3),
    assertEqualsInt(lists:nth(4, Rowlens0), 1),
    assertEqualsInt(lists:nth(5, Rowlens0), 40),
    assertEqualsInt(lists:nth(6, Rowlens0), 12),
    assertEqualsInt(lists:nth(7, Rowlens0), 10),
    assertEqualsInt(lists:nth(8, Rowlens0), 8),
    assertEqualsInt(lists:nth(10, Rowlens0), 9),
    assertEqualsInt(lists:nth(11, Rowlens0), 10),
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
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 4),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(0), "int4"),
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
    assertTrue(sqlrelay:sendQuery("select * from testtable order by testint")),
    sqlrelay:suspendResultSet(),
    assertTrue(sqlrelay:suspendSession()),
    {ok, Port2} = sqlrelay:getConnectionPort(),
    {ok, Socket2} = sqlrelay:getConnectionSocket(),
    assertTrue(sqlrelay:resumeSession(Port2, Socket2)),
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
    assertTrue(sqlrelay:sendQuery("select * from testtable order by testint")),
    sqlrelay:suspendResultSet(),
    assertTrue(sqlrelay:suspendSession()),
    {ok, Port3} = sqlrelay:getConnectionPort(),
    {ok, Socket3} = sqlrelay:getConnectionSocket(),
    assertTrue(sqlrelay:resumeSession(Port3, Socket3)),
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
    {ok, Port4} = sqlrelay:getConnectionPort(),
    {ok, Socket4} = sqlrelay:getConnectionSocket(),
    assertTrue(sqlrelay:resumeSession(Port4, Socket4)),
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
    assertEqualsInt(sqlrelay:colCount(), 11),
    io:format("~n"),

    %% COLUMN NAMES FOR CACHED RESULT SET
    io:format("COLUMN NAMES FOR CACHED RESULT SET: ~n"),
    assertEqualsString(sqlrelay:getColumnName(0), "testint"),
    assertEqualsString(sqlrelay:getColumnName(1), "testfloat"),
    assertEqualsString(sqlrelay:getColumnName(2), "testreal"),
    assertEqualsString(sqlrelay:getColumnName(3), "testsmallint"),
    assertEqualsString(sqlrelay:getColumnName(4), "testchar"),
    assertEqualsString(sqlrelay:getColumnName(5), "testvarchar"),
    assertEqualsString(sqlrelay:getColumnName(6), "testdate"),
    assertEqualsString(sqlrelay:getColumnName(7), "testtime"),
    assertEqualsString(sqlrelay:getColumnName(8), "testtimestamp"),
    {ok, Cols2} = sqlrelay:getColumnNames(),
    assertEqualsString(lists:nth(1, Cols2), "testint"),
    assertEqualsString(lists:nth(2, Cols2), "testfloat"),
    assertEqualsString(lists:nth(3, Cols2), "testreal"),
    assertEqualsString(lists:nth(4, Cols2), "testsmallint"),
    assertEqualsString(lists:nth(5, Cols2), "testchar"),
    assertEqualsString(lists:nth(6, Cols2), "testvarchar"),
    assertEqualsString(lists:nth(7, Cols2), "testdate"),
    assertEqualsString(lists:nth(8, Cols2), "testtime"),
    assertEqualsString(lists:nth(9, Cols2), "testtimestamp"),
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
    {ok, Port5} = sqlrelay:getConnectionPort(),
    {ok, Socket5} = sqlrelay:getConnectionSocket(),
    io:format("~n"),
    assertTrue(sqlrelay:resumeSession(Port5, Socket5)),
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
    assertTrue(sqlrelay:sendQuery("select * from testtable order by testint")),
    assertEqualsString(sqlrelay:getFieldByIndex(4, 0), "5"),
    assertEqualsString(sqlrelay:getFieldByIndex(5, 0), "6"),
    assertEqualsString(sqlrelay:getFieldByIndex(6, 0), "7"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    {ok, Id3} = sqlrelay:getResultSetId(),
    sqlrelay:suspendResultSet(),
    assertTrue(sqlrelay:suspendSession()),
    {ok, Port6} = sqlrelay:getConnectionPort(),
    {ok, Socket6} = sqlrelay:getConnectionSocket(),
    assertTrue(sqlrelay:resumeSession(Port6, Socket6)),
    assertTrue(sqlrelay:resumeResultSet(Id3)),
    assertEqualsString(sqlrelay:getFieldByIndex(4, 0), null),
    assertEqualsString(sqlrelay:getFieldByIndex(5, 0), null),
    assertEqualsString(sqlrelay:getFieldByIndex(6, 0), null),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), null),
    io:format("~n"),

    %% NESTED SELECTS
    %% SKIPPED: the C++ version opens a second cursor (secondcur) inside
    %% the loop and runs a parallel query against the same connection.
    %% The Erlang binding only supports one cursor per process, so two
    %% concurrent cursors cannot be held. We still run the outer query
    %% to exercise setResultSetBufferSize.
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
    %% process, so a second connection cannot be instantiated here.
    io:format("TRANSACTION BEHAVIOR - implicit/explicit/explicit-deferred/explicit-error/none: ~n"),
    io:format("(skipped - requires second concurrent connection)~n"),
    %% Keep postgresql out of an aborted/open transaction and drop the
    %% leftover testtable so subsequent sections start clean.
    sqlrelay:commit(),
    sqlrelay:beginTransaction(),
    sqlrelay:rollback(),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
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
    sqlrelay:prepareQuery("select $(var1),'$(var2)',$(var3)"),
    sqlrelay:subLong("var1", 1),
    sqlrelay:subString("var2", "hello"),
    sqlrelay:subDouble("var3", 10.5556, 6, 4),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "hello"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "10.5556"),
    io:format("~n"),

    %% ARRAY SUBSTITUTIONS (done individually; no array-subst API)
    io:format("ARRAY SUBSTITUTIONS: ~n"),
    sqlrelay:prepareQuery("select $(var1),$(var2),$(var3)"),
    sqlrelay:subLong("var1", 1),
    sqlrelay:subLong("var2", 2),
    sqlrelay:subLong("var3", 3),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "2"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "3"),
    io:format("~n"),
    sqlrelay:prepareQuery("select '$(var1)','$(var2)','$(var3)'"),
    sqlrelay:subString("var1", "hi"),
    sqlrelay:subString("var2", "hello"),
    sqlrelay:subString("var3", "bye"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "hi"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "hello"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "bye"),
    io:format("~n"),
    sqlrelay:prepareQuery("select $(var1),$(var2),$(var3)"),
    sqlrelay:subDouble("var1", 10.55, 4, 2),
    sqlrelay:subDouble("var2", 10.556, 5, 3),
    sqlrelay:subDouble("var3", 10.5556, 6, 4),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "10.55"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "10.556"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "10.5556"),
    io:format("~n"),

    %% NULLS AS NULLS
    io:format("NULLS AS NULLS: ~n"),
    sqlrelay:getNullsAsNulls(),
    assertTrue(sqlrelay:sendQuery("select NULL,1,NULL")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), null),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), null),
    sqlrelay:getNullsAsEmptyStrings(),
    assertTrue(sqlrelay:sendQuery("select NULL,1,NULL")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), ""),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), ""),
    io:format("~n"),

    %% NULL AND EMPTY LOBS
    io:format("NULL AND EMPTY LOBS: ~n"),
    sqlrelay:getNullsAsNulls(),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	testclob1 text, "
        "	testclob2 text, "
        "	testblob1 bytea, "
        "	testblob2 bytea)")),
    sqlrelay:prepareQuery(
        "insert into "
        "	testtable "
        "values ("
        "	$1, "
        "	$2, "
        "	$3, "
        "	$4)"),
    sqlrelay:inputBindClob("1", "", 0),
    sqlrelay:inputBindNull("2"),
    sqlrelay:inputBindBlob("3", "", 0),
    sqlrelay:inputBindNull("4"),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("select * from testtable"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), ""),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), null),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), ""),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 3), null),
    sqlrelay:getNullsAsEmptyStrings(),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% LONG LOBS
    io:format("LONG LOBS: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    sqlrelay:sendQuery(
        "create table testtable ("
        "	testtext text, "
        "	testbytea bytea)"),
    sqlrelay:prepareQuery("insert into testtable values ($1,$2)"),
    LargeBufferLength = 8192,
    LargeBuf = largeBuffer(LargeBufferLength),
    sqlrelay:inputBindClob("1", LargeBuf, LargeBufferLength),
    sqlrelay:inputBindBlob("2", LargeBuf, LargeBufferLength),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("select * from testtable"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testtext"),
                    LargeBufferLength),
    assertEqualsString(sqlrelay:getFieldByName(0, "testtext"), LargeBuf),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testbytea"),
                    LargeBufferLength),
    assertEqualsStringLen(sqlrelay:getFieldByName(0, "testbytea"),
                          LargeBuf, LargeBufferLength),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% OUTPUT BIND BY POSITION
    %% postgresql doesn't support output binds

    %% OUTPUT BIND BY NAME
    %% postgresql doesn't support output binds

    %% OUTPUT BIND BY NAME WITH VALIDATION
    %% postgresql doesn't support output binds

    %% LOB OUTPUT BIND
    %% postgresql doesn't support output binds

    %% LONG OUTPUT BIND
    %% postgresql doesn't support output binds

    %% NEGATIVE INPUT BIND
    io:format("NEGATIVE INPUT BIND: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    sqlrelay:sendQuery("create table testtable (testval int)"),
    sqlrelay:prepareQuery("insert into testtable values ($1)"),
    sqlrelay:inputBindLong("1", -1),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("select testval from testtable"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testval"), "-1"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% BIND VALIDATION
    %% postgresql doesn't support bind by name

    %% REBINDING
    io:format("REBINDING: ~n"),
    sqlrelay:sendQuery("drop function testfunc(int)"),
    assertTrue(sqlrelay:sendQuery(
        "create function testfunc(int) returns int as "
        "	' begin return $1; end;' language plpgsql")),
    sqlrelay:prepareQuery("select * from testfunc($1)"),
    sqlrelay:inputBindLong("1", 1),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    sqlrelay:inputBindLong("1", 2),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "2"),
    sqlrelay:inputBindLong("1", 3),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "3"),
    assertTrue(sqlrelay:sendQuery("drop function testfunc(int)")),
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
    sqlrelay:prepareQuery("select $1::int"),
    sqlrelay:inputBindLong("1", 1),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:rowCount(), 1),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    io:format("~n"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:rowCount(), 1),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    io:format("~n"),
    sqlrelay:inputBindLong("1", 2),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:rowCount(), 1),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "2"),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING NO VALUE
    io:format("STORED PROCEDURE RETURNING NO VALUE: ~n"),
    sqlrelay:sendQuery("drop function testfunc(int,float,char(20))"),
    assertTrue(sqlrelay:sendQuery(
        "create function testfunc("
        "	int,float,char(20)) "
        "returns void as ' "
        "	declare in1 int; "
        "	in2 float; "
        "	in3 char(20); "
        "begin "
        "	in1:=$1; "
        "	in2:=$2; "
        "	in3:=$3; "
        "	return; "
        "end;' language plpgsql")),
    sqlrelay:prepareQuery("select testfunc($1,$2,$3)"),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:inputBindDouble("2", 1.1, 4, 2),
    sqlrelay:inputBindString("3", "hello"),
    assertTrue(sqlrelay:executeQuery()),
    assertTrue(sqlrelay:sendQuery(
        "drop function testfunc(int,float,char(20))")),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING SINGLE VALUE
    io:format("STORED PROCEDURE RETURNING SINGLE VALUE: ~n"),
    sqlrelay:sendQuery("drop function testfunc(int,float,char(20))"),
    assertTrue(sqlrelay:sendQuery(
        "create function testfunc(int,float,char(20)) returns int as "
        "	' begin return $1; end;' language plpgsql")),
    sqlrelay:prepareQuery("select * from testfunc($1,$2,$3)"),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:inputBindDouble("2", 1.1, 4, 2),
    sqlrelay:inputBindString("3", "hello"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertTrue(sqlrelay:sendQuery(
        "drop function testfunc(int,float,char(20))")),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING MULTIPLE VALUES
    io:format("STORED PROCEDURE RETURNING MULTIPLE VALUES: ~n"),
    sqlrelay:sendQuery("drop function testfunc(int,float,char(20))"),
    assertTrue(sqlrelay:sendQuery(
        "create function testfunc("
        "	int,float,char(20)) "
        "returns record as ' "
        "	declare output record; "
        "begin "
        "	select $1,$2,$3 into output; "
        "	return output; "
        "end;' language plpgsql")),
    sqlrelay:prepareQuery(
        "select "
        "	* "
        "from "
        "	testfunc($1,$2,$3) "
        "	as (col1 int, "
        "		col2 float, "
        "		col3 bpchar) "),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:inputBindDouble("2", 1.1, 4, 2),
    sqlrelay:inputBindString("3", "hello"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "hello"),
    assertTrue(sqlrelay:sendQuery(
        "drop function testfunc(int,float,char(20))")),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING RESULT SET
    io:format("STORED PROCEDURE RETURNING RESULT SET: ~n"),
    sqlrelay:sendQuery("drop function testfunc()"),
    assertTrue(sqlrelay:sendQuery(
        "create function testfunc() "
        "returns setof record as ' "
        "	declare output record; "
        "begin "
        "	for output in "
        "		select 1 "
        "		union "
        "		select 2 "
        "		union "
        "		select 3 "
        "		union "
        "		select 4 "
        "		union "
        "		select 5 "
        "		union "
        "		select 6 "
        "		union "
        "		select 7 "
        "		union "
        "		select 8 "
        "	loop "
        "		return next output; "
        "	end loop; "
        "	return; "
        "end;' language plpgsql")),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testfunc() "
        "	as (testint int)")),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    assertTrue(sqlrelay:sendQuery("drop function testfunc()")),
    io:format("~n"),

    %% TEMPORARY TABLES
    io:format("TEMPORARY TABLES: ~n"),
    sqlrelay:sendQuery("drop table temptable\n"),
    sqlrelay:sendQuery("create temporary table temptable (col1 int)"),
    assertTrue(sqlrelay:sendQuery("insert into temptable values (1)")),
    assertTrue(sqlrelay:sendQuery("select count(*) from temptable")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    sqlrelay:endSession(),
    io:format("~n"),
    assertFalse(sqlrelay:sendQuery("select count(*) from temptable")),
    io:format("~n"),

    %% ENCODED BINARY DATA
    io:format("ENCODED BINARY DATA: ~n"),
    %% The prior endSession leaves server-side prepared statements that
    %% the sqlrelay cursor believes it still owns. Free the cursor so a
    %% fresh one is allocated for the following queries.
    sqlrelay:cursorFree(),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery("create table testtable (col1 bytea)")),
    Buffer = lists:seq(0, 255),
    HexStr = lists:flatten([io_lib:format("~2.16.0b", [B]) || B <- Buffer]),
    QueryStr = "insert into testtable values (decode('" ++ HexStr ++
               "','hex'))",
    assertTrue(sqlrelay:sendQuery(QueryStr)),
    assertTrue(sqlrelay:sendQuery("select col1 from testtable")),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 256),
    {ok, RawBytes} = sqlrelay:getFieldByIndex(0, 0),
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
        "	(col1 serial primary key, "
        "	col2 int)")),
    assertTrue(sqlrelay:sendQuery(
        "insert into testtable (col2) values (1)")),
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
    {ok, CatalogRowCount} = sqlrelay:rowCount(),
    assertTrue(CatalogRowCount > 0),
    io:format("~n"),

    %% SCHEMA LIST
    io:format("SCHEMA LIST: ~n"),
    assertTrue(sqlrelay:getSchemaList("")),
    assertEqualsString(sqlrelay:getColumnName(0), "Database"),
    {ok, SchemaRowCount} = sqlrelay:rowCount(),
    assertTrue(SchemaRowCount > 0),
    io:format("~n"),

    %% TABLE TYPE LIST
    io:format("TABLE TYPE LIST: ~n"),
    assertTrue(sqlrelay:getTableTypeList()),
    assertEqualsString(sqlrelay:getColumnName(0), "table_type"),
    {ok, TableTypeRowCount} = sqlrelay:rowCount(),
    TableTypeFound = searchTableTypeList(0, TableTypeRowCount),
    assertTrue(TableTypeFound),
    io:format("~n"),

    %% TABLE LIST
    io:format("TABLE LIST: ~n"),
    sqlrelay:sendQuery("drop table testtable1"),
    sqlrelay:sendQuery("drop table testtable2"),
    sqlrelay:sendQuery("drop table testtable3"),
    sqlrelay:sendQuery("drop table testtable4"),
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
    {ok, TableListRowCount} = sqlrelay:rowCount(),
    TableCount = countMatchingTables(0, TableListRowCount, 0),
    assertEqualsInt(TableCount, 4),
    assertTrue(sqlrelay:sendQuery("drop table testtable1")),
    assertTrue(sqlrelay:sendQuery("drop table testtable2")),
    assertTrue(sqlrelay:sendQuery("drop table testtable3")),
    assertTrue(sqlrelay:sendQuery("drop table testtable4")),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "10"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"),
                       "INTEGER"),
    assertTrue(sqlrelay:getTypeInfoList("char")),
    assertEqualsString(sqlrelay:getFieldByName(0, "type_name"), "CHAR"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "255"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"), "CHAR"),
    assertTrue(sqlrelay:getTypeInfoList("varchar")),
    assertEqualsString(sqlrelay:getFieldByName(0, "type_name"), "VARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "12"),
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "255"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"),
                       "VARCHAR"),
    assertTrue(sqlrelay:getTypeInfoList("date")),
    assertEqualsString(sqlrelay:getFieldByName(0, "type_name"), "DATE"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "91"),
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "10"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"), "DATE"),
    io:format("~n"),

    %% COLUMN LIST
    io:format("COLUMN LIST: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	testint int, "
        "	testfloat float, "
        "	testreal real, "
        "	testsmallint smallint, "
        "	testchar char(40), "
        "	testvarchar varchar(40), "
        "	testdate date, "
        "	testtime time, "
        "	testtimestamp timestamp, "
        "	testtext text, "
        "	testbytea bytea)")),
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
    assertEqualsString(sqlrelay:getFieldByName(1, "column_name"),
                       "testfloat"),
    assertEqualsString(sqlrelay:getFieldByName(2, "column_name"), "testreal"),
    assertEqualsString(sqlrelay:getFieldByName(3, "column_name"),
                       "testsmallint"),
    assertEqualsString(sqlrelay:getFieldByName(4, "column_name"), "testchar"),
    assertEqualsString(sqlrelay:getFieldByName(5, "column_name"),
                       "testvarchar"),
    assertEqualsString(sqlrelay:getFieldByName(6, "column_name"), "testdate"),
    assertEqualsString(sqlrelay:getFieldByName(7, "column_name"), "testtime"),
    assertEqualsString(sqlrelay:getFieldByName(8, "column_name"),
                       "testtimestamp"),
    assertEqualsString(sqlrelay:getFieldByName(9, "column_name"), "testtext"),
    assertEqualsString(sqlrelay:getFieldByName(10, "column_name"),
                       "testbytea"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "integer"),
    assertEqualsString(sqlrelay:getFieldByName(1, "data_type"),
                       "double precision"),
    assertEqualsString(sqlrelay:getFieldByName(2, "data_type"), "real"),
    assertEqualsString(sqlrelay:getFieldByName(3, "data_type"), "smallint"),
    assertEqualsString(sqlrelay:getFieldByName(4, "data_type"), "character"),
    assertEqualsString(sqlrelay:getFieldByName(5, "data_type"),
                       "character varying"),
    assertEqualsString(sqlrelay:getFieldByName(6, "data_type"), "date"),
    assertEqualsString(sqlrelay:getFieldByName(7, "data_type"),
                       "time without time zone"),
    assertEqualsString(sqlrelay:getFieldByName(8, "data_type"),
                       "timestamp without time zone"),
    assertEqualsString(sqlrelay:getFieldByName(9, "data_type"), "text"),
    assertEqualsString(sqlrelay:getFieldByName(10, "data_type"), "bytea"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% COLUMN LIST - auto_increment, primary key
    io:format("COLUMN LIST - auto_increment, primary key: ~n"),
    sqlrelay:sendQuery("drop table if exists testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 serial primary key, "
        "	col2 int)")),
    assertTrue(sqlrelay:getColumnList("testtable", "")),
    {ok, Extra0} = sqlrelay:getFieldByName(0, "extra"),
    assertTrue(contains(Extra0, "auto_increment")),
    {ok, Ck0} = sqlrelay:getFieldByName(0, "column_key"),
    assertTrue(contains(Ck0, "PRI")),
    {ok, Extra1} = sqlrelay:getFieldByName(1, "extra"),
    assertFalse(contains(Extra1, "auto_increment")),
    {ok, Ck1} = sqlrelay:getFieldByName(1, "column_key"),
    assertFalse(contains(Ck1, "PRI")),
    io:format("~n"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 int primary key, "
        "	col2 int)")),
    assertTrue(sqlrelay:getColumnList("testtable", "")),
    {ok, Extra2} = sqlrelay:getFieldByName(0, "extra"),
    assertFalse(contains(Extra2, "auto_increment")),
    {ok, Ck2} = sqlrelay:getFieldByName(0, "column_key"),
    assertTrue(contains(Ck2, "PRI")),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% PRIMARY KEYS LIST
    io:format("PRIMARY KEYS LIST: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
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
    {ok, PkName} = sqlrelay:getFieldByName(0, "key_name"),
    assertTrue(not isNullOrEmpty(PkName)),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% KEY AND INDEX LIST
    io:format("KEY AND INDEX LIST: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "non_unique"), "f"),
    assertEqualsString(sqlrelay:getFieldByName(0, "seq_in_index"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "column_name"), "col1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "collation"), "A"),
    assertEqualsString(sqlrelay:getFieldByName(0, "index_type"), "3"),
    {ok, KeyName2} = sqlrelay:getFieldByName(0, "key_name"),
    assertTrue(not isNullOrEmpty(KeyName2)),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% PROCEDURE LIST
    io:format("PROCEDURE LIST: ~n"),
    sqlrelay:sendQuery("drop function testproc1(int,char,varchar,date)"),
    sqlrelay:sendQuery("drop function testproc2(int,char,varchar,date)"),
    sqlrelay:sendQuery("drop function testproc3(int,char,varchar,date)"),
    sqlrelay:sendQuery("drop function testproc4(int,char,varchar,date)"),
    assertTrue(sqlrelay:sendQuery(
        "create function testproc1("
        "	in1 int, "
        "	in2 char(20), "
        "	in3 varchar(20), "
        "	in4 date) "
        "returns void "
        "as 'begin end;' "
        "language plpgsql")),
    assertTrue(sqlrelay:sendQuery(
        "create function testproc2("
        "	in1 int, "
        "	in2 char(20), "
        "	in3 varchar(20), "
        "	in4 date) "
        "returns void "
        "as 'begin end;' "
        "language plpgsql")),
    assertTrue(sqlrelay:sendQuery(
        "create function testproc3("
        "	in1 int, "
        "	in2 char(20), "
        "	in3 varchar(20), "
        "	in4 date) "
        "returns void "
        "as 'begin end;' "
        "language plpgsql")),
    assertTrue(sqlrelay:sendQuery(
        "create function testproc4("
        "	in1 int, "
        "	in2 char(20), "
        "	in3 varchar(20), "
        "	in4 date) "
        "returns void "
        "as 'begin end;' "
        "language plpgsql")),
    assertTrue(sqlrelay:getProcedureList("")),
    {ok, ProcRowCount} = sqlrelay:rowCount(),
    ProcCount = countMatchingProcs(0, ProcRowCount, 0),
    assertEqualsInt(ProcCount, 4),
    io:format("~n"),

    %% PROCEDURE PARAMETER LIST
    io:format("PROCEDURE PARAMETER LIST: ~n"),
    assertTrue(sqlrelay:getProcedureParameterList("testproc1", "")),
    assertEqualsString(sqlrelay:getColumnName(0), "parameter_name"),
    assertEqualsString(sqlrelay:getColumnName(1), "parameter_mode"),
    assertEqualsString(sqlrelay:getColumnName(2), "data_type"),
    assertEqualsString(sqlrelay:getColumnName(3), "character_maximum_length"),
    assertEqualsString(sqlrelay:getColumnName(4), "ordinal_position"),
    assertEqualsInt(sqlrelay:rowCount(), 4),
    assertEqualsString(sqlrelay:getFieldByName(0, "parameter_name"), "in1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "integer"),
    assertEqualsString(sqlrelay:getFieldByName(0, "ordinal_position"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(1, "parameter_name"), "in2"),
    assertEqualsString(sqlrelay:getFieldByName(1, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(1, "data_type"), "character"),
    assertEqualsString(sqlrelay:getFieldByName(1, "ordinal_position"), "2"),
    assertEqualsString(sqlrelay:getFieldByName(2, "parameter_name"), "in3"),
    assertEqualsString(sqlrelay:getFieldByName(2, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(2, "data_type"),
                       "character varying"),
    assertEqualsString(sqlrelay:getFieldByName(2, "ordinal_position"), "3"),
    assertEqualsString(sqlrelay:getFieldByName(3, "parameter_name"), "in4"),
    assertEqualsString(sqlrelay:getFieldByName(3, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(3, "data_type"), "date"),
    assertEqualsString(sqlrelay:getFieldByName(3, "ordinal_position"), "4"),
    assertTrue(sqlrelay:sendQuery(
        "drop function testproc1(int,char,varchar,date)")),
    assertTrue(sqlrelay:sendQuery(
        "drop function testproc2(int,char,varchar,date)")),
    assertTrue(sqlrelay:sendQuery(
        "drop function testproc3(int,char,varchar,date)")),
    assertTrue(sqlrelay:sendQuery(
        "drop function testproc4(int,char,varchar,date)")),
    io:format("~n"),

    %% INVALID QUERIES
    io:format("INVALID QUERIES: ~n"),
    assertFalse(sqlrelay:sendQuery(
        "select * from testtable order by testint")),
    assertFalse(sqlrelay:sendQuery(
        "select * from testtable order by testint")),
    assertFalse(sqlrelay:sendQuery(
        "select * from testtable order by testint")),
    assertFalse(sqlrelay:sendQuery(
        "select * from testtable order by testint")),
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

%%
%% Small helpers used in the main flow.
%%

%% case-insensitive emptiness check
isNullOrEmpty(undefined) -> true;
isNullOrEmpty(null)      -> true;
isNullOrEmpty([])        -> true;
isNullOrEmpty(_)         -> false.

%% String substring containment: does Haystack contain Needle ?
contains({ok, Haystack}, Needle) ->
    contains(Haystack, Needle);
contains(Haystack, Needle) when is_list(Haystack), is_list(Needle) ->
    string:str(Haystack, Needle) > 0;
contains(_, _) ->
    false.

%% Walk the table-type-list result looking for a row whose "table_type"
%% column is "TABLE".
searchTableTypeList(I, Count) when I >= Count ->
    false;
searchTableTypeList(I, Count) ->
    case sqlrelay:getFieldByName(I, "table_type") of
        {ok, Name} when is_list(Name) ->
            case Name =:= "TABLE" of
                true  -> true;
                false -> searchTableTypeList(I + 1, Count)
            end;
        _ ->
            searchTableTypeList(I + 1, Count)
    end.

%% Count rows in the table-list result whose "Tables_in_xxx" column is
%% one of the four test table names.
countMatchingTables(I, Count, Acc) when I >= Count ->
    Acc;
countMatchingTables(I, Count, Acc) ->
    Name = case sqlrelay:getFieldByName(I, "Tables_in_xxx") of
               {ok, N} when is_list(N) -> N;
               _ -> ""
           end,
    NewAcc = case Name of
                 "testtable1" -> Acc + 1;
                 "testtable2" -> Acc + 1;
                 "testtable3" -> Acc + 1;
                 "testtable4" -> Acc + 1;
                 _            -> Acc
             end,
    countMatchingTables(I + 1, Count, NewAcc).

%% Count rows in the procedure-list result whose "routine_name" is one
%% of the four test procedure names.
countMatchingProcs(I, Count, Acc) when I >= Count ->
    Acc;
countMatchingProcs(I, Count, Acc) ->
    Name = case sqlrelay:getFieldByName(I, "routine_name") of
               {ok, N} when is_list(N) -> N;
               _ -> ""
           end,
    NewAcc = case Name of
                 "testproc1" -> Acc + 1;
                 "testproc2" -> Acc + 1;
                 "testproc3" -> Acc + 1;
                 "testproc4" -> Acc + 1;
                 _           -> Acc
             end,
    countMatchingProcs(I + 1, Count, NewAcc).
