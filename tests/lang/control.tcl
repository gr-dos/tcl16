# if, while, for, foreach, break, continue, case.
set x 3
if {$x == 1} {puts one} elseif {$x == 3} {puts three} else {puts other}
if {$x > 5} then {puts big} else {puts small}
set i 0
while {$i < 5} {incr i; if {$i == 2} continue; if {$i == 4} break; puts "while $i"}
for {set i 0} {$i < 3} {incr i} {puts "for $i"}
set sum 0
foreach v {1 2 3 4 5} {set sum [expr {$sum + $v}]}
puts "sum $sum"
foreach v {a b c} {if {$v == "b"} break; puts "foreach $v"}
case abc in {a*} {puts "case a*"} default {puts "case default"}
case xyz in {a* b*} {puts ab} default {puts "case default"}
set n 0
for {set i 0} {$i < 1000} {incr i} {incr n 2}
puts "n $n"
puts [incr n -2000]
