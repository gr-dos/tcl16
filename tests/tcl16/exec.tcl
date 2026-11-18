# exec (src/exec.c), with CHILD.EXE (tests/tools/child.c).
puts [exec CHILD -o hello]
puts [exec child -a a {b c} {"q"} {} x\\y]
puts [exec CHILD.EXE -o ext]
puts [exec C:CHILD -o drive]
puts [exec .\\CHILD -o dot]
# Exit status and standard error.
puts [catch {exec CHILD -x 3} m]:$m:$errorCode
puts [catch {exec CHILD -o out -x 3} m]:$m
puts [catch {exec CHILD -e oops} m]:$m
puts [catch {exec CHILD -o a -e b} m]:$m
puts [catch {exec NOSUCH} m]:$m:$errorCode
puts [catch {exec NOSUCH | CHILD -o a} m]:$m
# Redirection and pipelines.
puts [exec CHILD -c << "line 1\nline 2"]
puts [exec CHILD -o piped | CHILD -c | CHILD -c]
puts [exec CHILD -o x -e y |& CHILD -c]
exec CHILD -o first > F.TXT
exec CHILD -o second >> F.TXT
puts [exec CHILD -c < F.TXT]
puts [exec CHILD -c <F.TXT]
exec CHILD -o a -e b >& F.TXT
puts [exec CHILD -c < F.TXT]
puts [exec < F.TXT CHILD -c | CHILD -c]
puts <[exec CHILD -c]>
puts [exec CHILD -o direct >@ stdout]
set f [open F.TXT w]
exec CHILD -o tofile >@ $f
close $f
puts [exec CHILD -c < F.TXT]
puts [exec CHILD -a a &b]
# The env array is the child's environment.
set env(TCL16X) fromtcl
puts [exec CHILD -g TCL16X]
unset env(TCL16X)
puts [exec CHILD -g TCL16X]
# COMMAND.COM's commands and batch files.
puts [exec echo hi there]
puts [exec T.BAT one]
puts [exec t two]
# Errors.
puts [catch {exec} m]:$m
puts [catch {exec | CHILD} m]:$m
puts [catch {exec CHILD |} m]:$m
puts [catch {exec CHILD | | CHILD} m]:$m
puts [catch {exec CHILD <} m]:$m
puts [catch {exec CHILD >>} m]:$m
puts [catch {exec CHILD < NOSUCH.TXT} m]:$m:$errorCode
puts [catch {exec CHILD > NODIR\\X.TXT} m]:$m
puts [catch {exec CHILD &} m]:$m
set long x
while {[string length $long] < 130} {append long x}
puts [catch {exec CHILD $long} m]:$m
# Memory left for a child (docs/COMPATIBILITY.md).
puts [expr {[exec CHILD -m] > 300000}]
file delete F.TXT
