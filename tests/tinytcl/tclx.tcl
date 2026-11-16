# Tiny Tcl only: TclX's loop and readdir (tclXgen.c, readdir.c).
loop j 0 3 {puts "loop $j"}
loop j 10 0 -4 {puts "loop $j"}
puts [expr {[llength [readdir .]] > 3}]
puts [expr {[lsearch [readdir .] TCLX.TCL] >= 0}]
