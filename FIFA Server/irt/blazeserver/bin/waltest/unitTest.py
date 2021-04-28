#!/usr/bin/python
# Global stuff used by each of the WAL unit tests
# Each unit test should be fairly compact
# Use example.py as a template for writing unit tests

import sys, getopt, telnetlib, socket, re

# ====== Global functions ======

# Each test has the same opts
# -h hostname -p port -r server
# --hostname hostname --port port --redirect server
# The redirect is not used for HttpProtocol URI's

def getOpts(argv):
    opts, extraparams = getopt.getopt(argv[1:],'h:p:r:k:q',['hostname=','port=','redirect=','key=','quiet']);
    return opts;

def getHostname(opts):
    for o,p in opts:
        if o in ['-h','--hostname']:
            return p;
    return "localhost"; #default
    
def getPort(opts):
    for o,p in opts:
        if o in ['-p','--port']:
            return p;
    return "12004";     #default
    
def getRedirect(opts):
    for o,p in opts:
        if o in ['-r','--redirector']:
            return p;
    return "gos";       #default
    
def getVerbose(opts):
    for o,p in opts:
        if o in ['-q','--quiet']:
            return False;
    return True;        #default
    
# Use this helper method to get the contents of a given XML tag in the response     
def parseXmlElement(xml,tag,startIndex = 0):
    start = xml.find("<" + tag + ">",startIndex);
    if(start == -1):
        start = xml.find("<" + tag + " ",startIndex);
        if(start == -1):
            return None, -1;    # Opening tag not found
    start += len(tag);
    start += 2;
    end = xml.find("</" + tag + ">",start);
    if(end == -1):
        return None, -1;    # Closing tag not found
    sweetFilling = xml[start:end];
    return sweetFilling, start;         # Returned index can be used for subsequent calls
    
# Use this method to abort a test
def abort(msg):
    print "[fatal] " + msg;
    sys.exit(1);
    
# ====== URI Class ======
# Represents a full path to a given URI

# For HttpProtocol tests, this will be constructed like:
# http://<host>:<port>/<component>/<rpcpath>
# <rpcpath> includes all the URI params (eg. "poke?text=hello")

# For WalProtocol tests, this will be constructed like:
# http://<host>:<port>/<redirect>/<component>/<path>
# <path> includes all the URI params (eg. "poke?text=hello")

class URI(object):

    def __init__(self):
        self.path = None;
        self.redirect = None;
        self.component = None;
        self.host = None;
        self.port = None;
        
    def setResourcePath(self,path):
        self.path = path.lstrip("/");
        
    def setRedirect(self,redirect):
        self.redirect = redirect;
        
    def setComponent(self,name):
        self.component = name;
        
    def setHost(self,host):
        self.host = host;
        
    def setPort(self,port):
        self.port = port;
    
    def getHttpPath(self):
        httpPath = "http://" + self.host + ":" + self.port
        if(self.redirect != None):
            httpPath = httpPath + "/" + self.redirect
        httpPath = httpPath + "/" + self.redirect + "/" + self.component + "/" + self.path;
        return httpPath;
            
    def getFullPath(self):
        fullPath = "";
        if(self.redirect != None):
            fullPath = fullPath + "/" + self.redirect;
        fullPath = fullPath + "/" + self.component + "/" + self.path;
        return fullPath;

# ====== AbstractTest Class ======
# An abstract test harness class

class AbstractTest(object):

    def __init__(self,name,indent=0,verbose=True):
        self.setIndent(indent);
        self.setName(name);
        self.setVerbose(verbose);
        
    def setName(self,name):
        self.name = name;
        
    def setVerbose(self,verbose):
        self.verbose = verbose;
    
    def setIndent(self,numTabs):
        if(numTabs >= 0):
            self.indent = numTabs;
        
    def write(self,str):
        print self.indent * "\t" + "[" + self.name + "] " + (str);
    
    def doTest(self): uimpl()   # To be implemented
            
    def test(self):
        
        # passed is a boolean
        # extraData can be anything
        passed, extraData = self.doTest();
        
        if(passed == True):
            self.write("PASSED");
        else:
            self.write("FAILED");

        return passed, extraData;

# ====== MasterTest Class ======
# A test class with two counters: nFailed and nPassed
# Which represent number of failed/passed subtests

class MasterTest(AbstractTest):

    def __init__(self,name,indent=0,verbose=True):
        AbstractTest.__init__(self,name,indent,verbose);
        self.nPassed = 0;
        self.nFailed = 0;
        self.nSkipped = 0;
        
    def addResult(self,passed):
        if(passed == True):
            self.nPassed += 1;
        else:
            self.nFailed += 1;
            
    def addSkipped(self,num):
        self.nSkipped += num;

    # Just print out the number of command tests which succeeded
    def test(self):
        self.write(str(self.nPassed) + " Passed, " + str(self.nFailed) + " Failed, " + str(self.nSkipped) + " Skipped");
        return (self.nFailed == 0), None;
    
# ====== ComponentTest Class ======
# Main harness for testing a component
# The construct takes a hostname and port to construct the base of the URI

