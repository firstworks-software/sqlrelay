#! /usr/bin/env perl

# Copyright (c) David Muse
# See the file COPYING for more information.

use DBI;
use DBI::Const::GetInfoType;
use Data::Dumper;

require "./asserts.pl";


# instantiation
my $prefix="DBI:SQLRelay(AutoCommit=>0,PrintError=>0):";
my $connectstring="host=sqlrelay;port=9000;socket=/tmp/test.socket;debug=0";
my $dsn=$prefix.$connectstring;

# parse dsn
if ($DBI::VERSION>=1.43) {


	# parse dsn
	print("PARSE DSN: \n");
	my ($scheme,$driver,$attr_string,$attr_hash,$driver_dsn)=DBI->parse_dsn($dsn);
	assertEqualString($scheme,"dbi");
	assertEqualString($driver,"SQLRelay");
	assertEqualString($attr_string,"AutoCommit=>0,PrintError=>0");
	assertEqual($attr_hash->{AutoCommit},0);
	assertEqual($attr_hash->{PrintError},0);
	assertEqualString($driver_dsn,$connectstring);
	print("\n");
}


# connect
print("CONNECT: \n");
my $dbh=DBI->connect($dsn,"testuser","testpassword") or die DBI->errstr;
assertEqualString($dbh->{Type},"db");
if ($DBI::VERSION>=1.40) {
	assertEqualString($dbh->{Username},"testuser");
}
assertDefined($dbh);
$dbh->disconnect();
$ENV{"DBI_DSN"}=$dsn;
my $dbh=DBI->connect(undef,"testuser","testpassword",{AutoCommit=>0,PrintError=>0}) or die DBI->errstr;
assertDefined($dbh);
assertEqualString($dbh->{Name},$connectstring);
print("\n");


# ping
print("PING: \n");
assertTrue($dbh->ping());
print("\n");

# drop existing table
$dbh->do("drop table testtable");


# create temptable
print("CREATE TEMPTABLE: \n");
if ($DBI::VERSION>=1.41) {
	$dbh->{Executed}=0;
}
my $stmt="create table testtable (testnumber number not null, testchar char(40), testvarchar varchar2(40), testdate date)";
assertEqualString($dbh->do($stmt),"0E0");
if ($DBI::VERSION>=1.41) {
	assertEqual($dbh->{Executed},1);
}
assertEqualString($dbh->{Statement},$stmt);
print("\n");


# insert and affected rows
print("INSERT and AFFECTED ROWS: \n");
assertTrue($dbh->do("insert into testtable values (1,'testchar1','testvarchar1','01-JAN-2001')"));
print("\n");


# do with bind values
print("DO WITH BIND VALUES: \n");
assertTrue($dbh->do("insert into testtable values (:var1,:var2,:var3,:var4)",undef,(2,"testchar2","testvarchar2","01-JAN-2002")));
print("\n");


# execute with bind values
print("EXECUTE WITH BIND VALUES: \n");
if ($DBI::VERSION>=1.41) {
	$dbh->{Executed}=0;
}
$stmt="insert into testtable values (:var1,:var2,:var3,:var4)";
my $sth=$dbh->prepare($stmt);
assertEqualString($sth->{Type},"st");
assertEqualString($sth->{Statement},$stmt);
assertEqual($dbh->{Kids},1);
assertEqual($dbh->{ActiveKids},0);
assertEqual($sth->{Active},0);
assertTrue($sth->execute(3,"testchar3","testvarchar3","01-JAN-2003"));
assertEqual($sth->{Active},1);
assertEqual($dbh->{ActiveKids},1);
if ($DBI::VERSION>1.41) {
	assertEqual($sth->{Executed},1);
	assertEqual($dbh->{Executed},1);
}
print("\n");


# affected rows
print("AFFECTED ROWS: \n");
assertEqual($sth->rows(),1);
print("\n");


# bind param by position
print("BIND PARAM BY POSITION: \n");
$sth->bind_param(1,4,SQL_INTEGER);
$sth->bind_param(2,"testchar4",SQL_CHAR);
$sth->bind_param(3,"testvarchar4",{type=>SQL_VARCHAR,length=>12});
$sth->bind_param(4,"01-JAN-2004",{type=>SQL_DATETIME});
assertEqual($sth->{ParamValues}->{1},4);
assertEqual($sth->{ParamValues}->{2},"testchar4");
assertEqual($sth->{ParamValues}->{3},"testvarchar4");
assertEqual($sth->{ParamValues}->{4},"01-JAN-2004");
assertEqual($sth->{ParamTypes}->{"var1"},"SQL_INTEGER");
assertEqual($sth->{ParamTypes}->{"var2"},"SQL_CHAR");
assertEqual($sth->{ParamTypes}->{"var3"},"SQL_VARCHAR");
assertEqual($sth->{ParamTypes}->{"var4"},"SQL_DATETIME");
assertTrue($sth->execute());
print("\n");


