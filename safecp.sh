#!/bin/sh

# A cp that replaces the destination rather than rewriting it in place.
#
# cp truncates an existing destination and writes over it, which keeps the
# same inode.  Every process that has that file mmap'd - a running
# sqlr-connection with a module dlopen'd, for instance - therefore sees its
# text pages change underneath it, faults, and spins.  See trac #8852.
#
# Removing the destination first gives the new file a new inode.  Processes
# holding the old one keep it until they exit, and the next process to start
# picks up the new one, which is what every package manager does.
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

if ( test -d "$DEST" )
then
	# SOURCE... DIRECTORY - remove each file's counterpart in the
	# directory, then copy it in
	for SOURCE in $SOURCES
	do
		rm -f "$DEST/`basename \"$SOURCE\"`"
		cp "$SOURCE" "$DEST" || exit $?
	done
else
	# SOURCE DEST - remove the destination, then copy onto it
	rm -f "$DEST"
	cp $SOURCES "$DEST" || exit $?
fi

exit 0
