# read and pwd at the deepest level the stack guard allows (patch 0012):
# their buffers must not run past the guard's margin. Recursion inside
# expr leaves the least stack (about 4.1 KB, measured with m1probe).
set f [open STKREAD.TXT w]
puts -nonewline $f [format %4500s x]
close $f
set before [pwd]
proc deep {n} {
    if {[catch {expr {[deep [expr {$n + 1}]] + 0}}]} {
        set f [open STKREAD.TXT r]
        set d [read $f]
        close $f
        return [list [string length $d] [pwd]]
    }
    return 0
}
set r [deep 0]
puts [lindex $r 0]
puts [expr {[lindex $r 1] == $before}]
file delete STKREAD.TXT
puts done
