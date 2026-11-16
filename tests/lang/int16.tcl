# host: skip
# Where DOS's 16-bit int still shows (patches/0003-int16.patch): the
# integer arguments of commands other than expr, incr and format (list
# and string indices, counts, widths) are C ints, -32768 to 32767, and a
# value outside that is an error, not a silent wrap.
puts [catch {lindex {a b} 65536} m]:$m
puts [catch {string range abc 0 99999} m]:$m
puts [catch {lrange {a b c} 0 40000} m]:$m
puts [lindex {a b c} 32767]:
puts [string range abc 0 32767]
puts [catch {format %40000s x} m]:$m
