# FINDTEXT.TCL - print the lines of files that match a regular expression,
# with their file names and line numbers.
#
#   TCL FINDTEXT.TCL [-i] REGEXP PATTERN...
#
# -i ignores case. PATTERNs are glob patterns (*.TXT); exit status 0 if
# any line matched, 1 if none did.

set nocase {}
if {[lindex $argv 0] == "-i"} {
    set nocase -nocase
    set argv [lrange $argv 1 end]
}
if {[llength $argv] < 2} {
    puts "usage: TCL FINDTEXT.TCL [-i] REGEXP PATTERN..."
    exit 2
}
set re [lindex $argv 0]
set found 0
foreach pattern [lrange $argv 1 end] {
    if {[catch {glob $pattern} names]} continue
    foreach name [lsort $names] {
        if {[file isdirectory $name]} continue
        set f [open $name r]
        set n 0
        while {[gets $f line] >= 0} {
            incr n
            if {[eval regexp $nocase {$re} {$line}]} {
                puts "$name:$n: $line"
                set found 1
            }
        }
        close $f
    }
}
exit [expr {!$found}]
