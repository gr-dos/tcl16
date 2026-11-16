# A script in DOS text format (CRLF line ends; puts writes "\n" as CRLF):
# a backslash continuation must join the lines, and a string spanning
# lines must not keep the CR. The port reads scripts in text mode, which
# turns CRLF into LF (Tiny Tcl: tests/tinytcl/crlf.tcl).
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
