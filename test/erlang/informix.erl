%% Copyright (c) David Muse
%% See the file COPYING for more information.

-module(informix).
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
    %% you can set the isolation level, but to get it, you have to
    %% have permissions to read from sysmaster:syssqlcurses
    assertTrue(sqlrelay:setIsolationLevel(Il)),
    io:format("~n"),
    setIsolationLevels(Rest).

main() ->
    sqlrelay:start(),
    waitForPort(50),
    {ok, _} = sqlrelay:alloc("sqlrelay", 9010, "/tmp/informixtest.socket",
                             "testuser", "testpassword", 0, 1),

    Hostname = shortHostname(),

    %% IDENTIFY
    io:format("IDENTIFY: ~n"),
    assertEqualsString(sqlrelay:identify(), "informix"),
    io:format("~n"),

    %% PING
    io:format("PING: ~n"),
    assertTrue(sqlrelay:ping()),
    io:format("~n"),

    %% TRANSACTION STATE
    io:format("TRANSACTION STATE: ~n"),
    assertEqualsString(sqlrelay:getDefaultTransactionModel(), "implicit"),
    assertEqualsString(sqlrelay:getTransactionModel(), "implicit"),
    assertTrue(sqlrelay:getInTransaction()),
    assertFalse(sqlrelay:getAutoCommit()),
    io:format("~n"),

    %% BIND FORMAT
    io:format("BIND FORMAT: ~n"),
    assertEqualsString(sqlrelay:bindFormat(), "?"),
    io:format("~n"),

    %% NEXTVAL FORMAT
    io:format("NEXTVAL FORMAT: ~n"),
    assertEqualsString(sqlrelay:nextvalFormat(), "%s.nextval"),
    io:format("~n"),

    %% ISOLATION LEVELS
    io:format("ISOLATION LEVELS: ~n"),
    IsolationLevels = ["committed read", "dirty read",
                       "cursor stability", "repeatable read"],
    setIsolationLevels(IsolationLevels),
    %% reset to the default isolation level
    assertTrue(sqlrelay:setIsolationLevel(hd(IsolationLevels))),
    io:format("~n"),

    %% CREATE TESTTABLE
    io:format("CREATE TESTTABLE: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	testboolean boolean, "
        "	testsmallint smallint, "
        "	testint integer, "
        "	testbigint bigint, "
        "	testint8 int8, "
        "	testdecimal decimal(10,2), "
        "	testmoney money, "
        "	testsmallfloat smallfloat, "
        "	testfloat float, "
        "	testchar char(40), "
        "	testnchar nchar(40), "
        "	testvarchar varchar(40), "
        "	testnvarchar nvarchar(40), "
        "	testlvarchar lvarchar(40), "
        "	testdate date, "
        "	testdatetime datetime year to second, "
        "	testtext text, "
        "	testbyte byte)")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% INSERT
    io:format("INSERT: ~n"),
    assertTrue(sqlrelay:sendQuery(
        "insert into "
        "	testtable "
        "values ("
        "	't', "
        "	1, "
        "	1, "
        "	1, "
        "	1, "
        "	1.5, "
        "	1.5, "
        "	1.5, "
        "	1.5, "
        "	'testchar1', "
        "	'testnchar1', "
        "	'testvarchar1', "
        "	'testnvarchar1', "
        "	'testlvarchar1', "
        "	'01/01/2001', "
        "	'2001-01-01 01:00:00', "
        "	'testtext1', "
        "	null)")),
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
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?)"),
    assertEqualsInt(sqlrelay:countBindVariables(), 18),
    sqlrelay:inputBindString("1", "t"),
    sqlrelay:inputBindLong("2", 2),
    sqlrelay:inputBindLong("3", 2),
    sqlrelay:inputBindLong("4", 2),
    sqlrelay:inputBindLong("5", 2),
    sqlrelay:inputBindDouble("6", 2.5, 4, 2),
    sqlrelay:inputBindDouble("7", 2.5, 4, 2),
    sqlrelay:inputBindDouble("8", 2.5, 4, 2),
    sqlrelay:inputBindDouble("9", 2.5, 4, 2),
    sqlrelay:inputBindString("10", "testchar2"),
    sqlrelay:inputBindString("11", "testnchar2"),
    sqlrelay:inputBindString("12", "testvarchar2"),
    sqlrelay:inputBindString("13", "testnvarchar2"),
    sqlrelay:inputBindString("14", "testlvarchar2"),
    sqlrelay:inputBindDate("15", 2002, 1, 1, -1, -1, -1, -1, "", 0),
    sqlrelay:inputBindDate("16", 2002, 1, 1, 2, 0, 0, 0, "", 0),
    sqlrelay:inputBindClob("17", "testtext2", 9),
    sqlrelay:inputBindBlob("18", "testbyte2", 9),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindString("1", "t"),
    sqlrelay:inputBindLong("2", 3),
    sqlrelay:inputBindLong("3", 3),
    sqlrelay:inputBindLong("4", 3),
    sqlrelay:inputBindLong("5", 3),
    sqlrelay:inputBindDouble("6", 3.5, 4, 2),
    sqlrelay:inputBindDouble("7", 3.5, 4, 2),
    sqlrelay:inputBindDouble("8", 3.5, 4, 2),
    sqlrelay:inputBindDouble("9", 3.5, 4, 2),
    sqlrelay:inputBindString("10", "testchar3"),
    sqlrelay:inputBindString("11", "testnchar3"),
    sqlrelay:inputBindString("12", "testvarchar3"),
    sqlrelay:inputBindString("13", "testnvarchar3"),
    sqlrelay:inputBindString("14", "testlvarchar3"),
    sqlrelay:inputBindDate("15", 2003, 1, 1, -1, -1, -1, -1, "", 0),
    sqlrelay:inputBindDate("16", 2003, 1, 1, 3, 0, 0, 0, "", 0),
    sqlrelay:inputBindClob("17", "testtext3", 9),
    sqlrelay:inputBindBlob("18", "testbyte3", 9),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindString("1", "t"),
    sqlrelay:inputBindLong("2", 4),
    sqlrelay:inputBindLong("3", 4),
    sqlrelay:inputBindLong("4", 4),
    sqlrelay:inputBindLong("5", 4),
    sqlrelay:inputBindDouble("6", 4.5, 4, 2),
    sqlrelay:inputBindDouble("7", 4.5, 4, 2),
    sqlrelay:inputBindDouble("8", 4.5, 4, 2),
    sqlrelay:inputBindDouble("9", 4.5, 4, 2),
    sqlrelay:inputBindString("10", "testchar4"),
    sqlrelay:inputBindString("11", "testnchar4"),
    sqlrelay:inputBindString("12", "testvarchar4"),
    sqlrelay:inputBindString("13", "testnvarchar4"),
    sqlrelay:inputBindString("14", "testlvarchar4"),
    sqlrelay:inputBindDate("15", 2004, 1, 1, -1, -1, -1, -1, "", 0),
    sqlrelay:inputBindDate("16", 2004, 1, 1, 4, 0, 0, 0, "", 0),
    sqlrelay:inputBindClob("17", "testtext4", 9),
    sqlrelay:inputBindBlob("18", "testbyte4", 9),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindString("1", "t"),
    sqlrelay:inputBindLong("2", 5),
    sqlrelay:inputBindLong("3", 5),
    sqlrelay:inputBindLong("4", 5),
    sqlrelay:inputBindLong("5", 5),
    sqlrelay:inputBindDouble("6", 5.5, 4, 2),
    sqlrelay:inputBindDouble("7", 5.5, 4, 2),
    sqlrelay:inputBindDouble("8", 5.5, 4, 2),
    sqlrelay:inputBindDouble("9", 5.5, 4, 2),
    sqlrelay:inputBindString("10", "testchar5"),
    sqlrelay:inputBindString("11", "testnchar5"),
    sqlrelay:inputBindString("12", "testvarchar5"),
    sqlrelay:inputBindString("13", "testnvarchar5"),
    sqlrelay:inputBindString("14", "testlvarchar5"),
    sqlrelay:inputBindDate("15", 2005, 1, 1, -1, -1, -1, -1, "", 0),
    sqlrelay:inputBindDate("16", 2005, 1, 1, 5, 0, 0, 0, "", 0),
    sqlrelay:inputBindClob("17", "testtext5", 9),
    sqlrelay:inputBindBlob("18", "testbyte5", 9),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindString("1", "t"),
    sqlrelay:inputBindLong("2", 6),
    sqlrelay:inputBindLong("3", 6),
    sqlrelay:inputBindLong("4", 6),
    sqlrelay:inputBindLong("5", 6),
    sqlrelay:inputBindDouble("6", 6.5, 4, 2),
    sqlrelay:inputBindDouble("7", 6.5, 4, 2),
    sqlrelay:inputBindDouble("8", 6.5, 4, 2),
    sqlrelay:inputBindDouble("9", 6.5, 4, 2),
    sqlrelay:inputBindString("10", "testchar6"),
    sqlrelay:inputBindString("11", "testnchar6"),
    sqlrelay:inputBindString("12", "testvarchar6"),
    sqlrelay:inputBindString("13", "testnvarchar6"),
    sqlrelay:inputBindString("14", "testlvarchar6"),
    sqlrelay:inputBindDate("15", 2006, 1, 1, -1, -1, -1, -1, "", 0),
    sqlrelay:inputBindDate("16", 2006, 1, 1, 6, 0, 0, 0, "", 0),
    sqlrelay:inputBindClob("17", "testtext6", 9),
    sqlrelay:inputBindBlob("18", "testbyte6", 9),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% ARRAY OF INPUT BINDS BY POSITION
    %% The Erlang binding has no array-inputBinds(); do the individual
    %% binds for row 7 manually to keep the section faithful.
    io:format("ARRAY OF INPUT BINDS BY POSITION: ~n"),
    sqlrelay:clearBinds(),
    sqlrelay:prepareQuery(
        "insert into "
        "	testtable "
        "values ("
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	null, "
        "	null)"),
    sqlrelay:inputBindString("1", "t"),
    sqlrelay:inputBindString("2", "7"),
    sqlrelay:inputBindString("3", "7"),
    sqlrelay:inputBindString("4", "7"),
    sqlrelay:inputBindString("5", "7"),
    sqlrelay:inputBindString("6", "7.5"),
    sqlrelay:inputBindString("7", "7.5"),
    sqlrelay:inputBindString("8", "7.5"),
    sqlrelay:inputBindString("9", "7.5"),
    sqlrelay:inputBindString("10", "testchar7"),
    sqlrelay:inputBindString("11", "testnchar7"),
    sqlrelay:inputBindString("12", "testvarchar7"),
    sqlrelay:inputBindString("13", "testnvarchar7"),
    sqlrelay:inputBindString("14", "testlvarchar7"),
    sqlrelay:inputBindString("15", "01/01/2007"),
    sqlrelay:inputBindString("16", "2007-01-01 07:00:00"),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% INPUT BIND BY POSITION WITH VALIDATION
    io:format("INPUT BIND BY POSITION WITH VALIDATION: ~n"),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindString("1", "t"),
    sqlrelay:inputBindLong("2", 8),
    sqlrelay:inputBindLong("3", 8),
    sqlrelay:inputBindLong("4", 8),
    sqlrelay:inputBindLong("5", 8),
    sqlrelay:inputBindDouble("6", 8.5, 4, 2),
    sqlrelay:inputBindDouble("7", 8.5, 4, 2),
    sqlrelay:inputBindDouble("8", 8.5, 4, 2),
    sqlrelay:inputBindDouble("9", 8.5, 4, 2),
    sqlrelay:inputBindString("10", "testchar8"),
    sqlrelay:inputBindString("11", "testnchar8"),
    sqlrelay:inputBindString("12", "testvarchar8"),
    sqlrelay:inputBindString("13", "testnvarchar8"),
    sqlrelay:inputBindString("14", "testlvarchar8"),
    sqlrelay:inputBindDate("15", 2008, 1, 1, -1, -1, -1, -1, "", 0),
    sqlrelay:inputBindDate("16", 2008, 1, 1, 8, 0, 0, 0, "", 0),
    sqlrelay:inputBindClob("17", "testtext8", 9),
    sqlrelay:inputBindBlob("18", "testbyte8", 9),
    sqlrelay:validateBinds(),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% INPUT BIND BY NAME
    %% informix doesn't support bind by name

    %% ARRAY OF INPUT BINDS BY NAME
    %% informix doesn't support bind by name

    %% INPUT BIND BY NAME WITH VALIDATION
    %% informix doesn't support bind by name

    %% SELECT
    io:format("SELECT: ~n"),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    io:format("~n"),

    %% COLUMN COUNT
    io:format("COLUMN COUNT: ~n"),
    assertEqualsInt(sqlrelay:colCount(), 18),
    io:format("~n"),

    %% COLUMN NAMES
    io:format("COLUMN NAMES: ~n"),
    assertEqualsString(sqlrelay:getColumnName(0), "testboolean"),
    assertEqualsString(sqlrelay:getColumnName(1), "testsmallint"),
    assertEqualsString(sqlrelay:getColumnName(2), "testint"),
    assertEqualsString(sqlrelay:getColumnName(3), "testbigint"),
    assertEqualsString(sqlrelay:getColumnName(4), "testint8"),
    assertEqualsString(sqlrelay:getColumnName(5), "testdecimal"),
    assertEqualsString(sqlrelay:getColumnName(6), "testmoney"),
    assertEqualsString(sqlrelay:getColumnName(7), "testsmallfloat"),
    assertEqualsString(sqlrelay:getColumnName(8), "testfloat"),
    assertEqualsString(sqlrelay:getColumnName(9), "testchar"),
    assertEqualsString(sqlrelay:getColumnName(10), "testnchar"),
    assertEqualsString(sqlrelay:getColumnName(11), "testvarchar"),
    assertEqualsString(sqlrelay:getColumnName(12), "testnvarchar"),
    assertEqualsString(sqlrelay:getColumnName(13), "testlvarchar"),
    assertEqualsString(sqlrelay:getColumnName(14), "testdate"),
    assertEqualsString(sqlrelay:getColumnName(15), "testdatetime"),
    assertEqualsString(sqlrelay:getColumnName(16), "testtext"),
    assertEqualsString(sqlrelay:getColumnName(17), "testbyte"),
    {ok, Cols1} = sqlrelay:getColumnNames(),
    assertEqualsString(lists:nth(1, Cols1), "testboolean"),
    assertEqualsString(lists:nth(2, Cols1), "testsmallint"),
    assertEqualsString(lists:nth(3, Cols1), "testint"),
    assertEqualsString(lists:nth(4, Cols1), "testbigint"),
    assertEqualsString(lists:nth(5, Cols1), "testint8"),
    assertEqualsString(lists:nth(6, Cols1), "testdecimal"),
    assertEqualsString(lists:nth(7, Cols1), "testmoney"),
    assertEqualsString(lists:nth(8, Cols1), "testsmallfloat"),
    assertEqualsString(lists:nth(9, Cols1), "testfloat"),
    assertEqualsString(lists:nth(10, Cols1), "testchar"),
    assertEqualsString(lists:nth(11, Cols1), "testnchar"),
    assertEqualsString(lists:nth(12, Cols1), "testvarchar"),
    assertEqualsString(lists:nth(13, Cols1), "testnvarchar"),
    assertEqualsString(lists:nth(14, Cols1), "testlvarchar"),
    assertEqualsString(lists:nth(15, Cols1), "testdate"),
    assertEqualsString(lists:nth(16, Cols1), "testdatetime"),
    assertEqualsString(lists:nth(17, Cols1), "testtext"),
    assertEqualsString(lists:nth(18, Cols1), "testbyte"),
    io:format("~n"),

    %% COLUMN TYPES
    io:format("COLUMN TYPES: ~n"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(0), "BOOLEAN"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testboolean"), "BOOLEAN"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(1), "SMALLINT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testsmallint"), "SMALLINT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(2), "INTEGER"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testint"), "INTEGER"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(3), "BIGINT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testbigint"), "BIGINT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(4), "INT8"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testint8"), "INT8"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(5), "DECIMAL"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testdecimal"), "DECIMAL"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(6), "MONEY"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testmoney"), "MONEY"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(7), "SMALLFLOAT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testsmallfloat"),
                       "SMALLFLOAT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(8), "FLOAT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testfloat"), "FLOAT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(9), "CHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testchar"), "CHAR"),
    % informix reports nchar as char, with no way to tell them apart
    assertEqualsString(sqlrelay:getColumnTypeByIndex(10), "CHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testnchar"), "CHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(11), "VARCHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testvarchar"), "VARCHAR"),
    % informix reports nvarchar as varchar, with no way to tell them apart
    assertEqualsString(sqlrelay:getColumnTypeByIndex(12), "VARCHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testnvarchar"), "VARCHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(13), "LVARCHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testlvarchar"), "LVARCHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(14), "DATE"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testdate"), "DATE"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(15), "DATETIME"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testdatetime"), "DATETIME"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(16), "TEXT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testtext"), "TEXT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(17), "BYTE"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testbyte"), "BYTE"),
    io:format("~n"),

    %% COLUMN LENGTH
    io:format("COLUMN LENGTH: ~n"),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 1),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testboolean"), 1),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(1), 5),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testsmallint"), 5),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(2), 10),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testint"), 10),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(3), 20),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testbigint"), 20),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(4), 20),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testint8"), 20),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(5), 10),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testdecimal"), 10),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(6), 16),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testmoney"), 16),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(7), 7),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testsmallfloat"), 7),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(8), 15),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testfloat"), 15),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(9), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testchar"), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(10), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testnchar"), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(11), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testvarchar"), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(12), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testnvarchar"), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(13), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testlvarchar"), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(14), 10),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testdate"), 10),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(15), 19),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testdatetime"), 19),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(16), 2147483647),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testtext"), 2147483647),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(17), 2147483647),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testbyte"), 2147483647),
    io:format("~n"),

    %% LONGEST COLUMN
    io:format("LONGEST COLUMN: ~n"),
    assertEqualsInt(sqlrelay:getLongestByIndex(0), 1),
    assertEqualsInt(sqlrelay:getLongestByName("testboolean"), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(1), 1),
    assertEqualsInt(sqlrelay:getLongestByName("testsmallint"), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(2), 1),
    assertEqualsInt(sqlrelay:getLongestByName("testint"), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(3), 1),
    assertEqualsInt(sqlrelay:getLongestByName("testbigint"), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(4), 1),
    assertEqualsInt(sqlrelay:getLongestByName("testint8"), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(5), 4),
    assertEqualsInt(sqlrelay:getLongestByName("testdecimal"), 4),
    assertEqualsInt(sqlrelay:getLongestByIndex(6), 4),
    assertEqualsInt(sqlrelay:getLongestByName("testmoney"), 4),
    assertEqualsInt(sqlrelay:getLongestByIndex(7), 3),
    assertEqualsInt(sqlrelay:getLongestByName("testsmallfloat"), 3),
    assertEqualsInt(sqlrelay:getLongestByIndex(8), 3),
    assertEqualsInt(sqlrelay:getLongestByName("testfloat"), 3),
    assertEqualsInt(sqlrelay:getLongestByIndex(9), 40),
    assertEqualsInt(sqlrelay:getLongestByName("testchar"), 40),
    assertEqualsInt(sqlrelay:getLongestByIndex(10), 40),
    assertEqualsInt(sqlrelay:getLongestByName("testnchar"), 40),
    assertEqualsInt(sqlrelay:getLongestByIndex(11), 12),
    assertEqualsInt(sqlrelay:getLongestByName("testvarchar"), 12),
    assertEqualsInt(sqlrelay:getLongestByIndex(12), 13),
    assertEqualsInt(sqlrelay:getLongestByName("testnvarchar"), 13),
    assertEqualsInt(sqlrelay:getLongestByIndex(13), 13),
    assertEqualsInt(sqlrelay:getLongestByName("testlvarchar"), 13),
    assertEqualsInt(sqlrelay:getLongestByIndex(14), 10),
    assertEqualsInt(sqlrelay:getLongestByName("testdate"), 10),
    assertEqualsInt(sqlrelay:getLongestByIndex(15), 19),
    assertEqualsInt(sqlrelay:getLongestByName("testdatetime"), 19),
    assertEqualsInt(sqlrelay:getLongestByIndex(16), 9),
    assertEqualsInt(sqlrelay:getLongestByName("testtext"), 9),
    assertEqualsInt(sqlrelay:getLongestByIndex(17), 9),
    assertEqualsInt(sqlrelay:getLongestByName("testbyte"), 9),
    io:format("~n"),

    %% ROW COUNT
    io:format("ROW COUNT: ~n"),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    io:format("~n"),

    %% TOTAL ROWS
    io:format("TOTAL ROWS: ~n"),
    assertEqualsInt(sqlrelay:totalRows(), 0),
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
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 3), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 4), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 5), "1.50"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 6), "1.50"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 7), "1.5"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 8), "1.5"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 9),
                       "testchar1                               "),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 10),
                       "testnchar1                              "),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 11), "testvarchar1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 12), "testnvarchar1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 13), "testlvarchar1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 14), "2001-01-01"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 15), "2001-01-01 01:00:00"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 16), "testtext1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 17), ""),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 2), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 3), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 4), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 5), "8.50"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 6), "8.50"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 7), "8.5"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 8), "8.5"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 9),
                       "testchar8                               "),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 10),
                       "testnchar8                              "),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 11), "testvarchar8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 12), "testnvarchar8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 13), "testlvarchar8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 14), "2008-01-01"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 15), "2008-01-01 08:00:00"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 16), ""),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 17), ""),
    io:format("~n"),

    %% FIELD LENGTHS BY INDEX
    io:format("FIELD LENGTHS BY INDEX: ~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 1), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 2), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 3), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 4), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 5), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 6), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 7), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 8), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 9), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 10), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 11), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 12), 13),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 14), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 15), 19),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 16), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 17), 0),
    io:format("~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 0), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 1), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 2), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 3), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 4), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 5), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 6), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 7), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 8), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 9), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 10), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 11), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 12), 13),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 14), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 15), 19),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 16), 0),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 17), 0),
    io:format("~n"),

    %% FIELDS BY NAME
    io:format("FIELDS BY NAME: ~n"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testboolean"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testsmallint"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testint"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testbigint"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testint8"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testdecimal"), "1.50"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testmoney"), "1.50"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testsmallfloat"), "1.5"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testfloat"), "1.5"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testchar"),
                       "testchar1                               "),
    assertEqualsString(sqlrelay:getFieldByName(0, "testnchar"),
                       "testnchar1                              "),
    assertEqualsString(sqlrelay:getFieldByName(0, "testvarchar"),
                       "testvarchar1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testnvarchar"),
                       "testnvarchar1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testlvarchar"),
                       "testlvarchar1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testdate"), "2001-01-01"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testdatetime"),
                       "2001-01-01 01:00:00"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testtext"), "testtext1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testbyte"), ""),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testboolean"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testsmallint"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testint"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testbigint"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testint8"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testdecimal"), "8.50"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testmoney"), "8.50"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testsmallfloat"), "8.5"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testfloat"), "8.5"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testchar"),
                       "testchar8                               "),
    assertEqualsString(sqlrelay:getFieldByName(7, "testnchar"),
                       "testnchar8                              "),
    assertEqualsString(sqlrelay:getFieldByName(7, "testvarchar"),
                       "testvarchar8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testnvarchar"),
                       "testnvarchar8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testlvarchar"),
                       "testlvarchar8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testdate"), "2008-01-01"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testdatetime"),
                       "2008-01-01 08:00:00"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testtext"), ""),
    assertEqualsString(sqlrelay:getFieldByName(7, "testbyte"), ""),
    io:format("~n"),

    %% FIELD LENGTHS BY NAME
    io:format("FIELD LENGTHS BY NAME: ~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testboolean"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testsmallint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testbigint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testint8"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testdecimal"), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testmoney"), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testsmallfloat"), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testfloat"), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testchar"), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testnchar"), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testvarchar"), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testnvarchar"), 13),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testlvarchar"), 13),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testdate"), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testdatetime"), 19),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testtext"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testbyte"), 0),
    io:format("~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testboolean"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testsmallint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testbigint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testint8"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testdecimal"), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testmoney"), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testsmallfloat"), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testfloat"), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testchar"), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testnchar"), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testvarchar"), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testnvarchar"), 13),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testlvarchar"), 13),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testdate"), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testdatetime"), 19),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testtext"), 0),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testbyte"), 0),
    io:format("~n"),

    %% FIELDS BY ARRAY
    io:format("FIELDS BY ARRAY: ~n"),
    {ok, Row0} = sqlrelay:getRow(0),
    assertEqualsString(lists:nth(1, Row0), "1"),
    assertEqualsString(lists:nth(2, Row0), "1"),
    assertEqualsString(lists:nth(3, Row0), "1"),
    assertEqualsString(lists:nth(4, Row0), "1"),
    assertEqualsString(lists:nth(5, Row0), "1"),
    assertEqualsString(lists:nth(6, Row0), "1.50"),
    assertEqualsString(lists:nth(7, Row0), "1.50"),
    assertEqualsString(lists:nth(8, Row0), "1.5"),
    assertEqualsString(lists:nth(9, Row0), "1.5"),
    assertEqualsString(lists:nth(10, Row0),
                       "testchar1                               "),
    assertEqualsString(lists:nth(11, Row0),
                       "testnchar1                              "),
    assertEqualsString(lists:nth(12, Row0), "testvarchar1"),
    assertEqualsString(lists:nth(13, Row0), "testnvarchar1"),
    assertEqualsString(lists:nth(14, Row0), "testlvarchar1"),
    assertEqualsString(lists:nth(15, Row0), "2001-01-01"),
    assertEqualsString(lists:nth(16, Row0), "2001-01-01 01:00:00"),
    assertEqualsString(lists:nth(17, Row0), "testtext1"),
    assertEqualsString(lists:nth(18, Row0), ""),
    io:format("~n"),

    %% FIELD LENGTHS BY ARRAY
    io:format("FIELD LENGTHS BY ARRAY: ~n"),
    {ok, Rowlens0} = sqlrelay:getRowLengths(0),
    assertEqualsInt(lists:nth(1, Rowlens0), 1),
    assertEqualsInt(lists:nth(2, Rowlens0), 1),
    assertEqualsInt(lists:nth(3, Rowlens0), 1),
    assertEqualsInt(lists:nth(4, Rowlens0), 1),
    assertEqualsInt(lists:nth(5, Rowlens0), 1),
    assertEqualsInt(lists:nth(6, Rowlens0), 4),
    assertEqualsInt(lists:nth(7, Rowlens0), 4),
    assertEqualsInt(lists:nth(8, Rowlens0), 3),
    assertEqualsInt(lists:nth(9, Rowlens0), 3),
    assertEqualsInt(lists:nth(10, Rowlens0), 40),
    assertEqualsInt(lists:nth(11, Rowlens0), 40),
    assertEqualsInt(lists:nth(12, Rowlens0), 12),
    assertEqualsInt(lists:nth(13, Rowlens0), 13),
    assertEqualsInt(lists:nth(15, Rowlens0), 10),
    assertEqualsInt(lists:nth(16, Rowlens0), 19),
    assertEqualsInt(lists:nth(17, Rowlens0), 9),
    assertEqualsInt(lists:nth(18, Rowlens0), 0),
    io:format("~n"),

    %% RESULT SET BUFFER SIZE
    io:format("RESULT SET BUFFER SIZE: ~n"),
    assertEqualsInt(sqlrelay:getResultSetBufferSize(), 0),
    sqlrelay:setResultSetBufferSize(2),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    assertEqualsInt(sqlrelay:getResultSetBufferSize(), 2),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 0),
    assertFalse(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 2),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(1, 1), "2"),
    assertEqualsString(sqlrelay:getFieldByIndex(2, 1), "3"),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 2),
    assertFalse(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 4),
    assertEqualsString(sqlrelay:getFieldByIndex(6, 1), "7"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8"),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 6),
    assertFalse(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    assertEqualsString(sqlrelay:getFieldByIndex(8, 1), null),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 8),
    assertTrue(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    sqlrelay:setResultSetBufferSize(0),
    io:format("~n"),

    %% DONT GET COLUMN INFO
    io:format("DONT GET COLUMN INFO: ~n"),
    sqlrelay:dontGetColumnInfo(),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    assertEqualsString(sqlrelay:getColumnName(1), null),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(1), 0),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(1), null),
    sqlrelay:getColumnInfo(),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    assertEqualsString(sqlrelay:getColumnName(1), "testsmallint"),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(1), 5),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(1), "SMALLINT"),
    io:format("~n"),

    %% SUSPENDED SESSION
    io:format("SUSPENDED SESSION: ~n"),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    sqlrelay:suspendResultSet(),
    assertTrue(sqlrelay:suspendSession()),
    {ok, Port1} = sqlrelay:getConnectionPort(),
    {ok, Socket1} = sqlrelay:getConnectionSocket(),
    assertTrue(sqlrelay:resumeSession(Port1, Socket1)),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(1, 1), "2"),
    assertEqualsString(sqlrelay:getFieldByIndex(2, 1), "3"),
    assertEqualsString(sqlrelay:getFieldByIndex(3, 1), "4"),
    assertEqualsString(sqlrelay:getFieldByIndex(4, 1), "5"),
    assertEqualsString(sqlrelay:getFieldByIndex(5, 1), "6"),
    assertEqualsString(sqlrelay:getFieldByIndex(6, 1), "7"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8"),
    io:format("~n"),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    sqlrelay:suspendResultSet(),
    assertTrue(sqlrelay:suspendSession()),
    {ok, Port2} = sqlrelay:getConnectionPort(),
    {ok, Socket2} = sqlrelay:getConnectionSocket(),
    assertTrue(sqlrelay:resumeSession(Port2, Socket2)),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(1, 1), "2"),
    assertEqualsString(sqlrelay:getFieldByIndex(2, 1), "3"),
    assertEqualsString(sqlrelay:getFieldByIndex(3, 1), "4"),
    assertEqualsString(sqlrelay:getFieldByIndex(4, 1), "5"),
    assertEqualsString(sqlrelay:getFieldByIndex(5, 1), "6"),
    assertEqualsString(sqlrelay:getFieldByIndex(6, 1), "7"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8"),
    io:format("~n"),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    sqlrelay:suspendResultSet(),
    assertTrue(sqlrelay:suspendSession()),
    {ok, Port3} = sqlrelay:getConnectionPort(),
    {ok, Socket3} = sqlrelay:getConnectionSocket(),
    assertTrue(sqlrelay:resumeSession(Port3, Socket3)),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(1, 1), "2"),
    assertEqualsString(sqlrelay:getFieldByIndex(2, 1), "3"),
    assertEqualsString(sqlrelay:getFieldByIndex(3, 1), "4"),
    assertEqualsString(sqlrelay:getFieldByIndex(4, 1), "5"),
    assertEqualsString(sqlrelay:getFieldByIndex(5, 1), "6"),
    assertEqualsString(sqlrelay:getFieldByIndex(6, 1), "7"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8"),
    io:format("~n"),

    %% SUSPENDED RESULT SET
    io:format("SUSPENDED RESULT SET: ~n"),
    sqlrelay:setResultSetBufferSize(2),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    assertEqualsString(sqlrelay:getFieldByIndex(2, 1), "3"),
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
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8"),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 6),
    assertFalse(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    assertEqualsString(sqlrelay:getFieldByIndex(8, 1), null),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 8),
    assertTrue(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    sqlrelay:setResultSetBufferSize(0),
    io:format("~n"),

    %% CACHED RESULT SET
    io:format("CACHED RESULT SET: ~n"),
    sqlrelay:cacheToFile("cachefile1-informix"),
    sqlrelay:setCacheTtl(200),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    {ok, Filename1} = sqlrelay:getCacheFileName(),
    assertEqualsString(Filename1, "cachefile1-informix"),
    sqlrelay:cacheOff(),
    assertTrue(sqlrelay:openCachedResultSet(Filename1)),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8"),
    io:format("~n"),

    %% COLUMN COUNT FOR CACHED RESULT SET
    io:format("COLUMN COUNT FOR CACHED RESULT SET: ~n"),
    assertEqualsInt(sqlrelay:colCount(), 18),
    io:format("~n"),

    %% COLUMN NAMES FOR CACHED RESULT SET
    io:format("COLUMN NAMES FOR CACHED RESULT SET: ~n"),
    assertEqualsString(sqlrelay:getColumnName(0), "testboolean"),
    assertEqualsString(sqlrelay:getColumnName(1), "testsmallint"),
    assertEqualsString(sqlrelay:getColumnName(2), "testint"),
    assertEqualsString(sqlrelay:getColumnName(3), "testbigint"),
    assertEqualsString(sqlrelay:getColumnName(4), "testint8"),
    assertEqualsString(sqlrelay:getColumnName(5), "testdecimal"),
    assertEqualsString(sqlrelay:getColumnName(6), "testmoney"),
    assertEqualsString(sqlrelay:getColumnName(7), "testsmallfloat"),
    assertEqualsString(sqlrelay:getColumnName(8), "testfloat"),
    assertEqualsString(sqlrelay:getColumnName(9), "testchar"),
    assertEqualsString(sqlrelay:getColumnName(10), "testnchar"),
    assertEqualsString(sqlrelay:getColumnName(11), "testvarchar"),
    assertEqualsString(sqlrelay:getColumnName(12), "testnvarchar"),
    assertEqualsString(sqlrelay:getColumnName(13), "testlvarchar"),
    assertEqualsString(sqlrelay:getColumnName(14), "testdate"),
    assertEqualsString(sqlrelay:getColumnName(15), "testdatetime"),
    assertEqualsString(sqlrelay:getColumnName(16), "testtext"),
    assertEqualsString(sqlrelay:getColumnName(17), "testbyte"),
    {ok, Cols2} = sqlrelay:getColumnNames(),
    assertEqualsString(lists:nth(1, Cols2), "testboolean"),
    assertEqualsString(lists:nth(2, Cols2), "testsmallint"),
    assertEqualsString(lists:nth(3, Cols2), "testint"),
    assertEqualsString(lists:nth(4, Cols2), "testbigint"),
    assertEqualsString(lists:nth(5, Cols2), "testint8"),
    assertEqualsString(lists:nth(6, Cols2), "testdecimal"),
    assertEqualsString(lists:nth(7, Cols2), "testmoney"),
    assertEqualsString(lists:nth(8, Cols2), "testsmallfloat"),
    assertEqualsString(lists:nth(9, Cols2), "testfloat"),
    assertEqualsString(lists:nth(10, Cols2), "testchar"),
    assertEqualsString(lists:nth(11, Cols2), "testnchar"),
    assertEqualsString(lists:nth(12, Cols2), "testvarchar"),
    assertEqualsString(lists:nth(13, Cols2), "testnvarchar"),
    assertEqualsString(lists:nth(14, Cols2), "testlvarchar"),
    assertEqualsString(lists:nth(15, Cols2), "testdate"),
    assertEqualsString(lists:nth(16, Cols2), "testdatetime"),
    assertEqualsString(lists:nth(17, Cols2), "testtext"),
    assertEqualsString(lists:nth(18, Cols2), "testbyte"),
    io:format("~n"),

    %% CACHED RESULT SET WITH RESULT SET BUFFER SIZE
    io:format("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ~n"),
    sqlrelay:setResultSetBufferSize(2),
    sqlrelay:cacheToFile("cachefile1-informix"),
    sqlrelay:setCacheTtl(200),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    {ok, Filename2} = sqlrelay:getCacheFileName(),
    assertEqualsString(Filename2, "cachefile1-informix"),
    sqlrelay:cacheOff(),
    assertTrue(sqlrelay:openCachedResultSet(Filename2)),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(8, 1), null),
    sqlrelay:setResultSetBufferSize(0),
    io:format("~n"),

    %% FROM ONE CACHE FILE TO ANOTHER
    io:format("FROM ONE CACHE FILE TO ANOTHER: ~n"),
    sqlrelay:cacheToFile("cachefile2-informix"),
    assertTrue(sqlrelay:openCachedResultSet("cachefile1-informix")),
    sqlrelay:cacheOff(),
    assertTrue(sqlrelay:openCachedResultSet("cachefile2-informix")),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(8, 1), null),
    io:format("~n"),

    %% FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET BUFFER SIZE
    io:format("FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET BUFFER SIZE: ~n"),
    sqlrelay:setResultSetBufferSize(2),
    sqlrelay:cacheToFile("cachefile2-informix"),
    assertTrue(sqlrelay:openCachedResultSet("cachefile1-informix")),
    sqlrelay:cacheOff(),
    assertTrue(sqlrelay:openCachedResultSet("cachefile2-informix")),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(8, 1), null),
    sqlrelay:setResultSetBufferSize(0),
    io:format("~n"),

    %% CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE
    io:format("CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE: ~n"),
    sqlrelay:setResultSetBufferSize(2),
    sqlrelay:cacheToFile("cachefile1-informix"),
    sqlrelay:setCacheTtl(200),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    assertEqualsString(sqlrelay:getFieldByIndex(2, 1), "3"),
    {ok, Filename3} = sqlrelay:getCacheFileName(),
    assertEqualsString(Filename3, "cachefile1-informix"),
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
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8"),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 6),
    assertFalse(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    assertEqualsString(sqlrelay:getFieldByIndex(8, 1), null),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 8),
    assertTrue(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    sqlrelay:cacheOff(),
    io:format("~n"),
    assertTrue(sqlrelay:openCachedResultSet(Filename3)),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(8, 1), null),
    sqlrelay:setResultSetBufferSize(0),
    io:format("~n"),

    %% FINISHED SUSPENDED SESSION
    io:format("FINISHED SUSPENDED SESSION: ~n"),
    assertTrue(sqlrelay:sendQuery("select * from testtable order by testint")),
    assertEqualsString(sqlrelay:getFieldByIndex(4, 1), "5"),
    assertEqualsString(sqlrelay:getFieldByIndex(5, 1), "6"),
    assertEqualsString(sqlrelay:getFieldByIndex(6, 1), "7"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8"),
    {ok, Id3} = sqlrelay:getResultSetId(),
    sqlrelay:suspendResultSet(),
    assertTrue(sqlrelay:suspendSession()),
    {ok, Port6} = sqlrelay:getConnectionPort(),
    {ok, Socket6} = sqlrelay:getConnectionSocket(),
    assertTrue(sqlrelay:resumeSession(Port6, Socket6)),
    assertTrue(sqlrelay:resumeResultSet(Id3)),
    assertEqualsString(sqlrelay:getFieldByIndex(4, 1), null),
    assertEqualsString(sqlrelay:getFieldByIndex(5, 1), null),
    assertEqualsString(sqlrelay:getFieldByIndex(6, 1), null),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), null),
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
    assertEqualsString(sqlrelay:getTransactionModel(), "implicit"),
    assertFalse(sqlrelay:getAutoCommit()),
    io:format("~n"),

    %% TRANSACTION BEHAVIOR - implicit/explicit/explicit-deferred/explicit-error/none
    %% SKIPPED: these blocks require a second concurrent connection
    %% (secondcon) and cursor (secondcur) to verify cross-connection
    %% isolation. The Erlang binding only supports one connection/cursor
    %% per process (see sqlrelay.erl: alloc uses a single
    %% connection/cursor slot in the process dictionary), so a second
    %% connection cannot be instantiated here.
    io:format("TRANSACTION BEHAVIOR - implicit/explicit/explicit-deferred/explicit-error/none: ~n"),
    io:format("(skipped - requires second concurrent connection)~n"),
    %% Drop the leftover testtable and commit so subsequent sections
    %% start clean.
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% RESET TRANSACTION BEHAVIOR
    io:format("RESET TRANSACTION BEHAVIOR: ~n"),
    {ok, DefaultModel} = sqlrelay:getDefaultTransactionModel(),
    assertTrue(sqlrelay:setTransactionModel(DefaultModel)),
    assertEqualsString(sqlrelay:getTransactionModel(), "implicit"),
    assertFalse(sqlrelay:getAutoCommit()),
    io:format("~n"),

    %% INDIVIDUAL SUBSTITUTIONS
    io:format("INDIVIDUAL SUBSTITUTIONS: ~n"),
    sqlrelay:prepareQuery(
        "select "
        "	$(var1), "
        "	'$(var2)', "
        "	'$(var3)' "
        "from "
        "	sysmaster:sysdual "),
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
    sqlrelay:prepareQuery(
        "select "
        "	'$(var1)', "
        "	'$(var2)', "
        "	'$(var3)' "
        "from "
        "	sysmaster:sysdual "),
    sqlrelay:subString("var1", "hi"),
    sqlrelay:subString("var2", "hello"),
    sqlrelay:subString("var3", "bye"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "hi"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "hello"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "bye"),
    io:format("~n"),
    sqlrelay:prepareQuery(
        "select "
        "	$(var1), "
        "	$(var2), "
        "	$(var3) "
        "from "
        "	sysmaster:sysdual "),
    sqlrelay:subLong("var1", 1),
    sqlrelay:subLong("var2", 2),
    sqlrelay:subLong("var3", 3),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "2"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "3"),
    io:format("~n"),
    sqlrelay:prepareQuery(
        "select "
        "	$(var1), "
        "	$(var2), "
        "	$(var3) "
        "from "
        "	sysmaster:sysdual "),
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
    assertTrue(sqlrelay:sendQuery(
        "select NULL::int,1,NULL::int from sysmaster:sysdual")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), null),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), null),
    sqlrelay:getNullsAsEmptyStrings(),
    assertTrue(sqlrelay:sendQuery(
        "select NULL::int,1,NULL::int from sysmaster:sysdual")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), ""),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), ""),
    io:format("~n"),

    %% OUTPUT BIND BY POSITION
    io:format("OUTPUT BIND BY POSITION: ~n"),
    sqlrelay:sendQuery("drop procedure testproc"),
    sqlrelay:getNullsAsNulls(),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc("
        "	out out1 int, "
        "	out out2 varchar(20), "
        "	out out3 float, "
        "	out out4 varchar(20)) "
        "let out1 = 1; "
        "	let out2 = 'hello'; "
        "	let out3 = 2.5; "
        "	let out4 = null; "
        "end procedure;")),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("{call testproc(?,?,?,?)}"),
    assertEqualsInt(sqlrelay:countBindVariables(), 4),
    sqlrelay:defineOutputBindInteger("1"),
    sqlrelay:defineOutputBindString("2", 20),
    sqlrelay:defineOutputBindDouble("3"),
    sqlrelay:defineOutputBindString("4", 20),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("1"), 1),
    assertEqualsString(sqlrelay:getOutputBindString("2"), "hello"),
    assertEqualsDouble(sqlrelay:getOutputBindDouble("3"), 2.5),
    assertEqualsString(sqlrelay:getOutputBindString("4"), null),
    sqlrelay:getNullsAsEmptyStrings(),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% OUTPUT BIND BY NAME
    %% informix doesn't support bind by name

    %% OUTPUT BIND BY NAME WITH VALIDATION
    %% informix doesn't support bind by name

    %% LOB OUTPUT BIND
    io:format("LOB OUTPUT BIND: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	testclob clob, "
        "	testblob blob)")),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("insert into testtable values (?,?)"),
    sqlrelay:inputBindClob("1", "hello", 5),
    sqlrelay:inputBindBlob("2", "hello", 5),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc("
        "	out out1 clob, "
        "	out out2 blob) "
        "select testclob, testblob "
        "	into out1, out2 "
        "	from testtable; "
        "end procedure;")),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("{call testproc(?,?)}"),
    sqlrelay:defineOutputBindClob("1"),
    sqlrelay:defineOutputBindBlob("2"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsStringLen(sqlrelay:getOutputBindClob("1"), "hello", 5),
    assertEqualsInt(sqlrelay:getOutputBindLength("1"), 5),
    assertEqualsStringLen(sqlrelay:getOutputBindBlob("2"), "hello", 5),
    assertEqualsInt(sqlrelay:getOutputBindLength("2"), 5),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc")),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% LONG OUTPUT BIND
    io:format("LONG OUTPUT BIND: ~n"),
    LargeBufferLength = 20 * 1024,
    LargeBuf = largeBuffer(LargeBufferLength),
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc("
        "	in1 clob, "
        "	out out1 clob) "
        "let out1 = in1; "
        "	end procedure;")),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("{call testproc(?,?)}"),
    sqlrelay:inputBindClob("1", LargeBuf, LargeBufferLength),
    sqlrelay:defineOutputBindClob("2"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindLength("2"), LargeBufferLength),
    assertEqualsString(sqlrelay:getOutputBindClob("2"), LargeBuf),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% NEGATIVE INPUT BIND
    io:format("NEGATIVE INPUT BIND: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    sqlrelay:sendQuery("create table testtable (testval int)"),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("insert into testtable values (?)"),
    sqlrelay:inputBindLong("1", -1),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("select testval from testtable"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testval"), "-1"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% BIND VALIDATION
    %% informix doesn't support bind by name

    %% REBINDING
    io:format("REBINDING: ~n"),
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc("
        "	in1 int, "
        "	out out1 int) "
        "let out1 = in1; "
        "end procedure;")),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("{call testproc(?,?)}"),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:defineOutputBindInteger("2"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("2"), 1),
    sqlrelay:inputBindLong("1", 2),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("2"), 2),
    sqlrelay:inputBindLong("1", 3),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("2"), 3),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% REEXECUTE
    io:format("REEXECUTE: ~n"),
    sqlrelay:prepareQuery("select 1 from sysmaster:sysdual"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:rowCount(), 1),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    io:format("~n"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:rowCount(), 1),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    io:format("~n"),
    sqlrelay:prepareQuery("select ?::int from sysmaster:sysdual"),
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
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc("
        "	in1 int, "
        "	in2 float, "
        "	in3 varchar(20)) "
        "end procedure;")),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("{call testproc(?,?,?)}"),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:inputBindDouble("2", 2.5, 2, 1),
    sqlrelay:inputBindString("3", "hello"),
    assertTrue(sqlrelay:executeQuery()),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING SINGLE VALUE
    io:format("STORED PROCEDURE RETURNING SINGLE VALUE: ~n"),
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc("
        "	in1 int, "
        "	in2 float, "
        "	in3 varchar(20), "
        "	out out1 int) "
        "let out1 = in1; "
        "end procedure;")),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("{call testproc(?,?,?,?)}"),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:inputBindDouble("2", 2.5, 2, 1),
    sqlrelay:inputBindString("3", "hello"),
    sqlrelay:defineOutputBindInteger("4"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("4"), 1),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING MULTIPLE VALUES
    io:format("STORED PROCEDURE RETURNING MULTIPLE VALUES: ~n"),
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc("
        "	in1 int, "
        "	in2 float, "
        "	in3 varchar(20), "
        "	out out1 int, "
        "	out out2 float, "
        "	out out3 varchar(20)) "
        "let out1 = in1; "
        "	let out2 = in2; "
        "	let out3 = in3; "
        "end procedure;")),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("{call testproc(?,?,?,?,?,?)}"),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:inputBindDouble("2", 2.5, 2, 1),
    sqlrelay:inputBindString("3", "hello"),
    sqlrelay:defineOutputBindInteger("4"),
    sqlrelay:defineOutputBindDouble("5"),
    sqlrelay:defineOutputBindString("6", 20),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("4"), 1),
    assertEqualsDouble(sqlrelay:getOutputBindDouble("5"), 2.5),
    assertEqualsString(sqlrelay:getOutputBindString("6"), "hello"),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING RESULT SET
    io:format("STORED PROCEDURE RETURNING RESULT SET: ~n"),
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc() "
        "returning boolean, smallint, varchar(40); "
        "	define out1 boolean; "
        "	define out2 smallint; "
        "	define out3 varchar(40); "
        "	foreach "
        "		select "
        "			testboolean, "
        "			testsmallint, "
        "			testvarchar "
        "		into out1,out2,out3 "
        "		from ( "
        "			select "
        "				't' as testboolean, "
        "				1 as testsmallint, "
        "				'1' as testvarchar "
        "			from "
        "				sysmaster:sysdual "
        "			union "
        "			select "
        "				't' as testboolean, "
        "				2 as testsmallint, "
        "				'2' as testvarchar "
        "			from "
        "				sysmaster:sysdual "
        "			union "
        "			select "
        "				't' as testboolean, "
        "				3 as testsmallint, "
        "				'3' as testvarchar "
        "			from "
        "				sysmaster:sysdual "
        "			union "
        "			select "
        "				't' as testboolean, "
        "				4 as testsmallint, "
        "				'4' as testvarchar "
        "			from "
        "				sysmaster:sysdual "
        "			union "
        "			select "
        "				't' as testboolean, "
        "				5 as testsmallint, "
        "				'5' as testvarchar "
        "			from "
        "				sysmaster:sysdual "
        "			union "
        "			select "
        "				't' as testboolean, "
        "				6 as testsmallint, "
        "				'6' as testvarchar "
        "			from "
        "				sysmaster:sysdual "
        "			union "
        "			select "
        "				't' as testboolean, "
        "				7 as testsmallint, "
        "				'7' as testvarchar "
        "			from "
        "				sysmaster:sysdual "
        "			union "
        "			select "
        "				't' as testboolean, "
        "				8 as testsmallint, "
        "				'8' as testvarchar "
        "			from "
        "				sysmaster:sysdual "
        "		) "
        "	return out1,out2,out3 "
        "	with resume; "
        "	end foreach; "
        "	end procedure;")),
    assertTrue(sqlrelay:commit()),
    assertTrue(sqlrelay:sendQuery("{call testproc()}")),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% NULL AND EMPTY LOBS
    io:format("NULL AND EMPTY LOBS: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    sqlrelay:getNullsAsNulls(),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	testclob1 clob, "
        "	testclob2 clob, "
        "	testblob1 blob, "
        "	testblob2 blob)")),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery(
        "insert into "
        "	testtable "
        "values ("
        "	?, "
        "	?, "
        "	?, "
        "	?)"),
    sqlrelay:inputBindClob("1", "", 0),
    sqlrelay:inputBindNull("2"),
    sqlrelay:inputBindBlob("3", "", 0),
    sqlrelay:inputBindNull("4"),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("select * from testtable"),
    %% informix returns a single \0 when you insert an empty blob/clob
    %% (same quirk seen on sap/freetds) — strip the trailing NUL.
    {ok, Nelob0} = sqlrelay:getFieldByIndex(0, 0),
    assertEqualsString(trimNull(Nelob0), ""),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), null),
    {ok, Nelob2} = sqlrelay:getFieldByIndex(0, 2),
    assertEqualsString(trimNull(Nelob2), ""),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 3), null),
    sqlrelay:getNullsAsEmptyStrings(),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% LONG LOBS
    io:format("LONG LOBS: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    sqlrelay:sendQuery(
        "create table testtable ("
        "	testtext text, "
        "	testbyte byte)"),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("insert into testtable values (?,?)"),
    sqlrelay:inputBindClob("1", LargeBuf, LargeBufferLength),
    sqlrelay:inputBindBlob("2", LargeBuf, LargeBufferLength),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("select * from testtable"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testtext"),
                    LargeBufferLength),
    assertEqualsString(sqlrelay:getFieldByName(0, "testtext"), LargeBuf),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testbyte"),
                    LargeBufferLength),
    assertEqualsStringLen(sqlrelay:getFieldByName(0, "testbyte"),
                          LargeBuf, LargeBufferLength),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% TEMPORARY TABLES
    io:format("TEMPORARY TABLES: ~n"),
    sqlrelay:sendQuery("drop table temptable"),
    sqlrelay:sendQuery("create temp table temptable (col1 int)"),
    assertTrue(sqlrelay:sendQuery("insert into temptable values (1)")),
    assertTrue(sqlrelay:sendQuery("select count(*) from temptable")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    sqlrelay:endSession(),
    io:format("~n"),
    assertFalse(sqlrelay:sendQuery("select count(*) from temptable")),
    io:format("~n"),

    %% ENCODED BINARY DATA
    %% informix doesn't support encoded binary data

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
    assertInResultSet("Database", Hostname),
    io:format("~n"),

    %% SCHEMA LIST
    io:format("SCHEMA LIST: ~n"),
    %% informix requires that a table exist that is
    %% owned by a user for the user to be reported
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 integer, "
        "	col2 integer)")),
    assertTrue(sqlrelay:commit()),
    assertTrue(sqlrelay:getSchemaList("")),
    assertEqualsString(sqlrelay:getColumnName(0), "Database"),
    assertInResultSet("Database", "testuser"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% TABLE TYPE LIST
    io:format("TABLE TYPE LIST: ~n"),
    assertTrue(sqlrelay:getTableTypeList()),
    assertEqualsString(sqlrelay:getColumnName(0), "table_type"),
    assertInResultSet("table_type", "TABLE"),
    io:format("~n"),

    %% TABLE LIST
    io:format("TABLE LIST: ~n"),
    sqlrelay:sendQuery("drop table testtable1"),
    sqlrelay:sendQuery("drop table testtable2"),
    sqlrelay:sendQuery("drop table testtable3"),
    sqlrelay:sendQuery("drop table testtable4"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable1 ("
        "	col1 integer, "
        "	col2 integer)")),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable2 ("
        "	col1 integer, "
        "	col2 integer)")),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable3 ("
        "	col1 integer, "
        "	col2 integer)")),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable4 ("
        "	col1 integer, "
        "	col2 integer)")),
    assertTrue(sqlrelay:commit()),
    assertTrue(sqlrelay:getTableList("")),
    assertInResultSet("Tables_in_xxx", "testtable1"),
    assertInResultSet("Tables_in_xxx", "testtable2"),
    assertInResultSet("Tables_in_xxx", "testtable3"),
    assertInResultSet("Tables_in_xxx", "testtable4"),
    assertTrue(sqlrelay:sendQuery("drop table testtable1")),
    assertTrue(sqlrelay:sendQuery("drop table testtable2")),
    assertTrue(sqlrelay:sendQuery("drop table testtable3")),
    assertTrue(sqlrelay:sendQuery("drop table testtable4")),
    assertTrue(sqlrelay:commit()),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "32767"),
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
        "	testboolean boolean, "
        "	testsmallint smallint, "
        "	testint integer, "
        "	testbigint bigint, "
        "	testint8 int8, "
        "	testdecimal decimal(10,2), "
        "	testmoney money, "
        "	testsmallfloat smallfloat, "
        "	testfloat float, "
        "	testchar char(40), "
        "	testnchar nchar(40), "
        "	testvarchar varchar(40), "
        "	testnvarchar nvarchar(40), "
        "	testlvarchar lvarchar(40), "
        "	testdate date, "
        "	testdatetime datetime year to second, "
        "	testtext text, "
        "	testbyte byte)")),
    assertTrue(sqlrelay:commit()),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "column_name"),
                       "testboolean"),
    assertEqualsString(sqlrelay:getFieldByName(1, "column_name"),
                       "testsmallint"),
    assertEqualsString(sqlrelay:getFieldByName(2, "column_name"), "testint"),
    assertEqualsString(sqlrelay:getFieldByName(3, "column_name"),
                       "testbigint"),
    assertEqualsString(sqlrelay:getFieldByName(4, "column_name"), "testint8"),
    assertEqualsString(sqlrelay:getFieldByName(5, "column_name"),
                       "testdecimal"),
    assertEqualsString(sqlrelay:getFieldByName(6, "column_name"),
                       "testmoney"),
    assertEqualsString(sqlrelay:getFieldByName(7, "column_name"),
                       "testsmallfloat"),
    assertEqualsString(sqlrelay:getFieldByName(8, "column_name"), "testfloat"),
    assertEqualsString(sqlrelay:getFieldByName(9, "column_name"), "testchar"),
    assertEqualsString(sqlrelay:getFieldByName(10, "column_name"),
                       "testnchar"),
    assertEqualsString(sqlrelay:getFieldByName(11, "column_name"),
                       "testvarchar"),
    assertEqualsString(sqlrelay:getFieldByName(12, "column_name"),
                       "testnvarchar"),
    assertEqualsString(sqlrelay:getFieldByName(13, "column_name"),
                       "testlvarchar"),
    assertEqualsString(sqlrelay:getFieldByName(14, "column_name"), "testdate"),
    assertEqualsString(sqlrelay:getFieldByName(15, "column_name"),
                       "testdatetime"),
    assertEqualsString(sqlrelay:getFieldByName(16, "column_name"), "testtext"),
    assertEqualsString(sqlrelay:getFieldByName(17, "column_name"), "testbyte"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "BOOLEAN"),
    assertEqualsString(sqlrelay:getFieldByName(1, "data_type"), "SMALLINT"),
    assertEqualsString(sqlrelay:getFieldByName(2, "data_type"), "INTEGER"),
    assertEqualsString(sqlrelay:getFieldByName(3, "data_type"), "BIGINT"),
    assertEqualsString(sqlrelay:getFieldByName(4, "data_type"), "INT8"),
    assertEqualsString(sqlrelay:getFieldByName(5, "data_type"), "DECIMAL"),
    assertEqualsString(sqlrelay:getFieldByName(6, "data_type"), "MONEY"),
    assertEqualsString(sqlrelay:getFieldByName(7, "data_type"), "SMALLFLOAT"),
    assertEqualsString(sqlrelay:getFieldByName(8, "data_type"), "FLOAT"),
    assertEqualsString(sqlrelay:getFieldByName(9, "data_type"), "CHAR"),
    assertEqualsString(sqlrelay:getFieldByName(10, "data_type"), "NCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(11, "data_type"), "VARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(12, "data_type"), "NVARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(13, "data_type"), "LVARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(14, "data_type"), "DATE"),
    assertEqualsString(sqlrelay:getFieldByName(15, "data_type"), "DATETIME"),
    assertEqualsString(sqlrelay:getFieldByName(16, "data_type"), "TEXT"),
    assertEqualsString(sqlrelay:getFieldByName(17, "data_type"), "BYTE"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% COLUMN LIST - auto_increment, primary key
    io:format("COLUMN LIST - auto_increment, primary key: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 serial primary key, "
        "	col2 int)")),
    assertTrue(sqlrelay:commit()),
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
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:commit()),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 int primary key, "
        "	col2 int)")),
    assertTrue(sqlrelay:getColumnList("testtable", "")),
    {ok, Ex2} = sqlrelay:getFieldByName(0, "extra"),
    assertEqualsString(Ex2, ""),
    {ok, Ck2} = sqlrelay:getFieldByName(0, "column_key"),
    assertEqualsString(Ck2, "PRI"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% PRIMARY KEYS LIST
    io:format("PRIMARY KEYS LIST: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 integer primary key, "
        "	col2 integer)")),
    assertTrue(sqlrelay:commit()),
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
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% KEY AND INDEX LIST
    io:format("KEY AND INDEX LIST: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 integer primary key, "
        "	col2 integer)")),
    assertTrue(sqlrelay:commit()),
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
    {ok, KeyName2} = sqlrelay:getFieldByName(0, "key_name"),
    assertTrue(not isNullOrEmpty(KeyName2)),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% PROCEDURE LIST
    io:format("PROCEDURE LIST: ~n"),
    sqlrelay:sendQuery("drop procedure testproc1"),
    sqlrelay:sendQuery("drop procedure testproc2"),
    sqlrelay:sendQuery("drop procedure testproc3"),
    sqlrelay:sendQuery("drop procedure testproc4"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc1("
        "	in1 integer, "
        "	in2 char(20), "
        "	in3 varchar(20), "
        "	in4 date) "
        "define x integer; "
        "let x = 1; "
        "end procedure;")),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc2("
        "	in1 integer, "
        "	in2 char(20), "
        "	in3 varchar(20), "
        "	in4 date) "
        "define x integer; "
        "let x = 1; "
        "end procedure;")),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc3("
        "	in1 integer, "
        "	in2 char(20), "
        "	in3 varchar(20), "
        "	in4 date) "
        "define x integer; "
        "let x = 1; "
        "end procedure;")),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc4("
        "	in1 integer, "
        "	in2 char(20), "
        "	in3 varchar(20), "
        "	in4 date) "
        "define x integer; "
        "let x = 1; "
        "end procedure;")),
    assertTrue(sqlrelay:commit()),
    assertTrue(sqlrelay:getProcedureList("")),
    assertInResultSet("routine_name", "testproc1"),
    assertInResultSet("routine_name", "testproc2"),
    assertInResultSet("routine_name", "testproc3"),
    assertInResultSet("routine_name", "testproc4"),
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
    assertEqualsString(sqlrelay:getFieldByName(1, "data_type"), "char"),
    assertEqualsString(sqlrelay:getFieldByName(1, "ordinal_position"), "2"),
    assertEqualsString(sqlrelay:getFieldByName(2, "parameter_name"), "in3"),
    assertEqualsString(sqlrelay:getFieldByName(2, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(2, "data_type"), "varchar"),
    assertEqualsString(sqlrelay:getFieldByName(2, "ordinal_position"), "3"),
    assertEqualsString(sqlrelay:getFieldByName(3, "parameter_name"), "in4"),
    assertEqualsString(sqlrelay:getFieldByName(3, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(3, "data_type"), "date"),
    assertEqualsString(sqlrelay:getFieldByName(3, "ordinal_position"), "4"),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc1")),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc2")),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc3")),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc4")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% INVALID QUERIES
    io:format("INVALID QUERIES: ~n"),
    assertFalse(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    assertFalse(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    assertFalse(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    assertFalse(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
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

%% case-insensitive substring search for isNullOrEmpty
isNullOrEmpty(undefined) -> true;
isNullOrEmpty(null)      -> true;
isNullOrEmpty([])        -> true;
isNullOrEmpty(_)         -> false.

%% Strip a trailing '\0' (or anything after the first NUL).
trimNull(undefined) -> "";
trimNull(null)      -> "";
trimNull(L) when is_list(L) ->
    lists:takewhile(fun(C) -> C =/= 0 end, L).
