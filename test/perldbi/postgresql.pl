#! /usr/bin/env perl

# Copyright (c) David Muse
# See the file COPYING for more information.

use DBI;

require "./asserts.pl";


# instantiation
# autocommit on, so an expected error doesn't leave postgresql's
# transaction aborted, with every statement after it failing with 25P02
my $prefix="DBI:SQLRelay(AutoCommit=>1,PrintError=>0):";
my $connectstring="host=sqlrelay;port=9003;socket=/tmp/postgresql.socket;debug=0";
my $dsn=$prefix.$connectstring;


# connect
print("CONNECT: \n");
my $dbh=DBI->connect($dsn,"testuser","testpassword") or die DBI->errstr;
assertDefined($dbh);
print("\n");


# drop existing table
$dbh->do("drop table testtable");


# error sqlstate - database handle
print("ERROR SQLSTATE - DATABASE HANDLE: \n");
assertEqualString($dbh->do("create table testtable (col1 int)"),"0E0");
assertEqualString($dbh->state,"");
# postgresql reports a duplicate table as 42P07, in every locale
assertEqual($dbh->do("create table testtable (col1 int)"),0);
assertEqualString($dbh->state,"42P07");
assertEqual($dbh->do("select * from nonexistenttable"),0);
assertEqualString($dbh->state,"42P01");
print("\n");


# error sqlstate - statement handle
print("ERROR SQLSTATE - STATEMENT HANDLE: \n");
my $sth=$dbh->prepare("create table testtable (col1 int)");
assertDefined($sth);
assertEqual($sth->execute(),0);
assertEqualString($sth->state,"42P07");
$sth->finish();
print("\n");


# clean up
assertEqualString($dbh->do("drop table testtable"),"0E0");
assertEqualString($dbh->state,"");


$dbh->disconnect();

reportTestStatus();

exit($status);
