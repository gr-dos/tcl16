# scan, through Tcl16's own scanner (src/scan.c): libc16 has no sscanf.
puts [scan "12 34" "%d %d" a b]:$a:$b
puts [scan "-12 0x1f 777" "%d %x %o" a b c]:$a:$b:$c
puts [scan "123456" "%2d%3d" a b]:$a:$b
puts [scan "1;2" "%d,%d" a b]:$a
puts [scan "" "%d" a]
puts [scan "abc" "%d" a]
puts [scan "hello world" "%3s%s" a b]:$a:$b
puts [scan "abc123" {%[a-c]%d} a b]:$a:$b
puts [scan "x=y" {%[^=]=%s} a b]:$a:$b
puts [scan "A" "%c" a]:$a
puts [scan "12 abc" "%*d %s" a]:$a
# Tcl_ScanCmd itself rejects "%%", as Tcl 6.7 does.
puts [catch {scan "50%" "%d%%" a} m]:$m
puts [scan "3.5 -2e3" "%f %e" a b]:$a:$b
puts [scan "0.1 1e300" "%F %E" a b]:$a:$b
