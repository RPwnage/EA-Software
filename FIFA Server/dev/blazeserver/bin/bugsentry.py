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

    def send(self,env="test"):
        bugSentryUrlTemplate = '%s/bugsentry/submit/'
        bugSentryServer={'test': 'https://rl-test.data.ea.com',
                 'prod' : 'https://rl.data.ea.com' }
        defaultServer = bugSentryServer['test']
        bugSentryUrl = bugSentryUrlTemplate % bugSentryServer.get(env, defaultServer)

        self.reportData = self.toXmlDoc().toxml()

        try:
            req = urllib2.Request( bugSentryUrl, self.reportData)
            response = urllib2.urlopen(req)
            the_page = response.read()
            message = "submited report to %s " % bugSentryUrl
        except urllib2.HTTPError:
            (type, value,traceback) = sys.exc_info()
            message = 'exception: type:%s value:%s trace:%s' % (type,value,traceback)

        return message
        
    def findstack(self,input,stripbt=True):
        if stripbt:
            t1 = input.find('full backtrace')
            t2 = input.find('info thread')
            bt = input[t1:t2]
        else:
            t1 = input.find('<signal handler called>')
            bt = input[t1:]

        # skip stack that reads "#6  0x0000000000000000 in ?? ()"   
        result = re.findall(r'#[0-9]+\s+(0x[0-9a-fA-F]+)\s+?in\s+(?!\?\?)', bt)     
        return ' '.join(result)
    
    def parseEnv(self):
        try:
            f = open('../etc/deploy.env','r')
            env = re.findall(r'ENV=(.*)', f.read())[0]
            if (env!='prod' ):
                print 'Environment is not prod(Actual: ENV=%s). Using stest bugsentry server. ' % env
                env='stest'
        except:
            (type, value,traceback) = sys.exc_info()
            message = 'exception: type:%s value:%s trace:%s' % (type,value,traceback)
            print 'Couldn''t read config(%s). Assume stest.' % message
            env='stest'
        return env

def main():
    # test driver for this module
    crashReport = CrashReport()

    infocorefile = sys.argv[1]
    f = open(infocorefile, 'r')
    infoCore=f.read()

    crashReport.stack = crashReport.findstack(infoCore,False)
    print crashReport.stack
    crashReport.buildsig = 'blaze-dev3-%s-%s' % (sys.argv[2],sys.argv[3])
    crashReport.systemConfig = commands.getoutput('uname -a')
    
    #hard code sku for demo
    crashReport.sku = 'ea.gos.blazeserver.13.1x.test'
    crashReport.createtime = '2012-01-12 21:33:22-08:00'

    global coreFileTimeStrBugsentry
    import hashlib
    
    crashReport.catId = hashlib.md5(' '.join(crashReport.stack.split()[0:3])).hexdigest()
    crashReport.contextdata = infoCore

    crashReport.send()

if __name__ == "__main__":
    main()
