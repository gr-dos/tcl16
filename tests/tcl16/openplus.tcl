# open's update modes, r+, w+ and a+ (libc16 0.17, M7).
close [open P.TXT w]
foreach m {r+ w+ a+} {puts $m:[catch {close [open P.TXT $m]} msg]}
file delete P.TXT
