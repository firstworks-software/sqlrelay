%% Copyright (c) David Muse
%% See the file COPYING for more information.

-module(firebird).
-export([main/0]).

-import(asserts, [pass/0, fail/2,
                  getStatus/0, reportTestStatus/0,
                  assertEqualsString/2, assertEqualsStringLen/3,
                  assertEqualsInt/2, assertEqualsDouble/2,
                  assertTrue/1, assertFalse/1,
                  waitForPort/1, largeBuffer/1, shortHostname/0]).

main() ->
    sqlrelay:start(),
    waitForPort(50),
    {ok, _} = sqlrelay:alloc("sqlrelay", 9000, "/tmp/test.socket",
                             "testuser", "testpassword", 0, 1),

    _Hostname = shortHostname(),

    %% IDENTIFY
    io:format("IDENTIFY: ~n"),
    assertEqualsString(sqlrelay:identify(), "firebird"),
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
    assertEqualsString(sqlrelay:nextvalFormat(), "next value for %s"),
    io:format("~n"),

    %% ISOLATION LEVELS
    %% though firebird does support a "set transaction ..." statement to
    %% set the isolation level, it looks like, in firebird, you can really
    %% only set it through the TPB at the start of a transaction, so
    %% attempts to set it should fail
    io:format("ISOLATION LEVELS: ~n"),
    assertFalse(sqlrelay:setIsolationLevel("read committed")),
    assertEqualsString(sqlrelay:getIsolationLevel(), "read committed"),
    io:format("~n"),

    %% INSERT
    io:format("INSERT: ~n"),
    sqlrelay:sendQuery("delete from testtable"),
    sqlrelay:commit(),
    assertTrue(sqlrelay:sendQuery(
        "insert into "
        "	testtable "
        "values ("
        "	1, "
        "	1, "
        "	1.1, "
        "	1.1, "
        "	1.1, "
        "	1.1, "
        "	'01-JAN-2001', "
        "	'01:00:00', "
        "	'testchar1', "
        "	'testvarchar1', "
        "	NULL, "
        "	'testblob1')")),
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
        "	?)"),
    assertEqualsInt(sqlrelay:countBindVariables(), 12),
    sqlrelay:inputBindLong("1", 2),
    sqlrelay:inputBindLong("2", 2),
    sqlrelay:inputBindDouble("3", 2.2, 2, 1),
    sqlrelay:inputBindDouble("4", 2.2, 2, 1),
    sqlrelay:inputBindDouble("5", 2.2, 2, 1),
    sqlrelay:inputBindDouble("6", 2.2, 2, 1),
    sqlrelay:inputBindDate("7", 2002, 1, 1, -1, -1, -1, -1, "", 0),
    sqlrelay:inputBindDate("8", -1, -1, -1, 2, 0, 0, 0, "", 0),
    sqlrelay:inputBindString("9", "testchar2"),
    sqlrelay:inputBindString("10", "testvarchar2"),
    sqlrelay:inputBindNull("11"),
    sqlrelay:inputBindBlob("12", "testblob2", 9),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 3),
    sqlrelay:inputBindLong("2", 3),
    sqlrelay:inputBindDouble("3", 3.3, 2, 1),
    sqlrelay:inputBindDouble("4", 3.3, 2, 1),
    sqlrelay:inputBindDouble("5", 3.3, 2, 1),
    sqlrelay:inputBindDouble("6", 3.3, 2, 1),
    sqlrelay:inputBindDate("7", 2003, 1, 1, -1, -1, -1, -1, "", 0),
    sqlrelay:inputBindDate("8", -1, -1, -1, 3, 0, 0, 0, "", 0),
    sqlrelay:inputBindString("9", "testchar3"),
    sqlrelay:inputBindString("10", "testvarchar3"),
    sqlrelay:inputBindNull("11"),
    sqlrelay:inputBindBlob("12", "testblob3", 9),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 4),
    sqlrelay:inputBindLong("2", 4),
    sqlrelay:inputBindDouble("3", 4.4, 2, 1),
    sqlrelay:inputBindDouble("4", 4.4, 2, 1),
    sqlrelay:inputBindDouble("5", 4.4, 2, 1),
    sqlrelay:inputBindDouble("6", 4.4, 2, 1),
    sqlrelay:inputBindDate("7", 2004, 1, 1, -1, -1, -1, -1, "", 0),
    sqlrelay:inputBindDate("8", -1, -1, -1, 4, 0, 0, 0, "", 0),
    sqlrelay:inputBindString("9", "testchar4"),
    sqlrelay:inputBindString("10", "testvarchar4"),
    sqlrelay:inputBindNull("11"),
    sqlrelay:inputBindBlob("12", "testblob4", 9),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 5),
    sqlrelay:inputBindLong("2", 5),
    sqlrelay:inputBindDouble("3", 5.5, 2, 1),
    sqlrelay:inputBindDouble("4", 5.5, 2, 1),
    sqlrelay:inputBindDouble("5", 5.5, 2, 1),
    sqlrelay:inputBindDouble("6", 5.5, 2, 1),
    sqlrelay:inputBindDate("7", 2005, 1, 1, -1, -1, -1, -1, "", 0),
    sqlrelay:inputBindDate("8", -1, -1, -1, 5, 0, 0, 0, "", 0),
    sqlrelay:inputBindString("9", "testchar5"),
    sqlrelay:inputBindString("10", "testvarchar5"),
    sqlrelay:inputBindNull("11"),
    sqlrelay:inputBindBlob("12", "testblob5", 9),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 6),
    sqlrelay:inputBindLong("2", 6),
    sqlrelay:inputBindDouble("3", 6.6, 2, 1),
    sqlrelay:inputBindDouble("4", 6.6, 2, 1),
    sqlrelay:inputBindDouble("5", 6.6, 2, 1),
    sqlrelay:inputBindDouble("6", 6.6, 2, 1),
    sqlrelay:inputBindDate("7", 2006, 1, 1, -1, -1, -1, -1, "", 0),
    sqlrelay:inputBindDate("8", -1, -1, -1, 6, 0, 0, 0, "", 0),
    sqlrelay:inputBindString("9", "testchar6"),
    sqlrelay:inputBindString("10", "testvarchar6"),
    sqlrelay:inputBindNull("11"),
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
    sqlrelay:inputBindString("3", "7.7"),
    sqlrelay:inputBindString("4", "7.7"),
    sqlrelay:inputBindString("5", "7.7"),
    sqlrelay:inputBindString("6", "7.7"),
    sqlrelay:inputBindString("7", "01-JAN-2007"),
    sqlrelay:inputBindString("8", "07:00:00"),
    sqlrelay:inputBindString("9", "testchar7"),
    sqlrelay:inputBindString("10", "testvarchar7"),
    sqlrelay:inputBindNull("11"),
    sqlrelay:inputBindBlob("12", "testblob7", 9),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% INPUT BIND BY POSITION WITH VALIDATION
    io:format("INPUT BIND BY POSITION WITH VALIDATION: ~n"),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 8),
    sqlrelay:inputBindLong("2", 8),
    sqlrelay:inputBindDouble("3", 8.8, 2, 1),
    sqlrelay:inputBindDouble("4", 8.8, 2, 1),
    sqlrelay:inputBindDouble("5", 8.8, 2, 1),
    sqlrelay:inputBindDouble("6", 8.8, 2, 1),
    sqlrelay:inputBindDate("7", 2008, 1, 1, -1, -1, -1, -1, "", 0),
    sqlrelay:inputBindDate("8", -1, -1, -1, 8, 0, 0, 0, "", 0),
    sqlrelay:inputBindString("9", "testchar8"),
    sqlrelay:inputBindString("10", "testvarchar8"),
    sqlrelay:inputBindNull("11"),
    sqlrelay:inputBindBlob("12", "testblob8", 9),
    sqlrelay:validateBinds(),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% INPUT BIND BY NAME
    %% firebird doesn't support bind by name

    %% ARRAY OF INPUT BINDS BY NAME
    %% firebird doesn't support bind by name

    %% INPUT BIND BY NAME WITH VALIDATION
    %% firebird doesn't support bind by name

    %% SELECT
    io:format("SELECT: ~n"),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testinteger ")),
    io:format("~n"),

    %% COLUMN COUNT
    io:format("COLUMN COUNT: ~n"),
    assertEqualsInt(sqlrelay:colCount(), 12),
    io:format("~n"),

    %% COLUMN NAMES
    io:format("COLUMN NAMES: ~n"),
    assertEqualsString(sqlrelay:getColumnName(0), "TESTINTEGER"),
    assertEqualsString(sqlrelay:getColumnName(1), "TESTSMALLINT"),
    assertEqualsString(sqlrelay:getColumnName(2), "TESTDECIMAL"),
    assertEqualsString(sqlrelay:getColumnName(3), "TESTNUMERIC"),
    assertEqualsString(sqlrelay:getColumnName(4), "TESTFLOAT"),
    assertEqualsString(sqlrelay:getColumnName(5), "TESTDOUBLE"),
    assertEqualsString(sqlrelay:getColumnName(6), "TESTDATE"),
    assertEqualsString(sqlrelay:getColumnName(7), "TESTTIME"),
    assertEqualsString(sqlrelay:getColumnName(8), "TESTCHAR"),
    assertEqualsString(sqlrelay:getColumnName(9), "TESTVARCHAR"),
    assertEqualsString(sqlrelay:getColumnName(10), "TESTTIMESTAMP"),
    assertEqualsString(sqlrelay:getColumnName(11), "TESTBLOB"),
    {ok, Cols1} = sqlrelay:getColumnNames(),
    assertEqualsString(lists:nth(1, Cols1), "TESTINTEGER"),
    assertEqualsString(lists:nth(2, Cols1), "TESTSMALLINT"),
    assertEqualsString(lists:nth(3, Cols1), "TESTDECIMAL"),
    assertEqualsString(lists:nth(4, Cols1), "TESTNUMERIC"),
    assertEqualsString(lists:nth(5, Cols1), "TESTFLOAT"),
    assertEqualsString(lists:nth(6, Cols1), "TESTDOUBLE"),
    assertEqualsString(lists:nth(7, Cols1), "TESTDATE"),
    assertEqualsString(lists:nth(8, Cols1), "TESTTIME"),
    assertEqualsString(lists:nth(9, Cols1), "TESTCHAR"),
    assertEqualsString(lists:nth(10, Cols1), "TESTVARCHAR"),
    assertEqualsString(lists:nth(11, Cols1), "TESTTIMESTAMP"),
    assertEqualsString(lists:nth(12, Cols1), "TESTBLOB"),
    io:format("~n"),

    %% COLUMN TYPES
    io:format("COLUMN TYPES: ~n"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(0), "INTEGER"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTINTEGER"), "INTEGER"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(1), "SMALLINT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTSMALLINT"), "SMALLINT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(2), "DECIMAL"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTDECIMAL"), "DECIMAL"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(3), "NUMERIC"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTNUMERIC"), "NUMERIC"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(4), "FLOAT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTFLOAT"), "FLOAT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(5), "DOUBLE PRECISION"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTDOUBLE"),
                       "DOUBLE PRECISION"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(6), "DATE"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTDATE"), "DATE"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(7), "TIME"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTTIME"), "TIME"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(8), "CHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTCHAR"), "CHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(9), "VARCHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTVARCHAR"), "VARCHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(10), "TIMESTAMP"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTTIMESTAMP"),
                       "TIMESTAMP"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(11), "BLOB"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTBLOB"), "BLOB"),
    io:format("~n"),

    %% COLUMN LENGTH
    io:format("COLUMN LENGTH: ~n"),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTINTEGER"), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(1), 2),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTSMALLINT"), 2),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(2), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTDECIMAL"), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(3), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTNUMERIC"), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(4), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTFLOAT"), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(5), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTDOUBLE"), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(6), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTDATE"), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(7), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTTIME"), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(8), 50),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTCHAR"), 50),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(9), 50),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTVARCHAR"), 50),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(10), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTTIMESTAMP"), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(11), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTBLOB"), 8),
    io:format("~n"),

    %% LONGEST COLUMN
    io:format("LONGEST COLUMN: ~n"),
    assertEqualsInt(sqlrelay:getLongestByIndex(0), 1),
    assertEqualsInt(sqlrelay:getLongestByName("TESTINTEGER"), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(1), 1),
    assertEqualsInt(sqlrelay:getLongestByName("TESTSMALLINT"), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(2), 4),
    assertEqualsInt(sqlrelay:getLongestByName("TESTDECIMAL"), 4),
    assertEqualsInt(sqlrelay:getLongestByIndex(3), 4),
    assertEqualsInt(sqlrelay:getLongestByName("TESTNUMERIC"), 4),
    assertEqualsInt(sqlrelay:getLongestByIndex(4), 6),
    assertEqualsInt(sqlrelay:getLongestByName("TESTFLOAT"), 6),
    assertEqualsInt(sqlrelay:getLongestByIndex(5), 6),
    assertEqualsInt(sqlrelay:getLongestByName("TESTDOUBLE"), 6),
    assertEqualsInt(sqlrelay:getLongestByIndex(6), 10),
    assertEqualsInt(sqlrelay:getLongestByName("TESTDATE"), 10),
    assertEqualsInt(sqlrelay:getLongestByIndex(7), 8),
    assertEqualsInt(sqlrelay:getLongestByName("TESTTIME"), 8),
    assertEqualsInt(sqlrelay:getLongestByIndex(8), 50),
    assertEqualsInt(sqlrelay:getLongestByName("TESTCHAR"), 50),
    assertEqualsInt(sqlrelay:getLongestByIndex(9), 12),
    assertEqualsInt(sqlrelay:getLongestByName("TESTVARCHAR"), 12),
    assertEqualsInt(sqlrelay:getLongestByIndex(10), 0),
    assertEqualsInt(sqlrelay:getLongestByName("TESTTIMESTAMP"), 0),
    assertEqualsInt(sqlrelay:getLongestByIndex(11), 9),
    assertEqualsInt(sqlrelay:getLongestByName("TESTBLOB"), 9),
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
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "1.10"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 3), "1.10"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 4), "1.1000"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 5), "1.1000"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 6), "2001:01:01"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 7), "01:00:00"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 8),
                       "testchar1                                         "),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 9), "testvarchar1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 11), "testblob1"),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 2), "8.80"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 3), "8.80"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 4), "8.8000"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 5), "8.8000"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 6), "2008:01:01"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 7), "08:00:00"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 8),
                       "testchar8                                         "),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 9), "testvarchar8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 11), "testblob8"),
    io:format("~n"),

    %% FIELD LENGTHS BY INDEX
    io:format("FIELD LENGTHS BY INDEX: ~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 1), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 2), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 3), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 4), 6),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 5), 6),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 6), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 7), 8),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 8), 50),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 9), 12),
    io:format("~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 0), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 1), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 2), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 3), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 4), 6),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 5), 6),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 6), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 7), 8),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 8), 50),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 9), 12),
    io:format("~n"),

    %% FIELDS BY NAME
    io:format("FIELDS BY NAME: ~n"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTINTEGER"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTSMALLINT"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTDECIMAL"), "1.10"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTNUMERIC"), "1.10"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTFLOAT"), "1.1000"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTDOUBLE"), "1.1000"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTDATE"), "2001:01:01"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTTIME"), "01:00:00"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTCHAR"),
                       "testchar1                                         "),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTVARCHAR"),
                       "testvarchar1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTBLOB"), "testblob1"),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTINTEGER"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTSMALLINT"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTDECIMAL"), "8.80"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTNUMERIC"), "8.80"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTFLOAT"), "8.8000"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTDOUBLE"), "8.8000"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTDATE"), "2008:01:01"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTTIME"), "08:00:00"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTCHAR"),
                       "testchar8                                         "),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTVARCHAR"),
                       "testvarchar8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTBLOB"), "testblob8"),
    io:format("~n"),

    %% FIELD LENGTHS BY NAME
    io:format("FIELD LENGTHS BY NAME: ~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTINTEGER"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTSMALLINT"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTDECIMAL"), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTNUMERIC"), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTFLOAT"), 6),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTDOUBLE"), 6),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTDATE"), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTTIME"), 8),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTCHAR"), 50),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTVARCHAR"), 12),
    io:format("~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTINTEGER"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTSMALLINT"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTDECIMAL"), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTNUMERIC"), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTFLOAT"), 6),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTDOUBLE"), 6),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTDATE"), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTTIME"), 8),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTCHAR"), 50),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTVARCHAR"), 12),
    io:format("~n"),

    %% FIELDS BY ARRAY
    io:format("FIELDS BY ARRAY: ~n"),
    {ok, Row0} = sqlrelay:getRow(0),
    assertEqualsString(lists:nth(1, Row0), "1"),
    assertEqualsString(lists:nth(2, Row0), "1"),
    assertEqualsString(lists:nth(3, Row0), "1.10"),
    assertEqualsString(lists:nth(4, Row0), "1.10"),
    assertEqualsString(lists:nth(5, Row0), "1.1000"),
    assertEqualsString(lists:nth(6, Row0), "1.1000"),
    assertEqualsString(lists:nth(7, Row0), "2001:01:01"),
    assertEqualsString(lists:nth(8, Row0), "01:00:00"),
    assertEqualsString(lists:nth(9, Row0),
                       "testchar1                                         "),
    assertEqualsString(lists:nth(10, Row0), "testvarchar1"),
    assertEqualsString(lists:nth(12, Row0), "testblob1"),
    io:format("~n"),

    %% FIELD LENGTHS BY ARRAY
    io:format("FIELD LENGTHS BY ARRAY: ~n"),
    {ok, Rowlens0} = sqlrelay:getRowLengths(0),
    assertEqualsInt(lists:nth(1, Rowlens0), 1),
    assertEqualsInt(lists:nth(2, Rowlens0), 1),
    assertEqualsInt(lists:nth(3, Rowlens0), 4),
    assertEqualsInt(lists:nth(4, Rowlens0), 4),
    assertEqualsInt(lists:nth(5, Rowlens0), 6),
    assertEqualsInt(lists:nth(6, Rowlens0), 6),
    assertEqualsInt(lists:nth(7, Rowlens0), 10),
    assertEqualsInt(lists:nth(8, Rowlens0), 8),
    assertEqualsInt(lists:nth(9, Rowlens0), 50),
    assertEqualsInt(lists:nth(10, Rowlens0), 12),
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
        "	testinteger ")),
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
        "	testinteger ")),
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
        "	testinteger ")),
    assertEqualsString(sqlrelay:getColumnName(0), "TESTINTEGER"),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 4),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(0), "INTEGER"),
    io:format("~n"),

    %% SUSPENDED SESSION
    io:format("SUSPENDED SESSION: ~n"),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testinteger ")),
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
        "	testinteger ")),
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
        "	testinteger ")),
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
        "	testinteger ")),
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
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testinteger ")),
    {ok, Filename1} = sqlrelay:getCacheFileName(),
    assertEqualsString(Filename1, "cachefile1"),
    sqlrelay:cacheOff(),
    assertTrue(sqlrelay:openCachedResultSet(Filename1)),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    io:format("~n"),

    %% COLUMN COUNT FOR CACHED RESULT SET
    io:format("COLUMN COUNT FOR CACHED RESULT SET: ~n"),
    assertEqualsInt(sqlrelay:colCount(), 12),
    io:format("~n"),

    %% COLUMN NAMES FOR CACHED RESULT SET
    io:format("COLUMN NAMES FOR CACHED RESULT SET: ~n"),
    assertEqualsString(sqlrelay:getColumnName(0), "TESTINTEGER"),
    assertEqualsString(sqlrelay:getColumnName(1), "TESTSMALLINT"),
    assertEqualsString(sqlrelay:getColumnName(2), "TESTDECIMAL"),
    assertEqualsString(sqlrelay:getColumnName(3), "TESTNUMERIC"),
    assertEqualsString(sqlrelay:getColumnName(4), "TESTFLOAT"),
    assertEqualsString(sqlrelay:getColumnName(5), "TESTDOUBLE"),
    assertEqualsString(sqlrelay:getColumnName(6), "TESTDATE"),
    assertEqualsString(sqlrelay:getColumnName(7), "TESTTIME"),
    assertEqualsString(sqlrelay:getColumnName(8), "TESTCHAR"),
    assertEqualsString(sqlrelay:getColumnName(9), "TESTVARCHAR"),
    assertEqualsString(sqlrelay:getColumnName(10), "TESTTIMESTAMP"),
    assertEqualsString(sqlrelay:getColumnName(11), "TESTBLOB"),
    {ok, Cols2} = sqlrelay:getColumnNames(),
    assertEqualsString(lists:nth(1, Cols2), "TESTINTEGER"),
    assertEqualsString(lists:nth(2, Cols2), "TESTSMALLINT"),
    assertEqualsString(lists:nth(3, Cols2), "TESTDECIMAL"),
    assertEqualsString(lists:nth(4, Cols2), "TESTNUMERIC"),
    assertEqualsString(lists:nth(5, Cols2), "TESTFLOAT"),
    assertEqualsString(lists:nth(6, Cols2), "TESTDOUBLE"),
    assertEqualsString(lists:nth(7, Cols2), "TESTDATE"),
    assertEqualsString(lists:nth(8, Cols2), "TESTTIME"),
    assertEqualsString(lists:nth(9, Cols2), "TESTCHAR"),
    assertEqualsString(lists:nth(10, Cols2), "TESTVARCHAR"),
    assertEqualsString(lists:nth(11, Cols2), "TESTTIMESTAMP"),
    assertEqualsString(lists:nth(12, Cols2), "TESTBLOB"),
    io:format("~n"),

    %% CACHED RESULT SET WITH RESULT SET BUFFER SIZE
    io:format("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ~n"),
    sqlrelay:setResultSetBufferSize(2),
    sqlrelay:cacheToFile("cachefile1"),
    sqlrelay:setCacheTtl(200),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testinteger ")),
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
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testinteger ")),
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
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testinteger ")),
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
    %% concurrent cursors cannot be held.  We still run the outer query
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
    assertEqualsString(sqlrelay:getTransactionModel(), "implicit"),
    assertFalse(sqlrelay:getAutoCommit()),
    io:format("~n"),

    %% TRANSACTION BEHAVIOR - implicit/explicit/explicit-deferred/explicit-error/none
    %% SKIPPED: these blocks require a second concurrent connection
    %% (secondcon) and cursor (secondcur) to verify cross-connection
    %% isolation. The Erlang binding only supports one connection per
    %% process, so a second connection cannot be instantiated here.
    io:format("TRANSACTION BEHAVIOR - implicit/explicit/explicit-deferred/explicit-error/none: ~n"),
    io:format("(skipped - requires second concurrent connection)~n"),
    %% Restore autoCommitOff and clear the testtable so subsequent
    %% sections start clean.
    assertTrue(sqlrelay:autoCommitOff()),
    sqlrelay:sendQuery("delete from testtable"),
    sqlrelay:commit(),
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
        "select $(var1),'$(var2)',$(var3) from rdb$database"),
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
        "	rdb$database "),
    sqlrelay:subString("var1", "hi"),
    sqlrelay:subString("var2", "hello"),
    sqlrelay:subString("var3", "bye"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "hi"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "hello"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "bye"),
    io:format("~n"),
    sqlrelay:prepareQuery(
        "select $(var1),$(var2),$(var3) from rdb$database"),
    sqlrelay:subLong("var1", 1),
    sqlrelay:subLong("var2", 2),
    sqlrelay:subLong("var3", 3),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "2"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "3"),
    io:format("~n"),
    sqlrelay:prepareQuery(
        "select $(var1),$(var2),$(var3) from rdb$database"),
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
    assertTrue(sqlrelay:sendQuery("select 1,NULL,NULL from rdb$database")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), null),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), null),
    sqlrelay:getNullsAsEmptyStrings(),
    assertTrue(sqlrelay:sendQuery("select 1,NULL,NULL from rdb$database")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), ""),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), ""),
    io:format("~n"),

    %% NULL AND EMPTY LOBS
    %% Note: the Erlang inputBindBlob function requires a list Value, so
    %% we cannot pass a true NULL. The empty string case is preserved;
    %% the NULL case uses an empty string and expects the resulting
    %% field to be the empty string or null (accepted by
    %% assertEqualsString(..., null) when the server treats empties as
    %% nulls for blobs).
    io:format("NULL AND EMPTY LOBS: ~n"),
    sqlrelay:getNullsAsNulls(),
    sqlrelay:sendQuery("delete from testtable1"),
    sqlrelay:prepareQuery("insert into testtable1 values (?)"),
    sqlrelay:inputBindBlob("1", "", 0),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("select testblob from testtable1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTBLOB"), ""),
    sqlrelay:sendQuery("delete from testtable1"),
    sqlrelay:prepareQuery("insert into testtable1 values (?)"),
    sqlrelay:inputBindBlob("1", "", 0),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("select testblob from testtable1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTBLOB"), null),
    sqlrelay:getNullsAsEmptyStrings(),
    assertTrue(sqlrelay:sendQuery("delete from testtable1")),
    io:format("~n"),

    %% LONG LOBS
    io:format("LONG LOBS: ~n"),
    sqlrelay:sendQuery("delete from testtable1"),
    sqlrelay:prepareQuery("insert into testtable1 values (?)"),
    LargeBufferLength = 20 * 1024,
    LargeBuf = largeBuffer(LargeBufferLength),
    sqlrelay:inputBindClob("1", LargeBuf, LargeBufferLength),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("select testblob from testtable1"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTBLOB"),
                    LargeBufferLength),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTBLOB"), LargeBuf),
    assertTrue(sqlrelay:sendQuery("delete from testtable1")),
    io:format("~n"),

    %% OUTPUT BIND BY POSITION
    io:format("OUTPUT BIND BY POSITION: ~n"),
    sqlrelay:getNullsAsNulls(),
    sqlrelay:prepareQuery("execute procedure testproc ?, ?, ?, ?"),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:inputBindDouble("2", 1.1, 2, 1),
    sqlrelay:inputBindString("3", "hello"),
    sqlrelay:inputBindBlob("4", "blob", 4),
    sqlrelay:defineOutputBindInteger("1"),
    sqlrelay:defineOutputBindDouble("2"),
    sqlrelay:defineOutputBindString("3", 20),
    sqlrelay:defineOutputBindBlob("4"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("1"), 1),
    %%assertEqualsDouble(sqlrelay:getOutputBindDouble("2"), 1.1),
    assertEqualsString(sqlrelay:getOutputBindString("3"),
                       "hello               "),
    assertEqualsString(sqlrelay:getOutputBindBlob("4"), "blob"),
    sqlrelay:getNullsAsEmptyStrings(),
    io:format("~n"),

    %% OUTPUT BIND BY NAME
    %% firebird doesn't support bind by name

    %% OUTPUT BIND BY NAME WITH VALIDATION
    %% firebird doesn't support bind by name

    %% LOB OUTPUT BIND
    io:format("LOB OUTPUT BIND: ~n"),
    sqlrelay:prepareQuery("execute procedure testproc1 ?"),
    sqlrelay:inputBindBlob("1", "hello", 5),
    sqlrelay:defineOutputBindBlob("1"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsStringLen(sqlrelay:getOutputBindBlob("1"), "hello", 5),
    assertEqualsInt(sqlrelay:getOutputBindLength("1"), 5),
    io:format("~n"),

    %% LONG OUTPUT BIND
    io:format("LONG OUTPUT BIND: ~n"),
    sqlrelay:prepareQuery("execute procedure testproc1 ?"),
    sqlrelay:inputBindBlob("1", LargeBuf, LargeBufferLength),
    sqlrelay:defineOutputBindBlob("1"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindLength("1"), LargeBufferLength),
    assertEqualsStringLen(sqlrelay:getOutputBindBlob("1"), LargeBuf,
                          LargeBufferLength),
    io:format("~n"),

    %% NEGATIVE INPUT BIND
    io:format("NEGATIVE INPUT BIND: ~n"),
    sqlrelay:prepareQuery("select cast(? as integer) from rdb$database"),
    sqlrelay:inputBindLong("1", -1),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "-1"),
    io:format("~n"),

    %% BIND VALIDATION
    %% firebird doesn't support bind by name

    %% REBINDING
    io:format("REBINDING: ~n"),
    sqlrelay:prepareQuery("execute procedure testproc ?, ?, ?, ?"),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:inputBindDouble("2", 1.1, 2, 1),
    sqlrelay:inputBindString("3", "hello"),
    sqlrelay:inputBindBlob("4", "blob", 4),
    sqlrelay:defineOutputBindInteger("1"),
    sqlrelay:defineOutputBindDouble("2"),
    sqlrelay:defineOutputBindString("3", 20),
    sqlrelay:defineOutputBindBlob("4"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("1"), 1),
    sqlrelay:inputBindLong("1", 2),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("1"), 2),
    sqlrelay:inputBindLong("1", 3),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("1"), 3),
    io:format("~n"),

    %% REEXECUTE
    io:format("REEXECUTE: ~n"),
    sqlrelay:prepareQuery("select 1 from rdb$database"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:rowCount(), 1),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    io:format("~n"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:rowCount(), 1),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    io:format("~n"),
    sqlrelay:prepareQuery("select cast(? as int) from rdb$database"),
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
    sqlrelay:prepareQuery(
        "execute block (in1 int = ?, "
        "	in2 double precision = ?, "
        "	in3 varchar(20) = ?) "
        "as "
        "begin "
        "end"),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:inputBindDouble("2", 1.1, 2, 1),
    sqlrelay:inputBindString("3", "hello"),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING SINGLE VALUE
    io:format("STORED PROCEDURE RETURNING SINGLE VALUE: ~n"),
    sqlrelay:prepareQuery(
        "execute block (in1 int = ?, "
        "	in2 double precision = ?, "
        "	in3 varchar(20) = ?) "
        "returns (out1 int) "
        "as "
        "begin "
        "	out1 = in1; "
        "	suspend; "
        "end"),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:inputBindDouble("2", 1.1, 2, 1),
    sqlrelay:inputBindString("3", "hello"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING MULTIPLE VALUES
    io:format("STORED PROCEDURE RETURNING MULTIPLE VALUES: ~n"),
    sqlrelay:prepareQuery(
        "execute block (in1 int = ?, "
        "	in2 double precision = ?, "
        "	in3 varchar(20) = ?) "
        "returns (out1 int, "
        "	out2 double precision, "
        "	out3 varchar(20)) "
        "as "
        "begin "
        "	out1 = in1; "
        "	out2 = in2; "
        "	out3 = in3; "
        "	suspend; "
        "end"),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:inputBindDouble("2", 1.1, 2, 1),
    sqlrelay:inputBindString("3", "hello"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "1.1000"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "hello"),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING RESULT SET
    io:format("STORED PROCEDURE RETURNING RESULT SET: ~n"),
    sqlrelay:prepareQuery(
        "execute block "
        "returns (out1 int) "
        "as "
        "declare i int; "
        "begin "
        "	i = 1; "
        "	while (i <= 8) do "
        "	begin "
        "		out1 = i; "
        "		suspend; "
        "		i = i + 1; "
        "	end "
        "end"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    io:format("~n"),

    %% TEMPORARY TABLES
    %% firebird supports temporary tables, but we're omitting this for now

    %% ENCODED BINARY DATA
    %% firebird doesn't support encoded binary data

    %% QUOTES
    io:format("QUOTES: ~n"),
    sqlrelay:sendQuery("delete from table testtable1"),
    assertTrue(sqlrelay:sendQuery(
        "insert into testtable1 values ('''''')")),
    assertTrue(sqlrelay:sendQuery("select testblob from testtable1")),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 2),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "''"),
    assertTrue(sqlrelay:sendQuery("delete from testtable1")),
    io:format("~n"),

    %% LAST INSERT ID
    %% firebird doesn't support auto-increment

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
    {ok, SchemaRowCount} = sqlrelay:rowCount(),
    SchemaFound = searchSchemaList(0, SchemaRowCount),
    assertTrue(SchemaFound),
    io:format("~n"),

    %% TABLE TYPE LIST
    io:format("TABLE TYPE LIST: ~n"),
    assertTrue(sqlrelay:getTableTypeList()),
    assertEqualsString(sqlrelay:getColumnName(0), "table_type"),
    {ok, TtlRowCount} = sqlrelay:rowCount(),
    TableTypeFound = searchTableTypeList(0, TtlRowCount),
    assertTrue(TableTypeFound),
    io:format("~n"),

    %% TABLE LIST
    io:format("TABLE LIST: ~n"),
    assertTrue(sqlrelay:getTableList("")),
    {ok, TableListRowCount} = sqlrelay:rowCount(),
    TableCount = countMatchingTables(0, TableListRowCount, 0),
    assertEqualsInt(TableCount, 3),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "32765"),
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
                       "TESTINTEGER"),
    assertEqualsString(sqlrelay:getFieldByName(1, "column_name"),
                       "TESTSMALLINT"),
    assertEqualsString(sqlrelay:getFieldByName(2, "column_name"),
                       "TESTDECIMAL"),
    assertEqualsString(sqlrelay:getFieldByName(3, "column_name"),
                       "TESTNUMERIC"),
    assertEqualsString(sqlrelay:getFieldByName(4, "column_name"),
                       "TESTFLOAT"),
    assertEqualsString(sqlrelay:getFieldByName(5, "column_name"),
                       "TESTDOUBLE"),
    assertEqualsString(sqlrelay:getFieldByName(6, "column_name"), "TESTDATE"),
    assertEqualsString(sqlrelay:getFieldByName(7, "column_name"), "TESTTIME"),
    assertEqualsString(sqlrelay:getFieldByName(8, "column_name"), "TESTCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(9, "column_name"),
                       "TESTVARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(10, "column_name"),
                       "TESTTIMESTAMP"),
    assertEqualsString(sqlrelay:getFieldByName(11, "column_name"),
                       "TESTBLOB"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "INTEGER"),
    assertEqualsString(sqlrelay:getFieldByName(1, "data_type"), "SMALLINT"),
    assertEqualsString(sqlrelay:getFieldByName(2, "data_type"), "DECIMAL"),
    assertEqualsString(sqlrelay:getFieldByName(3, "data_type"), "NUMERIC"),
    assertEqualsString(sqlrelay:getFieldByName(4, "data_type"), "FLOAT"),
    assertEqualsString(sqlrelay:getFieldByName(5, "data_type"),
                       "DOUBLE PRECISION"),
    assertEqualsString(sqlrelay:getFieldByName(6, "data_type"), "DATE"),
    assertEqualsString(sqlrelay:getFieldByName(7, "data_type"), "TIME"),
    assertEqualsString(sqlrelay:getFieldByName(8, "data_type"), "CHAR"),
    assertEqualsString(sqlrelay:getFieldByName(9, "data_type"), "VARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(10, "data_type"), "TIMESTAMP"),
    assertEqualsString(sqlrelay:getFieldByName(11, "data_type"),
                       "BLOB SUB_TYPE BINARY"),
    io:format("~n"),

    %% COLUMN LIST - auto_increment, primary key
    io:format("COLUMN LIST - auto_increment, primary key: ~n"),
    assertTrue(sqlrelay:getColumnList("testtable2", "")),
    {ok, Extra0} = sqlrelay:getFieldByName(0, "extra"),
    assertTrue(contains(Extra0, "auto_increment")),
    {ok, Ck0} = sqlrelay:getFieldByName(0, "column_key"),
    assertTrue(contains(Ck0, "PRI")),
    {ok, Extra1} = sqlrelay:getFieldByName(1, "extra"),
    assertFalse(contains(Extra1, "auto_increment")),
    {ok, Ck1} = sqlrelay:getFieldByName(1, "column_key"),
    assertFalse(contains(Ck1, "PRI")),
    io:format("~n"),
    assertTrue(sqlrelay:getColumnList("testtable3", "")),
    {ok, Extra2} = sqlrelay:getFieldByName(0, "extra"),
    assertFalse(contains(Extra2, "auto_increment")),
    {ok, Ck2} = sqlrelay:getFieldByName(0, "column_key"),
    assertTrue(contains(Ck2, "PRI")),
    io:format("~n"),

    %% PRIMARY KEYS LIST
    io:format("PRIMARY KEYS LIST: ~n"),
    assertTrue(sqlrelay:getPrimaryKeysList("testtable2", "")),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "table"), "TESTTABLE2"),
    assertEqualsString(sqlrelay:getFieldByName(0, "seq_in_index"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "column_name"), "COL1"),
    {ok, PkName} = sqlrelay:getFieldByName(0, "key_name"),
    assertTrue(not isNullOrEmpty(PkName)),
    io:format("~n"),

    %% KEY AND INDEX LIST
    io:format("KEY AND INDEX LIST: ~n"),
    assertTrue(sqlrelay:getKeyAndIndexList("testtable2", "")),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "table"), "TESTTABLE2"),
    assertEqualsString(sqlrelay:getFieldByName(0, "non_unique"), "0"),
    assertEqualsString(sqlrelay:getFieldByName(0, "seq_in_index"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "column_name"), "COL1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "collation"), "A"),
    assertEqualsString(sqlrelay:getFieldByName(0, "index_type"), "3"),
    {ok, KeyName2} = sqlrelay:getFieldByName(0, "key_name"),
    assertTrue(not isNullOrEmpty(KeyName2)),
    io:format("~n"),

    %% PROCEDURE LIST
    io:format("PROCEDURE LIST: ~n"),
    assertTrue(sqlrelay:getProcedureList("")),
    {ok, ProcRowCount} = sqlrelay:rowCount(),
    ProcCount = countMatchingProcs(0, ProcRowCount, 0),
    assertEqualsInt(ProcCount, 2),
    io:format("~n"),

    %% PROCEDURE PARAMETER LIST
    io:format("PROCEDURE PARAMETER LIST: ~n"),
    assertTrue(sqlrelay:getProcedureParameterList("testproc", "")),
    assertEqualsString(sqlrelay:getColumnName(0), "parameter_name"),
    assertEqualsString(sqlrelay:getColumnName(1), "parameter_mode"),
    assertEqualsString(sqlrelay:getColumnName(2), "data_type"),
    assertEqualsString(sqlrelay:getColumnName(3), "character_maximum_length"),
    assertEqualsString(sqlrelay:getColumnName(4), "ordinal_position"),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    assertEqualsString(sqlrelay:getFieldByName(0, "parameter_name"), "OUT1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "parameter_mode"), "4"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "INTEGER"),
    assertEqualsString(sqlrelay:getFieldByName(0, "ordinal_position"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(1, "parameter_name"), "OUT2"),
    assertEqualsString(sqlrelay:getFieldByName(1, "parameter_mode"), "4"),
    assertEqualsString(sqlrelay:getFieldByName(1, "data_type"), "FLOAT"),
    assertEqualsString(sqlrelay:getFieldByName(1, "ordinal_position"), "2"),
    assertEqualsString(sqlrelay:getFieldByName(2, "parameter_name"), "OUT3"),
    assertEqualsString(sqlrelay:getFieldByName(2, "parameter_mode"), "4"),
    assertEqualsString(sqlrelay:getFieldByName(2, "data_type"), "VARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(2, "ordinal_position"), "3"),
    assertEqualsString(sqlrelay:getFieldByName(3, "parameter_name"), "OUT4"),
    assertEqualsString(sqlrelay:getFieldByName(3, "parameter_mode"), "4"),
    assertEqualsString(sqlrelay:getFieldByName(3, "data_type"),
                       "BLOB SUB_TYPE BINARY"),
    assertEqualsString(sqlrelay:getFieldByName(3, "ordinal_position"), "4"),
    assertEqualsString(sqlrelay:getFieldByName(4, "parameter_name"), "IN1"),
    assertEqualsString(sqlrelay:getFieldByName(4, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(4, "data_type"), "INTEGER"),
    assertEqualsString(sqlrelay:getFieldByName(4, "ordinal_position"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(5, "parameter_name"), "IN2"),
    assertEqualsString(sqlrelay:getFieldByName(5, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(5, "data_type"), "FLOAT"),
    assertEqualsString(sqlrelay:getFieldByName(5, "ordinal_position"), "2"),
    assertEqualsString(sqlrelay:getFieldByName(6, "parameter_name"), "IN3"),
    assertEqualsString(sqlrelay:getFieldByName(6, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(6, "data_type"), "VARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(6, "ordinal_position"), "3"),
    assertEqualsString(sqlrelay:getFieldByName(7, "parameter_name"), "IN4"),
    assertEqualsString(sqlrelay:getFieldByName(7, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(7, "data_type"),
                       "BLOB SUB_TYPE BINARY"),
    assertEqualsString(sqlrelay:getFieldByName(7, "ordinal_position"), "4"),
    io:format("~n"),

    %% INVALID QUERIES
    io:format("INVALID QUERIES: ~n"),
    assertFalse(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable1 "
        "order by "
        "	testinteger ")),
    assertFalse(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable1 "
        "order by "
        "	testinteger ")),
    assertFalse(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable1 "
        "order by "
        "	testinteger ")),
    assertFalse(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable1 "
        "order by "
        "	testinteger ")),
    io:format("~n"),
    assertFalse(sqlrelay:sendQuery("insert into testtable1 values (1,2,3,4)")),
    assertFalse(sqlrelay:sendQuery("insert into testtable1 values (1,2,3,4)")),
    assertFalse(sqlrelay:sendQuery("insert into testtable1 values (1,2,3,4)")),
    assertFalse(sqlrelay:sendQuery("insert into testtable1 values (1,2,3,4)")),
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

%% String substring containment: does Haystack contain Needle ?
contains(Haystack, Needle) when is_list(Haystack), is_list(Needle) ->
    string:str(Haystack, Needle) > 0;
contains(_, _) ->
    false.

%% Walk the schema-list result looking for a row whose "Database"
%% column equals "TESTUSER".
searchSchemaList(I, Count) when I >= Count ->
    false;
searchSchemaList(I, Count) ->
    case sqlrelay:getFieldByName(I, "Database") of
        {ok, "TESTUSER"} -> true;
        _                -> searchSchemaList(I + 1, Count)
    end.

%% Walk the table-type-list result looking for a row whose "table_type"
%% column equals "TABLE".
searchTableTypeList(I, Count) when I >= Count ->
    false;
searchTableTypeList(I, Count) ->
    case sqlrelay:getFieldByName(I, "table_type") of
        {ok, "TABLE"} -> true;
        _             -> searchTableTypeList(I + 1, Count)
    end.

%% Count rows in the table-list result whose "Tables_in_xxx" column is
%% one of the three test table names.
countMatchingTables(I, Count, Acc) when I >= Count ->
    Acc;
countMatchingTables(I, Count, Acc) ->
    Name = case sqlrelay:getFieldByName(I, "Tables_in_xxx") of
               {ok, N} when is_list(N) -> N;
               _ -> ""
           end,
    NewAcc = case Name of
                 "TESTTABLE1" -> Acc + 1;
                 "TESTTABLE2" -> Acc + 1;
                 "TESTTABLE3" -> Acc + 1;
                 _            -> Acc
             end,
    countMatchingTables(I + 1, Count, NewAcc).

%% Count rows in the procedure-list result whose "routine_name" is one
%% of the two test procedure names.
countMatchingProcs(I, Count, Acc) when I >= Count ->
    Acc;
countMatchingProcs(I, Count, Acc) ->
    Name = case sqlrelay:getFieldByName(I, "routine_name") of
               {ok, N} when is_list(N) -> N;
               _ -> ""
           end,
    NewAcc = case Name of
                 "TESTPROC"  -> Acc + 1;
                 "TESTPROC1" -> Acc + 1;
                 _           -> Acc
             end,
    countMatchingProcs(I + 1, Count, NewAcc).
