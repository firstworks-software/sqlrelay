#!/bin/sh

# run with mysqlprotocoltlsserverauth instance

# MYSQLPROTOCOLTLSSERVERAUTHPORT1 names the port that instance ended up on,
# following the @INSTANCENAMEPORTn@ convention
# test/sqlrelay.conf.d/*.conf.in uses.  unset means 3308, the default
if ( test -z "$MYSQLPROTOCOLTLSSERVERAUTHPORT1" )
then
	MYSQLPROTOCOLTLSSERVERAUTHPORT1=3308
fi

mysql \
	-h sqlrelay \
	-P $MYSQLPROTOCOLTLSSERVERAUTHPORT1 \
	-u testuser \
	-ptestpassword \
	--ssl-mode=VERIFY_CA \
	--ssl-ca=../../sqlrelay.conf.d/tls/ca.pem \
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
