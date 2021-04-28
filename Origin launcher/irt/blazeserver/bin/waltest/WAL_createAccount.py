#!/usr/bin/python

import unitTest, sys

# Parse command line ops (uniform for each test)
opts = unitTest.getOpts(sys.argv);
hostname = unitTest.getHostname(opts);
port = unitTest.getPort(opts);
redirect = unitTest.getRedirect(opts);
verbose = unitTest.getVerbose(opts);

# Create the test harness for the authentication component
comp = unitTest.ComponentTest("authentication",hostname,port,redirect,0,verbose);

# Run createAccount
cmd = unitTest.CommandTest("createAccount","POST","/accounts/geoffreyh2@ea.com?pass=geoff&bday=28&bmon=9&byr=1985&ctry=us&lang=en&tosv=1");
passed, response = comp.testCommand(cmd);

# Get test summary
passed = comp.test();

# Exit 1 if failed; 0 if successful
if(passed == False):
    sys.exit(1);
    
sys.exit(0);