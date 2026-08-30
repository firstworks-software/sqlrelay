#!/bin/sh

# run with postgresqlprotocol instance

# that instance doesn't have to be on 5432 - two sessions running this suite
# at once can't both have it.  POSTGRESQLPROTOCOLPORT1 names the port it
# actually ended up on; it's the same variable
# test/sqlrelay.conf.d/postgresqlprotocol.conf.in's @POSTGRESQLPROTOCOLPORT1@
# is generated from, so one value drives both ends.  the pgpass entry
# wildcards the port, so it matches whatever this is
if ( test -z "$POSTGRESQLPROTOCOLPORT1" )
then
	POSTGRESQLPROTOCOLPORT1=5432
fi

PGPASSFILE=`pwd`/pgpass \
PGSSLMODE=disable \
psql \
	-h sqlrelay \
	-p $POSTGRESQLPROTOCOLPORT1 \
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
