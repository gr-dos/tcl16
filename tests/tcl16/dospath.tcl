# DOS paths in the file command (src/file.c): "/" and "\" both separate
# directories, and a drive "X:" is kept by dirname and is never part of
# a tail, root or extension.
puts [file tail {C:\DIR\NAME.EXT}]
puts [file dirname {C:\DIR\NAME.EXT}]
puts [file tail C:/DIR/NAME.EXT]
puts [file dirname C:/DIR/NAME.EXT]
puts [file extension {C:\DIR.X\NAME}]
puts [file rootname {C:\DIR.X\NAME}]
puts [file dirname {C:\NAME}]
puts [file dirname C:NAME]
puts [file dirname NAME]
puts [file dirname {\NAME}]
puts [file dirname {A\B/C}]
puts [file tail C:NAME.TXT]
puts [file extension C:NAME.TXT]
puts [file rootname {DIR\NAME.TAR.GZ}]
puts [file extension {.\X}]
puts [file tail C:\\]
# Abbreviations: "d" and "m" no longer select dirname and mtime.
puts [file di {A\B}]
puts [catch {file d x} m]:$m
puts [catch {file m x} m]
puts [catch {file dirname} m]:$m
puts [catch {file size} m]:$m
