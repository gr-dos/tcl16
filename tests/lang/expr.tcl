# expr: integers are C longs (32 bits even on DOS), doubles print with %g.
# In Tcl 6, expr takes exactly one argument (Tcl 7 concatenates several),
# has no math functions (Tcl 7.0) and truncates integer division towards
# zero, as C does (Tcl 7 floors: -7/2 is -4).
puts [catch {expr 1 + 2} m]:$m
puts [expr 1+2*3]
puts [expr (1+2)*3]
puts [expr 7/2]
puts [expr -7/2]
puts [expr 7%3]
puts [expr 2147483647]
puts [expr 2147483647+1]
puts [expr 100000*3]
puts [expr 65536*65536]
puts [expr 1<<16]
puts [expr 1<<31]
puts [expr 0x7fff+1]
puts [expr 010]
puts [expr ~0]
puts [expr 6&3]
puts [expr 6|3]
puts [expr 6^3]
puts [expr {3>2 && 2>1}]
puts [expr {3<2 || 0}]
puts [expr !5]
puts [expr {1 ? 10 : 20}]
puts [expr 1==1.0]
puts [expr 7/2.0]
puts [expr 1.0/3]
puts [expr 1e10]
puts [expr 2.5*4]
puts [expr 3.0-1]
puts [catch {expr int(3.9)} m]:$m
puts [expr {"abc" < "abd"}]
puts [expr {"x" == "x"}]
set n 40000
puts [expr $n*2]
puts [expr {$n + 1}]
