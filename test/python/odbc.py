#! /usr/bin/env python

# Copyright (c) David Muse
# See the file COPYING for more information.

from SQLRelay import PySQLRClient
import sys
from assert import *
import string


def main():

	PySQLRClient.getNumericFieldsAsNumbers()

	# instantiation
	con=PySQLRClient.sqlrconnection("sqlrelay",9000,
						"/tmp/test.socket",
						"testuser","testpassword")
	cur=PySQLRClient.sqlrcursor(con)

	# get database type
	print("IDENTIFY: ")
	assertEqual(con.identify(),"odbc")

	# ping
	print("PING: ")
	assertTrue(con.ping())
	print()

	# drop existing table
	cur.sendQuery("drop table testtable")

	# create a new table
	print("CREATE TEMPTABLE: ")
	assertTrue(cur.sendQuery("create table testtable (testint int, testchar char(40), testvarchar varchar(40), testdate date)"))
	print()

	print("INSERT: ")
	assertTrue(cur.sendQuery("insert into testtable values (1,'testchar1','testvarchar1','01-JAN-2001')"))
	assertTrue(cur.sendQuery("insert into testtable values (2,'testchar2','testvarchar2','02-JAN-2002')"))
	assertTrue(cur.sendQuery("insert into testtable values (3,'testchar3','testvarchar3','03-JAN-2003')"))
	assertTrue(cur.sendQuery("insert into testtable values (4,'testchar4','testvarchar4','04-JAN-2004')"))
	assertTrue(cur.sendQuery("insert into testtable values (5,'testchar5','testvarchar5','05-JAN-2005')"))
	assertTrue(cur.sendQuery("insert into testtable values (6,'testchar6','testvarchar6','06-JAN-2006')"))
	assertTrue(cur.sendQuery("insert into testtable values (7,'testchar7','testvarchar7','07-JAN-2007')"))
	assertTrue(cur.sendQuery("insert into testtable values (8,'testchar8','testvarchar8','08-JAN-2008')"))
	print()

	print("FINISHED SUSPENDED SESSION: ")
	assertTrue(cur.sendQuery("select * from testtable order by testint"))
	assertEqual(cur.getField(4,0),"5")
	assertEqual(cur.getField(5,0),"6")
	assertEqual(cur.getField(6,0),"7")
	assertEqual(cur.getField(7,0),"8")
	id=cur.getResultSetId()
	cur.suspendResultSet()
	assertTrue(con.suspendSession())
	port=con.getConnectionPort()
	socket=con.getConnectionSocket()
	assertTrue(con.resumeSession(port,socket))
	assertTrue(cur.resumeResultSet(id))
	assertEqual(cur.getField(4,0),None)
	assertEqual(cur.getField(5,0),None)
	assertEqual(cur.getField(6,0),None)
	assertEqual(cur.getField(7,0),None)
	print()

	# drop existing table
	cur.sendQuery("drop table testtable")

	# invalid queries...
	print("INVALID QUERIES: ")
	assertFalse(cur.sendQuery("select * from testtable"))
	assertFalse(cur.sendQuery("select * from testtable"))
	assertFalse(cur.sendQuery("select * from testtable"))
	assertFalse(cur.sendQuery("select * from testtable"))
	print()
	assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
	assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
	assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
	assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"))
	print()
	assertFalse(cur.sendQuery("create table testtable"))
	assertFalse(cur.sendQuery("create table testtable"))
	assertFalse(cur.sendQuery("create table testtable"))
	assertFalse(cur.sendQuery("create table testtable"))
	print()

if __name__ == "__main__":
	main()
	sys.exit(0)
