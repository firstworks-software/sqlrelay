#!/usr/bin/env python3
# Copyright (c) David Muse
# See the file COPYING for more information.

# For #10314: drive a python-oracledb thin session (a modern, non-OCI7
# client) through a fixed sequence of transaction-affecting calls on one
# connection, printing Connection.transaction_in_progress after each
# step. python-oracledb sets that attribute from bit 0x02 of the "end of
# call status" the server returns in every summary object (TTC message
# type 4) and status message (type 9), so run through oraproxy and
# decoded with oradecode, the printed value lines up with the call
# status in each reply.
#
#   ./callstatustest.py HOST PORT SERVICE USER PASSWORD [--sid]
#                                          [--sequence=main|bit4] [--nolob]
#
# SERVICE is a service name unless --sid is given, in which case it is a
# SID. --sequence=main (the default) is the broad sweep: select, ddl,
# dml, commit, rollback, failing ddl and dml, autocommit, merge, select
# for update, pl/sql, re-execute, executemany, parse-only, a lob read, a
# multi-round-trip fetch and a close with a transaction open.
# --sequence=bit4 is a narrower follow-up that pins down when the 0x04
# bit of the call status is set, which the main sweep showed isn't the
# session-wide flag the OCI7 captures (#10295/#10315) made it look like:
# re-executes of cursors opened before and after a commit, and failing
# ddl before and after the session's first transaction ends. --nolob
# skips the main sweep's lob read (s43), for a server that can't answer
# one and would leave the rest of the sweep unrun. HOST/PORT must be a real 12.1+ server or a sqlr-listener at the
# oracle protocol module's default serverversion (12.1) - python-oracledb
# thin doesn't support older servers (see mixedtypetest.py).
#
# Every statement carries a trailing /* sNN */ comment naming its step,
# so a step can be found in the decode by searching for its tag.
#
# Uses (drops and recreates) a scratch table, protocoltest10314, and
# leaves it behind - oraproxy relays only one connection, so there is no
# second session to drop it from.

import sys

import oracledb

CALL_TIMEOUT_MS = 10000
TABLE = "protocoltest10314"

conn = None


def show(tag, label):
	sys.stdout.write("%-4s %-52s txn_in_progress=%s\n" %
			(tag, label, conn.transaction_in_progress))
	sys.stdout.flush()


def run(cursor, tag, label, sql, ignore=None):
	sql = "%s /* %s */" % (sql, tag)
	try:
		cursor.execute(sql)
		if cursor.description is not None:
			rows = cursor.fetchall()
			if len(rows) == 1 and len(rows[0]) == 1:
				label = label + " = " + str(rows[0][0])
	except oracledb.Error as e:
		msg = str(e).split("\n")[0]
		if ignore is not None and ignore in msg:
			label = label + " (" + ignore + ", ignored)"
		else:
			label = label + " (error: " + msg + ")"
	show(tag, label)


def step(tag, label, func):
	try:
		func()
	except oracledb.Error as e:
		label = label + " (error: " + str(e).split("\n")[0] + ")"
	show(tag, label)


def rerun(cursor, tag, label, sql):
	try:
		cursor.execute(sql)
		if cursor.description is not None:
			cursor.fetchall()
	except oracledb.Error as e:
		label = label + " (error: " + str(e).split("\n")[0] + ")"
	show(tag, label)


def main():
	global conn

	args = [a for a in sys.argv[1:] if not a.startswith("--")]
	usesid = "--sid" in sys.argv[1:]
	nolob = "--nolob" in sys.argv[1:]
	sequence = "main"
	for a in sys.argv[1:]:
		if a.startswith("--sequence="):
			sequence = a[len("--sequence="):]
	if len(args) != 5 or sequence not in ("main", "bit4"):
		sys.stderr.write("usage: %s HOST PORT SERVICE USER "
					"PASSWORD [--sid] "
					"[--sequence=main|bit4] [--nolob]\n" %
					sys.argv[0])
		return 1

	host = args[0]
	port = int(args[1])
	service = args[2]
	user = args[3]
	password = args[4]

	if usesid:
		conn = oracledb.connect(user=user, password=password,
					host=host, port=port, sid=service)
	else:
		conn = oracledb.connect(user=user, password=password,
					host=host, port=port,
					service_name=service)
	conn.call_timeout = CALL_TIMEOUT_MS
	conn.autocommit = False
	show("s00", "login")

	if sequence == "bit4":
		bit4()
	else:
		mainsequence(nolob)

	# close with a txn open
	conn.close()
	sys.stdout.write("end  close with txn open: done\n")
	return 0


