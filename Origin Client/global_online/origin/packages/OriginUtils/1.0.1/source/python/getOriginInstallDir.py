import os
from _winreg import *

win32_install_key_path = r'SOFTWARE\Origin'
install_key = "ClientPath"

try:
    key = OpenKey(HKEY_LOCAL_MACHINE, win32_install_key_path, 0, KEY_QUERY_VALUE)
    install_exe, type = QueryValueEx(key, install_key)
    CloseKey(key)

    # Separate out the install path from executable
    install_path, separator, executable = install_exe.rpartition('\\')
    if install_path != separator:
        print install_path
    else:
        print "Error finding install path"
        
except OSError, e:
    print "OSError Exception"    
    print e
except WindowsError, e:
    print "WindowsError Exception"    
    print e
except:
    print "Unexpected Exception Error"
    
    

