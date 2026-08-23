#! /usr/bin/env tclsh
# DO NOT EDIT - this is a generated file; edit the corresponding .in template and rerun make in this directory

# Copyright (c) David Muse
# See the file COPYING for more information.

load /usr/lib64/sqlrelay/sqlrelay.so sqlrelay
source ./asserts.tcl


# hostname
set hostname [info hostname]
set dot [string first "." $hostname]
if {$dot > 0} {
	set hostname [string range $hostname 0 [expr {$dot - 1}]]
}


# instantiation
set con [sqlrcon -server "sqlrelay" -port 9007 -socket "/tmp/odbc-mssql.socket" -user "testuser" -password "testpassword" -retrytime 0 -tries 1]
set cur [$con sqlrcur]
setConnection $con
setCursor $cur


# identify
puts "IDENTIFY: "
assertEqual [$con identify] "odbc"
puts ""


# ping
puts "PING: "
assertTrue [$con ping]
puts ""


# transaction state
puts "TRANSACTION STATE: "
assertEqual [$con getDefaultTransactionModel] "explicit"
assertEqual [$con getTransactionModel] "explicit"
assertFalse [$con getInTransaction]
assertTrue [$con getAutoCommit]
puts ""


# bind format
puts "BIND FORMAT: "
assertEqual [$con bindFormat] "?"
puts ""


# nextval format
puts "NEXTVAL FORMAT: "
assertEqual [$con nextvalFormat] ""
puts ""


# isolation levels
puts "ISOLATION LEVELS: "
# the odbc module has no getIsolationLevelQuery() override, so
# sqlrserverconnection::getIsolationLevel() short-circuits and reports
# "unknown" whatever the level actually is
set isolationlevels [list "READ COMMITTED" "READ UNCOMMITTED" \
		"REPEATABLE READ" "SERIALIZABLE"]
foreach il $isolationlevels {
	assertTrue [$con setIsolationLevel $il]
	assertEqual [$con getIsolationLevel] "unknown"
	puts ""
}
# reset to the default isolation level
assertTrue [$con setIsolationLevel [lindex $isolationlevels 0]]
puts ""


# create testtable
puts "CREATE TESTTABLE: "
catch {$cur sendQuery "drop table testtable"}
assertTrue [$cur sendQuery "create table testtable (testint int, testsmallint smallint, testtinyint tinyint, testreal real, testfloat float, testdecimal decimal(4,1), testnumeric numeric(4,1), testmoney money, testsmallmoney smallmoney, testdatetime datetime, testsmalldatetime smalldatetime, testchar char(40), testvarchar varchar(40), testbit bit, testdate date, testtime time, testdatetime2 datetime2)"]
puts ""


# insert
puts "INSERT: "
assertTrue [$con begin]
assertTrue [$cur sendQuery "insert into testtable values (1, 1, 1, 1.5, 1.5, 1.5, 1.5, 1.00, 1.00, '01-Jan-2001 01:00:00', '01-Jan-2001 01:00:00', 'testchar1', 'testvarchar1', 1, '01-Jan-2001', '13:01:01', '01-Jan-2001 13:01:01')"]
puts ""


# affected rows
puts "AFFECTED ROWS: "
assertEqual [$cur affectedRows] 1
puts ""


# input bind by position
puts "INPUT BIND BY POSITION: "
$cur prepareQuery "insert into testtable values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"
assertEqual [$cur countBindVariables] 17
$cur inputBind "1" 2
$cur inputBind "2" 2
$cur inputBind "3" 2
$cur inputBind "4" 2.5 2 1
$cur inputBind "5" 2.5 2 1
$cur inputBind "6" 2.5 2 1
$cur inputBind "7" 2.5 2 1
$cur inputBind "8" 2.00 3 2
$cur inputBind "9" 2.00 3 2
$cur inputBind "10" "01-Jan-2002 02:00:00"
$cur inputBind "11" "01-Jan-2002 02:00:00"
$cur inputBind "12" "testchar2"
$cur inputBind "13" "testvarchar2"
$cur inputBind "14" 1
$cur inputBind "15" "01-Jan-2001"
$cur inputBind "16" "13:01:01"
$cur inputBind "17" "01-Jan-2001 13:01:01"
assertTrue [$cur executeQuery]
$cur clearBinds
$cur inputBind "1" 3
$cur inputBind "2" 3
$cur inputBind "3" 3
$cur inputBind "4" 3.5 2 1
$cur inputBind "5" 3.5 2 1
$cur inputBind "6" 3.5 2 1
$cur inputBind "7" 3.5 2 1
$cur inputBind "8" 3.00 3 2
$cur inputBind "9" 3.00 3 2
$cur inputBind "10" "01-Jan-2003 03:00:00"
$cur inputBind "11" "01-Jan-2003 03:00:00"
$cur inputBind "12" "testchar3"
$cur inputBind "13" "testvarchar3"
$cur inputBind "14" 1
$cur inputBind "15" "01-Jan-2001"
$cur inputBind "16" "13:01:01"
$cur inputBind "17" "01-Jan-2001 13:01:01"
assertTrue [$cur executeQuery]
puts ""


# array of input binds by position
# arrays of binds do work here - odbc binds them all as strings and mssql
# converts - but the fixture is already 8 rows without them, so there is
# nothing left for this section to insert


# input bind by position with validation
puts "INPUT BIND BY POSITION WITH VALIDATION: "
$cur clearBinds
$cur inputBind "1" 4
$cur inputBind "2" 4
$cur inputBind "3" 4
$cur inputBind "4" 4.5 2 1
$cur inputBind "5" 4.5 2 1
$cur inputBind "6" 4.5 2 1
$cur inputBind "7" 4.5 2 1
$cur inputBind "8" 4.00 3 2
$cur inputBind "9" 4.00 3 2
$cur inputBind "10" "01-Jan-2004 04:00:00"
$cur inputBind "11" "01-Jan-2004 04:00:00"
$cur inputBind "12" "testchar4"
$cur inputBind "13" "testvarchar4"
$cur inputBind "14" 1
$cur inputBind "15" "01-Jan-2001"
$cur inputBind "16" "13:01:01"
$cur inputBind "17" "01-Jan-2001 13:01:01"
$cur validateBinds
assertTrue [$cur executeQuery]
puts ""


# input bind by name
# odbc binds positionally, with "?", so there is nothing to bind by name.
# that is a contract rather than a defect: @varN gives "Must declare the
# scalar variable" and :varN gives "Incorrect syntax near ':'".
# translatebindvariables=yes would rewrite the binds, but it also mangles
# every create procedure below, so it isn't usable here


# array of input binds by name
# odbc binds positionally, so there is nothing to bind by name


# input bind by name with validation
# odbc binds positionally, so there is nothing to bind by name


# remaining fixture rows
# the freetds test puts rows 5 through 8 in by name.  they go in by
# position here instead, so the fixture is still 8 rows and every count
# and row index below carries over unchanged
puts "REMAINING FIXTURE ROWS: "
$cur clearBinds
$cur inputBind "1" 5
$cur inputBind "2" 5
$cur inputBind "3" 5
$cur inputBind "4" 5.5 2 1
$cur inputBind "5" 5.5 2 1
$cur inputBind "6" 5.5 2 1
$cur inputBind "7" 5.5 2 1
$cur inputBind "8" 5.00 3 2
$cur inputBind "9" 5.00 3 2
$cur inputBind "10" "01-Jan-2005 05:00:00"
$cur inputBind "11" "01-Jan-2005 05:00:00"
$cur inputBind "12" "testchar5"
$cur inputBind "13" "testvarchar5"
$cur inputBind "14" 1
$cur inputBind "15" "01-Jan-2001"
$cur inputBind "16" "13:01:01"
$cur inputBind "17" "01-Jan-2001 13:01:01"
assertTrue [$cur executeQuery]
$cur clearBinds
$cur inputBind "1" 6
$cur inputBind "2" 6
$cur inputBind "3" 6
$cur inputBind "4" 6.5 2 1
$cur inputBind "5" 6.5 2 1
$cur inputBind "6" 6.5 2 1
$cur inputBind "7" 6.5 2 1
$cur inputBind "8" 6.00 3 2
$cur inputBind "9" 6.00 3 2
$cur inputBind "10" "01-Jan-2006 06:00:00"
$cur inputBind "11" "01-Jan-2006 06:00:00"
$cur inputBind "12" "testchar6"
$cur inputBind "13" "testvarchar6"
$cur inputBind "14" 1
$cur inputBind "15" "01-Jan-2001"
$cur inputBind "16" "13:01:01"
$cur inputBind "17" "01-Jan-2001 13:01:01"
assertTrue [$cur executeQuery]
$cur clearBinds
$cur inputBind "1" 7
$cur inputBind "2" 7
$cur inputBind "3" 7
$cur inputBind "4" 7.5 2 1
$cur inputBind "5" 7.5 2 1
$cur inputBind "6" 7.5 2 1
$cur inputBind "7" 7.5 2 1
$cur inputBind "8" 7.00 3 2
$cur inputBind "9" 7.00 3 2
$cur inputBind "10" "01-Jan-2007 07:00:00"
$cur inputBind "11" "01-Jan-2007 07:00:00"
$cur inputBind "12" "testchar7"
$cur inputBind "13" "testvarchar7"
$cur inputBind "14" 1
$cur inputBind "15" "01-Jan-2001"
$cur inputBind "16" "13:01:01"
$cur inputBind "17" "01-Jan-2001 13:01:01"
assertTrue [$cur executeQuery]
puts ""
$cur clearBinds
$cur inputBind "1" 8
$cur inputBind "2" 8
$cur inputBind "3" 8
$cur inputBind "4" 8.5 2 1
$cur inputBind "5" 8.5 2 1
$cur inputBind "6" 8.5 2 1
$cur inputBind "7" 8.5 2 1
$cur inputBind "8" 8.00 3 2
$cur inputBind "9" 8.00 3 2
$cur inputBind "10" "01-Jan-2008 08:00:00"
$cur inputBind "11" "01-Jan-2008 08:00:00"
$cur inputBind "12" "testchar8"
$cur inputBind "13" "testvarchar8"
$cur inputBind "14" 1
$cur inputBind "15" "01-Jan-2001"
$cur inputBind "16" "13:01:01"
$cur inputBind "17" "01-Jan-2001 13:01:01"
assertTrue [$cur executeQuery]
puts ""


# select
puts "SELECT: "
assertTrue [$cur sendQuery "select * from testtable order by testint"]
puts ""


# column count
puts "COLUMN COUNT: "
assertEqual [$cur colCount] 17
puts ""


# column names
puts "COLUMN NAMES: "
assertEqual [$cur getColumnName 0] "testint"
assertEqual [$cur getColumnName 1] "testsmallint"
assertEqual [$cur getColumnName 2] "testtinyint"
assertEqual [$cur getColumnName 3] "testreal"
assertEqual [$cur getColumnName 4] "testfloat"
assertEqual [$cur getColumnName 5] "testdecimal"
assertEqual [$cur getColumnName 6] "testnumeric"
assertEqual [$cur getColumnName 7] "testmoney"
assertEqual [$cur getColumnName 8] "testsmallmoney"
assertEqual [$cur getColumnName 9] "testdatetime"
assertEqual [$cur getColumnName 10] "testsmalldatetime"
assertEqual [$cur getColumnName 11] "testchar"
assertEqual [$cur getColumnName 12] "testvarchar"
assertEqual [$cur getColumnName 13] "testbit"
assertEqual [$cur getColumnName 14] "testdate"
assertEqual [$cur getColumnName 15] "testtime"
assertEqual [$cur getColumnName 16] "testdatetime2"
set cols [$cur getColumnNames]
assertEqual [lindex $cols 0] "testint"
assertEqual [lindex $cols 1] "testsmallint"
assertEqual [lindex $cols 2] "testtinyint"
assertEqual [lindex $cols 3] "testreal"
assertEqual [lindex $cols 4] "testfloat"
assertEqual [lindex $cols 5] "testdecimal"
assertEqual [lindex $cols 6] "testnumeric"
assertEqual [lindex $cols 7] "testmoney"
assertEqual [lindex $cols 8] "testsmallmoney"
assertEqual [lindex $cols 9] "testdatetime"
assertEqual [lindex $cols 10] "testsmalldatetime"
assertEqual [lindex $cols 11] "testchar"
assertEqual [lindex $cols 12] "testvarchar"
assertEqual [lindex $cols 13] "testbit"
assertEqual [lindex $cols 14] "testdate"
assertEqual [lindex $cols 15] "testtime"
assertEqual [lindex $cols 16] "testdatetime2"
puts ""


