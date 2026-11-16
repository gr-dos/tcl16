# Arrays.
set a(one) 1
set a(two) 2
set a(three) 3
puts [lsort [array names a]]
puts [array size a]
puts [info exists a(two)]
puts [info exists a(four)]
unset a(two)
puts [lsort [array names a]]
set k three
puts $a($k)
incr a(one) 10
puts $a(one)
set id [array startsearch a]
set seen {}
while {[array anymore a $id]} {lappend seen [array nextelement a $id]}
array donesearch a $id
puts [lsort $seen]
for {set i 0} {$i < 200} {incr i} {set big($i) [expr {$i * $i}]}
puts [array size big]
puts $big(199)
proc arr {name} {upvar $name x; return [lsort [array names x]]}
puts [arr a]
puts [catch {set a} msg]$msg
unset a
puts [info exists a]
