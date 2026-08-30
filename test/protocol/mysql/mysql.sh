#!/bin/sh

# run with mysqlprotocol instance

# that instance doesn't have to be on 3306 - two sessions running this suite
# at once can't both have it.  MYSQLPROTOCOLPORT1 names the port it actually
# ended up on; it's the same variable
# test/sqlrelay.conf.d/mysqlprotocol.conf.in's @MYSQLPROTOCOLPORT1@ is
# generated from, so one value drives both ends
if ( test -z "$MYSQLPROTOCOLPORT1" )
then
	MYSQLPROTOCOLPORT1=3306
fi

mysql \
	-h sqlrelay \
	-P $MYSQLPROTOCOLPORT1 \
	-u testuser \
	-ptestpassword \
> /dev/null << EOF
select 1
EOF

if ( test "$?" = "0" )
then
	echo success
	exit 0
else
	echo failed
	exit 1
fi
