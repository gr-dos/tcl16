# scripts/measure: DOS memory left after representative workloads, each
# on top of the ones before (the heap does not give memory back to DOS).
proc mark {what} {puts "$what [m1probe dosfree]"}
mark start
proc fib {n} {if {$n < 2} {return $n}; return [expr {[fib [expr {$n-1}]] + [fib [expr {$n-2}]]}]}
catch {fib 10}
mark procs
set l {}
for {set i 0} {$i < 3000} {incr i} {lappend l item$i}
mark list3000
for {set i 0} {$i < 1000} {incr i} {set a($i) [expr {$i * $i}]}
mark array1000
set s 0123456789
while {[string length $s] < 20000} {append s $s}
append s [string range $s 0 9519]
mark string30000
set t $s
mark copy30000
unset l a s t
mark freed
