# pwd and cd.
puts [pwd]
cd ..
puts [pwd]
puts [catch {cd NOSUCHDIR} m]:$m