# column types
puts "COLUMN TYPES: "
assertEqual [$cur getColumnTypeByIndex 0] "INTEGER"
assertEqual [$cur getColumnTypeByName "testint"] "INTEGER"
assertEqual [$cur getColumnTypeByIndex 1] "SMALLINT"
assertEqual [$cur getColumnTypeByName "testsmallint"] "SMALLINT"
assertEqual [$cur getColumnTypeByIndex 2] "TINYINT"
assertEqual [$cur getColumnTypeByName "testtinyint"] "TINYINT"
assertEqual [$cur getColumnTypeByIndex 3] "REAL"
assertEqual [$cur getColumnTypeByName "testreal"] "REAL"
assertEqual [$cur getColumnTypeByIndex 4] "FLOAT"
assertEqual [$cur getColumnTypeByName "testfloat"] "FLOAT"
assertEqual [$cur getColumnTypeByIndex 5] "DECIMAL"
assertEqual [$cur getColumnTypeByName "testdecimal"] "DECIMAL"
assertEqual [$cur getColumnTypeByIndex 6] "NUMERIC"
assertEqual [$cur getColumnTypeByName "testnumeric"] "NUMERIC"
assertEqual [$cur getColumnTypeByIndex 7] "MONEY"
assertEqual [$cur getColumnTypeByName "testmoney"] "MONEY"
assertEqual [$cur getColumnTypeByIndex 8] "SMALLMONEY"
assertEqual [$cur getColumnTypeByName "testsmallmoney"] "SMALLMONEY"
assertEqual [$cur getColumnTypeByIndex 9] "DATETIME"
assertEqual [$cur getColumnTypeByName "testdatetime"] "DATETIME"
assertEqual [$cur getColumnTypeByIndex 10] "SMALLDATETIME"
assertEqual [$cur getColumnTypeByName "testsmalldatetime"] "SMALLDATETIME"
assertEqual [$cur getColumnTypeByIndex 11] "CHAR"
assertEqual [$cur getColumnTypeByName "testchar"] "CHAR"
assertEqual [$cur getColumnTypeByIndex 12] "VARCHAR"
assertEqual [$cur getColumnTypeByName "testvarchar"] "VARCHAR"
assertEqual [$cur getColumnTypeByIndex 13] "BIT"
assertEqual [$cur getColumnTypeByName "testbit"] "BIT"
assertEqual [$cur getColumnTypeByIndex 14] "DATE"
assertEqual [$cur getColumnTypeByName "testdate"] "DATE"
assertEqual [$cur getColumnTypeByIndex 15] "TIME"
assertEqual [$cur getColumnTypeByName "testtime"] "TIME"
assertEqual [$cur getColumnTypeByIndex 16] "TIMESTAMP"
assertEqual [$cur getColumnTypeByName "testdatetime2"] "TIMESTAMP"
puts ""


# column length
puts "COLUMN LENGTH: "
# odbc reports the ODBC column size - the number of characters it takes
# to display the value - where freetds reports the storage size in
# bytes, so every one of these differs from the freetds test
assertEqual [$cur getColumnLengthByIndex 0] 10
assertEqual [$cur getColumnLengthByName "testint"] 10
assertEqual [$cur getColumnLengthByIndex 1] 5
assertEqual [$cur getColumnLengthByName "testsmallint"] 5
assertEqual [$cur getColumnLengthByIndex 2] 3
assertEqual [$cur getColumnLengthByName "testtinyint"] 3
assertEqual [$cur getColumnLengthByIndex 3] 24
assertEqual [$cur getColumnLengthByName "testreal"] 24
assertEqual [$cur getColumnLengthByIndex 4] 53
assertEqual [$cur getColumnLengthByName "testfloat"] 53
assertEqual [$cur getColumnLengthByIndex 5] 4
assertEqual [$cur getColumnLengthByName "testdecimal"] 4
assertEqual [$cur getColumnLengthByIndex 6] 4
assertEqual [$cur getColumnLengthByName "testnumeric"] 4
assertEqual [$cur getColumnLengthByIndex 7] 19
assertEqual [$cur getColumnLengthByName "testmoney"] 19
assertEqual [$cur getColumnLengthByIndex 8] 10
assertEqual [$cur getColumnLengthByName "testsmallmoney"] 10
assertEqual [$cur getColumnLengthByIndex 9] 23
assertEqual [$cur getColumnLengthByName "testdatetime"] 23
assertEqual [$cur getColumnLengthByIndex 10] 16
assertEqual [$cur getColumnLengthByName "testsmalldatetime"] 16
# char(40)/varchar(40) report the declared length 40 (not multiplied)
assertEqual [$cur getColumnLengthByIndex 11] 40
assertEqual [$cur getColumnLengthByName "testchar"] 40
assertEqual [$cur getColumnLengthByIndex 12] 40
assertEqual [$cur getColumnLengthByName "testvarchar"] 40
assertEqual [$cur getColumnLengthByIndex 13] 1
assertEqual [$cur getColumnLengthByName "testbit"] 1
assertEqual [$cur getColumnLengthByIndex 14] 10
assertEqual [$cur getColumnLengthByName "testdate"] 10
assertEqual [$cur getColumnLengthByIndex 15] 16
assertEqual [$cur getColumnLengthByName "testtime"] 16
assertEqual [$cur getColumnLengthByIndex 16] 27
assertEqual [$cur getColumnLengthByName "testdatetime2"] 27
puts ""


# longest column
puts "LONGEST COLUMN: "
assertEqual [$cur getLongestByIndex 0] 1
assertEqual [$cur getLongestByName "testint"] 1
assertEqual [$cur getLongestByIndex 1] 1
assertEqual [$cur getLongestByName "testsmallint"] 1
assertEqual [$cur getLongestByIndex 2] 1
assertEqual [$cur getLongestByName "testtinyint"] 1
assertEqual [$cur getLongestByIndex 3] 3
assertEqual [$cur getLongestByName "testreal"] 3
assertEqual [$cur getLongestByIndex 4] 3
assertEqual [$cur getLongestByName "testfloat"] 3
assertEqual [$cur getLongestByIndex 5] 3
assertEqual [$cur getLongestByName "testdecimal"] 3
assertEqual [$cur getLongestByIndex 6] 3
assertEqual [$cur getLongestByName "testnumeric"] 3
assertMoneyEqualLen [$cur getLongestByIndex 7] 6
assertMoneyEqualLen [$cur getLongestByName "testmoney"] 6
assertMoneyEqualLen [$cur getLongestByIndex 8] 6
assertMoneyEqualLen [$cur getLongestByName "testsmallmoney"] 6
assertEqual [$cur getLongestByIndex 9] 23
assertEqual [$cur getLongestByName "testdatetime"] 23
assertEqual [$cur getLongestByIndex 10] 19
assertEqual [$cur getLongestByName "testsmalldatetime"] 19
assertEqual [$cur getLongestByIndex 11] 40
assertEqual [$cur getLongestByName "testchar"] 40
assertEqual [$cur getLongestByIndex 12] 12
assertEqual [$cur getLongestByName "testvarchar"] 12
assertEqual [$cur getLongestByIndex 13] 1
assertEqual [$cur getLongestByName "testbit"] 1
assertEqual [$cur getLongestByIndex 14] 10
assertEqual [$cur getLongestByName "testdate"] 10
assertEqual [$cur getLongestByIndex 15] 16
assertEqual [$cur getLongestByName "testtime"] 16
assertEqual [$cur getLongestByIndex 16] 27
assertEqual [$cur getLongestByName "testdatetime2"] 27
puts ""


# row count
puts "ROW COUNT: "
assertEqual [$cur rowCount] 8
puts ""


# total rows
puts "TOTAL ROWS: "
assertEqual [$cur totalRows] 0
puts ""


# first row index
puts "FIRST ROW INDEX: "
assertEqual [$cur firstRowIndex] 0
puts ""


# end of result set
puts "END OF RESULT SET: "
assertTrue [$cur endOfResultSet]
puts ""


# fields by index
puts "FIELDS BY INDEX: "
assertEqual [$cur getFieldByIndex 0 0] "1"
assertEqual [$cur getFieldByIndex 0 1] "1"
assertEqual [$cur getFieldByIndex 0 2] "1"
assertEqual [$cur getFieldByIndex 0 3] "1.5"
assertEqual [$cur getFieldByIndex 0 4] "1.5"
assertEqual [$cur getFieldByIndex 0 5] "1.5"
assertEqual [$cur getFieldByIndex 0 6] "1.5"
assertMoneyEqual [$cur getFieldByIndex 0 7] "1.0000"
assertMoneyEqual [$cur getFieldByIndex 0 8] "1.0000"
assertEqual [$cur getFieldByIndex 0 9] "2001-01-01 01:00:00.000"
assertEqual [$cur getFieldByIndex 0 10] "2001-01-01 01:00:00"
assertEqual [$cur getFieldByIndex 0 11] "testchar1                               "
assertEqual [$cur getFieldByIndex 0 12] "testvarchar1"
assertEqual [$cur getFieldByIndex 0 13] "1"
assertEqual [$cur getFieldByIndex 0 14] "2001-01-01"
assertEqual [$cur getFieldByIndex 0 15] "13:01:01.0000000"
assertEqual [$cur getFieldByIndex 0 16] "2001-01-01 13:01:01.0000000"
puts ""
assertEqual [$cur getFieldByIndex 7 0] "8"
assertEqual [$cur getFieldByIndex 7 1] "8"
assertEqual [$cur getFieldByIndex 7 2] "8"
assertEqual [$cur getFieldByIndex 7 3] "8.5"
assertEqual [$cur getFieldByIndex 7 4] "8.5"
assertEqual [$cur getFieldByIndex 7 5] "8.5"
assertEqual [$cur getFieldByIndex 7 6] "8.5"
assertMoneyEqual [$cur getFieldByIndex 7 7] "8.0000"
assertMoneyEqual [$cur getFieldByIndex 7 8] "8.0000"
assertEqual [$cur getFieldByIndex 7 9] "2008-01-01 08:00:00.000"
assertEqual [$cur getFieldByIndex 7 10] "2008-01-01 08:00:00"
assertEqual [$cur getFieldByIndex 7 11] "testchar8                               "
assertEqual [$cur getFieldByIndex 7 12] "testvarchar8"
assertEqual [$cur getFieldByIndex 7 13] "1"
assertEqual [$cur getFieldByIndex 7 14] "2001-01-01"
assertEqual [$cur getFieldByIndex 7 15] "13:01:01.0000000"
assertEqual [$cur getFieldByIndex 7 16] "2001-01-01 13:01:01.0000000"
puts ""


