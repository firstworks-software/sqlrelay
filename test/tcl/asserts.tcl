# Copyright (c) David Muse
# See the file COPYING for more information.

set status 0
set con ""
set cur ""
set secondcon ""
set secondcur ""

set success "\033\[32msuccess\033\[0m"
set failure "\033\[31mfailure\033\[0m"
set alltestssucceeded "\n\033\[34mAll tests succeeded\033\[0m"
set sometestsfailed "\n\033\[38;5;208mSome tests failed\033\[0m"

proc setConnection {c} {
	global con
	set con $c
}

proc setCursor {c} {
	global cur
	set cur $c
}

proc setSecondConnection {c} {
	global secondcon
	set secondcon $c
}

proc setSecondCursor {c} {
	global secondcur
	set secondcur $c
}

proc printErrors {} {

	global cur
	global secondcur
	global con
	global secondcon

	if {$cur ne ""} {
		set err [$cur errorMessage]
		if {$err ne ""} {
			puts $err
			return
		}
	}
	if {$secondcur ne ""} {
		set err [$secondcur errorMessage]
		if {$err ne ""} {
			puts $err
			return
		}
	}
	if {$con ne ""} {
		set err [$con errorMessage]
		if {$err ne ""} {
			puts $err
			return
		}
	}
	if {$secondcon ne ""} {
		set err [$secondcon errorMessage]
		if {$err ne ""} {
			puts $err
			return
		}
	}
}

proc assertUndef {actual} {

	global status
	global success
	global failure
	switch $actual "" {
		puts -nonewline "$success "
	} default {
		puts $failure
		puts "$actual != (undef)"
		printErrors
		set status 1
	}
}

proc assertEqual {actual expected} {

	global status
	global success
	global failure
	if {$actual eq $expected} {
		puts -nonewline "$success "
	} else {
		puts $failure
		puts "$actual != $expected"
		printErrors
		set status 1
	}
}

# Compare the first $length bytes of actual and expected.
# If expected is empty then actual must also be empty.
proc assertEqualLen {actual expected length} {

	global status
	global success
	global failure
	if {$expected eq ""} {
		if {$actual eq ""} {
			puts -nonewline "$success "
		} else {
			puts $failure
			puts "$actual != $expected"
			printErrors
			set status 1
		}
		return
	}
	if {[string range $actual 0 [expr $length - 1]] eq
		[string range $expected 0 [expr $length - 1]]} {
		puts -nonewline "$success "
	} else {
		puts $failure
		puts "$actual != $expected"
		printErrors
		set status 1
	}
}

proc assertTrue {actual} {

	global status
	global success
	global failure
	if {$actual==1} {
		puts -nonewline "$success "
	} else {
		puts $failure
		puts "$actual != 1"
		printErrors
		set status 1
	}
}

proc assertFalse {actual} {

	global status
	global success
	global failure
	if {$actual==0} {
		puts -nonewline "$success "
	} else {
		puts $failure
		puts "$actual != 0"
		printErrors
		set status 1
	}
}

proc assertInResultSet {cur column value} {

	global status
	global success
	global failure
	for {set i 0} {$i<[$cur rowCount]} {incr i} {
		if {[string equal [$cur getFieldByName $i $column] $value]} {
			puts -nonewline "$success "
			return
		}
	}
	puts $failure
	puts "\"$value\" not found in column \"$column\""
	printErrors
	set status 1
}

proc reportTestStatus {} {

	global status
	global alltestssucceeded
	global sometestsfailed
	if {$status==0} {
		puts $alltestssucceeded
	} else {
		puts $sometestsfailed
	}
}
