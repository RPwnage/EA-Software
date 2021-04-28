---
title: Building Framework
weight: 33
---

This topic explains how to build framework.

<a name="Section1"></a>
## Building Framework ##

Building Framework is generally not necessary unless you are making changes to the Framework source code.
A solution file exists in the root of the Framework package and you can use this solution to edit and rebuild framework.

If you do make a change to the framework source code and want to build it, simply right click on the solution in visual studio and hit rebuild solution.
This will replace the default Framework binaries. If you wish to submit these binaries to source control make sure you build using the Release config.

Framework also has unit tests included in the solution. 
Please run these tests before and after you make changes to ensure that your changes have not broken any of the tests.
The easiest way to run the tests is to open the test explorer in visual studio (Test > Windows > Test Explorer).

The main development line of framework is in the dev-na-cm stream in the TnT/Build/Framework directory. 
If you make changes in this stream you do not need to rebuild framework, it will be rebuilt and new binaries will be submitted automatically.
If you make changes to framework in other streams you will need to rebuild.
Because development is currently happening in the frostbite tree changes will naturally be copied up and merged down, 
so compared to when development was happening outside of the stream, there is now less chance of up stream changes been stomped.
