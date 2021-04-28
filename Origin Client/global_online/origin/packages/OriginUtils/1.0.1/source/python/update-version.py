'''
Author: D'arcy Gog
Date: 4/07/2011
version: 1.4


This is a new version of the update-version file that uses the Perforce counters.
It will grab the p4 counter from the P4 and create a version.h file out of that locally.
I have added support to check in the version.h file if the version number has changed (ie. increment_version != 0 )

If you don't want to pass in the p4client variable, you must have "p4 set P4CLIENT=<clientname>" to set the default P4 client
or the script will fail.

Usage: update_version -o<version_folder_path> -v<version_counter_name> -i<increment> -c<changelist> -s<p4port> -w<p4client> -u<p4username> -k<p4password>]

-o<version_folder_path> - path to version folder that holds the version.h.in and version.h files
-v<version_counter_name> - name of the p4 counter that holds the version number for this operation (ie. 8.0.0.0)
-i<increment> - 0=do not incremement patch #(optional p4 args are not required as it does not P4 access
                    - +1=increment patch# by n, 
                    - nnnnn=change patch to specd number.
-c<changelist> - new parameter to hold the current changelist number.

-s<p4port>: P4 server you are using."
-u<p4user>: P4 user you are using."
-k<p4password>: P4 password for the user."
-w<p4client>: P4 client you are using."

'''

import time;
import sys;
import os;
import stat;
import getopt;
from P4 import P4, P4Exception

VERSION_H_FILENAME = "version.h"
# VERSION_NUMBER_FILE = "version.txt"
VERSION_H_TEMPLATE = "version.h.in"

DATE_TIME_FORMAT = "%m/%d/%y %H:%M:%S"
MAJOR_VERSION_VARIABLE = "$(major)"
MINOR_VERSION_VARIABLE = "$(minor)"
PATCH_VERSION_VARIABLE = "$(patch)"
REVISION_VARIABLE = "$(revision)"
TIME_VARIABLE = "$(time)"
CL_VARIABLE = "$(changelist)"

'''
Clean this script up later
But for now, it does what it's supposed to do
 - Reads/Increments the current version
 - Builds new version.h file using template
 - Submits changes to Perforce
'''

def main():
    global os_version_folder_path
    global versionCounterName
    global cl_num

    try:
        optList, args = getopt.getopt(sys.argv[1:], 'o:v:i:c:s:u:k:w:')
    except getopt.GetoptError:
        usage()
        sys.exit(1)
    
    osdirectorypath = ""
    versionCounterName = ""
    version_inc = ""
    cl_num = ""
    port = ""
    user = ""
    password = ""
    clientspec=""
    
    for opt, val in optList:
        if opt == "-o":
            osdirectorypath = val
        elif opt == "-v":
            versionCounterName = val
        elif opt == "-i":
            version_inc = val
        elif opt == "-c":
            cl_num = val
        elif opt == "-s":
            port = val
        elif opt == "-u":
            user = val
        elif opt == "-k":
            password = val
        elif opt == "-w":
            clientspec = val
        else:
            assert False, "Unhandled Option"

    os_version_folder_path = "/".join(osdirectorypath.split("/")[:-1])
#    os_version_file_path = versionCounterName

    p4_init(port, clientspec, user, password)

    print "Old version number: " + get_whole_version_number()

# do not do any Perforce operations if we are not changing the version number 
    if version_inc != "0":
        inc_revision_number(version_inc)
        p4_edit_version_files()
        
    make_new_version_file()

# do not do any Perforce operations if we are not changing the version number 
    if version_inc != "0":
        p4_submit_version_files()

    p4.disconnect()

def get_version_num(version_part):
#    current_version_file_r = open(os_version_file_path + "/" + VERSION_NUMBER_FILE, 'r')
#    old_version_number = current_version_file_r.readlines()[0]
#    current_version_file_r.close()

    old_version_number=p4.run("counter",versionCounterName)[0]["value"]
	
    split_old_version = old_version_number.split(".")
    try:
        version_number = split_old_version[version_part]
    except:
        print "Improper version format. Use nn.nn.nn.nn"
        print "Counter name:" + versionCounterName + " value:" + old_version_number
	
    return version_number

def get_major_version_num():
    return get_version_num(0)

def get_minor_version_num():
    return get_version_num(1)

def get_patch_version_num():
    return get_version_num(2)

def get_revision_num():
    return get_version_num(3)

def get_changelist_num():
    return cl_num #os.getenv("P4_CORE_CL","00000")

