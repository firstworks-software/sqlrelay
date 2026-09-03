#!/usr/bin/env python3
# Copyright (c) David Muse
# See the file COPYING for more information.

# For #9635 step 1: run one query against the oracle protocol module
# through python-oracledb's thin driver (no OCI, no Oracle client
# libraries anywhere on the path - a different client than ociselect's
# OCI client) and print what comes back, or report a timeout if the
# module hangs instead of answering.
#
#   ./mixedtypetest.py HOST PORT SID USER PASSWORD QUERY
#
# HOST/PORT must name a listener at the module's default serverversion
# (12.1) - see mixedtypetest.conf and the "Server Version" section of
# doc/admin/configguide.wt.  python-oracledb doesn't support an
# serverversion="11.2" server at all, and against one it hangs forever at
# login, with no error - a different hang than #9635, but indistinguishable
# from the outside, so pointing this at the wrong listener mis-reproduces
# the ticket.
#
# connection.call_timeout bounds the query itself, not the login, so a
# real #9635 hang is reported as a timeout rather than hanging this
# script (and whatever ran it) forever.  10 seconds is generous - every
# one of the six queries in #9635 answers in well under a second when it
# answers at all.
#
# oracledb runs in thin mode by default (no Oracle client libraries
# involved) as long as nothing calls oracledb.init_oracle_client() -
# nothing here does.

import sys

import oracledb

CALL_TIMEOUT_MS = 10000


def main():
	if len(sys.argv) != 7:
		sys.stderr.write(
			"usage: %s HOST PORT SID USER PASSWORD QUERY\n" %
			sys.argv[0])
		return 1

	host = sys.argv[1]
	port = int(sys.argv[2])
	sid = sys.argv[3]
	user = sys.argv[4]
	password = sys.argv[5]
	query = sys.argv[6]

	sys.stdout.write("connecting to %s:%d (sid=%s) as %s "
				"(thin mode)...\n" % (host, port, sid, user))
	sys.stdout.flush()

	connection = oracledb.connect(user=user, password=password,
					host=host, port=port, sid=sid)

	sys.stdout.write("login ok\n")

	connection.call_timeout = CALL_TIMEOUT_MS

	sys.stdout.write("running: %s\n" % query)
	sys.stdout.flush()

	cursor = connection.cursor()
	try:
		cursor.execute(query)
		for row in cursor:
			sys.stdout.write("row: %s\n" % (row,))
	except oracledb.Error as e:
		sys.stdout.write("query failed or timed out: %s\n" % e)
		return 1

	sys.stdout.write("done\n")
	return 0


if __name__ == "__main__":
	sys.exit(main())
