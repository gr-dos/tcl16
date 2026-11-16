# eval, nested evaluation, source, info, trace.
set cmd {puts "from eval"}
eval $cmd
eval puts {"two words"}
eval [list set built [list a b]]
puts $built
puts [eval {expr {2 * [expr {3 + 4}]}}]
source INC.SRC
puts [incproc 5]
puts $incvar
puts [info exists incvar]
puts [lsort [info commands l*]]
puts [info tclversion]
set log {}
proc tr {n1 n2 op} {global log; lappend log "$n1 $op"}
trace variable watched w tr
set watched 1
set watched 2
puts $log
puts [llength [info globals]]>0
