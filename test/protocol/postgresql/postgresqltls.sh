#!/bin/sh

# run with postgresqlprotocoltls instance

# POSTGRESQLPROTOCOLTLSPORT1 names the port that instance ended up on,
# following the @INSTANCENAMEPORTn@ convention
# test/sqlrelay.conf.d/*.conf.in uses.  there's no postgresqlprotocoltls
# instance in test/sqlrelay.conf.d yet (#9560), so nothing generates that token
# today; unset means 5432, as before
if ( test -z "$POSTGRESQLPROTOCOLTLSPORT1" )
then
	POSTGRESQLPROTOCOLTLSPORT1=5432
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
