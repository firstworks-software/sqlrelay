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
	else
		echo "unknown"
		echo "$OUTPUT"
		exit 1
	fi
else
	echo "unknown"
	echo "$OUTPUT"
	exit 1
fi

# show commands compile a blr request and stream it back through
# op_compile/op_start_and_receive/op_start_send_and_receive rather than dsql -
# a database with no user tables still exercises the request/response path
OUTPUT=`printf 'show tables;\n' | $ISQL \
	-user testuser \
	-password testpassword \
	localhost:/u02/$HOST.gdb 2>&1`

if ( echo "$OUTPUT" | grep -q "Statement failed" )
then
	echo "failed"
	echo "$OUTPUT"
	exit 1
fi

if ( echo "$OUTPUT" | grep -q "There are no tables in this database" )
then
	echo "success"
else
	echo "unknown"
	echo "$OUTPUT"
	exit 1
fi

# show table <name> is the heaviest show variant - joins RDB$RELATION_FIELDS,
# RDB$FIELDS, RDB$INDICES, RDB$INDEX_SEGMENTS, RDB$RELATION_CONSTRAINTS,
# RDB$CHECK_CONSTRAINTS and RDB$TRIGGERS onto a single system table lookup
OUTPUT=`printf 'show table rdb\$relations;\n' | $ISQL \
	-user testuser \
	-password testpassword \
	localhost:/u02/$HOST.gdb 2>&1`

if ( echo "$OUTPUT" | grep -q "Statement failed" )
then
	echo "failed"
	echo "$OUTPUT"
	exit 1
fi

if ( echo "$OUTPUT" | grep -q "RDB\$RELATION_NAME" )
then
	echo "success"
else
	echo "unknown"
	echo "$OUTPUT"
	exit 1
fi

exit 0
