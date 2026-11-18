# todo: libc16's fopen has no update modes (due M7; DESIGN.md, 10.1)
# open's update modes, r+, w+ and a+.
close [open P.TXT w]
foreach m {r+ w+ a+} {puts $m:[catch {close [open P.TXT $m]} msg]}
file delete P.TXT
