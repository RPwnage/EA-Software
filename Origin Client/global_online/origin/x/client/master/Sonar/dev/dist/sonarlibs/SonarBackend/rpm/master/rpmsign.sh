#!/usr/bin/expect -f
# wrapper to make rpm --sign be non-interactive passwd is 1st arg, file to sign is 2nd
#send_user «$argv0 [lrange $argv 0 2]\n" 
set files [lrange $argv 0 $argc ]
spawn rpm --addsign $files 
expect "Enter pass phrase:"
send -- "\r"
expect eof
