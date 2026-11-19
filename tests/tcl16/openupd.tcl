# What open's update modes do (libc16 0.17; COMPATIBILITY.md, 3.3).
proc contents {name} {set f [open $name r]; set d [read $f]; close $f; return $d}
# w+: write, then read straight back (output is unbuffered), after a seek.
set f [open U.TXT w+]
puts $f one
puts $f two
puts [tell $f]
seek $f 0
puts [gets $f]
# A write after a read needs a seek, as in C.
seek $f [tell $f]
puts $f TWO
close $f
puts [list [contents U.TXT]]
# r+: an existing file, not truncated; overwrite in place.
set f [open U.TXT r+]
seek $f 5
puts -nonewline $f 2
close $f
puts [list [contents U.TXT]]
# a+: reads from anywhere, writes at the end.
set f [open U.TXT a+]
puts [gets $f]
puts $f three
seek $f 0
puts [gets $f]
close $f
puts [list [contents U.TXT]]
# r+ of a file that does not exist is an error.
puts [catch {open NOSUCH.TXT r+}]
file delete U.TXT
