# string, format, scan, regexp, regsub.
puts [string length "hello world"]
puts [string index hello 1]
puts [string range "hello world" 6 end]
puts [string range hello 1 3]
puts [string first l hello]
puts [string last l hello]
puts [string compare abc abd]
puts [string compare b a]
puts [string match h*o hello]
puts [string match {[a-c]x} bx]
puts [string toupper Hello]
puts [string tolower Hello]
puts "<[string trim "  pad  "]>"
puts "<[string trimleft xxabcxx x]>"
puts "<[string trimright xxabcxx x]>"
puts [format "%d %5d %-5d| %05d" 42 42 42 42]
puts [format "%x %X %o" 255 255 8]
puts [format "%s and %s" one two]
puts [format "%c" 65]
puts [format "%.3f %e %g" 3.14159 31415.9 0.0001]
puts [format "%ld" 100000]
puts [format "%%"]
scan "12 abc 3.5" "%d %s %f" n w f
puts "$n $w $f"
puts [regexp {([0-9]+)-([0-9]+)} "range 10-20 here" all lo hi]
puts "$all $lo $hi"
puts [regexp {^a.*z$} abcz]
puts [regexp {x+} abc]
regsub {o} "foo boo" 0 r1
puts $r1
regsub -all {o} "foo boo" 0 r2
puts $r2
regsub -all {([a-z]+)@([a-z]+)} "joe@host ann@box" {\2:\1} r3
puts $r3
set s ""
for {set i 0} {$i < 100} {incr i} {append s "0123456789"}
puts [string length $s]
