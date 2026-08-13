#!/bin/sh

# run with firebirdprotocoltest instance

# on fedora /usr/bin/isql is unixODBC's, firebird's is /usr/bin/isql-fb
ISQL=isql
if ( which isql-fb > /dev/null 2>&1 )
then
	ISQL=isql-fb
fi

HOST=`hostname | cut -d. -f1`

# the sqlrelay listener runs on this host, not on a host literally named
# "sqlrelay" - that name doesn't resolve here and isql-fb still exits 0
# when it can't connect at all, so a plain exit-status check would call
# that a pass
OUTPUT=`printf 'select 1 from rdb$database;\n' | $ISQL \
	-user testuser \
	-password testpassword \
	localhost:/u02/$HOST.gdb 2>&1`

# classify what isql actually printed - no default-pass branch, so an
# unrecognized transcript is its own failure rather than a silent pass
if ( echo "$OUTPUT" | grep -q "Statement failed" )
then
	echo "failed"
	echo "$OUTPUT"
	exit 1
fi

if ( echo "$OUTPUT" | grep -q "CONSTANT" )
then
	if ( echo "$OUTPUT" | grep -qE "^ *1 *\$" )
	then
		echo "success"
		exit 0
	fi
fi

echo "unknown"
echo "$OUTPUT"
exit 1
