#!/bin/sh

# run with postgresqlprotocoltlsmutualauth instance

# POSTGRESQLPROTOCOLTLSMUTUALAUTHPORT1 names the port that instance ended up
# on, following the @INSTANCENAMEPORTn@ convention
# test/sqlrelay.conf.d/*.conf.in uses.  unset means 5436, the default
if ( test -z "$POSTGRESQLPROTOCOLTLSMUTUALAUTHPORT1" )
then
	POSTGRESQLPROTOCOLTLSMUTUALAUTHPORT1=5436
fi

# libpq refuses a PGSSLKEY file with any group or world permission bits, and
# the repo's client.pem is 0664, so copy it to a run-local 0600 file rather
# than chmod the repo file itself
CLIENTPEM=./client.pem.tmp
cp ../../sqlrelay.conf.d/tls/client.pem $CLIENTPEM
chmod 600 $CLIENTPEM

PGPASSFILE=`pwd`/pgpass \
PGSSLMODE=verify-ca \
PGSSLCERT=$CLIENTPEM \
PGSSLKEY=$CLIENTPEM \
PGSSLROOTCERT=../../sqlrelay.conf.d/tls/ca.pem \
psql \
	-h sqlrelay \
	-p $POSTGRESQLPROTOCOLTLSMUTUALAUTHPORT1 \
	-U testuser \
	-w \
> /dev/null << EOF
select 1
EOF

STATUS=$?

rm -f $CLIENTPEM

if ( test "$STATUS" = "0" )
then
	echo success
	exit 0
else
	echo failed
	exit 1
fi
