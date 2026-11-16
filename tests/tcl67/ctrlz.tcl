# Tcl 6.7 DOS port: source reads a script in text mode, which ends at the
# DOS end-of-file byte (^Z) that old DOS editors put at the end of a text
# file (Tiny Tcl: tests/tinytcl/ctrlz.tcl).
set f [open CTRLZ.SRC w]
puts -nonewline $f "set sourced yes\n\032"
close $f
puts [catch {source CTRLZ.SRC} m]:$m
puts $sourced
