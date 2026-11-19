# dos (src/dosinfo.c) on a DOS that is not GR-DOS: DOSBox-X.
# version: n.nn, as DOS reports it.
puts [regexp {^[0-9]+\.[0-9][0-9]$} [dos version]]
# Not GR-DOS: kernel is unknown, kernelversion empty, and neither is an
# error.
puts <[dos kernel]>
puts <[dos kernelversion]>
# memory: bytes, in paragraphs, and what a program exec runs gets, less
# the program's own size (CHILD.EXE's, under 16 KB).
set m [dos memory]
set c [exec CHILD -m]
puts [expr {$m % 16}]
puts [expr {$m > $c && $m - $c < 16384}]
# drives: run-dos mounts C:; Z: is DOSBox-X's own.
puts [dos drives]
puts [llength [dos drives]]
# Errors.
puts [catch {dos} r]:$r
puts [catch {dos nope} r]:$r
puts [catch {dos kern} r]:$r
foreach o {version kernel kernelversion memory drives} {
    puts [catch {dos $o x} r]:$r
}
