# 
# remoteutility.py
#
# This is a helper utility script that is used by all remote*.py scripts
# It contains common functions such as providing ssh connections and printing outputs to and from linux hosts
# It also contains common definitions used by multiple scripts.
# Prerequisites: 1) Make sure python is installed and the executable path is set in your environment
#                2) please install pip ( python package installer ) from https://pip.pypa.io/en/stable/
#                3) please install paramiko using pip  "python -m pip install paramiko" for details please refer to http://www.paramiko.org/installing.html#paramiko-itself

from __future__ import print_function
import paramiko
import argparse
import time
import sys, os
import re
import glob

# host for building services
host = "eadpl0041.bio-sjc.ea.com"

class Project:
    def __init__(self,absPath):
        folders = absPath.split("\\")
        index = 0;
        for folder in folders:
            if folder == "games":
                break
            else:
                index += 1
        if index == -1:
            print("Error! invalid path provided, games folder not found.")
        else:
            self.title = folders[index + 1]
            self.year = folders[index + 2]
            self.gen = folders[index + 3]
            self.branch = folders[index + 4]
        if self.gen == "GenX":
            self.platforms = ["pc", "ps4", "ps5", "xone", "xbsx", "stadia"]
        else:
            print("Error! invalid generation found!")

        # build a map of cfg files with environments
        self.cfgDict = {}
        cfgs = glob.glob(Cfg().binPath+'server_*.cfg')
        for cfg in cfgs:
            filename = cfg.split("\\")[1]
            tokens = filename.split("_")
            suffix = "";
            if len(tokens) < 3 or len(tokens) > 4:
                print("Error! malformed cfg filename:" + filename)
            else:
                environment = tokens[1]
                if len(tokens) < 4:
                    platform = tokens[2].rsplit(".")[0]
                else:
                    platform = tokens[2]
                    suffix = tokens[3].rsplit(".")[0]
            for supportedPlatform in self.platforms:
                if platform == supportedPlatform:
                    if not environment in self.cfgDict:
                        self.cfgDict.update({environment:{suffix:[filename]}})
                    else:
                        if not suffix in self.cfgDict[environment]:
                            self.cfgDict[environment].update({suffix:[filename]})
                        else:
                            self.cfgDict[environment][suffix].append(filename)
#        print out full cfg dictionary for debugging purposes
#        print(self.cfgDict)

    def findCfgFiles(self, environment = "", suffix = ""):
        if environment == "":
            env = self.branch
        else:
            env = environment
        if not suffix in self.cfgDict[env]:
            print("Error! suffix: "+ suffix + " not supported in environment: " + env)
            return {}
        else:
            return self.cfgDict[env][suffix]

    def findCfgFile(self, platform, environment= "", suffix = ""):
        cfgFileName = ""
        cfgFiles = self.findCfgFiles(environment,suffix)
        for cfgFile in cfgFiles:
            if cfgFile.find(platform) != -1:
                cfgFileName = cfgFile
        return cfgFileName

    def supportedEnvironments(self):
        environments = list(self.cfgDict.keys())
        for environment in environments:
            if(environment == "test") or (environment == "dev"):
                environments.remove(environment)
        return environments

    def supportedSuffixes(self):
        suffixes = []
        for environment in self.supportedEnvironments():
            for suffix in list(self.cfgDict[environment]):
                if not suffix == "":
                    suffixes.append(suffix + "(" + environment +" only)")
        if len(suffixes) == 0:
            suffixes = "none"
        return suffixes

    def isPlatformSupported(self, platform):
        supported = False 
        for supportedPlatform in self.platforms:
            if(platform == supportedPlatform) or (platform == "") :
                supported = True
                break
        if not supported:
            print("Error! platform: " + platform + " not supported! Please choose one of the following:")
            print(str(self.platforms).strip("[]"))
            return False
        else:
            return True
            
    def isEnvironmentSupported(self, environment):
        supported = False
        for supportedEnvironment in self.supportedEnvironments():
            if (environment == supportedEnvironment) or (environment == ""):
                supported = True
                break
        if not supported:
            print("Environment/Suffix " + environment + " not supported! Please choose one of the following:")
            print(str(self.supportedEnvironments()).strip("[]"))
            return False
        else:
            return True
        
