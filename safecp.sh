#!/bin/sh

# A cp that replaces the destination rather than rewriting it in place.
#
# cp truncates an existing destination and writes over it, which keeps the
# same inode.  Every process that has that file mmap'd - a running
# sqlr-connection with a module dlopen'd, for instance - therefore sees its
# text pages change underneath it, faults, and spins.  See trac #8852.
#
# So copy to a temporary name in the destination's own directory and rename
# it over the target.  rename() is atomic, and gives the destination the
# temporary file's inode, so:
#
#	* processes holding the old file keep it until they exit, and the
#	  next process to start picks up the new one
#	* a copy that fails part way - disk full, permissions, interrupted -
#	  leaves the previous version in place, because nothing has replaced
#	  it yet
#
# Removing the destination first would get the first property but not the
# second.  This is what package managers do, and why #8852 listed it first.
#
# Usage is cp's, in the two forms the makefiles use:
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

# An interrupted run would otherwise leave its temporary behind in the
# install directory, so clean it up on the way out.  CURRENTTEMP is empty
# except while a copy is actually in progress.
CURRENTTEMP=""
cleanup() {
	if ( test -n "$CURRENTTEMP" )
	then
		rm -f "$CURRENTTEMP"
	fi
	exit 1
}
trap cleanup HUP INT QUIT TERM

# copy one file, through a temporary in the target's own directory so the
# rename is within one file system and is therefore atomic
copyone() {

	SOURCE="$1"
	TARGET="$2"

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