# field lengths by index
puts "FIELD LENGTHS BY INDEX: "
assertEqual [$cur getFieldLengthByIndex 0 0] 1
assertEqual [$cur getFieldLengthByIndex 0 1] 1
assertEqual [$cur getFieldLengthByIndex 0 2] 1
assertEqual [$cur getFieldLengthByIndex 0 3] 3
assertEqual [$cur getFieldLengthByIndex 0 4] 3
assertEqual [$cur getFieldLengthByIndex 0 5] 3
assertEqual [$cur getFieldLengthByIndex 0 6] 3
assertMoneyEqualLen [$cur getFieldLengthByIndex 0 7] 6
assertMoneyEqualLen [$cur getFieldLengthByIndex 0 8] 6
assertEqual [$cur getFieldLengthByIndex 0 9] 23
assertEqual [$cur getFieldLengthByIndex 0 10] 19
assertEqual [$cur getFieldLengthByIndex 0 11] 40
assertEqual [$cur getFieldLengthByIndex 0 12] 12
assertEqual [$cur getFieldLengthByIndex 0 13] 1
assertEqual [$cur getFieldLengthByIndex 0 14] 10
assertEqual [$cur getFieldLengthByIndex 0 15] 16
assertEqual [$cur getFieldLengthByIndex 0 16] 27
puts ""
assertEqual [$cur getFieldLengthByIndex 7 0] 1
assertEqual [$cur getFieldLengthByIndex 7 1] 1
assertEqual [$cur getFieldLengthByIndex 7 2] 1
assertEqual [$cur getFieldLengthByIndex 7 3] 3
assertEqual [$cur getFieldLengthByIndex 7 4] 3
assertEqual [$cur getFieldLengthByIndex 7 5] 3
assertEqual [$cur getFieldLengthByIndex 7 6] 3
assertMoneyEqualLen [$cur getFieldLengthByIndex 7 7] 6
assertMoneyEqualLen [$cur getFieldLengthByIndex 7 8] 6
assertEqual [$cur getFieldLengthByIndex 7 9] 23
assertEqual [$cur getFieldLengthByIndex 7 10] 19
assertEqual [$cur getFieldLengthByIndex 7 11] 40
assertEqual [$cur getFieldLengthByIndex 7 12] 12
assertEqual [$cur getFieldLengthByIndex 7 13] 1
assertEqual [$cur getFieldLengthByIndex 7 14] 10
assertEqual [$cur getFieldLengthByIndex 7 15] 16
assertEqual [$cur getFieldLengthByIndex 7 16] 27
puts ""


# fields by name
puts "FIELDS BY NAME: "
assertEqual [$cur getFieldByName 0 "testint"] "1"
assertEqual [$cur getFieldByName 0 "testsmallint"] "1"
assertEqual [$cur getFieldByName 0 "testtinyint"] "1"
assertEqual [$cur getFieldByName 0 "testreal"] "1.5"
assertEqual [$cur getFieldByName 0 "testfloat"] "1.5"
assertEqual [$cur getFieldByName 0 "testdecimal"] "1.5"
assertEqual [$cur getFieldByName 0 "testnumeric"] "1.5"
assertMoneyEqual [$cur getFieldByName 0 "testmoney"] "1.0000"
assertMoneyEqual [$cur getFieldByName 0 "testsmallmoney"] "1.0000"
assertEqual [$cur getFieldByName 0 "testdatetime"] "2001-01-01 01:00:00.000"
assertEqual [$cur getFieldByName 0 "testsmalldatetime"] "2001-01-01 01:00:00"
assertEqual [$cur getFieldByName 0 "testchar"] "testchar1                               "
assertEqual [$cur getFieldByName 0 "testvarchar"] "testvarchar1"
assertEqual [$cur getFieldByName 0 "testbit"] "1"
assertEqual [$cur getFieldByName 0 "testdate"] "2001-01-01"
assertEqual [$cur getFieldByName 0 "testtime"] "13:01:01.0000000"
assertEqual [$cur getFieldByName 0 "testdatetime2"] "2001-01-01 13:01:01.0000000"
puts ""
assertEqual [$cur getFieldByName 7 "testint"] "8"
assertEqual [$cur getFieldByName 7 "testsmallint"] "8"
assertEqual [$cur getFieldByName 7 "testtinyint"] "8"
assertEqual [$cur getFieldByName 7 "testreal"] "8.5"
assertEqual [$cur getFieldByName 7 "testfloat"] "8.5"
assertEqual [$cur getFieldByName 7 "testdecimal"] "8.5"
assertEqual [$cur getFieldByName 7 "testnumeric"] "8.5"
assertMoneyEqual [$cur getFieldByName 7 "testmoney"] "8.0000"
assertMoneyEqual [$cur getFieldByName 7 "testsmallmoney"] "8.0000"
assertEqual [$cur getFieldByName 7 "testdatetime"] "2008-01-01 08:00:00.000"
assertEqual [$cur getFieldByName 7 "testsmalldatetime"] "2008-01-01 08:00:00"
assertEqual [$cur getFieldByName 7 "testchar"] "testchar8                               "
assertEqual [$cur getFieldByName 7 "testvarchar"] "testvarchar8"
assertEqual [$cur getFieldByName 7 "testbit"] "1"
assertEqual [$cur getFieldByName 7 "testdate"] "2001-01-01"
assertEqual [$cur getFieldByName 7 "testtime"] "13:01:01.0000000"
assertEqual [$cur getFieldByName 7 "testdatetime2"] "2001-01-01 13:01:01.0000000"
puts ""


# field lengths by name
puts "FIELD LENGTHS BY NAME: "
assertEqual [$cur getFieldLengthByName 0 "testint"] 1
assertEqual [$cur getFieldLengthByName 0 "testsmallint"] 1
assertEqual [$cur getFieldLengthByName 0 "testtinyint"] 1
assertEqual [$cur getFieldLengthByName 0 "testreal"] 3
assertEqual [$cur getFieldLengthByName 0 "testfloat"] 3
assertEqual [$cur getFieldLengthByName 0 "testdecimal"] 3
assertEqual [$cur getFieldLengthByName 0 "testnumeric"] 3
assertMoneyEqualLen [$cur getFieldLengthByName 0 "testmoney"] 6
assertMoneyEqualLen [$cur getFieldLengthByName 0 "testsmallmoney"] 6
assertEqual [$cur getFieldLengthByName 0 "testdatetime"] 23
assertEqual [$cur getFieldLengthByName 0 "testsmalldatetime"] 19
assertEqual [$cur getFieldLengthByName 0 "testchar"] 40
assertEqual [$cur getFieldLengthByName 0 "testvarchar"] 12
assertEqual [$cur getFieldLengthByName 0 "testbit"] 1
assertEqual [$cur getFieldLengthByName 0 "testdate"] 10
assertEqual [$cur getFieldLengthByName 0 "testtime"] 16
assertEqual [$cur getFieldLengthByName 0 "testdatetime2"] 27
puts ""
assertEqual [$cur getFieldLengthByName 7 "testint"] 1
assertEqual [$cur getFieldLengthByName 7 "testsmallint"] 1
assertEqual [$cur getFieldLengthByName 7 "testtinyint"] 1
assertEqual [$cur getFieldLengthByName 7 "testreal"] 3
assertEqual [$cur getFieldLengthByName 7 "testfloat"] 3
assertEqual [$cur getFieldLengthByName 7 "testdecimal"] 3
assertEqual [$cur getFieldLengthByName 7 "testnumeric"] 3
assertMoneyEqualLen [$cur getFieldLengthByName 7 "testmoney"] 6
assertMoneyEqualLen [$cur getFieldLengthByName 7 "testsmallmoney"] 6
assertEqual [$cur getFieldLengthByName 7 "testdatetime"] 23
assertEqual [$cur getFieldLengthByName 7 "testsmalldatetime"] 19
assertEqual [$cur getFieldLengthByName 7 "testchar"] 40
assertEqual [$cur getFieldLengthByName 7 "testvarchar"] 12
assertEqual [$cur getFieldLengthByName 7 "testbit"] 1
assertEqual [$cur getFieldLengthByName 7 "testdate"] 10
assertEqual [$cur getFieldLengthByName 7 "testtime"] 16
assertEqual [$cur getFieldLengthByName 7 "testdatetime2"] 27
puts ""


# fields by array
puts "FIELDS BY ARRAY: "
set fields [$cur getRow 0]
assertEqual [lindex $fields 0] "1"
assertEqual [lindex $fields 1] "1"
assertEqual [lindex $fields 2] "1"
assertEqual [lindex $fields 3] "1.5"
assertEqual [lindex $fields 4] "1.5"
assertEqual [lindex $fields 5] "1.5"
assertEqual [lindex $fields 6] "1.5"
assertMoneyEqual [lindex $fields 7] "1.0000"
assertMoneyEqual [lindex $fields 8] "1.0000"
assertEqual [lindex $fields 9] "2001-01-01 01:00:00.000"
assertEqual [lindex $fields 10] "2001-01-01 01:00:00"
assertEqual [lindex $fields 11] "testchar1                               "
assertEqual [lindex $fields 12] "testvarchar1"
assertEqual [lindex $fields 13] "1"
assertEqual [lindex $fields 14] "2001-01-01"
assertEqual [lindex $fields 15] "13:01:01.0000000"
assertEqual [lindex $fields 16] "2001-01-01 13:01:01.0000000"
puts ""


# field lengths by array
puts "FIELD LENGTHS BY ARRAY: "
set fieldlens [$cur getRowLengths 0]
assertEqual [lindex $fieldlens 0] 1
assertEqual [lindex $fieldlens 1] 1
assertEqual [lindex $fieldlens 2] 1
assertEqual [lindex $fieldlens 3] 3
assertEqual [lindex $fieldlens 4] 3
assertEqual [lindex $fieldlens 5] 3
assertEqual [lindex $fieldlens 6] 3
assertMoneyEqualLen [lindex $fieldlens 7] 6
assertMoneyEqualLen [lindex $fieldlens 8] 6
assertEqual [lindex $fieldlens 9] 23
assertEqual [lindex $fieldlens 10] 19
assertEqual [lindex $fieldlens 11] 40
assertEqual [lindex $fieldlens 12] 12
assertEqual [lindex $fieldlens 13] 1
assertEqual [lindex $fieldlens 14] 10
assertEqual [lindex $fieldlens 15] 16
assertEqual [lindex $fieldlens 16] 27
puts ""


# result set buffer size
puts "RESULT SET BUFFER SIZE: "
assertEqual [$cur getResultSetBufferSize] 0
$cur setResultSetBufferSize 2
assertTrue [$cur sendQuery "select * from testtable order by testint"]
assertEqual [$cur getResultSetBufferSize] 2
puts ""
assertEqual [$cur firstRowIndex] 0
assertFalse [$cur endOfResultSet]
assertEqual [$cur rowCount] 2
assertEqual [$cur getFieldByIndex 0 0] "1"
assertEqual [$cur getFieldByIndex 1 0] "2"
assertEqual [$cur getFieldByIndex 2 0] "3"
puts ""
assertEqual [$cur firstRowIndex] 2
assertFalse [$cur endOfResultSet]
assertEqual [$cur rowCount] 4
assertEqual [$cur getFieldByIndex 6 0] "7"
assertEqual [$cur getFieldByIndex 7 0] "8"
puts ""
assertEqual [$cur firstRowIndex] 6
assertFalse [$cur endOfResultSet]
assertEqual [$cur rowCount] 8
assertUndef [$cur getFieldByIndex 8 0]
puts ""
assertEqual [$cur firstRowIndex] 8
assertTrue [$cur endOfResultSet]
assertEqual [$cur rowCount] 8
$cur setResultSetBufferSize 0
puts ""


