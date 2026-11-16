# Tcl 6.7 DOS port: pwd turns DOS's "\" into "/" (_BACKSLASH_PORT_), and
# file names given to Tcl may use either.
puts [pwd]
puts [file dirname C:/DIR/NAME.EXT]:[file tail C:/DIR/NAME.EXT]
puts [file exists C:/PWDSEP.TCL]:[file exists {C:\PWDSEP.TCL}]