class Cfg:
    def __init__(self):
        self.binPath = "../../bin/"

    def findBranch(self):
        return self.findStringFromFile("package.cfg", "branch")

    def findServiceHost(self, cfgFileName):
        return self.findStringFromFile(cfgFileName, ".bio-").rstrip().strip("[]")

    def findProxyHost(self, cfgFileName):
        serviceHost = self.findServiceHost(cfgFileName)
        return "gsproxy-game" + serviceHost[serviceHost.find("."):]

    def findEnv(self, cfgFileName):
        return self.findStringFromFile(cfgFileName, "env ")
        
    def findServiceName(self, cfgFileName):
        serviceName = self.findStringFromFile(cfgFileName, "serviceName")
        prefix = self.findStringFromFile(cfgFileName, "prefix")
        if not prefix == "":
            serviceName = serviceName.replace("%(prefix)s", prefix)
        platform = self.findStringFromFile(cfgFileName, "platform")
        if not platform == "":
            serviceName = serviceName.replace("%(platform)s", platform)
        suffix = self.findStringFromFile(cfgFileName, "suffix")
        if not suffix == "":
            serviceName = serviceName.replace("%(suffix)s", suffix)
        return serviceName
        
    def findStringFromFile(self,fileName, search):
        with open(self.binPath+fileName, 'r') as file:
            lines = file.readlines()
            for line in lines:
                if re.search(search, line):
                    if re.search("=", line):
                        return line.rstrip().split("=")[1].strip(" ")
                    else: 
                        return line
            return ""
                        
    def findRunset(self, cfgFileName):
        runset = {}
        startLine = self.findIndexFromFile(cfgFileName, "runset:")
        endLine = self.findIndexFromFile(cfgFileName, "redis_nodes:")
        with open(self.binPath+cfgFileName, 'r') as file:
            lines = file.readlines()
            lineNumber = 1
            for line in lines:
                if( lineNumber > startLine ) and ( lineNumber < endLine ):
                    list = line.rstrip().split("=")
                    if len(list) == 2:
                        key = list[0].strip(" ").strip("\t")
                        value = int(list[1].strip(" "))
                        runset.update({key : value})
                    elif len(list) == 1:
                        key = list[0].split(":")[0].strip(" ").strip("\t")
                        value = int(list[0].split(":")[1].strip(" "))
                        runset.update({key : value})
                    else:
                        print("Error unexpected runset format encountered, check " + cfgFilename + " for errors.")
                        return {}
                lineNumber += 1;
        return runset
        
    def findIndexFromFile(self,fileName, search):
        with open(self.binPath+fileName, 'r') as file:
            lines = file.readlines()
            lineNumber = 1
            for line in lines:
                if re.search(search, line):
                    return lineNumber
                else:
                    lineNumber += 1
class Ssh:
    def __init__(self, host, username, password):
        self.port = 22;
        self.waitForOutputTimeInterval = 0.5

        self.ssh = paramiko.SSHClient()
        self.ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
        self.ssh.connect(host, self.port, username, password)
        self.channel = self.ssh.invoke_shell()
        self.channel.settimeout(10)
        
    def __del__(self):
        self.ssh.close()

    def isConnectionClosed(self):
        buf = ''
        while not (buf.endswith('$ ') or buf.endswith('# ')):
            if self.channel.recv_ready():
                temp = str(self.channel.recv(9999).decode('utf-8'))
                if temp.find('Connection closed by') != -1:
                    return True
                elif temp.find('continue connecting (yes/no)?') != -1:
                    self.channel.send("yes \n")
                else:
                    buf += temp
            else:
                time.sleep(self.waitForOutputTimeInterval)
        return False
        
    def ignoreResult(self):
        buf = ''
        while not (buf.endswith('$ ') or buf.endswith('# ')):
            if self.channel.recv_ready():
                temp = str(self.channel.recv(9999).decode('utf-8'))
                if temp.find('continue connecting (yes/no)?') != -1:
                    self.channel.send("yes \n")
                else:
                    buf += temp
            else:
                time.sleep(self.waitForOutputTimeInterval)

    def printResult(self):
        success = True
        buf = ''
        waitTimeElapsed = 0
        while not (buf.endswith('$ ') or buf.endswith('# ')):
            if self.channel.recv_ready():
                temp = str(self.channel.recv(9999).decode('utf-8'))
                if temp.find('continue connecting (yes/no)?') != -1:
                    self.channel.send("yes \n")
                if (temp.find('A$') == -1) and (temp.find('Pushing') == -1) and (temp.find('Layer already exists') == -1):
                    buf += temp
                    print(buf)
                if(success == True) and (temp.find("FAILED") != -1 or temp.find("error ") != -1):
                    success = False
            else:
                if waitTimeElapsed >= 5:
                    print(".", end='')
                time.sleep(self.waitForOutputTimeInterval)
                waitTimeElapsed += self.waitForOutputTimeInterval
        return success

    def execute(self, command, exitOnError = True, printResult = True):
        self.channel.send(command + "\n")
        if printResult == True: 
            if (self.printResult() == False) and (exitOnError == True):
                sys.exit(0)

    def getNumericOutput(self):
        buf = ''
        while not (buf.endswith('$ ') or buf.endswith('# ')):
            buf = str(self.channel.recv(1024).decode('utf-8'))
            time.sleep(self.waitForOutputTimeInterval)
        # remove the prompt from the result
        index = buf.rfind("\n")
        buf = buf[:index]
        # Remove all output above the number
        index = buf.rfind("\n")
        if not index == -1:
            buf = buf[index:]
        return int(buf)

    def getStringOutput(self):
        buf =''
        while not (buf.endswith('$ ') or buf.endswith('# ')):
            buf = str(self.channel.recv(1024).decode('utf-8'))
            time.sleep(self.waitForOutputTimeInterval)
        index = buf.find("\n")
        if not index == -1:
            return buf[:index-1]
        else:
            return buf
    
    def getTransport(self):
        return self.ssh.get_transport()
