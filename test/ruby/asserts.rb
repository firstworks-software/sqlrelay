# Copyright (c) David Muse
# See the file COPYING for more information.

# keep stdout unbuffered so a crash is logged where it actually happens
$stdout.sync=true

$status=0

$success="\e[32msuccess\e[0m"
$failure="\e[31mfailure\e[0m"
$alltestssucceeded="\n\e[34mAll tests succeeded\e[0m"
$sometestsfailed="\n\e[38;5;208mSome tests failed\e[0m"

$cur=nil
$con=nil
$secondcur=nil
$secondcon=nil

def setConnection(c)
	$con=c
end

def setCursor(c)
	$cur=c
end

def setSecondConnection(c)
	$secondcon=c
end

def setSecondCursor(c)
	$secondcur=c
end

def printErrors()
	if $cur
		err=$cur.errorMessage()
		if err
			puts err
			return
		end
	end
	if $secondcur
		err=$secondcur.errorMessage()
		if err
			puts err
			return
		end
	end
	if $con
		err=$con.errorMessage()
		if err
			puts err
			return
		end
	end
	if $secondcon
		err=$secondcon.errorMessage()
		if err
			puts err
			return
		end
	end
end

def assertEqual(actual,expected)
	if actual==expected
		print $success , " "
	else
		puts $failure
		print actual , " != " , expected , "\n"
		printErrors()
		$status=1
	end
end

def assertEqualLen(actual,expected,length)
	if expected.nil?
		if actual.nil?
			print $success , " "
		else
			puts $failure
			print actual , " != " , expected , "\n"
			printErrors()
			$status=1
		end
		return
	end
	if actual && actual[0,length]==expected[0,length]
		print $success , " "
	else
		puts $failure
		print actual , " != " , expected , "\n"
		printErrors()
		$status=1
	end
end

def assertTrue(actual)
	if actual==1 || actual==true
		print $success , " "
	else
		puts $failure
		print actual , " != 1\n"
		printErrors()
		$status=1
	end
end

def assertFalse(actual)
	if actual==0 || actual==false || actual.nil?
		print $success , " "
	else
		puts $failure
		print actual , " != 0\n"
		printErrors()
		$status=1
	end
end

def assertStartsWith(actual,prefix)
	if actual && actual.start_with?(prefix)
		print $success , " "
	else
		puts $failure
		print actual , " doesn't start with " , prefix , "\n"
		printErrors()
		$status=1
	end
end

def assertInResultSet(cur,column,value)
	for i in 0..cur.rowCount()-1
		if cur.getField(i,column)==value
			print $success , " "
			return
		end
	end
	puts $failure
	print "\"" , value , "\" not found in column \"" , column , "\"\n"
	printErrors()
	$status=1
end

def reportTestStatus()
	if $status==0
		puts $alltestssucceeded
	else
		puts $sometestsfailed
	end
end