# param count
print("PARAM COUNT: \n");
assertEqual($sth->{NUM_OF_PARAMS},4);
print("\n");

if ($DBI::VERSION>=1.22) {


	# execute array
	print("EXECUTE ARRAY: \n");
	@var1s=(5,6);
	@var2s=("testchar5","testchar6");
	@var3s=("testvarchar5","testvarchar6");
	@var4s=("01-JAN-2005","01-JAN-2006");
	if ($DBI::VERSION>=1.41) {
		$dbh->{Executed}=0;
	}
	my ($tuples,$rows)=$sth->execute_array({ ArrayTupleStatus=>\my @tuple_status },\@var1s,\@var2s,\@var3s,\@var4s);
	if ($DBI::VERSION>=1.41) {
		assertEqual($sth->{Executed},1);
		assertEqual($dbh->{Executed},1);
	}
	assertEqual($tuples,2);
	if ($DBI::VERSION>=1.60) {
		assertEqual($rows,2);
	}
	for (my $index=0; $index<2; $index++) {
		assertEqual(@tuple_status[$index],1);
	}
	assertEqual($sth->{ParamArrays}->{1}->[0],5);
	assertEqual($sth->{ParamArrays}->{1}->[1],6);
	assertEqual($sth->{ParamArrays}->{2}->[0],"testchar5");
	assertEqual($sth->{ParamArrays}->{2}->[1],"testchar6");
	assertEqual($sth->{ParamArrays}->{3}->[0],"testvarchar5");
	assertEqual($sth->{ParamArrays}->{3}->[1],"testvarchar6");
	assertEqual($sth->{ParamArrays}->{4}->[0],"01-JAN-2005");
	assertEqual($sth->{ParamArrays}->{4}->[1],"01-JAN-2006");
	print("\n");


	# bind param array
	print("BIND PARAM ARRAY: \n");
	$sth->bind_param_array(1,[7,8]);
	$sth->bind_param_array(2,["testchar7","testchar8"]);
	$sth->bind_param_array(3,["testvarchar7","testvarchar8"]);
	$sth->bind_param_array(4,["01-JAN-2007","01-JAN-2008"]);
	my ($tuples,$rows)=$sth->execute_array({ ArrayTupleStatus=>\my @tuple_status });
	assertEqual($tuples,2);
	if ($DBI::VERSION>=1.60) {
		assertEqual($rows,2);
	}
	for (my $index=0; $index<2; $index++) {
		assertEqual(@tuple_status[$index],1);
	}
	print("\n");
} else {
	$dbh->do("insert into testtable values (5,'testchar5','testvarchar5','01-JAN-2005')");
	$dbh->do("insert into testtable values (6,'testchar6','testvarchar6','01-JAN-2006')");
	$dbh->do("insert into testtable values (7,'testchar7','testvarchar7','01-JAN-2007')");
	$dbh->do("insert into testtable values (8,'testchar8','testvarchar8','01-JAN-2008')");
}


# bind by name
print("BIND BY NAME: \n");
$sth->bind_param("var1",9);
$sth->bind_param("var2","testchar9");
# should work if leading delimiters are included too
$sth->bind_param(":var3","testvarchar9");
$sth->bind_param(":var4","01-JAN-2009");
assertEqual($sth->{ParamValues}->{"var1"},9);
assertEqual($sth->{ParamValues}->{"var2"},"testchar9");
assertEqual($sth->{ParamValues}->{":var3"},"testvarchar9");
assertEqual($sth->{ParamValues}->{":var4"},"01-JAN-2009");
assertEqual($sth->{ParamTypes}->{"var1"},"SQL_VARCHAR");
assertEqual($sth->{ParamTypes}->{"var2"},"SQL_VARCHAR");
assertEqual($sth->{ParamTypes}->{":var3"},"SQL_VARCHAR");
assertEqual($sth->{ParamTypes}->{":var4"},"SQL_VARCHAR");
assertTrue($sth->execute());
print("\n");


# output bind by name
print("OUTPUT BIND BY NAME: \n");
$sth=$dbh->prepare("begin  :numvar:=1; :stringvar:='hello'; :floatvar:=2.5; end;");
$sth->bind_param_inout("numvar",\$numvar,10);
$sth->bind_param_inout("stringvar",\$stringvar,10);
# should work if leading delimiters are included too
$sth->bind_param_inout(":floatvar",\$floatvar,10);
assertTrue($sth->execute());
assertEqualString($numvar,'1');
assertEqualString($stringvar,'hello');
assertEqualString($floatvar,'2.5');
print("\n");


