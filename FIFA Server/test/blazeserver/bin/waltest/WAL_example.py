#!/usr/bin/python

# This test demonstrates how to write a unit test using the unitTest.py framework 
# See TODO's to see what you need to change

######### Startup #########

import unitTest, sys

# Parse command line ops (uniform for each test)
opts = unitTest.getOpts(sys.argv);
hostname = unitTest.getHostname(opts);
port = unitTest.getPort(opts);
redirect = unitTest.getRedirect(opts);
verbose = unitTest.getVerbose(opts);

# Create the test harness for the component
## TODO: Replace "example" with the component you wish to test.
comp = unitTest.ComponentTest("example",hostname,port,redirect,0,verbose);

######### Run Tests #########

# Create a command test object for each RPC
## TODO: configure the parameters to this constructor
#   (1) test name (arbitrary)
#   (2) http method (GET, PUT, POST, or DELETE)
#   (3) path, including any query parameters
cmd = unitTest.CommandTest("poke","GET","/poke?num=12&text=hello");

# Attach the command to the component and test
passed, response = comp.testCommand(cmd);

# Optionally do something with the passed boolean and the XML response between tests
# Eg. Parse the session key from the expressLogin response,
# Parse the personaID from the createPersona response if the command succeeded.

## TODO: Add more commands to the test

######### Shutdown #########

# Get test summary ("passed" will equal False if any tests failed, True if all succeeded.)
passed = comp.test();

# Exit 1 if failed; 0 if successful
if(passed == False):
    sys.exit(1);
    
sys.exit(0);