'''
Author: D'arcy Gog
Date: Jan 28. 2012
Python: 2.6
'''

import getopt
import sys
import os
import string

def main():
    try:
        optList, args = getopt.getopt(sys.argv[1:],"i:o:v:u:")
    except:
        usage();
        sys.exit(1)
    
    inputFile = ""
    outputFile = ""
    version = ""
    url = ""    
        
    for opt, val in optList:
        if opt == "-i":
            inputFile = val
        elif opt == "-o":
            outputFile = val
        elif opt == "-v":
            version = val
        elif opt == "-u":
            url = val
        else:
            print "Unhandled argument"
            usage()
            sys.exit(1)
        
    updateTemplate(inputFile, outputFile, version, url)

def updateTemplate(inputFile, outputFile, version, url):
    f = open(inputFile, "r")
    data = f.read()
    f.close()
    data = string.replace(data, r"<version>", version)
    data = string.replace(data, r"<some url>", url)
    f = open(outputFile, "w")
    f.write(data)
    f.close()

def usage():
    print "updateAutoUpdateJSON"
    print "================\n"
    print "usage: updateAutoUpdateJSON.py -i<inputFile> -o<outputFile> -v<version> -u<url>"
    print "-i<inputFile> = path to the template file we are editing."
    print "-o<outputFile> = path to the output file we should save to."
    print "-v<version> = New Origin version in the format nn.nn.nn.nnnn"
    print "-u<url> = new url where we put the OriginUpdate file"

if __name__ == "__main__":
    main()
    
