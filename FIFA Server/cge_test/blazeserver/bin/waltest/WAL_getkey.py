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

# Run expressLogin
cmd = unitTest.CommandTest("expressLogin","GET","/accounts/geoffreyh2@ea.com/login/sessionKey?pass=geoff");
passed, response = comp.testCommand(cmd);

# Extract the session key
if(passed == True):
    key, start = unitTest.parseXmlElement(response,"sessionkey");
    print "Your session key is " + key + ".";

# Get test summary
passed = comp.test();

# Exit 1 if failed; 0 if successful
if(passed == False):
    sys.exit(1);
    
sys.exit(0);