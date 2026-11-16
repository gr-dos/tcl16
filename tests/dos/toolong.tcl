# status: 255
# A string that would pass 32767 bytes stops the interpreter with a
# message on standard error ("tcl16: string or list too long") and the
# status of abort(), 255 with Open Watcom, after keeping what was already
# printed; it does
# not overrun the heap (patches/0005-sizes.patch).
set s 0123456789
while {[string length $s] < 20000} {append s $s}
puts [string length $s]
append s $s
puts "not reached"