class ComponentTest(MasterTest):

    def __init__(self,name,host,port,redirect=None,indent=0,verbose=True):
        MasterTest.__init__(self,name,indent,verbose);
        self.uri = URI();
        self.uri.setHost(host);
        self.uri.setPort(port);
        self.uri.setComponent(name);
        self.uri.setRedirect(redirect);
        
    def testCommand(self,command):
        
        command.setIndent(self.indent + 1);
        command.setVerbose(self.verbose);
        command.setBase(self.uri);
    
        self.write("Testing " + command.name + " method.");
        passed, extraData = command.test();
        self.addResult(passed);
        
        return passed, extraData;

# ====== CommandTest Class ======
# This tests a single RPC command.
#
# It establishes a telnet connection to the blaze server,
# sends an HTTP request, then checks the response for HTTP/1.1 200 OK
# void responses are also acceptable.
#
# Use the testCommand method of the ComponentTest object to run instances of this test.
# The component base path will be prepended to the resource path to form a valid URI.
#
# Use addCondition to add success conditions based on the response XML.
# This class maintains a dictionary of XML element names and expected values.
# The response XML is checked to make sure that a) each element exists and b) it's value
# matches the expected value.

class CommandTest(AbstractTest):

    def __init__(self,name,method,path,key=None,indent=0,verbose=True):
        AbstractTest.__init__(self,name,indent,verbose);
        self.setHttpMethod(method);
        self.setTimeout(10);
        self.uri = URI();
        self.uri.setResourcePath(path);
        self.setSessionKey(key);
        self.xmlElem = [];
        self.xmlValues = [];
        
    def setBase(self,uri):
        self.uri.setHost(uri.host);
        self.uri.setPort(uri.port);
        self.uri.setComponent(uri.component);
        self.uri.setRedirect(uri.redirect);
        
    def setTimeout(self,seconds):
        self.timeout = seconds;

    def setHttpMethod(self,method):
        if method in ("GET","PUT","POST","DELETE","HEAD"):
            self.method = method;
        else:
            self.write("Error: " + method + " is not a valid HTTP method!");
            self.method = "";
            
    def setSessionKey(self,key):
        self.key = key;
        
    def addCondition(self,element,value):
        self.xmlElem.append(str(element));
        self.xmlValues.append(str(value).replace('\n','').replace('\t','').replace('\r',''));
        
    def doTest(self):
        
        if(self.method == ""):
            self.write("Error: HTTP method not set!");
            return False, None;
        if(self.uri == None):
            self.write("Error: URI not set!");
            return False, None;
            
        if(self.verbose == True):
            self.write(self.method + " " + self.uri.getFullPath());
                
        request = self.method + " " + self.uri.getFullPath() + " HTTP/1.1\r\n";
        if(self.key != None):
            request += "BLAZE-SESSION: " + self.key + "\r\n";
        if(self.method == "POST"):
            request += "Content-Type: application/x-www-form-urlencoded\r\n";
            request += "Content-Length: 0\r\n";
        request += "\r\n";

        t = telnetlib.Telnet();
        try:
            t.open(self.uri.host,self.uri.port);
        except socket.error:
            self.write("Error: Connection to " + self.uri.host + ":" + self.uri.port + " refused!");
            return False, None;
        t.write(request);
        
        endTransmission = []
        endTransmission.append(re.compile("^</.*",re.MULTILINE));
    
        response = None;
        try:
            response = t.expect(endTransmission,self.timeout);
            t.close();
        except EOFError:
            t.close();
        
        if(response != None and isinstance(response,tuple)):
            voidResponse = response[2].strip('\r\n\t ');
            if(voidResponse == ''):
            
                # Void response are acceptable
                self.write("Received void response.");
                
                # Return success unless there were expected XML elements
                return (len(self.xmlElem) == 0), None;      
                
            else:
                
                # Print response to stdout
                if(self.verbose == True):   
                    newline = "\n\t" + self.indent * "\t";
                    print newline + response[2].rstrip('\r\n').replace("\n",newline) + "\n";
                    
                httpOK = response[2].startswith("HTTP/1.1 200 OK");
                errors = response[2].find("<errorCode>");
                passed = (httpOK == True and errors == -1);
                
                # remove \n, \r, \t from output when testing conditions
                raw = response[2].replace('\n','').replace('\t','').replace('\r','').replace('    ','');
                
                if(passed):
                
                    # Check that the response meets all conditions
                    for i in range(0,len(self.xmlElem)):
                    
                        elem = self.xmlElem[i];
                        val = self.xmlValues[i];
                        index = 0;
                        match = None;
                        
                        # May have to search response repeatedly to find correct match
                        while True:
                            match, index = parseXmlElement(raw,elem,index);
                            if(match == val or index == -1): break;

                        # If we reached the end of the block then we didn't find a match for the condition
                        if(index == -1):
                            self.write("Condition FAILED: Could not find <" + elem + "> with value \"" + val + '\"');
                            passed = False;
                        else:
                            self.write("Condition PASSED: <" + elem + "> == " + val);
                
                return passed, response[2];
                
        return False, None;