# SQL Relay tests

## What is here

- test.sh - the harness everything runs through.  It is generated from
  test.sh.in by make, so edit the template, not the script.
- one directory per client API - c, c++, java, perl, python, and so on.
  Each holds one test program per instance.
- protocol/ - the wire protocol suites.  These do not use SQL Relay's own
  client API at all.  They point a real database's own client - the mysql
  client, psql, tsql, an oracle client - at a listener that speaks that
  database's protocol.
- sqlrelay.conf.d/ - the instance configs the harness starts.  These are
  generated from .conf.in templates by configure.
- freeport - a helper that prints free tcp ports.  Built by make in this
  directory.

## Running

    make tests

runs everything the configure flags enabled.  To run less than that:

    ./test.sh sqlite
    ./test.sh sqlite c++
    ./test.sh "mysql postgresql" "c c++"

The first argument is a space separated list of instances, the second a
space separated list of APIs.  Both default to everything built.

    ./test.sh -h

lists every option.

## Running against a build tree, with no install

By default the harness runs the programs installed in the prefix
(/usr/local/firstworks by default).  That is a problem for anyone changing a
module, because the module search path is compiled into the binaries: a
listener started out of a build tree still loads its protocol, connection,
auth and config modules out of the installed prefix.  Testing a change used
to mean a make install first, which is not an isolated test at all, and needs
write access to a directory other people share.

The -buildtree option removes that step:

    ./test.sh -buildtree mysqlprotocol

It does three things:

- runs make stagemodules to copy every module this build produced into
  test/stagedmodules, one flat directory.
- runs the build tree's own sqlr-start, sqlr-stop and sqlrsh, out of
  src/server and src/cmdline, rather than the ones in the prefix.
- passes -libexecdir test/stagedmodules and -bindir src/server to them, so
  the listener, the connections and the scaler all come out of the tree too,
  and all of them load the staged modules.

Nothing is written to the install prefix, and no sudo is needed.

What -buildtree covers is the server side - the listener, the connections,
the scaler, and every module they load.  It does not rebuild or relink the
client API test programs, which are linked against the installed client
libraries.  That is the right split for the protocol suites, where the client
is a third party one anyway, and for any change to a module.  A change to a
client API still needs an install.

The staged directory is rebuilt from scratch on every -buildtree run, so a
module that was rebuilt since the last run is picked up.  A module that this
build did not produce is a hard error rather than a silent omission - see
stagemodules.sh.

## Running on ports nothing else is using

Each protocol suite listens on the real database's well known port - 1521 for
oracle, 3306 for mysql, 3050 for firebird, and so on.  There is one of each
per host.  So two people, or two sessions, cannot run the same suite at the
same time, and starting one disturbs anyone already using that port.

The -randomports option moves a run out of the way:

    ./test.sh -randomports mysqlprotocol

It picks a free port for every protocol listener, using the freeport helper,
and then rewrites both ends to agree on them:

- the server side - the instance configs, regenerated from their .conf.in
  templates:
  - sqlrelay.conf.d/*protocol*.conf
  - sqlrelay.conf.d/freetds/etc/freetds.conf, which freetds and tsql read the
    port from.  There is no command line option or environment variable for
    it, so this file has to be rewritten or the tds suites stay pinned.
- the client side:
  - protocol/oracle/tnsnames.ora and protocol/teradata/odbc.ini, which are
    client config files, regenerated the same way.
  - the environment - MYSQLPROTOCOLPORT1, POSTGRESQLPROTOCOLPORT1 and the
    rest are exported before the test programs run.  Each test program falls
    back to its old well known port when its variable is unset.

One name drives both ends.  The token in a .conf.in file, the make variable
in test/Makefile, and the environment variable the test program reads all
have the same name.

Ports are picked one at a time rather than as a base plus offsets.  Only the
ports freeport actually bound are known to be free; a run of ports starting
at a free one is not free just because its first port is.  Nothing needs them
contiguous, since every port is its own token.

The rewritten files stay behind after the run.  To put the well known ports
back, run configure again, or:

    ./config.status

## Both at once

The two options combine, and together they give a protocol suite run that
touches neither the shared install prefix nor any well known port:

    ./test.sh -buildtree -randomports mysqlprotocol

That is the way to verify a change to a protocol, connection, auth or filter
module in isolation.

## Defaults are unchanged

Neither option is on by default.  With neither of them, test.sh runs exactly
what it always ran - the installed programs, and the ports configure baked
into the config files.

## What the harness needs from the host

- A reachable backend server for each instance being tested.  Every instance
  connects to a database named after the host, so instances that share a
  backend also share a schema and the same testtable.  They have to run one
  at a time relative to each other.  test.sh -h lists which groups those are.
- odbc.ini and odbcinst.ini entries for the odbc instances.
- $SYBASE/interfaces entries for the SAP ct-lib instances.

An instance whose connection module was not built, or whose prerequisites are
missing, is skipped rather than failed.
