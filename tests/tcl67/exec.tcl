# Tcl 6.7 DOS port: exec runs the command line through COMMAND.COM
# (system()), with < > >> and | handled by the shell, and returns what it
# wrote to standard output. A program that cannot be run is not an error:
# the shell's message is the result (README.!!!). open "|..." is refused.
puts [exec echo hello]
puts [catch {exec echo abc > EXEC1.TXT} m]:$m
set f [open EXEC1.TXT]; puts [gets $f]; close $f
puts [catch {exec nosuchprog} m]:[string match *nosuchprog* $m]
puts [catch {open "|echo" r} m]:$m
puts [catch {exec} m]:$m
