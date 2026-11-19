# examples/sysinfo.tcl, run with exec: its lines vary with the machine,
# so check their form.
set out [exec C:\\TCL.EXE SYSINFO.TCL]
foreach line [split $out \n] {
    # The environment variables it shows depend on the machine.
    if {[regexp {^([A-Za-z ]+):} $line all label]
            && [lsearch {COMSPEC PATH TEMP} $label] < 0} {
        puts $label
    }
}
puts [regexp {DOS version: +[0-9]+\.[0-9][0-9]} $out]
puts [regexp {Tcl version: +6\.7} $out]