# dont get column info
puts "DONT GET COLUMN INFO: "
$cur dontGetColumnInfo
assertTrue [$cur sendQuery "select * from testtable order by testint"]
assertUndef [$cur getColumnName 0]
assertEqual [$cur getColumnLengthByIndex 0] 0
assertUndef [$cur getColumnTypeByIndex 0]
$cur getColumnInfo
assertTrue [$cur sendQuery "select * from testtable order by testint"]
assertEqual [$cur getColumnName 0] "testint"
assertEqual [$cur getColumnLengthByIndex 0] 10
assertEqual [$cur getColumnTypeByIndex 0] "INTEGER"
puts ""


# suspended session
puts "SUSPENDED SESSION: "
assertTrue [$cur sendQuery "select * from testtable order by testint"]
$cur suspendResultSet
assertTrue [$con suspendSession]
set port [$con getConnectionPort]
set socket [$con getConnectionSocket]
assertTrue [$con resumeSession $port $socket]
puts ""
assertEqual [$cur getFieldByIndex 0 0] "1"
assertEqual [$cur getFieldByIndex 1 0] "2"
assertEqual [$cur getFieldByIndex 2 0] "3"
assertEqual [$cur getFieldByIndex 3 0] "4"
assertEqual [$cur getFieldByIndex 4 0] "5"
assertEqual [$cur getFieldByIndex 5 0] "6"
assertEqual [$cur getFieldByIndex 6 0] "7"
assertEqual [$cur getFieldByIndex 7 0] "8"
puts ""
assertTrue [$cur sendQuery "select * from testtable order by testint"]
$cur suspendResultSet
assertTrue [$con suspendSession]
set port [$con getConnectionPort]
set socket [$con getConnectionSocket]
assertTrue [$con resumeSession $port $socket]
assertEqual [$cur getFieldByIndex 0 0] "1"
assertEqual [$cur getFieldByIndex 1 0] "2"
assertEqual [$cur getFieldByIndex 2 0] "3"
assertEqual [$cur getFieldByIndex 3 0] "4"
assertEqual [$cur getFieldByIndex 4 0] "5"
assertEqual [$cur getFieldByIndex 5 0] "6"
assertEqual [$cur getFieldByIndex 6 0] "7"
assertEqual [$cur getFieldByIndex 7 0] "8"
puts ""
assertTrue [$cur sendQuery "select * from testtable order by testint"]
$cur suspendResultSet
assertTrue [$con suspendSession]
set port [$con getConnectionPort]
set socket [$con getConnectionSocket]
assertTrue [$con resumeSession $port $socket]
puts ""
assertEqual [$cur getFieldByIndex 0 0] "1"
assertEqual [$cur getFieldByIndex 1 0] "2"
assertEqual [$cur getFieldByIndex 2 0] "3"
assertEqual [$cur getFieldByIndex 3 0] "4"
assertEqual [$cur getFieldByIndex 4 0] "5"
assertEqual [$cur getFieldByIndex 5 0] "6"
assertEqual [$cur getFieldByIndex 6 0] "7"
assertEqual [$cur getFieldByIndex 7 0] "8"
puts ""


# suspended result set
puts "SUSPENDED RESULT SET: "
$cur setResultSetBufferSize 2
assertTrue [$cur sendQuery "select * from testtable order by testint"]
assertEqual [$cur getFieldByIndex 2 0] "3"
set id [$cur getResultSetId]
$cur suspendResultSet
assertTrue [$con suspendSession]
set port [$con getConnectionPort]
set socket [$con getConnectionSocket]
assertTrue [$con resumeSession $port $socket]
assertTrue [$cur resumeResultSet $id]
puts ""
assertEqual [$cur firstRowIndex] 4
assertFalse [$cur endOfResultSet]
assertEqual [$cur rowCount] 6
assertEqual [$cur getFieldByIndex 7 0] "8"
puts ""
assertEqual [$cur firstRowIndex] 6
assertFalse [$cur endOfResultSet]
assertEqual [$cur rowCount] 8
assertUndef [$cur getFieldByIndex 8 0]
puts ""
assertEqual [$cur firstRowIndex] 8
assertTrue [$cur endOfResultSet]
assertEqual [$cur rowCount] 8
$cur setResultSetBufferSize 0
puts ""


# cached result set
puts "CACHED RESULT SET: "
$cur cacheToFile "cachefile1-odbc-mssql"
$cur setCacheTtl 200
assertTrue [$cur sendQuery "select * from testtable order by testint"]
set filename [$cur getCacheFileName]
assertEqual $filename "cachefile1-odbc-mssql"
$cur cacheOff
assertTrue [$cur openCachedResultSet $filename]
assertEqual [$cur getFieldByIndex 7 0] "8"
puts ""


# column count for cached result set
puts "COLUMN COUNT FOR CACHED RESULT SET: "
assertEqual [$cur colCount] 17
puts ""


# column names for cached result set
puts "COLUMN NAMES FOR CACHED RESULT SET: "
assertEqual [$cur getColumnName 0] "testint"
assertEqual [$cur getColumnName 1] "testsmallint"
assertEqual [$cur getColumnName 2] "testtinyint"
assertEqual [$cur getColumnName 3] "testreal"
assertEqual [$cur getColumnName 4] "testfloat"
assertEqual [$cur getColumnName 5] "testdecimal"
assertEqual [$cur getColumnName 6] "testnumeric"
assertEqual [$cur getColumnName 7] "testmoney"
assertEqual [$cur getColumnName 8] "testsmallmoney"
assertEqual [$cur getColumnName 9] "testdatetime"
assertEqual [$cur getColumnName 10] "testsmalldatetime"
assertEqual [$cur getColumnName 11] "testchar"
assertEqual [$cur getColumnName 12] "testvarchar"
assertEqual [$cur getColumnName 13] "testbit"
assertEqual [$cur getColumnName 14] "testdate"
assertEqual [$cur getColumnName 15] "testtime"
assertEqual [$cur getColumnName 16] "testdatetime2"
set cols [$cur getColumnNames]
assertEqual [lindex $cols 0] "testint"
assertEqual [lindex $cols 1] "testsmallint"
assertEqual [lindex $cols 2] "testtinyint"
assertEqual [lindex $cols 3] "testreal"
assertEqual [lindex $cols 4] "testfloat"
assertEqual [lindex $cols 5] "testdecimal"
assertEqual [lindex $cols 6] "testnumeric"
assertEqual [lindex $cols 7] "testmoney"
assertEqual [lindex $cols 8] "testsmallmoney"
assertEqual [lindex $cols 9] "testdatetime"
assertEqual [lindex $cols 10] "testsmalldatetime"
assertEqual [lindex $cols 11] "testchar"
assertEqual [lindex $cols 12] "testvarchar"
assertEqual [lindex $cols 13] "testbit"
assertEqual [lindex $cols 14] "testdate"
assertEqual [lindex $cols 15] "testtime"
assertEqual [lindex $cols 16] "testdatetime2"
puts ""


# cached result set with result set buffer size
puts "CACHED RESULT SET WITH RESULT SET BUFFER SIZE: "
$cur setResultSetBufferSize 2
$cur cacheToFile "cachefile1-odbc-mssql"
$cur setCacheTtl 200
assertTrue [$cur sendQuery "select * from testtable order by testint"]
set filename [$cur getCacheFileName]
assertEqual $filename "cachefile1-odbc-mssql"
$cur cacheOff
assertTrue [$cur openCachedResultSet $filename]
assertEqual [$cur getFieldByIndex 7 0] "8"
assertUndef [$cur getFieldByIndex 8 0]
$cur setResultSetBufferSize 0
puts ""


# from one cache file to another
puts "FROM ONE CACHE FILE TO ANOTHER: "
$cur cacheToFile "cachefile2-odbc-mssql"
assertTrue [$cur openCachedResultSet "cachefile1-odbc-mssql"]
$cur cacheOff
assertTrue [$cur openCachedResultSet "cachefile2-odbc-mssql"]
assertEqual [$cur getFieldByIndex 7 0] "8"
assertUndef [$cur getFieldByIndex 8 0]
puts ""


# from one cache file to another with result set buffer size
puts "FROM ONE CACHE FILE TO ANOTHER WITH RESULT SET BUFFER SIZE: "
$cur setResultSetBufferSize 2
$cur cacheToFile "cachefile2-odbc-mssql"
assertTrue [$cur openCachedResultSet "cachefile1-odbc-mssql"]
$cur cacheOff
assertTrue [$cur openCachedResultSet "cachefile2-odbc-mssql"]
assertEqual [$cur getFieldByIndex 7 0] "8"
assertUndef [$cur getFieldByIndex 8 0]
$cur setResultSetBufferSize 0
puts ""


# cached result set with suspend and result set buffer size
puts "CACHED RESULT SET WITH SUSPEND AND RESULT SET BUFFER SIZE: "
$cur setResultSetBufferSize 2
$cur cacheToFile "cachefile1-odbc-mssql"
$cur setCacheTtl 200
assertTrue [$cur sendQuery "select * from testtable order by testint"]
assertEqual [$cur getFieldByIndex 2 0] "3"
set filename [$cur getCacheFileName]
assertEqual $filename "cachefile1-odbc-mssql"
set id [$cur getResultSetId]
$cur suspendResultSet
assertTrue [$con suspendSession]
set port [$con getConnectionPort]
set socket [$con getConnectionSocket]
puts ""
assertTrue [$con resumeSession $port $socket]
assertTrue [$cur resumeCachedResultSet $id $filename]
puts ""
assertEqual [$cur firstRowIndex] 4
assertFalse [$cur endOfResultSet]
assertEqual [$cur rowCount] 6
assertEqual [$cur getFieldByIndex 7 0] "8"
puts ""
assertEqual [$cur firstRowIndex] 6
assertFalse [$cur endOfResultSet]
assertEqual [$cur rowCount] 8
assertUndef [$cur getFieldByIndex 8 0]
puts ""
assertEqual [$cur firstRowIndex] 8
assertTrue [$cur endOfResultSet]
assertEqual [$cur rowCount] 8
$cur cacheOff
puts ""
assertTrue [$cur openCachedResultSet $filename]
assertEqual [$cur getFieldByIndex 7 0] "8"
assertUndef [$cur getFieldByIndex 8 0]
$cur setResultSetBufferSize 0
puts ""


# finished suspended session
puts "FINISHED SUSPENDED SESSION: "
assertTrue [$cur sendQuery "select * from testtable order by testint"]
assertEqual [$cur getFieldByIndex 4 0] "5"
assertEqual [$cur getFieldByIndex 5 0] "6"
assertEqual [$cur getFieldByIndex 6 0] "7"
assertEqual [$cur getFieldByIndex 7 0] "8"
set id [$cur getResultSetId]
$cur suspendResultSet
assertTrue [$con suspendSession]
set port [$con getConnectionPort]
set socket [$con getConnectionSocket]
assertTrue [$con resumeSession $port $socket]
assertTrue [$cur resumeResultSet $id]
assertUndef [$cur getFieldByIndex 4 0]
assertUndef [$cur getFieldByIndex 5 0]
assertUndef [$cur getFieldByIndex 6 0]
assertUndef [$cur getFieldByIndex 7 0]
puts ""


# nested selects
puts "NESTED SELECTS: "
# can't do this with odbc
#$cur setResultSetBufferSize 1
assertTrue [$cur sendQuery "select * from testtable"]
set secondcur [$con sqlrcur]
$secondcur setResultSetBufferSize 1
set i 0
while {[llength [$cur getRow $i]] > 0} {
	assertTrue [$secondcur sendQuery "select * from testtable"]
	incr i
}
$secondcur closeResultSet
#$cur setResultSetBufferSize 0
assertTrue [$con commit]
assertTrue [$cur sendQuery "drop table testtable"]
puts ""


