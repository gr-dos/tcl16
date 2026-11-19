# WC.TCL - count the lines, words and bytes of text files.
#
#   TCL WC.TCL FILE...
#
# Files are read as DOS text: CR LF is one line end, and a ^Z ends the
# text, so the byte count is of the text Tcl sees.

proc count {name} {
    set f [open $name r]
    set lines 0
    set words 0
    set bytes 0
    while {[gets $f line] >= 0} {
        incr lines
        # Words: the line's blanks as single spaces. (Tcl 6.7's regsub
        # leaves the variable alone when nothing matches.)
        set text [string trim $line]
        regsub -all "\[ \t\]+" $text " " text
        if {$text != ""} {incr words [llength [split $text " "]]}
        set bytes [expr {$bytes + [string length $line] + 1}]
    }
    close $f
    return [list $lines $words $bytes]
}

if {$argc == 0} {
    puts "usage: TCL WC.TCL FILE..."
    exit 2
}
set status 0
foreach name $argv {
    if {[catch {count $name} n]} {
        puts "$name: $n"
        set status 1
        continue
    }
    puts [format "%7d %7d %7d %s" [lindex $n 0] [lindex $n 1] [lindex $n 2] $name]
}
exit $status
