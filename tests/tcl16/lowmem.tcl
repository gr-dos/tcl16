# TCL.EXE with little conventional memory: CHILD -l K leaves K KB free
# and runs it (tests/tools/child.c). Measured (M7): it needs 170 KB free
# to load and about 175 KB to run a script; below, it stops cleanly.
# The sizes here leave room for TCL.EXE to grow.
proc tcl {k args} {
    global errorCode
    set r [catch {eval exec CHILD -l $k C:\\TCL.EXE $args} m]
    regsub {\(.*\)} $m {(...)} m
    if {$r} {return "$m / [lindex $errorCode 2]"}
    return $m
}
# Too little to load: DOS refuses it.
puts [tcl 140 -e {puts hi}]
# Enough for a small script...
puts [tcl 200 -e {puts hi; puts [expr {[dos memory] < 40000}]}]
# ...but not for a 30,000-byte string (LOWMEM.TIN): "out of memory",
# status 255.
puts [tcl 200 LOWMEM.TIN]
# With room, the same script runs.
puts [tcl 300 LOWMEM.TIN]