# reset transaction state
puts "RESET TRANSACTION STATE: "
assertTrue [$con commit]
assertEqual [$con getTransactionModel] "explicit"
assertTrue [$con getAutoCommit]
puts ""


# transaction behavior - implicit
puts "TRANSACTION BEHAVIOR - implicit: "
# switching to the implicit model turns autocommit off, so a table
# created after the switch stays inside an uncommitted transaction,
# and mssql holds a schema lock on it that blocks secondcon's reads -
# a lock that readpast can't skip.  create it while autocommit is
# still on, then switch
assertTrue [$cur sendQuery "create table testtable (col1 integer)"]
assertTrue [$con setTransactionModel "implicit"]
assertEqual [$con getTransactionModel] "implicit"
set secondcon [sqlrcon -server "sqlrelay" -port 9007 -socket "/tmp/odbc-mssql.socket" -user "testuser" -password "testpassword" -retrytime 0 -tries 1]
set secondcur [$secondcon sqlrcur]
# session is in a transaction; insert is not visible until commit
assertTrue [$con getInTransaction]
assertFalse [$con getAutoCommit]
assertTrue [$cur sendQuery "insert into testtable values (1)"]
# at read committed, a plain count(*) scan blocks on the writer's
# uncommitted row until the transaction ends, so the test would hang
# rather than fail.  readpast skips the locked row instead, which
# still counts only committed rows and so still catches a premature
# commit.  it does assume the writer's locks stay at row granularity;
# were they to escalate, committed rows would be skipped too and the
# counts would come back low
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "0"
# commit makes it visible, and implicitly starts a new transaction
assertTrue [$con commit]
assertTrue [$con getInTransaction]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "1"
# rollback discards, and implicitly starts a new transaction
assertTrue [$cur sendQuery "insert into testtable values (2)"]
assertTrue [$con rollback]
assertTrue [$con getInTransaction]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "1"
# autoCommitOn takes effect immediately
assertTrue [$con autoCommit true]
assertTrue [$con getAutoCommit]
assertFalse [$con getInTransaction]
assertTrue [$cur sendQuery "insert into testtable values (3)"]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "2"
# autoCommitOff takes effect immediately
assertTrue [$con autoCommit false]
assertFalse [$con getAutoCommit]
assertTrue [$con getInTransaction]
$secondcur closeResultSet
# autocommit-off left a transaction open, and switching the transaction
# model doesn't end it here.  the drop below would then sit in that
# transaction, holding a schema lock that the next section's reader
# blocks on rather than fails on, so put autocommit back on first
assertTrue [$con autoCommit true]
assertTrue [$cur sendQuery "drop table testtable"]
puts ""


# transaction behavior - explicit
puts "TRANSACTION BEHAVIOR - explicit: "
assertTrue [$con setTransactionModel "explicit"]
assertEqual [$con getTransactionModel] "explicit"
assertTrue [$cur sendQuery "create table testtable (col1 integer)"]
# begin starts a new transaction; insert is not visible until commit
assertTrue [$con begin]
assertTrue [$con getInTransaction]
assertTrue [$cur sendQuery "insert into testtable values (1)"]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "0"
# commit makes it visible; no new transaction is started
assertTrue [$con commit]
assertFalse [$con getInTransaction]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "1"
# begin, insert, rollback discards; no new transaction is started
assertTrue [$con begin]
assertTrue [$cur sendQuery "insert into testtable values (2)"]
assertTrue [$con rollback]
assertFalse [$con getInTransaction]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "1"
# autoCommitOn takes effect immediately
assertTrue [$con autoCommit true]
assertTrue [$con getAutoCommit]
assertTrue [$cur sendQuery "insert into testtable values (3)"]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "2"
# autoCommitOff takes effect immediately
assertTrue [$con autoCommit false]
assertFalse [$con getAutoCommit]
$secondcur closeResultSet
assertTrue [$cur sendQuery "drop table testtable"]
puts ""


# transaction behavior - explicit-deferred
puts "TRANSACTION BEHAVIOR - explicit-deferred: "
assertTrue [$con setTransactionModel "explicit-deferred"]
assertEqual [$con getTransactionModel] "explicit-deferred"
# switch to autocommit-on so the begin/commit cycles below
# bracket explicit transactions (autocommit-off semantics are
# exercised at the end of this block)
assertTrue [$con autoCommit true]
assertTrue [$con getAutoCommit]
assertTrue [$cur sendQuery "create table testtable (col1 integer)"]
# begin starts a transaction; commit makes it visible
assertTrue [$con begin]
assertTrue [$con getInTransaction]
assertTrue [$cur sendQuery "insert into testtable values (1)"]
assertTrue [$con commit]
assertFalse [$con getInTransaction]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "1"
# begin, insert, rollback discards
assertTrue [$con begin]
assertTrue [$cur sendQuery "insert into testtable values (2)"]
assertTrue [$con rollback]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "1"
# during a transaction started by begin(), autoCommitOn is a
# no-op: the autocommit setting takes effect after the user
# explicitly commits/rollbacks the tx (mysql-native semantic)
assertTrue [$con begin]
assertTrue [$cur sendQuery "insert into testtable values (3)"]
assertTrue [$con autoCommit true]
assertFalse [$con getAutoCommit]
assertTrue [$con getInTransaction]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "1"
# explicit commit ends the tx; autocommit-on now takes effect
assertTrue [$con commit]
assertTrue [$con getAutoCommit]
assertFalse [$con getInTransaction]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "2"
# autocommit is on; subsequent inserts are visible immediately
assertTrue [$cur sendQuery "insert into testtable values (4)"]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "3"
# autoCommitOff takes effect immediately when not in a transaction
assertTrue [$con autoCommit false]
assertFalse [$con getAutoCommit]
# autocommit-off persists across commit/rollback; each commit or
# rollback ends the current implicit tx and a new one starts for
# the next statement
assertTrue [$cur sendQuery "insert into testtable values (5)"]
assertTrue [$con commit]
assertFalse [$con getAutoCommit]
assertTrue [$con getInTransaction]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "4"
assertTrue [$cur sendQuery "insert into testtable values (6)"]
assertTrue [$con rollback]
assertFalse [$con getAutoCommit]
assertTrue [$con getInTransaction]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "4"
# autoCommitOff during a transaction changes the variable
# immediately but the in-flight tx continues; only after the
# next explicit commit/rollback does the new autocommit-off
# setting drop us into a new implicit tx (mysql-asymmetric
# semantic)
assertTrue [$con autoCommit true]
assertTrue [$con getAutoCommit]
assertTrue [$con begin]
assertTrue [$cur sendQuery "insert into testtable values (7)"]
assertTrue [$con autoCommit false]
assertFalse [$con getAutoCommit]
assertTrue [$con getInTransaction]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "4"
assertTrue [$con commit]
assertFalse [$con getAutoCommit]
assertTrue [$con getInTransaction]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "5"
$secondcur closeResultSet
assertTrue [$cur sendQuery "drop table testtable"]
puts ""


# transaction behavior - explicit-error
puts "TRANSACTION BEHAVIOR - explicit-error: "
assertTrue [$con setTransactionModel "explicit-error"]
assertEqual [$con getTransactionModel] "explicit-error"
assertTrue [$cur sendQuery "create table testtable (col1 integer)"]
# begin, insert, commit
assertTrue [$con begin]
assertTrue [$con getInTransaction]
assertTrue [$cur sendQuery "insert into testtable values (1)"]
assertTrue [$con commit]
assertFalse [$con getInTransaction]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "1"
# begin, insert, rollback
assertTrue [$con begin]
assertTrue [$cur sendQuery "insert into testtable values (2)"]
assertTrue [$con rollback]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "1"
# while in a transaction, autoCommitOn/Off throw an error
assertTrue [$con begin]
assertFalse [$con autoCommit true]
assertFalse [$con autoCommit false]
assertTrue [$con commit]
# outside of a transaction, autoCommitOn takes effect immediately
assertTrue [$con autoCommit true]
assertTrue [$con getAutoCommit]
assertTrue [$cur sendQuery "insert into testtable values (3)"]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "2"
# autoCommitOff takes effect immediately
assertTrue [$con autoCommit false]
assertFalse [$con getAutoCommit]
$secondcur closeResultSet
assertTrue [$cur sendQuery "drop table testtable"]
puts ""


# transaction behavior - none
puts "TRANSACTION BEHAVIOR - none: "
assertTrue [$con setTransactionModel "none"]
assertEqual [$con getTransactionModel] "none"
assertTrue [$cur sendQuery "create table testtable (col1 integer)"]
# no transactions; everything is visible immediately
assertTrue [$con getAutoCommit]
assertFalse [$con getInTransaction]
assertTrue [$cur sendQuery "insert into testtable values (1)"]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "1"
# commit and rollback are no-ops
assertTrue [$con commit]
assertTrue [$cur sendQuery "insert into testtable values (2)"]
assertTrue [$con rollback]
assertTrue [$secondcur sendQuery "select count(*) from testtable with (readpast)"]
assertEqual [$secondcur getFieldByIndex 0 0] "2"
# autocommit is always on; autoCommitOff is an error
assertFalse [$con autoCommit false]
assertTrue [$con getAutoCommit]
assertTrue [$con autoCommit true]
assertTrue [$con getAutoCommit]
$secondcur closeResultSet
assertTrue [$cur sendQuery "drop table testtable"]
puts ""


# reset transaction behavior
puts "RESET TRANSACTION BEHAVIOR: "
assertTrue [$con setTransactionModel [$con getDefaultTransactionModel]]
assertEqual [$con getTransactionModel] "explicit"
assertTrue [$con getAutoCommit]
puts ""


# individual substitutions
puts "INDIVIDUAL SUBSTITUTIONS: "
$cur prepareQuery "select \$(var1),'\$(var2)',\$(var3)"
$cur substitution "var1" 1
$cur substitution "var2" "hello"
$cur substitution "var3" 10.5556 6 4
assertTrue [$cur executeQuery]
assertEqual [$cur getFieldByIndex 0 0] "1"
assertEqual [$cur getFieldByIndex 0 1] "hello"
assertEqual [$cur getFieldByIndex 0 2] "10.5556"
puts ""


# array substitutions
puts "ARRAY SUBSTITUTIONS: "
$cur prepareQuery "select \$(var1),\$(var2),\$(var3)"
$cur substitutions {{"var1" 1} {"var2" 2} {"var3" 3}}
assertTrue [$cur executeQuery]
assertEqual [$cur getFieldByIndex 0 0] "1"
assertEqual [$cur getFieldByIndex 0 1] "2"
assertEqual [$cur getFieldByIndex 0 2] "3"
puts ""
$cur prepareQuery "select '\$(var1)','\$(var2)','\$(var3)'"
$cur substitutions {{"var1" "hi"} {"var2" "hello"} {"var3" "bye"}}
assertTrue [$cur executeQuery]
assertEqual [$cur getFieldByIndex 0 0] "hi"
assertEqual [$cur getFieldByIndex 0 1] "hello"
assertEqual [$cur getFieldByIndex 0 2] "bye"
puts ""
$cur prepareQuery "select \$(var1),\$(var2),\$(var3)"
$cur substitutions {{"var1" 10.55 4 2} {"var2" 10.556 5 3} {"var3" 10.5556 6 4}}
assertTrue [$cur executeQuery]
assertEqual [$cur getFieldByIndex 0 0] "10.55"
assertEqual [$cur getFieldByIndex 0 1] "10.556"
assertEqual [$cur getFieldByIndex 0 2] "10.5556"
puts ""


