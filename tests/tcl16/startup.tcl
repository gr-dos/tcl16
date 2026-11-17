# Startup (docs/COMPATIBILITY.md, section 4.3): no banner, no startup
# script and no tcl_library; info library reports TCL_LIBRARY, which is
# not set here; the language is Tcl 6.7; m1probe is only in the
# measurement builds.
puts [info exists tcl_library]
puts [catch {info library} m]:$m
puts [info tclversion]
puts [llength [info commands m1probe]]
puts [lsort [info procs]]
