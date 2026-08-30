#!/bin/sh

# run with mysqlprotocoltls instance

# MYSQLPROTOCOLTLSPORT1 names the port that instance ended up on, following
# the @INSTANCENAMEPORTn@ convention test/sqlrelay.conf.d/*.conf.in uses.
# there's no mysqlprotocoltls instance in test/sqlrelay.conf.d yet, so
# nothing generates that token today; unset means 3306, as before
if ( test -z "$MYSQLPROTOCOLTLSPORT1" )
then
	MYSQLPROTOCOLTLSPORT1=3306
fi

mysql \
	-h sqlrelay \
	-P $MYSQLPROTOCOLTLSPORT1 \
	-u testuser \
	-ptestpassword \
	--ssl \
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
