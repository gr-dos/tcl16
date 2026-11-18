# libc16's stdio has 8 streams, 3 of them the standard ones, so a script
# can have 5 files open at once (docs/COMPATIBILITY.md, section 2).
set n 0
while {$n < 10} {
    if [catch {open F$n.TXT w} f] {puts "$n: $f"; break}
    lappend fs $f
    incr n
}
foreach f $fs {close $f}
foreach f [glob F?.TXT] {file delete $f}
