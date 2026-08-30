#!/bin/sh

# run with postgresqlprotocoltls instance

# see the note in postgresqltls.sh - same instance, same variable
if ( test -z "$POSTGRESQLPROTOCOLTLSPORT1" )
then
	POSTGRESQLPROTOCOLTLSPORT1=5432
fi

PGPASSFILE=`pwd`/pgpass \
PGSSLMODE=verify-ca \
PGSSLROOTCERT=/usr/local/firstworks/etc/sqlrelay.conf.d/ca.pem \
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
