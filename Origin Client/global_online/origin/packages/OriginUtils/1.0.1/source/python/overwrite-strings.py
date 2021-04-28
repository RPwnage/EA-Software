'''
Created on August 3, 2009

@author: cpasillas
'''

import os
import fnmatch 
import sys
import re
import shutil
import smtplib
from email.MIMEText import MIMEText
from types import ListType
from P4 import P4, P4Exception

p4 = None
changelist = None

#p4 path of Core directory
basepath = None

#destination paths mapped to their exported string XML file patterns
#Add any new paths for files to this and the next list and they will be updated
PATH_PATTERN_LIST_OF_LISTS = [
	["originClient/dev/resources/lang", "*OriginStrings*"],
  ["originClient/dev/runtime/lang", "*CoreStrings*"],
	["OriginBootstrap/dev/source/Resources/lang", "*Bootstrap*"],
]

#portions of paths in p4 to be opened
P4_DEST_PATHS = [
	"/originClient/dev/resources/lang/...",
	"/originClient/dev/runtime/lang/...",
	"/OriginBootstrap/dev/source/Resources/lang/...",
]

#INSTALLER_WINDOWS_DEST_PATH = "../DownloadManagerInstallerNSI/resources"

#source paths for language strings
XML_FILE_SOURCE_PATH = 'originClient\dev\\temp\lang\XML\\'
#NSH_FILE_SOURCE_PATH = 'target\Translations.nsh'



def main():
    """
    First function that is run when the script is executed.
    arguments should be [directorypath, depotdirectorypath, clientspec, user, password, submit]
    directorypath: the directory of the 'Language' project on the local filesystem e.g. C:\p4\online\dev\EACore_RC_7.x\Core\Language
    depotdirectorypath: the p4 depot path to the 'Language' project e.g. //online/dev/EACore_RC_7.x/Core/Language/
    clientspec: the name of the p4clientspec to use
    user: p4 user name to use
    password: password for p4 user
    submit: pass anything here if you want the script to submit updated files to p4!
    """
    global changelist
    global basepath
    
    if len(sys.argv) not in range(6,8):
        usage()
        sys.exit(1)
    
    depotdirectorypath = sys.argv[2]
    clientspec = sys.argv[3]
    user = sys.argv[4]
    password = sys.argv[5]
    
    #e.g. //online/dev/EAAccess/3.0/Core
    #basepath = "/".join(depotdirectorypath.split("/")[:-1])
    basepath = depotdirectorypath
    
    print "basepath = " + basepath
    print "depotdirectorypath = " +depotdirectorypath
    print "clientspec = " + clientspec
    print "user = " + user
    print "password = " + password
    
    initP4(depotdirectorypath, clientspec, user, password)
    cleanUp()
    
    #changelist to create and eventually submit
    changelist = p4.fetch_change()
    changelist["Description"] = "[nnnnn] Hudson Language Strings update: see CL for affected files"
    
    openLanguageFilesForEdit()
    overwriteOrAddLanguageFiles()
    revertUnchangedFiles()
    
    #get up to date version of changelist after file revert
    changelist = p4.fetch_change(changelist["Change"])
    
    #if there are any arguments passed past the first 5, submit
    if len(sys.argv) > 6:
        submitFiles()
        
    else:
        print "Skipping submit"
        deleteChangelistIfEmpty()
    p4.disconnect()


def cleanUp():
    #revert files, leftover checkouts may interfere with this script otherwise
    try:
        print "Reverting all opened files on this client..."
        result = p4.run_revert(basepath + "/...")
        for revertedFileInfo in result:
            print "Reverted: " + revertedFileInfo['clientFile']
    except P4Exception:
        for w in p4.warnings:
            print "Warning: " + w
        for e in p4.errors: # Display errors
            print e
        if len(p4.errors) > 0:
            sys.exit(1)
    pendingChangelists = p4.run_changes("-u", p4.user, "-c", p4.client, "-s", "pending")
    for pendingChangelist in pendingChangelists:
        print "Deleting pending changelist " + pendingChangelist['change'] + ", " + pendingChangelist['desc']
        p4.run_change('-d', pendingChangelist['change'])

def initP4(depotdirectorypath, clientspec, user, password):
    global p4
    p4 = P4()
    p4.port = "eac-p4proxy:4487"
    p4.user = user
    p4.password = password
    p4.charset = "utf8"
    p4.client = clientspec
    p4.connect()
    p4.run_login()
    opened = p4.run_opened()


def openLanguageFilesForEdit():
    print "Opening files for edit..."
    currentpath = basepath
    try:
        for path in P4_DEST_PATHS:
            currentpath  = basepath + path
            print "Opening for edit: " + currentpath 
            info = p4.run_edit(currentpath)
    except P4Exception:
        print "Could not open files for edit: %s" %(currentpath)
        for w in p4.warnings:
            print "Warning: " + w
        for e in p4.errors: # Display errors
            print e
        if len(p4.errors) > 0:
            sys.exit(1)

