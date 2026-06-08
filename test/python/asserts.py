#! /usr/bin/env python

# Copyright (c) David Muse
# See the file COPYING for more information.

import sys

status=0

success="\033[32msuccess\033[0m"
failure="\033[31mfailure\033[0m"
alltestssucceeded="\n\033[34mAll tests succeeded\033[0m"
sometestsfailed="\n\033[38;5;208mSome tests failed\033[0m"

cur=None
con=None
secondcur=None
secondcon=None

def setConnection(c):
	global con
	con=c

def setCursor(c):
	global cur
	cur=c

def setSecondConnection(c):
	global secondcon
	secondcon=c

def setSecondCursor(c):
	global secondcur
	secondcur=c

def printErrors():
	global cur
	global con
	global secondcur
	global secondcon
	if cur:
		err=cur.errorMessage()
		if err:
			print(err)
			return
	if secondcur:
		err=secondcur.errorMessage()
		if err:
			print(err)
			return
	if con:
		err=con.errorMessage()
		if err:
			print(err)
			return
	if secondcon:
		err=secondcon.errorMessage()
		if err:
			print(err)
			return

def assertEquals(actual,expected):
	global status
	if actual==expected:
		print(success,end=" ")
	else:
		print(failure)
		print("wanted",type(expected),":",expected)
		print("got   ",type(actual),":",actual)
		printErrors()
		status=1

def assertEqualsBytes(actual,expected,length):
	global status
	if actual is None and expected is None:
		print(success,end=" ")
		return
	if actual is None or expected is None:
		print(failure)
		print("wanted",expected)
		print("got   ",actual)
		printErrors()
		status=1
		return
	if actual[:length]==expected[:length]:
		print(success,end=" ")
	else:
		print(failure)
		print("wanted",expected[:length])
		print("got   ",actual[:length])
		printErrors()
		status=1

def assertTrue(actual):
	global status
	if actual:
		print(success,end=" ")
	else:
		print(failure)
		print("wanted","true")
		print("got   ",type(actual),":",actual)
		printErrors()
		status=1

def assertFalse(actual):
	global status
	if not actual:
		print(success,end=" ")
	else:
		print(failure)
		print("wanted","false")
		print("got   ",type(actual),":",actual)
		printErrors()
		status=1

def assertNone(actual):
	global status
	if actual is None:
		print(success,end=" ")
	else:
		print(failure)
		print("wanted",None)
		print("got   ",type(actual),":",actual)
		printErrors()
		status=1

def assertInResultSet(cur,column,value):
	global status
	for i in range(cur.rowCount()):
		if cur.getField(i,column)==value:
			print(success,end=" ")
			return
	print(failure)
	print("\""+value+"\" not found in column \""+column+"\"")
	printErrors()
	status=1

def reportTestStatus():
	global status
	if status==0:
		print(alltestssucceeded)
	else:
		print(sometestsfailed)

# keep the old name for any tests that still reference it
assertEqual=assertEquals
