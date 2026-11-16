# Tcl 6.7 DOS port: "time" counts BIOS ticks (55 ms) and reports
# microseconds per iteration.
puts [regexp {^ *[0-9.e+]+ microseconds per iteration$} [time {set x 1} 10]]
puts [catch {time} m]:$m
# history: Tcl 6.7's, which the driver makes at start (src/main.c).
puts [llength [info commands history]]
puts [history nextid]
