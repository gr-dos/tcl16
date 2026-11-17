# TCL.EXE reads its own script as DOS text (src/source.c): this file
# has CR LF line ends, and ends with a ^Z after which nothing is read.
set x [list a \
    b]
puts $x
puts "two
lines"
puts {read past ^Z}
