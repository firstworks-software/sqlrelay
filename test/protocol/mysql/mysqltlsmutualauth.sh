#!/bin/sh

# run with mysqlprotocoltlsmutualauth instance

# see the note in mysqltls.sh - same idea, for the mutual-auth instance
if ( test -z "$MYSQLPROTOCOLTLSMUTUALAUTHPORT1" )
then
	MYSQLPROTOCOLTLSMUTUALAUTHPORT1=3306
fi

mysql \
	-h sqlrelay \
	-P $MYSQLPROTOCOLTLSMUTUALAUTHPORT1 \
	-u testuser \
	-ptestpassword \
	--ssl-cert=/usr/local/firstworks/etc/sqlrelay.conf.d/client.pem \
	--ssl-ca=/usr/local/firstworks/etc/sqlrelay.conf.d/ca.pem \
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