def mainsequence(nolob):
	cur = conn.cursor()

	# idle select, DDL from idle
	run(cur, "s01", "select from dual", "select 1 from dual")
	run(cur, "s02", "drop table (DDL)", "drop table " + TABLE,
		"ORA-00942")
	run(cur, "s03", "create table (DDL)",
		"create table " + TABLE + " (id number, name varchar2(32))")

	# DML, DML, select, commit
	run(cur, "s04", "insert",
		"insert into " + TABLE + " values (4,'s04')")
	run(cur, "s05", "second insert",
		"insert into " + TABLE + " values (5,'s05')")
	run(cur, "s06", "select with txn open",
		"select count(*) from " + TABLE)
	step("s07", "commit (TTI call)", conn.commit)

	# DML, rollback
	run(cur, "s08", "insert",
		"insert into " + TABLE + " values (8,'s08')")
	step("s09", "rollback (TTI call)", conn.rollback)

	# DDL with a txn open
	run(cur, "s10", "insert",
		"insert into " + TABLE + " values (10,'s10')")
	run(cur, "s11", "comment on table (DDL, txn open)",
		"comment on table " + TABLE + " is 's11'")

	# failing DDL from idle-after-commit, and with a txn open
	run(cur, "s12", "failing drop table (DDL, idle)",
		"drop table " + TABLE + "_nx")
	run(cur, "s13", "insert",
		"insert into " + TABLE + " values (13,'s13')")
	run(cur, "s14", "failing drop table (DDL, txn open)",
		"drop table " + TABLE + "_nx")
	run(cur, "s15", "select s13 row (committed by s14?)",
		"select count(*) from " + TABLE + " where id=13")
	step("s16", "rollback (TTI call)", conn.rollback)
	run(cur, "s17", "select s13 row after rollback",
		"select count(*) from " + TABLE + " where id=13")

	# failed DML without and with a txn open
	run(cur, "s18", "failed insert (idle)",
		"insert into " + TABLE + " values ('abc','s18')")
	run(cur, "s19", "insert",
		"insert into " + TABLE + " values (19,'s19')")
	run(cur, "s20", "failed insert (txn open)",
		"insert into " + TABLE + " values ('abc','s20')")
	step("s21", "rollback (TTI call)", conn.rollback)

	# autocommit on
	conn.autocommit = True
	show("s22", "autocommit=True (no round trip)")
	run(cur, "s23", "insert under autocommit",
		"insert into " + TABLE + " values (23,'s23')")
	run(cur, "s24", "select under autocommit",
		"select count(*) from " + TABLE)
	conn.autocommit = False
	show("s25", "autocommit=False (no round trip)")

	# merge, select for update
	run(cur, "s26", "merge",
		"merge into " + TABLE + " t using (select 26 id from dual) s "
		"on (t.id=s.id) when not matched then "
		"insert (id,name) values (26,'s26')")
	step("s27", "commit (TTI call)", conn.commit)
	run(cur, "s28", "select for update",
		"select id from " + TABLE + " where id=4 for update")
	step("s29", "rollback (TTI call)", conn.rollback)

	# PL/SQL
	run(cur, "s30", "plsql null block", "begin null; end;")
	run(cur, "s31", "plsql block doing an insert",
		"begin insert into " + TABLE + " values (31,'s31'); end;")
	step("s32", "rollback (TTI call)", conn.rollback)

	# re-execute - identical SQL on the same cursor, so the driver
	# sends a reexecute rather than a full execute the second time
	sql = "insert into " + TABLE + " values (33,'s33') /* s33 */"
	rerun(cur, "s33", "insert (full execute)", sql)
	step("s34", "commit (TTI call)", conn.commit)
	rerun(cur, "s35", "same insert again (reexecute, idle)", sql)
	rerun(cur, "s36", "same insert again (reexecute, txn open)", sql)
	step("s37", "rollback (TTI call)", conn.rollback)
	sql = "select 1 from dual /* s38 */"
	rerun(cur, "s38", "select (full execute)", sql)
	rerun(cur, "s39", "same select again (reexecute and fetch)", sql)

	# executemany
	step("s40", "executemany insert (3 rows)",
		lambda: cur.executemany("insert into " + TABLE +
					" values (:1,:2) /* s40 */",
					[(40, "s40a"), (41, "s40b"),
					(42, "s40c")]))
	step("s41", "commit (TTI call)", conn.commit)

	# parse only
	cur2 = conn.cursor()
	step("s42", "cursor.parse (parse-only execute)",
		lambda: cur2.parse("select id, name from " + TABLE +
					" /* s42 */"))

	# LOB read
	if not nolob:
		step("s43", "clob fetch + lob read (LOB op)",
			lambda: readlob(cur2))

	# multi-round-trip fetch with a txn open
	run(cur, "s44", "insert",
		"insert into " + TABLE + " values (44,'s44')")
	cur3 = conn.cursor()
	cur3.arraysize = 2
	cur3.prefetchrows = 2
	try:
		cur3.execute("select id from " + TABLE +
				" order by id /* s45 */")
	except oracledb.Error as e:
		sys.stdout.write("s45  error: %s\n" % str(e).split("\n")[0])
		return
	show("s45", "select, arraysize 2 (execute + prefetch)")
	n = 0
	while True:
		try:
			rows = cur3.fetchmany()
		except oracledb.Error as e:
			sys.stdout.write("s45x fetchmany error: %s\n" %
						str(e).split("\n")[0])
			break
		if not rows:
			break
		n = n + 1
		show("s45" + chr(ord("a") + n - 1),
			"fetchmany -> %d row(s)" % len(rows))
		if n >= 5:
			break
	cur3.close()


