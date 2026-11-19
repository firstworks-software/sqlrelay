#!/bin/sh

# run with the build tree's programs and the sqlite connection module
#
# checks that sqlr-start -wait reports Instance is ready., and exits 0, for a
# sqlrclient listener with only a port next to a mysql listener with a port
# and a socket, and that it exits 1 when the instance never accepts client
# connections or its listener never starts (#11032).  this doesn't go through
# test.sh.  it starts three instances of its own, from a config of its own, on
# ports from freeport and with a localstatedir of its own, so it can run beside
# a test.sh run
#
# takes about 35 seconds, nearly all of it the 30 second wait for the two
# instances that never are ready, which run at the same time

TESTDIR=`cd \`dirname $0\` && pwd`
BUILDDIR=`dirname $TESTDIR`
SQLRBINDIR=$BUILDDIR/src/server
MAKE=${MAKE:-make}

# the instance ids and the localstatedir stay short: the handoff socket's
# path has to fit in the 108 bytes of a unix socket address
READYID=sw1
FAILID=sw2
STALEID=sw3

# stop the instances and remove everything this made
cleanup() {
	if ( test -n "$WORKDIR" -a -d "$WORKDIR" )
	then
		if ( test -f "$WORKDIR/test.conf" )
		then
			for id in $READYID $FAILID $STALEID
			do
				$SQLRBINDIR/sqlr-stop $PATHOPTS -config $WORKDIR/test.conf -id $id > /dev/null 2>&1
			done
		fi
		rm -rf "$WORKDIR"
	fi
}

# report a failed check
fail() {
	echo "failed: $1"
	failures=`expr $failures + 1`
}

# prerequisites
if ( test ! -x "$TESTDIR/freeport" -o ! -x "$SQLRBINDIR/sqlr-start" )
then
	echo "freeport or sqlr-start is not built - run make in $BUILDDIR"
	exit 1
fi
if ( test ! -f "$BUILDDIR/src/connections/sqlrconnection_sqlite.la" -o ! -f "$BUILDDIR/src/protocols/sqlrprotocol_mysql.la" )
then
	echo "skipped: the sqlite connection or the mysql protocol was not built"
	exit 0
fi

# make a private work dir, and the modules and libraries to run out of
WORKDIR=`mktemp -d /tmp/sqlrsw.XXXXXX`
trap cleanup 0
trap "exit 1" 1 2 15
mkdir $WORKDIR/mods
( cd "$BUILDDIR" && $MAKE stagemodules STAGEDIR="$WORKDIR/mods" ) > $WORKDIR/stage.log 2>&1
if ( test "$?" -ne "0" )
then
	echo "make stagemodules failed:"
	cat $WORKDIR/stage.log
	exit 1
fi
LD_LIBRARY_PATH=$BUILDDIR/src/server/.libs:$BUILDDIR/src/util/.libs:$BUILDDIR/src/api/c++/.libs:$BUILDDIR/src/api/c/.libs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
export LD_LIBRARY_PATH
PATHOPTS="-libexecdir $WORKDIR/mods -bindir $SQLRBINDIR -localstatedir $WORKDIR/lsd"
mkdir $WORKDIR/lsd

# pick ports
set -- `$TESTDIR/freeport 2`
PORT1=$1
PORT2=$2
if ( test -z "$PORT1" -o -z "$PORT2" -o -n "$3" )
then
	echo "freeport did not return two ports: $*"
	exit 1
fi

# the first instance listens on a port with sqlrclient, and on a port and a
# socket with mysql.  the second logs in to a database in a directory that
# doesn't exist.  the third has a good database, but a stale listener pid file
# keeps its listener from starting
cat > $WORKDIR/test.conf << EOF
<?xml version="1.0"?>
<instances>
	<instance id="$READYID" dbase="sqlite" connections="1">
		<listeners>
			<listener protocol="sqlrclient" port="$PORT1"/>
			<listener protocol="mysql" port="$PORT2" socket="$WORKDIR/mysql.socket"/>
		</listeners>
		<connections>
			<connection string="db=$WORKDIR/ready.db"/>
		</connections>
	</instance>
	<instance id="$FAILID" dbase="sqlite" connections="1">
		<listeners>
			<listener protocol="sqlrclient" socket="$WORKDIR/fail.socket"/>
		</listeners>
		<connections>
			<connection string="db=$WORKDIR/nodir/fail.db"/>
		</connections>
	</instance>
	<instance id="$STALEID" dbase="sqlite" connections="1">
		<listeners>
			<listener protocol="sqlrclient" socket="$WORKDIR/stale.socket"/>
		</listeners>
		<connections>
			<connection string="db=$WORKDIR/stale.db"/>
		</connections>
	</instance>
</instances>
EOF

failures=0

# an instance that starts is ready
$SQLRBINDIR/sqlr-start $PATHOPTS -config $WORKDIR/test.conf -id $READYID -wait > $WORKDIR/ready.log 2>&1
status=$?
if ( test "$status" -ne "0" )
then
	fail "ready instance exited $status"
fi
if ( ! grep -q "Instance is ready." $WORKDIR/ready.log )
then
	fail "ready instance did not print Instance is ready."
fi

# make a listener pid file with the pid of a process that is gone
mkdir -p $WORKDIR/lsd/run/sqlrelay
sh -c "exit 0" &
deadpid=$!
wait $deadpid
if ( kill -0 $deadpid > /dev/null 2>&1 )
then
	echo "pid $deadpid is alive, can't make a stale pid file"
	exit 1
fi
echo $deadpid > $WORKDIR/lsd/run/sqlrelay/sqlr-listener-$STALEID.pid

# an instance that can't log in to its database never is, and an instance with
# a stale listener pid file never starts.  start both at once, so the two 30
# second waits overlap
$SQLRBINDIR/sqlr-start $PATHOPTS -config $WORKDIR/test.conf -id $FAILID -wait > $WORKDIR/fail.log 2>&1 &
failpid=$!
$SQLRBINDIR/sqlr-start $PATHOPTS -config $WORKDIR/test.conf -id $STALEID -wait > $WORKDIR/stale.log 2>&1 &
stalepid=$!
wait $failpid
status=$?
if ( test "$status" -ne "1" )
then
	fail "unready instance exited $status"
fi
if ( grep -q "Instance is ready." $WORKDIR/fail.log )
then
	fail "unready instance printed Instance is ready."
fi
if ( ! grep -q "Instance is not accepting client connections yet." $WORKDIR/fail.log )
then
	fail "unready instance did not print Instance is not accepting client connections yet."
fi
wait $stalepid
status=$?
if ( test "$status" -ne "1" )
then
	fail "stale pid file instance exited $status"
fi
if ( grep -q "Instance is ready." $WORKDIR/stale.log )
then
	fail "stale pid file instance printed Instance is ready."
fi
if ( grep -q "not accepting" $WORKDIR/stale.log )
then
	fail "stale pid file instance printed not accepting"
fi
if ( ! grep -q "Instance failed to start." $WORKDIR/stale.log )
then
	fail "stale pid file instance did not print Instance failed to start."
fi

# show what the failed checks saw
if ( test "$failures" -ne "0" )
then
	cat $WORKDIR/ready.log $WORKDIR/fail.log $WORKDIR/stale.log
	echo "failed"
	exit 1
fi
echo "success"
exit 0
