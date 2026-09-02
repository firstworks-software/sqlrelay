#! /usr/bin/env python

# Copyright (c) David Muse
# See the file COPYING for more information.

from SQLRelay import PySQLRDB
import sys


def main():

	# instantiation
	print("INSTANTIATION")
	con=PySQLRDB.connect(
		"sqlrelay",9003,
		"/tmp/postgresql.socket",
		"testuser","testpassword",
		0,1,
		# autocommit on, so an expected error doesn't leave
		# postgresql's transaction aborted, with every statement
		# after it failing with 25P02
		autocommit='yes')
	cur=con.cursor()
	print()
	print()


	# error sqlstate
	print("ERROR SQLSTATE")
	try:
		cur.execute("drop table testtable")
	except (PySQLRDB.DatabaseError):
		pass
	cur.execute("create table testtable (col1 int)")

	# postgresql reports a duplicate table as 42P07, in every locale
	try:
		cur.execute("create table testtable (col1 int)")
		raise Exception("the duplicate create table should have failed")
	except (PySQLRDB.DatabaseError) as de:
		print(de.sqlstate)
		assert de.sqlstate=="42P07"

	# and an undefined table as 42P01
	try:
		cur.execute("select * from nonexistenttable")
		raise Exception("the select should have failed")
	except (PySQLRDB.DatabaseError) as de:
		print(de.sqlstate)
		assert de.sqlstate=="42P01"

	cur.execute("drop table testtable")
	print()
	print()


	# error sqlstate - executemany
	print("ERROR SQLSTATE - EXECUTEMANY")
	try:
		cur.executemany("insert into nonexistenttable values ($1)",
					[{'1':1}])
		raise Exception("the insert should have failed")
	except (PySQLRDB.DatabaseError) as de:
		print(de.sqlstate)
		assert de.sqlstate=="42P01"
	print()
	print()


	cur.close()
	con.close()
	del cur
	del con

if __name__ == '__main__':
	main()
	sys.exit(0)