# output bind by position
print("OUTPUT BIND BY POSITION: \n");
$sth->bind_param_inout(1,\$numvar,10);
$sth->bind_param_inout(2,\$stringvar,10);
$sth->bind_param_inout(3,\$floatvar,10);
assertTrue($sth->execute());
assertEqualString($numvar,'1');
assertEqualString($stringvar,'hello');
assertEqualString($floatvar,'2.5');
print("\n");


# select
print("SELECT: \n");
$sth=$dbh->prepare("select * from testtable order by testnumber");
assertEqualString($sth->execute(),"0E0");
print("\n");


# column count
print("COLUMN COUNT: \n");
assertEqual($sth->{NUM_OF_FIELDS},4);
print("\n");


# column names
print("COLUMN NAMES: \n");
assertEqualString($sth->{NAME}->[0],"TESTNUMBER");
assertEqualString($sth->{NAME}->[1],"TESTCHAR");
assertEqualString($sth->{NAME}->[2],"TESTVARCHAR");
assertEqualString($sth->{NAME}->[3],"TESTDATE");
print("\n");


# column names (lc)
print("COLUMN NAMES (lc): \n");
assertEqualString($sth->{NAME_lc}->[0],"testnumber");
assertEqualString($sth->{NAME_lc}->[1],"testchar");
assertEqualString($sth->{NAME_lc}->[2],"testvarchar");
assertEqualString($sth->{NAME_lc}->[3],"testdate");
print("\n");


# column names (uc)
print("COLUMN NAMES (uc): \n");
assertEqualString($sth->{NAME_uc}->[0],"TESTNUMBER");
assertEqualString($sth->{NAME_uc}->[1],"TESTCHAR");
assertEqualString($sth->{NAME_uc}->[2],"TESTVARCHAR");
assertEqualString($sth->{NAME_uc}->[3],"TESTDATE");
print("\n");


# column types
print("COLUMN TYPES: \n");
assertEqualString($sth->{TYPE}->[0],"NUMBER");
assertEqualString($sth->{TYPE}->[1],"CHAR");
assertEqualString($sth->{TYPE}->[2],"VARCHAR2");
assertEqualString($sth->{TYPE}->[3],"DATE");
print("\n");


# column indices from name_hash
print("COLUMN INDICES FROM NAME_hash: \n");
assertEqualString($sth->{NAME_hash}->{TESTNUMBER},0);
assertEqualString($sth->{NAME_hash}->{TESTCHAR},1);
assertEqualString($sth->{NAME_hash}->{TESTVARCHAR},2);
assertEqualString($sth->{NAME_hash}->{TESTDATE},3);
print("\n");


# column indices from name_lc_hash
print("COLUMN INDICES FROM NAME_lc_hash: \n");
assertEqualString($sth->{NAME_lc_hash}->{testnumber},0);
assertEqualString($sth->{NAME_lc_hash}->{testchar},1);
assertEqualString($sth->{NAME_lc_hash}->{testvarchar},2);
assertEqualString($sth->{NAME_lc_hash}->{testdate},3);
print("\n");


# column indices from name_uc_hash
print("COLUMN INDICES FROM NAME_uc_hash: \n");
assertEqualString($sth->{NAME_uc_hash}->{TESTNUMBER},0);
assertEqualString($sth->{NAME_uc_hash}->{TESTCHAR},1);
assertEqualString($sth->{NAME_uc_hash}->{TESTVARCHAR},2);
assertEqualString($sth->{NAME_uc_hash}->{TESTDATE},3);
print("\n");


# precision
print("PRECISION: \n");
assertEqualString($sth->{PRECISION}->[0],0);
assertEqualString($sth->{PRECISION}->[1],0);
assertEqualString($sth->{PRECISION}->[2],0);
assertEqualString($sth->{PRECISION}->[3],0);
print("\n");


# scale
print("SCALE: \n");
assertEqualString($sth->{SCALE}->[0],129);
assertEqualString($sth->{SCALE}->[1],0);
assertEqualString($sth->{SCALE}->[2],0);
assertEqualString($sth->{SCALE}->[3],0);
print("\n");


# nullable
print("NULLABLE: \n");
assertEqualString($sth->{NULLABLE}->[0],0);
assertEqualString($sth->{NULLABLE}->[1],1);
assertEqualString($sth->{NULLABLE}->[2],1);
assertEqualString($sth->{NULLABLE}->[3],1);
print("\n");

#print("TYPE INFO ALL: \n");
#print("\n");

#print("TYPE INFO: \n");
#print("\n");


# fetch
print("FETCH: \n");
$fieldsref=$sth->fetch;
assertEqual($$fieldsref[0],1);
assertEqualString($$fieldsref[1],"testchar1                               ");
assertEqualString($$fieldsref[2],"testvarchar1");
assertEqualString($$fieldsref[3],"01-JAN-01");
print("\n");


