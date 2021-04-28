'''
    getLastBuildNumber will retrieve the last build number of the job specified on the command line.
    
    Parameters:
        -r<URL> : (root URL) the URL of the Jenkins home page that you want to retreive from
        -j<jobname> : (job) the name of the job that you want the information from.
        -[s,f] : if "s" is specified, will return last successful build. If "f" is specified, will return last failed. If omitted will return last build number
        -o<property name> : if specified, it will write to a maven properties file named "number.properties" with the property name specified. 
        
'''

import sys
import os
import urllib
import getopt
from xml.dom import minidom

def main():
    try:
        optList, args = getopt.getopt(sys.argv[1:], 'r:j:s:f:o:')
    except getopt.GetoptError:
        usage()
        sys.exit(1)
    
    
    filename = "lastBuildNum.txt"
    # root Jenkins URL
    jenkinsRoot = "http://ebisu-build01:8080/hudson/"
    # name of the job 
    jobName = ""
    # special options
    option = ""
    #property name
    propName = "upstream.build.num"

    
    for opt, val in optList:
        if opt == "-r":
            jenkinsRoot = val
        elif opt == "-j":
            jobName = val
        elif opt == "-o":
            propName = val
        elif opt == "-s":
            if option != "":
                print "Use EITHER option -s or -f. Not both."
            else:    
                option = val
        elif opt == "-f":
            if option != "":
                print "Use EITHER option -s or -f. Not both."
            else:    
                option = val
        else:
            assert False, "Unhandled Option"
    
    try:
        number = getBuildNumber(constructURL(jenkinsRoot, jobName, option))
    except:
        print "Error retrieving build number from " + constructURL(jenkinsRoot, jobName, option)
        sys.exit(1)
    
    file = open(filename, "w")
    file.write(number)
    file.close()
    
    print number

def constructURL(root, job, option):
    if option == "s":
        type = "lastSuccessfulBuild"
    else:
        type = "lastBuild"
    url = root + "/job/" + job + "/" + type + "/api/xml?tree=number"
    
    return url

def getBuildNumber(url):
    file = urllib.urlopen(url)
    
    if file.code == 404:
        raise Exception
    else:
        dom = minidom.parse(file)
        buildNum = dom.getElementsByTagName('number')[0].firstChild.data
    
    return buildNum
    
def usage():
    print "getLastBuildNumber - will return last build number of a"
    print "spcified Jenkins job. Will return -1 on error."
    print "-r<URL> : (root URL) the URL of the Jenkins home page that you want to retreive from"
    print "-j<jobname> : (job) the name of the job that you want the information from."
    print "-[s,f] : if 's' is specified, will return last successful build. If 'f' is specified, will return last failed."
    


if  __name__ == "__main__":
    main()
    

        