%% Copyright (c) David Muse
%% See the file COPYING for more information.

-module(mysql).
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

    Hostname = shortHostname(),

    %% IDENTIFY
    io:format("IDENTIFY: ~n"),
    assertEqualsString(sqlrelay:identify(), "mysql"),
    io:format("~n"),

    %% DB VERSION
    io:format("DB VERSION: ~n"),
    {ok, DbVersion} = sqlrelay:dbVersion(),
    MajorVersion = hd(DbVersion) - $0,
    io:format("~n"),

    %% PING
    io:format("PING: ~n"),
    assertTrue(sqlrelay:ping()),
    io:format("~n"),

    %% TRANSACTION STATE
    io:format("TRANSACTION STATE: ~n"),
    assertEqualsString(sqlrelay:getDefaultTransactionModel(), "explicit-deferred"),
    assertEqualsString(sqlrelay:getTransactionModel(), "explicit-deferred"),
    assertFalse(sqlrelay:getInTransaction()),
    assertTrue(sqlrelay:getAutoCommit()),
    io:format("~n"),

    %% BIND FORMAT
    io:format("BIND FORMAT: ~n"),
    case MajorVersion > 3 of
        true -> assertEqualsString(sqlrelay:bindFormat(), "?");
        false -> assertEqualsString(sqlrelay:bindFormat(), ":*")
    end,
    io:format("~n"),

    %% NEXTVAL FORMAT
    io:format("NEXTVAL FORMAT: ~n"),
    assertEqualsString(sqlrelay:nextvalFormat(), ""),
    io:format("~n"),

    %% ISOLATION LEVELS
    %% (mysql before 4.0 doesn't support setting the isolation level)
    io:format("ISOLATION LEVELS: ~n"),
    IsolationLevels = ["REPEATABLE-READ", "READ-UNCOMMITTED",
                       "READ-COMMITTED", "SERIALIZABLE"],
    case MajorVersion > 3 of
        true ->
            setIsolationLevels(IsolationLevels),
            %% reset to the default isolation level
            assertTrue(sqlrelay:setIsolationLevel(hd(IsolationLevels)));
        false -> ok
    end,
    io:format("~n"),

    %% CREATE TESTTABLE
    io:format("CREATE TESTTABLE: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	testtinyint tinyint, "
        "	testsmallint smallint, "
        "	testmediumint mediumint, "
        "	testint int, "
        "	testbigint bigint, "
        "	testfloat float, "
        "	testreal real, "
        "	testdecimal decimal(2,1), "
        "	testdate date, "
        "	testtime time, "
        "	testdatetime datetime, "
        "	testyear year, "
        "	testchar char(40), "
        "	testvarchar varchar(40), "
        "	testtext text, "
        "	testtinytext tinytext, "
        "	testmediumtext mediumtext, "
        "	testlongtext longtext, "
        "	testblob blob, "
        "	testtinyblob tinyblob, "
        "	testmediumblob mediumblob, "
        "	testlongblob longblob, "
        "	testtimestamp timestamp)")),
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
        "	1, "
        "	1, "
        "	1.5, "
        "	1.5, "
        "	1.5, "
        "	'2001-01-01', "
        "	'01:00:00', "
        "	'2001-01-01 01:00:00', "
        "	'2001', "
        "	'char1', "
        "	'varchar1', "
        "	'text1', "
        "	'tinytext1', "
        "	'mediumtext1', "
        "	'longtext1', "
        "	'blob1', "
        "	'tinyblob1', "
        "	'mediumblob1', "
        "	'longblob1', "
        "	NULL)")),
    assertTrue(sqlrelay:sendQuery(
        "insert into "
        "	testtable "
        "values ("
        "	2, "
        "	2, "
        "	2, "
        "	2, "
        "	2, "
        "	2.5, "
        "	2.5, "
        "	2.5, "
        "	'2002-01-01', "
        "	'02:00:00', "
        "	'2002-01-01 02:00:00', "
        "	'2002', "
        "	'char2', "
        "	'varchar2', "
        "	'text2', "
        "	'tinytext2', "
        "	'mediumtext2', "
        "	'longtext2', "
        "	'blob2', "
        "	'tinyblob2', "
        "	'mediumblob2', "
        "	'longblob2', "
        "	NULL)")),
    assertTrue(sqlrelay:sendQuery(
        "insert into "
        "	testtable "
        "values ("
        "	3, "
        "	3, "
        "	3, "
        "	3, "
        "	3, "
        "	3.5, "
        "	3.5, "
        "	3.5, "
        "	'2003-01-01', "
        "	'03:00:00', "
        "	'2003-01-01 03:00:00', "
        "	'2003', "
        "	'char3', "
        "	'varchar3', "
        "	'text3', "
        "	'tinytext3', "
        "	'mediumtext3', "
        "	'longtext3', "
        "	'blob3', "
        "	'tinyblob3', "
        "	'mediumblob3', "
        "	'longblob3', "
        "	NULL)")),
    assertTrue(sqlrelay:sendQuery(
        "insert into "
        "	testtable "
        "values ("
        "	4, "
        "	4, "
        "	4, "
        "	4, "
        "	4, "
        "	4.5, "
        "	4.5, "
        "	4.5, "
        "	'2004-01-01', "
        "	'04:00:00', "
        "	'2004-01-01 04:00:00', "
        "	'2004', "
        "	'char4', "
        "	'varchar4', "
        "	'text4', "
        "	'tinytext4', "
        "	'mediumtext4', "
        "	'longtext4', "
        "	'blob4', "
        "	'tinyblob4', "
        "	'mediumblob4', "
        "	'longblob4', "
        "	NULL)")),
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
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	?, "
        "	NULL)"),
    assertEqualsInt(sqlrelay:countBindVariables(), 22),
    sqlrelay:inputBindLong("1", 5),
    sqlrelay:inputBindLong("2", 5),
    sqlrelay:inputBindLong("3", 5),
    sqlrelay:inputBindLong("4", 5),
    sqlrelay:inputBindLong("5", 5),
    sqlrelay:inputBindDouble("6", 5.5, 2, 1),
    sqlrelay:inputBindDouble("7", 5.5, 2, 1),
    sqlrelay:inputBindDouble("8", 5.5, 2, 1),
    sqlrelay:inputBindString("9", "2005-01-01"),
    sqlrelay:inputBindString("10", "05:00:00"),
    sqlrelay:inputBindDate("11", 2005, 1, 1, 5, 0, 0, 0, "", 0),
    sqlrelay:inputBindString("12", "2005"),
    sqlrelay:inputBindString("13", "char5"),
    sqlrelay:inputBindString("14", "varchar5"),
    sqlrelay:inputBindClob("15", "text5", 5),
    sqlrelay:inputBindClob("16", "tinytext5", 9),
    sqlrelay:inputBindClob("17", "mediumtext5", 11),
    sqlrelay:inputBindClob("18", "longtext5", 9),
    sqlrelay:inputBindBlob("19", "blob5", 5),
    sqlrelay:inputBindBlob("20", "tinyblob5", 9),
    sqlrelay:inputBindBlob("21", "mediumblob5", 11),
    sqlrelay:inputBindBlob("22", "longblob5", 9),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 6),
    sqlrelay:inputBindLong("2", 6),
    sqlrelay:inputBindLong("3", 6),
    sqlrelay:inputBindLong("4", 6),
    sqlrelay:inputBindLong("5", 6),
    sqlrelay:inputBindDouble("6", 6.5, 2, 1),
    sqlrelay:inputBindDouble("7", 6.5, 2, 1),
    sqlrelay:inputBindDouble("8", 6.5, 2, 1),
    sqlrelay:inputBindString("9", "2006-01-01"),
    sqlrelay:inputBindString("10", "06:00:00"),
    sqlrelay:inputBindDate("11", 2006, 1, 1, 6, 0, 0, 0, "", 0),
    sqlrelay:inputBindString("12", "2006"),
    sqlrelay:inputBindString("13", "char6"),
    sqlrelay:inputBindString("14", "varchar6"),
    sqlrelay:inputBindClob("15", "text6", 5),
    sqlrelay:inputBindClob("16", "tinytext6", 9),
    sqlrelay:inputBindClob("17", "mediumtext6", 11),
    sqlrelay:inputBindClob("18", "longtext6", 9),
    sqlrelay:inputBindBlob("19", "blob6", 5),
    sqlrelay:inputBindBlob("20", "tinyblob6", 9),
    sqlrelay:inputBindBlob("21", "mediumblob6", 11),
    sqlrelay:inputBindBlob("22", "longblob6", 9),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 7),
    sqlrelay:inputBindLong("2", 7),
    sqlrelay:inputBindLong("3", 7),
    sqlrelay:inputBindLong("4", 7),
    sqlrelay:inputBindLong("5", 7),
    sqlrelay:inputBindDouble("6", 7.5, 2, 1),
    sqlrelay:inputBindDouble("7", 7.5, 2, 1),
    sqlrelay:inputBindDouble("8", 7.5, 2, 1),
    sqlrelay:inputBindString("9", "2007-01-01"),
    sqlrelay:inputBindString("10", "07:00:00"),
    sqlrelay:inputBindDate("11", 2007, 1, 1, 7, 0, 0, 0, "", 0),
    sqlrelay:inputBindString("12", "2007"),
    sqlrelay:inputBindString("13", "char7"),
    sqlrelay:inputBindString("14", "varchar7"),
    sqlrelay:inputBindClob("15", "text7", 5),
    sqlrelay:inputBindClob("16", "tinytext7", 9),
    sqlrelay:inputBindClob("17", "mediumtext7", 11),
    sqlrelay:inputBindClob("18", "longtext7", 9),
    sqlrelay:inputBindBlob("19", "blob7", 5),
    sqlrelay:inputBindBlob("20", "tinyblob7", 9),
    sqlrelay:inputBindBlob("21", "mediumblob7", 11),
    sqlrelay:inputBindBlob("22", "longblob7", 9),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% ARRAY OF INPUT BINDS BY POSITION
    %% mysql doesn't support implicit conversion of string binds to other
    %% data types, so arrays of binds don't generally work.

    %% INPUT BIND BY POSITION WITH VALIDATION
    io:format("BIND BY POSITION WITH VALIDATION: ~n"),
    sqlrelay:clearBinds(),
    sqlrelay:inputBindLong("1", 8),
    sqlrelay:inputBindLong("2", 8),
    sqlrelay:inputBindLong("3", 8),
    sqlrelay:inputBindLong("4", 8),
    sqlrelay:inputBindLong("5", 8),
    sqlrelay:inputBindDouble("6", 8.5, 2, 1),
    sqlrelay:inputBindDouble("7", 8.5, 2, 1),
    sqlrelay:inputBindDouble("8", 8.5, 2, 1),
    sqlrelay:inputBindString("9", "2008-01-01"),
    sqlrelay:inputBindString("10", "08:00:00"),
    sqlrelay:inputBindDate("11", 2008, 1, 1, 8, 0, 0, 0, "", 0),
    sqlrelay:inputBindString("12", "2008"),
    sqlrelay:inputBindString("13", "char8"),
    sqlrelay:inputBindString("14", "varchar8"),
    sqlrelay:inputBindClob("15", "text8", 5),
    sqlrelay:inputBindClob("16", "tinytext8", 9),
    sqlrelay:inputBindClob("17", "mediumtext8", 11),
    sqlrelay:inputBindClob("18", "longtext8", 9),
    sqlrelay:inputBindBlob("19", "blob8", 5),
    sqlrelay:inputBindBlob("20", "tinyblob8", 9),
    sqlrelay:inputBindBlob("21", "mediumblob8", 11),
    sqlrelay:inputBindBlob("22", "longblob8", 9),
    sqlrelay:validateBinds(),
    assertTrue(sqlrelay:executeQuery()),
    io:format("~n"),

    %% INPUT BIND BY NAME
    %% mysql doesn't support bind by name

    %% ARRAY OF INPUT BINDS BY NAME
    %% mysql doesn't support bind by name

    %% INPUT BIND BY NAME WITH VALIDATION
    %% mysql doesn't support bind by name

    %% SELECT
    io:format("SELECT: ~n"),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testtinyint ")),
    io:format("~n"),

    %% COLUMN COUNT
    io:format("COLUMN COUNT: ~n"),
    assertEqualsInt(sqlrelay:colCount(), 23),
    io:format("~n"),

    %% COLUMN NAMES
    io:format("COLUMN NAMES: ~n"),
    assertEqualsString(sqlrelay:getColumnName(0), "testtinyint"),
    assertEqualsString(sqlrelay:getColumnName(1), "testsmallint"),
    assertEqualsString(sqlrelay:getColumnName(2), "testmediumint"),
    assertEqualsString(sqlrelay:getColumnName(3), "testint"),
    assertEqualsString(sqlrelay:getColumnName(4), "testbigint"),
    assertEqualsString(sqlrelay:getColumnName(5), "testfloat"),
    assertEqualsString(sqlrelay:getColumnName(6), "testreal"),
    assertEqualsString(sqlrelay:getColumnName(7), "testdecimal"),
    assertEqualsString(sqlrelay:getColumnName(8), "testdate"),
    assertEqualsString(sqlrelay:getColumnName(9), "testtime"),
    assertEqualsString(sqlrelay:getColumnName(10), "testdatetime"),
    assertEqualsString(sqlrelay:getColumnName(11), "testyear"),
    assertEqualsString(sqlrelay:getColumnName(12), "testchar"),
    assertEqualsString(sqlrelay:getColumnName(13), "testvarchar"),
    assertEqualsString(sqlrelay:getColumnName(14), "testtext"),
    assertEqualsString(sqlrelay:getColumnName(15), "testtinytext"),
    assertEqualsString(sqlrelay:getColumnName(16), "testmediumtext"),
    assertEqualsString(sqlrelay:getColumnName(17), "testlongtext"),
    assertEqualsString(sqlrelay:getColumnName(18), "testblob"),
    assertEqualsString(sqlrelay:getColumnName(19), "testtinyblob"),
    assertEqualsString(sqlrelay:getColumnName(20), "testmediumblob"),
    assertEqualsString(sqlrelay:getColumnName(21), "testlongblob"),
    assertEqualsString(sqlrelay:getColumnName(22), "testtimestamp"),
    {ok, Cols1} = sqlrelay:getColumnNames(),
    assertEqualsString(lists:nth(1, Cols1), "testtinyint"),
    assertEqualsString(lists:nth(2, Cols1), "testsmallint"),
    assertEqualsString(lists:nth(3, Cols1), "testmediumint"),
    assertEqualsString(lists:nth(4, Cols1), "testint"),
    assertEqualsString(lists:nth(5, Cols1), "testbigint"),
    assertEqualsString(lists:nth(6, Cols1), "testfloat"),
    assertEqualsString(lists:nth(7, Cols1), "testreal"),
    assertEqualsString(lists:nth(8, Cols1), "testdecimal"),
    assertEqualsString(lists:nth(9, Cols1), "testdate"),
    assertEqualsString(lists:nth(10, Cols1), "testtime"),
    assertEqualsString(lists:nth(11, Cols1), "testdatetime"),
    assertEqualsString(lists:nth(12, Cols1), "testyear"),
    assertEqualsString(lists:nth(13, Cols1), "testchar"),
    assertEqualsString(lists:nth(14, Cols1), "testvarchar"),
    assertEqualsString(lists:nth(15, Cols1), "testtext"),
    assertEqualsString(lists:nth(16, Cols1), "testtinytext"),
    assertEqualsString(lists:nth(17, Cols1), "testmediumtext"),
    assertEqualsString(lists:nth(18, Cols1), "testlongtext"),
    assertEqualsString(lists:nth(19, Cols1), "testblob"),
    assertEqualsString(lists:nth(20, Cols1), "testtinyblob"),
    assertEqualsString(lists:nth(21, Cols1), "testmediumblob"),
    assertEqualsString(lists:nth(22, Cols1), "testlongblob"),
    assertEqualsString(lists:nth(23, Cols1), "testtimestamp"),
    io:format("~n"),

    %% COLUMN TYPES
    io:format("COLUMN TYPES: ~n"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(0), "TINYINT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(1), "SMALLINT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(2), "MEDIUMINT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(3), "INT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(4), "BIGINT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(5), "FLOAT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(6), "REAL"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(7), "DECIMAL"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(8), "DATE"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(9), "TIME"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(10), "DATETIME"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(11), "YEAR"),
    case MajorVersion > 3 of
        true -> assertEqualsString(sqlrelay:getColumnTypeByIndex(12), "STRING");
        false -> assertEqualsString(sqlrelay:getColumnTypeByIndex(12), "VARSTRING")
    end,
    assertEqualsString(sqlrelay:getColumnTypeByIndex(13), "VARSTRING"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(14), "TEXT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(15), "TINYTEXT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(16), "MEDIUMTEXT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(17), "LONGTEXT"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(18), "BLOB"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(19), "TINYBLOB"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(20), "MEDIUMBLOB"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(21), "LONGBLOB"),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(22), "TIMESTAMP"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testtinyint"), "TINYINT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testsmallint"), "SMALLINT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testmediumint"), "MEDIUMINT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testint"), "INT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testbigint"), "BIGINT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testfloat"), "FLOAT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testreal"), "REAL"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testdecimal"), "DECIMAL"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testdate"), "DATE"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testtime"), "TIME"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testdatetime"), "DATETIME"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testyear"), "YEAR"),
    case MajorVersion > 3 of
        true -> assertEqualsString(sqlrelay:getColumnTypeByName("testchar"), "STRING");
        false -> assertEqualsString(sqlrelay:getColumnTypeByName("testchar"), "VARSTRING")
    end,
    assertEqualsString(sqlrelay:getColumnTypeByName("testvarchar"), "VARSTRING"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testtext"), "TEXT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testtinytext"), "TINYTEXT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testmediumtext"), "MEDIUMTEXT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testlongtext"), "LONGTEXT"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testblob"), "BLOB"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testtinyblob"), "TINYBLOB"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testmediumblob"), "MEDIUMBLOB"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testlongblob"), "LONGBLOB"),
    assertEqualsString(sqlrelay:getColumnTypeByName("testtimestamp"), "TIMESTAMP"),
    io:format("~n"),

    %% COLUMN LENGTH
    %% mysql before 4 reports column lengths differently (charset)
    case MajorVersion > 3 of
        true ->
    io:format("COLUMN LENGTH: ~n"),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 1),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(1), 2),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(2), 3),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(3), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(4), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(5), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(6), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(7), 6),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(8), 3),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(9), 3),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(10), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(11), 1),
    %% testchar/testvarchar are char(40)/varchar(40); the connection charset
    %% is latin1 (1 byte/char) so the lengths are 40/41
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(12), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(13), 41),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(14), 65535),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(15), 255),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(16), 16777215),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(17), 2147483647),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(18), 65535),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(19), 255),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(20), 16777215),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(21), 2147483647),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(22), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testtinyint"), 1),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testsmallint"), 2),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testmediumint"), 3),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testint"), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testbigint"), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testfloat"), 4),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testreal"), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testdecimal"), 6),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testdate"), 3),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testtime"), 3),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testdatetime"), 8),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testyear"), 1),
    %% testchar/testvarchar are char(40)/varchar(40); the connection charset
    %% is latin1 (1 byte/char) so the lengths are 40/41
    assertEqualsInt(sqlrelay:getColumnLengthByName("testchar"), 40),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testvarchar"), 41),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testtext"), 65535),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testtinytext"), 255),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testmediumtext"), 16777215),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testlongtext"), 2147483647),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testblob"), 65535),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testtinyblob"), 255),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testmediumblob"), 16777215),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testlongblob"), 2147483647),
    assertEqualsInt(sqlrelay:getColumnLengthByName("testtimestamp"), 4),
    io:format("~n");
        false -> ok
    end,

    %% LONGEST COLUMN
    io:format("LONGEST COLUMN: ~n"),
    assertEqualsInt(sqlrelay:getLongestByIndex(0), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(1), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(2), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(3), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(4), 1),
    assertEqualsInt(sqlrelay:getLongestByIndex(5), 3),
    assertEqualsInt(sqlrelay:getLongestByIndex(6), 3),
    assertEqualsInt(sqlrelay:getLongestByIndex(7), 3),
    assertEqualsInt(sqlrelay:getLongestByIndex(8), 10),
    assertEqualsInt(sqlrelay:getLongestByIndex(9), 8),
    assertEqualsInt(sqlrelay:getLongestByIndex(10), 19),
    assertEqualsInt(sqlrelay:getLongestByIndex(11), 4),
    assertEqualsInt(sqlrelay:getLongestByIndex(12), 5),
    assertEqualsInt(sqlrelay:getLongestByIndex(13), 8),
    assertEqualsInt(sqlrelay:getLongestByIndex(14), 5),
    assertEqualsInt(sqlrelay:getLongestByIndex(15), 9),
    assertEqualsInt(sqlrelay:getLongestByIndex(16), 11),
    assertEqualsInt(sqlrelay:getLongestByIndex(17), 9),
    assertEqualsInt(sqlrelay:getLongestByIndex(18), 5),
    assertEqualsInt(sqlrelay:getLongestByIndex(19), 9),
    assertEqualsInt(sqlrelay:getLongestByIndex(20), 11),
    assertEqualsInt(sqlrelay:getLongestByIndex(21), 9),
    case MajorVersion > 3 of
        true -> assertEqualsInt(sqlrelay:getLongestByIndex(22), 19);
        false -> assertEqualsInt(sqlrelay:getLongestByIndex(22), 14)
    end,
    assertEqualsInt(sqlrelay:getLongestByName("testtinyint"), 1),
    assertEqualsInt(sqlrelay:getLongestByName("testsmallint"), 1),
    assertEqualsInt(sqlrelay:getLongestByName("testmediumint"), 1),
    assertEqualsInt(sqlrelay:getLongestByName("testint"), 1),
    assertEqualsInt(sqlrelay:getLongestByName("testbigint"), 1),
    assertEqualsInt(sqlrelay:getLongestByName("testfloat"), 3),
    assertEqualsInt(sqlrelay:getLongestByName("testreal"), 3),
    assertEqualsInt(sqlrelay:getLongestByName("testdecimal"), 3),
    assertEqualsInt(sqlrelay:getLongestByName("testdate"), 10),
    assertEqualsInt(sqlrelay:getLongestByName("testtime"), 8),
    assertEqualsInt(sqlrelay:getLongestByName("testdatetime"), 19),
    assertEqualsInt(sqlrelay:getLongestByName("testyear"), 4),
    assertEqualsInt(sqlrelay:getLongestByName("testchar"), 5),
    assertEqualsInt(sqlrelay:getLongestByName("testvarchar"), 8),
    assertEqualsInt(sqlrelay:getLongestByName("testtext"), 5),
    assertEqualsInt(sqlrelay:getLongestByName("testtinytext"), 9),
    assertEqualsInt(sqlrelay:getLongestByName("testmediumtext"), 11),
    assertEqualsInt(sqlrelay:getLongestByName("testlongtext"), 9),
    assertEqualsInt(sqlrelay:getLongestByName("testblob"), 5),
    assertEqualsInt(sqlrelay:getLongestByName("testtinyblob"), 9),
    assertEqualsInt(sqlrelay:getLongestByName("testmediumblob"), 11),
    assertEqualsInt(sqlrelay:getLongestByName("testlongblob"), 9),
    case MajorVersion > 3 of
        true -> assertEqualsInt(sqlrelay:getLongestByName("testtimestamp"), 19);
        false -> assertEqualsInt(sqlrelay:getLongestByName("testtimestamp"), 14)
    end,
    io:format("~n"),

    %% ROW COUNT
    io:format("ROW COUNT: ~n"),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    io:format("~n"),

    %% TOTAL ROWS
    io:format("TOTAL ROWS: ~n"),
    %% older versions of mysql know this
    %% assertEqualsInt(sqlrelay:totalRows(), 0),
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
    assertEqualsString(sqlrelay:getFieldByIndex(0, 5), "1.5"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 6), "1.5"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 7), "1.5"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 8), "2001-01-01"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 9), "01:00:00"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 10), "2001-01-01 01:00:00"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 11), "2001"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 12), "char1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 13), "varchar1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 14), "text1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 15), "tinytext1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 16), "mediumtext1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 17), "longtext1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 18), "blob1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 19), "tinyblob1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 20), "mediumblob1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 21), "longblob1"),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 1), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 2), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 3), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 4), "8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 5), "8.5"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 6), "8.5"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 7), "8.5"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 8), "2008-01-01"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 9), "08:00:00"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 10), "2008-01-01 08:00:00"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 11), "2008"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 12), "char8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 13), "varchar8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 14), "text8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 15), "tinytext8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 16), "mediumtext8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 17), "longtext8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 18), "blob8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 19), "tinyblob8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 20), "mediumblob8"),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 21), "longblob8"),
    io:format("~n"),

    %% FIELD LENGTHS BY INDEX
    io:format("FIELD LENGTHS BY INDEX: ~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 1), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 2), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 3), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 4), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 5), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 6), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 7), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 8), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 9), 8),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 10), 19),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 11), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 12), 5),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 13), 8),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 14), 5),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 15), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 16), 11),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 17), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 18), 5),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 19), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 20), 11),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 21), 9),
    io:format("~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 0), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 1), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 2), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 3), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 4), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 5), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 6), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 7), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 8), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 9), 8),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 10), 19),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 11), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 12), 5),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 13), 8),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 14), 5),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 15), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 16), 11),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 17), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 18), 5),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 19), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 20), 11),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(7, 21), 9),
    io:format("~n"),

    %% FIELDS BY NAME
    io:format("FIELDS BY NAME: ~n"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testtinyint"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testsmallint"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testmediumint"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testint"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testbigint"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testfloat"), "1.5"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testreal"), "1.5"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testdecimal"), "1.5"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testdate"), "2001-01-01"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testtime"), "01:00:00"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testdatetime"),
                       "2001-01-01 01:00:00"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testyear"), "2001"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testchar"), "char1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testvarchar"), "varchar1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testtext"), "text1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testtinytext"), "tinytext1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testmediumtext"),
                       "mediumtext1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testlongtext"), "longtext1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testblob"), "blob1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testlongblob"), "longblob1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testtinyblob"), "tinyblob1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testmediumblob"),
                       "mediumblob1"),
    io:format("~n"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testtinyint"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testsmallint"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testmediumint"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testint"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testbigint"), "8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testfloat"), "8.5"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testreal"), "8.5"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testdecimal"), "8.5"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testdate"), "2008-01-01"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testtime"), "08:00:00"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testdatetime"),
                       "2008-01-01 08:00:00"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testyear"), "2008"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testchar"), "char8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testvarchar"), "varchar8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testtext"), "text8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testtinytext"), "tinytext8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testmediumtext"),
                       "mediumtext8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testlongtext"), "longtext8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testblob"), "blob8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testlongblob"), "longblob8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testtinyblob"), "tinyblob8"),
    assertEqualsString(sqlrelay:getFieldByName(7, "testmediumblob"),
                       "mediumblob8"),
    io:format("~n"),

    %% FIELD LENGTHS BY NAME
    io:format("FIELD LENGTHS BY NAME: ~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testtinyint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testsmallint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testmediumint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testbigint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testfloat"), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testreal"), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testdecimal"), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testdate"), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testtime"), 8),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testdatetime"), 19),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testyear"), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testchar"), 5),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testvarchar"), 8),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testtext"), 5),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testtinytext"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testmediumtext"), 11),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testlongtext"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testblob"), 5),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testtinyblob"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testmediumblob"), 11),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testlongblob"), 9),
    io:format("~n"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testtinyint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testsmallint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testmediumint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testbigint"), 1),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testfloat"), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testreal"), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testdecimal"), 3),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testdate"), 10),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testtime"), 8),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testdatetime"), 19),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testyear"), 4),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testchar"), 5),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testvarchar"), 8),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testtext"), 5),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testtinytext"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testmediumtext"), 11),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testlongtext"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testblob"), 5),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testtinyblob"), 9),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testmediumblob"), 11),
    assertEqualsInt(sqlrelay:getFieldLengthByName(7, "testlongblob"), 9),
    io:format("~n"),

    %% FIELDS BY ARRAY
    io:format("FIELDS BY ARRAY: ~n"),
    {ok, Row0} = sqlrelay:getRow(0),
    assertEqualsString(lists:nth(1, Row0), "1"),
    assertEqualsString(lists:nth(2, Row0), "1"),
    assertEqualsString(lists:nth(3, Row0), "1"),
    assertEqualsString(lists:nth(4, Row0), "1"),
    assertEqualsString(lists:nth(5, Row0), "1"),
    assertEqualsString(lists:nth(6, Row0), "1.5"),
    assertEqualsString(lists:nth(7, Row0), "1.5"),
    assertEqualsString(lists:nth(8, Row0), "1.5"),
    assertEqualsString(lists:nth(9, Row0), "2001-01-01"),
    assertEqualsString(lists:nth(10, Row0), "01:00:00"),
    assertEqualsString(lists:nth(11, Row0), "2001-01-01 01:00:00"),
    assertEqualsString(lists:nth(12, Row0), "2001"),
    assertEqualsString(lists:nth(13, Row0), "char1"),
    assertEqualsString(lists:nth(14, Row0), "varchar1"),
    assertEqualsString(lists:nth(15, Row0), "text1"),
    assertEqualsString(lists:nth(16, Row0), "tinytext1"),
    assertEqualsString(lists:nth(17, Row0), "mediumtext1"),
    assertEqualsString(lists:nth(18, Row0), "longtext1"),
    assertEqualsString(lists:nth(19, Row0), "blob1"),
    assertEqualsString(lists:nth(20, Row0), "tinyblob1"),
    assertEqualsString(lists:nth(21, Row0), "mediumblob1"),
    assertEqualsString(lists:nth(22, Row0), "longblob1"),
    io:format("~n"),

    %% FIELD LENGTHS BY ARRAY
    io:format("FIELD LENGTHS BY ARRAY: ~n"),
    {ok, Rowlens0} = sqlrelay:getRowLengths(0),
    assertEqualsInt(lists:nth(1, Rowlens0), 1),
    assertEqualsInt(lists:nth(2, Rowlens0), 1),
    assertEqualsInt(lists:nth(3, Rowlens0), 1),
    assertEqualsInt(lists:nth(4, Rowlens0), 1),
    assertEqualsInt(lists:nth(5, Rowlens0), 1),
    assertEqualsInt(lists:nth(6, Rowlens0), 3),
    assertEqualsInt(lists:nth(7, Rowlens0), 3),
    assertEqualsInt(lists:nth(8, Rowlens0), 3),
    assertEqualsInt(lists:nth(9, Rowlens0), 10),
    assertEqualsInt(lists:nth(10, Rowlens0), 8),
    assertEqualsInt(lists:nth(11, Rowlens0), 19),
    assertEqualsInt(lists:nth(12, Rowlens0), 4),
    assertEqualsInt(lists:nth(13, Rowlens0), 5),
    assertEqualsInt(lists:nth(14, Rowlens0), 8),
    assertEqualsInt(lists:nth(15, Rowlens0), 5),
    assertEqualsInt(lists:nth(16, Rowlens0), 9),
    assertEqualsInt(lists:nth(17, Rowlens0), 11),
    assertEqualsInt(lists:nth(18, Rowlens0), 9),
    assertEqualsInt(lists:nth(19, Rowlens0), 5),
    assertEqualsInt(lists:nth(20, Rowlens0), 9),
    assertEqualsInt(lists:nth(21, Rowlens0), 11),
    assertEqualsInt(lists:nth(22, Rowlens0), 9),
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
        "	testtinyint ")),
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
        "	testtinyint ")),
    assertEqualsString(sqlrelay:getColumnName(0), null),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 0),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(0), null),
    io:format("~n"),
    sqlrelay:getColumnInfo(),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testtinyint ")),
    assertEqualsString(sqlrelay:getColumnName(0), "testtinyint"),
    assertEqualsInt(sqlrelay:getColumnLengthByIndex(0), 1),
    assertEqualsString(sqlrelay:getColumnTypeByIndex(0), "TINYINT"),
    io:format("~n"),

    %% SUSPENDED SESSION
    io:format("SUSPENDED SESSION: ~n"),
    assertTrue(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testtinyint ")),
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
        "	testtinyint ")),
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
        "	testtinyint ")),
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
        "	testtinyint ")),
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
        "	testtinyint ")),
    {ok, Filename1} = sqlrelay:getCacheFileName(),
    assertEqualsString(Filename1, "cachefile1"),
    sqlrelay:cacheOff(),
    assertTrue(sqlrelay:openCachedResultSet(Filename1)),
    assertEqualsString(sqlrelay:getFieldByIndex(7, 0), "8"),
    io:format("~n"),

    %% COLUMN COUNT FOR CACHED RESULT SET
    io:format("COLUMN COUNT FOR CACHED RESULT SET: ~n"),
    assertEqualsInt(sqlrelay:colCount(), 23),
    io:format("~n"),

    %% COLUMN NAMES FOR CACHED RESULT SET
    io:format("COLUMN NAMES FOR CACHED RESULT SET: ~n"),
    assertEqualsString(sqlrelay:getColumnName(0), "testtinyint"),
    assertEqualsString(sqlrelay:getColumnName(1), "testsmallint"),
    assertEqualsString(sqlrelay:getColumnName(2), "testmediumint"),
    assertEqualsString(sqlrelay:getColumnName(3), "testint"),
    assertEqualsString(sqlrelay:getColumnName(4), "testbigint"),
    assertEqualsString(sqlrelay:getColumnName(5), "testfloat"),
    assertEqualsString(sqlrelay:getColumnName(6), "testreal"),
    assertEqualsString(sqlrelay:getColumnName(7), "testdecimal"),
    assertEqualsString(sqlrelay:getColumnName(8), "testdate"),
    assertEqualsString(sqlrelay:getColumnName(9), "testtime"),
    assertEqualsString(sqlrelay:getColumnName(10), "testdatetime"),
    assertEqualsString(sqlrelay:getColumnName(11), "testyear"),
    assertEqualsString(sqlrelay:getColumnName(12), "testchar"),
    assertEqualsString(sqlrelay:getColumnName(13), "testvarchar"),
    assertEqualsString(sqlrelay:getColumnName(14), "testtext"),
    assertEqualsString(sqlrelay:getColumnName(15), "testtinytext"),
    assertEqualsString(sqlrelay:getColumnName(16), "testmediumtext"),
    assertEqualsString(sqlrelay:getColumnName(17), "testlongtext"),
    assertEqualsString(sqlrelay:getColumnName(18), "testblob"),
    assertEqualsString(sqlrelay:getColumnName(19), "testtinyblob"),
    assertEqualsString(sqlrelay:getColumnName(20), "testmediumblob"),
    assertEqualsString(sqlrelay:getColumnName(21), "testlongblob"),
    {ok, Cols2} = sqlrelay:getColumnNames(),
    assertEqualsString(lists:nth(1, Cols2), "testtinyint"),
    assertEqualsString(lists:nth(2, Cols2), "testsmallint"),
    assertEqualsString(lists:nth(3, Cols2), "testmediumint"),
    assertEqualsString(lists:nth(4, Cols2), "testint"),
    assertEqualsString(lists:nth(5, Cols2), "testbigint"),
    assertEqualsString(lists:nth(6, Cols2), "testfloat"),
    assertEqualsString(lists:nth(7, Cols2), "testreal"),
    assertEqualsString(lists:nth(8, Cols2), "testdecimal"),
    assertEqualsString(lists:nth(9, Cols2), "testdate"),
    assertEqualsString(lists:nth(10, Cols2), "testtime"),
    assertEqualsString(lists:nth(11, Cols2), "testdatetime"),
    assertEqualsString(lists:nth(12, Cols2), "testyear"),
    assertEqualsString(lists:nth(13, Cols2), "testchar"),
    assertEqualsString(lists:nth(14, Cols2), "testvarchar"),
    assertEqualsString(lists:nth(15, Cols2), "testtext"),
    assertEqualsString(lists:nth(16, Cols2), "testtinytext"),
    assertEqualsString(lists:nth(17, Cols2), "testmediumtext"),
    assertEqualsString(lists:nth(18, Cols2), "testlongtext"),
    assertEqualsString(lists:nth(19, Cols2), "testblob"),
    assertEqualsString(lists:nth(20, Cols2), "testtinyblob"),
    assertEqualsString(lists:nth(21, Cols2), "testmediumblob"),
    assertEqualsString(lists:nth(22, Cols2), "testlongblob"),
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
        "	testtinyint ")),
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
        "	testtinyint ")),
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
    %% concurrent cursors cannot be held. We still run the outer query.
    io:format("NESTED SELECTS: ~n"),
    assertTrue(sqlrelay:sendQuery("select * from testtable")),
    %% Inner loop with secondcur is omitted.
    io:format("~n"),

    %% RESET TRANSACTION STATE
    %% (transaction behavior differs on mysql before 4)
    case MajorVersion > 3 of
        true ->
            io:format("RESET TRANSACTION STATE: ~n"),
            assertTrue(sqlrelay:commit()),
            assertEqualsString(sqlrelay:getTransactionModel(),
                               "explicit-deferred"),
            assertTrue(sqlrelay:getAutoCommit()),
            io:format("~n"),

            %% TRANSACTION BEHAVIOR -
            %% implicit/explicit/explicit-deferred/explicit-error/none
            %% SKIPPED: these blocks require a second concurrent connection
            %% (secondcon) and cursor (secondcur) to verify cross-connection
            %% isolation. The Erlang binding only supports one connection per
            %% process, so a second connection cannot be instantiated here.
            io:format("TRANSACTION BEHAVIOR - implicit/explicit/explicit-deferred/explicit-error/none: ~n"),
            io:format("(skipped - requires second concurrent connection)~n"),
            io:format("~n");
        false -> ok
    end,

    %% RESET TRANSACTION BEHAVIOR
    %% (mysql before 4 has limited transaction support)
    io:format("RESET TRANSACTION BEHAVIOR: ~n"),
    case MajorVersion > 3 of
        true ->
            {ok, DefaultModel} = sqlrelay:getDefaultTransactionModel(),
            assertTrue(sqlrelay:setTransactionModel(DefaultModel)),
            assertEqualsString(sqlrelay:getTransactionModel(),
                               "explicit-deferred"),
            assertTrue(sqlrelay:getAutoCommit());
        false -> ok
    end,
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
        "	testclob1 longtext, "
        "	testclob2 longtext, "
        "	testblob1 longblob, "
        "	testblob2 longblob)")),
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
        "	testtext longtext, "
        "	testblob longblob)"),
    sqlrelay:prepareQuery("insert into testtable values (?,?)"),
    LargeBufferLength = 8192,
    LargeBuf = largeBuffer(LargeBufferLength),
    sqlrelay:inputBindClob("1", LargeBuf, LargeBufferLength),
    sqlrelay:inputBindBlob("2", LargeBuf, LargeBufferLength),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("select * from testtable"),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testtext"),
                    LargeBufferLength),
    assertEqualsString(sqlrelay:getFieldByName(0, "testtext"), LargeBuf),
    assertEqualsInt(sqlrelay:getFieldLengthByName(0, "testblob"),
                    LargeBufferLength),
    assertEqualsStringLen(sqlrelay:getFieldByName(0, "testblob"),
                          LargeBuf, LargeBufferLength),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% OUTPUT BIND BY POSITION
    %% mysql doesn't support output binds

    %% OUTPUT BIND BY NAME
    %% mysql doesn't support bind by name

    %% OUTPUT BIND BY NAME WITH VALIDATION
    %% mysql doesn't support bind by name

    %% LOB OUTPUT BIND
    %% mysql doesn't support output binds

    %% LONG OUTPUT BIND
    %% mysql doesn't support output binds

    %% NEGATIVE INPUT BIND
    io:format("NEGATIVE INPUT BIND: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    sqlrelay:sendQuery("create table testtable (testval int)"),
    sqlrelay:prepareQuery("insert into testtable values (?)"),
    sqlrelay:inputBindLong("1", -1),
    assertTrue(sqlrelay:executeQuery()),
    sqlrelay:sendQuery("select testval from testtable"),
    assertEqualsString(sqlrelay:getFieldByName(0, "testval"), "-1"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% BIND VALIDATION
    %% mysql doesn't support bind by name

    %% REBINDING
    %% mysql before 5.0 has no stored procedures
    case MajorVersion > 3 of
        true ->
    io:format("REBINDING: ~n"),
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc("
        "	in in1 int) "
        "begin "
        "	select in1; "
        "end")),
    sqlrelay:prepareQuery("call testproc(?)"),
    sqlrelay:inputBindLong("1", 1),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    sqlrelay:inputBindLong("1", 2),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "2"),
    sqlrelay:inputBindLong("1", 3),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "3"),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc")),
    io:format("~n");
        false -> ok
    end,

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
    sqlrelay:prepareQuery("select ?"),
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
    %% mysql before 5.0 has no stored procedures
    case MajorVersion > 3 of
        true ->
    io:format("STORED PROCEDURE RETURNING NO VALUE: ~n"),
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc("
        "	in in1 int, "
        "	in in2 double, "
        "	in in3 varchar(20)) "
        "begin "
        "end")),
    sqlrelay:prepareQuery("call testproc(?,?,?)"),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:inputBindDouble("2", 1.5, 2, 1),
    sqlrelay:inputBindString("3", "hello"),
    assertTrue(sqlrelay:executeQuery()),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc")),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING SINGLE VALUE
    io:format("STORED PROCEDURE RETURNING SINGLE VALUE: ~n"),
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc("
        "	in in1 int, "
        "	in in2 double, "
        "	in in3 varchar(20)) "
        "begin "
        "	select in1; "
        "end")),
    sqlrelay:prepareQuery("call testproc(?,?,?)"),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:inputBindDouble("2", 1.5, 2, 1),
    sqlrelay:inputBindString("3", "hello"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc")),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING MULTIPLE VALUES
    io:format("STORED PROCEDURE RETURNING MULTIPLE VALUES: ~n"),
    sqlrelay:sendQuery("drop procedure testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc("
        "	in in1 int, "
        "	in in2 double, "
        "	in in3 varchar(20)) "
        "begin "
        "	select in1, in2, in3; "
        "end")),
    sqlrelay:prepareQuery("call testproc(?,?,?)"),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:inputBindDouble("2", 1.5, 2, 1),
    sqlrelay:inputBindString("3", "hello"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "1.5"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "hello"),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc")),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING RESULT SET
    io:format("STORED PROCEDURE RETURNING RESULT SET: ~n"),
    sqlrelay:sendQuery("drop procedure testselectproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testselectproc() "
        "begin "
        "	select 1 "
        "	union "
        "	select 2 "
        "	union "
        "	select 3 "
        "	union "
        "	select 4 "
        "	union "
        "	select 5 "
        "	union "
        "	select 6 "
        "	union "
        "	select 7 "
        "	union "
        "	select 8; "
        "end")),
    assertTrue(sqlrelay:sendQuery("call testselectproc()")),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    assertTrue(sqlrelay:sendQuery("drop procedure testselectproc")),
    io:format("~n"),

    %% TEMPORARY TABLES
    io:format("TEMPORARY TABLES: ~n"),
    sqlrelay:sendQuery("drop table temptable"),
    sqlrelay:sendQuery("create temporary table temptable (col1 int)"),
    assertTrue(sqlrelay:sendQuery("insert into temptable values (1)")),
    assertTrue(sqlrelay:sendQuery("select count(*) from temptable")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    sqlrelay:endSession(),
    io:format("~n"),
    assertFalse(sqlrelay:sendQuery("select count(*) from temptable")),
    io:format("~n");
        false -> ok
    end,

    %% STORED PROCEDURE RETURNING NO VALUE (v > 3)
    %% mysql before 5.0 has no stored procedures
    case MajorVersion > 3 of
        true ->
    io:format("STORED PROCEDURE RETURNING NO VALUE: ~n"),
    sqlrelay:sendQuery("drop procedure if exists testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc("
        "	in in1 int, "
        "	in in2 float, "
        "	in in3 char(20)) "
        "begin "
        "	select in1, in2, in3; "
        "end;")),
    sqlrelay:prepareQuery("call testproc(?,?,?)"),
    sqlrelay:inputBindLong("1", 1),
    sqlrelay:inputBindDouble("2", 1.5, 4, 2),
    sqlrelay:inputBindString("3", "hello"),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 1), "1.5"),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "hello"),
    sqlrelay:sendQuery("drop procedure testproc"),
    io:format("~n"),

    %% FUNCTIONS
    io:format("FUNCTIONS: ~n"),
    sqlrelay:sendQuery("drop function if exists testfunc"),
    assertTrue(sqlrelay:sendQuery(
        "create function testfunc(in1 int, in2 "
        "	int) returns int return in1+in2;")),
    sqlrelay:prepareQuery("select testfunc(?,?)"),
    sqlrelay:inputBindLong("1", 10),
    sqlrelay:inputBindLong("2", 20),
    assertTrue(sqlrelay:executeQuery()),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "30"),
    sqlrelay:sendQuery("drop function if exists testfunc"),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING MULTIPLE VALUES (v > 3)
    io:format("STORED PROCEDURE RETURNING MULTIPLE VALUES: ~n"),
    sqlrelay:sendQuery("drop procedure if exists testproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc("
        "	out out1 int, "
        "	out out2 float, "
        "	out out3 char(20)) "
        "begin "
        "	select 1, 2.5, 'hello' "
        "		into out1, out2, out3; "
        "end;")),
    assertTrue(sqlrelay:sendQuery("set @out1=0, @out2=0.0, @out3=''")),
    assertTrue(sqlrelay:sendQuery("call testproc(@out1,@out2,@out3)")),
    assertTrue(sqlrelay:sendQuery("select @out1, @out2, @out3")),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "1"),
    assertEqualsDouble(sqlrelay:getFieldAsDoubleByIndex(0, 1), 2.5),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 2), "hello"),
    sqlrelay:sendQuery("drop procedure testproc"),
    io:format("~n"),

    %% STORED PROCEDURE RETURNING RESULT SET (v > 3)
    io:format("STORED PROCEDURE RETURNING RESULT SET: ~n"),
    sqlrelay:sendQuery("drop procedure if exists testselectproc"),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testselectproc() "
        "begin "
        "	select 1 "
        "	union "
        "	select 2 "
        "	union "
        "	select 3 "
        "	union "
        "	select 4 "
        "	union "
        "	select 5 "
        "	union "
        "	select 6 "
        "	union "
        "	select 7 "
        "	union "
        "	select 8; "
        "end")),
    assertTrue(sqlrelay:sendQuery("call testselectproc()")),
    assertEqualsInt(sqlrelay:rowCount(), 8),
    sqlrelay:sendQuery("drop procedure testselectproc"),
    io:format("~n");
        false -> ok
    end,

    %% ENCODED BINARY DATA - all chars - \-escaped
    case MajorVersion > 3 of
        true ->
    io:format("ENCODED BINARY DATA - all chars - \\-escaped: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery("create table testtable (col1 longblob)")),
    Buffer = lists:seq(0, 255),
    EncodedBuf = lists:flatten([encodeByteBackslash(B) || B <- Buffer]),
    Query1 = "insert into testtable values (_binary'" ++ EncodedBuf ++ "')",
    assertTrue(sqlrelay:sendQueryWithLength(Query1, length(Query1))),
    assertTrue(sqlrelay:sendQuery("select col1 from testtable")),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 256),
    {ok, RawBytes1} = sqlrelay:getFieldByIndex(0, 0),
    case RawBytes1 =:= Buffer of
        true  -> pass();
        false -> fail(RawBytes1, Buffer)
    end,
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% ENCODED BINARY DATA - (null)"" - unescaped
    io:format("ENCODED BINARY DATA - (null)\"\" - unescaped: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery("create table testtable (col1 longblob)")),
    Query2 = "insert into testtable values (_binary'" ++ [0, $", $"] ++ "')",
    assertTrue(sqlrelay:sendQueryWithLength(Query2, length(Query2))),
    assertTrue(sqlrelay:sendQuery("select col1 from testtable")),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 3),
    {ok, RawBytes2} = sqlrelay:getFieldByIndex(0, 0),
    case RawBytes2 =:= [0, $", $"] of
        true  -> pass();
        false -> fail(RawBytes2, [0, $", $"])
    end,
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% ENCODED BINARY DATA - (null)"" - \-escaped
    io:format("ENCODED BINARY DATA - \\(null)\\\"\\\" - \\-escaped: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery("create table testtable (col1 longblob)")),
    Query3 = "insert into testtable values (_binary'"
             ++ [$\\, 0, $\\, $", $\\, $"] ++ "')",
    assertTrue(sqlrelay:sendQueryWithLength(Query3, length(Query3))),
    assertTrue(sqlrelay:sendQuery("select col1 from testtable")),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 3),
    {ok, RawBytes3} = sqlrelay:getFieldByIndex(0, 0),
    case RawBytes3 =:= [0, $", $"] of
        true  -> pass();
        false -> fail(RawBytes3, [0, $", $"])
    end,
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n");
        false -> ok
    end,

    %% QUOTES - '' - ''-escaped
    io:format("QUOTES - '' - ''-escaped: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery("create table testtable (col1 varchar(4))")),
    assertTrue(sqlrelay:sendQuery("insert into testtable values ('''''')")),
    assertTrue(sqlrelay:sendQuery("select col1 from testtable")),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 2),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "''"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% QUOTES - '' - '',\-escaped
    io:format("QUOTES - '' - '',\\-escaped: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery("create table testtable (col1 varchar(4))")),
    assertTrue(sqlrelay:sendQuery("insert into testtable values ('''\\'')")),
    assertTrue(sqlrelay:sendQuery("select col1 from testtable")),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 2),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "''"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% QUOTES - '' - \,''-escaped
    io:format("QUOTES - '' - \\,''-escaped: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery("create table testtable (col1 varchar(4))")),
    assertTrue(sqlrelay:sendQuery("insert into testtable values ('\\'''')")),
    assertTrue(sqlrelay:sendQuery("select col1 from testtable")),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 2),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "''"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% QUOTES - \\' - \-escaped
    io:format("QUOTES - \\\\' - \\-escaped: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery("create table testtable (col1 varchar(4))")),
    assertTrue(sqlrelay:sendQuery("insert into testtable values ('\\\\\\'')")),
    assertTrue(sqlrelay:sendQuery("select col1 from testtable")),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 2),
    {ok, QBytes} = sqlrelay:getFieldByIndex(0, 0),
    case QBytes =:= [$\\, $'] of
        true  -> pass();
        false -> fail(QBytes, [$\\, $'])
    end,
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% QUOTES - "" - unescaped
    io:format("QUOTES - \"\" - unescaped: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery("create table testtable (col1 varchar(4))")),
    assertTrue(sqlrelay:sendQuery("insert into testtable values ('\"\"')")),
    assertTrue(sqlrelay:sendQuery("select col1 from testtable")),
    assertEqualsInt(sqlrelay:getFieldLengthByIndex(0, 0), 2),
    assertEqualsString(sqlrelay:getFieldByIndex(0, 0), "\"\""),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% LAST INSERT ID
    io:format("LAST INSERT ID: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable "
        "	(col1 int primary key auto_increment, "
        "	col2 int)")),
    assertTrue(sqlrelay:sendQuery("insert into testtable values (null,1)")),
    assertEqualsInt(sqlrelay:getLastInsertId(), 1),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% DATABASE IS SCHEMA
    io:format("DATABASE IS SCHEMA: ~n"),
    assertFalse(sqlrelay:getDatabaseIsSchema()),
    io:format("~n"),

    %% CATALOG LIST
    %% mysql before 5.0 has no information_schema for these metadata queries
    case MajorVersion > 3 of
        true ->
    io:format("CATALOG LIST: ~n"),
    assertTrue(sqlrelay:getCatalogList("")),
    assertEqualsString(sqlrelay:getColumnName(0), "Database"),
    assertInResultSet("Database", Hostname),
    io:format("~n"),

    %% SCHEMA LIST
    io:format("SCHEMA LIST: ~n"),
    assertTrue(sqlrelay:getSchemaList("")),
    assertEqualsString(sqlrelay:getColumnName(0), "Database"),
    assertEqualsInt(sqlrelay:rowCount(), 0),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "255"),
    assertEqualsString(sqlrelay:getFieldByName(0, "local_type_name"), "CHAR"),
    assertTrue(sqlrelay:getTypeInfoList("varchar")),
    assertEqualsString(sqlrelay:getFieldByName(0, "type_name"), "VARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "12"),
    assertEqualsString(sqlrelay:getFieldByName(0, "precision"), "65535"),
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
        "	testtinyint tinyint, "
        "	testsmallint smallint, "
        "	testmediumint mediumint, "
        "	testint int, "
        "	testbigint bigint, "
        "	testfloat float, "
        "	testreal real, "
        "	testdecimal decimal(2,1), "
        "	testdate date, "
        "	testtime time, "
        "	testdatetime datetime, "
        "	testyear year, "
        "	testchar char(40), "
        "	testvarchar varchar(40), "
        "	testtext text, "
        "	testtinytext tinytext, "
        "	testmediumtext mediumtext, "
        "	testlongtext longtext, "
        "	testblob blob, "
        "	testtinyblob tinyblob, "
        "	testmediumblob mediumblob, "
        "	testlongblob longblob, "
        "	testtimestamp timestamp)")),
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
                       "testtinyint"),
    assertEqualsString(sqlrelay:getFieldByName(1, "column_name"),
                       "testsmallint"),
    assertEqualsString(sqlrelay:getFieldByName(2, "column_name"),
                       "testmediumint"),
    assertEqualsString(sqlrelay:getFieldByName(3, "column_name"), "testint"),
    assertEqualsString(sqlrelay:getFieldByName(4, "column_name"),
                       "testbigint"),
    assertEqualsString(sqlrelay:getFieldByName(5, "column_name"),
                       "testfloat"),
    assertEqualsString(sqlrelay:getFieldByName(6, "column_name"), "testreal"),
    assertEqualsString(sqlrelay:getFieldByName(7, "column_name"),
                       "testdecimal"),
    assertEqualsString(sqlrelay:getFieldByName(8, "column_name"), "testdate"),
    assertEqualsString(sqlrelay:getFieldByName(9, "column_name"), "testtime"),
    assertEqualsString(sqlrelay:getFieldByName(10, "column_name"),
                       "testdatetime"),
    assertEqualsString(sqlrelay:getFieldByName(11, "column_name"), "testyear"),
    assertEqualsString(sqlrelay:getFieldByName(12, "column_name"), "testchar"),
    assertEqualsString(sqlrelay:getFieldByName(13, "column_name"),
                       "testvarchar"),
    assertEqualsString(sqlrelay:getFieldByName(14, "column_name"), "testtext"),
    assertEqualsString(sqlrelay:getFieldByName(15, "column_name"),
                       "testtinytext"),
    assertEqualsString(sqlrelay:getFieldByName(16, "column_name"),
                       "testmediumtext"),
    assertEqualsString(sqlrelay:getFieldByName(17, "column_name"),
                       "testlongtext"),
    assertEqualsString(sqlrelay:getFieldByName(18, "column_name"), "testblob"),
    assertEqualsString(sqlrelay:getFieldByName(19, "column_name"),
                       "testtinyblob"),
    assertEqualsString(sqlrelay:getFieldByName(20, "column_name"),
                       "testmediumblob"),
    assertEqualsString(sqlrelay:getFieldByName(21, "column_name"),
                       "testlongblob"),
    assertEqualsString(sqlrelay:getFieldByName(22, "column_name"),
                       "testtimestamp"),
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "TINYINT"),
    assertEqualsString(sqlrelay:getFieldByName(1, "data_type"), "SMALLINT"),
    assertEqualsString(sqlrelay:getFieldByName(2, "data_type"), "MEDIUMINT"),
    assertEqualsString(sqlrelay:getFieldByName(3, "data_type"), "INT"),
    assertEqualsString(sqlrelay:getFieldByName(4, "data_type"), "BIGINT"),
    assertEqualsString(sqlrelay:getFieldByName(5, "data_type"), "FLOAT"),
    assertEqualsString(sqlrelay:getFieldByName(6, "data_type"), "DOUBLE"),
    assertEqualsString(sqlrelay:getFieldByName(7, "data_type"), "DECIMAL"),
    assertEqualsString(sqlrelay:getFieldByName(8, "data_type"), "DATE"),
    assertEqualsString(sqlrelay:getFieldByName(9, "data_type"), "TIME"),
    assertEqualsString(sqlrelay:getFieldByName(10, "data_type"), "DATETIME"),
    assertEqualsString(sqlrelay:getFieldByName(11, "data_type"), "YEAR"),
    assertEqualsString(sqlrelay:getFieldByName(12, "data_type"), "CHAR"),
    assertEqualsString(sqlrelay:getFieldByName(13, "data_type"), "VARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(14, "data_type"), "TEXT"),
    assertEqualsString(sqlrelay:getFieldByName(15, "data_type"), "TINYTEXT"),
    assertEqualsString(sqlrelay:getFieldByName(16, "data_type"), "MEDIUMTEXT"),
    assertEqualsString(sqlrelay:getFieldByName(17, "data_type"), "LONGTEXT"),
    assertEqualsString(sqlrelay:getFieldByName(18, "data_type"), "BLOB"),
    assertEqualsString(sqlrelay:getFieldByName(19, "data_type"), "TINYBLOB"),
    assertEqualsString(sqlrelay:getFieldByName(20, "data_type"), "MEDIUMBLOB"),
    assertEqualsString(sqlrelay:getFieldByName(21, "data_type"), "LONGBLOB"),
    assertEqualsString(sqlrelay:getFieldByName(22, "data_type"), "TIMESTAMP"),
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
    io:format("~n"),

    %% COLUMN LIST - auto_increment, primary key
    io:format("COLUMN LIST - auto_increment, primary key: ~n"),
    sqlrelay:sendQuery("drop table testtable"),
    assertTrue(sqlrelay:sendQuery(
        "create table testtable ("
        "	col1 int auto_increment primary key, "
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
    assertTrue(sqlrelay:sendQuery("drop table testtable")),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "key_name"), "PRIMARY"),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "non_unique"), "false"),
    assertEqualsString(sqlrelay:getFieldByName(0, "seq_in_index"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "column_name"), "col1"),
    assertEqualsString(sqlrelay:getFieldByName(0, "collation"), "A"),
    assertEqualsString(sqlrelay:getFieldByName(0, "index_type"), "3"),
    assertEqualsString(sqlrelay:getFieldByName(0, "key_name"), "PRIMARY"),
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
        "	in in1 int, "
        "	in in2 char(20), "
        "	in in3 varchar(20), "
        "	in in4 date) "
        "begin end")),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc2("
        "	in in1 int, "
        "	in in2 char(20), "
        "	in in3 varchar(20), "
        "	in in4 date) "
        "begin end")),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc3("
        "	in in1 int, "
        "	in in2 char(20), "
        "	in in3 varchar(20), "
        "	in in4 date) "
        "begin end")),
    assertTrue(sqlrelay:sendQuery(
        "create procedure testproc4("
        "	in in1 int, "
        "	in in2 char(20), "
        "	in in3 varchar(20), "
        "	in in4 date) "
        "begin end")),
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
    assertEqualsString(sqlrelay:getFieldByName(0, "data_type"), "INT"),
    assertEqualsString(sqlrelay:getFieldByName(0, "ordinal_position"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(1, "parameter_name"), "in2"),
    assertEqualsString(sqlrelay:getFieldByName(1, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(1, "data_type"), "CHAR"),
    assertEqualsString(sqlrelay:getFieldByName(1, "ordinal_position"), "2"),
    assertEqualsString(sqlrelay:getFieldByName(2, "parameter_name"), "in3"),
    assertEqualsString(sqlrelay:getFieldByName(2, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(2, "data_type"), "VARCHAR"),
    assertEqualsString(sqlrelay:getFieldByName(2, "ordinal_position"), "3"),
    assertEqualsString(sqlrelay:getFieldByName(3, "parameter_name"), "in4"),
    assertEqualsString(sqlrelay:getFieldByName(3, "parameter_mode"), "1"),
    assertEqualsString(sqlrelay:getFieldByName(3, "data_type"), "DATE"),
    assertEqualsString(sqlrelay:getFieldByName(3, "ordinal_position"), "4"),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc1")),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc2")),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc3")),
    assertTrue(sqlrelay:sendQuery("drop procedure testproc4")),
    io:format("~n");
        false -> ok
    end,

    %% INVALID QUERIES
    io:format("INVALID QUERIES: ~n"),
    assertFalse(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testtinyint ")),
    assertFalse(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testtinyint ")),
    assertFalse(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testtinyint ")),
    assertFalse(sqlrelay:sendQuery(
        "select "
        "	* "
        "from "
        "	testtable "
        "order by "
        "	testtinyint ")),
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

%% Encode a single byte for an _binary'...'  mysql literal, escaping
%% ', ", and \ with a leading backslash. All other bytes pass through.
encodeByteBackslash($') -> [$\\, $'];
encodeByteBackslash($\\) -> [$\\, $\\];
encodeByteBackslash(B) -> [B].