def readlob(cursor):
	cursor.execute("select to_clob('s43') from dual /* s43 */")
	cursor.fetchone()[0].read()


def bit4():
	ca = conn.cursor()
	cb = conn.cursor()
	nx = "drop table " + TABLE + "_nx"
	run(conn.cursor(), "p00", "drop table (DDL)", "drop table " + TABLE,
		"ORA-00942")
	run(conn.cursor(), "p00b", "drop table (DDL)",
		"drop table " + TABLE + "b", "ORA-00942")
	run(conn.cursor(), "p00c", "create table (DDL)",
		"create table " + TABLE + " (id number, name varchar2(32))")

	# ca/cb keep their server cursors, so executing the same text on
	# them again sends a reexecute rather than a full execute
	sela = "select 1 from dual /* p01 */"
	insb = "insert into " + TABLE + " values (3,'p03') /* p03 */"
	rerun(ca, "p01", "select on ca (full execute)", sela)
	run(conn.cursor(), "p02", "failing drop (DDL, no txn yet)", nx)
	rerun(cb, "p03", "insert on cb (full execute)", insb)
	step("p04", "commit (TTI call)", conn.commit)
	rerun(ca, "p05", "select on ca again (reexecute+fetch)", sela)
	run(conn.cursor(), "p06", "select on a new cursor",
		"select 2 from dual")
	rerun(ca, "p07", "select on ca again (reexecute+fetch)", sela)
	rerun(cb, "p08", "insert on cb again (reexecute, idle)", insb)
	step("p09", "rollback (TTI call)", conn.rollback)
	run(conn.cursor(), "p10", "failing drop (DDL, idle after rollback)",
		nx)
	run(conn.cursor(), "p11", "insert on a new cursor",
		"insert into " + TABLE + " values (11,'p11')")
	run(conn.cursor(), "p12", "create table b (DDL, txn open)",
		"create table " + TABLE + "b (id number)")
	rerun(ca, "p13", "select on ca again (reexecute+fetch)", sela)
	run(conn.cursor(), "p14", "failing drop (DDL, idle after ddl)", nx)
	run(conn.cursor(), "p15", "failing drop again (DDL)", nx)
	run(conn.cursor(), "p16", "drop table b (DDL, idle)",
		"drop table " + TABLE + "b")
	run(conn.cursor(), "p17", "select on a new cursor",
		"select 3 from dual")
	run(conn.cursor(), "p18", "failing drop (DDL, idle after ddl+select)",
		nx)
	rerun(cb, "p19", "insert on cb again (reexecute, idle)", insb)
	run(conn.cursor(), "p20", "insert on a new cursor (txn open)",
		"insert into " + TABLE + " values (20,'p20')")
	rerun(cb, "p21", "insert on cb again (reexecute, txn open)", insb)
	step("p22", "commit (TTI call)", conn.commit)
	run(conn.cursor(), "p23", "failing insert on a new cursor (idle)",
		"insert into " + TABLE + " values ('abc','p23')")
	run(conn.cursor(), "p24", "insert on a new cursor (after commit)",
		"insert into " + TABLE + " values (24,'p24')")


if __name__ == "__main__":
	sys.exit(main())
