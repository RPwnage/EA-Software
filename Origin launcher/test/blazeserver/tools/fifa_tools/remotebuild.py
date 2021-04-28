# 
# remotebuild.py
#
# This is a helper script that would build, package and post specific configs of a blazeserver on the Linux host   
# The script can be run on windows with user's desired configurations, username and RSA token.
# Note: It will always build the head of a specific branch
# Prerequisites: 1) Make sure python is installed and the executable path is set in your environment
#                2) please install pip ( python package installer ) from https://pip.pypa.io/en/stable/
#                3) please install paramiko using pip  "python -m pip install paramiko" for details please refer to http://www.paramiko.org/installing.html#paramiko-itself

from __future__ import print_function
import argparse
from remoteutility import *

def getChangelist(string):
        index = string.find(" ")
        string = string[index+1:]
        index = string.find(" ")
        return string[:index]

def build(project, environment, suffix, changelist, username, password, clean):
    branch = project.branch
    title = project.title
    gen = project.gen
    year = project.year
    if project.isEnvironmentSupported(environment):
        ssh = Ssh(host, username, password)
        try:
            ssh.execute("sudo -H -u fifa bash")
            ssh.execute("cd /opt/home/fifa/p4/gosdev/games/" + title + "/" + year + "/" + gen + "/" + branch + "/")
            if not changelist == "":
                ssh.execute("p4 sync //gosdev/games/" + title + "/" + year + "/" + gen + "/" + branch + "/...@" + changelist)
            else:
                ssh.execute("p4 sync //gosdev/games/" + title + "/" + year + "/" + gen + "/" + branch + "/...")
            ssh.execute("cd blazeserver")
            if changelist == "":
                ssh.execute("p4 changes -m1 //gosdev/games/" + title +"/" + year + "/" + gen+ "/" + branch + "/...", printResult = False)
                changelist = getChangelist(ssh.getStringOutput())
            cleanString = ""
            if clean:
                cleanString = " --clean"
            ssh.execute("sudo /opt/gs/blazepkg build" + cleanString )
            ssh.execute("sudo /opt/gs/blazepkg pack")
            ssh.execute("sudo /opt/gs/blazepkg post --changelist " + changelist )
            cfgFiles = project.findCfgFiles(environment, suffix)
            for cfgFile in cfgFiles:
                ssh.execute("sudo /opt/gs/blazepkg post --serverfiles bin/" + cfgFile)
            print("Packaged CL " + changelist)
        except Exception as e:
            print(e)
                
project = Project(os.path.abspath(__file__))

parser = argparse.ArgumentParser()
parser.add_argument("--env", nargs='?', default="", help="Supported environments: " + str(project.supportedEnvironments()).strip("[]"))
parser.add_argument("--suffix", nargs='?', default="", help="Supported suffixes: " + str(project.supportedSuffixes()).strip("[]"))
parser.add_argument("--changelist", nargs='?', default="", help="p4 changelist to build with. Default is to build with head.")
parser.add_argument("user", help="username")
parser.add_argument("rsa", help="RSA token (8-digit code)")
parser.add_argument("--clean", help="clean build", action="store_true")
args = parser.parse_args()

build(project, args.env, args.suffix, args.changelist, args.user, args.rsa, args.clean)
