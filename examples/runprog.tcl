# RUNPROG.TCL - run a program with exec, and report its output and exit
# status the way a batch file cannot.
#
#   TCL RUNPROG.TCL PROGRAM [ARG...]
#
# exec returns the program's output; a status other than 0 is an error,
# with errorCode "CHILDSTATUS 0 STATUS". The program reads NUL unless
# its input is redirected, and COMMAND.COM's own commands (DIR, TYPE...)
# run through COMSPEC.

if {$argc == 0} {
    puts "usage: TCL RUNPROG.TCL PROGRAM [ARG...]"
    exit 2
}
if {[catch {eval exec $argv} output]} {
    if {[lindex $errorCode 0] == "CHILDSTATUS"} {
        set status [lindex $errorCode 2]
    } else {
        puts "cannot run [lindex $argv 0]: $output"
        exit 1
    }
} else {
    set status 0
}
set lines [split $output \n]
puts "[lindex $argv 0]: status $status, [llength $lines] line(s) of output"
foreach line $lines {puts "  | $line"}
