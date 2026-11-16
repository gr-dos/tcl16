# todo: source reads DOS (CRLF) scripts in binary mode
# A script in DOS text format (CRLF line ends; puts writes "\n" as CRLF):
# a backslash continuation must join the lines, and a string spanning
# lines must not keep the CR. Tiny Tcl's source reads the file in binary
# mode, so "\" is followed by CR, not newline, and the next line runs as
# a command of its own (Tcl 6.7 DOS port: tests/tcl67/crlf.tcl).
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
