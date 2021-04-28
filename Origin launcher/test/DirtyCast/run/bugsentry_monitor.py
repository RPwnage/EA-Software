#!/usr/bin/python26
import sys, re, commands, time, smtplib, datetime, socket, fcntl
from email.mime.multipart  import MIMEMultipart
from email.mime.text import MIMEText
from bugsentry import ReportToBugSentry
from os import walk, path, makedirs, rename

# try to lock the email interval file
# if the read the last sent time from the file if email interval period (min) has passed send email
def checkEmailInterval(f, targetPath, emailInterval):
    try:
        fcntl.flock(f, fcntl.LOCK_EX | fcntl.LOCK_NB)
        line = f.readline()
        f.seek(0)
        print "Last Email Sent: " + line + " Current time: " + str(time.time())
        #if the first line is empty we just created the file so just return true and out the current time to it
        if line == "":
            f.write(str(time.time()))
            return True
        #check if the mail interval has elapsed
        else:
            lastSentTime = float(line)
            if ((time.time() - lastSentTime) > (emailInterval * 60)):
                f.truncate()
                f.write(str(time.time()))
                print "Email interval has past we will be sending a crash alert email"
                return True
            else:
                print "Email interval has not past. Not sending email"
                return False
    except IOError:
        print "Another Script is running skip sending the email"
        return False

def logTail(f, window=25):
    if window == 0:
        return []
    BUFSIZE = 1024
    f.seek(0, 2)
    bytes = f.tell()
    size = window + 1
    block = -1
    data = []
    while size > 0 and bytes > 0:
        if bytes - BUFSIZE > 0:
            # Seek back one whole BUFSIZE
            f.seek(block * BUFSIZE, 2)
            # read BUFFER
            data.insert(0, f.read(BUFSIZE))
        else:
            # file too small, start from begining
            f.seek(0,0)
            # only read what was not read
            data.insert(0, f.read(bytes))
        linesFound = data[0].count('\n')
        size -= linesFound
        bytes -= BUFSIZE
        block -= 1
    return ''.join(data).splitlines()[-window:]

def sendmail(inCoreFile, numCoreGen, executablefile, corePath, logFile, sender, recipients, gamename, version, runsetName, siteName, platform, env, changelist, pid, emailInterval):
    #get hostname and construct dc name
    print "Constructing email"
    hostname = socket.gethostname()
    dirtycastName = runsetName + "." + platform + "." + version + "." + changelist + "." + env.lower()

    #construct email body
    msg = MIMEMultipart()
    msg["Subject"] = "[DirtyCast][Monitor] DirtyCast on " + hostname + " crashed"
    msg["From"] = sender
    msg["To"] = recipients

    bodyText = "DirtyCast Server has generated core files.\n"
    bodyText = bodyText + "DirtyCast Name: " + dirtycastName + "\n"
    bodyText = bodyText + "Hostname: " + hostname + "\n"
    bodyText = bodyText + "Site Name: " + siteName + "\n"
    bodyText = bodyText + "Platform: " + platform + "\n"
    bodyText = bodyText + "Pid: " + pid + "\n"
    bodyText = bodyText + "Time: " + datetime.datetime.now().isoformat() +"\n\n"
    bodyText = bodyText + str(numCoreGen) + " cores have been generated in the past " + str(emailInterval) + " minutes.\n\n"

    bodyText = bodyText + "Relevant logs:\n"

    #append all the logs to the body
    print "Processing email logs"

    f = open(logFile, "r")
    fullPath = path.abspath(logFile)
    bodyText = bodyText + "\n" + "Log Path: " + fullPath + "\n"
    tailLines = logTail(f)
    for line in tailLines:
        bodyText = bodyText + line + "\n"
    f.close()

    print "Processing email callstack"
    bodyText = bodyText + "\n\n" + "Relevant Callstack:\n"

    #append all the callstack info
    coreNameSplit = 0
    infocorefilebase = inCoreFile
    if (inCoreFile.endswith('.gz')):
        commands.getoutput('gunzip %s' % inCoreFile)
        infocorefilebase = inCoreFile.split('.gz')[0]
        coreNameSplit = 1
    btCommand = 'gdb -ex "bt" -batch -n -q -c %s %s' % (infocorefilebase,executablefile)
    print btCommand
    fullbt = commands.getoutput(btCommand)
    bodyText = bodyText + "Core Path: " + corePath + inCoreFile + "\n"
    bodyText = bodyText + fullbt + "\n\n"

    if (coreNameSplit == 1):
        commands.getoutput('gzip %s' % infocorefilebase)

    msg.attach(MIMEText(bodyText))

    print "Sending crash alert email"
    smtpObj = smtplib.SMTP("localhost")
    smtpObj.sendmail(msg["From"], msg["To"].split(","), msg.as_string())
    smtpObj.quit()