# nulls as nulls
# The tcl api always returns NULL as an empty string (""); it doesn't have
# getNullsAsNulls / getNullsAsEmptyStrings.
puts "NULLS AS NULLS: "
assertTrue [$cur sendQuery "select NULL,1,NULL"]
assertEqual [$cur getFieldByIndex 0 0] ""
assertEqual [$cur getFieldByIndex 0 1] "1"
assertEqual [$cur getFieldByIndex 0 2] ""
assertTrue [$cur sendQuery "select NULL,1,NULL"]
assertEqual [$cur getFieldByIndex 0 0] ""
assertEqual [$cur getFieldByIndex 0 1] "1"
assertEqual [$cur getFieldByIndex 0 2] ""
puts ""


# null and empty lobs
puts "NULL AND EMPTY LOBS: "
catch {$cur sendQuery "drop table testtable"}
assertTrue [$cur sendQuery "create table testtable (testclob1 text NULL, testclob2 text NULL, testblob1 image NULL, testblob2 image NULL)"]
$cur prepareQuery "insert into testtable values (?, ?, ?, ?)"
$cur inputBindClob "1" "" 0
$cur inputBindNull "2"
$cur inputBindBlob "3" "" 0
$cur inputBindNull "4"
assertTrue [$cur executeQuery]
$cur sendQuery "select * from testtable"
# odbc reports a true zero length for an empty (non-NULL) blob column,
# where freetds gives it the single 0x00 byte its encoder emits.  both
# compare equal to an empty string
assertEqual [$cur getFieldByIndex 0 0] ""
assertEqual [$cur getFieldByIndex 0 1] ""
assertEqual [$cur getFieldByIndex 0 2] ""
assertEqual [$cur getFieldByIndex 0 3] ""
assertTrue [$cur sendQuery "drop table testtable"]
puts ""


# long lobs
puts "LONG LOBS: "
catch {$cur sendQuery "drop table testtable"}
$cur sendQuery "create table testtable (testclob text, testblob image)"
$cur prepareQuery "insert into testtable values (?,?)"
set largebuffer [string repeat "C" 8192]
$cur inputBindClob "1" $largebuffer 8192
$cur inputBindBlob "2" $largebuffer 8192
assertTrue [$cur executeQuery]
$cur sendQuery "select * from testtable"
assertEqual [$cur getFieldLengthByName 0 "testclob"] 8192
assertEqual [$cur getFieldByName 0 "testclob"] $largebuffer
assertEqual [$cur getFieldLengthByName 0 "testblob"] 8192
assertEqualLen [$cur getFieldByName 0 "testblob"] $largebuffer 8192
assertTrue [$cur sendQuery "drop table testtable"]
puts ""


# wide nchar column
# #9411 - SQLBindCol was binding the driver's UCS-2 output directly into
# the caller's UTF-8-sized buffer, truncating wide nvarchar columns to
# roughly half their length in unicode mode.  a 4000-char value still
# fits inside a half-truncated buffer sized against the default 32768
# maxfieldsize, so this connects to a second instance whose
# maxfieldsize is reduced to 4096, where the truncation is reproducible
# at a practical column length
puts "WIDE NCHAR COLUMN: "
set secondcon [sqlrcon -server "sqlrelay" -port 9033 -socket "/tmp/odbcmssqlmaxfieldsize.socket" -user "testuser" -password "testpassword" -retrytime 0 -tries 1]
set secondcur [$secondcon sqlrcur]
catch {$secondcur sendQuery "drop table testtable"}
assertTrue [$secondcur sendQuery "create table testtable (testnchar nvarchar(4000))"]
set widencharbuffer [string repeat "N" 4000]
$secondcur prepareQuery "insert into testtable values (?)"
# not "inputBind "1" $widencharbuffer 4000" - the tcl binding's 3-arg
# (variable value length) form reads the length from the wrong Tcl_Obj
# and segfaults on a non-numeric value this long (#9423).  $widencharbuffer
# has no embedded nulls, so the 2-arg form (which computes the length
# itself) round-trips it just as well.
$secondcur inputBind "1" $widencharbuffer
assertTrue [$secondcur executeQuery]
assertTrue [$secondcur sendQuery "select testnchar from testtable"]
assertEqual [$secondcur getFieldLengthByName 0 "testnchar"] 4000
assertEqual [$secondcur getFieldByName 0 "testnchar"] $widencharbuffer
assertTrue [$secondcur sendQuery "drop table testtable"]
$secondcur closeResultSet
puts ""


# output bind by position
# the odbc module needs a placeholder for each parameter in the query -
# "exec testproc" on its own counts 0 bind variables and fails to
# execute.  "{call testproc(?,?,?,?,?)}" works too
puts "OUTPUT BIND BY POSITION: "
catch {$cur sendQuery "drop procedure testproc"}
assertTrue [$cur sendQuery "create procedure testproc @out1 int output, @out2 varchar(20) output, @out3 float output, @out4 datetime output, @out5 varchar(20) output as select @out1=1, @out2='hello', @out3=2.5, @out4='2001-02-03', @out5=null"]
$cur prepareQuery "exec testproc ?,?,?,?,?"
assertEqual [$cur countBindVariables] 5
$cur defineOutputBindInteger "1"
$cur defineOutputBindString "2" 20
$cur defineOutputBindDouble "3"
$cur defineOutputBindDate "4"
$cur defineOutputBindString "5" 20
assertTrue [$cur executeQuery]
set numvar [$cur getOutputBindInteger "1"]
set stringvar [$cur getOutputBindString "2"]
set stringvarlen [$cur getOutputBindLength "2"]
set floatvar [$cur getOutputBindDouble "3"]
set year [$cur getOutputBindDateYear "4"]
set month [$cur getOutputBindDateMonth "4"]
set day [$cur getOutputBindDateDay "4"]
set hour [$cur getOutputBindDateHour "4"]
set minute [$cur getOutputBindDateMinute "4"]
set second [$cur getOutputBindDateSecond "4"]
set microsecond [$cur getOutputBindDateMicrosecond "4"]
set tz [$cur getOutputBindDateTz "4"]
set isnegative [$cur getOutputBindDateIsNegative "4"]
set nullvar [$cur getOutputBindString "5"]
assertEqual $numvar 1
assertEqual $stringvar "hello"
assertEqual $stringvarlen 5
assertEqual $floatvar 2.5
assertEqual $year 2001
assertEqual $month 2
assertEqual $day 3
assertEqual $hour 0
assertEqual $minute 0
assertEqual $second 0
assertEqual $microsecond 0
assertEqual $tz ""
assertFalse $isnegative
assertUndef $nullvar
assertTrue [$cur sendQuery "drop procedure testproc"]
puts ""


# failed execute after output bind date
# ticket #9408 - an unbraced odbc call escape ("call testproc(...)")
# fails to execute.  reusing this cursor's date output bind
# (successfully populated by the execute above) across a
# prepareQuery/executeQuery pair that fails to execute, followed by
# another prepareQuery, used to double free a stale timezone pointer
# and abort the client
puts "FAILED EXECUTE AFTER OUTPUT BIND DATE: "
catch {$cur sendQuery "drop procedure testproc"}
assertTrue [$cur sendQuery "create procedure testproc @out1 int output, @out2 varchar(20) output, @out3 float output, @out4 datetime output, @out5 varchar(20) output as select @out1=1, @out2='hello', @out3=2.5, @out4='2001-02-03', @out5=null"]
$cur prepareQuery "call testproc(?,?,?,?,?)"
$cur defineOutputBindInteger "1"
$cur defineOutputBindString "2" 20
$cur defineOutputBindDouble "3"
$cur defineOutputBindDate "4"
$cur defineOutputBindString "5" 20
assertTrue [catch {$cur executeQuery}]
$cur prepareQuery "select 1"
assertTrue [$cur executeQuery]
assertEqual [$cur rowCount] 1
assertEqual [$cur getFieldByIndex 0 0] "1"
puts ""


# output bind by name
# odbc binds positionally, so there is nothing to bind by name


# output bind by name with validation
# odbc binds positionally, so there is nothing to bind by name.  even
# if there were, validateBinds() couldn't be used for output binds
# here.  When executing a procedure you don't declare any bind
# variable delimiters in the query.  eg, you just do: "exec testproc",
# not "exec testproc(@out1,@out2)".  If you call validateBinds(), it
# won't find any binds in the query, and will filter out any binds
# that you declare


# lob output bind
# the deprecated text, ntext and image types can't be output
# parameters, and there's no way to directly select into a lob bind
# variable


# long output bind
puts "LONG OUTPUT BIND: "
catch {$cur sendQuery "drop procedure testproc"}
set longoutputbindbuffer [string repeat "C" 8000]
assertTrue [$cur sendQuery "create procedure testproc @bindval varchar(8000) output as set @bindval='$longoutputbindbuffer'"]
$cur prepareQuery "exec testproc ?"
$cur defineOutputBindString "1" 8000
assertTrue [$cur executeQuery]
assertEqual [$cur getOutputBindLength "1"] 8000
assertEqual [$cur getOutputBindString "1"] $longoutputbindbuffer
assertTrue [$cur sendQuery "drop procedure testproc"]
puts ""


# negative input bind
puts "NEGATIVE INPUT BIND: "
catch {$cur sendQuery "drop table testtable"}
$cur sendQuery "create table testtable (testval int)"
$cur prepareQuery "insert into testtable values (?)"
$cur inputBind "1" -1
assertTrue [$cur executeQuery]
$cur sendQuery "select testval from testtable"
assertEqual [$cur getFieldByName 0 "testval"] "-1"
assertTrue [$cur sendQuery "drop table testtable"]
puts ""


# bind validation
# odbc binds positionally, and validateBinds() skips bind-by-position
# variables, so there is nothing to validate


# rebinding
puts "REBINDING: "
catch {$cur sendQuery "drop procedure testproc"}
assertTrue [$cur sendQuery "create procedure testproc @in1 int, @out1 int output as select @out1=@in1"]
$cur prepareQuery "exec testproc ?,?"
$cur inputBind "1" 1
$cur defineOutputBindInteger "2"
assertTrue [$cur executeQuery]
assertEqual [$cur getOutputBindInteger "2"] 1
$cur inputBind "1" 2
assertTrue [$cur executeQuery]
assertEqual [$cur getOutputBindInteger "2"] 2
$cur inputBind "1" 3
assertTrue [$cur executeQuery]
assertEqual [$cur getOutputBindInteger "2"] 3
assertTrue [$cur sendQuery "drop procedure testproc"]
puts ""


# reexecute
puts "REEXECUTE: "
$cur prepareQuery "select 1"
assertTrue [$cur executeQuery]
assertEqual [$cur rowCount] 1
assertEqual [$cur getFieldByIndex 0 0] "1"
puts ""
assertTrue [$cur executeQuery]
assertEqual [$cur rowCount] 1
assertEqual [$cur getFieldByIndex 0 0] "1"
puts ""
$cur prepareQuery "select cast(? as int)"
$cur inputBind "1" 1
assertTrue [$cur executeQuery]
assertEqual [$cur rowCount] 1
assertEqual [$cur getFieldByIndex 0 0] "1"
puts ""
assertTrue [$cur executeQuery]
assertEqual [$cur rowCount] 1
assertEqual [$cur getFieldByIndex 0 0] "1"
puts ""
$cur inputBind "1" 2
assertTrue [$cur executeQuery]
assertEqual [$cur rowCount] 1
assertEqual [$cur getFieldByIndex 0 0] "2"
puts ""


