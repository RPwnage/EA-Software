# 
# remotecheck.py
#
# This is a helper script that would check the health of a blazeserver via a Linux host   
# The script can be run on windows with user's desired configurations, username, specific host and password allocated by https://se-pm.bio-iad.ea.com:8443/.
# Prerequisites: 1) Make sure python is installed and the executable path is set in your environment
#                2) please install pip ( python package installer ) from https://pip.pypa.io/en/stable/
#                3) please install paramiko using pip  "python -m pip install paramiko" for details please refer to http://www.paramiko.org/installing.html#paramiko-itself

from __future__ import print_function
import argparse
from remoteutility import *

def checkServiceHealth(ssh, cfgFileName):
    ssh.execute("ssh -A " + Cfg().findProxyHost(cfgFileName), printResult = False)
     #flush the output stream
    ssh.ignoreResult()
    ssh.execute("ssh -A " + Cfg().findServiceHost(cfgFileName), printResult = False)
    if not ssh.isConnectionClosed():
        serviceName = Cfg().findServiceName(cfgFileName)
        print("Connected to " + Cfg().findServiceHost(cfgFileName) + ", checking instances of " + serviceName)
        runset = Cfg().findRunset(cfgFileName)
        for name, expectedInstances in runset.items(): 
            command =  "ps aux | grep  "+ serviceName + " | grep -v 'app/run/monitor' | grep " + name + " | wc -l"
            ssh.execute(command, printResult = False)
            actualInstances = ssh.getNumericOutput()
            if actualInstances >= expectedInstances:
                if actualInstances == 0:
                    print("Instance " + name + "* at " + serviceName + " is not found!")
                elif actualInstances == 1: 
                    print(str(actualInstances) + " instance of " + name + " is up and running.")
                else: 
                    print(str(actualInstances) + " instances of " + name + " are up and running.")
            else:
                print("Error! Unexpected number of instances detected for " + name + "! Expected: " + str(expectedInstances) + " Actual: " + str(actualInstances))
        ssh.execute("exit", printResult = False)
        #flush the output stream
        ssh.ignoreResult()
    else: 
        print("Connection to " + Cfg().findServiceHost(cfgFileName) + " unsuccessful. Connection closed by host.")
    ssh.execute("exit", printResult = False)
    #flush the output stream
    ssh.ignoreResult()

def check(project, platform, environment, suffix, host, user, password):
    if project.isPlatformSupported(platform):
        if project.isEnvironmentSupported(environment):
            ssh = Ssh(host, user, password)
            try:
                if not platform == "":
                    checkServiceHealth(ssh, project.findCfgFile(platform, environment, suffix))
                else:
                    cfgFiles = project.findCfgFiles(environment, suffix)
                    for cfgFileName in cfgFiles:
                        checkServiceHealth(ssh, cfgFileName)
            except Exception as e:
                print(e)

project = Project(os.path.abspath(__file__))

parser = argparse.ArgumentParser()
parser.add_argument("--platform", nargs='?', default="", help="Supported platforms: " + str(project.platforms).strip("[]") +". Check for all platforms if not specified")
parser.add_argument("--env", nargs='?', default="", help="Supported environments: " + str(project.supportedEnvironments()).strip("[]"))
parser.add_argument("--suffix", nargs='?', default="", help="Supported suffixes: " + str(project.supportedSuffixes()).strip("[]"))
parser.add_argument("user", help="username to login to https://se-pm.bio-iad.ea.com:8443/")
parser.add_argument("host", help="host (from slack channel, reserve timeslot at https://se-pm.bio-iad.ea.com:8443/)")
parser.add_argument("password", help="password in single or double brackets i.e. 'SOMEPASSWORD' (from slack channel, reserve timeslot at https://se-pm.bio-iad.ea.com:8443/)")
args = parser.parse_args()

check(project, args.platform, args.env, args.suffix, args.host, args.user, args.password)
