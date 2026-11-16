# Tiny Tcl: glob returns names as DOS stores them (upper case) and
# matches patterns case-sensitively, so a pattern must be upper case.
close [open GLOBCASE.DAT w]
puts [glob GLOBC*.DAT]
puts [catch {glob globc*.dat} m]:$m
