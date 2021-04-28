# 
# remotedeploy.py
#
# This is a helper script that would fetch blazeserver logs from the Linux host it is running on   
# The script can be run on windows with user's desired configurations, username, specific host and password allocated by https://se-pm.bio-iad.ea.com:8443/.
# Prerequisites: 1) Make sure python is installed and the executable path is set in your environment
#                2) please install pip ( python package installer ) from https://pip.pypa.io/en/stable/
#                3) please install paramiko using pip  "python -m pip install paramiko" for details please refer to http://www.paramiko.org/installing.html#paramiko-itself
#                4) please install scp using pip, "python -m pip install scp"

from __future__ import print_function
import argparse
from remoteutility import *
from scp import SCPClient

# Fetches logs from the host defined in the cfg file
def getLogsFromHost(ssh, user, cfgFileName, crash):
    ssh.execute("ssh -A " + Cfg().findProxyHost(cfgFileName))

    serviceName = Cfg().findServiceName(cfgFileName)
    if crash:
        zipFileName = serviceName + "-crash-logs.zip"
        logPath = "/opt/home/fifa/arc/" + serviceName
    else:
        zipFileName = serviceName + "-logs.zip"
        logPath = "/opt/home/fifa/" + serviceName + "/log" 
    serviceHost = Cfg().findServiceHost(cfgFileName)

    ssh.execute("scp -r " + user + "@" + serviceHost + ":" + logPath + " .")
    if crash:
        ssh.execute("zip -r " + zipFileName + " " + serviceName +"/*.gz")
        ssh.execute("rm -rf " + serviceName)
    else:
        ssh.execute("zip -r " + zipFileName + " log/*.log")
        ssh.execute("rm -rf log")
    ssh.execute("exit", printResult = False)
    #flush the output stream
    ssh.ignoreResult()

    ssh.execute("scp -r " + user + "@" + Cfg().findProxyHost(cfgFileName) + ":" + zipFileName +" .")

    ssh.execute("ssh -A " + Cfg().findProxyHost(cfgFileName))
    ssh.execute("rm " + zipFileName)
    ssh.execute("exit", printResult = False)
    #flush the output stream
    ssh.ignoreResult()

    scp = SCPClient(ssh.getTransport())
    scp.get(zipFileName)
    scp.close()
    ssh.execute("rm " + zipFileName)

def getLogs(project, platform, environment, suffix, host, user, password, crash):
    if project.isPlatformSupported(platform):
        if project.isEnvironmentSupported(suffix):
            ssh = Ssh(host, user, password)
            try:
                cfgFileName = project.findCfgFile(platform, environment, suffix)
                getLogsFromHost(ssh, user, cfgFileName, crash)
            except Exception as e:
                print(e)
                    
project = Project(os.path.abspath(__file__))

parser = argparse.ArgumentParser()
parser.add_argument("--env", nargs='?', default="", help="Supported environments: " + str(project.supportedEnvironments()).strip("[]"))
parser.add_argument("--suffix", nargs='?', default="", help="Supported suffixes: " + str(project.supportedSuffixes()).strip("[]"))
parser.add_argument("--crash", help="retrieve crash logs", action="store_true")
parser.add_argument("user", help="username to login to https://se-pm.bio-iad.ea.com:8443/")
parser.add_argument("host", help="host (from slack channel, reserve timeslot at https://se-pm.bio-iad.ea.com:8443/)")
parser.add_argument("password", help="password in single or double brackets i.e. 'SOMEPASSWORD' (from slack channel, reserve timeslot at https://se-pm.bio-iad.ea.com:8443/)")
parser.add_argument("platform", help="Supported platforms: " + str(project.platforms).strip("[]"))
args = parser.parse_args()

getLogs(project, args.platform, args.env, args.suffix, args.host, args.user, args.password, args.crash)

