# todo: file dirname and file tail split only at "/"
# DOS paths with backslashes and drive letters in the file command.
puts [file tail {C:\DIR\NAME.EXT}]
puts [file dirname {C:\DIR\NAME.EXT}]
puts [file tail C:/DIR/NAME.EXT]
puts [file dirname C:/DIR/NAME.EXT]
puts [file extension {C:\DIR.X\NAME}]
puts [file rootname {C:\DIR.X\NAME}]
