#!/bin/sh

# run with mysqlprotocoltlsmutualauth instance

# MYSQLPROTOCOLTLSMUTUALAUTHPORT1 names the port that instance ended up on,
# following the @INSTANCENAMEPORTn@ convention
# test/sqlrelay.conf.d/*.conf.in uses.  unset means 3309, the default
if ( test -z "$MYSQLPROTOCOLTLSMUTUALAUTHPORT1" )
then
	MYSQLPROTOCOLTLSMUTUALAUTHPORT1=3309
fi

mysql \
	-h sqlrelay \
	-P $MYSQLPROTOCOLTLSMUTUALAUTHPORT1 \
	-u testuser \
	-ptestpassword \
	--ssl-mode=VERIFY_CA \
	--ssl-ca=../../sqlrelay.conf.d/tls/ca.pem \
	--ssl-cert=../../sqlrelay.conf.d/tls/client.pem \
	--ssl-key=../../sqlrelay.conf.d/tls/client.pem \
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
