#!/usr/bin/python25
import sys, re, commands, urllib2
from xml.dom.minidom import Document
import base64
from xml.sax.saxutils import escape

class CrashReport:
    def __init__(self):
        # report version is 2
        self.version = '2'
        self.sessionid = ''
        self.type = 'crash'
        self.sku = ''
        self.createtime = ''
        self.buildsig = ''
        self.systemConfig = ''
        self.catId = 'unknown'
        self.stack = ''
        self.threads = ''
        self.screenshot = ''
        self.memdump = ''
        self.contextdata = ''

        self.reportData = ''

    def saveToFile(self,path):
        bugsentryReportFile='%s/bugsentryReport.%s.xml' % (path,self.catId)
        f = open(bugsentryReportFile, 'w')
        f.write(self.reportData)
        f.close()

    def toXmlDoc(self):
        report = Document()
        reportElement = report.createElement("report")
        versionElement = report.createElement("version")
        sessionidElement = report.createElement("sessionid")
        typeElement = report.createElement("type")
        skuElement = report.createElement("sku")
        createtimeElement = report.createElement("createtime")
        buildsigElement = report.createElement("buildsignature")
        systemcfgElement = report.createElement("systemconfig")
        catIdElement = report.createElement("categoryid")
        stackElement = report.createElement("stack")
        threadsElement = report.createElement("threads")
        screenshotElement = report.createElement("screenshot")
        memdumpElement = report.createElement("memdump")
        contextdataElement = report.createElement("contextdata")

        # Append childs to report
        report.appendChild(reportElement)

        reportElement.appendChild(versionElement)
        reportElement.appendChild(sessionidElement)
        reportElement.appendChild(typeElement)
        reportElement.appendChild(skuElement)
        reportElement.appendChild(createtimeElement)
        reportElement.appendChild(buildsigElement)
        reportElement.appendChild(systemcfgElement)
        reportElement.appendChild(catIdElement)
        reportElement.appendChild(stackElement)
        reportElement.appendChild(threadsElement)
        reportElement.appendChild(screenshotElement)
        reportElement.appendChild(memdumpElement)
        reportElement.appendChild(contextdataElement)

        # Add stack values
        stackprintText = report.createTextNode(self.stack)
        stackElement.appendChild(stackprintText)
        
        reportversionText = report.createTextNode(self.version)
        versionElement.appendChild(reportversionText)
        
        reportTypeText = report.createTextNode(self.type)
        typeElement.appendChild(reportTypeText)
        
        systemConfigText = report.createTextNode(self.systemConfig)
        systemcfgElement.appendChild(systemConfigText)
        
        skuText = report.createTextNode(self.sku)
        skuElement.appendChild(skuText)
        
        buildsigText = report.createTextNode(self.buildsig)
        buildsigElement.appendChild(buildsigText)
        
        createtimeText = report.createTextNode(self.createtime)
        createtimeElement.appendChild(createtimeText)
        
        catIdText = report.createTextNode(self.catId)
        catIdElement.appendChild(catIdText)

        contextDataBase64Text = report.createTextNode(base64.b64encode(escape(self.contextdata)))
        contextdataElement.appendChild(contextDataBase64Text)

        self.report = report
        return report

    def send(self,env=None):
        if (env==None):
            # assume prod by default
            env = 'https://rl.data.ea.com'
        bugSentryUrlTemplate = '%s/bugsentry/submit/'

        bugSentryUrl = bugSentryUrlTemplate % env
        self.reportData = self.toXmlDoc().toxml()
        err = 1

        try:
            req = urllib2.Request( bugSentryUrl, self.reportData)
            response = urllib2.urlopen(req)
            the_page = response.read()
        except urllib2.HTTPError:
            (type, value,traceback) = sys.exc_info()
            err = -1

        return err
        
    def findstack(self,input):
        # skip stack that reads "#6  0x0000000000000000 in ?? ()"   
        result = re.findall(r'#[0-9]+\s+(0x[0-9a-fA-F]+)\s+?in\s+(?!\?\?)', input)     
        if (len(result) == 0):
            return -1
        return ' '.join(result)

    def findstacknames(self,input):
        # extract the function names
        # skip stack that reads "#6  0x0000000000000000 in ?? ()"   
        result = re.findall(r'#[0-9]+\s+0x[0-9a-fA-F]+\s+?in\s+([_0-9a-zA-Z]+)', input)     
        if (len(result) == 0):
            return None
        return result
    
def ReportToBugSentry(infocorefile, executablefile, sku, buildsignature, bugsentryaddress, pid, targetPath):
    # test driver for this module
    crashReport = CrashReport()

    # get the arguments
    infocorefilebase = infocorefile
    
    # decompress if necessary
    coreNameSplit = 0
    if (infocorefile.endswith('.gz')):
        commands.getoutput('gunzip %s' % infocorefile)
        infocorefilebase = infocorefile.split('.gz')[0]
        coreNameSplit = 1
  
    # extract the callstack
    btCommand = 'gdb -ex "bt" -batch -n -q -c %s %s' % (infocorefilebase,executablefile)
    fullbt = commands.getoutput(btCommand)
    
    #recompress if necessary
    if (coreNameSplit == 1):
        commands.getoutput('gzip %s' % infocorefilebase)

    # get the date the report is generated (not necessarily when the crash occured)
    dateCommand = "ls -l -c --time-style='+%s' %s" % ("%Y-%m-%d %H:%M:%S",infocorefile)
    createdDateResult = commands.getoutput(dateCommand).split()
    dateResult = createdDateResult[5]
    timeResult = createdDateResult[6]
    crashReport.createtime = ' '.join([dateResult, timeResult])
    print crashReport.createtime
    
    # isolate the stack addresses from the backtrace
    crashReport.stack = crashReport.findstack(fullbt)
    print crashReport.stack
    if (crashReport.stack == -1):
        return -1
    
    # set the build signature
    if (buildsignature == None):
        crashReport.buildsig = 'dirtycast.undefined'
    else:
        crashReport.buildsig = buildsignature
        
    # get the system config
    crashReport.systemConfig = commands.getoutput('uname -a')
    
    # set the bugsentry sku, which is not the same as the blaze service name
    if (sku==None):
        crashReport.sku = "ea.eadp.dirtycast.unix"
    else:
        crashReport.sku = sku
    
    # use the top of the stack for the category id
    stacknames = crashReport.findstacknames(fullbt)
    if (len(stacknames) == 0):
        crashReport.catId = crashReport.stack.split()[0]
    else:
        crashReport.catId = crashReport.stack.split()[0] + "-" + stacknames[0]
    print crashReport.catId
    
    # stick the rest of the backtrace in the contextData
    # be careful what goes in here as the report is XML formatted and special characters might do some harm
    pidstring = "pid: %s\n" % pid
    corefilestring = "core file: %s/%s\n" % (targetPath, infocorefile)
    crashReport.contextdata = pidstring + corefilestring + fullbt 

    # try to send
    if (crashReport.send(bugsentryaddress) == 1):
        return 1
    else:
        return -1