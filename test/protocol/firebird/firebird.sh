#!/bin/sh

# run with firebirdprotocoltest instance

# on fedora /usr/bin/isql is unixODBC's, firebird's is /usr/bin/isql-fb
ISQL=isql
if ( which isql-fb > /dev/null 2>&1 )
then
	ISQL=isql-fb
fi

HOST=`hostname | cut -d. -f1`

$ISQL \
	-user testuser \
	-password testpassword \
	sqlrelay:/u02/$HOST.gdb \
> /dev/null << 'EOF'
select 1 from rdb$database;
EOF

if ( test "$?" = "0" )
then
	echo success
	exit 0
else
	echo failed
	exit 1
fi