# fields by arrayref
print("FIELDS BY ARRAYREF: \n");
$fieldsref=$sth->fetchrow_arrayref;
assertEqual($$fieldsref[0],2);
assertEqualString($$fieldsref[1],"testchar2                               ");
assertEqualString($$fieldsref[2],"testvarchar2");
assertEqualString($$fieldsref[3],"01-JAN-02");
print("\n");


# fields by array
print("FIELDS BY ARRAY: \n");
@fields=$sth->fetchrow_array;
assertEqual($fields[0],3);
assertEqualString($fields[1],"testchar3                               ");
assertEqualString($fields[2],"testvarchar3");
assertEqualString($fields[3],"01-JAN-03");
print("\n");


# fields by hash
print("FIELDS BY HASH: \n");
$fieldshashref=$sth->fetchrow_hashref;
assertEqual($$fieldshashref{"TESTNUMBER"},4);
assertEqualString($$fieldshashref{"TESTCHAR"},"testchar4                               ");
assertEqualString($$fieldshashref{"TESTVARCHAR"},"testvarchar4");
assertEqualString($$fieldshashref{"TESTDATE"},"01-JAN-04");
print("\n");


# fetchall_arrayref
print("FETCHALL_ARRAYREF: \n");
$sth=$dbh->prepare("select * from testtable order by testnumber");
assertEqualString($sth->execute(),"0E0");
$rows=$sth->fetchall_arrayref();
assertEqual($$rows[0][0],1);
assertEqualString($$rows[0][1],"testchar1                               ");
assertEqualString($$rows[0][2],"testvarchar1");
assertEqualString($$rows[0][3],"01-JAN-01");
assertEqual($$rows[6][0],7);
assertEqualString($$rows[6][1],"testchar7                               ");
assertEqualString($$rows[6][2],"testvarchar7");
assertEqualString($$rows[6][3],"01-JAN-07");
print("\n");
$sth=$dbh->prepare("select * from testtable order by testnumber");
assertEqualString($sth->execute(),"0E0");
$rows=$sth->fetchall_arrayref([2,3]);
assertEqualString($$rows[0][0],"testvarchar1");
assertEqualString($$rows[0][1],"01-JAN-01");
assertEqualString($$rows[6][0],"testvarchar7");
assertEqualString($$rows[6][1],"01-JAN-07");
print("\n");
$sth=$dbh->prepare("select * from testtable order by testnumber");
assertEqualString($sth->execute(),"0E0");
$rows=$sth->fetchall_arrayref([2,3],1);
assertEqualString($$rows[0][0],"testvarchar1");
assertEqualString($$rows[0][1],"01-JAN-01");
assertUndef($$rows[1][0]);
assertUndef($$rows[1][1]);
print("\n");


# fetchall_hashref
print("FETCHALL_HASHREF: \n");
$sth=$dbh->prepare("select * from testtable order by testnumber");
assertEqualString($sth->execute(),"0E0");
$rows=$sth->fetchall_hashref("TESTNUMBER");
assertEqualString($$rows{1}->{TESTCHAR},"testchar1                               ");
assertEqualString($$rows{1}->{TESTVARCHAR},"testvarchar1");
assertEqualString($$rows{1}->{TESTDATE},"01-JAN-01");
assertEqualString($$rows{7}->{TESTCHAR},"testchar7                               ");
assertEqualString($$rows{7}->{TESTVARCHAR},"testvarchar7");
assertEqualString($$rows{7}->{TESTDATE},"01-JAN-07");
print("\n");
$sth=$dbh->prepare("select * from testtable order by testnumber");
assertEqualString($sth->execute(),"0E0");
$rows=$sth->fetchall_hashref(1);
assertEqualString($$rows{1}->{TESTCHAR},"testchar1                               ");
assertEqualString($$rows{1}->{TESTVARCHAR},"testvarchar1");
assertEqualString($$rows{1}->{TESTDATE},"01-JAN-01");
assertEqualString($$rows{7}->{TESTCHAR},"testchar7                               ");
assertEqualString($$rows{7}->{TESTVARCHAR},"testvarchar7");
assertEqualString($$rows{7}->{TESTDATE},"01-JAN-07");
print("\n");


# selectrow_array
print("SELECTROW_ARRAY: \n");
@row=$dbh->selectrow_array("select * from testtable order by testnumber");
assertEqual($row[0],1);
assertEqualString($row[1],"testchar1                               ");
assertEqualString($row[2],"testvarchar1");
assertEqualString($row[3],"01-JAN-01");
print("\n");


