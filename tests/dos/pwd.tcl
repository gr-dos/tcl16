# pwd and cd. Whether pwd separates with "\\" or "/" differs by candidate
# (tests/tinytcl/pwdsep.tcl, tests/tcl67/pwdsep.tcl): compare it with "/".
proc slashed {path} {regsub -all {\\} $path / path; return $path}
puts [slashed [pwd]]
cd ..
puts [slashed [pwd]]
puts [catch {cd NOSUCHDIR} m]:$m
