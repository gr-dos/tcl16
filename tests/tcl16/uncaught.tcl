# status: 1
# stderr: merged
# An uncaught error in a script: what the script printed, then "Error: "
# and errorInfo, which says where, on standard error; exit status 1.
puts before
proc p {} {error "deep error"}
p
puts after
