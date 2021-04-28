#!/usr/bin/python

import unitTest, sys

# In each test we are testing the echo command of the arson component.
# This is a simple RPC which returns copies the request directly into the response
# (they are the same TDF class)
# The purpose is to test the WAL decoder

# Parse command line ops (uniform for each test)
opts = unitTest.getOpts(sys.argv);
hostname = unitTest.getHostname(opts);
port = unitTest.getPort(opts);
redirect = unitTest.getRedirect(opts);
verbose = unitTest.getVerbose(opts);

# Create the test harness for the component
comp = unitTest.ComponentTest("arson",hostname,port,None,0,verbose);

## TEST 1 - plain int
cmd = unitTest.CommandTest("PlainInt","GET","echo?a=34");
cmd.addCondition("int",34);
comp.testCommand(cmd);
    
## TEST 2 - plain str
cmd = unitTest.CommandTest("PlainString","GET","echo?y=hello");
cmd.addCondition("string","hello");
comp.testCommand(cmd);
    
## TEST 3 - now both together!!!
cmd = unitTest.CommandTest("IntString","GET","echo?y=hello&a=-50");
cmd.addCondition("int",-50);
cmd.addCondition("string","hello");
comp.testCommand(cmd);
    
## TEST 4 - a bunch of ints
cmd = unitTest.CommandTest("FourInts","GET","echo?a=1&e=2&g=3&i=4");
cmd.addCondition("int",1);
cmd.addCondition("int2",2);
cmd.addCondition("int3",3);
cmd.addCondition("int4",4);
comp.testCommand(cmd);

## TEST 5 - list of ints
cmd = unitTest.CommandTest("IntList","GET","echo?f|0=3&f|1=6&f|2=9&f|3=12");
cmd.addCondition("intlist",3);
cmd.addCondition("intlist",6);
cmd.addCondition("intlist",9);
cmd.addCondition("intlist",12);
comp.testCommand(cmd);

## TEST 6 - list of strings
cmd = unitTest.CommandTest("StringList","GET","echo?h|0=milk&h|1=bread&h|2=eggs&h|3=coke");
cmd.addCondition("stringlist","milk");
cmd.addCondition("stringlist","bread");
cmd.addCondition("stringlist","eggs");
cmd.addCondition("stringlist","coke");
comp.testCommand(cmd);

## TEST 7 - int,int map
cmd = unitTest.CommandTest("IntIntMap","GET","echo?n|0=12&n|6=-56&n|-8=100");
cmd.addCondition("entry",'key="6">-56');
cmd.addCondition("entry",'key="0">12');
cmd.addCondition("entry",'key="-8">100');
comp.testCommand(cmd);

## TEST 8 - string,string map
cmd = unitTest.CommandTest("StrStrMap","GET","echo?p|foo=bar&p|grapes=fruit&p|tomato=vegetable");
cmd.addCondition("entry",'key="foo">bar');
cmd.addCondition("entry",'key="tomato">vegetable');
cmd.addCondition("entry",'key="grapes">fruit');
comp.testCommand(cmd);

## TEST 9 - struct
cmd = unitTest.CommandTest("Structs","GET","echo?c|fa=923");
cmd.addCondition("fooint",923);
comp.testCommand(cmd);

## TEST 10 - nested struct
cmd = unitTest.CommandTest("NestedStruct","GET","echo?b|ba=17&b|bb|fa=88");
cmd.addCondition("fooint",88);
cmd.addCondition("barint",17);
comp.testCommand(cmd);

## TEST 11 - list of structs
cmd = unitTest.CommandTest("ListOfStructs","GET","echo?d|0|ba=-34&d|0|bb|fa=25&d|1|ba=777&d|1|bb|fa=-4");
cmd.addCondition("bar",'<barint>-34</barint><barfoo><fooint>25</fooint></barfoo>');
cmd.addCondition("bar",'<barint>777</barint><barfoo><fooint>-4</fooint></barfoo>');
comp.testCommand(cmd);

## TEST 12 - int,struct map
cmd = unitTest.CommandTest("IntStructMap","GET","echo?l|12|ba=99&l|12|bb|fa=-88&l|-2|ba=100&l|-2|bb|fa=535");
cmd.addCondition("entry",'key="12"><bar><barint>99</barint><barfoo><fooint>-88</fooint></barfoo></bar>');
cmd.addCondition("entry",'key="-2"><bar><barint>100</barint><barfoo><fooint>535</fooint></barfoo></bar>');
comp.testCommand(cmd);

## TEST 13 - string,struct map
cmd = unitTest.CommandTest("StringStructMap","GET","echo?j|ketchup|ba=99&j|ketchup|bb|fa=-88&j|mustard|ba=100&j|mustard|bb|fa=535");
cmd.addCondition("entry",'key="mustard"><bar><barint>100</barint><barfoo><fooint>535</fooint></barfoo></bar>');
cmd.addCondition("entry",'key="ketchup"><bar><barint>99</barint><barfoo><fooint>-88</fooint></barfoo></bar>');
comp.testCommand(cmd);

## TEST 14 - unions
cmd = unitTest.CommandTest("Unions","GET","echo?r|mInt=45&t|mString=hello&v|mFoo|fa=-378&x|mBar|ba=56&x|mBar|bb|fa=1002");
cmd.addCondition("myunion",'member="0"><valu><fooint>-378</fooint></valu>');
cmd.addCondition("myunion",'member="1"><valu><barint>56</barint><barfoo><fooint>1002</fooint></barfoo></valu>');
cmd.addCondition("myunion",'member="2"><valu>45</valu>');
cmd.addCondition("myunion",'member="3"><valu>hello</valu>');
comp.testCommand(cmd);

# Get test summary
passed = comp.test();

# Exit 1 if failed; 0 if successful
if(passed == False):
    sys.exit(1);
    
sys.exit(0);
