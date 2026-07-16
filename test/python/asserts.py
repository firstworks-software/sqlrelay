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

# python 2's print is a statement, so the python 3 print() forms used here -
# print(), print(a,b) and print(x,end=...) - don't parse or behave the same
# there.  route output through this helper so the tests run unchanged under
# both python 2 and python 3.
def output(*args,**kwargs):
	sys.stdout.write(" ".join([str(a) for a in args])+kwargs.get("end","\n"))

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
			output(err)
			return
	if secondcur:
		err=secondcur.errorMessage()
		if err:
			output(err)
			return
	if con:
		err=con.errorMessage()
		if err:
			output(err)
			return
	if secondcon:
		err=secondcon.errorMessage()
		if err:
			output(err)
			return

def assertEquals(actual,expected):
	global status
	if actual==expected:
		output(success,end=" ")
	else:
		output(failure)
		output("wanted",type(expected),":",expected)
		output("got   ",type(actual),":",actual)
		printErrors()
		status=1

def assertEqualsBytes(actual,expected,length):
	global status
	if actual is None and expected is None:
		output(success,end=" ")
		return
	if actual is None or expected is None:
		output(failure)
		output("wanted",expected)
		output("got   ",actual)
		printErrors()
		status=1
		return
	if actual[:length]==expected[:length]:
		output(success,end=" ")
	else:
		output(failure)
		output("wanted",expected[:length])
		output("got   ",actual[:length])
		printErrors()
		status=1

def assertTrue(actual):
	global status
	if actual:
		output(success,end=" ")
	else:
		output(failure)
		output("wanted","true")
		output("got   ",type(actual),":",actual)
		printErrors()
		status=1

def assertFalse(actual):
	global status
	if not actual:
		output(success,end=" ")
	else:
		output(failure)
		output("wanted","false")
		output("got   ",type(actual),":",actual)
		printErrors()
		status=1

def assertNone(actual):
	global status
	if actual is None:
		output(success,end=" ")
	else:
		output(failure)
		output("wanted",None)
		output("got   ",type(actual),":",actual)
		printErrors()
		status=1

def assertStartsWith(actual,prefix):
	global status
	if actual is not None and actual.startswith(prefix):
		output(success,end=" ")
	else:
		output(failure)
		output("wanted prefix",prefix)
		output("got         ",type(actual),":",actual)
		printErrors()
		status=1

def assertInResultSet(cur,column,value):
	global status
	for i in range(cur.rowCount()):
		if cur.getField(i,column)==value:
			output(success,end=" ")
			return
	output(failure)
	output("\""+value+"\" not found in column \""+column+"\"")
	printErrors()
	status=1

def normalizeMoney(v):
	if v is None:
		return v
	v=v.replace("$","").replace(",","")
	if "." in v:
		v=v.rstrip("0")
		if v.endswith("."):
			v=v[:-1]
	return v

def assertMoneyEquals(actual,expected):
	global status
	# old freetds renders money with 2 decimal places instead of 4
	if actual is None or expected is None:
		assertEquals(actual,expected)
		return
	if normalizeMoney(actual)==normalizeMoney(expected):
		output(success,end=" ")
	else:
		output(failure)
		output("wanted",type(expected),":",expected)
		output("got   ",type(actual),":",actual)
		printErrors()
		status=1

def assertMoneyLengthEquals(actual,expected):
	global status
	# old freetds renders money with 2 decimal places instead of 4
	if actual is not None and (actual==expected or actual==expected-2):
		output(success,end=" ")
	else:
		output(failure)
		output("wanted",type(expected),":",expected)
		output("got   ",type(actual),":",actual)
		printErrors()
		status=1

def reportTestStatus():
	global status
	if status==0:
		output(alltestssucceeded)
	else:
		output(sometestsfailed)

# keep the old name for any tests that still reference it
assertEqual=assertEquals
