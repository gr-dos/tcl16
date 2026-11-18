# file mkdir, rename and delete (src/file.c): Tcl16 extensions, with
# Tcl 7.6's syntax.
file mkdir {NEWDIR\SUB\DEEP}
puts [file isdirectory {NEWDIR\SUB\DEEP}]
file mkdir NEWDIR
close [open {NEWDIR\SUB\A.TXT} w]
puts [catch {file mkdir {NEWDIR\SUB\A.TXT}} m]:$m
puts [catch {file mkdir {NEWDIR\SUB\A.TXT\X}} m]:$m
file rename {NEWDIR\SUB\A.TXT} {NEWDIR\B.TXT}
puts [file exists {NEWDIR\SUB\A.TXT}][file exists {NEWDIR\B.TXT}]
close [open C.TXT w]
puts [catch {file rename C.TXT {NEWDIR\B.TXT}} m]:$m
file rename -force C.TXT {NEWDIR\B.TXT}
puts [file exists C.TXT]
file rename {NEWDIR\B.TXT} {NEWDIR\SUB}
puts [file exists {NEWDIR\SUB\B.TXT}]
puts [catch {file rename NOSUCH X} m]:$m
puts [catch {file rename A} m]:$m
puts [catch {file delete NEWDIR} m]:$m
file delete NOSUCH.TXT
puts [catch {file delete -force NEWDIR} m]:$m
puts [file exists NEWDIR]
file mkdir E
file delete -- E
puts [file exists E]
puts [file executable CHILD.EXE][file executable T.BAT][file executable .]
puts [file executable FILEOPS.TCL][file executable NOSUCH.EXE]
puts [catch {file mkdir} m]:$m
puts [catch {file delete} m]:$m
