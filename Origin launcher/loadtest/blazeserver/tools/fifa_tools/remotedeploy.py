# 
# remotedeploy.py
#
# This is a helper script that would deploy a blazeserver on the Linux host   
# The script can be run on windows with user's desired configurations, username, specific host and password allocated by https://se-pm.bio-iad.ea.com:8443/.
# Prerequisites: 1) Make sure python is installed and the executable path is set in your environment
#                2) please install pip ( python package installer ) from https://pip.pypa.io/en/stable/
#                3) please install paramiko using pip  "python -m pip install paramiko" for details please refer to http://www.paramiko.org/installing.html#paramiko-itself

from __future__ import print_function
import argparse
from remoteutility import *

# Checks service status immediately after deployment by searhcing for FATAL errors in logs. 
# This is not enough of an indication of server health, a seperate check server status script should be called at a later time after deployment.
def checkImmediateErrors(ssh, cfgFileName):
    ssh.execute("ssh -A " + Cfg().findProxyHost(cfgFileName))

    serviceName = Cfg().findServiceName(cfgFileName)
    logPath = "/opt/home/fifa/" + serviceName + "/log" 
    serviceHost = Cfg().findServiceHost(cfgFileName)
    ssh.execute("ssh -A " + serviceHost, printResult = False)
    if not ssh.isConnectionClosed():
        print("Connected to " + Cfg().findServiceHost(cfgFileName))
        ssh.execute("find " + logPath + " -type f -name *.log -exec grep -o 'FATAL' {} \; | wc -l", printResult = False)
        errors = ssh.getNumericOutput()
        if not errors == 0:
            print("There is a problem with the deployment. " + str(errors) + " FATAL errors found. \nPlease check logs in " + logPath + " at host " + serviceHost + " for details. \n")
            sys.exit(0)
        else: 
            print(serviceName + " and it's components have started up with " + str(errors) + " FATAL errors immediately after deployment.\nPlease run remotecheck.py after a few minutes to check server status. \n")
        ssh.execute("exit", printResult = False)
        #flush the output stream
        ssh.ignoreResult()
    else: 
         print("Connection to " + Cfg().findServiceHost(cfgFileName) + " unsuccessful.")
    ssh.execute("exit", printResult = False)
    #flush the output stream
    ssh.ignoreResult()
    
# execute the fabric command
def executeFab(ssh, cfgFileName, changelist, deployonly):
    ssh.execute("ssh -A " + Cfg().findProxyHost(cfgFileName))
    ssh.execute("rm archive/*.* ")

    env = Cfg().findEnv(cfgFileName)
    serviceName = Cfg().findServiceName(cfgFileName)
    branch = Cfg().findBranch()
    if deployonly:
        deployCommand = " deploy:restart=no,dbdestructive=yes" 
    else:
        deployCommand = " deploy:dbdestructive=yes" 
    command =  "/opt/gs/fab "+ env + ":" + serviceName + " pull:branch=" + branch + ",changelist=" + changelist + deployCommand
    ssh.execute(command)
    ssh.execute("exit", printResult = False)
    #flush the output stream
    ssh.ignoreResult()

def deploy(project, platform, environment, suffix, changelist, host, user, password, deployonly):
    if project.isPlatformSupported(platform):
        if project.isEnvironmentSupported(environment):
            ssh = Ssh(host, user, password)
            try:
                if not platform == "":
                    cfgFileName = project.findCfgFile(platform, environment, suffix)
                    executeFab(ssh, cfgFileName, changelist, deployonly)
                    if not deployonly:
                        checkImmediateErrors(ssh, cfgFileName)
                else:
                    cfgFiles = project.findCfgFiles(environment, suffix)
                    for cfgFileName in cfgFiles:
                        executeFab(ssh, cfgFileName, changelist, deployonly)
                    if not deployonly:
                        for cfgFileName in cfgFiles:
                            checkImmediateErrors(ssh, cfgFileName)
            except Exception as e:
                print(e)
                    
project = Project(os.path.abspath(__file__))

parser = argparse.ArgumentParser()
parser.add_argument("--platform", nargs='?', default="", help="Supported platforms: " + str(project.platforms).strip("[]") +". Deploy for all platforms if not specified")
parser.add_argument("--env", nargs='?', default="", help="Supported environments: " + str(project.supportedEnvironments()).strip("[]"))
parser.add_argument("--suffix", nargs='?', default="", help="Supported suffixes: " + str(project.supportedSuffixes()).strip("[]"))
parser.add_argument("changelist", help="p4 CL number")
parser.add_argument("user", help="username to login to https://se-pm.bio-iad.ea.com:8443/")
parser.add_argument("host", help="host (from slack channel, reserve timeslot at https://se-pm.bio-iad.ea.com:8443/)")
parser.add_argument("password", help="password in single or dobule brackets i.e. 'SOMEPASSWORD' (from slack channel, reserve timeslot at https://se-pm.bio-iad.ea.com:8443/)")
parser.add_argument("--deployonly", help="deploy only, do not stop or start the service", action="store_true")

args = parser.parse_args()

deploy(project, args.platform, args.env, args.suffix, args.changelist, args.host, args.user, args.password, args.deployonly)