# selectrow_arrayref
print("SELECTROW_ARRAYREF: \n");
$row=$dbh->selectrow_arrayref("select * from testtable order by testnumber");
assertEqual($$row[0],1);
assertEqualString($$row[1],"testchar1                               ");
assertEqualString($$row[2],"testvarchar1");
assertEqualString($$row[3],"01-JAN-01");
print("\n");


# selectrow_hashref
print("SELECTROW_HASHREF: \n");
$row=$dbh->selectrow_hashref("select * from testtable order by testnumber");
assertEqual($$row{TESTNUMBER},1);
assertEqualString($$row{TESTCHAR},"testchar1                               ");
assertEqualString($$row{TESTVARCHAR},"testvarchar1");
assertEqualString($$row{TESTDATE},"01-JAN-01");
print("\n");


# selectall_arrayref
print("SELECTALL_ARRAYREF: \n");
$rows=$dbh->selectall_arrayref("select * from testtable order by testnumber");
assertEqual($$rows[0][0],1);
assertEqualString($$rows[0][1],"testchar1                               ");
assertEqualString($$rows[0][2],"testvarchar1");
assertEqualString($$rows[0][3],"01-JAN-01");
assertEqual($$rows[6][0],7);
assertEqualString($$rows[6][1],"testchar7                               ");
assertEqualString($$rows[6][2],"testvarchar7");
assertEqualString($$rows[6][3],"01-JAN-07");
print("\n");
$sth=$dbh->prepare("select * from testtable order by testnumber");
$rows=$dbh->selectall_arrayref($sth);
assertEqual($$rows[0][0],1);
assertEqualString($$rows[0][1],"testchar1                               ");
assertEqualString($$rows[0][2],"testvarchar1");
assertEqualString($$rows[0][3],"01-JAN-01");
assertEqual($$rows[6][0],7);
assertEqualString($$rows[6][1],"testchar7                               ");
assertEqualString($$rows[6][2],"testvarchar7");
assertEqualString($$rows[6][3],"01-JAN-07");
print("\n");
$rows=$dbh->selectall_arrayref($sth,{Slice=>[2,3]});
assertEqualString($$rows[0][0],"testvarchar1");
assertEqualString($$rows[0][1],"01-JAN-01");
assertEqualString($$rows[6][0],"testvarchar7");
assertEqualString($$rows[6][1],"01-JAN-07");
print("\n");
$rows=$dbh->selectall_arrayref($sth,{Slice=>[2,3],MaxRows=>1});
assertEqualString($$rows[0][0],"testvarchar1");
assertEqualString($$rows[0][1],"01-JAN-01");
assertUndef($$rows[6][0]);
assertUndef($$rows[6][1]);
print("\n");
$rows=$dbh->selectall_arrayref($sth,{Slice=>{}});
assertEqualString($$rows[0]{TESTVARCHAR},"testvarchar1");
assertEqualString($$rows[0]{TESTDATE},"01-JAN-01");
assertEqualString($$rows[6]{TESTVARCHAR},"testvarchar7");
assertEqualString($$rows[6]{TESTDATE},"01-JAN-07");
@rows=@{$dbh->selectall_arrayref($sth)};
assertEqualString($rows[0][0],"1");
assertEqualString($rows[0][1],"testchar1                               ");
assertEqualString($rows[6][0],"7");
assertEqualString($rows[6][1],"testchar7                               ");
print("\n");
$rows=$dbh->selectall_arrayref("select * from testtable where testnumber=:var1 or testnumber=:var2 order by testnumber",undef,('1','2'));
assertEqualString($rows[0][0],"1");
assertEqualString($rows[0][1],"testchar1                               ");
assertEqualString($rows[1][0],"2");
assertEqualString($rows[1][1],"testchar2                               ");
assertUndef($$rows[6][0]);
assertUndef($$rows[6][1]);
$rows=$dbh->selectall_arrayref("select * from testtable where testnumber=:var1 or testnumber=:var2 order by testnumber",undef,'1','2');
assertEqualString($rows[0][0],"1");
assertEqualString($rows[0][1],"testchar1                               ");
assertEqualString($rows[1][0],"2");
assertEqualString($rows[1][1],"testchar2                               ");
assertUndef($$rows[6][0]);
assertUndef($$rows[6][1]);
print("\n");


