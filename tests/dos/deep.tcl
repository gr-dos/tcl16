# Deep recursion ends in a Tcl error, not a crash: Tcl_Eval stops when the
# stack is nearly gone (patches/0004-stack-guard.patch), long before Tiny
# Tcl's MAX_NESTING_DEPTH of 100 levels on an 8086 with a 32 KB stack.
proc down {n} {return [down [expr {$n + 1}]]}
puts [catch {down 0} m]:$m
proc count {n} {if {$n == 0} {return 0}; return [expr {1 + [count [expr {$n - 1}]]}]}
puts [count 10]
puts [catch {count 40} m]:$m
puts "still running"
