#!/usr/bin/perl

# Writes the doxygen input stub from the call-seq doc comments in
# sql_relay.cpp.  The php-visible signature exists nowhere in the C++ - zend
# pulls the arguments at runtime - so the call-seq line carries it, the way
# rdoc's call-seq does for the ruby api.
#
# usage: mkstub.pl sql_relay.cpp sql_relay.doc.php

use FileHandle;

my $group="sql_relay";

my $cppfile=new FileHandle;
$cppfile->open($ARGV[0]) || die "couldn't open $ARGV[0]\n";
my $cpp=join('',<$cppfile>);
$cppfile->close();

# what the extension exports
my %exported;
foreach $name($cpp=~m/ZEND_FE\((\w+)\s*,/g) {
	$exported{lc($name)}=1;
}

# which arities each function accepts
#
# Only a ZEND_NUM_ARGS() test that GET_PARAMETERS follows is a guard.
# inputBind() and substitution() also test it mid-body, to pick an overload;
# those aren't arities the function accepts.
my %arity;
while ($cpp=~m/DLEXPORT ZEND_FUNCTION\((\w+)\)(.*?)\n\}\n/gs) {
	my $zend=lc($1);
	my $body=$2;
	foreach $count($body=~m/ZEND_NUM_ARGS\(\)\s*[!=]=\s*(\d+)\s*(?:\|\||&&)\s*GET_PARAMETERS/g) {
		$arity{$zend}->{$count}=1;
	}
}

my @errors;
my @stub;
my %documented;

# each call-seq comment and the function it sits above
while ($cpp=~m|/\*\*\n \*  call-seq:\n \*  (\w+)\((.*?)\)\n \*\n(.*?)\*/\nDLEXPORT ZEND_FUNCTION\((\w+)\)|gs) {

	my $name=$1;
	my $params=$2;
	my $body=$3;
	my $zend=$4;

	# the call-seq must name the function it documents
	if (lc($name) ne lc($zend)) {
		push(@errors,"call-seq says $name() but it documents ".
				"ZEND_FUNCTION($zend)");
		next;
	}

	# ...and that function must be exported to php
	if (!$exported{lc($zend)}) {
		push(@errors,"$name() is documented but the extension ".
				"never exports it");
		next;
	}

	$documented{lc($zend)}=1;

	# ...and the arity it documents must be one the function accepts -
	# the multi-arity functions document just one representative arity
	my @accepted=sort {$a<=>$b} keys %{$arity{lc($zend)}};
	my @args=grep(/\S/,split(/,/,$params));
	if (!@accepted) {
		push(@errors,"$name() is documented but has no arity guard");
	} elsif (!grep($_==$#args+1,@accepted)) {
		push(@errors,"$name() documents ".($#args+1)." argument(s) ".
				"but the function accepts ".
				join(" or ",@accepted));
	}

	my @prose;
	foreach $line(split(/\n/,$body)) {
		$line=~s/^ \* {0,2}//;
		$line=~s/\s+$//;
		push(@prose,$line);
	}
	while ($#prose>=0 && $prose[$#prose]=~m/^$/) {
		pop(@prose);
	}

	push(@stub,"/** \@ingroup $group");
	foreach $line(@prose) {
		my $out=" *  ".$line;
		$out=~s/\s+$//;
		push(@stub,$out);
	}
	$stub[$#stub].=" */";
	push(@stub,"function $name($params){}");
	push(@stub,"");
}

# an exported function with no doc comment is a gap, not an omission
foreach $name(sort keys %exported) {
	if (!$documented{$name}) {
		push(@errors,"$name() is exported but has no doc comment");
	}
}

if ($#errors>=0) {
	print STDERR "mkstub.pl: ".($#errors+1)." problem(s)\n";
	foreach $error(@errors) {
		print STDERR "  $error\n";
	}
	exit 1;
}

my $out=new FileHandle;
$out->open(">$ARGV[1]") || die "couldn't write $ARGV[1]\n";
print $out "<?php\n\n";
print $out "/** \@file\n *  \@defgroup $group $group */\n\n";
foreach $line(@stub) {
	print $out "$line\n";
}
$out->close();

print STDERR "mkstub.pl: ".scalar(keys %documented)." functions documented, ".
		scalar(keys %exported)." exported\n";
exit;