# selectall_hashref
print("SELECTALL_HASHREF: \n");
$rows=$dbh->selectall_hashref("select * from testtable order by testnumber","TESTNUMBER");
assertEqualString($$rows{1}->{TESTCHAR},"testchar1                               ");
assertEqualString($$rows{1}->{TESTVARCHAR},"testvarchar1");
assertEqualString($$rows{1}->{TESTDATE},"01-JAN-01");
assertEqualString($$rows{7}->{TESTCHAR},"testchar7                               ");
assertEqualString($$rows{7}->{TESTVARCHAR},"testvarchar7");
assertEqualString($$rows{7}->{TESTDATE},"01-JAN-07");
print("\n");
$rows=$dbh->selectall_hashref("select * from testtable order by testnumber",1);
assertEqualString($$rows{1}->{TESTCHAR},"testchar1                               ");
assertEqualString($$rows{1}->{TESTVARCHAR},"testvarchar1");
assertEqualString($$rows{1}->{TESTDATE},"01-JAN-01");
assertEqualString($$rows{7}->{TESTCHAR},"testchar7                               ");
assertEqualString($$rows{7}->{TESTVARCHAR},"testvarchar7");
assertEqualString($$rows{7}->{TESTDATE},"01-JAN-07");
$sth=$dbh->prepare("select * from testtable order by testnumber");
$rows=$dbh->selectall_hashref($sth,1);
assertEqualString($$rows{1}->{TESTCHAR},"testchar1                               ");
assertEqualString($$rows{1}->{TESTVARCHAR},"testvarchar1");
assertEqualString($$rows{1}->{TESTDATE},"01-JAN-01");
assertEqualString($$rows{7}->{TESTCHAR},"testchar7                               ");
assertEqualString($$rows{7}->{TESTVARCHAR},"testvarchar7");
assertEqualString($$rows{7}->{TESTDATE},"01-JAN-07");
print("\n");
$rows=$dbh->selectall_hashref("select * from testtable where testnumber=:var1 or testnumber=:var2 order by testnumber",1,undef,('1','2'));
assertEqualString($$rows{1}->{TESTCHAR},"testchar1                               ");
assertEqualString($$rows{1}->{TESTVARCHAR},"testvarchar1");
assertEqualString($$rows{1}->{TESTDATE},"01-JAN-01");
assertEqualString($$rows{2}->{TESTCHAR},"testchar2                               ");
assertEqualString($$rows{2}->{TESTVARCHAR},"testvarchar2");
assertEqualString($$rows{2}->{TESTDATE},"01-JAN-02");
assertUndef($$rows{7}->{TESTCHAR});
assertUndef($$rows{7}->{TESTVARCHAR});
assertUndef($$rows{7}->{TESTDATE});
print("\n");
$rows=$dbh->selectall_hashref("select * from testtable where testnumber=:var1 or testnumber=:var2 order by testnumber",1,undef,'1','2');
assertEqualString($$rows{1}->{TESTCHAR},"testchar1                               ");
assertEqualString($$rows{1}->{TESTVARCHAR},"testvarchar1");
assertEqualString($$rows{1}->{TESTDATE},"01-JAN-01");
assertEqualString($$rows{2}->{TESTCHAR},"testchar2                               ");
assertEqualString($$rows{2}->{TESTVARCHAR},"testvarchar2");
assertEqualString($$rows{2}->{TESTDATE},"01-JAN-02");
assertUndef($$rows{7}->{TESTCHAR});
assertUndef($$rows{7}->{TESTVARCHAR});
assertUndef($$rows{7}->{TESTDATE});
print("\n");


# selectcol_arrayref
print("SELECTCOL_ARRAYREF: \n");
$cols=$dbh->selectcol_arrayref("select * from testtable order by testnumber");
assertEqual($$cols[0],1);
assertEqual($$cols[1],2);
assertEqual($$cols[2],3);
assertEqual($$cols[3],4);
assertEqual($$cols[4],5);
assertEqual($$cols[5],6);
assertEqual($$cols[6],7);
print("\n");
$cols=$dbh->selectcol_arrayref("select * from testtable where testnumber=:var1 or testnumber=:var2 order by testnumber",undef,(1,2));
assertEqual($$cols[0],1);
assertEqual($$cols[1],2);
assertUndef($$cols[2]);
assertUndef($$cols[3]);
assertUndef($$cols[4]);
assertUndef($$cols[5]);
assertUndef($$cols[6]);
print("\n");


# chop blanks
print("CHOP BLANKS: \n");
$dbh->{ChopBlanks}=1;
$rows=$dbh->selectall_arrayref("select * from testtable order by testnumber");
assertEqualString($$rows[0][1],"testchar1");
assertEqualString($$rows[6][1],"testchar7");
$dbh->{ChopBlanks}=0;
$rows=$dbh->selectall_arrayref("select * from testtable order by testnumber");
assertEqualString($$rows[0][1],"testchar1                               ");
assertEqualString($$rows[6][1],"testchar7                               ");
$sth=$dbh->prepare("select * from testtable order by testnumber");
assertEqualString($sth->execute(),"0E0");
$sth->{ChopBlanks}=1;
$rows=$sth->fetchall_arrayref();
assertEqualString($$rows[0][1],"testchar1");
assertEqualString($$rows[6][1],"testchar7");
$sth=$dbh->prepare("select * from testtable order by testnumber");
assertEqualString($sth->execute(),"0E0");
$sth->{ChopBlanks}=0;
$rows=$sth->fetchall_arrayref();
assertEqualString($$rows[0][1],"testchar1                               ");
assertEqualString($$rows[6][1],"testchar7                               ");
print("\n");


