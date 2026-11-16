# Tcl 6.7 DOS port: glob returns names in lower case, and patterns match
# case-sensitively, so a pattern must be lower case (the port's
# README.!!!: "For all glob operations you HAVE to use lowercase").
close [open GLOBCASE.DAT w]
puts [glob globc*.dat]
puts [catch {glob GLOBC*.DAT} m]:$m
