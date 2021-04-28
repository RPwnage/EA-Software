import fnmatch
import os
import re
import sys
from optparse import OptionParser
import subprocess
import logging
        
pattern = '*_*.sql'
cmdpath = sys.argv[0].replace('dbmigall','dbmig')
cmd = 'python %s upgrade -c %s -d mysql -r %s -o %s --cpath %s'

parser = OptionParser()
parser.add_option('-r','--root',dest='root',
    metavar='PATH',help='PATH to root of trunk.')
parser.add_option('-o','--options',dest='dbopts',
    metavar='OPTIONS',help='OPTIONS specific to a database.')
(options, args) = parser.parse_args()

rpath = os.path.normpath(options.root) + os.sep

component_map = {}

for root, dirs, files in os.walk(rpath):
    if 'dbmig' in dirs:
        dirs.remove('dbmig')
    for filename in files:
        if fnmatch.fnmatch(filename, pattern):
            component = re.split('(_|\.)', filename)[2]
            if component not in component_map:
                path = os.path.join(root, filename)
                parent = os.sep.join(path.split(os.sep)[:-3])
                component_map[component] = parent.replace(rpath,'')

for key in component_map.keys():
    command = cmd % (cmdpath, key, rpath, options.dbopts, component_map[key])
    print 'Evaluating component %s for upgrade' % key
    p = subprocess.Popen(command, stdout=subprocess.PIPE)
    for line in p.stdout.readlines():
        logging.info(line)
    
