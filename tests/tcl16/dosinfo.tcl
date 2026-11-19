# dos (src/dosinfo.c), on any kernel: DOSBox-X's, or one booted
# (run-dos -b).
# version: n.nn, as DOS reports it.
puts [regexp {^[0-9]+\.[0-9][0-9]$} [dos version]]
# kernel and kernelversion agree: GR-DOS and n.nn, or unknown and empty
# (tests/grdos and tests/notgrdos check which).
set k [dos kernel]
set kv [dos kernelversion]
puts [expr {($k == "GR-DOS" && [regexp {^[0-9]+\.[0-9][0-9]$} $kv])
    || ($k == "unknown" && $kv == "")}]
# memory: bytes, in paragraphs, and what a program exec runs gets, less
# the program's own size (CHILD.EXE's, under 16 KB).
set m [dos memory]
set c [exec CHILD -m]
puts [expr {$m % 16}]
puts [expr {$m > $c && $m - $c < 16384}]
# drives: letters with colons, in order, C: among them (run-dos's C: is
# the work directory). The others depend on the machine: DOSBox-X has
# its Z:, a booted kernel A: and B:.
set d [dos drives]
puts [regexp {^([A-Z]: )*[A-Z]:$} $d]
puts [expr {[lsearch $d C:] >= 0}]
puts [expr {[lsort $d] == $d}]
# Errors.
puts [catch {dos} r]:$r
puts [catch {dos nope} r]:$r
puts [catch {dos kern} r]:$r
foreach o {version kernel kernelversion memory drives} {
    puts [catch {dos $o x} r]:$r
}