# commit and rollback
print("COMMIT AND ROLLBACK: \n");
my $dbh2=DBI->connect($dsn,"testuser","testpassword",{AutoCommit=>0}) or die DBI->errstr;
my @row=$dbh2->selectrow_array("select count(*) from testtable");
assertEqual($row[0],0);
if ($DBI::VERSION>=1.41) {
	assertEqual($dbh->{Executed},1);
}
assertTrue($dbh->commit());
if ($DBI::VERSION>=1.41) {
	assertEqual($dbh->{Executed},0);
}
@row=$dbh2->selectrow_array("select count(*) from testtable");
assertEqual($row[0],9);
$dbh->{AutoCommit}=1;
assertTrue($dbh->do("insert into testtable values (10,'testchar10','testvarchar10','01-JAN-2010')"));
my @row=$dbh2->selectrow_array("select count(*) from testtable");
assertEqual($row[0],10);
$dbh2->disconnect();
$dbh->{AutoCommit}=0;
assertTrue($dbh->do("insert into testtable values (11,'testchar11','testvarchar11','01-JAN-2011')"));
my @row=$dbh->selectrow_array("select count(*) from testtable");
assertEqual($row[0],11);
my @row=$dbh2->selectrow_array("select count(*) from testtable");
assertEqual($row[0],10);
if ($DBI::VERSION>=1.41) {
	assertEqual($dbh->{Executed},1);
}
assertTrue($dbh->rollback());
if ($DBI::VERSION>=1.41) {
	assertEqual($dbh->{Executed},0);
}
my @row=$dbh2->selectrow_array("select count(*) from testtable");
assertEqual($row[0],10);
print("\n");


