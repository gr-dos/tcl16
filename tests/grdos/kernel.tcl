# dos (src/dosinfo.c) on GR-DOS itself (make test-grdos: run-dos -b).
puts [dos kernel]
set v [dos kernelversion]
puts [regexp {^[0-9]+\.[0-9][0-9]$} $v]
# GRVER, on the boot floppy, reports the same release.
puts [expr {[exec A:\\GRVER.EXE] == "GR-DOS $v"}]
# The MS-DOS version GR-DOS reports is MS-DOS 4.00's, whatever its
# release.
puts [dos version]
puts [expr {[dos memory] > 0}]
puts [lindex [dos drives] 0]
