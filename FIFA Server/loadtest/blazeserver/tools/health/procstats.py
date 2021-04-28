#!/bin/env python

import os
import sys
import time
from datetime import datetime
from optparse import OptionParser


# set up arguments
argparser = OptionParser()
argparser.add_option("-d","--delay",type="int",dest="delay", default=30, help="delay interval in seconds between calls to ps", metavar="DELAY")
argparser.add_option("-i","--iterations",type="int",dest="iterations", default=0, help="number of times to run ps, time of run = delay * iterations", metavar="COUNT")
argparser.add_option("-p","--pid",dest="pids", help="PIDs to scan, if none specified then checks all processes for the current session", metavar="PID1,PID2,...,PIDn")
argparser.add_option("-H","--threads",action="store_true",dest="viewthreads",default=False,help="Display information for all threads in processes.")

(options, args) = argparser.parse_args()

if options.pids != None:
    pids = options.pids.split(',')
else:
    pids = None
    
columns = []


# main loop

# extract column names first - if already defined then ignore
# extract useful information from subsequent lines until hitting the end.
iterations = options.iterations
if iterations == 0:
    iterations = 10000000
    
#ps options
# H - Show threads as if they were processes
# L - Show threads, possibly with LWP and NLWP columns
# p - Select by PID
# u - only by my user id
psopts = ''
if options.viewthreads == True:  
    psopts = psopts + ' H -L'
    
while iterations > 0:
    curtime = datetime.now()
    if pids:
        tmp = os.popen('ps u' + psopts + ' -p' + ','.join(pids)).read()
    else:
        tmp = os.popen('ps u H -L').read()
        
    pslines = tmp.splitlines()
   
    for lineIndex in range(len(pslines)):
        items = pslines[lineIndex].split()
        
        if lineIndex == 0:
            if columns == []:
                columns = items
                columns.insert(0, 'DATETIME')
                print ','.join(columns)
        else:
            items.insert(0, time.strftime('%m/%d/%Y %H:%M:%S', time.localtime()))
            print ','.join(items)

    # flush the print buffer so we can tail the output.
    sys.stdout.flush()
                    
    time.sleep(options.delay)
    iterations = iterations - 1
