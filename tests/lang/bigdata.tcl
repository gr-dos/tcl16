# Larger objects, within the 16-bit limits (docs/M1-REPORT.md, section
# 7.4): a 30000-byte string (and a copy), a 3000-element list, a 1000-element array,
# and sourcing a script longer than 32767 bytes, which Tcl_EvalFile reads
# line by line (patches/0003-int16.patch).
set s 0123456789
while {[string length $s] < 20000} {append s $s}
append s [string range $s 0 9519]
puts [string length $s]
puts [string range $s 29995 29999]
set t $s
# All the words of one command share one parse buffer, so together they
# must fit in 32767 bytes: compare 15000-byte pieces, not the whole.
puts [string compare [string range $s 0 14999] [string range $t 0 14999]]
set l {}
for {set i 0} {$i < 3000} {incr i} {lappend l item$i}
puts [llength $l]
puts [string length $l]
puts [lindex $l 2999]
puts [lrange $l 2998 end]
puts [lreplace [lrange $l 0 3] 2 end]
puts [llength [split $l]]
for {set i 0} {$i < 1000} {incr i} {set arr($i) $i}
puts [array size arr]
unset arr
set f [open BIG.SRC w]
set line "set total \[expr {\$total + 1}\]                                         ;# padding"
puts $f "set total 0"
for {set i 0} {$i < 450} {incr i} {puts $f $line}
puts $f "proc bigproc {} {"
puts $f "    return \"defined after\
 byte 32767\""
puts $f "}"
close $f
puts [expr {[file size BIG.SRC] > 32767}]
source BIG.SRC
puts $total
puts [bigproc]
