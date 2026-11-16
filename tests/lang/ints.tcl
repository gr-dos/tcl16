# Integers beyond 16 bits: incr and format work in longs, as expr does,
# so these agree with a 32-bit host (patches/0003-int16.patch).
set x 1
puts [incr x 70000]
puts [incr x 2147413646]
set y 32767
puts [incr y]
puts [incr y -65536]
puts [catch {incr y 99999999999} m]:$m
puts [catch {incr y 1.5} m]:$m
puts [format %d 40000]
puts [format "%s %D %s" abc 5 def]
puts [format %x -1]
puts [format %u -1]
puts [format %X 65535]
puts [format %o 32768]
puts [format %5d|%-6x| 70000 255]
puts [format %hd 40000]
puts [format %c 97]
puts [format "%d%%" 99]
set n 0
for {set i 0} {$i < 50} {incr i} {incr n 1000}
puts $n
