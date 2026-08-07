%% Copyright (c) David Muse
%% See the file COPYING for more information.

-module(db2).
-export([main/0]).

-import(asserts, [pass/0, fail/2,
                  getStatus/0, reportTestStatus/0,
                  assertEqualsString/2, assertEqualsStringLen/3,
                  assertStartsWith/2,
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
    {ok, _} = sqlrelay:alloc("sqlrelay", 9008, "/tmp/db2test.socket",
                             "db2inst1", "testpassword", 0, 1),

    Hostname = shortHostname(),

    %% IDENTIFY
    io:format("IDENTIFY: ~n"),
    assertEqualsString(sqlrelay:identify(), "db2"),
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
    assertEqualsString(sqlrelay:nextvalFormat(), "(nextval for %s)"),
    io:format("~n"),

    %% ISOLATION LEVELS
    io:format("ISOLATION LEVELS: ~n"),
    IsolationLevels = ["CS", "UR", "RS", "RR"],
    setIsolationLevels(IsolationLevels),
    %% reset to the default isolation level
    assertTrue(sqlrelay:setIsolationLevel(hd(IsolationLevels))),
    io:format("~n"),

    %% CREATE TESTTABLE
    io:format("CREATE TESTTABLE: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	testsmallint smallint, "
        "	testint integer, "
        "	testbigint bigint, "
        "	testdecimal decimal(10,2), "
        "	testreal real, "
        "	testdouble double, "
        "	testchar char(40), "
        "	testvarchar varchar(40), "
        "	testdate date, "
        "	testtime time, "
        "	testtimestamp timestamp, "
        "	testclob clob, "
        "	testblob blob)")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% INSERT
    io:format("INSERT: ~n"),
    assertTrue(sqlrelay:sendQuery(
        "insert into "
        "	testtable "
        "values ("
        "	1, "
        "	1, "
        "	1, "
        "	1.5, "
        "	1.5, "
        "	1.5, "
        "	'testchar1', "
        "	'testvarchar1', "
        "	'01/01/2001', "
        "	'01:00:00', "
        "	NULL, "
        "	'testclob1', "
        "	blob('testblob1'))")),
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
        "	NULL, "
        "	?, "
        "	?)"),
    assertEqualsInt(sqlrelay:countBindVariables(), 12),
    sqlrelay:inputBindLong("1", 2),
    sqlrelay:inputBindLong("2", 2),
    sqlrelay:inputBindLong("3", 2),
    sqlrelay:inputBindDouble("4", 2.5, 4, 2),
    sqlrelay:inputBindDouble("5", 2.5, 4, 2),
    sqlrelay:inputBindDouble("6", 2.5, 4, 2),
    sqlrelay:inputBindString("7", "testchar2"),
    sqlrelay:inputBindString("8", "testvarchar2"),
    sqlrelay:inputBindDate("9", 2002, 1, 1, -1, -1, -1, -1, "", 0),
    sqlrelay:inputBindDate("10", -1, -1, -1, 2, 0, 0, 0, "", 0),
    sqlrelay:inputBindClob("11", "testclob2", 9),
    sqlrelay:inputBindBlob("12", "testblob2", 9),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 3),
    sqlrelay:inputBindLong("2", 3),
    sqlrelay:inputBindLong("3", 3),
    sqlrelay:inputBindDouble("4", 3.5, 4, 2),
    sqlrelay:inputBindDouble("5", 3.5, 4, 2),
    sqlrelay:inputBindDouble("6", 3.5, 4, 2),
    sqlrelay:inputBindString("7", "testchar3"),
    sqlrelay:inputBindString("8", "testvarchar3"),
    sqlrelay:inputBindDate("9", 2003, 1, 1, -1, -1, -1, -1, "", 0),
    sqlrelay:inputBindDate("10", -1, -1, -1, 3, 0, 0, 0, "", 0),
    sqlrelay:inputBindClob("11", "testclob3", 9),
    sqlrelay:inputBindBlob("12", "testblob3", 9),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 4),
    sqlrelay:inputBindLong("2", 4),
    sqlrelay:inputBindLong("3", 4),
    sqlrelay:inputBindDouble("4", 4.5, 4, 2),
    sqlrelay:inputBindDouble("5", 4.5, 4, 2),
    sqlrelay:inputBindDouble("6", 4.5, 4, 2),
    sqlrelay:inputBindString("7", "testchar4"),
    sqlrelay:inputBindString("8", "testvarchar4"),
    sqlrelay:inputBindDate("9", 2004, 1, 1, -1, -1, -1, -1, "", 0),
    sqlrelay:inputBindDate("10", -1, -1, -1, 4, 0, 0, 0, "", 0),
    sqlrelay:inputBindClob("11", "testclob4", 9),
    sqlrelay:inputBindBlob("12", "testblob4", 9),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 5),
    sqlrelay:inputBindLong("2", 5),
    sqlrelay:inputBindLong("3", 5),
    sqlrelay:inputBindDouble("4", 5.5, 4, 2),
    sqlrelay:inputBindDouble("5", 5.5, 4, 2),
    sqlrelay:inputBindDouble("6", 5.5, 4, 2),
    sqlrelay:inputBindString("7", "testchar5"),
    sqlrelay:inputBindString("8", "testvarchar5"),
    sqlrelay:inputBindDate("9", 2005, 1, 1, -1, -1, -1, -1, "", 0),
    sqlrelay:inputBindDate("10", -1, -1, -1, 5, 0, 0, 0, "", 0),
    sqlrelay:inputBindClob("11", "testclob5", 9),
    sqlrelay:inputBindBlob("12", "testblob5", 9),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 6),
    sqlrelay:inputBindLong("2", 6),
    sqlrelay:inputBindLong("3", 6),
    sqlrelay:inputBindDouble("4", 6.5, 4, 2),
    sqlrelay:inputBindDouble("5", 6.5, 4, 2),
    sqlrelay:inputBindDouble("6", 6.5, 4, 2),
    sqlrelay:inputBindString("7", "testchar6"),
    sqlrelay:inputBindString("8", "testvarchar6"),
    sqlrelay:inputBindDate("9", 2006, 1, 1, -1, -1, -1, -1, "", 0),
    sqlrelay:inputBindDate("10", -1, -1, -1, 6, 0, 0, 0, "", 0),
    sqlrelay:inputBindClob("11", "testclob6", 9),
    sqlrelay:inputBindBlob("12", "testblob6", 9),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% ARRAY OF INPUT BINDS BY POSITION
    %% The Erlang binding has no array-inputBinds(); do the individual
    %% binds for row 7 manually to keep the section faithful.
    io:format("ARRAY OF INPUT BINDS BY POSITION: ~n"),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindString("1", "7"),
    sqlrelay:inputBindString("2", "7"),
    sqlrelay:inputBindString("3", "7"),
    sqlrelay:inputBindString("4", "7.5"),
    sqlrelay:inputBindString("5", "7.5"),
    sqlrelay:inputBindString("6", "7.5"),
    sqlrelay:inputBindString("7", "testchar7"),
    sqlrelay:inputBindString("8", "testvarchar7"),
    sqlrelay:inputBindString("9", "01/01/2007"),
    sqlrelay:inputBindString("10", "07:00:00"),
    sqlrelay:inputBindString("11", "testclob7"),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% INPUT BIND BY POSITION WITH VALIDATION
    io:format("INPUT BIND BY POSITION WITH VALIDATION: ~n"),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 8),
    sqlrelay:inputBindLong("2", 8),
    sqlrelay:inputBindLong("3", 8),
    sqlrelay:inputBindDouble("4", 8.5, 4, 2),
    sqlrelay:inputBindDouble("5", 8.5, 4, 2),
    sqlrelay:inputBindDouble("6", 8.5, 4, 2),
    sqlrelay:inputBindString("7", "testchar8"),
    sqlrelay:inputBindString("8", "testvarchar8"),
    sqlrelay:inputBindDate("9", 2008, 1, 1, -1, -1, -1, -1, "", 0),
    sqlrelay:inputBindDate("10", -1, -1, -1, 8, 0, 0, 0, "", 0),
    sqlrelay:inputBindClob("11", "testclob8", 9),
    sqlrelay:inputBindBlob("12", "testblob8", 9),
    sqlrelay:validateBinds(),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% INPUT BIND BY NAME
    %% db2 doesn't support bind by name

    %% ARRAY OF INPUT BINDS BY NAME
    %% db2 doesn't support bind by name

    %% INPUT BIND BY NAME WITH VALIDATION
    %% db2 doesn't support bind by name

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
    assertEqualsInt(sqlrelay:colCount(), 13),
    io:format("~n"),

    %% COLUMN NAMES
    io:format("COLUMN NAMES: ~n"),
    assertEqualsString(sqlrelay:getColumnName(0), "TESTSMALLINT"),
    assertEqualsString(sqlrelay:getColumnName(1), "TESTINT"),
    assertEqualsString(sqlrelay:getColumnName(2), "TESTBIGINT"),
    assertEqualsString(sqlrelay:getColumnName(3), "TESTDECIMAL"),
    assertEqualsString(sqlrelay:getColumnName(4), "TESTREAL"),
    assertEqualsString(sqlrelay:getColumnName(5), "TESTDOUBLE"),
    assertEqualsString(sqlrelay:getColumnName(6), "TESTCHAR"),
    assertEqualsString(sqlrelay:getColumnName(7), "TESTVARCHAR"),
    assertEqualsString(sqlrelay:getColumnName(8), "TESTDATE"),
    assertEqualsString(sqlrelay:getColumnName(9), "TESTTIME"),
    assertEqualsString(sqlrelay:getColumnName(10), "TESTTIMESTAMP"),
    {ok, Cols1} = sqlrelay:getColumnNames(),
    assertEqualsString(lists:nth(1, Cols1), "TESTSMALLINT"),
    assertEqualsString(lists:nth(2, Cols1), "TESTINT"),
    assertEqualsString(lists:nth(3, Cols1), "TESTBIGINT"),
    assertEqualsString(lists:nth(4, Cols1), "TESTDECIMAL"),
    assertEqualsString(lists:nth(5, Cols1), "TESTREAL"),
    assertEqualsString(lists:nth(6, Cols1), "TESTDOUBLE"),
    assertEqualsString(lists:nth(7, Cols1), "TESTCHAR"),
    assertEqualsString(lists:nth(8, Cols1), "TESTVARCHAR"),
    assertEqualsString(lists:nth(9, Cols1), "TESTDATE"),
    assertEqualsString(lists:nth(10, Cols1), "TESTTIME"),
    assertEqualsString(lists:nth(11, Cols1), "TESTTIMESTAMP"),
    io:format("~n"),

    %% COLUMN TYPES
    io:format("COLUMN TYPES: ~n"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(0), "SMALLINT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTSMALLINT"), "SMALLINT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(1), "INTEGER"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTINT"), "INTEGER"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(2), "BIGINT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTBIGINT"), "BIGINT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(3), "DECIMAL"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTDECIMAL"), "DECIMAL"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(4), "REAL"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTREAL"), "REAL"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(5), "DOUBLE"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTDOUBLE"), "DOUBLE"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(6), "CHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTCHAR"), "CHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(7), "VARCHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTVARCHAR"), "VARCHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(8), "DATE"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTDATE"), "DATE"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(9), "TIME"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTTIME"), "TIME"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(10), "TIMESTAMP"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTTIMESTAMP"), "TIMESTAMP"),
    io:format("~n"),

    %% COLUMN LENGTH
    io:format("COLUMN LENGTH: ~n"),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 2),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTSMALLINT"), 2),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(1), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTINT"), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(2), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTBIGINT"), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(3), 12),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTDECIMAL"), 12),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(4), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTREAL"), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(5), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTDOUBLE"), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(6), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTCHAR"), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(7), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTVARCHAR"), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(8), 6),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTDATE"), 6),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(9), 6),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTTIME"), 6),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(10), 16),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTTIMESTAMP"), 16),
    io:format("~n"),

    %% LONGEST COLUMN
    io:format("LONGEST COLUMN: ~n"),
    assertEqualsInt(sqlrelay:getLongestByIndex(0), 1),
    assertEqualsInt(sqlrelay:getLongestByName("TESTSMALLINT"), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(1), 1),
    assertEqualsInt(sqlrelay:getLongestByName("TESTINT"), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(2), 1),
    assertEqualsInt(sqlrelay:getLongestByName("TESTBIGINT"), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(3), 4),
    assertEqualsInt(sqlrelay:getLongestByName("TESTDECIMAL"), 4),
    assertEqualsInt(sqlrelay:getLongestByIndex(4), 12),
    assertEqualsInt(sqlrelay:getLongestByName("TESTREAL"), 12),
    assertEqualsInt(sqlrelay:getLongestByIndex(5), 21),
    assertEqualsInt(sqlrelay:getLongestByName("TESTDOUBLE"), 21),
    assertEqualsInt(sqlrelay:getLongestByIndex(6), 40),
    assertEqualsInt(sqlrelay:getLongestByName("TESTCHAR"), 40),
    assertEqualsInt(sqlrelay:getLongestByIndex(7), 12),
    assertEqualsInt(sqlrelay:getLongestByName("TESTVARCHAR"), 12),
    assertEqualsInt(sqlrelay:getLongestByIndex(8), 10),
    assertEqualsInt(sqlrelay:getLongestByName("TESTDATE"), 10),
    assertEqualsInt(sqlrelay:getLongestByIndex(9), 8),
    assertEqualsInt(sqlrelay:getLongestByName("TESTTIME"), 8),
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
    assertEqualsString(sqlrelay:getFieldByIndex(0, 3), "1.50"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 4), "1.500000E+00"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 5), "1.50000000000000E+000"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 6),
                       "testchar1                               "),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 7), "testvarchar1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 8), "2001-01-01"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 9), "01:00:00"),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 2), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 3), "8.50"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 4), "8.500000E+00"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 5), "8.50000000000000E+000"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 6),
                       "testchar8                               "),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 7), "testvarchar8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 8), "2008-01-01"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 9), "08:00:00"),
    io:format("~n"),

    %% FIELD LENGTHS BY INDEX
    io:format("FIELD LENGTHS BY INDEX: ~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 1), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 2), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 3), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 4), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 5), 21),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 6), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 7), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 8), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 9), 8),
    io:format("~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 0), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 1), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 2), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 3), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 4), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 5), 21),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 6), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 7), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 8), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 9), 8),
    io:format("~n"),

    %% FIELDS BY NAME
    io:format("FIELDS BY NAME: ~n"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTSMALLINT"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTINT"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTBIGINT"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTDECIMAL"), "1.50"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTREAL"), "1.500000E+00"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTDOUBLE"),
                       "1.50000000000000E+000"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTCHAR"),
                       "testchar1                               "),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTVARCHAR"), "testvarchar1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTDATE"), "2001-01-01"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTTIME"), "01:00:00"),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTSMALLINT"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTINT"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTBIGINT"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTDECIMAL"), "8.50"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTREAL"), "8.500000E+00"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTDOUBLE"),
                       "8.50000000000000E+000"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTCHAR"),
                       "testchar8                               "),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTVARCHAR"), "testvarchar8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTDATE"), "2008-01-01"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTTIME"), "08:00:00"),
    io:format("~n"),

    %% FIELD LENGTHS BY NAME
    io:format("FIELD LENGTHS BY NAME: ~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTSMALLINT"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTINT"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTBIGINT"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTDECIMAL"), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTREAL"), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTDOUBLE"), 21),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTCHAR"), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTVARCHAR"), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTDATE"), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTTIME"), 8),
    io:format("~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTSMALLINT"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTINT"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTBIGINT"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTDECIMAL"), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTREAL"), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTDOUBLE"), 21),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTCHAR"), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTVARCHAR"), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTDATE"), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTTIME"), 8),
    io:format("~n"),

    %% FIELDS BY ARRAY
    io:format("FIELDS BY ARRAY: ~n"),
    {ok, Row0} = sqlrelay:getRow(0),
    assertEqualsString(lists:nth(1, Row0), "1"),
    assertEqualsString(lists:nth(2, Row0), "1"),
    assertEqualsString(lists:nth(3, Row0), "1"),
    assertEqualsString(lists:nth(4, Row0), "1.50"),
    assertEqualsString(lists:nth(5, Row0), "1.500000E+00"),
    assertEqualsString(lists:nth(6, Row0), "1.50000000000000E+000"),
    assertEqualsString(lists:nth(7, Row0),
                       "testchar1                               "),
    assertEqualsString(lists:nth(8, Row0), "testvarchar1"),
    assertEqualsString(lists:nth(9, Row0), "2001-01-01"),
    assertEqualsString(lists:nth(10, Row0), "01:00:00"),
    io:format("~n"),

    %% FIELD LENGTHS BY ARRAY
    io:format("FIELD LENGTHS BY ARRAY: ~n"),
    {ok, Rowlens0} = sqlrelay:getRowLengths(0),
    assertEqualsInt(lists:nth(1, Rowlens0), 1),
    assertEqualsInt(lists:nth(2, Rowlens0), 1),
    assertEqualsInt(lists:nth(3, Rowlens0), 1),
    assertEqualsInt(lists:nth(4, Rowlens0), 4),
    assertEqualsInt(lists:nth(5, Rowlens0), 12),
    assertEqualsInt(lists:nth(6, Rowlens0), 21),
    assertEqualsInt(lists:nth(7, Rowlens0), 40),
    assertEqualsInt(lists:nth(8, Rowlens0), 12),
    assertEqualsInt(lists:nth(9, Rowlens0), 10),
    assertEqualsInt(lists:nth(10, Rowlens0), 8),
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
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    assertEqualsString(sqlrelay:getColumnName(0), null),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 0),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(0), null),
    sqlrelay:getColumnInfo(),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    assertEqualsString(sqlrelay:getColumnName(0), "TESTSMALLINT"),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 2),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(0), "SMALLINT"),
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
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
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
    sqlrelay:cacheToFile("cachefile1-db2"),
    sqlrelay:setCacheTtl(200),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    {ok, Filename1} = sqlrelay:getCacheFileName(),
    assertEqualsString(Filename1, "cachefile1-db2"),
    sqlrelay:cacheOff(),
    assertTrue(sqlrelay:openCachedResultSet(Filename1)),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    io:format("~n"),

    %% COLUMN COUNT FOR CACHED RESULT SET
    io:format("COLUMN COUNT FOR CACHED RESULT SET: ~n"),
    assertEqualsInt(sqlrelay:colCount(), 13),
    io:format("~n"),

    %% COLUMN NAMES FOR CACHED RESULT SET
    io:format("COLUMN NAMES FOR CACHED RESULT SET: ~n"),
    assertEqualsString(sqlrelay:getColumnName(0), "TESTSMALLINT"),
    assertEqualsString(sqlrelay:getColumnName(1), "TESTINT"),
    assertEqualsString(sqlrelay:getColumnName(2), "TESTBIGINT"),
    assertEqualsString(sqlrelay:getColumnName(3), "TESTDECIMAL"),
    assertEqualsString(sqlrelay:getColumnName(4), "TESTREAL"),
    assertEqualsString(sqlrelay:getColumnName(5), "TESTDOUBLE"),
    assertEqualsString(sqlrelay:getColumnName(6), "TESTCHAR"),
    assertEqualsString(sqlrelay:getColumnName(7), "TESTVARCHAR"),
    assertEqualsString(sqlrelay:getColumnName(8), "TESTDATE"),
    assertEqualsString(sqlrelay:getColumnName(9), "TESTTIME"),
    assertEqualsString(sqlrelay:getColumnName(10), "TESTTIMESTAMP"),
    {ok, Cols2} = sqlrelay:getColumnNames(),
    assertEqualsString(lists:nth(1, Cols2), "TESTSMALLINT"),
    assertEqualsString(lists:nth(2, Cols2), "TESTINT"),
    assertEqualsString(lists:nth(3, Cols2), "TESTBIGINT"),
    assertEqualsString(lists:nth(4, Cols2), "TESTDECIMAL"),
    assertEqualsString(lists:nth(5, Cols2), "TESTREAL"),
    assertEqualsString(lists:nth(6, Cols2), "TESTDOUBLE"),
    assertEqualsString(lists:nth(7, Cols2), "TESTCHAR"),
    assertEqualsString(lists:nth(8, Cols2), "TESTVARCHAR"),
    assertEqualsString(lists:nth(9, Cols2), "TESTDATE"),
    assertEqualsString(lists:nth(10, Cols2), "TESTTIME"),
    assertEqualsString(lists:nth(11, Cols2), "TESTTIMESTAMP"),
    io:format("~n"),

    %% CACHED RESULT SET WITH RESULT SET BUFFER SIZE
    io:format("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ~n"),
    sqlrelay:setResultSetBufferSize(2),
    sqlrelay:cacheToFile("cachefile1-db2"),
    sqlrelay:setCacheTtl(200),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    {ok, Filename2} = sqlrelay:getCacheFileName(),
    assertEqualsString(Filename2, "cachefile1-db2"),
    sqlrelay:cacheOff(),
    assertTrue(sqlrelay:openCachedResultSet(Filename2)),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(8, 0), null),
    sqlrelay:setResultSetBufferSize(0),
    io:format("~n"),

    %% FROM ONE CACHE FILE TO ANOTHER
    io:format("FROM ONE CACHE FILE TO ANOTHER: ~n"),
    sqlrelay:cacheToFile("cachefile2-db2"),
    assertTrue(sqlrelay:openCachedResultSet("cachefile1-db2")),
    sqlrelay:cacheOff(),
    assertTrue(sqlrelay:openCachedResultSet("cachefile2-db2")),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(8, 0), null),
    io:format("~n"),

    %% FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET BUFFER SIZE
    io:format("FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET BUFFER SIZE: ~n"),
    sqlrelay:setResultSetBufferSize(2),
    sqlrelay:cacheToFile("cachefile2-db2"),
    assertTrue(sqlrelay:openCachedResultSet("cachefile1-db2")),
    sqlrelay:cacheOff(),
    assertTrue(sqlrelay:openCachedResultSet("cachefile2-db2")),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(8, 0), null),
    sqlrelay:setResultSetBufferSize(0),
    io:format("~n"),

    %% CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE
    io:format("CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE: ~n"),
    sqlrelay:setResultSetBufferSize(2),
    sqlrelay:cacheToFile("cachefile1-db2"),
    sqlrelay:setCacheTtl(200),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testsmallint ")),
    assertEqualsString(sqlrelay:getFieldByIndex(2, 0), "3"),
    {ok, Filename3} = sqlrelay:getCacheFileName(),
    assertEqualsString(Filename3, "cachefile1-db2"),
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
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testint")),
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
    %% isolation. The Erlang binding only supports one connection per
    %% process (see sqlrelay.erl: alloc/connectionFree use a single
    %% connection slot in the process dictionary), so a second
    %% connection cannot be instantiated here.
    io:format("TRANSACTION BEHAVIOR - implicit/explicit/explicit-deferred/explicit-error/none: ~n"),
    io:format("(skipped - requires second concurrent connection)~n"),
    %% Drop the leftover testtable and restore autoCommitOff so
    %% subsequent sections start clean.
    assertTrue(sqlrelay:commit()),
    assertTrue(sqlrelay:autoCommitOff()),
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
    sqlrelay:prepareQuery("values ($(var1),'$(var2)','$(var3)')"),
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
    sqlrelay:prepareQuery("values ('$(var1)','$(var2)','$(var3)')"),
    sqlrelay:subString("var1", "hi"),
    sqlrelay:subString("var2", "hello"),
    sqlrelay:subString("var3", "bye"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "hi"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "hello"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "bye"),
    io:format("~n"),
    sqlrelay:prepareQuery("values ($(var1),$(var2),$(var3))"),
    sqlrelay:subLong("var1", 1),
    sqlrelay:subLong("var2", 2),
    sqlrelay:subLong("var3", 3),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "2"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "3"),
    io:format("~n"),
    sqlrelay:prepareQuery("values ($(var1),$(var2),$(var3))"),
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
    assertTrue(sqlrelay:sendQuery("select NULL,1,NULL from sysibm.sysdummy1")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), null),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), null),
    sqlrelay:getNullsAsEmptyStrings(),
    assertTrue(sqlrelay:sendQuery("select NULL,1,NULL from sysibm.sysdummy1")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), ""),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), ""),
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
    sqlrelay:inputBindClob("2", "", 0),
    sqlrelay:inputBindBlob("3", "", 0),
    sqlrelay:inputBindBlob("4", "", 0),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("select * from testtable"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), ""),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), ""),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), ""),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 3), ""),
    sqlrelay:getNullsAsEmptyStrings(),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% LONG LOBS
    io:format("LONG LOBS: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	testclob clob, "
        "	testblob blob)")),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("insert into testtable values (?,?)"),
    LargeBufferLength = 20480,
    LargeBuf = largeBuffer(LargeBufferLength),
    sqlrelay:inputBindClob("1", LargeBuf, LargeBufferLength),
    sqlrelay:inputBindBlob("2", LargeBuf, LargeBufferLength),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("select * from testtable"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTCLOB"),
                    LargeBufferLength),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTCLOB"), LargeBuf),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTBLOB"),
                    LargeBufferLength),
    assertEqualsStringLen(sqlrelay:getFieldByName(0, "TESTBLOB"),
                          LargeBuf, LargeBufferLength),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% OUTPUT BIND BY POSITION
    io:format("OUTPUT BIND BY POSITION: ~n"),
    sqlrelay:sendQuery("drop procedure testproc"),
    sqlrelay:getNullsAsNulls(),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc("
        "	out out1 int, "
        "	out out2 varchar(20), "
        "	out out3 double, "
        "	out out4 date, "
        "	out out5 varchar(20)) "
        "language sql "
        "begin "
        "	set out1 = 1; "
        "	set out2 = 'hello'; "
        "	set out3 = 2.5; "
        "	set out4 = '2001-02-03'; "
        "	set out5 = null; "
        "end")),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("call testproc(?,?,?,?,?)"),
    assertEqualsInt(sqlrelay:countBindVariables(), 5),
    sqlrelay:defineOutputBindInteger("1"),
    sqlrelay:defineOutputBindString("2", 20),
    sqlrelay:defineOutputBindDouble("3"),
    sqlrelay:defineOutputBindDate("4"),
    sqlrelay:defineOutputBindString("5", 20),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("1"), 1),
    assertEqualsString(sqlrelay:getOutputBindString("2"), "hello"),
    assertEqualsDouble(sqlrelay:getOutputBindDouble("3"), 2.5),
    assertEqualsInt(sqlrelay:getOutputBindDateYear("4"), 2001),
    assertEqualsInt(sqlrelay:getOutputBindDateMonth("4"), 2),
    assertEqualsInt(sqlrelay:getOutputBindDateDay("4"), 3),
    assertEqualsInt(sqlrelay:getOutputBindDateHour("4"), 0),
    assertEqualsInt(sqlrelay:getOutputBindDateMinute("4"), 0),
    assertEqualsInt(sqlrelay:getOutputBindDateSecond("4"), 0),
    assertEqualsInt(sqlrelay:getOutputBindDateMicrosecond("4"), 0),
    assertEqualsString(sqlrelay:getOutputBindDateTz("4"), ""),
    assertFalse(sqlrelay:getOutputBindDateIsNegative("4")),
    assertEqualsString(sqlrelay:getOutputBindString("5"), null),
    sqlrelay:getNullsAsEmptyStrings(),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% OUTPUT BIND BY NAME
    %% db2 doesn't support bind by name

    %% OUTPUT BIND BY NAME WITH VALIDATION
    %% db2 doesn't support bind by name

    %% LOB OUTPUT BIND
    io:format("LOB OUTPUT BIND: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    sqlrelay:sendQuery(
        "create table testtable ("
        "	testclob clob, "
        "	testblob blob)"),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("insert into testtable values ('hello',?)"),
    sqlrelay:inputBindBlob("1", "hello", 5),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc("
        "	out out1 clob, "
        "	out out2 blob) "
        "language sql "
        "begin "
        "	select testclob into out1 from testtable; "
        "	select testblob into out2 from testtable; "
        "end")),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("call testproc(?,?)"),
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
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc("
        "	in in1 clob, "
        "	out out1 clob) "
        "language sql "
        "begin "
        "	set out1 = in1; "
        "end")),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("call testproc(?,?)"),
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
    sqlrelay:sendQuery("create table testtable (testval integer)"),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("insert into testtable values (?)"),
    sqlrelay:inputBindLong("1", -1),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("select testval from testtable"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTVAL"), "-1"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% BIND VALIDATION
    %% db2 doesn't support bind by name

    %% REBINDING
    io:format("REBINDING: ~n"),
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc("
        "	in in1 int, "
        "	out out1 int) "
        "language sql "
        "begin "
        "	set out1 = in1; "
        "end")),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("call testproc(?,?)"),
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
    sqlrelay:prepareQuery("select 1 from sysibm.sysdummy1"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:rowCount(), 1),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    io:format("~n"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:rowCount(), 1),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    io:format("~n"),
    sqlrelay:prepareQuery("select cast(? as integer) from sysibm.sysdummy1"),
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
        "	in in1 int, "
        "	in in2 double, "
        "	in in3 varchar(20)) "
        "language sql "
        "begin "
        "	return; "
        "end")),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("call testproc(?,?,?)"),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:inputBindDouble("2", 2.5, 2, 1),
    sqlrelay:inputBindString("3", "hello"),
    assertTrue(sqlrelay:executeQuery()),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING SINGLE VALUE
    io:format("STORED PROCEDURE RETURNING SINGLE VALUE: ~n"),
    sqlrelay:sendQuery("drop function testfunc"),
    assertTrue(sqlrelay:sendQuery(
        "create function testfunc("
        "	in1 int, "
        "	in2 double, "
        "	in3 varchar(20)) "
        "returns int "
        "language sql "
        "begin "
        "	return in1; "
        "end")),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("select testfunc(?,?,?) from sysibm.sysdummy1"),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:inputBindDouble("2", 2.5, 2, 1),
    sqlrelay:inputBindString("3", "hello"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertTrue(sqlrelay:sendQuery("drop function testfunc")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING MULTIPLE VALUES
    io:format("STORED PROCEDURE RETURNING MULTIPLE VALUES: ~n"),
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc("
        "	in in1 int, "
        "	in in2 double, "
        "	in in3 varchar(20), "
        "	in in4 clob, "
        "	in in5 blob, "
        "	out out1 int, "
        "	out out2 double, "
        "	out out3 varchar(20), "
        "	out out4 clob, "
        "	out out5 blob) "
        "language sql "
        "begin "
        "	set out1 = in1; "
        "	set out2 = in2; "
        "	set out3 = in3; "
        "	set out4 = in4; "
        "	set out5 = in5; "
        "end")),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("call testproc(?,?,?,?,?,?,?,?,?,?)"),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:inputBindDouble("2", 2.5, 2, 1),
    sqlrelay:inputBindString("3", "hello"),
    sqlrelay:inputBindClob("4", "clob", 4),
    sqlrelay:inputBindBlob("5", "blob", 4),
    sqlrelay:defineOutputBindInteger("6"),
    sqlrelay:defineOutputBindDouble("7"),
    sqlrelay:defineOutputBindString("8", 20),
    sqlrelay:defineOutputBindClob("9"),
    sqlrelay:defineOutputBindBlob("10"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("6"), 1),
    assertEqualsDouble(sqlrelay:getOutputBindDouble("7"), 2.5),
    assertEqualsString(sqlrelay:getOutputBindString("8"), "hello"),
    assertEqualsString(sqlrelay:getOutputBindClob("9"), "clob"),
    assertEqualsString(sqlrelay:getOutputBindBlob("10"), "blob"),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING RESULT SET
    io:format("STORED PROCEDURE RETURNING RESULT SET: ~n"),
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc() "
        "result set 1 "
        "language sql "
        "begin "
        "	declare c1 cursor with return for "
        "		select 1 from sysibm.sysdummy1 "
        "		union "
        "		select 2 from sysibm.sysdummy1 "
        "		union "
        "		select 3 from sysibm.sysdummy1 "
        "		union "
        "		select 4 from sysibm.sysdummy1 "
        "		union "
        "		select 5 from sysibm.sysdummy1 "
        "		union "
        "		select 6 from sysibm.sysdummy1 "
        "		union "
        "		select 7 from sysibm.sysdummy1 "
        "		union "
        "		select 8 from sysibm.sysdummy1; "
        "	open c1; "
        "end")),
    assertTrue(sqlrelay:commit()),
    assertTrue(sqlrelay:sendQuery("call testproc()")),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% TEMPORARY TABLES
    io:format("TEMPORARY TABLES: ~n"),
    sqlrelay:sendQuery("drop table session.temptable"),
    assertTrue(sqlrelay:sendQuery(
        "declare global temporary table session.temptable ("
        "	col1 int "
        ") not logged")),
    assertTrue(sqlrelay:sendQuery("insert into session.temptable values (1)")),
    assertTrue(sqlrelay:sendQuery("select count(*) from session.temptable")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    sqlrelay:endSession(),
    io:format("~n"),
    assertFalse(sqlrelay:sendQuery("select count(*) from session.temptable")),
    io:format("~n"),

    %% declared temp table with an unqualified name; session. must be
    %% prepended for the end-of-session drop to succeed
    sqlrelay:sendQuery("drop table session.temptable"),
    assertTrue(sqlrelay:sendQuery(
        "declare global temporary table temptable ("
        "	col1 int "
        ") not logged")),
    assertTrue(sqlrelay:sendQuery("insert into session.temptable values (1)")),
    assertTrue(sqlrelay:sendQuery("select count(*) from session.temptable")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    sqlrelay:endSession(),
    io:format("~n"),
    assertFalse(sqlrelay:sendQuery("select count(*) from session.temptable")),
    io:format("~n"),

    %% created temp table; its rows are truncated rather than the table
    %% being dropped at the end of the session (it isn't dropped here -
    %% dropping a still-instantiated created temp table hangs in db2)
    sqlrelay:sendQuery("drop table ctemptable"),
    assertTrue(sqlrelay:sendQuery(
        "create global temporary table ctemptable ("
        "	col1 int "
        ") on commit preserve rows")),
    assertTrue(sqlrelay:sendQuery("insert into ctemptable values (1)")),
    assertTrue(sqlrelay:sendQuery("select count(*) from ctemptable")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    sqlrelay:endSession(),
    io:format("~n"),
    assertTrue(sqlrelay:sendQuery("select count(*) from ctemptable")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "0"),
    io:format("~n"),

    %% ENCODED BINARY DATA
    io:format("ENCODED BINARY DATA: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery("create table testtable (col1 blob)")),
    Buffer = lists:seq(0, 255),
    HexStr = lists:flatten([io_lib:format("~2.16.0b", [B]) || B <- Buffer]),
    QueryStr = "insert into testtable values (blob(X'" ++ HexStr ++ "'))",
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
            "	(col1 int not null "
            "	generated always as identity, "
            "	col2 int, "
            "	primary key(col1))")),
    assertTrue(sqlrelay:sendQuery(
            "insert into testtable (col2) values (1)")),
    assertEqualsInt(sqlrelay:getLastInsertId(), 1),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% DATABASE IS SCHEMA
    io:format("DATABASE IS SCHEMA: ~n"),
    assertTrue(sqlrelay:getDatabaseIsSchema()),
    io:format("~n"),

    %% CATALOG LIST
    io:format("CATALOG LIST: ~n"),
    assertTrue(sqlrelay:getCatalogList("")),
    assertEqualsString(sqlrelay:getColumnName(0), "Database"),
    assertEqualsInt(sqlrelay:rowCount(), 0),
    io:format("~n"),

    %% SCHEMA LIST
    io:format("SCHEMA LIST: ~n"),
    assertTrue(sqlrelay:getSchemaList("")),
    assertEqualsString(sqlrelay:getColumnName(0), "Database"),
    assertInResultSet("Database", "DB2INST1"),
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
    assertInResultSet("Tables_in_xxx", "TESTTABLE1"),
    assertInResultSet("Tables_in_xxx", "TESTTABLE2"),
    assertInResultSet("Tables_in_xxx", "TESTTABLE3"),
    assertInResultSet("Tables_in_xxx", "TESTTABLE4"),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"), "INTEGER"),
    assertTrue(sqlrelay:getTypeInfoList("char")),
    assertEqualsString(sqlrelay:getFieldByName(0, "type_name"), "CHAR"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "254"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"), "CHAR"),
    assertTrue(sqlrelay:getTypeInfoList("varchar")),
    assertEqualsString(sqlrelay:getFieldByName(0, "type_name"), "VARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "12"),
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "32672"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"), "VARCHAR"),
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
        "	testsmallint smallint, "
        "	testint integer, "
        "	testbigint bigint, "
        "	testdecimal decimal(10,2), "
        "	testreal real, "
        "	testdouble double, "
        "	testchar char(40), "
        "	testvarchar varchar(40), "
        "	testdate date, "
        "	testtime time, "
        "	testtimestamp timestamp, "
        "	testclob clob, "
        "	testblob blob)")),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "column_name"), "TESTSMALLINT"),
    assertEqualsString(sqlrelay:getFieldByName(1, "column_name"), "TESTINT"),
    assertEqualsString(sqlrelay:getFieldByName(2, "column_name"), "TESTBIGINT"),
    assertEqualsString(sqlrelay:getFieldByName(3, "column_name"), "TESTDECIMAL"),
    assertEqualsString(sqlrelay:getFieldByName(4, "column_name"), "TESTREAL"),
    assertEqualsString(sqlrelay:getFieldByName(5, "column_name"), "TESTDOUBLE"),
    assertEqualsString(sqlrelay:getFieldByName(6, "column_name"), "TESTCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(7, "column_name"), "TESTVARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(8, "column_name"), "TESTDATE"),
    assertEqualsString(sqlrelay:getFieldByName(9, "column_name"), "TESTTIME"),
    assertEqualsString(sqlrelay:getFieldByName(10, "column_name"),
                       "TESTTIMESTAMP"),
    assertEqualsString(sqlrelay:getFieldByName(11, "column_name"), "TESTCLOB"),
    assertEqualsString(sqlrelay:getFieldByName(12, "column_name"), "TESTBLOB"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "SMALLINT"),
    assertEqualsString(sqlrelay:getFieldByName(1, "data_type"), "INTEGER"),
    assertEqualsString(sqlrelay:getFieldByName(2, "data_type"), "BIGINT"),
    assertEqualsString(sqlrelay:getFieldByName(3, "data_type"), "DECIMAL"),
    assertEqualsString(sqlrelay:getFieldByName(4, "data_type"), "REAL"),
    assertEqualsString(sqlrelay:getFieldByName(5, "data_type"), "DOUBLE"),
    assertEqualsString(sqlrelay:getFieldByName(6, "data_type"), "CHARACTER"),
    assertEqualsString(sqlrelay:getFieldByName(7, "data_type"), "VARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(8, "data_type"), "DATE"),
    assertEqualsString(sqlrelay:getFieldByName(9, "data_type"), "TIME"),
    assertEqualsString(sqlrelay:getFieldByName(10, "data_type"), "TIMESTAMP"),
    assertEqualsString(sqlrelay:getFieldByName(11, "data_type"), "CLOB"),
    assertEqualsString(sqlrelay:getFieldByName(12, "data_type"), "BLOB"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% COLUMN LIST - auto_increment, primary key
    io:format("COLUMN LIST - auto_increment, primary key: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 int generated always as identity primary key, "
        "	col2 int)")),
    assertTrue(sqlrelay:commit()),
    assertTrue(sqlrelay:getColumnList("testtable", "")),
    {ok, Extra0} = sqlrelay:getFieldByName(0, "extra"),
    assertEqualsString(Extra0, "auto_increment"),
    {ok, Ck0} = sqlrelay:getFieldByName(0, "column_key"),
    assertEqualsString(Ck0, "PRI"),
    {ok, Extra1} = sqlrelay:getFieldByName(1, "extra"),
    assertEqualsString(Extra1, ""),
    {ok, Ck1} = sqlrelay:getFieldByName(1, "column_key"),
    assertEqualsString(Ck1, ""),
    io:format("~n"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 int not null primary key, "
        "	col2 int)")),
    assertTrue(sqlrelay:commit()),
    assertTrue(sqlrelay:getColumnList("testtable", "")),
    {ok, Extra0b} = sqlrelay:getFieldByName(0, "extra"),
    assertEqualsString(Extra0b, ""),
    {ok, Ck0b} = sqlrelay:getFieldByName(0, "column_key"),
    assertEqualsString(Ck0b, "PRI"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% PRIMARY KEYS LIST
    io:format("PRIMARY KEYS LIST: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 int not null primary key, "
        "	col2 int)")),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "table"), "TESTTABLE"),
    assertEqualsString(sqlrelay:getFieldByName(0, "seq_in_index"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "column_name"), "COL1"),
    {ok, PkName} = sqlrelay:getFieldByName(0, "key_name"),
    assertStartsWith(PkName, "SQL"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    assertTrue(sqlrelay:commit()),
    io:format("~n"),

    %% KEY AND INDEX LIST
    io:format("KEY AND INDEX LIST: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 int not null primary key, "
        "	col2 int)")),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "table"), "TESTTABLE"),
    assertEqualsString(sqlrelay:getFieldByName(0, "non_unique"), "0"),
    assertEqualsString(sqlrelay:getFieldByName(0, "seq_in_index"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "column_name"), "COL1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "collation"), "A"),
    assertEqualsString(sqlrelay:getFieldByName(0, "index_type"), "3"),
    {ok, KeyName2} = sqlrelay:getFieldByName(0, "key_name"),
    assertStartsWith(KeyName2, "SQL"),
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
        "	in in1 integer, "
        "	in in2 char(20), "
        "	in in3 varchar(20), "
        "	in in4 date) "
        "language sql begin end")),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc2("
        "	in in1 integer, "
        "	in in2 char(20), "
        "	in in3 varchar(20), "
        "	in in4 date) "
        "language sql begin end")),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc3("
        "	in in1 integer, "
        "	in in2 char(20), "
        "	in in3 varchar(20), "
        "	in in4 date) "
        "language sql begin end")),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc4("
        "	in in1 integer, "
        "	in in2 char(20), "
        "	in in3 varchar(20), "
        "	in in4 date) "
        "language sql begin end")),
    assertTrue(sqlrelay:commit()),
    assertTrue(sqlrelay:getProcedureList("")),
    assertInResultSet("routine_name", "TESTPROC1"),
    assertInResultSet("routine_name", "TESTPROC2"),
    assertInResultSet("routine_name", "TESTPROC3"),
    assertInResultSet("routine_name", "TESTPROC4"),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "parameter_name"), "IN1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "INTEGER"),
    assertEqualsString(sqlrelay:getFieldByName(0, "ordinal_position"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(1, "parameter_name"), "IN2"),
    assertEqualsString(sqlrelay:getFieldByName(1, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(1, "data_type"), "CHARACTER"),
    assertEqualsString(sqlrelay:getFieldByName(1, "ordinal_position"), "2"),
    assertEqualsString(sqlrelay:getFieldByName(2, "parameter_name"), "IN3"),
    assertEqualsString(sqlrelay:getFieldByName(2, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(2, "data_type"), "VARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(2, "ordinal_position"), "3"),
    assertEqualsString(sqlrelay:getFieldByName(3, "parameter_name"), "IN4"),
    assertEqualsString(sqlrelay:getFieldByName(3, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(3, "data_type"), "DATE"),
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

    %% Silence unused-variable warning for Hostname.
    _ = Hostname,

    reportTestStatus(),

    sqlrelay:cursorFree(),
    sqlrelay:connectionFree(),
    sqlrelay:stop(),
    init:stop(getStatus()).
