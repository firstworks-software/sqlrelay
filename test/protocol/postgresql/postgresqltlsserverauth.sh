#!/bin/sh

# run with postgresqlprotocoltlsserverauth instance

# POSTGRESQLPROTOCOLTLSSERVERAUTHPORT1 names the port that instance ended up
# on, following the @INSTANCENAMEPORTn@ convention
# test/sqlrelay.conf.d/*.conf.in uses.  unset means 5435, the default
if ( test -z "$POSTGRESQLPROTOCOLTLSSERVERAUTHPORT1" )
then
	POSTGRESQLPROTOCOLTLSSERVERAUTHPORT1=5435
fi

PGPASSFILE=`pwd`/pgpass \
PGSSLMODE=verify-ca \
PGSSLROOTCERT=../../sqlrelay.conf.d/tls/ca.pem \
psql \
	-h sqlrelay \
	-p $POSTGRESQLPROTOCOLTLSSERVERAUTHPORT1 \
	-U testuser \
	-w \
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
