# A script in DOS text format (CRLF line ends; puts writes "\n" as CRLF):
# a backslash continuation must join the lines, and a string spanning
# lines must not keep the CR. Tiny Tcl's source read the file in binary
# mode, so "\" was followed by CR, not newline, and the next line ran as
# a command of its own; Tcl16's source reads DOS text (src/source.c;
# Tcl 6.7 DOS port: tests/tcl67/crlf.tcl).
set f [open CRLF.SRC w]
puts $f "set x \[list a \\"
puts $f "    b\]"
puts $f "set y \"ab"
puts $f "cd\""
puts $f "set z done"
close $f
puts [file size CRLF.SRC]
puts [catch {source CRLF.SRC} m]:$m
puts $x
puts [string length $y]
