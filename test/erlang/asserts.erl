%% Copyright (c) David Muse
%% See the file COPYING for more information.

%%
%% Assertion helpers (mimic asserts.cpp)
%%
%% Status is tracked in the process dictionary because Erlang variables
%% are immutable.
%%

-module(asserts).
-export([pass/0, fail/2]).
-export([getStatus/0, setFailed/0, reportTestStatus/0]).
-export([assertEqualsString/2, assertEqualsStringLen/3]).
-export([assertStartsWith/2]).
-export([assertEqualsInt/2, assertEqualsDouble/2]).
-export([assertTrue/1, assertFalse/1]).
-export([assertInResultSet/2]).
-export([waitForPort/1, largeBuffer/1, shortHostname/0]).

success()            -> "\e[32msuccess\e[0m".
failure()            -> "\e[31mfailure\e[0m".
alltestssucceeded()  -> "\n\e[34mAll tests succeeded\e[0m\n".
sometestsfailed()    -> "\n\e[38;5;208mSome tests failed\e[0m\n".

getStatus() ->
    case get(status) of
        undefined -> 0;
        V -> V
    end.

setFailed() ->
    put(status, 1).

pass() ->
    io:format("~s ", [success()]).

fail(Actual, Expected) ->
    io:format("~s~n", [failure()]),
    io:format("~p!=~p~n", [Actual, Expected]),
    printErrors(),
    setFailed().

printErrors() ->
    case sqlrelay:errorMessage() of
        {ok, Msg} when is_list(Msg), Msg =/= [] ->
            io:format("~s~n", [Msg]);
        _ ->
            case sqlrelay:connectionErrorMessage() of
                {ok, Msg2} when is_list(Msg2), Msg2 =/= [] ->
                    io:format("~s~n", [Msg2]);
                _ ->
                    ok
            end
    end.

%% String comparison.
%%  - expected = null atom matches actual = undefined, [] (empty), or
%%    the atom null.
%%  - otherwise require exact list equality.
assertEqualsString({ok, Actual}, Expected) ->
    assertEqualsString(Actual, Expected);
assertEqualsString(Actual, null) ->
    case isNullish(Actual) of
        true  -> pass();
        false -> fail(Actual, null)
    end;
assertEqualsString(Actual, Expected) when is_list(Actual), is_list(Expected) ->
    case Actual =:= Expected of
        true  -> pass();
        false -> fail(Actual, Expected)
    end;
assertEqualsString(Actual, Expected) ->
    fail(Actual, Expected).

isNullish(undefined) -> true;
isNullish(null)      -> true;
isNullish([])        -> true;
isNullish(_)         -> false.

%% String comparison with a length limit (substring compare).
assertEqualsStringLen({ok, Actual}, Expected, Length) ->
    assertEqualsStringLen(Actual, Expected, Length);
assertEqualsStringLen(Actual, null, _Length) ->
    case isNullish(Actual) of
        true  -> pass();
        false -> fail(Actual, null)
    end;
assertEqualsStringLen(Actual, Expected, Length)
        when is_list(Actual), is_list(Expected) ->
    A = lists:sublist(Actual, Length),
    E = lists:sublist(Expected, Length),
    case A =:= E of
        true  -> pass();
        false -> fail(Actual, Expected)
    end;
assertEqualsStringLen(Actual, Expected, _Length) ->
    fail(Actual, Expected).

%% String prefix check - actual must start with prefix.
assertStartsWith({ok, Actual}, Prefix) ->
    assertStartsWith(Actual, Prefix);
assertStartsWith(Actual, Prefix) when is_list(Actual), is_list(Prefix) ->
    case lists:prefix(Prefix, Actual) of
        true  -> pass();
        false -> fail(Actual, Prefix)
    end;
assertStartsWith(Actual, Prefix) ->
    fail(Actual, Prefix).

assertEqualsInt({ok, Actual}, Expected) ->
    assertEqualsInt(Actual, Expected);
assertEqualsInt(Actual, Expected) when is_integer(Actual), is_integer(Expected) ->
    case Actual =:= Expected of
        true  -> pass();
        false -> fail(Actual, Expected)
    end;
assertEqualsInt(Actual, Expected) ->
    fail(Actual, Expected).

assertEqualsDouble({ok, Actual}, Expected) ->
    assertEqualsDouble(Actual, Expected);
assertEqualsDouble(Actual, Expected) when is_number(Actual), is_number(Expected) ->
    case (Actual + 0.0) =:= (Expected + 0.0) of
        true  -> pass();
        false -> fail(Actual, Expected)
    end;
assertEqualsDouble(Actual, Expected) ->
    fail(Actual, Expected).

assertTrue({ok, 1})           -> pass();
assertTrue({ok, true})        -> pass();
assertTrue({ok, Other})       -> fail(Other, 1);
assertTrue(true)              -> pass();
assertTrue(1)                 -> pass();
assertTrue(Other)             -> fail(Other, true).

assertFalse({ok, 0})          -> pass();
assertFalse({ok, false})      -> pass();
assertFalse({ok, Other})      -> fail(Other, 0);
assertFalse(false)            -> pass();
assertFalse(0)                -> pass();
assertFalse(Other)            -> fail(Other, false).

assertInResultSet(Column, Value) ->
    {ok, RowCount} = sqlrelay:rowCount(),
    assertInResultSet(Column, Value, 0, RowCount).

assertInResultSet(Column, Value, I, RowCount) when I >= RowCount ->
    io:format("~s~n", [failure()]),
    io:format("~p not found in column ~p~n", [Value, Column]),
    printErrors(),
    setFailed();
assertInResultSet(Column, Value, I, RowCount) ->
    case sqlrelay:getFieldByName(I, Column) of
        {ok, Value} -> pass();
        _           -> assertInResultSet(Column, Value, I + 1, RowCount)
    end.

reportTestStatus() ->
    case getStatus() of
        0 -> io:format("~s", [alltestssucceeded()]);
        _ -> io:format("~s", [sometestsfailed()])
    end.

%% sqlrelay:start/0 is an async spawn_link — the registered name
%% 'sqlrelay' isn't available immediately.  Poll until it is.
waitForPort(0) ->
    exit(port_not_registered);
waitForPort(N) ->
    case whereis(sqlrelay) of
        undefined -> timer:sleep(10), waitForPort(N - 1);
        _         -> ok
    end.

%% Build a string of N 'C' characters.
largeBuffer(Length) ->
    lists:duplicate(Length, $C).

%% Return inet:gethostname() stripped of any trailing .domain suffix.
shortHostname() ->
    {ok, H} = inet:gethostname(),
    case string:chr(H, $.) of
        0 -> H;
        N -> string:substr(H, 1, N - 1)
    end.
