# File I/O and the file command, with DOS paths.
set f [open OUT1.TXT w]
puts $f "first line"
puts -nonewline $f "second"
puts $f " line"
close $f
set f [open OUT1.TXT r]
puts [gets $f]
puts [gets $f line]:$line
puts [gets $f line]:[eof $f]
close $f
puts [file size OUT1.TXT]
set f [open OUT1.TXT a]
puts $f third
close $f
set f [open OUT1.TXT]
set all [read $f]
close $f
puts [llength [split [string trim $all] "\n"]]
set f [open OUT1.TXT]
seek $f 6
puts [tell $f]:[gets $f]
close $f
puts [file exists OUT1.TXT]:[file exists NOPE.TXT]
puts [file isfile OUT1.TXT]:[file isdirectory .]
puts [file extension OUT1.TXT]:[file rootname OUT1.TXT]
puts [file exists {C:\FILES.TCL}]:[file exists C:/FILES.TCL]
puts [lsort [glob *.TXT]]
puts [lsort [glob FIX*.*]]
puts [catch {glob *.NONE} m]:$m
puts [catch {open NODIR\\X.TXT r} m]:[string match {couldn't open "NODIR\\X.TXT": *} $m]
set f [open FIXTURE.DAT]
puts [gets $f]
close $f
puts [expr {[llength [readdir .]] > 3}]
