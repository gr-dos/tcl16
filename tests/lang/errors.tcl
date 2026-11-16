# catch, error, errorInfo, errorCode, and propagation through procs.
puts [catch {error "plain"} m]:$m
puts [catch {expr 1/0} m]:$m
puts [catch {nosuch arg} m]:$m
puts [catch {set undefined} m]:$m
puts [catch {lindex} m]:$m
puts [catch {expr {1 +}} m]:$m
puts [catch {incr notset} m]:$m
puts [catch {incr x abc} m]:$m
proc inner {} {error "deep" "custom info" {MYCODE 42}}
proc outer {} {inner}
puts [catch outer m]:$m
puts $errorCode
puts [catch {return early} m]:$m
puts [catch {break} m]:$m
puts [catch {continue} m]:$m
puts [catch {set ok 1} m]:$m
proc thrower {} {error inproc}
catch thrower
puts [lindex [split $errorInfo "\n"] 0]
puts [catch {catch} m]:$m
