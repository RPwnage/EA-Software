Installing nant on unix/Cygwin .

Framework folder has a script file called nant.  This script file launches nant executable using Mono:

       #!/bin/sh
       exec mono ~/packages/Framework/bin/NAnt.exe "$@"

Script assumes that Framework package located in the user home directory in a folder named "packages".
If Framework package location is different edit path to nant.exe in the script.

Place file 'nant' in a suitable location in your file system (eg. /usr/local/bin).

Ensure that nant has permission to execute:

    chmod a+x /usr/local/bin/nant

Also, make sure that when you sync this script file from perforce, your perforce setup has
"Line ending characters for text files" uses "Local: defaults to the current operating system".  
Otherwise you will get errors if your script files contains Windows line ending \r\n characters.