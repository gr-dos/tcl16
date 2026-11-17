# TCL.EXE's 48 KB stack (docs/DESIGN.md, section 12): 20 levels of
# recursion inside expr and fib 15, which the 32 KB stack before M4 could
# not reach (about 15 levels); deeper recursion is still a Tcl error.
proc count {n} {if {$n == 0} {return 0}; return [expr {1 + [count [expr {$n - 1}]]}]}
puts [count 20]
proc fib {n} {if {$n < 2} {return $n}; return [expr {[fib [expr {$n - 1}]] + [fib [expr {$n - 2}]]}]}
puts [fib 15]
puts [catch {count 40} m]:$m
