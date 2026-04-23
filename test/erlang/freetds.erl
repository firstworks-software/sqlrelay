%% Copyright (c) David Muse
%% See the file COPYING for more information.

-module(freetds).
-export([main/0]).

-import(asserts, [pass/0, fail/2,
                  getStatus/0, reportTestStatus/0,
                  assertEqualsString/2, assertEqualsStringLen/3,
                  assertEqualsInt/2, assertEqualsDouble/2,
                  assertTrue/1, assertFalse/1,
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

    Hostname = shortHostname(),
    DumpTran = "dump tran " ++ Hostname ++ " with truncate_only",

    %% IDENTIFY
    io:format("IDENTIFY: ~n"),
    assertEqualsString(sqlrelay:identify(), "freetds"),
    io:format("~n"),

    %% PING
    io:format("PING: ~n"),
    assertTrue(sqlrelay:ping()),
    io:format("~n"),

    %% BIND FORMAT
    io:format("BIND FORMAT: ~n"),
    assertEqualsString(sqlrelay:bindFormat(), "@*"),
    io:format("~n"),

    %% NEXTVAL FORMAT
    io:format("NEXTVAL FORMAT: ~n"),
    assertEqualsString(sqlrelay:nextvalFormat(), "%s.nextval"),
    io:format("~n"),

    %% ISOLATION LEVELS
    io:format("ISOLATION LEVELS: ~n"),
    IsolationLevels = ["1", "0", "2", "3"],
    setIsolationLevels(IsolationLevels),
    %% reset to the default isolation level
    assertTrue(sqlrelay:setIsolationLevel(hd(IsolationLevels))),
    io:format("~n"),

    %% CREATE TESTTABLE
    io:format("CREATE TESTTABLE: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    sqlrelay:sendQuery(DumpTran),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	testint int, "
        "	testsmallint smallint, "
        "	testtinyint tinyint, "
        "	testreal real, "
        "	testfloat float, "
        "	testdecimal decimal(4,1), "
        "	testnumeric numeric(4,1), "
        "	testmoney money, "
        "	testsmallmoney smallmoney, "
        "	testdatetime datetime, "
        "	testsmalldatetime smalldatetime, "
        "	testchar char(40), "
        "	testvarchar varchar(40), "
        "	testbit bit) lock datarows")),
    io:format("~n"),

    %% INSERT
    io:format("INSERT: ~n"),
    assertTrue(sqlrelay:beginTransaction()),
    assertTrue(sqlrelay:sendQuery(
        "insert into "
        "	testtable "
        "values ("
        "	1, "
        "	1, "
        "	1, "
        "	1.1, "
        "	1.1, "
        "	1.1, "
        "	1.1, "
        "	1.00, "
        "	1.00, "
        "	'01-Jan-2001 01:00:00', "
        "	'01-Jan-2001 01:00:00', "
        "	'testchar1', "
        "	'testvarchar1', "
        "	1)")),
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
        "	?)"),
    assertEqualsInt(sqlrelay:countBindVariables(), 14),
    sqlrelay:inputBindLong("1", 2),
    sqlrelay:inputBindLong("2", 2),
    sqlrelay:inputBindLong("3", 2),
    sqlrelay:inputBindDouble("4", 2.2, 2, 1),
    sqlrelay:inputBindDouble("5", 2.2, 2, 1),
    sqlrelay:inputBindDouble("6", 2.2, 2, 1),
    sqlrelay:inputBindDouble("7", 2.2, 2, 1),
    sqlrelay:inputBindDouble("8", 2.00, 3, 2),
    sqlrelay:inputBindDouble("9", 2.00, 3, 2),
    sqlrelay:inputBindString("10", "01-Jan-2002 02:00:00"),
    sqlrelay:inputBindString("11", "01-Jan-2002 02:00:00"),
    sqlrelay:inputBindString("12", "testchar2"),
    sqlrelay:inputBindString("13", "testvarchar2"),
    sqlrelay:inputBindLong("14", 1),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 3),
    sqlrelay:inputBindLong("2", 3),
    sqlrelay:inputBindLong("3", 3),
    sqlrelay:inputBindDouble("4", 3.3, 2, 1),
    sqlrelay:inputBindDouble("5", 3.3, 2, 1),
    sqlrelay:inputBindDouble("6", 3.3, 2, 1),
    sqlrelay:inputBindDouble("7", 3.3, 2, 1),
    sqlrelay:inputBindDouble("8", 3.00, 3, 2),
    sqlrelay:inputBindDouble("9", 3.00, 3, 2),
    sqlrelay:inputBindString("10", "01-Jan-2003 03:00:00"),
    sqlrelay:inputBindString("11", "01-Jan-2003 03:00:00"),
    sqlrelay:inputBindString("12", "testchar3"),
    sqlrelay:inputBindString("13", "testvarchar3"),
    sqlrelay:inputBindLong("14", 1),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% ARRAY OF INPUT BINDS BY POSITION
    %% freetds doesn't support implicit conversion of string binds to
    %% other data types, so arrays of binds don't generally work.

    %% INPUT BIND BY POSITION WITH VALIDATION
    io:format("INPUT BIND BY POSITION WITH VALIDATION: ~n"),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 4),
    sqlrelay:inputBindLong("2", 4),
    sqlrelay:inputBindLong("3", 4),
    sqlrelay:inputBindDouble("4", 4.4, 2, 1),
    sqlrelay:inputBindDouble("5", 4.4, 2, 1),
    sqlrelay:inputBindDouble("6", 4.4, 2, 1),
    sqlrelay:inputBindDouble("7", 4.4, 2, 1),
    sqlrelay:inputBindDouble("8", 4.00, 3, 2),
    sqlrelay:inputBindDouble("9", 4.00, 3, 2),
    sqlrelay:inputBindString("10", "01-Jan-2004 04:00:00"),
    sqlrelay:inputBindString("11", "01-Jan-2004 04:00:00"),
    sqlrelay:inputBindString("12", "testchar4"),
    sqlrelay:inputBindString("13", "testvarchar4"),
    sqlrelay:inputBindLong("14", 1),
    sqlrelay:validateBinds(),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% INPUT BIND BY NAME
    io:format("INPUT BIND BY NAME: ~n"),
    sqlrelay:clearBinds(),
    sqlrelay:prepareQuery(
        "insert into "
        "	testtable "
        "values ("
        "	@var1, "
        "	@var2, "
        "	@var3, "
        "	@var4, "
        "	@var5, "
        "	@var6, "
        "	@var7, "
        "	@var8, "
        "	@var9, "
        "	@var10, "
        "	@var11, "
        "	@var12, "
        "	@var13, "
        "	@var14)"),
    assertEqualsInt(sqlrelay:countBindVariables(), 14),
    sqlrelay:inputBindLong("var1", 5),
    sqlrelay:inputBindLong("var2", 5),
    sqlrelay:inputBindLong("var3", 5),
    sqlrelay:inputBindDouble("var4", 5.5, 2, 1),
    sqlrelay:inputBindDouble("var5", 5.5, 2, 1),
    sqlrelay:inputBindDouble("var6", 5.5, 2, 1),
    sqlrelay:inputBindDouble("var7", 5.5, 2, 1),
    sqlrelay:inputBindDouble("var8", 5.00, 3, 2),
    sqlrelay:inputBindDouble("var9", 5.00, 3, 2),
    sqlrelay:inputBindString("var10", "01-Jan-2005 05:00:00"),
    sqlrelay:inputBindString("var11", "01-Jan-2005 05:00:00"),
    sqlrelay:inputBindString("var12", "testchar5"),
    sqlrelay:inputBindString("var13", "testvarchar5"),
    sqlrelay:inputBindLong("var14", 1),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("var1", 6),
    sqlrelay:inputBindLong("var2", 6),
    sqlrelay:inputBindLong("var3", 6),
    sqlrelay:inputBindDouble("var4", 6.6, 2, 1),
    sqlrelay:inputBindDouble("var5", 6.6, 2, 1),
    sqlrelay:inputBindDouble("var6", 6.6, 2, 1),
    sqlrelay:inputBindDouble("var7", 6.6, 2, 1),
    sqlrelay:inputBindDouble("var8", 6.00, 3, 2),
    sqlrelay:inputBindDouble("var9", 6.00, 3, 2),
    sqlrelay:inputBindString("var10", "01-Jan-2006 06:00:00"),
    sqlrelay:inputBindString("var11", "01-Jan-2006 06:00:00"),
    sqlrelay:inputBindString("var12", "testchar6"),
    sqlrelay:inputBindString("var13", "testvarchar6"),
    sqlrelay:inputBindLong("var14", 1),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("var1", 7),
    sqlrelay:inputBindLong("var2", 7),
    sqlrelay:inputBindLong("var3", 7),
    sqlrelay:inputBindDouble("var4", 7.7, 2, 1),
    sqlrelay:inputBindDouble("var5", 7.7, 2, 1),
    sqlrelay:inputBindDouble("var6", 7.7, 2, 1),
    sqlrelay:inputBindDouble("var7", 7.7, 2, 1),
    sqlrelay:inputBindDouble("var8", 7.00, 3, 2),
    sqlrelay:inputBindDouble("var9", 7.00, 3, 2),
    sqlrelay:inputBindString("var10", "01-Jan-2007 07:00:00"),
    sqlrelay:inputBindString("var11", "01-Jan-2007 07:00:00"),
    sqlrelay:inputBindString("var12", "testchar7"),
    sqlrelay:inputBindString("var13", "testvarchar7"),
    sqlrelay:inputBindLong("var14", 1),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% ARRAY OF INPUT BINDS BY NAME
    %% freetds doesn't support implicit conversion of string binds to
    %% other data types, so arrays of binds don't generally work.

    %% INPUT BIND BY NAME WITH VALIDATION
    io:format("INPUT BIND BY NAME WITH VALIDATION: ~n"),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("var1", 8),
    sqlrelay:inputBindLong("var2", 8),
    sqlrelay:inputBindLong("var3", 8),
    sqlrelay:inputBindDouble("var4", 8.8, 2, 1),
    sqlrelay:inputBindDouble("var5", 8.8, 2, 1),
    sqlrelay:inputBindDouble("var6", 8.8, 2, 1),
    sqlrelay:inputBindDouble("var7", 8.8, 2, 1),
    sqlrelay:inputBindDouble("var8", 8.00, 3, 2),
    sqlrelay:inputBindDouble("var9", 8.00, 3, 2),
    sqlrelay:inputBindString("var10", "01-Jan-2008 08:00:00"),
    sqlrelay:inputBindString("var11", "01-Jan-2008 08:00:00"),
    sqlrelay:inputBindString("var12", "testchar8"),
    sqlrelay:inputBindString("var13", "testvarchar8"),
    sqlrelay:inputBindLong("var14", 1),
    sqlrelay:inputBindString("var15", "junkvalue"),
    sqlrelay:validateBinds(),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% SELECT
    io:format("SELECT: ~n"),
    assertTrue(sqlrelay:sendQuery(
        "select * from testtable order by testint")),
    io:format("~n"),

    %% COLUMN COUNT
    io:format("COLUMN COUNT: ~n"),
    assertEqualsInt(sqlrelay:colCount(), 14),
    io:format("~n"),

    %% COLUMN NAMES
    io:format("COLUMN NAMES: ~n"),
    assertEqualsString(sqlrelay:getColumnName(0), "testint"),
    assertEqualsString(sqlrelay:getColumnName(1), "testsmallint"),
    assertEqualsString(sqlrelay:getColumnName(2), "testtinyint"),
    assertEqualsString(sqlrelay:getColumnName(3), "testreal"),
    assertEqualsString(sqlrelay:getColumnName(4), "testfloat"),
    assertEqualsString(sqlrelay:getColumnName(5), "testdecimal"),
    assertEqualsString(sqlrelay:getColumnName(6), "testnumeric"),
    assertEqualsString(sqlrelay:getColumnName(7), "testmoney"),
    assertEqualsString(sqlrelay:getColumnName(8), "testsmallmoney"),
    assertEqualsString(sqlrelay:getColumnName(9), "testdatetime"),
    assertEqualsString(sqlrelay:getColumnName(10), "testsmalldatetime"),
    assertEqualsString(sqlrelay:getColumnName(11), "testchar"),
    assertEqualsString(sqlrelay:getColumnName(12), "testvarchar"),
    assertEqualsString(sqlrelay:getColumnName(13), "testbit"),
    {ok, Cols1} = sqlrelay:getColumnNames(),
    assertEqualsString(lists:nth(1, Cols1), "testint"),
    assertEqualsString(lists:nth(2, Cols1), "testsmallint"),
    assertEqualsString(lists:nth(3, Cols1), "testtinyint"),
    assertEqualsString(lists:nth(4, Cols1), "testreal"),
    assertEqualsString(lists:nth(5, Cols1), "testfloat"),
    assertEqualsString(lists:nth(6, Cols1), "testdecimal"),
    assertEqualsString(lists:nth(7, Cols1), "testnumeric"),
    assertEqualsString(lists:nth(8, Cols1), "testmoney"),
    assertEqualsString(lists:nth(9, Cols1), "testsmallmoney"),
    assertEqualsString(lists:nth(10, Cols1), "testdatetime"),
    assertEqualsString(lists:nth(11, Cols1), "testsmalldatetime"),
    assertEqualsString(lists:nth(12, Cols1), "testchar"),
    assertEqualsString(lists:nth(13, Cols1), "testvarchar"),
    assertEqualsString(lists:nth(14, Cols1), "testbit"),
    io:format("~n"),

    %% COLUMN TYPES
    io:format("COLUMN TYPES: ~n"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(0), "INT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testint"), "INT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(1), "SMALLINT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testsmallint"),
                       "SMALLINT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(2), "TINYINT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testtinyint"),
                       "TINYINT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(3), "REAL"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testreal"), "REAL"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(4), "FLOAT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testfloat"), "FLOAT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(5), "DECIMAL"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testdecimal"),
                       "DECIMAL"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(6), "NUMERIC"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testnumeric"),
                       "NUMERIC"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(7), "MONEY"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testmoney"), "MONEY"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(8), "SMALLMONEY"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testsmallmoney"),
                       "SMALLMONEY"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(9), "DATETIME"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testdatetime"),
                       "DATETIME"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(10), "SMALLDATETIME"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testsmalldatetime"),
                       "SMALLDATETIME"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(11), "CHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testchar"), "CHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(12), "CHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testvarchar"), "CHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(13), "BIT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testbit"), "BIT"),
    io:format("~n"),

    %% COLUMN LENGTH
    io:format("COLUMN LENGTH: ~n"),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testint"), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(1), 2),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testsmallint"), 2),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(2), 1),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testtinyint"), 1),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(3), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testreal"), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(4), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testfloat"), 8),
    %% these seem to fluctuate with every freetds release
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(7), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testmoney"), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(8), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testsmallmoney"), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(9), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testdatetime"), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(10), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testsmalldatetime"), 4),
    %% these seem to fluctuate too
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(13), 1),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testbit"), 1),
    io:format("~n"),

    %% LONGEST COLUMN
    io:format("LONGEST COLUMN: ~n"),
    assertEqualsInt(sqlrelay:getLongestByIndex(0), 1),
    assertEqualsInt(sqlrelay:getLongestByName("testint"), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(1), 1),
    assertEqualsInt(sqlrelay:getLongestByName("testsmallint"), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(2), 1),
    assertEqualsInt(sqlrelay:getLongestByName("testtinyint"), 1),
    %% these seem to fluctuate with every freetds release
    assertEqualsInt(sqlrelay:getLongestByIndex(11), 40),
    assertEqualsInt(sqlrelay:getLongestByName("testchar"), 40),
    assertEqualsInt(sqlrelay:getLongestByIndex(12), 12),
    assertEqualsInt(sqlrelay:getLongestByName("testvarchar"), 12),
    assertEqualsInt(sqlrelay:getLongestByIndex(13), 1),
    assertEqualsInt(sqlrelay:getLongestByName("testbit"), 1),
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
    %% these seem to fluctuate with every freetds release
    assertEqualsString(sqlrelay:getFieldByIndex(0, 5), "1.1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 6), "1.1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 11),
                       "testchar1                               "),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 12), "testvarchar1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 13), "1"),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 2), "8"),
    %% these seem to fluctuate with every freetds release
    {ok, F075} = sqlrelay:getFieldByIndex(7, 5),
    assertEqualsStringLen(lists:sublist(F075, 3), "8.8", 3),
    {ok, F076} = sqlrelay:getFieldByIndex(7, 6),
    assertEqualsStringLen(lists:sublist(F076, 3), "8.8", 3),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 11),
                       "testchar8                               "),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 12), "testvarchar8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 13), "1"),
    io:format("~n"),

    %% FIELD LENGTHS BY INDEX
    io:format("FIELD LENGTHS BY INDEX: ~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 1), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 2), 1),
    %% these seem to fluctuate with every freetds release
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 11), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 12), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 13), 1),
    io:format("~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 0), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 1), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 2), 1),
    %% these seem to fluctuate with every freetds release
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 11), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 12), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 13), 1),
    io:format("~n"),

    %% FIELDS BY NAME
    io:format("FIELDS BY NAME: ~n"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testint"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testsmallint"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testtinyint"), "1"),
    %% these seem to fluctuate with every freetds release
    assertEqualsString(sqlrelay:getFieldByName(0, "testdecimal"), "1.1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testnumeric"), "1.1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testchar"),
                       "testchar1                               "),
    assertEqualsString(sqlrelay:getFieldByName(0, "testvarchar"),
                       "testvarchar1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testbit"), "1"),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testint"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testsmallint"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testtinyint"), "8"),
    %% these seem to fluctuate with every freetds release
    {ok, Fd7} = sqlrelay:getFieldByName(7, "testdecimal"),
    assertEqualsStringLen(lists:sublist(Fd7, 3), "8.8", 3),
    {ok, Fn7} = sqlrelay:getFieldByName(7, "testnumeric"),
    assertEqualsStringLen(lists:sublist(Fn7, 3), "8.8", 3),
    assertEqualsString(sqlrelay:getFieldByName(7, "testchar"),
                       "testchar8                               "),
    assertEqualsString(sqlrelay:getFieldByName(7, "testvarchar"),
                       "testvarchar8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testbit"), "1"),
    io:format("~n"),

    %% FIELD LENGTHS BY NAME
    io:format("FIELD LENGTHS BY NAME: ~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testsmallint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testtinyint"), 1),
    %% these seem to fluctuate with every freetds release
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testchar"), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testvarchar"), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testbit"), 1),
    io:format("~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testsmallint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testtinyint"), 1),
    %% these seem to fluctuate with every freetds release
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testchar"), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testvarchar"), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testbit"), 1),
    io:format("~n"),

    %% FIELDS BY ARRAY
    io:format("FIELDS BY ARRAY: ~n"),
    {ok, Row0} = sqlrelay:getRow(0),
    assertEqualsString(lists:nth(1, Row0), "1"),
    assertEqualsString(lists:nth(2, Row0), "1"),
    assertEqualsString(lists:nth(3, Row0), "1"),
    %% these seem to fluctuate with every freetds release
    assertEqualsString(lists:nth(6, Row0), "1.1"),
    assertEqualsString(lists:nth(7, Row0), "1.1"),
    assertEqualsString(lists:nth(12, Row0),
                       "testchar1                               "),
    assertEqualsString(lists:nth(13, Row0), "testvarchar1"),
    assertEqualsString(lists:nth(14, Row0), "1"),
    io:format("~n"),

    %% FIELD LENGTHS BY ARRAY
    io:format("FIELD LENGTHS BY ARRAY: ~n"),
    {ok, Rowlens0} = sqlrelay:getRowLengths(0),
    assertEqualsInt(lists:nth(1, Rowlens0), 1),
    assertEqualsInt(lists:nth(2, Rowlens0), 1),
    assertEqualsInt(lists:nth(3, Rowlens0), 1),
    %% these seem to fluctuate with every freetds release
    assertEqualsInt(lists:nth(12, Rowlens0), 40),
    assertEqualsInt(lists:nth(13, Rowlens0), 12),
    assertEqualsInt(lists:nth(14, Rowlens0), 1),
    io:format("~n"),

    %% RESULT SET BUFFER SIZE
    io:format("RESULT SET BUFFER SIZE: ~n"),
    assertEqualsInt(sqlrelay:getResultSetBufferSize(), 0),
    sqlrelay:setResultSetBufferSize(2),
    assertTrue(sqlrelay:sendQuery(
        "select * from testtable order by testint")),
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
    assertTrue(sqlrelay:sendQuery(
        "select * from testtable order by testint")),
    assertEqualsString(sqlrelay:getColumnName(0), null),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 0),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(0), null),
    sqlrelay:getColumnInfo(),
    assertTrue(sqlrelay:sendQuery(
        "select * from testtable order by testint")),
    assertEqualsString(sqlrelay:getColumnName(0), "testint"),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 4),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(0), "INT"),
    io:format("~n"),

    %% SUSPENDED SESSION
    io:format("SUSPENDED SESSION: ~n"),
    assertTrue(sqlrelay:sendQuery(
        "select * from testtable order by testint")),
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
    assertTrue(sqlrelay:sendQuery(
        "select * from testtable order by testint")),
    sqlrelay:suspendResultSet(),
    assertTrue(sqlrelay:suspendSession()),
    {ok, Port2} = sqlrelay:getConnectionPort(),
    {ok, Socket2} = sqlrelay:getConnectionSocket(),
    assertTrue(sqlrelay:resumeSession(Port2, Socket2)),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(1, 0), "2"),
    assertEqualsString(sqlrelay:getFieldByIndex(2, 0), "3"),
    assertEqualsString(sqlrelay:getFieldByIndex(3, 0), "4"),
    assertEqualsString(sqlrelay:getFieldByIndex(4, 0), "5"),
    assertEqualsString(sqlrelay:getFieldByIndex(5, 0), "6"),
    assertEqualsString(sqlrelay:getFieldByIndex(6, 0), "7"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    io:format("~n"),
    assertTrue(sqlrelay:sendQuery(
        "select * from testtable order by testint")),
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
    assertTrue(sqlrelay:sendQuery(
        "select * from testtable order by testint")),
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
    assertTrue(sqlrelay:sendQuery(
        "select * from testtable order by testint")),
    {ok, Filename1} = sqlrelay:getCacheFileName(),
    assertEqualsString(Filename1, "cachefile1"),
    sqlrelay:cacheOff(),
    assertTrue(sqlrelay:openCachedResultSet(Filename1)),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    io:format("~n"),

    %% COLUMN COUNT FOR CACHED RESULT SET
    io:format("COLUMN COUNT FOR CACHED RESULT SET: ~n"),
    assertEqualsInt(sqlrelay:colCount(), 14),
    io:format("~n"),

    %% COLUMN NAMES FOR CACHED RESULT SET
    io:format("COLUMN NAMES FOR CACHED RESULT SET: ~n"),
    assertEqualsString(sqlrelay:getColumnName(0), "testint"),
    assertEqualsString(sqlrelay:getColumnName(1), "testsmallint"),
    assertEqualsString(sqlrelay:getColumnName(2), "testtinyint"),
    assertEqualsString(sqlrelay:getColumnName(3), "testreal"),
    assertEqualsString(sqlrelay:getColumnName(4), "testfloat"),
    assertEqualsString(sqlrelay:getColumnName(5), "testdecimal"),
    assertEqualsString(sqlrelay:getColumnName(6), "testnumeric"),
    assertEqualsString(sqlrelay:getColumnName(7), "testmoney"),
    assertEqualsString(sqlrelay:getColumnName(8), "testsmallmoney"),
    assertEqualsString(sqlrelay:getColumnName(9), "testdatetime"),
    assertEqualsString(sqlrelay:getColumnName(10), "testsmalldatetime"),
    assertEqualsString(sqlrelay:getColumnName(11), "testchar"),
    assertEqualsString(sqlrelay:getColumnName(12), "testvarchar"),
    assertEqualsString(sqlrelay:getColumnName(13), "testbit"),
    {ok, Cols2} = sqlrelay:getColumnNames(),
    assertEqualsString(lists:nth(1, Cols2), "testint"),
    assertEqualsString(lists:nth(2, Cols2), "testsmallint"),
    assertEqualsString(lists:nth(3, Cols2), "testtinyint"),
    assertEqualsString(lists:nth(4, Cols2), "testreal"),
    assertEqualsString(lists:nth(5, Cols2), "testfloat"),
    assertEqualsString(lists:nth(6, Cols2), "testdecimal"),
    assertEqualsString(lists:nth(7, Cols2), "testnumeric"),
    assertEqualsString(lists:nth(8, Cols2), "testmoney"),
    assertEqualsString(lists:nth(9, Cols2), "testsmallmoney"),
    assertEqualsString(lists:nth(10, Cols2), "testdatetime"),
    assertEqualsString(lists:nth(11, Cols2), "testsmalldatetime"),
    assertEqualsString(lists:nth(12, Cols2), "testchar"),
    assertEqualsString(lists:nth(13, Cols2), "testvarchar"),
    assertEqualsString(lists:nth(14, Cols2), "testbit"),
    io:format("~n"),

    %% CACHED RESULT SET WITH RESULT SET BUFFER SIZE
    io:format("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ~n"),
    sqlrelay:setResultSetBufferSize(2),
    sqlrelay:cacheToFile("cachefile1"),
    sqlrelay:setCacheTtl(200),
    assertTrue(sqlrelay:sendQuery(
        "select * from testtable order by testint")),
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
    assertTrue(sqlrelay:sendQuery(
        "select * from testtable order by testint")),
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
    assertTrue(sqlrelay:sendQuery(
        "select * from testtable order by testint")),
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
    %% concurrent cursors cannot be held. We still run the outer query.
    io:format("NESTED SELECTS: ~n"),
    assertTrue(sqlrelay:sendQuery("select * from testtable")),
    %% Inner loop with secondcur is omitted.
    io:format("~n"),

    %% COMMIT AND ROLLBACK
    %% SKIPPED: this section requires a second concurrent connection
    %% (secondcon) to verify cross-connection isolation. The Erlang
    %% binding only supports one connection per process, so a second
    %% connection cannot be instantiated here.
    io:format("COMMIT AND ROLLBACK: ~n"),
    io:format("(skipped - requires second concurrent connection)~n"),
    %% We still need to drop the test table so the rest of the test works.
    assertTrue(sqlrelay:commit()),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
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
        "	testclob1 text NULL, "
        "	testclob2 text NULL, "
        "	testblob1 image NULL, "
        "	testblob2 image NULL)")),
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
    %% sap converts empty strings to a single space.
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), " "),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), null),
    %% sap doesn't really support inserting an empty string into a binary
    %% column. The minimum that can be inserted is a single \0, which
    %% reads back as a one-byte string containing \0 — treat that as "".
    {ok, Blob2} = sqlrelay:getFieldByIndex(0, 2),
    Blob2Trim = case Blob2 of
        undefined -> "";
        null      -> "";
        _         -> lists:takewhile(fun(C) -> C =/= 0 end, Blob2)
    end,
    assertEqualsString(Blob2Trim, ""),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 3), null),
    sqlrelay:getNullsAsEmptyStrings(),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% LONG LOBS
    io:format("LONG LOBS: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    sqlrelay:sendQuery(
        "create table testtable ("
        "	testclob text, "
        "	testblob image) lock datarows"),
    sqlrelay:prepareQuery("insert into testtable values (?,?)"),
    LargeBufferLength = 8192,
    LargeBuf = largeBuffer(LargeBufferLength),
    sqlrelay:inputBindClob("1", LargeBuf, LargeBufferLength),
    sqlrelay:inputBindBlob("2", LargeBuf, LargeBufferLength),
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
    %% FreeTDS needs to support cursors for this to work - disabled in C++

    %% OUTPUT BIND BY NAME
    %% FreeTDS needs to support cursors for this to work - disabled in C++

    %% OUTPUT BIND BY NAME WITH VALIDATION
    %% Even if FreeTDS supported cursors, sap doesn't declare bind
    %% delimiters with exec, so validateBinds can't be used - disabled in C++

    %% LOB OUTPUT BIND
    %% sap doesn't support lobs as output parameters - disabled in C++

    %% LONG OUTPUT BIND
    %% FreeTDS needs to support cursors for this to work - disabled in C++

    %% NEGATIVE INPUT BIND
    io:format("NEGATIVE INPUT BIND: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    sqlrelay:sendQuery("create table testtable (testval int)"),
    sqlrelay:prepareQuery("insert into testtable values (@testval)"),
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
    sqlrelay:subString("var1", "@var1"),
    assertTrue(sqlrelay:validBind("var1")),
    assertFalse(sqlrelay:validBind("var2")),
    assertFalse(sqlrelay:validBind("var3")),
    assertFalse(sqlrelay:validBind("var4")),
    io:format("~n"),
    sqlrelay:subString("var2", "@var2"),
    assertTrue(sqlrelay:validBind("var1")),
    assertTrue(sqlrelay:validBind("var2")),
    assertFalse(sqlrelay:validBind("var3")),
    assertFalse(sqlrelay:validBind("var4")),
    io:format("~n"),
    sqlrelay:subString("var3", "@var3"),
    assertTrue(sqlrelay:validBind("var1")),
    assertTrue(sqlrelay:validBind("var2")),
    assertTrue(sqlrelay:validBind("var3")),
    assertFalse(sqlrelay:validBind("var4")),
    assertTrue(sqlrelay:executeQuery()),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% REBINDING
    %% FreeTDS needs to support cursors for this to work - disabled in C++

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
    sqlrelay:prepareQuery("select cast(? as int)"),
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
    %% FreeTDS needs to support cursors for this to work - disabled in C++

    %% STORED PROCEDURE RETURNING SINGLE VALUE
    %% FreeTDS needs to support cursors for this to work - disabled in C++

    %% STORED PROCEDURE RETURNING MULTIPLE VALUES
    %% FreeTDS needs to support cursors for this to work - disabled in C++

    %% STORED PROCEDURE RETURNING RESULT SET
    io:format("STORED PROCEDURE RETURNING RESULT SET: ~n"),
    sqlrelay:sendQuery("drop procedure testselectproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testselectproc as "
        "       select 1 "
        "       union "
        "       select 2 "
        "       union "
        "       select 3 "
        "       union "
        "       select 4 "
        "       union "
        "       select 5 "
        "       union "
        "       select 6 "
        "       union "
        "       select 7 "
        "       union "
        "       select 8")),
    assertTrue(sqlrelay:sendQuery("exec testselectproc")),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    assertTrue(sqlrelay:sendQuery("drop procedure testselectproc")),
    io:format("~n"),

    %% TEMPORARY TABLES
    io:format("TEMPORARY TABLES: ~n"),
    sqlrelay:sendQuery("drop table #temptable"),
    sqlrelay:sendQuery("create table #temptable (col1 int)"),
    assertTrue(sqlrelay:sendQuery("insert into #temptable values (1)")),
    assertTrue(sqlrelay:sendQuery("select count(*) from #temptable")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    sqlrelay:endSession(),
    io:format("~n"),
    assertFalse(sqlrelay:sendQuery("select count(*) from #temptable")),
    io:format("~n"),

    %% ENCODED BINARY DATA
    io:format("ENCODED BINARY DATA: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery("create table testtable (col1 image)")),
    Buffer = lists:seq(0, 255),
    HexStr = lists:flatten([io_lib:format("~2.16.0b", [B]) || B <- Buffer]),
    QueryStr = "insert into testtable values (0x" ++ HexStr ++ ")",
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
    assertTrue(sqlrelay:sendQuery(
        "create table testtable (col1 varchar(4))")),
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
        "	(col1 int identity primary key, "
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
    sqlrelay:sendQuery("drop table testtable"),
    %% the get schema list query that is used with sap will only return
    %% the names of schemas that have at least one database object in
    %% them, so to be sure that there is one, we'll create a table
    assertTrue(sqlrelay:sendQuery("create table testtable (col1 int)")),
    assertTrue(sqlrelay:getSchemaList("")),
    assertEqualsString(sqlrelay:getColumnName(0), "Database"),
    {ok, SchemaRowCount} = sqlrelay:rowCount(),
    assertTrue(SchemaRowCount > 0),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    _ = Hostname,
    io:format("~n"),

    %% TABLE TYPE LIST
    io:format("TABLE TYPE LIST: ~n"),
    assertTrue(sqlrelay:getTableTypeList()),
    assertEqualsString(sqlrelay:getColumnName(0), "table_type"),
    {ok, TTRowCount} = sqlrelay:rowCount(),
    TTFound = findTableType(0, TTRowCount),
    assertTrue(TTFound),
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
    assertTrue(sqlrelay:getTypeInfoList("int")),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "type_name"), "INT"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "4"),
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "10"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"), "INT"),
    assertTrue(sqlrelay:getTypeInfoList("char")),
    assertEqualsString(sqlrelay:getFieldByName(0, "type_name"), "CHAR"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "8000"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"), "CHAR"),
    assertTrue(sqlrelay:getTypeInfoList("varchar")),
    assertEqualsString(sqlrelay:getFieldByName(0, "type_name"), "VARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "12"),
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "8000"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"),
                       "VARCHAR"),
    assertTrue(sqlrelay:getTypeInfoList("datetime")),
    assertEqualsString(sqlrelay:getFieldByName(0, "type_name"), "DATETIME"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "93"),
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "23"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"),
                       "DATETIME"),
    io:format("~n"),

    %% COLUMN LIST
    io:format("COLUMN LIST: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	testint int, "
        "	testsmallint smallint, "
        "	testtinyint tinyint, "
        "	testreal real, "
        "	testfloat float, "
        "	testdecimal decimal(4,1), "
        "	testnumeric numeric(4,1), "
        "	testmoney money, "
        "	testsmallmoney smallmoney, "
        "	testdatetime datetime, "
        "	testsmalldatetime smalldatetime, "
        "	testchar char(40), "
        "	testvarchar varchar(40), "
        "	testbit bit)")),
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
                       "testsmallint"),
    assertEqualsString(sqlrelay:getFieldByName(2, "column_name"),
                       "testtinyint"),
    assertEqualsString(sqlrelay:getFieldByName(3, "column_name"), "testreal"),
    assertEqualsString(sqlrelay:getFieldByName(4, "column_name"), "testfloat"),
    assertEqualsString(sqlrelay:getFieldByName(5, "column_name"),
                       "testdecimal"),
    assertEqualsString(sqlrelay:getFieldByName(6, "column_name"),
                       "testnumeric"),
    assertEqualsString(sqlrelay:getFieldByName(7, "column_name"),
                       "testmoney"),
    assertEqualsString(sqlrelay:getFieldByName(8, "column_name"),
                       "testsmallmoney"),
    assertEqualsString(sqlrelay:getFieldByName(9, "column_name"),
                       "testdatetime"),
    assertEqualsString(sqlrelay:getFieldByName(10, "column_name"),
                       "testsmalldatetime"),
    assertEqualsString(sqlrelay:getFieldByName(11, "column_name"),
                       "testchar"),
    assertEqualsString(sqlrelay:getFieldByName(12, "column_name"),
                       "testvarchar"),
    assertEqualsString(sqlrelay:getFieldByName(13, "column_name"), "testbit"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "int"),
    assertEqualsString(sqlrelay:getFieldByName(1, "data_type"), "smallint"),
    assertEqualsString(sqlrelay:getFieldByName(2, "data_type"), "tinyint"),
    assertEqualsString(sqlrelay:getFieldByName(3, "data_type"), "real"),
    assertEqualsString(sqlrelay:getFieldByName(4, "data_type"), "float"),
    assertEqualsString(sqlrelay:getFieldByName(5, "data_type"), "decimal"),
    assertEqualsString(sqlrelay:getFieldByName(6, "data_type"), "numeric"),
    assertEqualsString(sqlrelay:getFieldByName(7, "data_type"), "money"),
    assertEqualsString(sqlrelay:getFieldByName(8, "data_type"), "smallmoney"),
    assertEqualsString(sqlrelay:getFieldByName(9, "data_type"), "datetime"),
    assertEqualsString(sqlrelay:getFieldByName(10, "data_type"),
                       "smalldatetime"),
    assertEqualsString(sqlrelay:getFieldByName(11, "data_type"), "char"),
    assertEqualsString(sqlrelay:getFieldByName(12, "data_type"), "varchar"),
    assertEqualsString(sqlrelay:getFieldByName(13, "data_type"), "bit"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% COLUMN LIST - auto_increment, primary key
    io:format("COLUMN LIST - auto_increment, primary key: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 int identity primary key, "
        "	col2 int)")),
    assertTrue(sqlrelay:getColumnList("testtable", "")),
    {ok, Ex0} = sqlrelay:getFieldByName(0, "extra"),
    assertTrue(contains(Ex0, "auto_increment")),
    {ok, Ck0} = sqlrelay:getFieldByName(0, "column_key"),
    assertTrue(contains(Ck0, "PRI")),
    {ok, Ex1} = sqlrelay:getFieldByName(1, "extra"),
    assertFalse(contains(Ex1, "auto_increment")),
    {ok, Ck1} = sqlrelay:getFieldByName(1, "column_key"),
    assertFalse(contains(Ck1, "PRI")),
    io:format("~n"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 int primary key, "
        "	col2 int)")),
    assertTrue(sqlrelay:getColumnList("testtable", "")),
    {ok, Ex0b} = sqlrelay:getFieldByName(0, "extra"),
    assertFalse(contains(Ex0b, "auto_increment")),
    {ok, Ck0b} = sqlrelay:getFieldByName(0, "column_key"),
    assertTrue(contains(Ck0b, "PRI")),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "non_unique"), "0"),
    assertEqualsString(sqlrelay:getFieldByName(0, "seq_in_index"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "column_name"), "col1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "collation"), "A"),
    assertEqualsString(sqlrelay:getFieldByName(0, "index_type"), "1"),
    {ok, KeyName2} = sqlrelay:getFieldByName(0, "key_name"),
    assertTrue(not isNullOrEmpty(KeyName2)),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% PROCEDURE LIST
    io:format("PROCEDURE LIST: ~n"),
    sqlrelay:sendQuery("drop procedure testproc1"),
    sqlrelay:sendQuery("drop procedure testproc2"),
    sqlrelay:sendQuery("drop procedure testproc3"),
    sqlrelay:sendQuery("drop procedure testproc4"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc1 "
        "	@in1 int, "
        "	@in2 char(20), "
        "	@in3 varchar(20), "
        "	@in4 datetime "
        "as select 1")),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc2 "
        "	@in1 int, "
        "	@in2 char(20), "
        "	@in3 varchar(20), "
        "	@in4 datetime "
        "as select 1")),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc3 "
        "	@in1 int, "
        "	@in2 char(20), "
        "	@in3 varchar(20), "
        "	@in4 datetime "
        "as select 1")),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc4 "
        "	@in1 int, "
        "	@in2 char(20), "
        "	@in3 varchar(20), "
        "	@in4 datetime "
        "as select 1")),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "parameter_name"), "@in1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "int"),
    assertEqualsString(sqlrelay:getFieldByName(0, "ordinal_position"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(1, "parameter_name"), "@in2"),
    assertEqualsString(sqlrelay:getFieldByName(1, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(1, "data_type"), "char"),
    assertEqualsString(sqlrelay:getFieldByName(1, "ordinal_position"), "2"),
    assertEqualsString(sqlrelay:getFieldByName(2, "parameter_name"), "@in3"),
    assertEqualsString(sqlrelay:getFieldByName(2, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(2, "data_type"), "varchar"),
    assertEqualsString(sqlrelay:getFieldByName(2, "ordinal_position"), "3"),
    assertEqualsString(sqlrelay:getFieldByName(3, "parameter_name"), "@in4"),
    assertEqualsString(sqlrelay:getFieldByName(3, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(3, "data_type"), "datetime"),
    assertEqualsString(sqlrelay:getFieldByName(3, "ordinal_position"), "4"),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc1")),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc2")),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc3")),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc4")),
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

%% case-insensitive null/empty check
isNullOrEmpty(undefined) -> true;
isNullOrEmpty(null)      -> true;
isNullOrEmpty([])        -> true;
isNullOrEmpty(_)         -> false.

%% String substring containment: does Haystack contain Needle ?
contains(Haystack, Needle) when is_list(Haystack), is_list(Needle) ->
    string:str(Haystack, Needle) > 0;
contains(_, _) ->
    false.

%% Walk the table-type-list result looking for a "TABLE" entry.
findTableType(I, Count) when I >= Count ->
    false;
findTableType(I, Count) ->
    case sqlrelay:getFieldByName(I, "table_type") of
        {ok, "TABLE"} -> true;
        _             -> findTableType(I + 1, Count)
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
