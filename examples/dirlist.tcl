# DIRLIST.TCL - list files with their sizes, and the total, like DIR.
#
#   TCL DIRLIST.TCL [PATTERN...]       (default *.*)
#
# glob matches without regard to case, as DOS does, and returns names in
# upper case. Directories are marked <DIR>.

if {$argc == 0} {set argv {*.*}}
set count 0
set total 0
foreach pattern $argv {
    if {[catch {glob $pattern} names]} {
        puts "$pattern: no match"
        continue
    }
    foreach name [lsort $names] {
        if {[file isdirectory $name]} {
            puts [format "%-12s %10s" [file tail $name] <DIR>]
        } else {
            set size [file size $name]
            puts [format "%-12s %10d" [file tail $name] $size]
            set total [expr {$total + $size}]
        }
        incr count
    }
}
puts [format "%d file(s) %d bytes" $count $total]
