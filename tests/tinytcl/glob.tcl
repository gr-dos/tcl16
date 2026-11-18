# glob returns names as DOS stores them (upper case) and, since M5,
# matches patterns ignoring case (src/glob.c).
close [open GLOBCASE.DAT w]
puts [glob GLOBC*.DAT]
puts [glob globc*.dat]