# stored procedure returning no value
puts "STORED PROCEDURE RETURNING NO VALUE: "
catch {$cur sendQuery "drop procedure testproc"}
assertTrue [$cur sendQuery "create procedure testproc @in1 int, @in2 float, @in3 varchar(20) as return"]
$cur prepareQuery "exec testproc ?,?,?"
$cur inputBind "1" 1
$cur inputBind "2" 2.5 2 1
$cur inputBind "3" "hello"
assertTrue [$cur executeQuery]
assertTrue [$cur sendQuery "drop procedure testproc"]
puts ""


# stored procedure returning single value
puts "STORED PROCEDURE RETURNING SINGLE VALUE: "
catch {$cur sendQuery "drop procedure testproc"}
assertTrue [$cur sendQuery "create procedure testproc @in1 int, @in2 float, @in3 varchar(20), @out1 int output as select @out1=@in1"]
$cur prepareQuery "exec testproc ?,?,?,?"
$cur inputBind "1" 1
$cur inputBind "2" 2.5 2 1
$cur inputBind "3" "hello"
$cur defineOutputBindInteger "4"
assertTrue [$cur executeQuery]
assertEqual [$cur getOutputBindInteger "4"] 1
assertTrue [$cur sendQuery "drop procedure testproc"]
puts ""


# stored procedure returning multiple values
puts "STORED PROCEDURE RETURNING MULTIPLE VALUES: "
catch {$cur sendQuery "drop procedure testproc"}
assertTrue [$cur sendQuery "create procedure testproc @in1 int, @in2 float, @in3 varchar(20), @out1 int output, @out2 float output, @out3 varchar(20) output as select @out1=@in1, @out2=@in2, @out3=@in3"]
$cur prepareQuery "exec testproc ?,?,?,?,?,?"
$cur inputBind "1" 1
$cur inputBind "2" 2.5 2 1
$cur inputBind "3" "hello"
$cur defineOutputBindInteger "4"
$cur defineOutputBindDouble "5"
$cur defineOutputBindString "6" 20
assertTrue [$cur executeQuery]
assertEqual [$cur getOutputBindInteger "4"] 1
assertEqual [$cur getOutputBindDouble "5"] 2.5
assertEqual [$cur getOutputBindString "6"] "hello"
assertTrue [$cur sendQuery "drop procedure testproc"]
puts ""


# stored procedure returning result set
puts "STORED PROCEDURE RETURNING RESULT SET: "
catch {$cur sendQuery "drop procedure testselectproc"}
assertTrue [$cur sendQuery "create procedure testselectproc as select 1 union select 2 union select 3 union select 4 union select 5 union select 6 union select 7 union select 8"]
assertTrue [$cur sendQuery "exec testselectproc"]
assertEqual [$cur rowCount] 8
assertTrue [$cur sendQuery "drop procedure testselectproc"]
puts ""


# temporary tables
puts "TEMPORARY TABLES: "
catch {$cur sendQuery "drop table #temptable"}
$cur sendQuery "create table #temptable (col1 int)"
assertTrue [$cur sendQuery "insert into #temptable values (1)"]
assertTrue [$cur sendQuery "select count(*) from #temptable"]
assertEqual [$cur getFieldByIndex 0 0] "1"
$con endSession
puts ""
assertTrue [catch {$cur sendQuery "select count(*) from #temptable"}]
puts ""


# encoded binary data
puts "ENCODED BINARY DATA: "
catch {$cur sendQuery "drop table testtable"}
assertTrue [$cur sendQuery "create table testtable (col1 image)"]
set buffer ""
for {set j 0} {$j < 256} {incr j} {
	append buffer [binary format c $j]
}
set hex [binary encode hex $buffer]
assertTrue [$cur sendQuery "insert into testtable values (0x$hex)"]
assertTrue [$cur sendQuery "select col1 from testtable"]
assertEqual [$cur getFieldLengthByIndex 0 0] 256
assertEqualLen [$cur getFieldByIndex 0 0] $buffer 256
assertTrue [$cur sendQuery "drop table testtable"]
puts ""


# quotes
puts "QUOTES: "
catch {$cur sendQuery "drop table testtable"}
assertTrue [$cur sendQuery "create table testtable (col1 varchar(4))"]
assertTrue [$cur sendQuery "insert into testtable values ('''''')"]
assertTrue [$cur sendQuery "select col1 from testtable"]
assertEqual [$cur getFieldLengthByIndex 0 0] 2
assertEqual [$cur getFieldByIndex 0 0] "''"
assertTrue [$cur sendQuery "drop table testtable"]
puts ""


# last insert id
puts "LAST INSERT ID: "
catch {$cur sendQuery "drop table testtable"}
assertTrue [$cur sendQuery "create table testtable (col1 int identity primary key, col2 int)"]
assertTrue [$cur sendQuery "insert into testtable (col2) values (1)"]
assertEqual [$con getLastInsertId] 1
assertTrue [$cur sendQuery "drop table testtable"]
puts ""


# database is schema
puts "DATABASE IS SCHEMA: "
assertFalse [$con getDatabaseIsSchema]
puts ""


# catalog list
puts "CATALOG LIST: "
assertTrue [$cur getCatalogList ""]
assertEqual [$cur getColumnName 0] "Database"
assertInResultSet $cur "Database" $hostname
puts ""


# schema list
puts "SCHEMA LIST: "
assertTrue [$cur getSchemaList ""]
assertEqual [$cur getColumnName 0] "Database"
# odbc lists INFORMATION_SCHEMA, sys and testuser - the schemas that
# own an object - rather than every schema, so dbo isn't there
assertInResultSet $cur "Database" "testuser"
puts ""


# table type list
puts "TABLE TYPE LIST: "
assertTrue [$cur getTableTypeList]
assertEqual [$cur getColumnName 0] "table_type"
assertInResultSet $cur "table_type" "TABLE"
puts ""


# table list
puts "TABLE LIST: "
catch {$cur sendQuery "drop table testtable1"}
catch {$cur sendQuery "drop table testtable2"}
catch {$cur sendQuery "drop table testtable3"}
catch {$cur sendQuery "drop table testtable4"}
assertTrue [$cur sendQuery "create table testtable1 (col1 int, col2 int)"]
assertTrue [$cur sendQuery "create table testtable2 (col1 int, col2 int)"]
assertTrue [$cur sendQuery "create table testtable3 (col1 int, col2 int)"]
assertTrue [$cur sendQuery "create table testtable4 (col1 int, col2 int)"]
assertTrue [$cur getTableList ""]
assertInResultSet $cur "Tables_in_xxx" "testtable1"
assertInResultSet $cur "Tables_in_xxx" "testtable2"
assertInResultSet $cur "Tables_in_xxx" "testtable3"
assertInResultSet $cur "Tables_in_xxx" "testtable4"
assertTrue [$cur sendQuery "drop table testtable1"]
assertTrue [$cur sendQuery "drop table testtable2"]
assertTrue [$cur sendQuery "drop table testtable3"]
assertTrue [$cur sendQuery "drop table testtable4"]
puts ""


# type info list
puts "TYPE INFO LIST: "
# the odbc module maps odbc type names, not sql server ones, so "int"
# and "datetime" both come back "Optional feature not implemented" -
# INTEGER and TIMESTAMP are the names to ask for.  the names it returns
# are the sql server ones, lowercased
assertTrue [$cur getTypeInfoList "INTEGER"]
assertEqual [$cur getColumnName 0] "type_name"
assertEqual [$cur getColumnName 1] "data_type"
assertEqual [$cur getColumnName 2] "precision"
assertEqual [$cur getColumnName 3] "literal_prefix"
assertEqual [$cur getColumnName 4] "literal_suffix"
assertEqual [$cur getColumnName 5] "create_params"
assertEqual [$cur getColumnName 6] "nullable"
assertEqual [$cur getColumnName 7] "case_sensitive"
assertEqual [$cur getColumnName 8] "searchable"
assertEqual [$cur getColumnName 9] "unsigned_attribute"
assertEqual [$cur getColumnName 10] "fixed_prec_scale"
assertEqual [$cur getColumnName 11] "auto_increment"
assertEqual [$cur getColumnName 12] "local_type_name"
assertEqual [$cur getColumnName 13] "minumum_scale"
assertEqual [$cur getColumnName 14] "maxiumm_scale"
assertEqual [$cur getColumnName 15] "sql_data_type"
assertEqual [$cur getColumnName 16] "sql_datetime_sub"
assertEqual [$cur getColumnName 17] "num_prec_radix"
assertEqual [$cur getColumnName 18] "interval_precision"
assertEqual [$cur getFieldByName 0 "type_name"] "int"
assertEqual [$cur getFieldByName 0 "data_type"] "4"
assertEqual [$cur getFieldByName 0 "precision"] "10"
assertEqual [$cur getFieldByName 0 "local_type_name"] "int"
assertTrue [$cur getTypeInfoList "CHAR"]
assertEqual [$cur getFieldByName 0 "type_name"] "char"
assertEqual [$cur getFieldByName 0 "data_type"] "1"
assertEqual [$cur getFieldByName 0 "precision"] "8000"
assertEqual [$cur getFieldByName 0 "local_type_name"] "char"
assertTrue [$cur getTypeInfoList "VARCHAR"]
assertEqual [$cur getFieldByName 0 "type_name"] "varchar"
assertEqual [$cur getFieldByName 0 "data_type"] "12"
assertEqual [$cur getFieldByName 0 "precision"] "8000"
assertEqual [$cur getFieldByName 0 "local_type_name"] "varchar"
# TIMESTAMP comes back as three rows - datetime2, datetime and
# smalldatetime, in that order - so datetime has to be searched for
# rather than read out of row 0
assertTrue [$cur getTypeInfoList "TIMESTAMP"]
assertInResultSet $cur "type_name" "datetime"
assertInResultSet $cur "type_name" "datetime2"
assertInResultSet $cur "type_name" "smalldatetime"
assertEqual [$cur getFieldByIndex 1 0] "datetime"
assertEqual [$cur getFieldByIndex 1 1] "93"
assertEqual [$cur getFieldByIndex 1 2] "23"
assertEqual [$cur getFieldByIndex 1 12] "datetime"
puts ""


