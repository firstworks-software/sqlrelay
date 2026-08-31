#!/bin/sh

# run with mysqlprotocoltls instance

# MYSQLPROTOCOLTLSPORT1 names the port that instance ended up on, following
# the @INSTANCENAMEPORTn@ convention test/sqlrelay.conf.d/*.conf.in uses.
# unset means 3307, the default
if ( test -z "$MYSQLPROTOCOLTLSPORT1" )
then
	MYSQLPROTOCOLTLSPORT1=3307
fi

mysql \
	-h sqlrelay \
	-P $MYSQLPROTOCOLTLSPORT1 \
	-u testuser \
	-ptestpassword \
	--ssl-mode=REQUIRED \
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
