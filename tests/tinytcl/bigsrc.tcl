# Tiny Tcl: source reads a script longer than 32767 bytes command by
# command (Tcl_SmallFootprintEvalFile; patches/0003-int16.patch), so its
# size is limited only by its longest command.
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
