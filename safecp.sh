#!/bin/sh

# a cp that replaces the destination rather than rewriting it in place.  cp
# keeps the destination's inode, so every process that has the file mmap'd -
# a running sqlr-connection with a module dlopen'd, for instance - sees its
# text pages change underneath it, faults, and spins.  copying to a temporary
# and renaming it over the target gives the destination a new inode instead,
# and, unlike removing the destination first, leaves the previous version in
# place when a copy fails part way.
#
# usage is cp's, in the two forms the makefiles use:
#	safecp.sh SOURCE DEST
#	safecp.sh SOURCE... DIRECTORY

# need at least a source and a destination
if ( test $# -lt 2 )
then
	echo "usage: safecp.sh SOURCE... DEST" 1>&2
	exit 1
fi

# the destination is the last argument
DEST=""
for ARG
do
	DEST="$ARG"
done

# collect the sources - every argument but the last
SOURCES=""
COUNT=1
TOTAL=$#
for ARG
do
	if ( test $COUNT -lt $TOTAL )
	then
		SOURCES="$SOURCES $ARG"
	fi
	COUNT=`expr $COUNT + 1`
done

# clean up the temporary an interrupted run would otherwise leave behind
CURRENTTEMP=""
cleanup() {
	if ( test -n "$CURRENTTEMP" )
	then
		rm -f "$CURRENTTEMP"
	fi
	exit 1
}
trap cleanup HUP INT QUIT TERM

copyone() {

	SOURCE="$1"
	TARGET="$2"

	# the temporary goes in the target's own directory, so the rename
	# is within one file system and is therefore atomic
	TARGETDIR="`dirname \"$TARGET\"`"
	TEMP="$TARGETDIR/.safecp$$.`basename \"$TARGET\"`"
	CURRENTTEMP="$TEMP"

	if ( cp "$SOURCE" "$TEMP" )
	then
		:
	else
		rm -f "$TEMP"
		CURRENTTEMP=""
		return 1
	fi

	if ( mv "$TEMP" "$TARGET" )
	then
		CURRENTTEMP=""
		return 0
	fi

	rm -f "$TEMP"
	CURRENTTEMP=""
	return 1
}

if ( test -d "$DEST" )
then
	# SOURCE... DIRECTORY
	for SOURCE in $SOURCES
	do
		copyone "$SOURCE" "$DEST/`basename \"$SOURCE\"`" || exit 1
	done
else
	# SOURCE DEST
	copyone $SOURCES "$DEST" || exit 1
fi

exit 0
