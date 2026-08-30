#!/bin/sh

# Copies the modules that libtool built in the current directory into a single
# flat staging directory, giving each the same file name that "make install"
# would have given it.
#
# A listener or connection started out of the build tree can then be pointed at
# that directory with -libexecdir, and it loads freshly built modules without a
# "make install" into the shared prefix.
#
# Run from a module directory - src/auths, src/protocols, and so on - by that
# directory's stagemodules make target.  Each module directory stages into the
# same directory, so the result is one flat directory containing every module.

STAGEDIR=$1
MODULESUFFIX=$2

DIR=`pwd`

die() {
	echo "stagemodules.sh: $1" >&2
	exit 1
}

if ( test -z "$STAGEDIR" -o -z "$MODULESUFFIX" )
then
	die "usage: stagemodules.sh STAGEDIR MODULESUFFIX"
fi

# each module directory runs this with its own directory as the working
# directory, so a relative staging directory would mean a different place
# for each of them
case "$STAGEDIR" in
	/*)
		;;
	*)
		die "STAGEDIR must be an absolute path, but is \"$STAGEDIR\""
		;;
esac

# The .la files are exactly the modules that this directory's "make all" built.
# Anything listed here but missing from .libs is a module that didn't build,
# and staging it silently would mean testing a stale module - or no module at
# all - without being told, so it's a hard error.
LAFILES=`ls *.la 2>/dev/null`
if ( test -z "$LAFILES" )
then
	die "no modules have been built in $DIR - run make there first"
fi

if ( test ! -d "$STAGEDIR" )
then
	mkdir -p "$STAGEDIR"
fi
if ( test ! -d "$STAGEDIR" )
then
	die "could not create $STAGEDIR"
fi

for LA in $LAFILES
do
	NAME=`basename "$LA" .la`
	SO=.libs/$NAME.so
	if ( test ! -f "$SO" )
	then
		die "$DIR/$SO is missing - the $NAME module didn't build"
	fi
	# a copy rather than a symlink, so the staged directory keeps working
	# if the build tree is cleaned, and so it can also be bind-mounted over
	# the real libexecdir, where a symlink would point at itself
	if ( ! cp -f "$SO" "$STAGEDIR/$NAME.$MODULESUFFIX" )
	then
		die "could not copy $DIR/$SO to $STAGEDIR/$NAME.$MODULESUFFIX"
	fi
	echo "staged $NAME.$MODULESUFFIX"
done