def main():
    sender = ""
    recipients = ""
    emailInterval = 0
    if (len(sys.argv) == 15):
        # email is provided read in the arguments
        sender = sys.argv[12]
        recipients = sys.argv[13]
        emailInterval = int(sys.argv[14])
    elif (len(sys.argv) == 13):
        emailInterval = int(sys.argv[12])
    elif (len(sys.argv) == 12):
        # if email is not provided set to ""
        sender = ""
        recipients = ""
        emailInterval = 0
    else:
        print "Usage: \npython bugsentry_monitor.py <environment> <gamename> <version> <exe> <pid> <name> <sitename> <platform> <changelist> <index> <parentindex> <crashemailsender> <crashemailrecipients> <crashemailinterval>"
        print "Usage: \npython bugsentry_monitor.py <environment> <gamename> <version> <exe> <pid> <name> <sitename> <platform> <changelist> <index> <parentindex>"
        return -1

    # get the arguments
    env = sys.argv[1]
    gamename = sys.argv[2]
    version = sys.argv[3]
    executablefile = sys.argv[4]
    pid = sys.argv[5]
    runsetName = sys.argv[6]
    siteName = sys.argv[7]
    platform = sys.argv[8]
    changelist = sys.argv[9]
    procIndex = int(sys.argv[10])
    parentIndex = int(sys.argv[11])

    sku = "ea.dirtycast." + gamename + "." + version
    buildsignature = gamename + "_" + version
    if (env.lower() == "prod"):
        bugsentryaddress = "https://rl.data.ea.com"
    else:
        bugsentryaddress = "https://rl-test.data.ea.com"

    print "Repoting %s cores to BugSentry" % executablefile
    print executablefile
    print sku
    print buildsignature
    print bugsentryaddress
    print pid

    # get the list of core files and logs files in the directory
    findCoresResult = []
    findLogsResult = []
    for root, dirs, files in walk("."):
        for name in files:
            if (name.startswith("core.")):
                findCoresResult.append(path.join(root, name))
            if (name.endswith(".log")):
                findLogsResult.append(path.join(root, name))

    print "Cores found:"
    print findCoresResult
    print "Logs found: "
    print findLogsResult

    if (len(findCoresResult) == 0):
        # nothing to process, exit gracefully
        return 1

    # after processing files are to be moved to the current user's home directory under the following subfolder
    # ~/core/<currentDeployFolder>/<date>
    myPath = path.realpath(__file__).split('/')
    targetPath = path.expanduser("~") + "/core/" + myPath[len(myPath)-2] + "/" + str(datetime.date.today()) + "/"

    # create the directory if it does not exist already
    if not path.exists(targetPath):
        makedirs(targetPath)

    #determine if the core for this process and log are found
    bProcessCoreLogFound = False
    logPath = ""
    if (("./core." + pid) in findCoresResult):
        for log in findLogsResult:
            logNameSplit = log.split("/")
            #check the parent index
            if ((("%04d")%parentIndex) in logNameSplit[1]) and ((("%04d") %procIndex) in logNameSplit[2]):
                bProcessCoreLogFound = True
                logPath = log
                break

    print "Log Path: " + logPath

    # need to open the emailInterval file here to make sure we lock it so other instance of this script can't mod the file
    # if we can't get a lock on the file we will skip send an email
    f = open(targetPath + "emailInterval.txt", "a+")
    bSendEmail = checkEmailInterval(f, targetPath, emailInterval)

    if (not((sender == "") or (recipients == "")) and bSendEmail and bProcessCoreLogFound):
        sendmail("./core." + pid, len(findCoresResult), executablefile, targetPath, logPath, sender, recipients, gamename, version, runsetName, siteName, platform, env, changelist, pid, emailInterval)

    # we need to hold on to the lock after we finish sending the email
    # this will unlock the emailInterval file
    f.close()

    for newCore in findCoresResult:
        print "Handling %s" % newCore
        result = ReportToBugSentry(newCore, executablefile, sku, buildsignature, bugsentryaddress, pid, targetPath)

        # if successfully handled, move to the new path
        if (result >= 0):
            rename(newCore, targetPath + newCore)

    return 1

if __name__ == "__main__":
    main()