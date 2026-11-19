# SYSINFO.TCL - what TCL.EXE is running on: DOS's version, the kernel,
# memory and drives (the dos command), and a few environment variables.
#
#   TCL SYSINFO.TCL

puts "DOS version:     [dos version]"
if {[dos kernel] == "GR-DOS"} {
    puts "Kernel:          GR-DOS [dos kernelversion]"
} else {
    puts "Kernel:          not GR-DOS"
}
puts "Free memory:     [expr {[dos memory] / 1024}] KB for programs exec runs"
puts "Drives:          [dos drives]"
puts "Directory:       [pwd]"
foreach name {COMSPEC PATH TEMP} {
    if {[info exists env($name)]} {
        puts [format "%-16s %s" $name: $env($name)]
    }
}
puts "Tcl version:     [info tclversion]"
