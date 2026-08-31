#!/bin/sh

# run with postgresqlprotocoltls instance

# POSTGRESQLPROTOCOLTLSPORT1 names the port that instance ended up on,
# following the @INSTANCENAMEPORTn@ convention
# test/sqlrelay.conf.d/*.conf.in uses.  unset means 5434, the default
if ( test -z "$POSTGRESQLPROTOCOLTLSPORT1" )
then
	POSTGRESQLPROTOCOLTLSPORT1=5434
fi

PGPASSFILE=`pwd`/pgpass \
PGSSLMODE=require \
psql \
	-h sqlrelay \
	-p $POSTGRESQLPROTOCOLTLSPORT1 \
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