# column list
puts "COLUMN LIST: "
catch {$cur sendQuery "drop table testtable"}
assertTrue [$cur sendQuery "create table testtable (testint int, testsmallint smallint, testtinyint tinyint, testreal real, testfloat float, testdecimal decimal(4,1), testnumeric numeric(4,1), testmoney money, testsmallmoney smallmoney, testdatetime datetime, testsmalldatetime smalldatetime, testchar char(40), testvarchar varchar(40), testbit bit, testdate date, testtime time, testdatetime2 datetime2)"]
assertTrue [$cur getColumnList "testtable" ""]
assertEqual [$cur getColumnName 0] "column_name"
assertEqual [$cur getColumnName 1] "data_type"
assertEqual [$cur getColumnName 2] "character_maximum_length"
assertEqual [$cur getColumnName 3] "numeric_precision"
assertEqual [$cur getColumnName 4] "numeric_scale"
assertEqual [$cur getColumnName 5] "is_nullable"
assertEqual [$cur getColumnName 6] "column_key"
assertEqual [$cur getColumnName 7] "column_default"
assertEqual [$cur getColumnName 8] "extra"
assertEqual [$cur getFieldByName 0 "column_name"] "testint"
assertEqual [$cur getFieldByName 1 "column_name"] "testsmallint"
assertEqual [$cur getFieldByName 2 "column_name"] "testtinyint"
assertEqual [$cur getFieldByName 3 "column_name"] "testreal"
assertEqual [$cur getFieldByName 4 "column_name"] "testfloat"
assertEqual [$cur getFieldByName 5 "column_name"] "testdecimal"
assertEqual [$cur getFieldByName 6 "column_name"] "testnumeric"
assertEqual [$cur getFieldByName 7 "column_name"] "testmoney"
assertEqual [$cur getFieldByName 8 "column_name"] "testsmallmoney"
assertEqual [$cur getFieldByName 9 "column_name"] "testdatetime"
assertEqual [$cur getFieldByName 10 "column_name"] "testsmalldatetime"
assertEqual [$cur getFieldByName 11 "column_name"] "testchar"
assertEqual [$cur getFieldByName 12 "column_name"] "testvarchar"
assertEqual [$cur getFieldByName 13 "column_name"] "testbit"
assertEqual [$cur getFieldByName 14 "column_name"] "testdate"
assertEqual [$cur getFieldByName 15 "column_name"] "testtime"
assertEqual [$cur getFieldByName 16 "column_name"] "testdatetime2"
assertEqual [$cur getFieldByName 0 "data_type"] "int"
assertEqual [$cur getFieldByName 1 "data_type"] "smallint"
assertEqual [$cur getFieldByName 2 "data_type"] "tinyint"
assertEqual [$cur getFieldByName 3 "data_type"] "real"
assertEqual [$cur getFieldByName 4 "data_type"] "float"
assertEqual [$cur getFieldByName 5 "data_type"] "decimal"
assertEqual [$cur getFieldByName 6 "data_type"] "numeric"
assertEqual [$cur getFieldByName 7 "data_type"] "money"
assertEqual [$cur getFieldByName 8 "data_type"] "smallmoney"
assertEqual [$cur getFieldByName 9 "data_type"] "datetime"
assertEqual [$cur getFieldByName 10 "data_type"] "smalldatetime"
assertEqual [$cur getFieldByName 11 "data_type"] "char"
assertEqual [$cur getFieldByName 12 "data_type"] "varchar"
assertEqual [$cur getFieldByName 13 "data_type"] "bit"
assertEqual [$cur getFieldByName 14 "data_type"] "date"
assertEqual [$cur getFieldByName 15 "data_type"] "time"
assertEqual [$cur getFieldByName 16 "data_type"] "datetime2"
assertTrue [$cur sendQuery "drop table testtable"]
puts ""


# column list - auto_increment, primary key
puts "COLUMN LIST - auto_increment, primary key: "
catch {$cur sendQuery "drop table testtable"}
assertTrue [$cur sendQuery "create table testtable (col1 int identity primary key, col2 int)"]
assertTrue [$cur getColumnList "testtable" ""]
assertEqual [$cur getFieldByName 0 "extra"] "auto_increment"
assertEqual [$cur getFieldByName 0 "column_key"] "PRI"
assertEqual [$cur getFieldByName 1 "extra"] ""
assertEqual [$cur getFieldByName 1 "column_key"] ""
puts ""
assertTrue [$cur sendQuery "drop table testtable"]
assertTrue [$cur sendQuery "create table testtable (col1 int primary key, col2 int)"]
assertTrue [$cur getColumnList "testtable" ""]
assertEqual [$cur getFieldByName 0 "extra"] ""
assertEqual [$cur getFieldByName 0 "column_key"] "PRI"
assertTrue [$cur sendQuery "drop table testtable"]
puts ""


# primary keys list
puts "PRIMARY KEYS LIST: "
catch {$cur sendQuery "drop table testtable"}
assertTrue [$cur sendQuery "create table testtable (col1 int primary key, col2 int)"]
assertTrue [$cur getPrimaryKeysList "testtable" ""]
assertEqual [$cur getColumnName 0] "table"
assertEqual [$cur getColumnName 1] "non_unique"
assertEqual [$cur getColumnName 2] "key_name"
assertEqual [$cur getColumnName 3] "seq_in_index"
assertEqual [$cur getColumnName 4] "column_name"
assertEqual [$cur getColumnName 5] "collation"
assertEqual [$cur getColumnName 6] "cardinality"
assertEqual [$cur getColumnName 7] "sub_part"
assertEqual [$cur getColumnName 8] "packed"
assertEqual [$cur getColumnName 9] "null"
assertEqual [$cur getColumnName 10] "index_type"
assertEqual [$cur getColumnName 11] "comment"
assertEqual [$cur getColumnName 12] "index_comment"
assertEqual [$cur rowCount] 1
assertEqual [$cur getFieldByName 0 "table"] "testtable"
assertEqual [$cur getFieldByName 0 "seq_in_index"] "1"
assertEqual [$cur getFieldByName 0 "column_name"] "col1"
# mssql auto-names an unnamed primary key constraint
# PK__<table name, truncated to 8 chars>__<hex>, and the hex is
# generated per creation, so only the prefix is stable
assertStartsWith [$cur getFieldByName 0 "key_name"] "PK__testtabl__"
assertTrue [$cur sendQuery "drop table testtable"]
puts ""


# key and index list
puts "KEY AND INDEX LIST: "
catch {$cur sendQuery "drop table testtable"}
assertTrue [$cur sendQuery "create table testtable (col1 int primary key, col2 int)"]
assertTrue [$cur getKeyAndIndexList "testtable" ""]
assertEqual [$cur getColumnName 0] "table"
assertEqual [$cur getColumnName 1] "non_unique"
assertEqual [$cur getColumnName 2] "key_name"
assertEqual [$cur getColumnName 3] "seq_in_index"
assertEqual [$cur getColumnName 4] "column_name"
assertEqual [$cur getColumnName 5] "collation"
assertEqual [$cur getColumnName 6] "cardinality"
assertEqual [$cur getColumnName 7] "sub_part"
assertEqual [$cur getColumnName 8] "packed"
assertEqual [$cur getColumnName 9] "null"
assertEqual [$cur getColumnName 10] "index_type"
assertEqual [$cur getColumnName 11] "comment"
assertEqual [$cur getColumnName 12] "index_comment"
# the odbc module emits a leading SQL_TABLE_STAT row - table statistics
# rather than an index - so the index itself is row 1
assertEqual [$cur rowCount] 2
assertEqual [$cur getFieldByName 0 "table"] "testtable"
assertEqual [$cur getFieldByName 0 "key_name"] ""
assertEqual [$cur getFieldByName 0 "cardinality"] "0"
assertEqual [$cur getFieldByName 0 "index_type"] "0"
assertEqual [$cur getFieldByName 1 "table"] "testtable"
assertEqual [$cur getFieldByName 1 "non_unique"] "0"
assertEqual [$cur getFieldByName 1 "seq_in_index"] "1"
assertEqual [$cur getFieldByName 1 "column_name"] "col1"
assertEqual [$cur getFieldByName 1 "collation"] "A"
assertEqual [$cur getFieldByName 1 "cardinality"] "0"
assertEqual [$cur getFieldByName 1 "index_type"] "1"
# mssql auto-names an unnamed primary key constraint
# PK__<table name, truncated to 8 chars>__<hex>, and the hex is
# generated per creation, so only the prefix is stable
assertStartsWith [$cur getFieldByName 1 "key_name"] "PK__testtabl__"
assertTrue [$cur sendQuery "drop table testtable"]
puts ""


# procedure list
puts "PROCEDURE LIST: "
catch {$cur sendQuery "drop procedure testproc1"}
catch {$cur sendQuery "drop procedure testproc2"}
catch {$cur sendQuery "drop procedure testproc3"}
catch {$cur sendQuery "drop procedure testproc4"}
assertTrue [$cur sendQuery "create procedure testproc1 @in1 int, @in2 char(20), @in3 varchar(20), @in4 datetime as select 1"]
assertTrue [$cur sendQuery "create procedure testproc2 @in1 int, @in2 char(20), @in3 varchar(20), @in4 datetime as select 1"]
assertTrue [$cur sendQuery "create procedure testproc3 @in1 int, @in2 char(20), @in3 varchar(20), @in4 datetime as select 1"]
assertTrue [$cur sendQuery "create procedure testproc4 @in1 int, @in2 char(20), @in3 varchar(20), @in4 datetime as select 1"]
assertTrue [$cur getProcedureList ""]
# odbc reports the procedure group number too - mssql lets several
# procedures share a name, distinguished by the number after the
# semicolon, and an ungrouped procedure is number 1
assertInResultSet $cur "routine_name" "testproc1;1"
assertInResultSet $cur "routine_name" "testproc2;1"
assertInResultSet $cur "routine_name" "testproc3;1"
assertInResultSet $cur "routine_name" "testproc4;1"
puts ""


# procedure parameter list
puts "PROCEDURE PARAMETER LIST: "
assertTrue [$cur getProcedureParameterList "testproc1" ""]
assertEqual [$cur getColumnName 0] "parameter_name"
assertEqual [$cur getColumnName 1] "parameter_mode"
assertEqual [$cur getColumnName 2] "data_type"
assertEqual [$cur getColumnName 3] "character_maximum_length"
assertEqual [$cur getColumnName 4] "ordinal_position"
assertEqual [$cur rowCount] 4
assertEqual [$cur getFieldByName 0 "parameter_name"] "@in1"
assertEqual [$cur getFieldByName 0 "parameter_mode"] "1"
assertEqual [$cur getFieldByName 0 "data_type"] "int"
assertEqual [$cur getFieldByName 0 "ordinal_position"] "1"
assertEqual [$cur getFieldByName 1 "parameter_name"] "@in2"
assertEqual [$cur getFieldByName 1 "parameter_mode"] "1"
assertEqual [$cur getFieldByName 1 "data_type"] "char"
assertEqual [$cur getFieldByName 1 "ordinal_position"] "2"
assertEqual [$cur getFieldByName 2 "parameter_name"] "@in3"
assertEqual [$cur getFieldByName 2 "parameter_mode"] "1"
assertEqual [$cur getFieldByName 2 "data_type"] "varchar"
assertEqual [$cur getFieldByName 2 "ordinal_position"] "3"
assertEqual [$cur getFieldByName 3 "parameter_name"] "@in4"
assertEqual [$cur getFieldByName 3 "parameter_mode"] "1"
assertEqual [$cur getFieldByName 3 "data_type"] "datetime"
assertEqual [$cur getFieldByName 3 "ordinal_position"] "4"
assertTrue [$cur sendQuery "drop procedure testproc1"]
assertTrue [$cur sendQuery "drop procedure testproc2"]
assertTrue [$cur sendQuery "drop procedure testproc3"]
assertTrue [$cur sendQuery "drop procedure testproc4"]
puts ""


# invalid queries
puts "INVALID QUERIES: "
assertTrue [catch {$cur sendQuery "select * from testtable order by testint"}]
assertTrue [catch {$cur sendQuery "select * from testtable order by testint"}]
assertTrue [catch {$cur sendQuery "select * from testtable order by testint"}]
assertTrue [catch {$cur sendQuery "select * from testtable order by testint"}]
puts ""
assertTrue [catch {$cur sendQuery "insert into testtable values (1,2,3,4)"}]
assertTrue [catch {$cur sendQuery "insert into testtable values (1,2,3,4)"}]
assertTrue [catch {$cur sendQuery "insert into testtable values (1,2,3,4)"}]
assertTrue [catch {$cur sendQuery "insert into testtable values (1,2,3,4)"}]
puts ""
assertTrue [catch {$cur sendQuery "create table testtable"}]
assertTrue [catch {$cur sendQuery "create table testtable"}]
assertTrue [catch {$cur sendQuery "create table testtable"}]
assertTrue [catch {$cur sendQuery "create table testtable"}]
puts ""

reportTestStatus

exit $status
