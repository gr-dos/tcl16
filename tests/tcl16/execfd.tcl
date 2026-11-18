# exec with few free DOS handles (PR #8 review). CHILD -h K runs TCL.EXE
# with only K of its 20 handles free. With 1 or 2 free, exec cannot make
# its temporary files; with 3 to 5, it cannot save or redirect the
# standard handles, so it reports an error, runs nothing and leaves
# Tcl's own standard output working ("after"). With 7, the child runs.
# (With 6, DOS itself has no handle to open the program file; how it
# reports that depends on the DOS, so it is not tested.)
foreach k {1 3 4 5 7} {
    catch {exec CHILD.EXE -h $k TCL.EXE EXECFD.TIN} m
    puts "$k: [join [split $m \n] { / }]"
}

# A missing program with a name longer than a DOS path: the message has
# the whole name. exec used to cut the argument itself short at 128
# bytes to fit its message buffer, which showed as a truncated message.
set long ""
while {[string length $long] < 200} {append long ABCDEFGHIJ}
catch {exec $long -o x} m
puts [expr {$m == "couldn't find \"$long\" to execute"}]
