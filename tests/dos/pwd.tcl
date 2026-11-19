# pwd and cd. Whether pwd separates with "\\" or "/" differs by candidate
# (tests/tinytcl/pwdsep.tcl, tests/tcl67/pwdsep.tcl): compare it with "/".
proc slashed {path} {regsub -all {\\} $path / path; return $path}
puts [slashed [pwd]]
# At the root, cd .. is an error on DOS (DOSBox-X allows it): either
# way, the directory is still the root.
catch {cd ..}
puts [slashed [pwd]]
puts [catch {cd NOSUCHDIR} m]:$m