# row cache size
print("ROW CACHE SIZE: \n");
assertEqual($dbh->{RowCacheSize},0);
$sth=$dbh->prepare("select * from testtable order by testnumber");
assertEqualString($sth->execute(),"0E0");
assertEqual($sth->{RowsInCache},10);
for (my $i=10; $i>0; $i--) {
	@row=$sth->fetchrow_array();
	assertEqual($sth->{RowsInCache},$i-1);
}
print("\n");
$dbh->{RowCacheSize}=0;
$sth=$dbh->prepare("select * from testtable order by testnumber");
assertEqualString($sth->execute(),"0E0");
assertEqual($sth->{RowsInCache},10);
for (my $i=10; $i>0; $i--) {
	@row=$sth->fetchrow_array();
	assertEqual($sth->{RowsInCache},$i-1);
}
print("\n");
$dbh->{RowCacheSize}=-1;
$sth=$dbh->prepare("select * from testtable order by testnumber");
assertEqualString($sth->execute(),"0E0");
assertEqual($sth->{RowsInCache},10);
for (my $i=10; $i>0; $i--) {
	@row=$sth->fetchrow_array();
	assertEqual($sth->{RowsInCache},$i-1);
}
print("\n");
$dbh->{RowCacheSize}=1;
$sth=$dbh->prepare("select * from testtable order by testnumber");
assertEqualString($sth->execute(),"0E0");
assertEqual($sth->{RowsInCache},1);
for (my $i=10; $i>0; $i--) {
	@row=$sth->fetchrow_array();
	assertEqual($sth->{RowsInCache},0);
}
print("\n");
$dbh->{RowCacheSize}=10;
$sth=$dbh->prepare("select * from testtable order by testnumber");
assertEqualString($sth->execute(),"0E0");
@rows=@{$sth->fetchall_arrayref()};
assertEqual($#rows+1,10);
print("\n");


# lots of rows
print("LOTS OF ROWS: \n");
$dbh->do("delete from testtable");
for ($i=0; $i<200; $i++) {
	$dbh->do("insert into testtable values (1,'testchar1','testvarchar1','01-JAN-2001')");
}
$sth=$dbh->prepare("select * from testtable order by testnumber");
assertEqualString($sth->execute(),"0E0");
for ($i=0; $i<200; $i++) {
	@fields=$sth->fetchrow_array;
	if ($fields[0]!=1) {
		break;
	}
}
assertEqual($i,200);
print("\n");


# null binds
print("NULL BINDS: \n");
$dbh->do("delete from testtable");
$sth=$dbh->prepare("insert into testtable values (1,:var2,:var3,:var4)");
$sth->bind_param(1,undef);
$sth->bind_param(2,undef);
$sth->bind_param(3,undef);
assertTrue($sth->execute());
$sth=$dbh->prepare("select * from testtable order by testnumber");
assertEqualString($sth->execute(),"0E0");
@fields=$sth->fetchrow_array;
assertUndef($fields[1]);
assertUndef($fields[2]);
assertUndef($fields[3]);
print("\n");

# drop existing table
$dbh->do("drop table testtable");


# clob/blob binds
print("CLOB/BLOB BINDS: \n");
$dbh->do("drop table testtable");
assertEqualString($dbh->do("create table testtable (testclob clob, testblob blob)"),"0E0");
my $sth=$dbh->prepare("insert into testtable values (:var1,:var2)");
$sth->bind_param("var1","testclob",DBD::SQLRelay::SQL_CLOB);
$sth->bind_param("var2","testblob",{type=>DBD::SQLRelay::SQL_BLOB,length=>8});
assertTrue($sth->execute());
$sth=$dbh->prepare("begin select testclob into :clobvar from testtable;  select testblob into :blobvar from testtable; end;");
$sth->bind_param_inout("clobvar",\$testclob,undef,DBD::SQLRelay::SQL_CLOB);
$sth->bind_param_inout("blobvar",\$testblob,undef,DBD::SQLRelay::SQL_BLOB);
assertTrue($sth->execute());
assertEqualString($testclob,"testclob");
assertEqualString($testblob,"testblob");
$dbh->do("drop table testtable");
print("\n");

# prepare_cached


# prepare cached
print("PREPARE CACHED: \n");
$sth=$dbh->prepare_cached("select 1 from dual");
my $sth1=$dbh->prepare_cached("select 1 from dual");
my $sth2=$dbh->prepare_cached("select 2 from dual");
assertEqual($sth,$sth1);
$s="false";
if ($sth2==$sth) {
	$s="true"
}
assertEqualString($s,"false");
print("\n");


# get info
print("GET INFO: \n");
#assertEqualString($dbh->get_info($GetInfoType{SQL_DATA_SOURCE_NAME}),"TESTUSER");
assertEqualString($dbh->get_info($GetInfoType{SQL_DBMS_NAME}),"oracle");
assertEqualString($dbh->get_info($GetInfoType{SQL_DBMS_VER}),"Oracle Database 12c Enterprise Edition Release 12.2.0.1.0 - 64bit Production");
assertEqualString($dbh->get_info($GetInfoType{SQL_USER_NAME}),"testuser");
assertEqualString($dbh->get_info($GetInfoType{SQL_IDENTIFIER_QUOTE_CHAR}),"\"");
assertEqualString($dbh->get_info($GetInfoType{SQL_CATALOG_NAME_SEPARATOR}),"@");
assertEqualString($dbh->get_info($GetInfoType{SQL_CATALOG_LOCATION}),2);
print("\n");


# quote
print("QUOTE: \n");
assertEqualString($dbh->quote("don't"),"'don''t'");
assertEqualString($dbh->quote("don't",SQL_CHAR),"'don''t'");
assertEqualString($dbh->quote("don't",SQL_VARCHAR),"'don''t'");
assertEqualString($dbh->quote("123",SQL_INTEGER),"'123'");
print("\n");
assertEqualString($dbh->quote_identifier("mytable"),"\"mytable\"");
assertEqualString($dbh->quote_identifier("mycatalog","myschema","mytable"),"\"myschema\".\"mytable\"\@\"mycatalog\"");
print("\n");


# non-lazy connect
print("NON-LAZY CONNECT: \n");
$dsn = $prefix."sqlrelay:host=invalidhost;port=0;socket=/invalidsocket;tries=1;retrytime=1;debug=0;lazyconnect=0";
assertUndef(DBI->connect($dsn,"testuser","testpassword"));
print("\n");

# invalid queries...


# invalid queries
print("INVALID QUERIES: \n");
assertEqual($dbh->do("select * from testtable order by testnumber"),0);
assertEqual($dbh->do("select * from testtable order by testnumber"),0);
assertEqual($dbh->do("select * from testtable order by testnumber"),0);
assertEqual($dbh->do("select * from testtable order by testnumber"),0);
print("\n");
assertEqual($dbh->do("insert into testtable values (1,2,3,4)"),0);
assertEqual($dbh->do("insert into testtable values (1,2,3,4)"),0);
assertEqual($dbh->do("insert into testtable values (1,2,3,4)"),0);
assertEqual($dbh->do("insert into testtable values (1,2,3,4)"),0);
print("\n");
assertEqual($dbh->do("create table testtable"),0);
assertEqual($dbh->do("create table testtable"),0);
assertEqual($dbh->do("create table testtable"),0);
assertEqual($dbh->do("create table testtable"),0);
print("\n");


$dbh->disconnect();

reportTestStatus();

exit($status);
