# In a script of more than 32767 bytes, which source reads line by line
# (src/source.c), "return" ends the script, as in a smaller one, and an
# error's line is counted from the start of the file.
set line "set n 0                                                               ;# padding"
set f [open BIGRET.SRC w]
for {set i 0} {$i < 400} {incr i} {puts $f $line}
puts $f "return early"
puts $f "set after 1"
close $f
puts [expr {[file size BIGRET.SRC] > 32767}]
puts [catch {source BIGRET.SRC} m]:$m
puts [info exists after]
set f [open BIGERR.SRC w]
for {set i 0} {$i < 400} {incr i} {puts $f $line}
puts $f "proc p {} {"
puts $f "    error oops"
puts $f "}"
puts $f "p"
close $f
puts [catch {source BIGERR.SRC} m]:$m
puts $errorInfo
