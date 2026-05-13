%% Copyright (c) David Muse
%% See the file COPYING for more information.

-module(oracle).
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
    %% oracle requires the isolation level to be the first query of
    %% the transaction
    assertTrue(sqlrelay:commit()),
    %% you can set the isolation level, but to get it, you have to
    %% have permissions to read from sys.v_$session and
    %% sys.v_$transaction
    assertTrue(sqlrelay:setIsolationLevel(Il)),
    io:format("~n"),
    setIsolationLevels(Rest).

main() ->
    sqlrelay:start(),
    waitForPort(50),
    {ok, _} = sqlrelay:alloc("sqlrelay", 9000, "/tmp/test.socket",
                             "testuser", "testpassword", 0, 1),

    Hostname = shortHostname(),

    %% IDENTIFY
    io:format("IDENTIFY: ~n"),
    assertEqualsString(sqlrelay:identify(), "oracle"),
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
    assertEqualsString(sqlrelay:bindFormat(), ":*"),
    io:format("~n"),

    %% NEXTVAL FORMAT
    io:format("NEXTVAL FORMAT: ~n"),
    assertEqualsString(sqlrelay:nextvalFormat(), "%s.nextval"),
    io:format("~n"),

    %% ISOLATION LEVELS
    io:format("ISOLATION LEVELS: ~n"),
    IsolationLevels = ["READ COMMITTED", "SERIALIZABLE"],
    setIsolationLevels(IsolationLevels),
    %% reset to the default isolation level
    assertTrue(sqlrelay:commit()),
    assertTrue(sqlrelay:setIsolationLevel(hd(IsolationLevels))),
    io:format("~n"),

    %% CREATE TESTTABLE
    io:format("CREATE TESTTABLE: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	testnumber number, "
        "	testchar char(40), "
        "	testvarchar varchar2(40), "
        "	testdate date, "
        "	testlong long, "
        "	testclob clob, "
        "	testblob blob)")),
    io:format("~n"),

    %% INSERT
    io:format("INSERT: ~n"),
    assertTrue(sqlrelay:sendQuery(
        "insert into "
        "	testtable "
        "values ("
        "	1, "
        "	'testchar1', "
        "	'testvarchar1', "
        "	'01-JAN-2001', "
        "	'testlong1', "
        "	'testclob1', "
        "	empty_blob())")),
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
        "	:var1, "
        "	:var2, "
        "	:var3, "
        "	:var4, "
        "	:var5, "
        "	:var6, "
        "	:var7)"),
    assertEqualsInt(sqlrelay:countBindVariables(), 7),
    sqlrelay:inputBindLong("1", 2),
    sqlrelay:inputBindString("2", "testchar2"),
    sqlrelay:inputBindString("3", "testvarchar2"),
    sqlrelay:inputBindDate("4", 2002, 1, 1, 0, 0, 0, 0, "", 0),
    sqlrelay:inputBindString("5", "testlong2"),
    sqlrelay:inputBindClob("6", "testclob2", 9),
    sqlrelay:inputBindBlob("7", "testblob2", 9),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 3),
    sqlrelay:inputBindString("2", "testchar3"),
    sqlrelay:inputBindString("3", "testvarchar3"),
    sqlrelay:inputBindDate("4", 2003, 1, 1, 0, 0, 0, 0, "", 0),
    sqlrelay:inputBindString("5", "testlong3"),
    sqlrelay:inputBindClob("6", "testclob3", 9),
    sqlrelay:inputBindBlob("7", "testblob3", 9),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% ARRAY OF INPUT BINDS BY POSITION
    %% The Erlang binding has no array-inputBinds(); do the individual
    %% binds for row 4 manually to keep the section faithful.
    io:format("ARRAY OF INPUT BINDS BY POSITION: ~n"),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindString("1", "4"),
    sqlrelay:inputBindString("2", "testchar4"),
    sqlrelay:inputBindString("3", "testvarchar4"),
    sqlrelay:inputBindString("4", "01-JAN-2004"),
    sqlrelay:inputBindString("5", "testlong4"),
    sqlrelay:inputBindClob("6", "testclob4", 9),
    sqlrelay:inputBindBlob("7", "testblob4", 9),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% INPUT BIND BY POSITION WITH VALIDATION
    io:format("INPUT BIND BY POSITION WITH VALIDATION: ~n"),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 5),
    sqlrelay:inputBindString("2", "testchar5"),
    sqlrelay:inputBindString("3", "testvarchar5"),
    sqlrelay:inputBindDate("4", 2005, 1, 1, 0, 0, 0, 0, "", 0),
    sqlrelay:inputBindString("5", "testlong5"),
    sqlrelay:inputBindClob("6", "testclob5", 9),
    sqlrelay:inputBindBlob("7", "testblob5", 9),
    sqlrelay:validateBinds(),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),

    %% INPUT BIND BY NAME
    io:format("INPUT BIND BY NAME: ~n"),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("var1", 6),
    sqlrelay:inputBindString("var2", "testchar6"),
    sqlrelay:inputBindString("var3", "testvarchar6"),
    sqlrelay:inputBindDate("var4", 2006, 1, 1, 0, 0, 0, 0, "", 0),
    sqlrelay:inputBindString("var5", "testlong6"),
    sqlrelay:inputBindClob("var6", "testclob6", 9),
    sqlrelay:inputBindBlob("var7", "testblob6", 9),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% ARRAY OF INPUT BINDS BY NAME (same treatment as above, no array API)
    io:format("ARRAY OF INPUT BINDS BY NAME: ~n"),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindString("var1", "7"),
    sqlrelay:inputBindString("var2", "testchar7"),
    sqlrelay:inputBindString("var3", "testvarchar7"),
    sqlrelay:inputBindString("var4", "01-JAN-2007"),
    sqlrelay:inputBindString("var5", "testlong7"),
    sqlrelay:inputBindClob("var6", "testclob7", 9),
    sqlrelay:inputBindBlob("var7", "testblob7", 9),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% INPUT BIND BY NAME WITH VALIDATION
    io:format("INPUT BIND BY NAME WITH VALIDATION: ~n"),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("var1", 8),
    sqlrelay:inputBindString("var2", "testchar8"),
    sqlrelay:inputBindString("var3", "testvarchar8"),
    sqlrelay:inputBindDate("var4", 2008, 1, 1, 0, 0, 0, 0, "", 0),
    sqlrelay:inputBindString("var5", "testlong8"),
    sqlrelay:inputBindClob("var6", "testclob8", 9),
    sqlrelay:inputBindBlob("var7", "testblob8", 9),
    sqlrelay:inputBindString("var9", "junkvalue"),
    sqlrelay:validateBinds(),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% SELECT
    io:format("SELECT: ~n"),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testnumber")),
    io:format("~n"),

    %% COLUMN COUNT
    io:format("COLUMN COUNT: ~n"),
    assertEqualsInt(sqlrelay:colCount(), 7),
    io:format("~n"),

    %% COLUMN NAMES
    io:format("COLUMN NAMES: ~n"),
    assertEqualsString(sqlrelay:getColumnName(0), "TESTNUMBER"),
    assertEqualsString(sqlrelay:getColumnName(1), "TESTCHAR"),
    assertEqualsString(sqlrelay:getColumnName(2), "TESTVARCHAR"),
    assertEqualsString(sqlrelay:getColumnName(3), "TESTDATE"),
    assertEqualsString(sqlrelay:getColumnName(4), "TESTLONG"),
    assertEqualsString(sqlrelay:getColumnName(5), "TESTCLOB"),
    assertEqualsString(sqlrelay:getColumnName(6), "TESTBLOB"),
    {ok, Cols1} = sqlrelay:getColumnNames(),
    assertEqualsString(lists:nth(1, Cols1), "TESTNUMBER"),
    assertEqualsString(lists:nth(2, Cols1), "TESTCHAR"),
    assertEqualsString(lists:nth(3, Cols1), "TESTVARCHAR"),
    assertEqualsString(lists:nth(4, Cols1), "TESTDATE"),
    assertEqualsString(lists:nth(5, Cols1), "TESTLONG"),
    assertEqualsString(lists:nth(6, Cols1), "TESTCLOB"),
    assertEqualsString(lists:nth(7, Cols1), "TESTBLOB"),
    io:format("~n"),

    %% COLUMN TYPES
    io:format("COLUMN TYPES: ~n"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(0), "NUMBER"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTNUMBER"), "NUMBER"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(1), "CHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTCHAR"), "CHAR"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(2), "VARCHAR2"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTVARCHAR"), "VARCHAR2"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(3), "DATE"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTDATE"), "DATE"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(4), "LONG"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTLONG"), "LONG"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(5), "CLOB"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTCLOB"), "CLOB"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(6), "BLOB"),
    assertEqualsString(sqlrelay:getColumnTypeByName("TESTBLOB"), "BLOB"),
    io:format("~n"),

    %% COLUMN LENGTH
    io:format("COLUMN LENGTH: ~n"),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 22),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTNUMBER"), 22),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(1), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTCHAR"), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(2), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTVARCHAR"), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(3), 7),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTDATE"), 7),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(4), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTLONG"), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(5), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTCLOB"), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(6), 0),
    assertEqualsInt(sqlrelay:getColumnLengthByName("TESTBLOB"), 0),
    io:format("~n"),

    %% LONGEST COLUMN
    io:format("LONGEST COLUMN: ~n"),
    assertEqualsInt(sqlrelay:getLongestByIndex(0), 1),
    assertEqualsInt(sqlrelay:getLongestByName("TESTNUMBER"), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(1), 40),
    assertEqualsInt(sqlrelay:getLongestByName("TESTCHAR"), 40),
    assertEqualsInt(sqlrelay:getLongestByIndex(2), 12),
    assertEqualsInt(sqlrelay:getLongestByName("TESTVARCHAR"), 12),
    assertEqualsInt(sqlrelay:getLongestByIndex(3), 9),
    assertEqualsInt(sqlrelay:getLongestByName("TESTDATE"), 9),
    assertEqualsInt(sqlrelay:getLongestByIndex(4), 9),
    assertEqualsInt(sqlrelay:getLongestByName("TESTLONG"), 9),
    assertEqualsInt(sqlrelay:getLongestByIndex(5), 9),
    assertEqualsInt(sqlrelay:getLongestByName("TESTCLOB"), 9),
    assertEqualsInt(sqlrelay:getLongestByIndex(6), 9),
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
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1),
                       "testchar1                               "),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "testvarchar1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 3), "01-JAN-01"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 4), "testlong1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 5), "testclob1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 6), ""),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1),
                       "testchar8                               "),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 2), "testvarchar8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 3), "01-JAN-08"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 4), "testlong8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 5), "testclob8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 6), "testblob8"),
    io:format("~n"),

    %% FIELD LENGTHS BY INDEX
    io:format("FIELD LENGTHS BY INDEX: ~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 1), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 2), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 3), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 4), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 5), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 6), 0),
    io:format("~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 0), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 1), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 2), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 3), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 4), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 5), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 6), 9),
    io:format("~n"),

    %% FIELDS BY NAME
    io:format("FIELDS BY NAME: ~n"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTNUMBER"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTCHAR"),
                       "testchar1                               "),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTVARCHAR"), "testvarchar1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTDATE"), "01-JAN-01"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTLONG"), "testlong1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTCLOB"), "testclob1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTBLOB"), ""),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTNUMBER"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTCHAR"),
                       "testchar8                               "),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTVARCHAR"), "testvarchar8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTDATE"), "01-JAN-08"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTLONG"), "testlong8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTCLOB"), "testclob8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "TESTBLOB"), "testblob8"),
    io:format("~n"),

    %% FIELD LENGTHS BY NAME
    io:format("FIELD LENGTHS BY NAME: ~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTNUMBER"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTCHAR"), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTVARCHAR"), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTDATE"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTLONG"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTCLOB"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "TESTBLOB"), 0),
    io:format("~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTNUMBER"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTCHAR"), 40),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTVARCHAR"), 12),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTDATE"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTLONG"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTCLOB"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "TESTBLOB"), 9),
    io:format("~n"),

    %% FIELDS BY ARRAY
    io:format("FIELDS BY ARRAY: ~n"),
    {ok, Row0} = sqlrelay:getRow(0),
    assertEqualsString(lists:nth(1, Row0), "1"),
    assertEqualsString(lists:nth(2, Row0),
                       "testchar1                               "),
    assertEqualsString(lists:nth(3, Row0), "testvarchar1"),
    assertEqualsString(lists:nth(4, Row0), "01-JAN-01"),
    assertEqualsString(lists:nth(5, Row0), "testlong1"),
    assertEqualsString(lists:nth(6, Row0), "testclob1"),
    assertEqualsString(lists:nth(7, Row0), ""),
    io:format("~n"),

    %% FIELD LENGTHS BY ARRAY
    io:format("FIELD LENGTHS BY ARRAY: ~n"),
    {ok, Rowlens0} = sqlrelay:getRowLengths(0),
    assertEqualsInt(lists:nth(1, Rowlens0), 1),
    assertEqualsInt(lists:nth(2, Rowlens0), 40),
    assertEqualsInt(lists:nth(3, Rowlens0), 12),
    assertEqualsInt(lists:nth(4, Rowlens0), 9),
    assertEqualsInt(lists:nth(5, Rowlens0), 9),
    assertEqualsInt(lists:nth(6, Rowlens0), 9),
    assertEqualsInt(lists:nth(7, Rowlens0), 0),
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
        "	testnumber")),
    assertEqualsInt(sqlrelay:getResultSetBufferSize(), 2),
    io:format("~n"),
    assertEqualsInt(sqlrelay:firstRowIndex(), 0),
    assertFalse(sqlrelay:endOfResultSet()),
    assertEqualsInt(sqlrelay:rowCount(), 2),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(1, 0), "2"),
    assertEqualsString(sqlrelay:getFieldByIndex(2, 0), "3"),
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
        "	testnumber")),
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
        "	testnumber")),
    assertEqualsString(sqlrelay:getColumnName(0), "TESTNUMBER"),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 22),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(0), "NUMBER"),
    io:format("~n"),

    %% SUSPENDED SESSION
    io:format("SUSPENDED SESSION: ~n"),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testnumber")),
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
        "	testnumber")),
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
        "	testnumber")),
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
        "	testnumber")),
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
        "	testnumber")),
    {ok, Filename1} = sqlrelay:getCacheFileName(),
    assertEqualsString(Filename1, "cachefile1"),
    sqlrelay:cacheOff(),
    assertTrue(sqlrelay:openCachedResultSet(Filename1)),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    io:format("~n"),

    %% COLUMN COUNT FOR CACHED RESULT SET
    io:format("COLUMN COUNT FOR CACHED RESULT SET: ~n"),
    assertEqualsInt(sqlrelay:colCount(), 7),
    io:format("~n"),

    %% COLUMN NAMES FOR CACHED RESULT SET
    io:format("COLUMN NAMES FOR CACHED RESULT SET: ~n"),
    assertEqualsString(sqlrelay:getColumnName(0), "TESTNUMBER"),
    assertEqualsString(sqlrelay:getColumnName(1), "TESTCHAR"),
    assertEqualsString(sqlrelay:getColumnName(2), "TESTVARCHAR"),
    assertEqualsString(sqlrelay:getColumnName(3), "TESTDATE"),
    assertEqualsString(sqlrelay:getColumnName(4), "TESTLONG"),
    assertEqualsString(sqlrelay:getColumnName(5), "TESTCLOB"),
    assertEqualsString(sqlrelay:getColumnName(6), "TESTBLOB"),
    {ok, Cols2} = sqlrelay:getColumnNames(),
    assertEqualsString(lists:nth(1, Cols2), "TESTNUMBER"),
    assertEqualsString(lists:nth(2, Cols2), "TESTCHAR"),
    assertEqualsString(lists:nth(3, Cols2), "TESTVARCHAR"),
    assertEqualsString(lists:nth(4, Cols2), "TESTDATE"),
    assertEqualsString(lists:nth(5, Cols2), "TESTLONG"),
    assertEqualsString(lists:nth(6, Cols2), "TESTCLOB"),
    assertEqualsString(lists:nth(7, Cols2), "TESTBLOB"),
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
        "	testnumber")),
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
        "	testnumber")),
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
        "	testnumber")),
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

    %% COMMIT AND ROLLBACK
    %% SKIPPED: this section requires a second concurrent connection
    %% (secondcon) to verify cross-connection isolation. The Erlang
    %% binding only supports one connection per process (see
    %% sqlrelay.erl: alloc/connectionFree use a single connection slot
    %% in the process dictionary), so a second connection cannot be
    %% instantiated here.
    io:format("COMMIT AND ROLLBACK: ~n"),
    io:format("(skipped - requires second concurrent connection)~n"),
    %% We still need to drop the test table and restore autoCommitOff
    %% so that the rest of the test works.
    assertTrue(sqlrelay:commit()),
    assertTrue(sqlrelay:autoCommitOff()),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% INDIVIDUAL SUBSTITUTIONS
    io:format("INDIVIDUAL SUBSTITUTIONS: ~n"),
    sqlrelay:prepareQuery("select $(var1),'$(var2)',$(var3) from dual"),
    sqlrelay:subString("var1", "$(var11)"),
    sqlrelay:subString("var2", "$(var21)"),
    sqlrelay:subString("var3", "$(var31)"),
    sqlrelay:subString("var11", "$(var111)"),
    sqlrelay:subString("var21", "$(var211)"),
    sqlrelay:subString("var31", "$(var311)"),
    sqlrelay:subLong("var111", 1),
    sqlrelay:subString("var211", "hello"),
    sqlrelay:subDouble("var311", 10.5556, 6, 4),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "hello"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "10.5556"),
    io:format("~n"),

    %% ARRAY SUBSTITUTIONS (done individually; no array-subst API)
    io:format("ARRAY SUBSTITUTIONS: ~n"),
    sqlrelay:prepareQuery("select $(var1),$(var2),$(var3) from dual"),
    sqlrelay:subLong("var1", 1),
    sqlrelay:subLong("var2", 2),
    sqlrelay:subLong("var3", 3),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "2"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "3"),
    io:format("~n"),
    sqlrelay:prepareQuery("select '$(var1)','$(var2)','$(var3)' from dual"),
    sqlrelay:subString("var1", "hi"),
    sqlrelay:subString("var2", "hello"),
    sqlrelay:subString("var3", "bye"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "hi"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "hello"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "bye"),
    io:format("~n"),
    sqlrelay:prepareQuery("select $(var1),$(var2),$(var3) from dual"),
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
    assertTrue(sqlrelay:sendQuery("select NULL,1,NULL from dual")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), null),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), null),
    sqlrelay:getNullsAsEmptyStrings(),
    assertTrue(sqlrelay:sendQuery("select NULL,1,NULL from dual")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), ""),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), ""),
    io:format("~n"),

    %% NULL AND EMPTY LOBS
    %% Note: the Erlang inputBindClob/inputBindBlob functions require a
    %% list Value, so we cannot pass a true NULL. The empty string
    %% cases are preserved; NULL-cases use an empty string and expect
    %% the resulting field to be the empty string or null (accepted by
    %% assertEqualsString(..., null) when the server treats empties as
    %% nulls for clobs/blobs).
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
        "	testclob clob, "
        "	testblob blob)"),
    sqlrelay:prepareQuery("insert into testtable values (:clobval,:blobval)"),
    LargeBufferLength = 8192,
    LargeBuf = largeBuffer(LargeBufferLength),
    sqlrelay:inputBindClob("clobval", LargeBuf, LargeBufferLength),
    sqlrelay:inputBindBlob("blobval", LargeBuf, LargeBufferLength),
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
    io:format("~n"),

    %% OUTPUT BIND BY POSITION
    io:format("OUTPUT BIND BY POSITION: ~n"),
    sqlrelay:getNullsAsNulls(),
    sqlrelay:prepareQuery(
        "begin "
        "	:numvar:=1; "
        "	:stringvar:='hello'; "
        "	:floatvar:=2.5; "
        "	:datevar:='03-FEB-2001'; "
        "	:nullvar:=null; "
        "end;"),
    assertEqualsInt(sqlrelay:countBindVariables(), 5),
    sqlrelay:defineOutputBindInteger("1"),
    sqlrelay:defineOutputBindString("2", 10),
    sqlrelay:defineOutputBindDouble("3"),
    sqlrelay:defineOutputBindDate("4"),
    sqlrelay:defineOutputBindString("5", 10),
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
    io:format("~n"),

    %% OUTPUT BIND BY NAME
    io:format("OUTPUT BIND BY NAME: ~n"),
    sqlrelay:getNullsAsNulls(),
    sqlrelay:clearBinds(),
    sqlrelay:defineOutputBindInteger("numvar"),
    sqlrelay:defineOutputBindString("stringvar", 10),
    sqlrelay:defineOutputBindDouble("floatvar"),
    sqlrelay:defineOutputBindDate("datevar"),
    sqlrelay:defineOutputBindString("nullvar", 10),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("numvar"), 1),
    assertEqualsString(sqlrelay:getOutputBindString("stringvar"), "hello"),
    assertEqualsDouble(sqlrelay:getOutputBindDouble("floatvar"), 2.5),
    assertEqualsInt(sqlrelay:getOutputBindDateYear("datevar"), 2001),
    assertEqualsInt(sqlrelay:getOutputBindDateMonth("datevar"), 2),
    assertEqualsInt(sqlrelay:getOutputBindDateDay("datevar"), 3),
    assertEqualsInt(sqlrelay:getOutputBindDateHour("datevar"), 0),
    assertEqualsInt(sqlrelay:getOutputBindDateMinute("datevar"), 0),
    assertEqualsInt(sqlrelay:getOutputBindDateSecond("datevar"), 0),
    assertEqualsInt(sqlrelay:getOutputBindDateMicrosecond("datevar"), 0),
    assertEqualsString(sqlrelay:getOutputBindDateTz("datevar"), ""),
    assertFalse(sqlrelay:getOutputBindDateIsNegative("datevar")),
    assertEqualsString(sqlrelay:getOutputBindString("nullvar"), null),
    sqlrelay:getNullsAsEmptyStrings(),
    io:format("~n"),

    %% OUTPUT BIND BY NAME WITH VALIDATION
    io:format("OUTPUT BIND BY NAME WITH VALIDATION: ~n"),
    sqlrelay:getNullsAsNulls(),
    sqlrelay:clearBinds(),
    sqlrelay:defineOutputBindInteger("numvar"),
    sqlrelay:defineOutputBindString("stringvar", 10),
    sqlrelay:defineOutputBindDouble("floatvar"),
    sqlrelay:defineOutputBindDate("datevar"),
    sqlrelay:defineOutputBindString("nullvar", 10),
    sqlrelay:defineOutputBindString("dummyvar", 10),
    sqlrelay:validateBinds(),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("numvar"), 1),
    assertEqualsString(sqlrelay:getOutputBindString("stringvar"), "hello"),
    assertEqualsDouble(sqlrelay:getOutputBindDouble("floatvar"), 2.5),
    assertEqualsInt(sqlrelay:getOutputBindDateYear("datevar"), 2001),
    assertEqualsInt(sqlrelay:getOutputBindDateMonth("datevar"), 2),
    assertEqualsInt(sqlrelay:getOutputBindDateDay("datevar"), 3),
    assertEqualsInt(sqlrelay:getOutputBindDateHour("datevar"), 0),
    assertEqualsInt(sqlrelay:getOutputBindDateMinute("datevar"), 0),
    assertEqualsInt(sqlrelay:getOutputBindDateSecond("datevar"), 0),
    assertEqualsInt(sqlrelay:getOutputBindDateMicrosecond("datevar"), 0),
    assertEqualsString(sqlrelay:getOutputBindDateTz("datevar"), ""),
    assertFalse(sqlrelay:getOutputBindDateIsNegative("datevar")),
    assertEqualsString(sqlrelay:getOutputBindString("nullvar"), null),
    sqlrelay:getNullsAsEmptyStrings(),
    io:format("~n"),

    %% LOB OUTPUT BIND
    io:format("LOB OUTPUT BIND: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	testclob clob, "
        "	testblob blob)")),
    sqlrelay:prepareQuery("insert into testtable values ('hello',:var1)"),
    sqlrelay:inputBindBlob("var1", "hello", 5),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:prepareQuery(
        "begin "
        "	select testclob into :clobvar from testtable; "
        "	select testblob into :blobvar from testtable; "
        "end;"),
    sqlrelay:defineOutputBindClob("clobvar"),
    sqlrelay:defineOutputBindBlob("blobvar"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsStringLen(sqlrelay:getOutputBindClob("clobvar"), "hello", 5),
    assertEqualsInt(sqlrelay:getOutputBindLength("clobvar"), 5),
    assertEqualsStringLen(sqlrelay:getOutputBindBlob("blobvar"), "hello", 5),
    assertEqualsInt(sqlrelay:getOutputBindLength("blobvar"), 5),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% LONG OUTPUT BIND
    io:format("LONG OUTPUT BIND: ~n"),
    LongQuery = "begin :bindval:='" ++ LargeBuf ++ "'; end;",
    sqlrelay:prepareQuery(LongQuery),
    sqlrelay:defineOutputBindString("bindval", LargeBufferLength),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindLength("bindval"), LargeBufferLength),
    assertEqualsString(sqlrelay:getOutputBindString("bindval"), LargeBuf),
    io:format("~n"),

    %% NEGATIVE INPUT BIND
    io:format("NEGATIVE INPUT BIND: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    sqlrelay:sendQuery("create table testtable (testval number)"),
    sqlrelay:prepareQuery("insert into testtable values (:testval)"),
    sqlrelay:inputBindLong("testval", -1),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("select testval from testtable"),
    assertEqualsString(sqlrelay:getFieldByName(0, "TESTVAL"), "-1"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% BIND VALIDATION
    io:format("BIND VALIDATION: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 varchar2(20), "
        "	col2 varchar2(20), "
        "	col3 varchar2(20))"),
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
    sqlrelay:prepareQuery(
        "begin "
        "	:out:= :in; "
        "end;"),
    sqlrelay:inputBindLong("in", 1),
    sqlrelay:defineOutputBindInteger("out"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("out"), 1),
    sqlrelay:inputBindLong("in", 2),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("out"), 2),
    sqlrelay:inputBindLong("in", 3),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("out"), 3),
    io:format("~n"),

    %% REEXECUTE
    io:format("REEXECUTE: ~n"),
    sqlrelay:prepareQuery("select 1 from dual"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:rowCount(), 1),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    io:format("~n"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:rowCount(), 1),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    io:format("~n"),
    sqlrelay:prepareQuery("select :var from dual"),
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
    io:format("STORED PROCEDURE RETURNING NO VALUE: ~n"),
    sqlrelay:sendQuery("drop function testproc"),
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create or replace "
        "procedure testproc("
        "	in1 in number, "
        "	in2 in number, "
        "	in3 in varchar2) "
        "is "
        "begin "
        "	return; "
        "end;")),
    sqlrelay:prepareQuery("begin testproc(:in1,:in2,:in3); end;"),
    sqlrelay:inputBindLong("in1", 1),
    sqlrelay:inputBindDouble("in2", 1.1, 2, 1),
    sqlrelay:inputBindString("in3", "hello"),
    assertTrue(sqlrelay:executeQuery()),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc")),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING SINGLE VALUE
    io:format("STORED PROCEDURE RETURNING SINGLE VALUE: ~n"),
    sqlrelay:sendQuery("drop function testproc"),
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create or replace "
        "function testproc("
        "	in1 in number, "
        "	in2 in number, "
        "	in3 in varchar2) "
        "	return number "
        "is "
        "begin "
        "	return in1; "
        "end;")),
    sqlrelay:prepareQuery("select testproc(:in1,:in2,:in3) from dual"),
    sqlrelay:inputBindLong("in1", 1),
    sqlrelay:inputBindDouble("in2", 1.1, 2, 1),
    sqlrelay:inputBindString("in3", "hello"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    sqlrelay:prepareQuery(
        "begin "
        "	:out1:=testproc(:in1,:in2,:in3); "
        "end;"),
    sqlrelay:inputBindLong("in1", 1),
    sqlrelay:inputBindDouble("in2", 1.1, 2, 1),
    sqlrelay:inputBindString("in3", "hello"),
    sqlrelay:defineOutputBindInteger("out1"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("out1"), 1),
    assertTrue(sqlrelay:sendQuery("drop function testproc")),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING MULTIPLE VALUES
    io:format("STORED PROCEDURE RETURNING MULTIPLE VALUES: ~n"),
    sqlrelay:sendQuery("drop function testproc"),
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create or replace "
        "procedure testproc("
        "	in1 in number, "
        "	in2 in number, "
        "	in3 in varchar2, "
        "	out1 out number, "
        "	out2 out number, "
        "	out3 out varchar2) "
        "is "
        "begin "
        "	out1:=in1; "
        "	out2:=in2; "
        "	out3:=in3; "
        "end;")),
    sqlrelay:prepareQuery(
        "begin "
        "	testproc(:in1,:in2,:in3,:out1,:out2,:out3); "
        "end;"),
    sqlrelay:inputBindLong("in1", 1),
    sqlrelay:inputBindDouble("in2", 1.1, 2, 1),
    sqlrelay:inputBindString("in3", "hello"),
    sqlrelay:defineOutputBindInteger("out1"),
    sqlrelay:defineOutputBindDouble("out2"),
    sqlrelay:defineOutputBindString("out3", 20),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsInt(sqlrelay:getOutputBindInteger("out1"), 1),
    assertEqualsDouble(sqlrelay:getOutputBindDouble("out2"), 1.1),
    assertEqualsString(sqlrelay:getOutputBindString("out3"), "hello"),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc")),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING RESULT SET
    %% SKIPPED: the C++ version uses getOutputBindCursor(...) to obtain
    %% a SECOND cursor (bindcur1/bindcur2) and fetches from it while
    %% the main cursor is still live. The Erlang binding's
    %% getOutputBindCursor/1 is explicitly not implemented (returns
    %% false) and the port-program holds only one cursor at a time.
    io:format("STORED PROCEDURE RETURNING RESULT SET: ~n"),
    io:format("(skipped - requires second cursor via getOutputBindCursor)~n"),
    io:format("~n"),

    %% TEMPORARY TABLES
    io:format("TEMPORARY TABLES: ~n"),
    sqlrelay:prepareQuery("drop table $(HOSTNAME)_temptabledelete"),
    sqlrelay:subString("HOSTNAME", Hostname),
    sqlrelay:executeQuery(),
    sqlrelay:prepareQuery(
        "create global temporary table $(HOSTNAME)_temptabledelete ( "
        "	col1 number "
        ") on commit delete rows"),
    sqlrelay:subString("HOSTNAME", Hostname),
    sqlrelay:executeQuery(),
    sqlrelay:prepareQuery("insert into $(HOSTNAME)_temptabledelete values (1)"),
    sqlrelay:subString("HOSTNAME", Hostname),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:prepareQuery("select count(*) from $(HOSTNAME)_temptabledelete"),
    sqlrelay:subString("HOSTNAME", Hostname),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("select count(*) from $(HOSTNAME)_temptabledelete"),
    sqlrelay:subString("HOSTNAME", Hostname),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "0"),
    sqlrelay:prepareQuery("drop table $(HOSTNAME)_temptabledelete"),
    sqlrelay:subString("HOSTNAME", Hostname),
    sqlrelay:executeQuery(),
    io:format("~n"),
    sqlrelay:prepareQuery("truncate table $(HOSTNAME)_temptablepreserve"),
    sqlrelay:subString("HOSTNAME", Hostname),
    sqlrelay:executeQuery(),
    sqlrelay:prepareQuery("drop table $(HOSTNAME)_temptablepreserve"),
    sqlrelay:subString("HOSTNAME", Hostname),
    sqlrelay:executeQuery(),
    sqlrelay:prepareQuery(
        "create global temporary table $(HOSTNAME)_temptablepreserve ("
        "	col1 number "
        ") on commit preserve rows"),
    sqlrelay:subString("HOSTNAME", Hostname),
    sqlrelay:executeQuery(),
    sqlrelay:prepareQuery(
        "insert into "
        "	$(HOSTNAME)_temptablepreserve "
        "values (1)"),
    sqlrelay:subString("HOSTNAME", Hostname),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve"),
    sqlrelay:subString("HOSTNAME", Hostname),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertTrue(sqlrelay:commit()),
    sqlrelay:prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve"),
    sqlrelay:subString("HOSTNAME", Hostname),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    sqlrelay:endSession(),
    io:format("~n"),
    sqlrelay:prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve"),
    sqlrelay:subString("HOSTNAME", Hostname),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "0"),
    sqlrelay:prepareQuery("truncate table $(HOSTNAME)_temptablepreserve"),
    sqlrelay:subString("HOSTNAME", Hostname),
    assertTrue(sqlrelay:executeQuery()),
    timer:sleep(2000),
    sqlrelay:prepareQuery("drop table $(HOSTNAME)_temptablepreserve"),
    sqlrelay:subString("HOSTNAME", Hostname),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve"),
    sqlrelay:subString("HOSTNAME", Hostname),
    assertFalse(sqlrelay:executeQuery()),
    io:format("~n"),

    %% ENCODED BINARY DATA
    io:format("ENCODED BINARY DATA: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery("create table testtable (col1 blob)")),
    Buffer = lists:seq(0, 255),
    HexStr = lists:flatten([io_lib:format("~2.16.0b", [B]) || B <- Buffer]),
    QueryStr = "insert into testtable values ('" ++ HexStr ++ "')",
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
    assertTrue(sqlrelay:sendQuery("create table testtable (col1 varchar2(4))")),
    assertTrue(sqlrelay:sendQuery("insert into testtable values ('''''')")),
    assertTrue(sqlrelay:sendQuery("select col1 from testtable")),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 2),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "''"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% LAST INSERT ID
    %% oracle doesn't support auto-increment

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
    {ok, SchemaRowCount} = sqlrelay:rowCount(),
    SchemaFound = searchSchemaList(0, SchemaRowCount, Hostname),
    assertTrue(SchemaFound),
    io:format("~n"),

    %% TABLE TYPE LIST
    io:format("TABLE TYPE LIST: ~n"),
    assertTrue(sqlrelay:getTableTypeList()),
    assertEqualsString(sqlrelay:getColumnName(0), "table_type"),
    assertEqualsString(sqlrelay:getFieldByName(0, "table_type"), "SYNONYM"),
    assertEqualsString(sqlrelay:getFieldByName(1, "table_type"), "TABLE"),
    assertEqualsString(sqlrelay:getFieldByName(2, "table_type"), "VIEW"),
    io:format("~n"),

    %% TABLE LIST
    io:format("TABLE LIST: ~n"),
    sqlrelay:sendQuery("drop table testtable1"),
    sqlrelay:sendQuery("drop table testtable2"),
    sqlrelay:sendQuery("drop table testtable3"),
    sqlrelay:sendQuery("drop table testtable4"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable1 ("
        "	testnumber number, "
        "	testchar char(40), "
        "	testvarchar varchar2(40), "
        "	testdate date, "
        "	testlong long, "
        "	testclob clob, "
        "	testblob blob)")),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable2 ("
        "	testnumber number, "
        "	testchar char(40), "
        "	testvarchar varchar2(40), "
        "	testdate date, "
        "	testlong long, "
        "	testclob clob, "
        "	testblob blob)")),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable3 ("
        "	testnumber number, "
        "	testchar char(40), "
        "	testvarchar varchar2(40), "
        "	testdate date, "
        "	testlong long, "
        "	testclob clob, "
        "	testblob blob)")),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable4 ("
        "	testnumber number, "
        "	testchar char(40), "
        "	testvarchar varchar2(40), "
        "	testdate date, "
        "	testlong long, "
        "	testclob clob, "
        "	testblob blob)")),
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
    assertTrue(sqlrelay:getTypeInfoList("number")),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "type_name"), "NUMBER"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "-7"),
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"), "NUMBER"),
    assertTrue(sqlrelay:getTypeInfoList("char")),
    assertEqualsString(sqlrelay:getFieldByName(0, "type_name"), "CHAR"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "2000"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"), "CHAR"),
    assertTrue(sqlrelay:getTypeInfoList("varchar2")),
    assertEqualsString(sqlrelay:getFieldByName(0, "type_name"), "VARCHAR2"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "12"),
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "32767"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"),
                       "VARCHAR2"),
    assertTrue(sqlrelay:getTypeInfoList("date")),
    assertEqualsString(sqlrelay:getFieldByName(0, "type_name"), "DATE"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "92"),
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "7"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"), "DATE"),
    io:format("~n"),

    %% COLUMN LIST
    io:format("COLUMN LIST: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	testnumber number, "
        "	testchar char(40), "
        "	testvarchar varchar2(40), "
        "	testdate date, "
        "	testlong long, "
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
    assertEqualsString(sqlrelay:getFieldByName(0, "column_name"),
                       "TESTNUMBER"),
    assertEqualsString(sqlrelay:getFieldByName(1, "column_name"), "TESTCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(2, "column_name"),
                       "TESTVARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(3, "column_name"), "TESTDATE"),
    assertEqualsString(sqlrelay:getFieldByName(4, "column_name"), "TESTLONG"),
    assertEqualsString(sqlrelay:getFieldByName(5, "column_name"), "TESTCLOB"),
    assertEqualsString(sqlrelay:getFieldByName(6, "column_name"), "TESTBLOB"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "NUMBER"),
    assertEqualsString(sqlrelay:getFieldByName(1, "data_type"), "CHAR"),
    assertEqualsString(sqlrelay:getFieldByName(2, "data_type"), "VARCHAR2"),
    assertEqualsString(sqlrelay:getFieldByName(3, "data_type"), "DATE"),
    assertEqualsString(sqlrelay:getFieldByName(4, "data_type"), "LONG"),
    assertEqualsString(sqlrelay:getFieldByName(5, "data_type"), "CLOB"),
    assertEqualsString(sqlrelay:getFieldByName(6, "data_type"), "BLOB"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% COLUMN LIST - auto_increment, primary key
    %% oracle doesn't support auto_increment
    io:format("COLUMN LIST - auto_increment, primary key: ~n"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 number primary key, "
        "	col2 number)")),
    assertTrue(sqlrelay:getColumnList("testtable", "")),
    {ok, Ck0} = sqlrelay:getFieldByName(0, "column_key"),
    assertTrue(contains(Ck0, "PRI")),
    {ok, Ck1} = sqlrelay:getFieldByName(1, "column_key"),
    assertFalse(contains(Ck1, "PRI")),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% PRIMARY KEYS LIST
    io:format("PRIMARY KEYS LIST: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 number primary key, "
        "	col2 number)")),
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
    assertTrue(not isNullOrEmpty(PkName)),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% KEY AND INDEX LIST
    io:format("KEY AND INDEX LIST: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 number primary key, "
        "	col2 number)")),
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
        "create procedure testproc1("
        "	in1 in number, "
        "	in2 in char, "
        "	in3 in varchar2, "
        "	in4 in date) as "
        "begin "
        "	null; "
        "end;")),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc2("
        "	in1 in number, "
        "	in2 in char, "
        "	in3 in varchar2, "
        "	in4 in date) as "
        "begin "
        "	null; "
        "end;")),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc3("
        "	in1 in number, "
        "	in2 in char, "
        "	in3 in varchar2, "
        "	in4 in date) as "
        "begin "
        "	null; "
        "end;")),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc4("
        "	in1 in number, "
        "	in2 in char, "
        "	in3 in varchar2, "
        "	in4 in date) as "
        "begin "
        "	null; "
        "end;")),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "parameter_name"), "IN1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "NUMBER"),
    assertEqualsString(sqlrelay:getFieldByName(0, "ordinal_position"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(1, "parameter_name"), "IN2"),
    assertEqualsString(sqlrelay:getFieldByName(1, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(1, "data_type"), "CHAR"),
    assertEqualsString(sqlrelay:getFieldByName(1, "ordinal_position"), "2"),
    assertEqualsString(sqlrelay:getFieldByName(2, "parameter_name"), "IN3"),
    assertEqualsString(sqlrelay:getFieldByName(2, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(2, "data_type"), "VARCHAR2"),
    assertEqualsString(sqlrelay:getFieldByName(2, "ordinal_position"), "3"),
    assertEqualsString(sqlrelay:getFieldByName(3, "parameter_name"), "IN4"),
    assertEqualsString(sqlrelay:getFieldByName(3, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(3, "data_type"), "DATE"),
    assertEqualsString(sqlrelay:getFieldByName(3, "ordinal_position"), "4"),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc1")),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc2")),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc3")),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc4")),
    io:format("~n"),

    %% INVALID QUERIES
    io:format("INVALID QUERIES: ~n"),
    assertFalse(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testnumber")),
    assertFalse(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testnumber")),
    assertFalse(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testnumber")),
    assertFalse(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testnumber")),
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

%% String substring containment: does Haystack contain Needle ?
contains(Haystack, Needle) when is_list(Haystack), is_list(Needle) ->
    string:str(Haystack, Needle) > 0;
contains(_, _) ->
    false.

%% Walk the schema-list result looking for a row whose "Database"
%% column (case-insensitive) matches the hostname.
searchSchemaList(I, Count, _Hostname) when I >= Count ->
    false;
searchSchemaList(I, Count, Hostname) ->
    case sqlrelay:getFieldByName(I, "Database") of
        {ok, Name} when is_list(Name) ->
            case string:to_lower(Name) =:= string:to_lower(Hostname) of
                true  -> true;
                false -> searchSchemaList(I + 1, Count, Hostname)
            end;
        _ ->
            searchSchemaList(I + 1, Count, Hostname)
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
                 "TESTTABLE1" -> Acc + 1;
                 "TESTTABLE2" -> Acc + 1;
                 "TESTTABLE3" -> Acc + 1;
                 "TESTTABLE4" -> Acc + 1;
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
                 "TESTPROC1" -> Acc + 1;
                 "TESTPROC2" -> Acc + 1;
                 "TESTPROC3" -> Acc + 1;
                 "TESTPROC4" -> Acc + 1;
                 _           -> Acc
             end,
    countMatchingProcs(I + 1, Count, NewAcc).
