# Quoting, braces, backslashes and substitution.
set a 5
set b "a is $a"
puts $b
puts {a is $a}
puts "brackets: [expr {$a + 1}]"
puts {brackets: [expr $a + 1]}
puts "nested \"quotes\" and \\backslash"
puts "tab:\tend"
puts "octal \101\102; Tcl 6 has no hex escape: \x43"
puts "dollar \$a and brace \{"
set name a
puts "indirect: [set $name]"
set x(1) one
set i 1
puts "array elem: $x($i)"
puts [list a {b c} "d e" {} \{]
puts "multi\
line"
puts {nested {braces {deep}} ok}
set s {}
append s "x" {y} [list z]
puts $s
puts [string length "\n"]