def overwriteOrAddLanguageFiles():
    global changelist
    #get list of xml files separated by which component they belong to (filter on filenames)
    xmlLanguageFiles = os.listdir(XML_FILE_SOURCE_PATH)
    #prepend the relative path onto each filename, e.g. target\XML\ProgressStrings_th_TH.xml
    xmlLanguageFiles = map(lambda relpath: XML_FILE_SOURCE_PATH + relpath, xmlLanguageFiles)
    
    #copy language file for installer over to installer directory
    #shutil.copy(NSH_FILE_SOURCE_PATH, INSTALLER_WINDOWS_DEST_PATH)
    
    openedFiles = []
    #copy xml files to their needed directories for other projects, also adds copied files to openedFiles list
    for pathpattern in PATH_PATTERN_LIST_OF_LISTS:
        openedFiles.extend(copyFiles(fnmatch.filter(xmlLanguageFiles, pathpattern[1]), pathpattern[0]))
    
    changelist["Files"] = openedFiles
    print "Attempting to create changelist with files: " 
    print changelist["Files"]
    #spec = p4.run_add('-t', "binary", changelist["Files"])
    #print "[DEBUG]The spec:"
    #print spec
    message = p4.save_change(changelist)
    #set changelist number assigned to this changelist
    changelist["Change"] = message[0].split(" ")[1]
    

def copyFiles(files, destPath):
    copiedFiles = []
#    destParentProject = "/".join(destPath.split("/")[1:])
    destParentProject = destPath
#    print "Parent project path = " + destParentProject
    hudsonHome=os.path.normpath(os.getenv("HUDSON_HOME","C:/ebisu/hudson"))
    hudsonJob=os.getenv("JOB_NAME","Language-Strings-Update-Ebisu-main")
    for file in files:
        print "Copying file %s to %s" %(file, destPath)
        fullP4Path = "/".join([basepath, destParentProject, os.path.basename(file)])
        relativeWindowsPath = destPath +  "/" + os.path.basename(file)
        #In case this is a new file, open it for add
        p4.run_add('-t', "binary", relativeWindowsPath)
        shutil.copy(file, hudsonHome + "/jobs/" + hudsonJob + "/workspace/origin/client/" + destPath)
        #add p4 path to file to openedFiles list
        copiedFiles.append(fullP4Path)
    return copiedFiles


def revertUnchangedFiles():
    print "Reverting all unchanged files"
    p4.run_revert("-a")


def submitFiles():
    global changelist
    
    if deleteChangelistIfEmpty():
        return
    
    printAndEmailChangelist()
    
    try:
        p4.run_submit("-c", changelist["Change"])
        print "Submit successful."
    except P4Exception:
        print "Could not open submit files!"
        for e in p4.errors: # Display errors
            print e
        sys.exit(1)


def printAndEmailChangelist():
    emailtext = ""
    currtext = ""
    print "Submitting changelist with these details:"
    for (k, v) in changelist.items():
        if isinstance(v, ListType):
            currtext = "%s:" %(k)
            emailtext += currtext + "\n"
            print currtext
            for listitem in changelist[k]:
                print listitem
                emailtext += listitem + "\n"
            emailtext += "\n"
        else:
            currtext = "%s:\t%s" %(k, v)
            print currtext
            emailtext += currtext + "\n\n"
    sendEmail(emailtext)


def sendEmail(text):
  smtpserver = "mailhost.ea.com"
  AUTHREQUIRED = 0 # if you need to use SMTP AUTH set to 1
  
  RECIPIENTS = ['EAAccessEAHQ@ea.com']
  SENDER = 'EAAccessEAHQ@ea.com'
  MESSAGE_INTRO = "Here are the details for the changelist of modified and/or new language strings.\n\n"
  
  msg = MIMEText(MESSAGE_INTRO + text)
  
  msg['TO'] = "; ".join(RECIPIENTS)
  msg['FROM'] = SENDER
  msg['SUBJECT'] = "Hudson auto-generated notification of updated language strings!" 
  
  session = smtplib.SMTP(smtpserver)
  
  print ""
  print "Sending email to recipients: "
  for recipient in RECIPIENTS:
    print recipient
  smtpresult = session.sendmail(SENDER, RECIPIENTS, msg.as_string())
  
  if smtpresult:
      errstr = ""
      for recip in smtpresult.keys():
          errstr = """Could not delivery mail to: %s
  
  Server said: %s
  %s
  
  %s""" % (recip, smtpresult[recip][0], smtpresult[recip][1], errstr)
      raise smtplib.SMTPException, errstr
    

def deleteChangelistIfEmpty():
    if "Files" not in changelist:
        print "No changed files, deleting empty changelist"
        p4.delete_change(changelist["Change"])
        return True
    return False


def usage():
    print ""
    print "Usage: " + os.path.basename(sys.argv[0]) + " directorypath depotdirectorypath clientspec user password [submit]"
    print ""
    print "directorypath: the directory of the 'Language' project on the local filesystem e.g. C:\\p4\\online\\dev\\EACore_RC_7.x\\Core\\Language"
    print "depotdirectorypath: the p4 depot path to the 'Language' project e.g. //online/dev/EACore_RC_7.x/Core/Language/"
    print "clientspec: the name of the p4clientspec to use"
    print "user: p4 user name to use"
    print "password: password for p4 user"
    print "submit: pass anything here if you want the script to submit updated files to p4!" 

if __name__ == '__main__':
    main()
