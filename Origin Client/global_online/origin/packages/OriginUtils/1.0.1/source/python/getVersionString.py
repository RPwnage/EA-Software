'''
Author: D'arcy Gog
Date: 4/07/2011
version: 1.0

Note: requires win32api extension for Python. I used the latest build at the time (build 216) from 
http://sourceforge.net/projects/pywin32/files/pywin32/Build216/
'''


from win32api import GetFileVersionInfo, LOWORD, HIWORD
import getopt
import sys
import os

def main():
    try:
        optList, args = getopt.getopt(sys.argv[1:], 'f:e:o:p:d:')
    except getopt.GetoptError:
        usage()
        sys.exit(1)
    
    # exe filename we want to get the version from
    exeFilename = ""
    # environment variable to write the result to.
    enviroVar = ""
    # output file to write the result to.
    outputFile = ""
    # Maven property to put in the version output file
    propertyName = ""
    # Delimiter
    delimiter = "."

    for opt, val in optList:
        if opt == "-f":
            exeFilename = val
        elif opt == "-e":
            enviroVar = val
        elif opt == "-o":
            outputFile = val
        elif opt == "-p":
            propertyName = (val + "=")
        elif opt == "-d":
            delimiter = val
        else:
            assert False, "Unhandled Option"
    
    try:
        versionString = delimiter.join ([str (i) for i in get_version_number (exeFilename)])
    except:
        print "Error retrieving version from " + exeFilename
        sys.exit(1)
    
    if (outputFile != ""):
        file = open(outputFile, "w")
        file.write(propertyName + versionString)
        file.close()
    
    if (enviroVar != ""):
        os.putenv(enviroVar,versionString)
    
    print versionString

def get_version_number (filename):
    info = GetFileVersionInfo (filename, "\\")
    ms = info['FileVersionMS']
    ls = info['FileVersionLS']
    return HIWORD (ms), LOWORD (ms), HIWORD (ls), LOWORD (ls)

def usage():
    print "getVersionString - will return the version number of the Win32 executable passed in."
    print "-f<filename> : (filename) Relative or absolute path to file you want the version number from"
    print "-e<environment var> : (ENV var) Name of the enviroment var you want to set with the return value."
    print "-o<filename> : (filename) Relative or absolute path to file you want to output the version number to."
    print "-p<property name> : (propertyName) Name for the Maven property you want to assign. Will write out to the output file but"
    print "will prepend <PropertyName>= for using with the Maven property plug-in to import properties directly into pom files."
    print "-d<delimiter> : (delimiter) What you want the delimiter between version numbers. Defaults to \".\""

if __name__ == '__main__':
    main()
