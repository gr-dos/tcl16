# scripts/measure: the deepest recursion before Tcl_Eval's stack guard
# stops it, for a plain recursive proc and for one that recurses inside
# expr, and the stack used per level.
proc down {n} {global deepest; set deepest $n; down [expr {$n + 1}]}
set deepest 0
catch {down 1} m
puts "plain $deepest"
puts "plainmsg $m"
proc count {n} {global deepest; set deepest $n; return [expr {1 + [count [expr {$n + 1}]]}]}
set deepest 0
catch {count 1} m
puts "inexpr $deepest"
proc stk {n} {if {$n == 0} {return [m1probe stack]}; return [stk [expr {$n - 1}]]}
set s0 [stk 0]
set s5 [stk 5]
puts "perlevel [expr {($s0 - $s5) / 5}]"
