#!/usr/bin/tclsh

# This tool helps detect memory leaks in a -ZA trace from Ghostscript.
# It reads a memory trace from stdin and prints unmatched allocations on
# stdout.  Currently it is slightly specialized for our PCL5 environment,
# in that it looks for the string "memory allocated" in the trace to mark
# the beginning and end of the interesting region.

# We keep track of the trace in the following global arrays:
#	A(<addr>) holds a string of the form line#:line of the last
#	   allocation event that allocated a block at address addr.
#	lines(<i>) holds other interesting lines of the input trace file -
#	  the "allocated" and "Final" lines, and anomalous alloc/free events.
#	next holds the line number of the next line.

proc init_leaks {} {
    global A lines next
    catch {unset A}
    catch {unset lines}
    set next 0
}

proc add1+0 {il addr} {global A;set A($addr) $il}
proc add1+1 {il addr} {
    global A lines
    regexp {^(.*):(.*)$} $A($addr) all i l
    puts "**** Warning: reallocation: $il"
    puts "**** Previous allocation: $A($addr)"
    set lines($i) $l
    set A($addr) $il
}
proc add1-1 {il addr} {global A;unset A($addr)}
proc add1-0 {il addr} {
    if {$addr == "0"} {return}
    regexp {^([0-9]+):(.*)$} $il ignore i l
    puts "**** Warning: no alloc event: $il"
    set lines($i) $l
}
proc add0+0 {il addr} {
    if [regexp {Final|allocated} $il] {
	uplevel {set lines($n) $l}
	if [regexp "Final time" $il] {uplevel {set go 0}}
    }
}
proc add0+1 {il addr} [info body add0+0]
proc add0-0 {il addr} [info body add0+0]
proc add0-1 {il addr} [info body add0+0]

proc read_trace {{fname %stdin}} {
    global lines A next
    set n $next
    if {$fname == "%stdin"} {
	set in stdin
    } else {
	set in [open $fname]
    }
	# Skip to the first "allocated" line
    while 1 {
	gets $in l
	if [regexp "memory allocated" $l] break
	incr n
    }
    set lines($n) $l
    incr n
    set sign +			;# arbitrary, + or -
    set addr ""			;# arbitrary
    set go 1
    while {$go} {
	gets $in l
	add[regexp {^\[a.*([+-]).*\].*0x([0-9a-f]+)} $l all sign addr]${sign}[info exists A($addr)] "$n:$l" $addr
	incr n
    }
    incr n -1
    set lines($n) $l
    incr n
    if {$fname != "%stdin"} {
	close $in
    }
    set next $n
}

proc print_leaks {} {
    global A lines
    foreach addr [array names A] {
	regexp {^([0-9]+):(.*)$} $A($addr) all i l
	set lines($i) $l
    }
    foreach i [lsort -integer [array names lines]] {
	puts "$i: $lines($i)"
    }
}

if {$argv0 != "tclsh"} {
    init_leaks
    read_trace
    print_leaks
}

