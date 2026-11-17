# A DOS text script of more than 32767 bytes, which source reads line by
# line (src/source.c): CR LF line ends, a backslash continuation, a value
# spanning lines, and a ^Z after which nothing is read.
set f [open BIGCRLF.SRC w]
puts $f "set total 0"
puts $f "set joined \[list a \\"
puts $f "    b\]"
puts $f "set multi \"ab"
puts $f "cd\""
set line "incr total                                                            ;# padding"
for {set i 0} {$i < 400} {incr i} {puts $f $line}
puts $f "set last \[string length \$multi\]"
puts -nonewline $f "\032error {read past ^Z}\n"
close $f
puts [expr {[file size BIGCRLF.SRC] > 32767}]
puts [catch {source BIGCRLF.SRC} m]:$m
puts $total
puts $joined
puts $last
