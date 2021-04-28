'''
killProcess.py - This script will kill all running EADM and EACoreServer instances
running on the local PC by querying the tasklist and forcing them to shutdown
see usage function at bottom for usage.
'''

import sys
import os
import getopt
import string
OSX_NAME = "darwin"
def main():
    try:
        optList, args = getopt.getopt(sys.argv[1:], 'p:')
    except getopt.GetoptError:
        usage()
        sys.exit(1)

    for opt, val in optList:
        if opt == "-p":
            processes = val.split(",")
        else:
            assert False, "Unhandled Option"
    
    for process in processes:
        killAllProcess(process)

def killAllProcess(processName):
    if sys.platform ==OSX_NAME:
        print ("Killing process: " + processName)
        try:
            os.system("killall "+processName)
        except Exception as e:
            pass
    else:
        while getTaskIsRunning(processName):
            print ("Killing process: " + processName)
            os.system("taskkill /F /IM " + processName)
        
def getTaskIsRunning(processName):
    result = os.popen("tasklist /FI \"IMAGENAME eq " + processName + "\"").readlines()
    for line in result:
        if line.find(processName) != -1:
            return 1
    return 0

def usage():
    print ("Invalid Option")
    print ("Usage: python killProcess.py -p[process1[process2,...]]")
    print ("Note: Process names are CASE SENSITIVE")
    print ("Example: python killProcess.py -pEADMUI.exe,EACoreServer.exe")
    
if __name__ == '__main__':
    main()