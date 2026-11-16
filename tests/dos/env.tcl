# The environment: env reads DOS's, and sets are seen by env and getenv.
puts [info exists env(COMSPEC)]
puts [info exists env(PATH)]
puts [string match *COMMAND.COM $env(COMSPEC)]
set env(TCL16TEST) hello
puts $env(TCL16TEST)
unset env(TCL16TEST)
puts [info exists env(TCL16TEST)]
puts [catch {set env(NOSUCHVAR)} m]:$m
