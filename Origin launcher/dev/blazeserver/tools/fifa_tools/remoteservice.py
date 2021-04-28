# 
# remotedeploy.py
#
# This is a helper script that would perform basic blazeserver operations(start/stop/restart/kill) on the Linux host   
# The script can be run on windows with user's desired configurations, username, specific host and password allocated by https://se-pm.bio-iad.ea.com:8443/.
# Prerequisites: 1) Make sure python is installed and the executable path is set in your environment
#                2) please install pip ( python package installer ) from https://pip.pypa.io/en/stable/
#                3) please install paramiko using pip  "python -m pip install paramiko" for details please refer to http://www.paramiko.org/installing.html#paramiko-itself

from __future__ import print_function
import argparse
from remoteutility import *

# execute the fabric command
def executeFab(ssh, cfgFileName, operation):
    ssh.execute("ssh -A " + Cfg().findProxyHost(cfgFileName))

    env = Cfg().findEnv(cfgFileName)
    serviceName = Cfg().findServiceName(cfgFileName)
    branch = Cfg().findBranch()
    
    command =  "/opt/gs/fab "+ env + ":" + serviceName + " servers." + operation
    if operation == "start" or operation == "restart":
        command = command + ":dbdestructive=yes"

    ssh.execute(command)
    ssh.execute("exit", printResult = False)
    #flush the output stream
    ssh.ignoreResult()

def action(project, platform, environment, suffix, host, user, password, operation):
    if operation != "start" and operation != "stop" and operation != "restart" and operation != "kill":
        print("Invalid operation: " + operation + " not supported")
    else: 
        if project.isPlatformSupported(platform):
            if project.isEnvironmentSupported(suffix):
                ssh = Ssh(host, user, password)
                try:
                    if not platform == "":
                        cfgFileName = project.findCfgFile(platform, environment, suffix)
                        executeFab(ssh, cfgFileName, operation)
                    else:
                        cfgFiles = project.findCfgFiles(environment, suffix)
                        for cfgFileName in cfgFiles:
                            executeFab(ssh, cfgFileName, operation)
                except Exception as e:
                    print(e)
                    
project = Project(os.path.abspath(__file__))

parser = argparse.ArgumentParser()
parser.add_argument("--platform", nargs='?', default="", help="Supported platforms: " + str(project.platforms).strip("[]") +". Deploy for all platforms if not specified")
parser.add_argument("--env", nargs='?', default="", help="Supported environments: " + str(project.supportedEnvironments()).strip("[]"))
parser.add_argument("--suffix", nargs='?', default="", help="Supported suffixes: " + str(project.supportedSuffixes()).strip("[]"))
parser.add_argument("user", help="username to login to https://se-pm.bio-iad.ea.com:8443/")
parser.add_argument("host", help="host (from slack channel, reserve timeslot at https://se-pm.bio-iad.ea.com:8443/)")
parser.add_argument("password", help="password in single or double brackets i.e. 'SOMEPASSWORD' (from slack channel, reserve timeslot at https://se-pm.bio-iad.ea.com:8443/)")
parser.add_argument("operation", help="start/stop/restart/kill")

args = parser.parse_args()

action(project, args.platform, args.env, args.suffix, args.host, args.user, args.password, args.operation)