def inc_revision_number(version_num):
#    current_version_file_r = open(os_version_file_path + "/" + VERSION_NUMBER_FILE, 'r')
#    old_version_number = current_version_file_r.readlines()[0]
#    current_version_file_r.close()
	
    old_version_number=p4.run("counter",versionCounterName)[0]["value"]
    
    split_old_version = old_version_number.split(".")
    revision = split_old_version[3]
    if version_num.startswith("+"):
        version_num=version_num.strip("+")
        revision = int(revision) + int(version_num)
    else:
        revision = int(version_num)
    revision = str(revision)

#    current_version_file_w = open(os_version_file_path + "/" + VERSION_NUMBER_FILE, 'w')
#    current_version_file_w.write(split_old_version[0] + "." + split_old_version[1] + "." + split_old_version[2] + "." + revision)
#    current_version_file_w.close()
	
    old_version_number=p4.run("counter",versionCounterName,split_old_version[0] + "." + split_old_version[1] + "." + split_old_version[2] + "." + revision)
    
    return revision

def make_new_version_file():
    versionHFile=os_version_folder_path + "/" +  VERSION_H_FILENAME
    if os.path.isfile(versionHFile):
        os.chmod(versionHFile, stat.S_IWRITE)
    current_time = time.strftime(DATE_TIME_FORMAT)
    version_file = open(os_version_folder_path + "/" +  VERSION_H_FILENAME, 'w')
    template_file = open(os_version_folder_path + "/" + VERSION_H_TEMPLATE, 'r')

    major_version_num = get_major_version_num()
    minor_version_num = get_minor_version_num()
    patch_version_num = get_patch_version_num()
    revision_num = get_revision_num()
    p4changelist = get_changelist_num()

    for line in template_file:
        temp_line = line.replace(MAJOR_VERSION_VARIABLE, major_version_num)
        temp_line = temp_line.replace(MINOR_VERSION_VARIABLE, minor_version_num)
        temp_line = temp_line.replace(PATCH_VERSION_VARIABLE, patch_version_num)
        temp_line = temp_line.replace(REVISION_VARIABLE, revision_num)
        temp_line = temp_line.replace(TIME_VARIABLE, current_time)
        temp_line = temp_line.replace(CL_VARIABLE, p4changelist)
        version_file.write(temp_line)

    version_file.close()
    template_file.close()

    print "New version number: " + get_whole_version_number()

def p4_get_version_file_path():
    result = p4.run("where", os_version_folder_path + "/version.h")[0]
    clientFile=result["depotFile"]
    return clientFile
    
def p4_edit_version_files():
    global changelist
    changelist = p4.fetch_change()
    
    depot_version_file_path=p4_get_version_file_path()

    info = p4.run_edit(depot_version_file_path)

    changelist["Description"] = "[00000] Hudson Revision Number Update\nNew Version Number: " + get_whole_version_number()
    opened_files = [depot_version_file_path]
    changelist["Files"] = opened_files

def get_whole_version_number():
    major_version_num = get_major_version_num()
    minor_version_num = get_minor_version_num()
    patch_version_num = get_patch_version_num()
    revision_num = get_revision_num()
    p4changelist = get_changelist_num()
    
    return major_version_num + "." + minor_version_num + "." + patch_version_num + "." + revision_num + "-" + p4changelist

def p4_submit_version_files():
    print "attempting to submit: "
    print changelist["Files"]
    p4.run("revert", "-a")
    p4.input = changelist
    p4.run("submit", "-i")

def p4_init(port, clientspec, user, password):
    global p4
    p4 = P4()
    p4.port = port
    p4.user = user
    p4.password = password
    p4.charset = "utf8"
    if clientspec != "":
        p4.client = clientspec
    p4.connect()
    p4.run_login()
    opened = p4.run_opened()

def usage():
    print ""
    print "Wrong arguments:\n"
    print "Usage: update_version -o<version_folder_path> -v<version_counter_name> -i<increment> -c<changelist> -s<p4port> -w<p4client> -u<p4username> -k<p4password>"
    print "\t -o<version_folder_path> - path to version folder that holds the version.h.in and version.h files"
    print "\t -v<version_counter_name> - name of the p4 counter that holds the version number for this operation (ie. 8.0.0.0)"
    print "\t -i<increment> - 0=do not incremement patch #, +1=increment patch# by n, nnnn=change patch to specd number."
    print "\t -c<changelist> - new parameter to hold the current changelist number."
    print "\t -s<p4port> - P4 client you are using."
    print "\t -u<p4user> - P4 user you are using."
    print "\t -k<p4password> - P4 password for the user."
    print "\t -w<p4client> - P4 client you are using."

if __name__ == '__main__':
    main()
