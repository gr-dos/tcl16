# Tcl 6.7 DOS port: source reads a script into one block, and int-sized
# lengths limit that block, so a script of 32767 bytes or more is an
# error (patches/tcl67/0003-int16.patch), not cut short or overrun.
set f [open BIG.SRC w]
set line "set total \[expr {\$total + 1}\]                                         ;# padding"
puts $f "set total 0"
for {set i 0} {$i < 450} {incr i} {puts $f $line}
close $f
puts [expr {[file size BIG.SRC] > 32767}]
puts [catch {source BIG.SRC} m]:$m
puts [info exists total]
