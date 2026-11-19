# dos kernel and kernelversion (src/dosinfo.c) with FAKEID's imitations
# of GR-DOS's identification call (tests/tools/fakeid.asm). Only a valid
# answer, the same to both nonces, is GR-DOS; anything else is unknown,
# never an error. dos version is DOS's, whatever the kernel says.
set v [dos version]
set script {puts "[dos kernel] <[dos kernelversion]> [dos version]"}
foreach mode {U D S C M R G} {
    set r [exec FAKEID $mode C:\\TCL.EXE -e $script]
    puts "$mode: [lrange $r 0 1] [expr {[lindex $r 2] == $v}]"
}
# GR-DOS, in a script: what a script does to tell.
set script {if {[dos kernel] == "GR-DOS"} {exit 0} else {exit 1}}
puts [catch {exec FAKEID G C:\\TCL.EXE -e $script}]
puts [catch {exec FAKEID U C:\\TCL.EXE -e $script}]:$errorCode
