#!/bin/sh

# run with mysqlprotocoltls instance

# see the note in mysqltls.sh - same instance, same variable
if ( test -z "$MYSQLPROTOCOLTLSPORT1" )
then
	MYSQLPROTOCOLTLSPORT1=3306
fi

mysql \
	-h sqlrelay \
	-P $MYSQLPROTOCOLTLSPORT1 \
	-u testuser \
	-ptestpassword \
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
