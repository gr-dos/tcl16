# dos (src/dosinfo.c) on a kernel that is not GR-DOS: DOSBox-X's (make
# test) and MS-DOS 4.00 (make test-msdos). kernel is unknown,
# kernelversion empty, and neither is an error.
puts <[dos kernel]>
puts <[dos kernelversion]>
puts [regexp {^[0-9]+\.[0-9][0-9]$} [dos version]]
