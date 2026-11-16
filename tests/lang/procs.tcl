# proc, return, defaults, args, scope, global, upvar, uplevel, info.
proc add {a {b 10}} {return [expr {$a + $b}]}
puts [add 1 2]
puts [add 1]
proc many {first args} {return "$first|[llength $args]|$args"}
puts [many a b c d]
puts [many a]
proc noret {} {set x 42}
puts [noret]
set g 7
proc useglobal {} {global g; return $g}
puts [useglobal]
proc local {} {set g 99; return $g}
puts [local]
puts $g
proc setit {varName val} {upvar $varName v; set v $val}
setit fresh hello
puts $fresh
proc up {} {uplevel {set fromup yes}}
up
puts $fromup
proc depth {} {return [info level]}
puts [depth]
proc fact {n} {if {$n <= 1} {return 1}; return [expr {$n * [fact [expr {$n - 1}]]}]}
puts [fact 12]
puts [info args add]
puts [info body noret]
puts [info default add b dflt]$dflt
puts [lsort [info procs f*]]
rename add plus
puts [plus 2 3]
puts [info procs add]
proc rec {n} {if {$n == 0} {return 0}; return [expr {1 + [rec [expr {$n - 1}]]}]}
puts [rec 10]
