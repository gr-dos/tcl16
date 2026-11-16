# scripts/measure: BIOS ticks (18.2 a second) for small benchmarks, under
# DOSBox-X at a fixed 315 cycles per millisecond (about a 4.77 MHz 8088).
# A benchmark that fails (fib15 runs out of stack with less than 32 KB)
# prints "error" and the others still run.
proc t {name script} {
    set t0 [m1probe ticks]
    if {[catch {uplevel $script}]} {
        puts "$name error"
    } else {
        puts "$name [expr {[m1probe ticks] - $t0}]"
    }
}
t loop1000 {for {set i 0} {$i < 1000} {incr i} {}}
t expr1000 {for {set i 0} {$i < 1000} {incr i} {set x [expr {$i * 3 + 7}]}}
t fexpr1000 {for {set i 0} {$i < 1000} {incr i} {set x [expr {$i * 1.5 + 0.25}]}}
proc nop {} {}
t call500 {for {set i 0} {$i < 500} {incr i} {nop}}
proc fib {n} {if {$n < 2} {return $n}; return [expr {[fib [expr {$n-1}]] + [fib [expr {$n-2}]]}]}
t fib15 {fib 15}
t lappend2000 {set l {}; for {set i 0} {$i < 2000} {incr i} {lappend l $i}}
t lsort500 {set l {}; for {set i 500} {$i > 0} {incr i -1} {lappend l x$i}; lsort $l}
t string500 {set s {}; for {set i 0} {$i < 500} {incr i} {append s [string toupper abc]}}
t regexp200 {for {set i 0} {$i < 200} {incr i} {regexp {([a-z]+)([0-9]+)} abc123 all a b}}
t format500 {for {set i 0} {$i < 500} {incr i} {format "%5d %s %x" $i abc $i}}
