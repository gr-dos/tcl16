# glob on DOS (src/glob.c): "\" and "/" separate directories and are
# kept as written, a drive may start the pattern, and matching ignores
# case; found names are as DOS stores them.
file mkdir GD/SUB
foreach f {GD/A.TCL GD/B.TXT GD/SUB/C.TCL} {close [open $f w]}
# A name with a "\" is a list element in braces.
puts [glob "gd\\*.tcl"]
puts [join [lsort [glob gd/*]]]
puts [join [lsort [glob "gd\\*\\*.tcl"]]]
puts [join [lsort [glob "gd\\{a,b}.*"]]]
puts [join [lsort [glob "gd\\\[ab\].t*"]]]
puts [join [lsort [glob "C:gd\\*.txt"]]]
puts [join [lsort [glob "C:\\gd\\*.TXT"]]]
puts [join [lsort [glob "\\gd\\s?b"]]]
puts [glob -nocomplain "gd\\*.xyz"]
puts [catch {glob "gd\\*.xyz"} m]:$m
puts [join [glob "gd\\nosuch"]]
puts [join [lsort [glob gd/sub/*]]]
puts [lsort [readdir gd]]
set p "gd\\\{a"
puts [catch {glob $p} m]:$m
file delete -force GD
