---
title: Target Functions
weight: 1018
---
## Summary ##
Collection of target manipulation routines.


## Overview ##
| Function | Summary |
| -------- | ------- |
| [String TargetExists(String targetName)]({{< relref "#string-targetexists-string-targetname" >}}) | Check if the specified target exists. |
## Detailed overview ##
#### String TargetExists(String targetName) ####
##### Summary #####
Check if the specified target exists.

###### Parameter:  project ######


###### Parameter:  targetName ######
The target name to check.

###### Return Value: ######
True or False.

###### Example ######

```xml

<project xmlns="schemas/ea/framework3.xsd">
    <target name='build' />
    <fail unless="@{TargetExists('build')}" />
</project>

```





