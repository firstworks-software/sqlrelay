#!/bin/sh

# expands the server-side include directives in .html.in files, writing each
# one back out without its .in suffix
#
# usage: ssi.sh file.html.in [file.html.in ...]
#
# a directive looks like:
#	<!--#include virtual="path" -->
# and is replaced by the contents of the file it names.  the path is relative
# to the directory of the file holding the directive, the way apache's
# mod_include treats virtual.  when the named file has a .in counterpart, the
# .in file is expanded instead, so the order the files are generated in does
# not matter.

# writes a message to stderr and gives up
die() {
	echo "ssi.sh: $1" >&2
	exit 1
}

# writes one line to stdout, expanding its include directive, if it has one
# $1 - the line
# $2 - the directory to resolve the include path against
# $3 - the chain of files currently being expanded
expandline() {
	_elline=$1
	_eldir=$2
	_elchain=$3

	# pass through any line without a directive
	case "$_elline" in
		*'<!--#include'*)
			;;
		*)
			printf '%s\n' "$_elline"
			return 0
			;;
	esac

	# split the line around the directive
	_elpre=`printf '%s' "$_elline" | sed -e 's|<!--#include.*$||'`
	_elpost=`printf '%s' "$_elline" | sed -e 's|^.*-->||'`
	_eltarget=`printf '%s' "$_elline" | sed -e 's|^.*virtual="||' -e 's|".*$||'`

	if ( test -z "$_eltarget" )
	then
		die "malformed include directive: $_elline"
	fi

	# prefer the .in file, when there is one
	_elfile=$_eldir/$_eltarget
	if ( test -f "$_elfile.in" )
	then
		_elfile=$_elfile.in
	fi

	printf '%s' "$_elpre"

	( expandfile "$_elfile" "$_elchain" )
	if ( test $? -ne 0 )
	then
		exit 1
	fi

	printf '%s\n' "$_elpost"

	return 0
}

# writes a file to stdout, expanding its include directives
# $1 - the file
# $2 - the chain of files currently being expanded
expandfile() {
	_efname=$1
	_efchain=$2

	# catch an include cycle before it runs away
	case "$_efchain" in
		*" $_efname "*)
			die "include cycle at $_efname"
			;;
	esac

	if ( test ! -f "$_efname" )
	then
		die "no such file: $_efname"
	fi

	_efchain="$_efchain $_efname "
	_efdir=`dirname "$_efname"`

	# read is deliberately not wrapped in parentheses.  a subshell would
	# throw away the line it read.
	while IFS= read -r _efline
	do
		expandline "$_efline" "$_efdir" "$_efchain"
	done < "$_efname"

	# a last line with no newline after it leaves the loop without ever
	# reaching the body
	if ( test -n "$_efline" )
	then
		printf '%s' "$_efline"
	fi

	return 0
}

if ( test $# -lt 1 )
then
	echo "usage: ssi.sh file.html.in [file.html.in ...]" >&2
	exit 1
fi

while ( test $# -gt 0 )
do
	infile=$1
	shift

	case "$infile" in
		*.in)
			;;
		*)
			die "input file name does not end in .in: $infile"
			;;
	esac

	outfile=`echo "$infile" | sed -e 's|\.in$||'`

	( expandfile "$infile" "" ) > "$outfile"
	if ( test $? -ne 0 )
	then
		rm -f "$outfile"
		exit 1
	fi
done

exit 0
