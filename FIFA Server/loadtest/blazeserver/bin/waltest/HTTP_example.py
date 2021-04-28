#!/usr/bin/python

# You can also use unitTest.py to test HTTP calls
# Just remember to:
# 1) Specify the port argument (-p PORT).  HTTP port is usually 12002.
# 2) set redirector to None when creating the ComponentTest
# 3) Always use GET or POST when creating a CommandTest

######### Startup #########

import unitTest, sys

# Parse command line ops (uniform for each test)
opts = unitTest.getOpts(sys.argv);
hostname = unitTest.getHostname(opts);
port = unitTest.getPort(opts);
redirect = unitTest.getRedirect(opts);
verbose = unitTest.getVerbose(opts);

# Create the test harness for the component
comp = unitTest.ComponentTest("example",hostname,port,None,0,verbose);

######### Run Tests #########

# Create a command test object for each RPC
cmd = unitTest.CommandTest("poke","GET","poke?num=12&text=hello");
passed, response = comp.testCommand(cmd);

######### Shutdown #########

# Get test summary
passed, response = comp.test();

# Exit 1 if failed; 0 if successful
if(passed == False):
    sys.exit(1);
    
sys.exit(0);