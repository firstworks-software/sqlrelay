#!/bin/sh

# run with tdsprotocoltlstest instance
#
# freetds takes its tls settings from freetds.conf, not from the command
# line, so this drives the [sqlrelaytlsserverauth] entry in
# test/sqlrelay.conf.d/freetds/etc/freetds.conf, which adds a ca file and
# turns on certificate hostname checking

FREETDSCONF=../../sqlrelay.conf.d/freetds/etc/freetds.conf
export FREETDSCONF

echo "select 1" | tsql \
	-S sqlrelaytlsserverauth \
	-U testuser \
	-P testpassword \
	> /dev/null

if ( test "$?" = "0" )
then
	echo success
	exit 0
else
	echo failed
	exit 1
fi
