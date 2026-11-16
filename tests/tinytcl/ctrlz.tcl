# Tiny Tcl: source reads a script in binary mode, so the DOS end-of-file
# byte (^Z) that old DOS editors put at the end of a text file is read as
# a command, and the script fails (Tcl 6.7 DOS port: tests/tcl67/ctrlz.tcl).
set f [open CTRLZ.SRC w]
puts -nonewline $f "set sourced yes\n\032"
close $f
puts [catch {source CTRLZ.SRC} m]
puts $sourced
